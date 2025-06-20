//
//  TSClient.c
//  thinstream
//
//  Created by Marcos Ortega on 8/5/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/client/TSClient.h"
#include "core/client/TSClientOpq.h"
#include "core/logic/TSClientRoot.h"
#include "core/logic/TSDeviceLocal.h"
#include "core/logic/TSCertificate.h"
#include "core/logic/TSStreamsStorage.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBThread.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBString.h"
#include "nb/core/NBArray.h"
#include "nb/compress/NBZlib.h"
#include "nb/crypto/NBMd5.h"
#include "nb/crypto/NBPkcs12.h"
#include "nb/crypto/NBAes256.h"
#include "core/TSRequestsMap.h"
//
#include <stdlib.h>	//for rand()

//Stats
STTSClientStatsData TSClientStats_getDataLockedOpq_(STTSClientOpq* opq, STNBArray* dstPortsStats /*STNBRtpClientPortStatsData*/, STNBArray* dstSsrcsStats /*STNBRtpClientSsrcStatsData*/, const BOOL resetAccum);
void TSClientStats_release_(STTSClientStatsData* obj);

//

void TSClient_init(STTSClient* obj){
	STTSClientOpq* opq		= obj->opaque = NBMemory_allocType(STTSClientOpq);
	NBMemory_setZeroSt(*opq, STTSClientOpq);
	opq->threadWorking2		= FALSE;
	opq->shuttingDown		= FALSE;
	NBThreadMutex_init(&opq->mutex);
	NBThreadCond_init(&opq->cond);
	TSCypher_init(&opq->cypher);
	opq->cypherKeyLocalSeq	= 0;	//Key local
	opq->cypherItfRemoteSeq	= 0;	//Itf remote
	opq->context			= NULL;
	opq->caCert				= NULL;
	//Sync
	{
		NBMemory_setZero(opq->sync);
		NBThreadMutex_init(&opq->sync.mutex);
		opq->sync.isReceiving	= FALSE;
		opq->sync.isFirstDone	= FALSE;
		NBString_init(&opq->sync.streamId);
	}
	//Ssl
	{
		NBString_init(&opq->ssl.curCertSerialHex);
		opq->ssl.defCtxt	= NBSslContext_alloc(NULL);
		NBArray_init(&opq->ssl.ctxts, sizeof(STTSClientCtx*), NULL);
		opq->ssl.ctxtsSeq	= 0;
	}
	TSRequestsMap_init(&opq->reqsMap);
	NBArray_init(&opq->channels, sizeof(STTSClientChannel*), NULL);
	opq->channelsReqsSeq	= 0;
	//App's orientation
	{
		opq->orientation.portraitEnforcers	= 0; //Amount of elements that required portrait mode
	}
	//streams
	{
		opq->streams.reqs = TSClientRequester_alloc(NULL);
	}
}

void TSClient_release(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	//
	{
		TSClient_startShutdown(obj);
		TSClient_waitForAll(obj);
	}
	//
	NBThreadMutex_lock(&opq->mutex);
	{
		//Sync
		{
			NBString_release(&opq->sync.streamId);
			NBThreadMutex_release(&opq->sync.mutex);
		}
		//
		if(opq->caCert != NULL){
			NBX509_release(opq->caCert);
			NBMemory_free(opq->caCert);
			opq->caCert = NULL;
		}
		{
			UI32 i; for(i = 0; i < opq->channels.use; i++){
				STTSClientChannel* c = NBArray_itmValueAtIndex(&opq->channels, STTSClientChannel*, i);
				TSClientChannel_release(c);
				NBMemory_free(c);
			}
			NBArray_empty(&opq->channels);
			NBArray_release(&opq->channels);
		}
		TSCypher_release(&opq->cypher);
		//Ssl
		{
			NBString_release(&opq->ssl.curCertSerialHex);
			NBSslContext_release(&opq->ssl.defCtxt);
			{
				SI32 i; for(i = 0 ; i < opq->ssl.ctxts.use; i++){
					STTSClientCtx* c = NBArray_itmValueAtIndex(&opq->ssl.ctxts, STTSClientCtx*, i);
					if(c != NULL){
						NBX509_release(&c->cert);
						NBPKey_release(&c->key);
						NBSslContext_release(&c->sslCtx);
						NBMemory_free(c);
					}
				}
				NBArray_empty(&opq->ssl.ctxts);
				NBArray_release(&opq->ssl.ctxts);
			}
		}
		//streams
		{
			if(TSClientRequester_isSet(opq->streams.reqs)){
				TSClientRequester_release(&opq->streams.reqs);
				TSClientRequester_null(&opq->streams.reqs);
			}
		}
		//
		opq->context	= NULL;
		TSRequestsMap_release(&opq->reqsMap);
	}
	NBThreadMutex_unlock(&opq->mutex);
	NBThreadCond_release(&opq->cond);
	NBThreadMutex_release(&opq->mutex);
	//
	NBMemory_free(opq);
	obj->opaque = NULL;
}

//App's orientation (amount of elements that required portrait mode)

SI32 TSClient_getAppPortraitEnforcersCount(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return opq->orientation.portraitEnforcers;
}

void TSClient_appPortraitEnforcerPush(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	opq->orientation.portraitEnforcers++;
	if(opq->orientation.portraitEnforcers == 1){
		PRINTF_INFO("TSClient, enforcing app orientation to portrait.\n");
	}
}

void TSClient_appPortraitEnforcerPop(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	opq->orientation.portraitEnforcers--;
	if(opq->orientation.portraitEnforcers == 0){
		PRINTF_INFO("TSClient, allowing app any orientation.\n");
	}
}

UI64 TSClient_addRequestOpq(STTSClientOpq* opq, const ENTSRequestId reqId, const char** paramsPairsAndNull, const char* body, const UI32 bodySz, const UI64 secsWait, TSClientChannelReqCallbackFunc lstnr, void* lstnrParam);
void TSClient_removeRequestsByListenerParamOpq(STTSClientOpq* opq, void* lstnrParam);

UI64 TSClient_addRequest_(void* obj, const ENTSRequestId reqId, const char** paramsPairsAndNull, const char* body, const UI32 bodySz, const UI64 secsWait, TSClientChannelReqCallbackFunc lstnr, void* lstnrParam){
	return TSClient_addRequestOpq((STTSClientOpq*)obj, reqId, paramsPairsAndNull, body, bodySz, secsWait, lstnr, lstnrParam);
}

void TSClient_removeRequestsByListenerParam_(void* obj, void* lstnrParam){
	return TSClient_removeRequestsByListenerParamOpq((STTSClientOpq*)obj, lstnrParam);
}

