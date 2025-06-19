//
//  DRStMailboxes.c
//  thinstream
//
//  Created by Marcos Ortega on 1/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSApiRestClt.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/net/NBHttpRequest.h"
#include "nb/net/NBHttpClient.h"
#include "nb/net/NBSocket.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/ssl/NBSsl.h"
#include "nb/ssl/NBSslContext.h"

typedef struct STTSApiRestReq_ {
	STNBHttpRequest	req;
} STTSApiRestReq;

typedef struct STTSApiRestCltOpq_ {
	BOOL				isConfigured;
	STTSCfgRestApiClt	cfg;
	//Network
	struct {
		BOOL			isConnected;
		UI64			lstConnStartTime;
		UI64			lstConnSecsDuration;
		UI32			smsSentCount;
		UI32			smsErrCount;
		//STNBPKey		key;
		//STNBX509		cert;
		STNBSslContextRef sslContext;
		STNBSslRef		ssl;
		STNBSocketRef	socket;
		STNBThreadMutex	mutex;
	} net;
	//Thread
	struct {
		STNBArray		jobs;			//STTSApiRestReq
		STNBThreadMutex	mutex;
		STNBThreadCond	cond;
		BOOL			stopFlag;
		UI32			isRunning;
	} thread;
} STTSApiRestCltOpq;

void TSApiRestClt_init(STTSApiRestClt* obj){
	STTSApiRestCltOpq* opq = obj->opaque = NBMemory_allocType(STTSApiRestCltOpq);
	NBMemory_setZeroSt(*opq, STTSApiRestCltOpq);
	//
	opq->isConfigured = FALSE;
	//Network
	{
		//NBPKey_init(&opq->net.key);
		//NBX509_init(&opq->net.cert);
		opq->net.sslContext = NBSslContext_alloc(NULL);
		NBThreadMutex_init(&opq->net.mutex);
	}
	//Jobs
	{
		NBArray_init(&opq->thread.jobs, sizeof(STTSApiRestReq), NULL);
		NBThreadMutex_init(&opq->thread.mutex);
		NBThreadCond_init(&opq->thread.cond);
	}
}

void TSApiRestClt_stopLockedOpq(STTSApiRestCltOpq* opq);

