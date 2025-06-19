//
//  DRStMailboxes.c
//  thinstream
//
//  Created by Marcos Ortega on 1/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSServerLogicClt.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBArraySorted.h"
#include "nb/net/NBHttpRequest.h"
#include "nb/net/NBHttpClient.h"
#include "nb/net/NBSocket.h"
//
#include "core/logic/TSDevice.h"

typedef struct STTSServerLogicCltOpq_ {
	BOOL				isConfigured;
	STTSCfgServer		cfg;
	STTSContext*	context;
	STNBPKey*			caKey;
	STNBString			storageRoot;
	STNBString			meetingsRoot;
	STNBString			remindersRoot;
	//
	STTSServerStats*	stats;
	STNBHttpStatsRef	channelsStats;
	//Itf
	struct {
		PtrAddMailToLocalQueueFunc	funcSendMail;
		void*						funcSendMailParam;
	} itf;
	//Thread
	struct {
		STNBThreadMutex	mutex;
		STNBThreadCond	cond;
		BOOL			stopFlag;
		UI32			isRunning;
	} thread;
} STTSServerLogicCltOpq;

void TSServerLogicClt_init(STTSServerLogicClt* obj){
	STTSServerLogicCltOpq* opq = obj->opaque = NBMemory_allocType(STTSServerLogicCltOpq);
	NBMemory_setZeroSt(*opq, STTSServerLogicCltOpq);
	//
	opq->isConfigured	= FALSE;
	NBMemory_setZero(opq->cfg);
	opq->context		= NULL;
	opq->caKey			= NULL;
	NBString_init(&opq->storageRoot);
	NBString_init(&opq->meetingsRoot);
	NBString_init(&opq->remindersRoot);
	//
	opq->stats			= NULL;
	//Itf
	{
		opq->itf.funcSendMail		= NULL;
		opq->itf.funcSendMailParam	= NULL;
	}
	//Jobs
	{
		NBThreadMutex_init(&opq->thread.mutex);
		NBThreadCond_init(&opq->thread.cond);
		opq->thread.stopFlag	= FALSE;
		opq->thread.isRunning	= FALSE;
	}
}

void TSServerLogicClt_stopLockedOpq(STTSServerLogicCltOpq* opq);

