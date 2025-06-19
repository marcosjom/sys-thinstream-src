//
//  DRStMailboxes.c
//  thinstream
//
//  Created by Marcos Ortega on 1/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSFirebaseCMClt.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/net/NBHttpRequest.h"
#include "nb/net/NBHttpClient.h"
#include "nb/net/NBHttp2Parser.h"
#include "nb/net/NBHttp2Hpack.h"
#include "nb/net/NBHttp2Peer.h"
#include "nb/net/NBSocket.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/crypto/NBPkcs12.h"
#include "nb/ssl/NBSsl.h"
#include "nb/ssl/NBSslContext.h"

typedef struct STTSFirebaseMessageNotif_ {
	STNBString	deviceToken;
	STNBString	text;
	STNBString	extraDataName;
	STNBString	extraDataValue;
} STTSFirebaseMessageNotif;

typedef struct STTSFirebaseCMCltOpq_ {
	BOOL				isConfigured;
	STTSCfgFirebaseCM	cfg;
	//Network
	struct {
		BOOL			isConnected;
		UI64			lstConnStartTime;
		UI64			lstConnSecsDuration;
		UI32			notifsSentCount;
		UI32			notifsErrCount;
		STNBPKey		key;
		STNBX509		cert;
		STNBSslContextRef sslContext;
		STNBSslRef		ssl;
		STNBSocketRef	socket;
		STNBThreadMutex	mutex;
	} net;
	//Thread
	struct {
		STNBArray		jobs;			//STTSFirebaseMessageNotif
		STNBThreadMutex	mutex;
		STNBThreadCond	cond;
		BOOL			stopFlag;
		UI32			isRunning;
	} thread;
} STTSFirebaseCMCltOpq;

void TSFirebaseCMClt_init(STTSFirebaseCMClt* obj){
	STTSFirebaseCMCltOpq* opq = obj->opaque = NBMemory_allocType(STTSFirebaseCMCltOpq);
	NBMemory_setZeroSt(*opq, STTSFirebaseCMCltOpq);
	//Network
	{
		NBPKey_init(&opq->net.key);
		NBX509_init(&opq->net.cert);
		opq->net.sslContext = NBSslContext_alloc(NULL);
		NBThreadMutex_init(&opq->net.mutex);
	}
	//Jobs
	{
		NBArray_init(&opq->thread.jobs, sizeof(STTSFirebaseMessageNotif), NULL);
		NBThreadMutex_init(&opq->thread.mutex);
		NBThreadCond_init(&opq->thread.cond);
	}
}

void TSFirebaseCMClt_stopLockedOpq(STTSFirebaseCMCltOpq* opq);