SI64 TSClient_runRequestTriggerAndRelease_(STNBThread* remoteDecrypt, void* param){
	STTSClientOpq* opq = (STTSClientOpq*)param;
	//Start
	{
		NBThreadMutex_lock(&opq->mutex);
		opq->threadWorking2 = TRUE;
		NBThreadMutex_unlock(&opq->mutex);
	}
	//Run
	{
		NBThreadMutex_lock(&opq->mutex);
		while(!opq->shuttingDown){
			//Trigger all channels
			{
				UI32 i; for(i = 0; i < opq->channels.use; i++){
					STTSClientChannel* c = NBArray_itmValueAtIndex(&opq->channels, STTSClientChannel*, i);
					TSClientChannel_triggerTimedRequests(c);
				}
			}
			//Wait
			{
				NBThreadMutex_unlock(&opq->mutex);
				{
					NBThread_mSleep(100);
				}
				NBThreadMutex_lock(&opq->mutex);
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	//Exit
	{
		NBThreadMutex_lock(&opq->mutex);
		opq->threadWorking2 = FALSE;
		NBThreadCond_broadcast(&opq->cond);
		NBThreadMutex_unlock(&opq->mutex);
	}
	//Release remoteDecrypt
	if(remoteDecrypt != NULL){
		NBThread_release(remoteDecrypt);
		NBMemory_free(remoteDecrypt);
		remoteDecrypt = NULL;
	}
	return 0;
}


//Stats
void TSClientStats_concatDataStreamsLocked_(STTSClientOpq* opq, const STTSClientStatsData* obj, const STTSCfgTelemetry* cfg, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, const STNBRtpClientPortStatsData* portsStats, const UI32 portsStatsSz, STNBString* dst);
void TSClientStats_concatStateStreamsLocked_(STTSClientOpq* opq, const STTSClientStatsState* obj, const STTSCfgTelemetry* cfg, const ENTSClientStartsGrp grp, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, const STNBRtpClientPortStatsData* portsStats, const UI32 portsStatsSz, STNBString* dst);
#ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
void TSClientStats_concatProcessState(const STTSClientStatsProcessState* obj, const ENNBLogLevel logLvl, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst);
#endif

BOOL TSClient_prepare(STTSClient* obj, const char* rootPath, STTSContext* context, const STTSCfg* pCfg, const void* caCertData, const UI32 caCertDataSz){
	BOOL r = TRUE;
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(opq->isPreparing){
		//Multiple calls to prepare
	} else {
		const STTSCfg* pCfg			= TSContext_getCfg(context);
		const char* runId			= TSContext_getRunId(context);
		STNBDataPtrsPoolRef bPool	= TSContext_getBuffersPool(context);
		STNBIOPollstersProviderRef pProv = TSContext_getPollstersProvider(context);
		opq->isPreparing = TRUE;
		//Load CA's certificate
		if(r){
			if(caCertData != NULL && caCertDataSz > 0){
				//Load from memory
				STNBX509* cert = NBMemory_allocType(STNBX509);
				NBX509_init(cert);
				if(!NBX509_createFromDERBytes(cert, caCertData, caCertDataSz)){
					PRINTF_CONSOLE_ERROR("TSClient, could not load ca-cert from %d bytes at memory.\n", caCertDataSz);
					r = FALSE;
				} else {
					opq->caCert = cert;
					cert = NULL;
				}
				if(cert != NULL){
					NBX509_release(cert);
					NBMemory_free(cert);
					cert = NULL;
				}
			} else {
				//Load from file
				const STNBHttpCertSrcCfg* s = &pCfg->context.caCert;
				if(s->path == 0){
					PRINTF_CONSOLE_ERROR("TSClient, requires 'caCert/path'.\n");
					r = FALSE;
				} else {
					const char* pCert = s->path;
					if(pCert == NULL || pCert[0] == '\0'){
						PRINTF_CONSOLE_ERROR("TSClient, requires a non-empty 'caCert/path'.\n");
						r = FALSE;
					} else {
						STNBFilesystem* fs = TSContext_getFilesystem(context);
						STNBString fullpath;
						NBString_init(&fullpath);
						NBString_concat(&fullpath, rootPath);
						NBString_concat(&fullpath, pCert);
                        STNBFileRef file = NBFile_alloc(NULL);
						if(!NBFilesystem_open(fs, fullpath.str, ENNBFileMode_Read, file)){
							PRINTF_CONSOLE_ERROR("TSClient, could not open ca-cert: '%s'.\n", fullpath.str);
						} else {
							NBFile_lock(file);
							{
								STNBX509* cert = NBMemory_allocType(STNBX509);
								NBX509_init(cert);
								if(!NBX509_createFromDERFile(cert, file)){
									PRINTF_CONSOLE_ERROR("TSClient, could not load ca-cert: '%s'.\n", fullpath.str);
									r = FALSE;
								} else {
									opq->caCert = cert;
									cert = NULL;
								}
								if(cert != NULL){
									NBX509_release(cert);
									NBMemory_free(cert);
									cert = NULL;
								}
							}
							NBFile_unlock(file);
						}
						NBFile_release(&file);
						NBString_release(&fullpath);
					}
				}
			}
		}
		//Create default ssl context
		if(r){
			if(!NBSslContext_create(opq->ssl.defCtxt, NBSslContext_getClientMode)){
				PRINTF_CONSOLE_ERROR("TSClient, could not create default sslContext.\n");
				r = FALSE;
			}
		}
		//Load cypher
		if(r){
			if(!TSCypher_loadFromConfig(&opq->cypher, context, opq->caCert, &pCfg->context.cypher)){
				PRINTF_CONSOLE_ERROR("TSClient, could not load cypher's config.\n");
				r = FALSE;
			} else if(!TSCypher_startThreads(&opq->cypher)){
				PRINTF_CONSOLE_ERROR("TSClient, could not start cypher's remoteDecrypts.\n");
				r = FALSE;
			}
		}
		//Create channels
		/*if(r && pCfg->channels != NULL){
			UI32 i; for(i = 0; i < pCfg->channelsSz && r; i++){
				STNBHttpPortCfg* cfg = &pCfg->channels[i];
				if(!cfg->isDisabled){
					STTSClientChannel* channel = NBMemory_allocType(STTSClientChannel);
					TSClientChannel_init(channel);
					if(!TSClientChannel_prepare(channel, opq, context, &opq->cypher, opq->ssl.defCtxt, FALSE, opq->caCert, &opq->reqsMap, cfg)){
						PRINTF_CONSOLE_ERROR("Could not load channel #%d from loaded cfg.\n", (i + 1));
						TSClientChannel_release(channel);
						NBMemory_free(channel);
						channel = NULL;
						r = FALSE;
					} else {
						NBArray_addValue(&opq->channels, channel);
					}
				}
			}
		}*/
		//Create work thread
		if(r){
			STNBThread* remoteDecrypt = NBMemory_allocType(STNBThread);
			NBThread_init(remoteDecrypt);
			NBThread_setIsJoinable(remoteDecrypt, FALSE);
			opq->threadWorking2 = TRUE;
			if(!NBThread_start(remoteDecrypt, TSClient_runRequestTriggerAndRelease_, opq, NULL)){
				PRINTF_CONSOLE_ERROR("TSClient, Could not start client's request trigger remoteDecrypt.\n");
				opq->threadWorking2 = FALSE;
				NBThread_release(remoteDecrypt);
				NBMemory_free(remoteDecrypt);
				remoteDecrypt = NULL;
				r = FALSE;
			}
		}
		//Add stream source (debug / testing)
		/*if(TSClientRequester_isSet(opq->streams.reqs)){
			//BOOL TSClientRequester_setStatsProvider(STTSClientRequesterRef ref, STNBRtspClientStats* rtspStats, STTSClientRequesterConnStats* reqsStats);
			if(!TSClientRequester_setExternalBuffers(opq->streams.reqs, bPool)){
				PRINTF_CONSOLE_ERROR("TSClient, TSClientRequester_setExternalBuffers failed.\n");
				r = FALSE;
			} else if(!TSClientRequester_setPollstersProvider(opq->streams.reqs, pProv)){
				PRINTF_CONSOLE_ERROR("TSClient, TSClientRequester_setPollstersProvider failed.\n");
				r = FALSE;
			} else if(!TSClientRequester_setCfg(opq->streams.reqs, &pCfg->server.rtsp, runId, ENNBRtspRessState_Described)){
				PRINTF_CONSOLE_ERROR("TSClient, TSClientRequester_setCfg failed.\n");
				r = FALSE;
			} else if(!TSClientRequester_prepare(opq->streams.reqs)){
				PRINTF_CONSOLE_ERROR("TSClient, TSClientRequester_prepare failed.\n");
				r = FALSE;
			} else if(!TSClientRequester_start(opq->streams.reqs)){
				PRINTF_CONSOLE_ERROR("TSClient, TSClientRequester_prepare failed.\n");
				r = FALSE;
			} else {
				//"127.0.0.1", "192.168.0.115", "192.168.0.105", "192.168.1.176"
				if(!TSClientRequester_addDeviceThinstream(opq->streams.reqs, "127.0.0.1", 30443)){
					PRINTF_CONSOLE_ERROR("TSClient, TSClientRequester_addDeviceThinstream(127.0.0.1) failed.\n");
					r = FALSE;
                / *} else if(!TSClientRequester_addDeviceThinstream(opq->streams.reqs, "192.168.0.9", 30443)){
                    PRINTF_CONSOLE_ERROR("TSClient, TSClientRequester_addDeviceThinstream(192.168.0.9) failed.\n");
                    r = FALSE;
                } else if(!TSClientRequester_addDeviceThinstream(opq->streams.reqs, "192.168.0.10", 30443)){
                    PRINTF_CONSOLE_ERROR("TSClient, TSClientRequester_addDeviceThinstream(192.168.0.10) failed.\n");
                    r = FALSE;
                } else if(!TSClientRequester_addDeviceThinstream(opq->streams.reqs, "192.168.0.105", 30443)){
                    PRINTF_CONSOLE_ERROR("TSClient, TSClientRequester_addDeviceThinstream(192.168.0.105) failed.\n");
                    r = FALSE;* /
				} else {
					//Add static streams (must be done after start)
					if(pCfg->server.streamsGrps != NULL && pCfg->server.streamsGrpsSz > 0){
						UI32 i; for(i = 0; i < pCfg->server.streamsGrpsSz && r; i++){
							const STTSCfgStreamsGrp* grp = &pCfg->server.streamsGrps[i];
							if(!grp->isDisabled && grp->streams != NULL && grp->streamsSz > 0){
								UI32 i; for(i = 0; i < grp->streamsSz && r; i++){
									const STTSCfgStream* s = &grp->streams[i];
									if(!s->isDisabled && s->versions != NULL && s->versionsSz > 0){
										UI32 i; for(i = 0; i < s->versionsSz  && r; i++){
											STTSCfgStreamVersion* v = &s->versions[i];
											if(!v->isDisabled){
												STTSStreamAddress address;
												NBMemory_setZeroSt(address, STTSStreamAddress);
												address.server		= s->device.server;
												address.port		= s->device.port;
												address.useSSL		= s->device.useSSL;
												address.uri			= v->uri;
												address.user		= s->device.user;
												address.pass		= s->device.pass;
												if(!TSClientRequester_addDeviceRtspStream(opq->streams.reqs, grp->uid, grp->name, s->device.uid, s->device.name, v->sid, v->sid, &address, (v->params != NULL ? v->params : &pCfg->server.defaults.streams.params))){
													PRINTF_CONSOLE_ERROR("TSClient, TSClientRequester_rtspAddViewer failed for static viewer: '%s'.\n", v->uri);
													r = FALSE;
												} else {
													PRINTF_INFO("TSClient, rtspStream added: '%s'.\n", v->uri);
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}*/
		//Context
		if(r){
			opq->context	= context;
		}
		//Make all workers to continue
		opq->isPreparing = FALSE;
		NBThreadCond_broadcast(&opq->cond);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

//Sync flag

BOOL TSClient_syncIsReceiving(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	BOOL r = FALSE;
	NBThreadMutex_lock(&opq->sync.mutex);
	{
		r = opq->sync.isReceiving;
	}
	NBThreadMutex_unlock(&opq->sync.mutex);
	return r;
}

BOOL TSClient_syncIsFirstDone(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	BOOL r = FALSE;
	NBThreadMutex_lock(&opq->sync.mutex);
	{
		r = opq->sync.isFirstDone;
	}
	NBThreadMutex_unlock(&opq->sync.mutex);
	return r;
}

void TSClient_syncConcatStreamId(STTSClient* obj, STNBString* dst){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	if(dst != NULL){
		NBThreadMutex_lock(&opq->sync.mutex);
		{
			NBString_set(dst, opq->sync.streamId.str);
		}
		NBThreadMutex_unlock(&opq->sync.mutex);
	}
}

void TSClient_syncSetReceiving(STTSClient* obj, const BOOL isReceiving){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->sync.mutex);
	{
		opq->sync.isReceiving = isReceiving;
		//Reset other
		if(!isReceiving){
			opq->sync.isFirstDone = FALSE;
			NBString_empty(&opq->sync.streamId);
		}
	}
	NBThreadMutex_unlock(&opq->sync.mutex);
}

void TSClient_syncSetFirstDone(STTSClient* obj, const BOOL isSyncFirstDone){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->sync.mutex);
	{
		opq->sync.isFirstDone = isSyncFirstDone;
	}
	NBThreadMutex_unlock(&opq->sync.mutex);
}

void TSClient_syncSetStreamId(STTSClient* obj, const char* streamId){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->sync.mutex);
	{
		NBString_set(&opq->sync.streamId, streamId);
	}
	NBThreadMutex_unlock(&opq->sync.mutex);
}

//Locale

const char* TSClient_getLocalePreferedLangAtIdx(STTSClient* obj, const UI32 idx, const char* lngDef){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	STNBLocale* locale = TSContext_getLocale(opq->context);
	return NBLocale_getPreferedLangAtIdx(locale, idx, lngDef);
}

STNBLocale* TSClient_getLocale(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSContext_getLocale(opq->context);
}

const char* TSClient_getStr(STTSClient* obj, const char* uid, const char* strDefault){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSContext_getStr(opq->context, uid, strDefault);
}

//

void TSClient_concatRootFolderPath(const STTSClient* obj, STNBString* dst){
	const STTSClientOpq* opq = (const STTSClientOpq*)obj->opaque;
	const STTSCfg* cfg = TSContext_getCfg(opq->context);
	if(!NBString_strIsEmpty(cfg->context.filesystem.rootFolder)){
		NBString_concat(dst, cfg->context.filesystem.rootFolder);
		if(dst->length > 0){
			if(dst->str[dst->length - 1] != '/'){
				NBString_concatByte(dst, '/');
			}
		}
	}
}

const char* TSClient_getRootFolderName(const STTSClient* obj){
	const STTSClientOpq* opq = (const STTSClientOpq*)obj->opaque;
	const STTSCfg* cfg = TSContext_getCfg(opq->context);
	return cfg->context.filesystem.rootFolder;
}


STTSContext* TSClient_getContext(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return opq->context;
}

STTSCypher* TSClient_getCypher(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return &opq->cypher;
}

STNBFilesystem* TSClient_getFilesystem(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSContext_getFilesystem(opq->context);
}

STTSClientRequesterRef TSClient_getStreamsRequester(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return opq->streams.reqs;
}

BOOL TSClient_setDeviceCertAndKeyOpq(STTSClientOpq* opq, STNBPKey* key, STNBX509* cert){
	BOOL r = TRUE;
	NBThreadMutex_lock(&opq->mutex);
	{
		STTSClientCtx* ctx = NBMemory_allocType(STTSClientCtx);
		NBMemory_setZeroSt(*ctx, STTSClientCtx);
		NBPKey_init(&ctx->key);
		NBX509_init(&ctx->cert);
		ctx->sslCtx = NBSslContext_alloc(NULL);
		//Load key
		if(r && key != NULL){
			if(!NBPKey_createFromOther(&ctx->key, key)){
				PRINTF_CONSOLE_ERROR("TSClient, NBPKey_createFromOther failed.\n");
				r = FALSE;
			}
		}
		//Load cert
		if(r && cert != NULL){
			if(!NBX509_createFromOther(&ctx->cert, cert)){
				PRINTF_CONSOLE_ERROR("TSClient, NBX509_createFromOther failed.\n");
				r = FALSE;
			}
		}
		//Create context
		if(r){
			if(!NBSslContext_create(ctx->sslCtx, NBSslContext_getClientMode)){
				PRINTF_CONSOLE_ERROR("TSClient, NBSslContext_create failed.\n");
				r = FALSE;
			} else {
				if(key != NULL && cert != NULL){
					if(!NBSslContext_attachCertAndkey(ctx->sslCtx, cert, key)){
						PRINTF_CONSOLE_ERROR("TSClient, NBSslContext_attachCertAndkey failed.\n");
						r = FALSE;
					}
				}
			}
		}
		//Set channels contexts
		if(r){
			{
				UI32 i; for(i = 0; i < opq->channels.use; i++){
					STTSClientChannel* c = NBArray_itmValueAtIndex(&opq->channels, STTSClientChannel*, i);
					if(!TSClientChannel_sslContextSet(c, ctx->sslCtx, TRUE)){
						PRINTF_CONSOLE_ERROR("TSClient, TSClientChannel_sslContextSet failed.\n");
						r = FALSE;
						break;
					}
				}
			}
			//Consume context
			if(r){
#				ifdef NB_CONFIG_INCLUDE_ASSERTS
				if(cert != NULL){
					STNBString ss;
					NBString_init(&ss);
					if(NBX509_concatSerialHex(cert, &ss)){
						//PRINTF_INFO("TSClient_setDeviceCertAndKeyOpq success for '%s'.\n", ss.str);
					}
					{
						STNBString test, enc, dec;
						NBString_initWithStr(&test, "test");
						NBString_init(&enc);
						NBString_init(&dec);
						NBASSERT(NBPKey_concatEncryptedBytes(&ctx->key, test.str, test.length, &enc))
						NBASSERT(NBX509_concatDecryptedBytes(&ctx->cert, enc.str, enc.length, &dec))
						NBASSERT(NBString_isEqual(&dec, test.str))
						NBString_release(&enc);
						NBString_release(&dec);
						NBString_release(&test);
					}
					NBString_release(&ss);
				}
#				else
				PRINTF_INFO("TSClient, TSClient_setDeviceCertAndKeyOpq success.\n");
#				endif
				//Set current certSerial
				{
					NBString_empty(&opq->ssl.curCertSerialHex);
					if(cert != NULL){
						NBX509_concatSerialHex(cert, &opq->ssl.curCertSerialHex);
					}
				}
				//Add
				{
					NBArray_addValue(&opq->ssl.ctxts, ctx);
					ctx = NULL;
				}
				//Change seq
				opq->ssl.ctxtsSeq++;
			}
		}
		//Release (if not consumed)
		if(ctx != NULL){
			NBPKey_release(&ctx->key);
			NBX509_release(&ctx->cert);
			NBSslContext_release(&ctx->sslCtx);
			NBMemory_free(ctx);
			ctx = NULL;
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

//Log

void TSClient_logAnalyzeFile(STTSClient* obj, const char* filepath, const BOOL printContent){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	TSContext_logAnalyzeFile(opq->context, filepath, printContent);
}

UI32 TSClient_logPendLength(STTSClient* obj){
	UI32 r = 0;
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	r = TSContext_logPendLength(opq->context);
	return r;
}

void TSClient_logPendGet(STTSClient* obj, STNBString* dst){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	TSContext_logPendGet(opq->context, dst);
}

void TSClient_logPendClearOpq(STTSClientOpq* opq){
	TSContext_logPendClear(opq->context);
}

void TSClient_logPendClear(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	TSClient_logPendClearOpq(opq);
}

//

STNBX509* TSClient_getCACert(STTSClient* obj){
	STNBX509* r = NULL;
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	if(opq != NULL){
		r = opq->caCert;
	}
	return r;
}

UI32 TSClient_getDeviceKeySeq(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return opq->ssl.ctxtsSeq;
}

BOOL TSClient_isDeviceKeySet(STTSClient* obj){
	BOOL r = FALSE;
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(opq->ssl.ctxts.use > 0){
		STTSClientCtx* ctx = NBArray_itmValueAtIndex(&opq->ssl.ctxts, STTSClientCtx*, opq->ssl.ctxts.use - 1);
		if(NBPKey_isCreated(&ctx->key)){
			r = TRUE;
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

/*BOOL TSClient_setDeviceCertAndKey(STTSClient* obj, STNBPKey* key, STNBX509* cert){
	BOOL r = FALSE;
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	r = TSClient_setDeviceCertAndKeyOpq(opq, key, cert);
	return r;
}*/

/*BOOL TSClient_touchCurrentKeyAndCert(STTSClient* obj){ //Force new connections
	BOOL r = TRUE;
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	{
		UI32 i; for(i = 0; i < opq->channels.use; i++){
			STTSClientChannel* c = NBArray_itmValueAtIndex(&opq->channels, STTSClientChannel*, i);
			if(!TSClientChannel_sslContextTouch(c)){
				PRINTF_CONSOLE_ERROR("TSClientChannel_sslContextTouch failed.\n");
				r = FALSE;
				break;
			}
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}*/

BOOL TSClient_encForCurrentKey(STTSClient* obj, const void* plain, const UI32 plainSz, STNBString* dst){
	BOOL r = FALSE;
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(opq->ssl.ctxts.use > 0){
		STTSClientCtx* ctx = NBArray_itmValueAtIndex(&opq->ssl.ctxts, STTSClientCtx*, opq->ssl.ctxts.use - 1);
		r = NBX509_concatEncryptedBytes(&ctx->cert, plain, plainSz, dst);
		/*if(r){
			STNBString hex;
			NBString_init(&hex);
			NBString_concatBytesHex(&hex, (const char*)dst->str, dst->length);
			PRINTF_INFO("Encrypted to '%s'.\n", hex.str);
			NBString_release(&hex);
		}*/
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSClient_decWithCurrentKey(STTSClient* obj, const void* enc, const UI32 encSz, STNBString* dst){
	BOOL r = FALSE;
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(opq->ssl.ctxts.use > 0){
		/*{
			STNBString hex;
			NBString_init(&hex);
			NBString_concatBytesHex(&hex, (const char*)enc, encSz);
			PRINTF_INFO("Decrypting from '%s'.\n", hex.str);
			NBString_release(&hex);
		}*/
		STTSClientCtx* ctx = NBArray_itmValueAtIndex(&opq->ssl.ctxts, STTSClientCtx*, opq->ssl.ctxts.use - 1);
		r = NBPKey_concatDecryptedBytes(&ctx->key, enc, encSz, dst);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSClient_decKeyLocalOpq(STTSClientOpq* opq, const STTSCypherDataKey* keyEnc, STTSCypherDataKey* dstKeyPlain){
	BOOL r = FALSE;
	if(!TSCypherDataKey_isEmpty(keyEnc)){
		NBThreadMutex_lock(&opq->mutex);
		if(opq->ssl.ctxts.use > 0){
			STTSClientCtx* ctx = NBArray_itmValueAtIndex(&opq->ssl.ctxts, STTSClientCtx*, opq->ssl.ctxts.use - 1);
			STNBString pass, salt;
			NBString_init(&pass);
			NBString_init(&salt);
			if(!NBPKey_concatDecryptedBytes(&ctx->key, keyEnc->pass, keyEnc->passSz, &pass)){
				//PRINTF_CONSOLE_ERROR("TSClient_decKeyLocalOpq, NBPKey_concatDecryptedBytes failed for enc-pass-sz(%d).\n", keyEnc->passSz);
			} else if(!NBPKey_concatDecryptedBytes(&ctx->key, keyEnc->salt, keyEnc->saltSz, &salt)){
				//PRINTF_CONSOLE_ERROR("TSClient_decKeyLocalOpq, NBPKey_concatDecryptedBytes failed for end-salt-sz(%d).\n", keyEnc->saltSz);
			} else {
				if(dstKeyPlain != NULL){
					dstKeyPlain->iterations = keyEnc->iterations;
					NBString_strFreeAndNewBufferBytes((char**)&dstKeyPlain->pass, pass.str, pass.length);
					NBString_strFreeAndNewBufferBytes((char**)&dstKeyPlain->salt, salt.str, salt.length);
					dstKeyPlain->passSz = pass.length;
					dstKeyPlain->saltSz = salt.length;
				}
				r = TRUE;
			}
			NBString_release(&pass);
			NBString_release(&salt);
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	return r;
}

BOOL TSClient_decKeyLocal(STTSClient* obj, const STTSCypherDataKey* keyEnc, STTSCypherDataKey* dstKeyPlain){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSClient_decKeyLocalOpq(opq, keyEnc, dstKeyPlain);
}

BOOL TSClient_decDataKeyOpq(void* pOpq, STTSCypherDataKey* dstKeyPlain, const STTSCypherDataKey* keyEncRemote){
	BOOL r = FALSE;
	STTSClientOpq* opq = (STTSClientOpq*)pOpq;
	STTSCypherDataKey keyEnc;
	NBMemory_setZeroSt(keyEnc, STTSCypherDataKey);
	{
		r = TSClient_decKeyLocalOpq(opq, &keyEnc, dstKeyPlain);
	}
	NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &keyEnc, sizeof(keyEnc));
	return r;
}

//Stats

UI32 TSClient_getStatsTryConnAccum(STTSClient* obj, const BOOL resetAccum){
	UI32 r = 0;
	if(obj != NULL){
		STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
		NBThreadMutex_lock(&opq->mutex);
		{
			UI32 i; for(i = 0; i < opq->channels.use; i++){
				STTSClientChannel* c = NBArray_itmValueAtIndex(&opq->channels, STTSClientChannel*, i);
				r += TSClientChannel_getStatsTryConnAccum(c, resetAccum);
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	return r;
}

//

UI64 TSClient_addRequestOpq(STTSClientOpq* opq, const ENTSRequestId reqId, const char** paramsPairsAndNull, const char* body, const UI32 bodySz, const UI64 secsWait, TSClientChannelReqCallbackFunc lstnr, void* lstnrParam){
	UI64 r = 0;
	NBThreadMutex_lock(&opq->mutex);
	{
		const UI64 reqUniqueId = ++opq->channelsReqsSeq;
		//Determine language
		STNBLocale* locale = TSContext_getLocale(opq->context);
		const char* lang = NBLocale_getPreferedLangAtIdx(locale, 0, "en");
		//Add request
		{
			UI32 i; for(i = 0; i < opq->channels.use; i++){
				STTSClientChannel* c = NBArray_itmValueAtIndex(&opq->channels, STTSClientChannel*, i);
				if(0 != (r = TSClientChannel_addRequest(c, &reqUniqueId, reqId, lang, paramsPairsAndNull, body, bodySz, secsWait, lstnr, lstnrParam))){
					//PRINTF_INFO("Request accepted by channel #%d port(%d): '%s'.\n", (i + 1), TSClientChannel_getCfg(c)->port, TSRequestId_getSharedEnumMap()->records[reqId].varName);
					break;
				}
			}
		}
		if(r == 0){
			PRINTF_CONSOLE_ERROR("TSClient, no channel accepted the request.\n");
			NBASSERT(FALSE)
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}
	
UI64 TSClient_addRequest(STTSClient* obj, const ENTSRequestId reqId, const char** paramsPairsAndNull, const char* body, const UI32 bodySz, const UI64 secsWait, TSClientChannelReqCallbackFunc lstnr, void* lstnrParam){
	return TSClient_addRequestOpq((STTSClientOpq*)obj->opaque, reqId, paramsPairsAndNull, body, bodySz, secsWait, lstnr, lstnrParam);
}

void TSClient_removeRequestsByListenerParamOpq(STTSClientOpq* opq, void* lstnrParam){
	NBThreadMutex_lock(&opq->mutex);
	UI32 i; for(i = 0; i < opq->channels.use; i++){
		STTSClientChannel* c = NBArray_itmValueAtIndex(&opq->channels, STTSClientChannel*, i);
		TSClientChannel_removeRequestsByListenerParam(c, lstnrParam);
	}
	NBThreadMutex_unlock(&opq->mutex);
}

void TSClient_removeRequestsByListenerParam(STTSClient* obj, void* lstnrParam){
	TSClient_removeRequestsByListenerParamOpq((STTSClientOpq*)obj->opaque, lstnrParam);
}

void TSClient_notifyAll(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	//Request notifications
	{
		NBThreadMutex_lock(&opq->mutex);
		if(opq->channels.use > 0){
			//Copy requests results
			STNBArray toNotifyAndRelease;
			NBArray_init(&toNotifyAndRelease, sizeof(STTSClientChannelRequest*), NULL);
			{
				UI32 i; for(i = 0; i < opq->channels.use; i++){
					STTSClientChannel* c = NBArray_itmValueAtIndex(&opq->channels, STTSClientChannel*, i);
					TSClientChannel_getRequestsResultsToNotifyAndRelease(c, &toNotifyAndRelease);
				}
			}
			if(toNotifyAndRelease.use > 0){
				NBThreadMutex_unlock(&opq->mutex);
				{
					//Notify while unlocked
					UI32 i; for(i = 0; i < toNotifyAndRelease.use; i++){
						STTSClientChannelRequest* req = NBArray_itmValueAtIndex(&toNotifyAndRelease, STTSClientChannelRequest*, i);
						if(req->lstnr != NULL){
							(*req->lstnr)(req, req->lstnrParam);
						}
						TSClientChannelRequest_release(req);
						NBMemory_free(req);
					}
				}
				NBThreadMutex_lock(&opq->mutex);
			}
			NBArray_release(&toNotifyAndRelease);
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	//Records notifications
	{
		TSContext_notifyAll(opq->context);
	}
}

BOOL TSClient_isCypherKeyAnySet(STTSClient* obj){		//When decryption is made local or remote
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSCypher_isKeyAnySet(&opq->cypher);
}

BOOL TSClient_isCypherKeyLocalSet(STTSClient* obj){		//When decryption is made local
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSCypher_isKeyLocalSet(&opq->cypher);
}

UI32 TSClient_getCypherKeyLocalSeq(STTSClient* obj){ //Key local
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return opq->cypherKeyLocalSeq;
}

UI32 TSClient_getCypherItfRemoteSeq(STTSClient* obj){ //Itf remote
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return opq->cypherItfRemoteSeq;
}

BOOL TSClient_setCypherCertAndKeyOpq(STTSClientOpq* opq, STNBX509* cert, STNBPKey* key, const STTSCypherDataKey* sharedKeyEnc){
	BOOL r = FALSE;
	PRINTF_INFO("TSClient, TSClient_setCypherCertAndKey.\n");
	if(!TSCypher_setCertAndKey_(&opq->cypher, cert, key)){
		PRINTF_CONSOLE_ERROR("TSClient, TSClient_setCypherCertAndKey, TSCypher_setCertAndKey_ failed.\n");
	} else {
		r = TRUE;
		if(r && sharedKeyEnc != NULL){
			if(!TSCypher_setSharedKey_(&opq->cypher, sharedKeyEnc)){
				PRINTF_CONSOLE_ERROR("TSClient, TSClient_setCypherCertAndKey, TSCypher_setSharedKey, failed.\n");
				r = FALSE;
			}
		}
		if(r){
			//Increase sequence
			{
				opq->cypherKeyLocalSeq++;
			}
			//Force new connections
			NBThreadMutex_lock(&opq->mutex);
			{
				UI32 i; for(i = 0; i < opq->channels.use; i++){
					STTSClientChannel* c = NBArray_itmValueAtIndex(&opq->channels, STTSClientChannel*, i);
					if(!TSClientChannel_sslContextTouch(c)){
						PRINTF_CONSOLE_ERROR("TSClient, TSClient_setCypherCertAndKey, TSClientChannel_sslContextTouch failed.\n");
						r = FALSE;
						break;
					}
				}
			}
			NBThreadMutex_unlock(&opq->mutex);
		}
	}
	return r;
}

//

/*BOOL TSClient_setCypherCertAndKey(STTSClient* obj, STNBX509* cert, STNBPKey* key, const STTSCypherDataKey* sharedKeyEnc){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSClient_setCypherCertAndKeyOpq(opq, cert, key, sharedKeyEnc);
}*/

void TSClient_disconnect(STTSClient* obj, const UI64* filterReqUid){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	UI32 i; for(i = 0; i < opq->channels.use; i++){
		STTSClientChannel* c = NBArray_itmValueAtIndex(&opq->channels, STTSClientChannel*, i);
		TSClientChannel_disconnect(c, filterReqUid);
	}
	NBThreadMutex_unlock(&opq->mutex);
}

void TSClient_startShutdown(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	{
		NBThreadMutex_lock(&opq->mutex);
		{
			//streams
			{
				TSClientRequester_stopFlag(opq->streams.reqs);
			}
			//Start channels shutdown
			{
				UI32 i; for(i = 0; i < opq->channels.use; i++){
					STTSClientChannel* c = NBArray_itmValueAtIndex(&opq->channels, STTSClientChannel*, i);
					TSClientChannel_startShutdown(c);
				}
			}
			//Start my thread shutdown
			{
				opq->shuttingDown = TRUE;
				NBThreadCond_broadcast(&opq->cond);
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
}

void TSClient_waitForAll(STTSClient* obj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	{
		NBThreadMutex_lock(&opq->mutex);
		{
			//Wait for clients
			{
				UI32 i; for(i = 0; i < opq->channels.use; i++){
					STTSClientChannel* c = NBArray_itmValueAtIndex(&opq->channels, STTSClientChannel*, i);
					TSClientChannel_waitForAll(c);
				}
			}
			//Wait for thread
			{
				while(opq->threadWorking2){
					NBThreadCond_broadcast(&opq->cond);
					NBThreadCond_wait(&opq->cond, &opq->mutex);
				}
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
}

//

BOOL TSClient_concatDeviceIdOpq(STTSClientOpq* opq, STNBString* dst){
	BOOL r = FALSE;
	if(opq != NULL){
		STNBStorageRecordRead ref = TSContext_getStorageRecordForRead(opq->context, "client/_root/", "_current", NULL, TSClientRoot_getSharedStructMap());
		if(ref.data != NULL){
			STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
			if(priv != NULL){
				if(dst != NULL){
					if(NBString_strIsEmpty(priv->deviceId)){
						NBString_empty(dst);
					} else {
						NBString_set(dst, priv->deviceId);
					}
				}
				r = TRUE;
			}
		}
		TSContext_returnStorageRecordFromRead(opq->context, &ref);
	}
	return r;
}

BOOL TSClient_concatDeviceId(STTSClient* obj, STNBString* dst){
	return TSClient_concatDeviceIdOpq((STTSClientOpq*)obj->opaque, dst);
}

//Certiticates validation

BOOL TSClient_certIsSignedByCAOpq(STTSClientOpq* opq, STNBX509* cert){
	BOOL r = FALSE;
	if(cert != NULL && opq->caCert != NULL){
		r = NBX509_isSignedBy(cert, opq->caCert);
	}
	return r;
}

BOOL TSClient_certIsSignedByCA(STTSClient* obj, STNBX509* cert){
	BOOL r = FALSE;
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	r = TSClient_certIsSignedByCAOpq(opq, cert);
	return r;
}

//Records listeners

void TSClient_addRecordListener(STTSClient* obj, const char* storagePath, const char* recordPath, void* refObj, const STNBStorageCacheItmLstnr* methods){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	TSContext_addRecordListener(opq->context, storagePath, recordPath, refObj, methods);
}

void TSClient_addRecordListenerAtClinic(STTSClient* obj, const char* clinicId, const char* storagePath, const char* recordPath, void* refObj, const STNBStorageCacheItmLstnr* methods){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	TSContext_addRecordListenerAtClinic(opq->context, clinicId, storagePath, recordPath, refObj, methods);
}


void TSClient_removeRecordListener(STTSClient* obj, const char* storagePath, const char* recordPath, void* refObj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	TSContext_removeRecordListener(opq->context, storagePath, recordPath, refObj);
}

void TSClient_removeRecordListenerAtClinic(STTSClient* obj, const char* clinicId, const char* storagePath, const char* recordPath, void* refObj){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	TSContext_removeRecordListenerAtClinic(opq->context, clinicId, storagePath, recordPath, refObj);
}

//Read record

STNBStorageRecordRead TSClient_getStorageRecordForRead(STTSClient* obj, const char* storagePath, const char* recordPath, const STNBStructMap* pubMap, const STNBStructMap* privMap){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSContext_getStorageRecordForRead(opq->context, storagePath, recordPath, pubMap, privMap);
}

STNBStorageRecordRead TSClient_getStorageRecordForReadAtClinic(STTSClient* obj, const char* clinicId, const char* storagePath, const char* recordPath, const STNBStructMap* pubMap, const STNBStructMap* privMap){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSContext_getStorageRecordForReadAtClinic(opq->context, clinicId, storagePath, recordPath, pubMap, privMap);
}

void TSClient_returnStorageRecordFromRead(STTSClient* obj, STNBStorageRecordRead* ref){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSContext_returnStorageRecordFromRead(opq->context, ref);
}

//Write record

STNBStorageRecordWrite TSClient_getStorageRecordForWrite(STTSClient* obj, const char* storagePath, const char* recordPath, const BOOL createIfNew, const STNBStructMap* pubMap, const STNBStructMap* privMap){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSContext_getStorageRecordForWrite(opq->context, storagePath, recordPath, createIfNew, pubMap, privMap);
}

STNBStorageRecordWrite TSClient_getStorageRecordForWriteAtClinic(STTSClient* obj, const char* clinicId, const char* storagePath, const char* recordPath, const BOOL createIfNew, const STNBStructMap* pubMap, const STNBStructMap* privMap){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSContext_getStorageRecordForWriteAtClinic(opq->context, clinicId, storagePath, recordPath, createIfNew, pubMap, privMap);
}

void TSClient_returnStorageRecordFromWrite(STTSClient* obj, STNBStorageRecordWrite* ref){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSContext_returnStorageRecordFromWrite(opq->context, ref);
}

//Touch record (trigger a notification to listeners without saving to file)

void TSClient_touchRecord(STTSClient* obj, const char* storagePath, const char* recordPath){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSContext_touchRecord(opq->context, storagePath, recordPath);
}

void TSClient_touchRecordAtClinic(STTSClient* obj, const char* clinicId, const char* storagePath, const char* recordPath){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	return TSContext_touchRecordAtClinic(opq->context, clinicId, storagePath, recordPath);
}

//Time format

void TSClient_concatHumanTimeFromSeconds(STTSClient* obj, const UI32 seconds, const BOOL useShortTags, const UI32 maxPartsCount, STNBString* dst){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	TSContext_concatHumanTimeFromSeconds(opq->context, seconds, useShortTags, maxPartsCount, dst);
}

void TSClient_concatHumanDate(STTSClient* obj, const STNBDatetime localDatetime, const BOOL useShortTags, const char* separatorDateFromTime, STNBString* dst){
	STTSClientOpq* opq = (STTSClientOpq*)obj->opaque;
	TSContext_concatHumanDate(opq->context, localDatetime, useShortTags, separatorDateFromTime, dst);
}

//
// Build request "DeviceNew"
//

typedef struct STTSClient_asyncBuildRequestDeviceNewRoutineParam_ {
	STTSClient*			clt;
	STNBPKey**			dstKey;
	STNBX509Req**		dstCsr;
	BOOL*				dstIsBuilding;
} STTSClient_asyncBuildRequestDeviceNewRoutineParam;

SI64 TSClient_asyncBuildPkeyAndCsrRoutine_(STNBThread* remoteDecrypt, void* param){
	SI64 r = 0;
	if(param != NULL){
		STTSClient_asyncBuildRequestDeviceNewRoutineParam* p = (STTSClient_asyncBuildRequestDeviceNewRoutineParam*)param;
		NBASSERT(p->clt != NULL)
		NBASSERT(p->dstKey != NULL)
		NBASSERT(p->dstCsr != NULL)
		NBASSERT(p->dstIsBuilding != NULL && *p->dstIsBuilding)
		//End-of-task
		if(p->dstIsBuilding != NULL){
			NBASSERT(*p->dstIsBuilding)
			*p->dstIsBuilding = TRUE;
		}
		//Generate keys
		{
			STTSContext* context = TSClient_getContext(p->clt);
			const STTSCfg* cfg = TSContext_getCfg(context);
			STNBPKey* devPKey = NBMemory_allocType(STNBPKey);
			STNBX509Req* devCsr = NBMemory_allocType(STNBX509Req);
			NBPKey_init(devPKey);
			NBX509Req_init(devCsr);
			if(!NBX509Req_createSelfSigned(devCsr, cfg->context.deviceId.def.bits, cfg->context.deviceId.def.subject, cfg->context.deviceId.def.subjectSz, devPKey)){
				PRINTF_CONSOLE_ERROR("Could not generate devCsr.\n");
			} else {
				*p->dstKey	= devPKey;
				*p->dstCsr	= devCsr;
				devPKey		= NULL;
				devCsr		= NULL;
			}
			//Release (if not consumed)
			if(devCsr != NULL){
				NBX509Req_release(devCsr);
				devCsr = NULL;
			}
			if(devPKey != NULL){
				NBPKey_release(devPKey);
				devPKey = NULL;
			}
		}
		//End-of-task
		if(p->dstIsBuilding != NULL){
			NBASSERT(*p->dstIsBuilding)
			*p->dstIsBuilding = FALSE;
		}
	}
	//Release
	if(remoteDecrypt != NULL){
		NBThread_release(remoteDecrypt);
		NBMemory_free(remoteDecrypt);
		remoteDecrypt = NULL;
	}
	return r;
}
SI32 TSClient_asyncBuildPkeyAndCsr(STTSClient* clt, STNBPKey** dstKey, STNBX509Req** dstCsr, BOOL* dstIsBuilding){
	BOOL r = FALSE;
	if(clt != NULL && dstKey != NULL && dstCsr != NULL && dstIsBuilding != NULL){
		STTSClient_asyncBuildRequestDeviceNewRoutineParam* d = NBMemory_allocType(STTSClient_asyncBuildRequestDeviceNewRoutineParam);
		NBMemory_setZeroSt(*d, STTSClient_asyncBuildRequestDeviceNewRoutineParam);
		d->clt				= clt;
		d->dstKey			= dstKey;
		d->dstCsr			= dstCsr;
		d->dstIsBuilding	= dstIsBuilding;
		{
			STNBThread* t = NBMemory_allocType(STNBThread);
			NBThread_init(t);
			NBThread_setIsJoinable(t, FALSE);
			if(d->dstIsBuilding != NULL){
				*d->dstIsBuilding = TRUE;
			}
			if(NBThread_start(t, TSClient_asyncBuildPkeyAndCsrRoutine_, d, NULL)){
				r = TRUE;
			} else {
				if(d->dstIsBuilding != NULL){
					*d->dstIsBuilding = FALSE;
				}
				//Release
				NBMemory_free(d);
				d = NULL;
				//Release
				NBThread_release(t);
				NBMemory_free(t);
				t = NULL;
			}
		}
	}
	return r;
}


//---------------------------------------

//
// Request current PKey and Cert
//

SI32 TSClient_loadCurrentDeviceKeyOpq(STTSClientOpq* opq){
	BOOL r = FALSE;
	STNBString devId;
	STNBPKey key;
	STNBX509 cert;
	NBString_init(&devId);
	NBPKey_init(&key);
	NBX509_init(&cert);
	{
		STTSCypherDataKey sharedKeyEnc;
		NBMemory_setZeroSt(sharedKeyEnc, STTSCypherDataKey);
		//Load deviceId and shared key
		{
			STNBStorageRecordRead ref = TSContext_getStorageRecordForRead(opq->context, "client/_root/", "_current", NULL, TSClientRoot_getSharedStructMap());
			if(ref.data != NULL){
				STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
				if(priv != NULL){
					if(!NBString_strIsEmpty(priv->deviceId)){
						NBString_set(&devId, priv->deviceId);
					}
					NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &sharedKeyEnc, sizeof(sharedKeyEnc));
					NBStruct_stClone(TSCypherDataKey_getSharedStructMap(), &priv->sharedKeyEnc, sizeof(priv->sharedKeyEnc), &sharedKeyEnc, sizeof(sharedKeyEnc)); 
				}
			}
			TSContext_returnStorageRecordFromRead(opq->context, &ref);
		}
		//Load Cert and Key
		if(devId.length > 0){
			//Load p12 pass
			STNBString pass;
			NBString_init(&pass);
			TSDevice_getPassword(&opq->cypher, &pass, devId.str, 32, FALSE);
			if(pass.length <= 0){
				PRINTF_CONSOLE_ERROR("TSClient, TSClient_loadCurrentDeviceKey, could not load password for certificate.\n")
			} else if(!TSCertificate_getPKeyAndCert(opq->context, "devices/_certs/", devId.str, ENTSCertificateType_Device, devId.str, &key, &cert, pass.str)){
				PRINTF_CONSOLE_ERROR("TSClient, TSClient_loadCurrentDeviceKey, could not load key/cert from certificate record.\n")
			} else if(!TSClient_setDeviceCertAndKeyOpq(opq, &key, &cert)){
				PRINTF_CONSOLE_ERROR("TSClient, TSClient_loadCurrentDeviceKey, TSClient_setDeviceCertAndKeyOpq failed.\n")
			} else {
				//PRINTF_INFO("TSClient_loadCurrentDeviceKey, certificate loaded (pkey and cert).\n");
				//Gen 'sharedKeyEnc' if necesary
				if(TSCypherDataKey_isEmpty(&sharedKeyEnc)){
					STTSCypherDataKey keyPlain;
					NBMemory_setZeroSt(keyPlain, STTSCypherDataKey);
					if(!TSCypher_genDataKey(&sharedKeyEnc, &keyPlain, &cert)){
						PRINTF_CONSOLE_ERROR("TSClient, SSCypher_genDataKey failed.\n");
					} else {
						STNBStorageRecordWrite ref = TSContext_getStorageRecordForWrite(opq->context, "client/_root/", "_current", FALSE, NULL, TSClientRoot_getSharedStructMap());
						if(ref.data != NULL){
							STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
							if(priv != NULL){
								NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &priv->sharedKeyEnc, sizeof(priv->sharedKeyEnc));
								NBStruct_stClone(TSCypherDataKey_getSharedStructMap(), &sharedKeyEnc, sizeof(sharedKeyEnc), &priv->sharedKeyEnc, sizeof(priv->sharedKeyEnc));
								ref.privModified = TRUE;
							}
						}
						TSContext_returnStorageRecordFromWrite(opq->context, &ref);
					}
					NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &keyPlain, sizeof(keyPlain));
				}
				//Set cypher
				if(!TSClient_setCypherCertAndKeyOpq(opq, &cert, &key, &sharedKeyEnc)){
					PRINTF_CONSOLE_ERROR("TSClient, TSClient_setCypherCertAndKey failed.\n");
				} else {
					PRINTF_INFO("TSClient, Certificate loaded to SSL and Cypher.\n");
					r = TRUE;
				}
			}
			NBString_release(&pass);
		}
		NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &sharedKeyEnc, sizeof(sharedKeyEnc));
	}
	NBPKey_release(&key);
	NBX509_release(&cert);
	NBString_release(&devId);
	return r;
}

SI32 TSClient_loadCurrentDeviceKey(STTSClient* clt){
	return TSClient_loadCurrentDeviceKeyOpq((STTSClientOpq*)clt->opaque);
}

//---------------------------------------

//List data records

void TSClient_getStorageFiles(STTSClient* clt, const char* storagePath, STNBString* dstStrs, STNBArray* dstFiles /*STNBFilesystemFile*/){
	STTSClientOpq* opq = (STTSClientOpq*)clt->opaque;
	TSContext_getStorageFiles(opq->context, storagePath, dstStrs, dstFiles);
}

//Reset all records data

void TSClient_resetLocalData(STTSClient* clt){
	STTSClientOpq* opq = (STTSClientOpq*)clt->opaque;
	//Remove identity key
	TSClient_setCypherCertAndKeyOpq(opq, NULL, NULL, NULL);
	//Remove connections key
	TSClient_setDeviceCertAndKeyOpq(opq, NULL, NULL);
	//Remove all records data
	TSContext_resetAllStorages(opq->context, TRUE);
}

//
// Delete currrent identity (local profile, private keys, certiticates and indentity-block)
//

SI32 TSClient_deleteCurrentIdentityAll(STTSClient* clt){
	BOOL r = TRUE;
	STTSClientOpq* opq = (STTSClientOpq*)clt->opaque;
	{
		STNBString devId;
		NBString_init(&devId);
		//Root
		{
			STNBStorageRecordWrite ref = TSClient_getStorageRecordForWrite(clt, "client/_root/", "_current", TRUE, NULL, TSClientRoot_getSharedStructMap());
			if(ref.data != NULL){
				STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
				NBString_set(&devId, priv->deviceId);
				//Empty root
				NBStruct_stRelease(TSClientRoot_getSharedStructMap(), priv, sizeof(*priv));
				ref.privModified = TRUE;
			}
			TSClient_returnStorageRecordFromWrite(clt, &ref);
		}
		//Device data
		{
			STNBStorageRecordWrite ref = TSClient_getStorageRecordForWrite(clt, "client/_device/", "_current", TRUE, NULL, TSDeviceLocal_getSharedStructMap());
			if(ref.data != NULL){
				STTSDeviceLocal* priv = (STTSDeviceLocal*)ref.data->priv.stData;
				NBStruct_stRelease(TSDeviceLocal_getSharedStructMap(), priv, sizeof(*priv));
				ref.privModified = TRUE;
			}
			TSClient_returnStorageRecordFromWrite(clt, &ref);
		}
		//DeviceCer
		if(devId.length > 0){
			STNBStorageRecordWrite ref = TSClient_getStorageRecordForWrite(clt, "devices/_certs/", devId.str, TRUE, NULL, TSCertificate_getSharedStructMap());
			if(ref.data != NULL){
				STTSCertificate* priv = (STTSCertificate*)ref.data->priv.stData;
				NBStruct_stRelease(TSCertificate_getSharedStructMap(), priv, sizeof(*priv));
				ref.privModified = TRUE;
			}
			TSClient_returnStorageRecordFromWrite(clt, &ref);
		}
		//Remove from cypher and context
		{
			TSClient_setCypherCertAndKeyOpq(opq, NULL, NULL, NULL);
			TSClient_setDeviceCertAndKeyOpq(opq, NULL, NULL);
		}
		NBString_release(&devId);
	}
	return r;
}

//Extract files from package to app's library folder
void TSClient_extractFilesFromPackageToPath(STNBFilesystem* fss, const char* dstPath, const STPkgLoadDef* loadDefs, const SI32 loadDefsSz){
	STNBString filePath;
	NBString_init(&filePath);
	{
		SI32 i; for(i = 0; i < loadDefsSz; i++){
			const STPkgLoadDef* def = &loadDefs[i];
            STNBFileRef dstFile = NBFile_alloc(NULL);
			//Build path
			{
				NBString_empty(&filePath);
				NBString_concat(&filePath, dstPath);
				if(filePath.length > 0){
					if(filePath.str[filePath.length - 1] != '/' && filePath.str[filePath.length - 1] != '\\'){
						NBString_concatByte(&filePath, '/');
					}
				}
				NBString_concat(&filePath, def->path);
			}
			if(NBFile_open(dstFile, filePath.str, ENNBFileMode_Read)){
				PRINTF_INFO("TSClient, TSClient_extractFilesFromPackageToPath, file already exists: '%s'.\n", filePath.str);
			} else {
				NBFile_close(dstFile);
				if(def->data != NULL && def->dataSz > 0){
					//From memory
                    STNBFileRef dstFile = NBFile_alloc(NULL);
					NBFilesystem_createFolderPathOfFile(fss, filePath.str);
					if(!NBFile_open(dstFile, filePath.str, ENNBFileMode_Write)){
						PRINTF_CONSOLE_ERROR("TSClient, TSClient_extractFilesFromPackageToPath, NBFilesystem_openAtRoot failed to write: '%s'.\n", filePath.str);
					} else {
						NBFile_lock(dstFile);
						{
							NBFile_write(dstFile, def->data, def->dataSz);
							PRINTF_INFO("TSClient, TSClient_extractFilesFromPackageToPath, %d bytes written for: '%s'.\n", def->dataSz, filePath.str);
						}
						NBFile_unlock(dstFile);
						NBFile_close(dstFile);
					}
					NBFile_release(&dstFile);
				} else {
					//From file
                    STNBFileRef srcFile = NBFile_alloc(NULL);
					if(!NBFilesystem_openAtRoot(fss, ENNBFilesystemRoot_AppBundle, def->path, ENNBFileMode_Read, srcFile)){
						PRINTF_CONSOLE_ERROR("TSClient, TSClient_extractFilesFromPackageToPath, NBFilesystem_openAtRoot failed for read: '%s'.\n", def->path);
					} else {
						NBFile_lock(srcFile);
						{
                            STNBFileRef dstFile = NBFile_alloc(NULL);
							NBFilesystem_createFolderPathOfFile(fss, filePath.str);
							if(!NBFile_open(dstFile, filePath.str, ENNBFileMode_Write)){
								PRINTF_CONSOLE_ERROR("TSClient, TSClient_extractFilesFromPackageToPath, NBFilesystem_openAtRoot failed to write: '%s'.\n", filePath.str);
							} else {
								NBFile_lock(dstFile);
								{
									BYTE buff[4096];
									SI32 readd = 0, total = 0;
									while(TRUE){
										readd = NBFile_read(srcFile, buff, sizeof(buff));
										if(readd <= 0) break;
										total += readd;
										NBFile_write(dstFile, buff, readd);
									}
									PRINTF_INFO("TSClient, TSClient_extractFilesFromPackageToPath, %d bytes written for: '%s'.\n", total, filePath.str);
								}
								NBFile_unlock(dstFile);
								NBFile_close(dstFile);
							}
							NBFile_release(&dstFile);
						}
						NBFile_unlock(srcFile);
						NBFile_close(srcFile);
					}
					NBFile_release(&srcFile);
				}
			}
			NBFile_release(&dstFile);
		}
	}
	NBString_release(&filePath);
}

//Stats

STTSClientStatsData TSClientStats_getDataLockedOpq_(STTSClientOpq* opq, STNBArray* dstPortsStats /*STNBRtpClientPortStatsData*/, STNBArray* dstSsrcsStats /*STNBRtpClientSsrcStatsData*/, const BOOL resetAccum){
#	ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
	const STTSCfg* cfg			= TSContext_getCfg(opq->context);
#	endif
	STNBDataPtrsPoolRef bPool	= TSContext_getBuffersPool(opq->context);
	STNBDataPtrsStatsRef bStats	= TSContext_getBuffersStats(opq->context);
	STTSClientStatsData r;
	//flush accumulated stats (atomicStats is disabled by default)
	NBDataPtrsPool_flushStats(bPool);
	//build stats
	const STNBDataPtrsStatsData buffsStats			= NBDataPtrsStats_getData(bStats, resetAccum);
#	ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
	const STTSCfgTelemetryProcess* cfgProcess		= &cfg->server.telemetry.process;
	STNBMngrProcessStatsData processStats;
	STNBMngrProcessStatsData* threadStats			= NULL;
	UI32 threadStatsSz								= 0;
	NBMngrProcessStatsData_init(&processStats);
	//load process stats
	{
		NBMemory_setZero(processStats);
		if(cfgProcess->statsLevel > ENNBLogLevel_None){
			NBMngrProcess_getGlobalStatsData(&processStats, cfgProcess->locksByMethod, resetAccum);
		}
		if(cfgProcess->threads != NULL && cfgProcess->threadsSz > 0){
			NBASSERT(threadStatsSz == 0)
			//Count threads with visible data
			{
				UI32 i; for(i = 0; i < cfgProcess->threadsSz; i++){
					const STTSCfgTelemetryThread* cfgT = &cfgProcess->threads[i];
					if(cfgT->statsLevel > ENNBLogLevel_None && !NBString_strIsEmpty(cfgT->firstKnownFunc)){
						threadStatsSz++;
					}
				}
			}
			//Allocate stats
			if(threadStatsSz > 0){
				UI32 threadStatsUse = 0;								
				threadStats = NBMemory_allocTypes(STNBMngrProcessStatsData, threadStatsSz);
				{
					UI32 i; for(i = 0; i < cfgProcess->threadsSz; i++){
						const STTSCfgTelemetryThread* cfgT = &cfgProcess->threads[i];
						if(cfgT->statsLevel > ENNBLogLevel_None && !NBString_strIsEmpty(cfgT->firstKnownFunc)){
							STNBMngrProcessStatsData* d = &threadStats[threadStatsUse++]; NBASSERT(threadStatsUse <= threadStatsSz)
							NBMngrProcessStatsData_init(d);
							NBMngrProcess_getThreadStatsDataByFirstKwnonFuncName(cfgT->firstKnownFunc, d, cfgT->locksByMethod, resetAccum);
						}
					}
				}
				NBASSERT(threadStatsUse == threadStatsSz)
			}
		}
	}
#	endif
	//Process stats data
	{
		NBMemory_setZeroSt(r, STTSClientStatsData);
		//loaded
		r.loaded.buffs		= buffsStats.loaded;
#		ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
		if(cfg->server.telemetry.process.statsLevel > ENNBLogLevel_None){
			r.loaded.process.stats = processStats.alive;
		}
		if(threadStats != NULL && threadStatsSz > 0){
			r.loaded.process.threadsSz	= threadStatsSz;
			r.loaded.process.threads	= NBMemory_allocTypes(STTSClientStatsThreadState, threadStatsSz);
			{
				UI32 i; for(i = 0; i < threadStatsSz; i++){
					STTSClientStatsThreadState* dst = &r.loaded.process.threads[i];
					const STNBMngrProcessStatsData* src = &threadStats[i];
					const STTSCfgTelemetryThread* cfgT = &cfgProcess->threads[i];
					NBMemory_setZeroSt(*dst, STTSClientStatsThreadState);
					dst->name				= cfgT->name;
					dst->firstKnownMethod	= cfgT->firstKnownFunc;
					dst->statsLevel			= cfgT->statsLevel;
					dst->stats				= src->alive; 
				}
			}
		}
#		endif
		//accum
		r.accum.buffs	= buffsStats.accum;
#		ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
		if(cfg->server.telemetry.process.statsLevel > ENNBLogLevel_None){
			r.accum.process.stats			= processStats.accum;
		}
		if(threadStats != NULL && threadStatsSz > 0){
			r.accum.process.threadsSz	= threadStatsSz;
			r.accum.process.threads		= NBMemory_allocTypes(STTSClientStatsThreadState, threadStatsSz);
			{
				UI32 i; for(i = 0; i < threadStatsSz; i++){
					STTSClientStatsThreadState* dst = &r.accum.process.threads[i];
					const STNBMngrProcessStatsData* src = &threadStats[i];
					const STTSCfgTelemetryThread* cfgT = &cfgProcess->threads[i];
					NBMemory_setZeroSt(*dst, STTSClientStatsThreadState);
					dst->name				= cfgT->name;
					dst->firstKnownMethod	= cfgT->firstKnownFunc;
					dst->statsLevel			= cfgT->statsLevel;
					dst->stats				= src->accum; 
				}
			}
		}
#		endif
		//total
		r.total.buffs	= buffsStats.total;
#		ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
		if(cfg->server.telemetry.process.statsLevel > ENNBLogLevel_None){
			r.total.process.stats = processStats.total;
		}
		if(threadStats != NULL && threadStatsSz > 0){
			r.total.process.threadsSz	= threadStatsSz;
			r.total.process.threads	= NBMemory_allocTypes(STTSClientStatsThreadState, threadStatsSz);
			{
				UI32 i; for(i = 0; i < threadStatsSz; i++){
					STTSClientStatsThreadState* dst = &r.total.process.threads[i];
					const STNBMngrProcessStatsData* src = &threadStats[i];
					const STTSCfgTelemetryThread* cfgT = &cfgProcess->threads[i];
					NBMemory_setZeroSt(*dst, STTSClientStatsThreadState);
					dst->name				= cfgT->name;
					dst->firstKnownMethod	= cfgT->firstKnownFunc;
					dst->statsLevel			= cfgT->statsLevel;
					dst->stats				= src->total;
				}
			}
		}
#		endif
	}
	//
#	ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
	{
		//threads
		{
			if(threadStats != NULL){
				if(threadStatsSz > 0){
					UI32 i; for(i = 0; i < threadStatsSz; i++){
						STNBMngrProcessStatsData* d = &threadStats[i];
						NBMngrProcessStatsData_release(d);
					}
				}
				NBMemory_free(threadStats);
				threadStats = NULL;
			}
			threadStatsSz = 0;
		}
		//process
		NBMngrProcessStatsData_release(&processStats);
	}
#	endif
	//
	return r;
}

void TSClientStats_release_(STTSClientStatsData* obj){
	//process
#	ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
	{
		{
			if(obj->loaded.process.threads != NULL){
				NBMemory_free(obj->loaded.process.threads);
				obj->loaded.process.threads = NULL;
			}
			obj->loaded.process.threadsSz = 0;
		}
		{
			if(obj->accum.process.threads != NULL){
				NBMemory_free(obj->accum.process.threads);
				obj->accum.process.threads = NULL;
			}
			obj->accum.process.threadsSz = 0;
		}
		{
			if(obj->total.process.threads != NULL){
				NBMemory_free(obj->total.process.threads);
				obj->total.process.threads = NULL;
			}
			obj->total.process.threadsSz = 0;
		}
	}
#	endif
}

void TSClientStats_concatDataStreamsLocked_(STTSClientOpq* opq, const STTSClientStatsData* obj, const STTSCfgTelemetry* cfg, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, const STNBRtpClientPortStatsData* portsStats, const UI32 portsStatsSz, STNBString* dst){
	if(dst != NULL){
		BOOL opened = FALSE;
		const char* preExtra = "        |- ";
		STNBString str, pre;
		NBString_initWithSz(&str, 0, 256, 0.10f);
		NBString_initWithSz(&pre, NBString_strLenBytes(prefixOthers) + NBString_strLenBytes(preExtra) + 1, 256, 0.10f);
		NBString_concat(&pre, prefixOthers);
		NBString_concat(&pre, preExtra);
		if(loaded){
			NBString_empty(&str);
			TSClientStats_concatStateStreamsLocked_(opq, &obj->loaded, cfg, ENTSClientStartsGrp_Loaded, "", pre.str, ignoreEmpties, portsStats, portsStatsSz, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "loaded:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		if(accum){
			NBString_empty(&str);
			TSClientStats_concatStateStreamsLocked_(opq, &obj->accum, cfg, ENTSClientStartsGrp_Accum, "", pre.str, ignoreEmpties, portsStats, portsStatsSz, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  " accum:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		if(total){
			NBString_empty(&str);
			TSClientStats_concatStateStreamsLocked_(opq, &obj->total, cfg, ENTSClientStartsGrp_Total, "", pre.str, ignoreEmpties, portsStats, portsStatsSz, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  " total:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		NBString_release(&pre);
		NBString_release(&str);
	}
}

void TSClientStats_concatStateStreamsLocked_(STTSClientOpq* opq, const STTSClientStatsState* obj, const STTSCfgTelemetry* cfg, const ENTSClientStartsGrp grp, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, const STNBRtpClientPortStatsData* portsStats, const UI32 portsStatsSz, STNBString* dst){
	if(dst != NULL){
		BOOL opened = FALSE;
		const char* preExtra = "       |- ";
		STNBString str, pre;
		NBString_initWithSz(&str, 0, 256, 0.10f);
		NBString_initWithSz(&pre, NBString_strLenBytes(prefixOthers) + NBString_strLenBytes(preExtra) + 1, 256, 0.10f);
		NBString_concat(&pre, prefixOthers);
		NBString_concat(&pre, preExtra);
		//buffs
		{
			NBString_empty(&str);
			NBDataPtrsStats_concatState(&obj->buffs, &cfg->buffers, (grp == ENTSClientStartsGrp_Loaded), "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "  buffs:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		//threads
		{
#			ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
			NBString_empty(&str);
			TSClientStats_concatProcessState(&obj->process, cfg->process.statsLevel, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "threads:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
#			endif
		}
		//ports
		if(portsStats != NULL && portsStatsSz > 0){
			UI32 i; for(i = 0; i < portsStatsSz; i++){
				const STNBRtpClientPortStatsData* port = &portsStats[i];
				NBString_empty(&str);
				NBRtpClientStats_concatState((grp == ENTSClientStartsGrp_Total ? &port->data.total : grp == ENTSClientStartsGrp_Accum ? &port->data.accum : &port->data.loaded), &cfg->streams.rtsp.rtp, "", pre.str, ignoreEmpties, &str);
				if(str.length > 0){
					if(opened){
						NBString_concat(dst, "\n");
						NBString_concat(dst, prefixOthers);
					} else {
						NBString_concat(dst, prefixFirst);
					}
					//name
					{
						NBString_concat(dst,  "port-");
						NBString_concatUI32(dst, (UI32)port->port);
					}
					NBString_concat(dst,  ": ");
					NBString_concat(dst, str.str);
					opened = TRUE;
				}
			}
		}
		NBString_release(&pre);
		NBString_release(&str);
	}
}

#ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
void TSClientStats_concatProcessState(const STTSClientStatsProcessState* obj, const ENNBLogLevel logLvl, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL){
		BOOL opened = FALSE;
		const char* preExtra = "       |- ";
		STNBString str, pre;
		NBString_initWithSz(&str, 0, 256, 0.10f);
		NBString_initWithSz(&pre, NBString_strLenBytes(prefixOthers) + NBString_strLenBytes(preExtra) + 1, 256, 0.10f);
		NBString_concat(&pre, prefixOthers);
		NBString_concat(&pre, preExtra);
		//global
		{
			NBString_empty(&str);
			NBMngrProcess_concatStatsState(&obj->stats, logLvl, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "global:  ");
				if(str.length <= 0){
					NBString_concat(dst, "(empty)");
				} else {
					NBString_concat(dst, str.str);
				}
				opened = TRUE;
			}
		}
		//threads
		if(obj->threads != NULL && obj->threadsSz > 0){
			UI32 i; for(i = 0; i < obj->threadsSz; i++){
				const STTSClientStatsThreadState* t = &obj->threads[i];
				NBString_empty(&str);
				NBMngrProcess_concatStatsState(&t->stats, t->statsLevel, "", pre.str, ignoreEmpties, &str);
				if(str.length > 0){
					if(opened){
						NBString_concat(dst, "\n");
						NBString_concat(dst, prefixOthers);
					} else {
						NBString_concat(dst, prefixFirst);
					}
					NBString_concat(dst, t->name);
					NBString_concat(dst,  ": ");
					if(str.length <= 0){
						NBString_concat(dst, "(empty)");
					} else {
						NBString_concat(dst, str.str);
					}
					opened = TRUE;
				}
			}
		}
		NBString_release(&pre);
		NBString_release(&str);
	}
}
#endif