void TSApiRestClt_release(STTSApiRestClt* obj){
	STTSApiRestCltOpq* opq = (STTSApiRestCltOpq*)obj->opaque;
	//Wait for jobs
	{
		NBThreadMutex_lock(&opq->thread.mutex);
		TSApiRestClt_stopLockedOpq(opq);
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
				STTSApiRestReq* j = NBArray_itmPtrAtIndex(&opq->thread.jobs, STTSApiRestReq, i);
				NBHttpRequest_release(&j->req);
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
	NBStruct_stRelease(TSCfgRestApiClt_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
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
		//NBX509_release(&opq->net.cert);
		//NBPKey_release(&opq->net.key);
		NBThreadMutex_release(&opq->net.mutex);
	}
	NBMemory_free(obj->opaque);
	obj->opaque = NULL;
}

BOOL TSApiRestClt_loadFromConfig(STTSApiRestClt* obj, const STTSCfgRestApiClt* cfg){
	BOOL r = FALSE;
	STTSApiRestCltOpq* opq = (STTSApiRestCltOpq*)obj->opaque;
	if(cfg != NULL && !opq->isConfigured){
		if(!(NBString_strIsEmpty(cfg->server.server) && cfg->server.port <= 0)){
			r = opq->isConfigured = TRUE;
			if(cfg->server.useSSL){
				r = opq->isConfigured = FALSE;
				if(!NBSslContext_create(opq->net.sslContext, NBSslContext_getClientMode)){
					PRINTF_CONSOLE_ERROR("TSApiRestClt, could not create the ssl context.\n");
				} else {
					r = opq->isConfigured = TRUE;
				}
				/*if(!NBString_strIsEmpty(cfg->cert.key.path) && !NBString_strIsEmpty(cfg->cert.key.pass)){
					r = opq->isConfigured = FALSE;
					{
						STNBPkcs12 p12;
						NBPkcs12_init(&p12);
						if(!NBPkcs12_createFromDERFile(&p12, cfg->cert.key.path)){
							PRINTF_CONSOLE_ERROR("TSApiRestClt, could not load p12: '%s'.\n", cfg->cert.key.path);
						} else if(!NBPkcs12_getCertAndKey(&p12, &opq->net.key, &opq->net.cert, cfg->cert.key.pass)){
							PRINTF_CONSOLE_ERROR("TSApiRestClt, could not extract cert/key from p12: '%s'.\n", cfg->cert.key.path);
						} else if(!NBSslContext_create(&opq->net.sslContext, NBSslContext_getClientMode)){
							PRINTF_CONSOLE_ERROR("TSApiRestClt, could not create the ssl context: '%s'.\n", cfg->cert.key.path);
						} else if(!NBSslContext_attachCertAndkey(&opq->net.sslContext, &opq->net.cert, &opq->net.key)){
							PRINTF_CONSOLE_ERROR("TSApiRestClt, could not attach cert/key to ssl context: '%s'.\n", cfg->cert.key.path);
						} else {
							PRINTF_INFO("TSApiRestClt, loaded with key: '%s'.\n", cfg->cert.key.path);
							r = opq->isConfigured = TRUE;
						}
						NBPkcs12_release(&p12);
					}
				}*/
			}
			//
			if(r){
				NBStruct_stRelease(TSCfgRestApiClt_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
				NBStruct_stClone(TSCfgRestApiClt_getSharedStructMap(), cfg, sizeof(*cfg), &opq->cfg, sizeof(opq->cfg));
			}
		}
	}
	return r;
}

BOOL TSApiRestClt_addRequest(STTSApiRestClt* obj, const STNBHttpRequest* req){
	BOOL r = FALSE;
	STTSApiRestCltOpq* opq = (STTSApiRestCltOpq*)obj->opaque;
	if(opq->isConfigured && req != NULL){
		NBThreadMutex_lock(&opq->thread.mutex);
		{
			STTSApiRestReq n;
			NBHttpRequest_initWithOther(&n.req, req);
			NBArray_addValue(&opq->thread.jobs, n);
			r = TRUE;
		}
		NBThreadCond_broadcast(&opq->thread.cond);
		NBThreadMutex_unlock(&opq->thread.mutex);
	}
	return r;
}


void TSApiRestClt_stopFlag(STTSApiRestClt* obj){
	STTSApiRestCltOpq* opq = (STTSApiRestCltOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->thread.mutex);
	{
		opq->thread.stopFlag = TRUE;
		NBThreadCond_broadcast(&opq->thread.cond);
	}
	NBThreadMutex_unlock(&opq->thread.mutex);
}

void TSApiRestClt_stop(STTSApiRestClt* obj){
	STTSApiRestCltOpq* opq = (STTSApiRestCltOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->thread.mutex);
	TSApiRestClt_stopLockedOpq(opq);
	NBThreadMutex_unlock(&opq->thread.mutex);
}

void TSApiRestClt_stopLockedOpq(STTSApiRestCltOpq* opq){
	while(opq->thread.isRunning){
		{
			opq->thread.stopFlag = TRUE;
			NBThreadCond_broadcast(&opq->thread.cond);
		}
		NBThreadCond_wait(&opq->thread.cond, &opq->thread.mutex);
	}
}

//Jobs

SI64 TSApiRestClt_sendReqMethd_(STNBThread* t, void* param){
	SI64 r = 0;
	//
	if(param != NULL){
		STTSApiRestCltOpq* opq = (STTSApiRestCltOpq*)param;
		NBASSERT(opq->thread.isRunning)
		NBASSERT(!opq->net.isConnected)
		NBASSERT(opq->net.lstConnStartTime == 0)
		NBASSERT(opq->net.lstConnSecsDuration == 0)
		NBASSERT(opq->net.smsSentCount == 0)
		NBASSERT(opq->net.smsErrCount == 0)
		{
			const char* httpBoundTag = "aaabbbcccdddAUClient";
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
						STTSApiRestReq j = NBArray_itmValueAtIndex(&opq->thread.jobs, STTSApiRestReq, 0);
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
										PRINTF_CONSOLE_ERROR("TSApiRestClt, executing attempt #%d.\n", attameptCount);
									}
									//Connect
									if(!opq->net.isConnected){
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
										if(!NBSocket_connect(opq->net.socket, opq->cfg.server.server, opq->cfg.server.port)){
											PRINTF_CONSOLE_ERROR("TSApiRestClt, could not connect to '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
										} else if(!NBSsl_connect(opq->net.ssl, opq->net.sslContext, opq->net.socket)){
											PRINTF_CONSOLE_ERROR("TSApiRestClt, could not negotiate ssl to '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
										} else {
											PRINTF_INFO("TSApiRestClt, connected to '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
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
											STNBString buff;
											//PRINTF_INFO("Connected to '%s':%d (ssl: %s).\n", data->server.str, data->serverPort, (data->useSSL ? "yes" : "no"));
											NBString_initWithSz(&buff, 4096, 4096, 0.1f);
											//Concat http header
											NBHttpRequest_concatHeaderStart(&j.req, &buff, opq->cfg.server.server, opq->cfg.server.reqId, NULL, httpBoundTag);
											//PRINTF_INFO("Sending http request:\n%s\n", strREQUEST.str);
											if(!NBSsl_write(opq->net.ssl, buff.str, buff.length)){
												PRINTF_CONSOLE_ERROR("TSApiRestClt, could not send header to '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
												errFnd = TRUE;
											} else {
												NBString_empty(&buff);
												//Send BODY
												{
													void* dataToSend = NULL;
													UI32 dataToSendSz = 0;
													UI32 iStep = 0;
													while(NBHttpRequest_concatBodyContinue(&j.req, &buff, httpBoundTag, &dataToSend, &dataToSendSz, iStep++)){
														if(dataToSendSz > 0){
															if(!NBSsl_write(opq->net.ssl, dataToSend, dataToSendSz)){
																PRINTF_CONSOLE_ERROR("TSApiRestClt, could not send body part to '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
																errFnd = TRUE;
																break;
															}
														}
														NBString_empty(&buff);
													}
												}
											}
											//
											NBString_release(&buff);
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
														PRINTF_WARNING("TSApiRestClt, stream closed by server after (%d bytes received).\n", respTotalBytes);
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
													//PRINTF_INFO("TSApiRestClt, %d bytes response received.\n", respTotalBytes);
												}
											}
											NBHttpResponse_release(&resp);
										}
										//Force a reconnect
										if(errFnd){
											opq->net.isConnected = FALSE;
											opq->net.smsErrCount++;
											//PRINTF_INFO("TSApiRestClt, smsSentCount(%d) smsErrCount++(%d).\n", opq->net.smsSentCount, opq->net.smsErrCount);
										} else {
											attemptsAllowed = 0; //sucess, do not retry
											opq->net.smsSentCount++;
											//PRINTF_INFO("TSApiRestClt, smsSentCount++(%d) smsErrCount(%d).\n", opq->net.smsSentCount, opq->net.smsErrCount);
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
							NBHttpRequest_release(&j.req);
						}
					}
				}
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

BOOL TSApiRestClt_prepare(STTSApiRestClt* obj){
	BOOL r = FALSE;
	STTSApiRestCltOpq* opq = (STTSApiRestCltOpq*)obj->opaque;
	{
		NBThreadMutex_lock(&opq->thread.mutex);
		if(opq->thread.isRunning){
			PRINTF_CONSOLE_ERROR("Could not start TSApiRestCltThread, already running.\n");
		} else if(!opq->isConfigured){
			PRINTF_CONSOLE_ERROR("Could not start TSApiRestCltThread, not configured yet.\n");
		} else {
			NBASSERT(!opq->net.isConnected)
			r = TRUE;
			//Start send thread
			if(r){
				STNBThread* t = NBMemory_allocType(STNBThread);
				NBThread_init(t);
				NBThread_setIsJoinable(t, FALSE);
				opq->thread.isRunning = TRUE;
				if(!NBThread_start(t, TSApiRestClt_sendReqMethd_, opq, NULL)){
					PRINTF_CONSOLE_ERROR("Could not start TSApiRestClt_sendReqMethd_.\n");
					opq->thread.isRunning = FALSE;
					r = FALSE;
				} else {
					PRINTF_INFO("TSApiRestClt_sendReqMethd_ started.\n");
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