void TSFirebaseCMClt_release(STTSFirebaseCMClt* obj){
	STTSFirebaseCMCltOpq* opq = (STTSFirebaseCMCltOpq*)obj->opaque;
	//Wait for jobs
	{
		NBThreadMutex_lock(&opq->thread.mutex);
		TSFirebaseCMClt_stopLockedOpq(opq);
		//Release
		if(NBSsl_isSet(opq->net.ssl)){
			NBSsl_shutdown(opq->net.ssl);
			NBSsl_release(&opq->net.ssl);
			NBSsl_null(&opq->net.ssl);
		}
		if(NBSocket_isSet(opq->net.socket)){
			NBSocket_shutdown(opq->net.socket, NB_IO_BITS_RDWR);
			NBSocket_release(&opq->net.socket);
			NBSocket_null(&opq->net.socket);
		}
		//Release all pending jobs
		{
			SI32 i; for(i = 0; i < opq->thread.jobs.use; i++){
				STTSFirebaseMessageNotif* j = NBArray_itmPtrAtIndex(&opq->thread.jobs, STTSFirebaseMessageNotif, i);
				NBString_release(&j->deviceToken);
				NBString_release(&j->text);
				NBString_release(&j->extraDataName);
				NBString_release(&j->extraDataValue);
			}
			NBArray_empty(&opq->thread.jobs);
			NBArray_release(&opq->thread.jobs);
		}
		NBThreadMutex_unlock(&opq->thread.mutex);
		NBThreadMutex_release(&opq->thread.mutex);
		NBThreadCond_release(&opq->thread.cond);
		opq->thread.isRunning	= FALSE;
		opq->thread.stopFlag	= FALSE;
	}
	//
	NBStruct_stRelease(TSCfgFirebaseCM_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
	//Network
	{
		{
			//Disconnect
			if(NBSocket_isSet(opq->net.socket)){
				NBSocket_shutdown(opq->net.socket, NB_IO_BITS_RDWR);
				NBSocket_close(opq->net.socket);
			}
			//Release
			if(NBSsl_isSet(opq->net.ssl)){
				NBSsl_shutdown(opq->net.ssl);
				NBSsl_release(&opq->net.ssl);
				NBSsl_null(&opq->net.ssl);
			}
			if(NBSocket_isSet(opq->net.socket)){
				NBSocket_shutdown(opq->net.socket, NB_IO_BITS_RDWR);
				NBSocket_release(&opq->net.socket);
				NBSocket_null(&opq->net.socket);
			}
		}
		NBSslContext_release(&opq->net.sslContext);
		NBX509_release(&opq->net.cert);
		NBPKey_release(&opq->net.key);
		NBThreadMutex_release(&opq->net.mutex);
	}
	NBMemory_free(obj->opaque);
	obj->opaque = NULL;
}

BOOL TSFirebaseCMClt_loadFromConfig(STTSFirebaseCMClt* obj, const STTSCfgFirebaseCM* cfg){
	BOOL r = FALSE;
	STTSFirebaseCMCltOpq* opq = (STTSFirebaseCMCltOpq*)obj->opaque;
	if(cfg != NULL && !opq->isConfigured){
		if(!(NBString_strIsEmpty(cfg->server) && cfg->port <= 0)){
			r = TRUE;
			if(cfg->useSSL){
				r = FALSE;
				if(!NBString_strIsEmpty(cfg->cert.key.path) && !NBString_strIsEmpty(cfg->cert.key.pass)){
					r = FALSE;
					{
						STNBPkcs12 p12;
						NBPkcs12_init(&p12);
						if(!NBPkcs12_createFromDERFile(&p12, cfg->cert.key.path)){
							PRINTF_CONSOLE_ERROR("TSFirebaseCMClt, could not load p12: '%s'.\n", cfg->cert.key.path);
						} else if(!NBPkcs12_getCertAndKey(&p12, &opq->net.key, &opq->net.cert, cfg->cert.key.pass)){
							PRINTF_CONSOLE_ERROR("TSFirebaseCMClt, could not extract cert/key from p12: '%s'.\n", cfg->cert.key.path);
						} else if(!NBSslContext_create(opq->net.sslContext, NBSslContext_getClientMode)){
							PRINTF_CONSOLE_ERROR("TSFirebaseCMClt, could not create the ssl context: '%s'.\n", cfg->cert.key.path);
						} else if(!NBSslContext_attachCertAndkey(opq->net.sslContext, &opq->net.cert, &opq->net.key)){
							PRINTF_CONSOLE_ERROR("TSFirebaseCMClt, could not attach cert/key to ssl context: '%s'.\n", cfg->cert.key.path);
						} else {
							PRINTF_INFO("TSFirebaseCMClt, loaded with key: '%s'.\n", cfg->cert.key.path);
							r = TRUE;
						}
						NBPkcs12_release(&p12);
					}
				} else {
					if(!NBSslContext_create(opq->net.sslContext, NBSslContext_getClientMode)){
						PRINTF_CONSOLE_ERROR("TSFirebaseCMClt, could not create the ssl context without key.\n");
					} else {
						PRINTF_INFO("TSFirebaseCMClt, loaded without key.\n");
						r = TRUE;
					}
				}
			}
			//
			if(r){
				NBStruct_stRelease(TSCfgFirebaseCM_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
				NBStruct_stClone(TSCfgFirebaseCM_getSharedStructMap(), cfg, sizeof(*cfg), &opq->cfg, sizeof(opq->cfg));
				opq->isConfigured = TRUE;
			}
		}
	}
	return r;
}

BOOL TSFirebaseCMClt_addNotification(STTSFirebaseCMClt* obj, const char* token, const char* text, const char* extraDataName, const char* extraDataValue){
	BOOL r = FALSE;
	STTSFirebaseCMCltOpq* opq = (STTSFirebaseCMCltOpq*)obj->opaque;
	if(opq->isConfigured){
		NBThreadMutex_lock(&opq->thread.mutex);
		{
			STTSFirebaseMessageNotif n;
			NBString_initWithStr(&n.deviceToken, token);
			NBString_initWithStr(&n.text, text);
			NBString_initWithStr(&n.extraDataName, extraDataName);
			NBString_initWithStr(&n.extraDataValue, extraDataValue);
			NBArray_addValue(&opq->thread.jobs, n);
			r = TRUE;
		}
		NBThreadCond_broadcast(&opq->thread.cond);
		NBThreadMutex_unlock(&opq->thread.mutex);
	}
	return r;
}

void TSFirebaseCMClt_stopFlag(STTSFirebaseCMClt* obj){
	STTSFirebaseCMCltOpq* opq = (STTSFirebaseCMCltOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->thread.mutex);
	{
		opq->thread.stopFlag = TRUE;
		NBThreadCond_broadcast(&opq->thread.cond);
	}
	NBThreadMutex_unlock(&opq->thread.mutex);
}

void TSFirebaseCMClt_stop(STTSFirebaseCMClt* obj){
	STTSFirebaseCMCltOpq* opq = (STTSFirebaseCMCltOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->thread.mutex);
	TSFirebaseCMClt_stopLockedOpq(opq);
	NBThreadMutex_unlock(&opq->thread.mutex);
}

void TSFirebaseCMClt_stopLockedOpq(STTSFirebaseCMCltOpq* opq){
	while(opq->thread.isRunning){
		{
			opq->thread.stopFlag = TRUE;
			//Disconnect
			//if(NBSocket_isSet(opq->net.socket)){
			//	NBSocket_shutdown(opq->net.socket, NB_IO_BITS_RDWR);
			//	NBSocket_close(opq->net.socket);
			//}
			NBThreadCond_broadcast(&opq->thread.cond);
		}
		NBThreadCond_wait(&opq->thread.cond, &opq->thread.mutex);
	}
}

//Jobs

SI64 TSFirebaseCMClt_sendNotifsMethd_(STNBThread* t, void* param){
	SI64 r = 0;
	//
	if(param != NULL){
		STTSFirebaseCMCltOpq* opq = (STTSFirebaseCMCltOpq*)param;
		NBASSERT(opq->thread.isRunning)
		NBASSERT(!opq->net.isConnected)
		NBASSERT(opq->net.lstConnStartTime == 0)
		NBASSERT(opq->net.lstConnSecsDuration == 0)
		NBASSERT(opq->net.notifsSentCount == 0)
		NBASSERT(opq->net.notifsErrCount == 0)
		{
			UI32 streamId = 1;
			const char* httpBoundTag = "aaabbbcccdddAUClient";
			STNBString frameBuff, payBuff;
			NBString_initWithSz(&frameBuff, 4096, 4096, 0.1f);
			NBString_initWithSz(&payBuff, 4096, 4096, 0.1f);
			{
				NBThreadMutex_lock(&opq->thread.mutex);
				NBASSERT(opq->thread.isRunning)
				opq->thread.isRunning = TRUE;
				while(!opq->thread.stopFlag){
					//Wait for new jobs
					while(!opq->thread.stopFlag && opq->thread.jobs.use == 0){
						if(opq->net.isConnected){
							const UI64 curTime = NBDatetime_getCurUTCTimestamp();
							if(opq->net.lstConnStartTime < curTime){
								opq->net.lstConnSecsDuration = (curTime - opq->net.lstConnStartTime);
							}
						}
						NBThreadCond_wait(&opq->thread.cond, &opq->thread.mutex);
					}
					//Process job
					if(!opq->thread.stopFlag && opq->thread.jobs.use > 0){
						//Extract job
						STTSFirebaseMessageNotif j = NBArray_itmValueAtIndex(&opq->thread.jobs, STTSFirebaseMessageNotif, 0);
						//Remove job (will be released after processed)
						NBArray_removeItemAtIndex(&opq->thread.jobs, 0);
						//Process job (unlocked)
						{
							NBThreadMutex_unlock(&opq->thread.mutex);
							{
								SI32 attameptCount = 0, attemptsAllowed = 2;
								while(!opq->thread.stopFlag && attemptsAllowed  > 0){
									attameptCount++;
									if(attameptCount > 1){
										PRINTF_CONSOLE_ERROR("TSFirebaseCMClt, executing attempt #%d for deviceToken '%s'.\n", attameptCount, j.deviceToken.str);
									}
									//Connect
									if(!opq->net.isConnected){
										streamId		= 1; //start streams count
										attemptsAllowed	= 0; //if connection fails, do not retry
										//Release previous conn
										{
											NBASSERT((NBSsl_isSet(opq->net.ssl) && NBSocket_isSet(opq->net.socket)) || (!NBSsl_isSet(opq->net.ssl) && !NBSocket_isSet(opq->net.socket)))
											if(NBSsl_isSet(opq->net.ssl) && NBSocket_isSet(opq->net.socket)){
												//Disconnect
												if(NBSocket_isSet(opq->net.socket)){
													NBSocket_shutdown(opq->net.socket, NB_IO_BITS_RDWR);
													NBSocket_close(opq->net.socket);
												}
												//Release ssl
												if(NBSsl_isSet(opq->net.ssl)){
													NBSsl_shutdown(opq->net.ssl);
													NBSsl_release(&opq->net.ssl);
													NBSsl_null(&opq->net.ssl);
												}
												//Release socket
												if(NBSocket_isSet(opq->net.socket)){
													NBSocket_shutdown(opq->net.socket, NB_IO_BITS_RDWR);
													NBSocket_release(&opq->net.socket);
													NBSocket_null(&opq->net.socket);
												}
											}
										}
										//Start new conn
										NBASSERT(!NBSsl_isSet(opq->net.ssl))
										NBASSERT(!NBSocket_isSet(opq->net.socket))
										opq->net.socket	= NBSocket_alloc(NULL);
										opq->net.ssl	= NBSsl_alloc(NULL);
										NBSocket_createHnd(opq->net.socket);
										NBSocket_setNoSIGPIPE(opq->net.socket, TRUE);
										NBSocket_setCorkEnabled(opq->net.socket, FALSE);
										NBSocket_setDelayEnabled(opq->net.socket, FALSE);
										if(!NBSocket_connect(opq->net.socket, opq->cfg.server, opq->cfg.port)){
											PRINTF_CONSOLE_ERROR("TSFirebaseCMClt, could not connect to '%s:%d'.\n", opq->cfg.server, opq->cfg.port);
										} else if(!NBSsl_connect(opq->net.ssl, opq->net.sslContext, opq->net.socket)){
											PRINTF_CONSOLE_ERROR("TSFirebaseCMClt, could not negotiate ssl to '%s:%d'.\n", opq->cfg.server, opq->cfg.port);
										} else {
											opq->net.isConnected			= TRUE;
											opq->net.lstConnStartTime		= NBDatetime_getCurUTCTimestamp();
											opq->net.lstConnSecsDuration	= 0;
										}
									}
									//Send notification
									if(opq->net.isConnected){
										BOOL errFnd = FALSE;
										attemptsAllowed--; //decrease attempt
										//Send HEADER
										if(!errFnd){
											STNBHttpRequest req;
											NBHttpRequest_init(&req);
											//Build request
											{
												STNBString strTmp;
												NBString_init(&strTmp);
												//Add headers
												{
													NBHttpRequest_setContentType(&req, "application/json");
													{
														NBString_empty(&strTmp);
														NBString_concat(&strTmp, "key=");
														NBString_concat(&strTmp, opq->cfg.key);
														NBHttpRequest_addHeader(&req, "Authorization", strTmp.str);
													}
												}
												//Add body
												{
													NBString_empty(&strTmp);
													NBString_concat(&strTmp, "{");
													NBString_concat(&strTmp, "\"to\": \""); NBJson_concatScaped(&strTmp, j.deviceToken.str); NBString_concat(&strTmp, "\"");
													//Notification
													{
														NBString_concat(&strTmp, ", \"notification\": {");
														NBString_concat(&strTmp, "\"body\": \""); NBJson_concatScaped(&strTmp, j.text.str); NBString_concat(&strTmp, "\"");
														NBString_concat(&strTmp, "}");
													}
													//Data
													{
														//ToDo: implement 'data' node
													}
													NBString_concat(&strTmp, "}");
													NBHttpRequest_setContent(&req, strTmp.str);
												}
												NBString_release(&strTmp);
											}
											{
												STNBString buff;
												//PRINTF_INFO("Connected to '%s':%d (ssl: %s).\n", data->server.str, data->serverPort, (data->useSSL ? "yes" : "no"));
												NBString_initWithSz(&buff, 4096, 4096, 0.1f);
												//Concat http header
												NBHttpRequest_concatHeaderStart(&req, &buff, opq->cfg.server, "/fcm/send", NULL, httpBoundTag);
												//PRINTF_INFO("TSFirebaseCMClt, sending http request:\n%s\n", buff.str);
												if(!NBSsl_write(opq->net.ssl, buff.str, buff.length)){
													PRINTF_CONSOLE_ERROR("TSFirebaseCMClt, could not send header to '%s:%d'.\n", opq->cfg.server, opq->cfg.port);
													errFnd = TRUE;
												} else {
													NBString_empty(&buff);
													//Send BODY
													{
														void* dataToSend = NULL;
														UI32 dataToSendSz = 0;
														UI32 iStep = 0;
														while(NBHttpRequest_concatBodyContinue(&req, &buff, httpBoundTag, &dataToSend, &dataToSendSz, iStep++)){
															if(dataToSendSz > 0){
																if(!NBSsl_write(opq->net.ssl, dataToSend, dataToSendSz)){
																	PRINTF_CONSOLE_ERROR("TSFirebaseCMClt, could not send body part to '%s:%d'.\n", opq->cfg.server, opq->cfg.port);
																	errFnd = TRUE;
																	break;
																} else {
																	STNBString strTmp;
																	NBString_initWithSz(&strTmp, dataToSendSz, 1024, 0.10f);
																	NBString_concatBytes(&strTmp, dataToSend, dataToSendSz);
																	//PRINTF_INFO("TSFirebaseCMClt, sending http request:\n%s\n", strTmp.str);
																	NBString_release(&strTmp);
																}
															}
															NBString_empty(&buff);
														}
													}
												}
												//
												NBString_release(&buff);
											}
											NBHttpRequest_release(&req);
										}
										//Receive response
										if(!errFnd){
											STNBHttpResponse resp;
											NBHttpResponse_init(&resp);
											{
												UI32 respTotalBytes = 0;
												char buff[1024];
												do {
													const SI32 rcvd = NBSsl_read(opq->net.ssl, buff, 1024);
													if(rcvd <= 0){
														PRINTF_WARNING("TSFirebaseCMClt, stream closed by server after (%d bytes received).\n", respTotalBytes);
														break;
													} else {
														respTotalBytes += (UI32)rcvd;
														if(!NBHttpResponse_consumeBytes(&resp, buff, rcvd, "HTTP")){
															break;
														} else if(resp._transfEnded){
															break;
														}
													}
												} while(TRUE);
												//Result
												if(!resp._transfEnded){
													errFnd = TRUE;
												} else {
													//PRINT ERROR
													if(!(resp.code >= 200 && resp.code < 300)){
														PRINTF_CONSOLE_ERROR("TSFirebaseCMClt, %d bytes response received:\n%s%s\n", respTotalBytes, resp.header.str, resp.body.str);
													}
												}
											}
											NBHttpResponse_release(&resp);
										}
										//Force a reconnect
										if(errFnd){
											opq->net.isConnected = FALSE;
											opq->net.notifsErrCount++;
											PRINTF_INFO("TSFirebaseCMClt, notifsSentCount(%d) notifsErrCount++(%d).\n", opq->net.notifsSentCount, opq->net.notifsErrCount);
										} else {
											streamId += 2; //New streamId (must be odd)
											opq->net.notifsSentCount++;
											PRINTF_INFO("TSFirebaseCMClt, notifsSentCount++(%d) notifsErrCount(%d).\n", opq->net.notifsSentCount, opq->net.notifsErrCount);
											//Update conn duration
											{
												const UI64 curTime = NBDatetime_getCurUTCTimestamp();
												if(opq->net.lstConnStartTime < curTime){
													opq->net.lstConnSecsDuration = (curTime - opq->net.lstConnStartTime);
												}
											}
										}
									}
								}
							}
							NBThreadMutex_lock(&opq->thread.mutex);
						}
						//Release job
						{
							NBString_release(&j.deviceToken);
							NBString_release(&j.text);
							NBString_release(&j.extraDataName);
							NBString_release(&j.extraDataValue);
						}
					}
				}
				NBASSERT(opq->thread.isRunning)
				opq->thread.isRunning = FALSE;
				NBThreadCond_broadcast(&opq->thread.cond);
				NBThreadMutex_unlock(&opq->thread.mutex);
			}
			NBString_release(&payBuff);
			NBString_release(&frameBuff);
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

BOOL TSFirebaseCMClt_prepare(STTSFirebaseCMClt* obj){
	BOOL r = FALSE;
	STTSFirebaseCMCltOpq* opq = (STTSFirebaseCMCltOpq*)obj->opaque;
	{
		NBThreadMutex_lock(&opq->thread.mutex);
		if(opq->thread.isRunning){
			PRINTF_CONSOLE_ERROR("Could not start TSFirebaseCMCltThread, already running.\n");
		} else if(!opq->isConfigured){
			PRINTF_CONSOLE_ERROR("Could not start TSFirebaseCMCltThread, not configured yet.\n");
		} else {
			NBASSERT(!opq->net.isConnected)
			r = TRUE;
			//Start send thread
			if(r){
				STNBThread* t = NBMemory_allocType(STNBThread);
				NBThread_init(t);
				NBThread_setIsJoinable(t, FALSE);
				opq->thread.isRunning = TRUE;
				if(!NBThread_start(t, TSFirebaseCMClt_sendNotifsMethd_, opq, NULL)){
					PRINTF_CONSOLE_ERROR("Could not start TSFirebaseCMClt_sendNotifsMethd_.\n");
					opq->thread.isRunning = FALSE;
					r = FALSE;
				} else {
					PRINTF_INFO("TSFirebaseCMClt_sendNotifsMethd_ started.\n");
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