void TSServerLogicClt_release(STTSServerLogicClt* obj){
	STTSServerLogicCltOpq* opq = (STTSServerLogicCltOpq*)obj->opaque;
	//Wait for jobs
	{
		NBThreadMutex_lock(&opq->thread.mutex);
		TSServerLogicClt_stopLockedOpq(opq);
		NBThreadMutex_unlock(&opq->thread.mutex);
		NBThreadCond_release(&opq->thread.cond);
		NBThreadMutex_release(&opq->thread.mutex);
		opq->thread.isRunning	= FALSE;
		opq->thread.stopFlag	= FALSE;
	}
	//
	opq->stats			= NULL;
	if(NBHttpStats_isSet(opq->channelsStats)){
		NBHttpStats_release(&opq->channelsStats);
		NBHttpStats_null(&opq->channelsStats);
	}
	//
	opq->context	= NULL;
	opq->caKey		= NULL;
	NBString_release(&opq->storageRoot);
	NBString_release(&opq->meetingsRoot);
	NBString_release(&opq->remindersRoot);
	//
	NBStruct_stRelease(TSCfgServer_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
	NBMemory_free(obj->opaque);
	obj->opaque = NULL;
}

BOOL TSServerLogicClt_loadFromConfig(STTSServerLogicClt* obj, const STTSCfgServer* cfg, STTSContext* context, STNBPKey* caKey, PtrAddMailToLocalQueueFunc funcSendMail, void* funcSendMailParam, STTSServerStats* stats, STNBHttpStatsRef channelsStats){
	BOOL r = FALSE;
	STTSServerLogicCltOpq* opq = (STTSServerLogicCltOpq*)obj->opaque;
	if(cfg != NULL && !opq->isConfigured){
		r = TRUE;
		//
		if(context == NULL){
			PRINTF_CONSOLE_ERROR("TSServerLogicClt_loadFromConfig, context is required.\n");
		}
		//
		if(r){
			NBStruct_stRelease(TSCfgServer_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
			NBStruct_stClone(TSCfgServer_getSharedStructMap(), cfg, sizeof(*cfg), &opq->cfg, sizeof(opq->cfg));
			//
			opq->context	= context;
			opq->caKey		= caKey;
			//
			opq->stats			= stats;
			opq->channelsStats	= channelsStats;
			//Itf
			{
				opq->itf.funcSendMail		= funcSendMail;
				opq->itf.funcSendMailParam	= funcSendMailParam;
			}
			{
				const char* storageRoot = TSContext_getRootRelative(context);
				NBString_set(&opq->storageRoot, storageRoot);
				{
					NBString_set(&opq->meetingsRoot, storageRoot);
					if(opq->meetingsRoot.length > 0){
						if(opq->meetingsRoot.str[opq->meetingsRoot.length - 1] != '/'){
							NBString_concatByte(&opq->meetingsRoot, '/');
						}
					}
					NBString_concat(&opq->meetingsRoot, "meetings");
				}
				{
					NBString_set(&opq->remindersRoot, storageRoot);
					if(opq->remindersRoot.length > 0){
						if(opq->remindersRoot.str[opq->remindersRoot.length - 1] != '/'){
							NBString_concatByte(&opq->remindersRoot, '/');
						}
					}
					NBString_concat(&opq->remindersRoot, "identities");
				}
			}
			//
			opq->isConfigured = TRUE;
		}
	}
	return r;
}

void TSServerLogicClt_stopFlag(STTSServerLogicClt* obj){
	STTSServerLogicCltOpq* opq = (STTSServerLogicCltOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->thread.mutex);
	{
		opq->thread.stopFlag = TRUE;
		NBThreadCond_broadcast(&opq->thread.cond);
	}
	NBThreadMutex_unlock(&opq->thread.mutex);
}

void TSServerLogicClt_stop(STTSServerLogicClt* obj){
	STTSServerLogicCltOpq* opq = (STTSServerLogicCltOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->thread.mutex);
	TSServerLogicClt_stopLockedOpq(opq);
	NBThreadMutex_unlock(&opq->thread.mutex);
}

void TSServerLogicClt_stopLockedOpq(STTSServerLogicCltOpq* opq){
	while(opq->thread.isRunning){
		{
			opq->thread.stopFlag = TRUE;
			NBThreadCond_broadcast(&opq->thread.cond);
		}
		NBThreadCond_wait(&opq->thread.cond, &opq->thread.mutex);
	}
}
	
//Strings list

typedef struct STTSServerLogicCltStr_ {
	char*				value;
	char*				lang;
} STTSServerLogicCltStr;

void TSServerLogicClt_listEmpty_(STNBArray* list){
	if(list != NULL){
		SI32 i; for(i = 0; i < list->use; i++){
			STTSServerLogicCltStr* val = NBArray_itmPtrAtIndex(list, STTSServerLogicCltStr, i);
			if(val->value != NULL) NBMemory_free(val->value); val->value = NULL;
			if(val->lang != NULL) NBMemory_free(val->lang); val->lang = NULL;
		}
		NBArray_empty(list);
	}
}

void TSServerLogicClt_listAdd_(STNBArray* list, const char* pValue, const char* pLang){
	if(list != NULL && pValue != NULL && pLang != NULL){
		SI32 i; for(i = 0; i < list->use; i++){
			STTSServerLogicCltStr* val = NBArray_itmPtrAtIndex(list, STTSServerLogicCltStr, i);
			if(val->value != NULL){
				if(NBString_strIsLike(val->value, pValue)){
					break;
				}
			}
		}
		//Add new
		if(i == list->use){
			STTSServerLogicCltStr val;
			NBMemory_setZeroSt(val, STTSServerLogicCltStr);
			val.value	= NBString_strNewBuffer(pValue);
			val.lang	= NBString_strNewBuffer(pLang);
			NBArray_addValue(list, val);
		}
	}
}

//Encrypted list

typedef struct TSServerLogicCltEncData_ {
	STTSCypherDataKey	encKey;
	STTSCypherData		encData;
	char*				lang;
} TSServerLogicCltEncData;

void TSServerLogicClt_encListEmpty_(STNBArray* list){
	if(list != NULL){
		const STNBStructMap* stKey = TSCypherDataKey_getSharedStructMap();
		const STNBStructMap* stData = TSCypherData_getSharedStructMap();
		SI32 i; for(i = 0; i < list->use; i++){
			TSServerLogicCltEncData* enc = NBArray_itmPtrAtIndex(list, TSServerLogicCltEncData, i);
			NBStruct_stRelease(stKey, &enc->encKey, sizeof(enc->encKey));
			NBStruct_stRelease(stData, &enc->encData, sizeof(enc->encData));
			if(enc->lang != NULL) NBMemory_free(enc->lang); enc->lang = NULL;
		}
		NBArray_empty(list);
	}
}

void TSServerLogicClt_encListAdd_(STNBArray* list, STTSCypherDataKey* encKey, STTSCypherData* encData, const char* lang){
	if(list != NULL && encKey != NULL && encData != NULL && lang != NULL){
		if(!TSCypherDataKey_isEmpty(encKey) && encData->data != NULL && encData->dataSz > 0){
			//Add new allways
			TSServerLogicCltEncData enc;
			NBMemory_setZero(enc);
			NBStruct_stClone(TSCypherDataKey_getSharedStructMap(), encKey, sizeof(*encKey), &enc.encKey, sizeof(enc.encKey));
			NBStruct_stClone(TSCypherData_getSharedStructMap(), encData, sizeof(*encData), &enc.encData, sizeof(enc.encData));
			enc.lang = NBString_strNewBuffer(lang);
			NBArray_addValue(list, enc);
		}
	}
}

//

BOOL NBCompare_STNBHttpStatsReqByCount(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz){
	NBASSERT(dataSz == sizeof(STNBHttpStatsReq))
	if(dataSz == sizeof(STNBHttpStatsReq)){
		const STNBHttpStatsReq* o1 = (const STNBHttpStatsReq*)data1;
		const STNBHttpStatsReq* o2 = (const STNBHttpStatsReq*)data2;
		switch (mode) {
			case ENCompareMode_Equal:
				return o1->count == o2->count ? TRUE : FALSE;
			case ENCompareMode_Lower:
				return o1->count < o2->count ? TRUE : FALSE;
			case ENCompareMode_LowerOrEqual:
				return o1->count <= o2->count ? TRUE : FALSE;
			case ENCompareMode_Greater:
				return o1->count > o2->count ? TRUE : FALSE;
			case ENCompareMode_GreaterOrEqual:
				return o1->count >= o2->count ? TRUE : FALSE;
			default:
				NBASSERT(FALSE)
				break;
		}
	}
	return FALSE;
}

BOOL NBCompare_STNBHttpStatsRespByCount(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz){
	NBASSERT(dataSz == sizeof(STNBHttpStatsResp))
	if(dataSz == sizeof(STNBHttpStatsResp)){
		const STNBHttpStatsResp* o1 = (const STNBHttpStatsResp*)data1;
		const STNBHttpStatsResp* o2 = (const STNBHttpStatsResp*)data2;
		switch (mode) {
			case ENCompareMode_Equal:
				return o1->count == o2->count ? TRUE : FALSE;
			case ENCompareMode_Lower:
				return o1->count < o2->count ? TRUE : FALSE;
			case ENCompareMode_LowerOrEqual:
				return o1->count <= o2->count ? TRUE : FALSE;
			case ENCompareMode_Greater:
				return o1->count > o2->count ? TRUE : FALSE;
			case ENCompareMode_GreaterOrEqual:
				return o1->count >= o2->count ? TRUE : FALSE;
			default:
				NBASSERT(FALSE)
				break;
		}
	}
	return FALSE;
}

//

SI64 TSServerLogicClt_sendMailsMethd_(STNBThread* t, void* param){
	SI64 r = 0;
	//
	if(param != NULL){
		STTSServerLogicCltOpq* opq = (STTSServerLogicCltOpq*)param;
		NBASSERT(opq->thread.isRunning)
		{
			{
				NBThreadMutex_lock(&opq->thread.mutex);
				NBASSERT(opq->thread.isRunning)
				opq->thread.isRunning = TRUE;
				NBThreadMutex_unlock(&opq->thread.mutex);
			}
			//Run
			{
				UI32 msAccum = 0;
				const UI32 msSleep = 50;
				STNBString str;
				STNBArray found, identities, emails, emailsEnc;
				NBString_init(&str);
				NBArray_init(&found, sizeof(STNBFilesystemFile), NULL);
				NBArray_init(&identities, sizeof(STTSServerLogicCltStr), NULL);
				NBArray_init(&emails, sizeof(STTSServerLogicCltStr), NULL);
				NBArray_init(&emailsEnc, sizeof(TSServerLogicCltEncData), NULL);
				while(!opq->thread.stopFlag){
					//Sleep
					{
						NBThread_mSleep(msSleep);
						msAccum += msSleep;
					}
					//Trigger a second
					if(!opq->thread.stopFlag && msAccum > 1000){
						msAccum = 0;
						//Stats
						{
							BOOL save = FALSE, send = FALSE;
							const UI64 curTime = NBDatetime_getCurUTCTimestamp();
							NBASSERT(opq->stats->timeStart > 0)
							if(curTime < opq->stats->timeStart || curTime < opq->stats->timeEnd){
								save = send = TRUE;
								PRINTF_WARNING("SystemReport, timeStart or timeEnd not valid.\n");
							} else {
								const UI64 secsSinceStart	= (curTime - opq->stats->timeStart);
								const UI64 secsSinceEnd		= (curTime - opq->stats->timeEnd);
								if(opq->cfg.systemReports.secsToSave > 0 && opq->cfg.systemReports.secsToSave <= secsSinceEnd){
									save = TRUE;
								}
								if(opq->cfg.systemReports.minsToNext > 0 && (opq->cfg.systemReports.minsToNext * 60) <= secsSinceStart){
									send = TRUE;
								}
								//PRINTF_INFO("SystemReport, secsSinceStart(%llu) secsSinceEnd(%llu).\n", secsSinceStart, secsSinceEnd);
							}
							if(send || save){
								opq->stats->timeEnd = curTime;
								NBHttpStats_getData(opq->channelsStats, &opq->stats->stats, (send ? TRUE : FALSE));
								NBASSERT(opq->stats->timeStart > 0)
								if(send){
									//Build and send emails
									if(opq->cfg.systemReports.emailsSz > 0){
										const STNBDatetime time0 = NBDatetime_getUTCFromTimestamp(opq->stats->timeStart);
										const STNBDatetime time1 = NBDatetime_getUTCFromTimestamp(opq->stats->timeEnd);
										STNBString body, json, filename;
										NBString_init(&body);
										NBString_init(&json);
										NBString_init(&filename);
										NBStruct_stConcatAsJson(&json, TSServerStats_getSharedStructMap(), opq->stats, sizeof(*opq->stats));
										//Build filename
										{
											NBString_concatDateTimeCompact(&filename, time0);
											NBString_concatByte(&filename, '_');
											NBString_concatDateTimeCompact(&filename, time1);
											NBString_concat(&filename, ".json");
										}
										//Build email body
										{
											NBString_concat(&body, "<b>Dental Robot</b> server:<br>\n");
											NBString_concat(&body, "<br>\n");
											NBString_concat(&body, "Since: "); NBString_concatSqlDatetime(&body, time0); NBString_concat(&body, "<br>\n");
											NBString_concat(&body, "Until: "); NBString_concatSqlDatetime(&body, time1); NBString_concat(&body, "<br>\n");
											NBString_concat(&body, "Duration: <b>"); TSContext_concatDurationFromSecs(opq->context, &body, (opq->stats->timeEnd - opq->stats->timeStart), ENTSDurationPart_Second, "en"); NBString_concat(&body, "</b>.<br>\n");
											NBString_concat(&body, "<br>\n");
											//Starts and shutdowns
											if(opq->stats->starts != 0 || opq->stats->shutdowns != 0){
												NBString_concat(&body, "Service starts: "); NBString_concatSI32(&body, opq->stats->starts); NBString_concat(&body, "<br>\n");
												NBString_concat(&body, "Service shutdowns: "); NBString_concatSI32(&body, opq->stats->shutdowns); NBString_concat(&body, "<br>\n");
												NBString_concat(&body, "Balance: <b>"); NBString_concatSI32(&body, opq->stats->starts - opq->stats->shutdowns); NBString_concat(&body, "</b><br>\n");
												NBString_concat(&body, "<br>\n");
											}
											//Http Service
											{
												const STNBHttpStatsData* pay = &opq->stats->stats;
												//Traffic
												{
													NBString_concat(&body, "Connections: "); NBString_concatUI64(&body, pay->flow.connsIn);
													if(pay->flow.connsRejects > 0){
														NBString_concat(&body, " ("); NBString_concatUI64(&body, pay->flow.connsRejects); NBString_concat(&body, " rejected)");
													}
													NBString_concat(&body, "<br>\n");
												}
												NBString_concat(&body, "Requests: "); NBString_concatUI64(&body, pay->flow.requests); NBString_concat(&body, "<br>\n");
												if(pay->flow.bytes.in > 0 || pay->flow.bytes.out > 0){
													NBString_concat(&body, "Traffic: ");
													{
														const UI64 val = pay->flow.bytes.in;
														if(val > (1024 * 1024 * 1024)){
															const float sz = (float)val / (float)(1024 * 1024 * 1024);
															NBString_concatFloat(&body, sz, 1);
															NBString_concat(&body, " GBs");
														} else if(val > (1024 * 1024)){
															const float sz = (float)val / (float)(1024 * 1024);
															NBString_concatFloat(&body, sz, 1);
															NBString_concat(&body, " MBs");
														} else if(val > (1024)){
															NBString_concatSI32(&body, (SI32)val / 1024);
															NBString_concat(&body, " KBs");
														} else {
															NBString_concatSI32(&body, (SI32)val);
															NBString_concat(&body, " bytes");
														}
													}
													NBString_concat(&body, " in / ");
													{
														const UI64 val = pay->flow.bytes.out;
														if(val > (1024 * 1024 * 1024)){
															const float sz = (float)val / (float)(1024 * 1024 * 1024);
															NBString_concatFloat(&body, sz, 1);
															NBString_concat(&body, " GBs");
														} else if(val > (1024 * 1024)){
															const float sz = (float)val / (float)(1024 * 1024);
															NBString_concatFloat(&body, sz, 1);
															NBString_concat(&body, " MBs");
														} else if(val > (1024)){
															NBString_concatSI32(&body, (SI32)val / 1024);
															NBString_concat(&body, " KBs");
														} else {
															NBString_concatSI32(&body, (SI32)val);
															NBString_concat(&body, " bytes");
														}
													}
													NBString_concat(&body, " out <br>\n");
												}
												NBString_concat(&body, "<br>\n");
												//Requests
												if(pay->reqs != NULL && pay->reqsSz > 0){
													NBString_concat(&body, "Requests details: ");
													NBString_concat(&body, "<br>\n");
													{
														STNBArraySorted arr, arr2;
														NBArraySorted_init(&arr, sizeof(STNBHttpStatsReq), NBCompare_STNBHttpStatsReqByCount);
														NBArraySorted_init(&arr2, sizeof(STNBHttpStatsResp), NBCompare_STNBHttpStatsRespByCount);
														{
															SI32 i;
															for(i = 0; i < pay->reqsSz; i++){
																const STNBHttpStatsReq* req = &pay->reqs[i];
																NBArraySorted_addValue(&arr, *req);
															}
															for(i = (arr.use - 1); i >= 0; i--){
																const STNBHttpStatsReq* req = NBArraySorted_itmPtrAtIndex(&arr, STNBHttpStatsReq, i);
																BOOL colorOpened = FALSE;
																/*{
																	NBString_concat(&body, "<font color=\"#0000FF\">");
																	colorOpened = TRUE;
																}*/
																NBString_concat(&body, "(x");
																NBString_concatUI64(&body, req->count);
																NBString_concat(&body, ") = ");
																NBString_concat(&body, req->target);
																NBString_concat(&body, "<br>\n");
																if(req->resps != NULL && req->respsSz > 0){
																	NBString_concat(&body, "<font color=\"#999999\">");
																	NBArraySorted_empty(&arr2);
																	{
																		SI32 i; for(i = 0; i < req->respsSz; i++){
																			const STNBHttpStatsResp* resp = &req->resps[i];
																			NBArraySorted_addValue(&arr2, *resp);
																		}
																		for(i = (arr2.use - 1); i >= 0; i--){
																			const STNBHttpStatsResp* resp = NBArraySorted_itmPtrAtIndex(&arr2, STNBHttpStatsResp, i);
																			BOOL colorOpened = FALSE;
																			if(resp->code < 200 || resp->code >= 300){
																				NBString_concat(&body, "<font color=\"#FF0000\">");
																				colorOpened = TRUE;
																			}
																			NBString_concat(&body, "&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(x");
																			NBString_concatUI64(&body, resp->count);
																			NBString_concat(&body, ") = ");
																			NBString_concatSI32(&body, resp->code);
																			NBString_concat(&body, " '");
																			NBString_concat(&body, resp->reason);
																			NBString_concat(&body, "'<br>\n");
																			if(colorOpened){
																				NBString_concat(&body, "</font>");
																				colorOpened = FALSE;
																			}
																		}
																	}
																	NBString_concat(&body, "</font>");
																}
																if(colorOpened){
																	NBString_concat(&body, "</font>");
																	colorOpened = FALSE;
																}
															}
														}
														NBArraySorted_release(&arr2);
														NBArraySorted_release(&arr);
													}
													NBString_concat(&body, "<br>\n");
												}
												NBString_concat(&body, "--END-OF-REPORT--<br>\n");
											}
										}
										//Send
										{
											SI32 i; for(i = 0; i < opq->cfg.systemReports.emailsSz; i++){
												const char* email = opq->cfg.systemReports.emails[i];
												if(!NBString_strIsEmpty(email)){
													if(!(*opq->itf.funcSendMail)(opq->itf.funcSendMailParam, email, "System report", body.str, filename.str, "application/json", json.str, json.length)){
														PRINTF_CONSOLE_ERROR("Email could not be queued.\n");
													} else {
														//PRINTF_INFO("Email queued.\n");
													}
												}
											}
										}
										NBString_release(&body);
										NBString_release(&json);
										NBString_release(&filename);
									}
									//Save and reset time
									{
										if(!TSServerStats_saveAndOpenNewStats(opq->context, opq->stats)){
											PRINTF_CONSOLE_ERROR("TSServerStats_saveAndOpenNewStats failed.\n");
										} else {
											//PRINTF_INFO("TSServerStats_saveAndOpenNewStats success.\n");
										}
										opq->stats->timeStart = curTime;
									}
									NBASSERT(opq->stats->timeStart > 0)
								} else if(save){
									NBASSERT(opq->stats->timeStart > 0)
									if(!TSServerStats_saveCurrentStats(opq->context, opq->stats)){
										PRINTF_CONSOLE_ERROR("TSServerStats_saveCurrentStats failed.\n");
									} else {
										//PRINTF_INFO("TSServerStats_saveCurrentStats success.\n");
									}
									NBASSERT(opq->stats->timeStart > 0)
								}
							}
						}
					}
				}
				TSServerLogicClt_listEmpty_(&identities);
				TSServerLogicClt_listEmpty_(&emails);
				TSServerLogicClt_encListEmpty_(&emailsEnc);
				NBArray_release(&identities);
				NBArray_release(&emails);
				NBArray_release(&emailsEnc);
				NBString_release(&str);
				NBArray_release(&found);
			}
			{
				NBThreadMutex_lock(&opq->thread.mutex);
				NBASSERT(opq->thread.isRunning)
				opq->thread.isRunning = FALSE;
				NBThreadCond_broadcast(&opq->thread.cond);
				NBThreadMutex_unlock(&opq->thread.mutex);
			}
			
		}
	}
	//Release
	if(t != NULL){
		NBThread_release(t);
		NBMemory_free(t);
		t = NULL;
	}
	return r;
}

BOOL TSServerLogicClt_prepare(STTSServerLogicClt* obj){
	BOOL r = FALSE;
	STTSServerLogicCltOpq* opq = (STTSServerLogicCltOpq*)obj->opaque;
	{
		NBThreadMutex_lock(&opq->thread.mutex);
		if(opq->thread.isRunning){
			PRINTF_CONSOLE_ERROR("Could not start TSServerLogicCltThread, already running.\n");
		} else if(!opq->isConfigured){
			PRINTF_CONSOLE_ERROR("Could not start TSServerLogicCltThread, not configured yet.\n");
		} else {
			r = TRUE;
			//Start send thread
			if(r){
				STNBThread* t = NBMemory_allocType(STNBThread);
				NBThread_init(t);
				NBThread_setIsJoinable(t, FALSE);
				opq->thread.isRunning = TRUE;
				if(!NBThread_start(t, TSServerLogicClt_sendMailsMethd_, opq, NULL)){
					PRINTF_CONSOLE_ERROR("Could not start TSServerLogicClt_sendMailsMethd_.\n");
					opq->thread.isRunning = FALSE;
					r = FALSE;
				} else {
					//PRINTF_INFO("TSServerLogicClt_sendMailsMethd_ started.\n");
					t = NULL;
				}
				//Release (if not consumed)
				if(t){
					NBThread_release(t);
					NBMemory_free(t);
					t = NULL;
				}
			}
		}
		NBThreadMutex_unlock(&opq->thread.mutex);
	}
	return r;
}
