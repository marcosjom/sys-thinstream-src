//
//  DRStMailboxes.c
//  thinstream
//
//  Created by Marcos Ortega on 1/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSApplePushServicesClt.h"
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

typedef struct STTSApplePushServicesNotif_ {
	STNBString	deviceToken;
	STNBString	text;
	STNBString	extraDataName;
	STNBString	extraDataValue;
} STTSApplePushServicesNotif;

typedef struct STTSApplePushServicesCltOpq_ {
	BOOL				isConfigured;
	STTSCfgApplePushServices cfg;
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
		STNBHttp2Peer	peer;
		STNBThreadMutex	mutex;
	} net;
	//Thread
	struct {
		STNBArray		jobs;			//STTSApplePushServicesNotif
		STNBThreadMutex	mutex;
		STNBThreadCond	cond;
		BOOL			stopFlag;
		UI32			isRunning;
	} thread;
} STTSApplePushServicesCltOpq;

void TSApplePushServicesClt_init(STTSApplePushServicesClt* obj){
	STTSApplePushServicesCltOpq* opq = obj->opaque = NBMemory_allocType(STTSApplePushServicesCltOpq);
	NBMemory_setZeroSt(*opq, STTSApplePushServicesCltOpq);
	//Network
	{
		NBPKey_init(&opq->net.key);
		NBX509_init(&opq->net.cert);
		opq->net.sslContext = NBSslContext_alloc(NULL);
		NBHttp2Peer_init(&opq->net.peer);
		NBThreadMutex_init(&opq->net.mutex);
	}
	//Jobs
	{
		NBArray_init(&opq->thread.jobs, sizeof(STTSApplePushServicesNotif), NULL);
		NBThreadMutex_init(&opq->thread.mutex);
		NBThreadCond_init(&opq->thread.cond);
	}
}

void TSApplePushServicesClt_stopLockedOpq(STTSApplePushServicesCltOpq* opq);

void TSApplePushServicesClt_release(STTSApplePushServicesClt* obj){
	STTSApplePushServicesCltOpq* opq = (STTSApplePushServicesCltOpq*)obj->opaque;
	//Wait for jobs
	{
		NBThreadMutex_lock(&opq->thread.mutex);
		TSApplePushServicesClt_stopLockedOpq(opq);
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
				STTSApplePushServicesNotif* j = NBArray_itmPtrAtIndex(&opq->thread.jobs, STTSApplePushServicesNotif, i);
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
	NBStruct_stRelease(TSCfgApplePushServices_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
	//Network
	{
		NBHttp2Peer_release(&opq->net.peer);
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

BOOL TSApplePushServicesClt_loadFromConfig(STTSApplePushServicesClt* obj, const STTSCfgApplePushServices* cfg){
	BOOL r = FALSE;
	STTSApplePushServicesCltOpq* opq = (STTSApplePushServicesCltOpq*)obj->opaque;
	if(cfg != NULL && !opq->isConfigured){
		if(!(NBString_strIsEmpty(cfg->server) && cfg->port <= 0)){
			r = TRUE;
			if(cfg->useSSL){
				r = FALSE;
				if(!NBString_strIsEmpty(cfg->cert.key.path) && !NBString_strIsEmpty(cfg->cert.key.pass)){
					STNBPkcs12 p12;
					NBPkcs12_init(&p12);
					if(!NBPkcs12_createFromDERFile(&p12, cfg->cert.key.path)){
						PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, could not load p12: '%s'.\n", cfg->cert.key.path);
					} else if(!NBPkcs12_getCertAndKey(&p12, &opq->net.key, &opq->net.cert, cfg->cert.key.pass)){
						PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, could not extract cert/key from p12: '%s'.\n", cfg->cert.key.path);
					} else if(!NBSslContext_create(opq->net.sslContext, NBSslContext_getClientMode)){
						PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, could not create the ssl context: '%s'.\n", cfg->cert.key.path);
					} else if(!NBSslContext_attachCertAndkey(opq->net.sslContext, &opq->net.cert, &opq->net.key)){
						PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, could not attach cert/key to ssl context: '%s'.\n", cfg->cert.key.path);
					} else {
						PRINTF_INFO("TSApplePushServicesClt, loaded with key: '%s'.\n", cfg->cert.key.path);
						r = TRUE;
					}
					NBPkcs12_release(&p12);
				} else {
					if(!NBSslContext_create(opq->net.sslContext, NBSslContext_getClientMode)){
						PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, could not create the ssl context wihtout key.\n");
					} else {
						PRINTF_INFO("TSApplePushServicesClt, loaded without key.\n");
						r = TRUE;
					}
				}
			}
			//
			if(r){
				NBStruct_stRelease(TSCfgApplePushServices_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
				NBStruct_stClone(TSCfgApplePushServices_getSharedStructMap(), cfg, sizeof(*cfg), &opq->cfg, sizeof(opq->cfg));
				opq->isConfigured = TRUE;
			}
		}
	}
	return r;
}

BOOL TSApplePushServicesClt_addNotification(STTSApplePushServicesClt* obj, const char* token, const char* text, const char* extraDataName, const char* extraDataValue){
	BOOL r = FALSE;
	STTSApplePushServicesCltOpq* opq = (STTSApplePushServicesCltOpq*)obj->opaque;
	if(opq->isConfigured){
		NBThreadMutex_lock(&opq->thread.mutex);
		{
			STTSApplePushServicesNotif n;
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

void TSApplePushServicesClt_stopFlag(STTSApplePushServicesClt* obj){
	STTSApplePushServicesCltOpq* opq = (STTSApplePushServicesCltOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->thread.mutex);
	{
		opq->thread.stopFlag = TRUE;
		NBThreadCond_broadcast(&opq->thread.cond);
	}
	NBThreadMutex_unlock(&opq->thread.mutex);
}

void TSApplePushServicesClt_stop(STTSApplePushServicesClt* obj){
	STTSApplePushServicesCltOpq* opq = (STTSApplePushServicesCltOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->thread.mutex);
	TSApplePushServicesClt_stopLockedOpq(opq);
	NBThreadMutex_unlock(&opq->thread.mutex);
}

void TSApplePushServicesClt_stopLockedOpq(STTSApplePushServicesCltOpq* opq){
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

SI64 TSApplePushServicesClt_sendNotifsMethd_(STNBThread* t, void* param){
	SI64 r = 0;
	//
	if(param != NULL){
		STTSApplePushServicesCltOpq* opq = (STTSApplePushServicesCltOpq*)param;
		NBASSERT(opq->thread.isRunning)
		NBASSERT(!opq->net.isConnected)
		NBASSERT(opq->net.lstConnStartTime == 0)
		NBASSERT(opq->net.lstConnSecsDuration == 0)
		NBASSERT(opq->net.notifsSentCount == 0)
		NBASSERT(opq->net.notifsErrCount == 0)
		{
			UI32 streamId = 1;
			const char* http2Preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
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
						STTSApplePushServicesNotif j = NBArray_itmValueAtIndex(&opq->thread.jobs, STTSApplePushServicesNotif, 0);
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
										PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, executing attempt #%d for deviceToken '%s'.\n", attameptCount, j.deviceToken.str);
									}
									//Connect
									if(!opq->net.isConnected){
										streamId		= 1; //start streams count
										attemptsAllowed	= 0; //if connection fails, do not retry
										//Release previous conn
										{
											
											NBHttp2Peer_release(&opq->net.peer);
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
										NBHttp2Peer_init(&opq->net.peer);
										if(!NBSocket_connect(opq->net.socket, opq->cfg.server, opq->cfg.port)){
											PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, could not connect to '%s:%d'.\n", opq->cfg.server, opq->cfg.port);
										} else if(!NBSsl_connect(opq->net.ssl, opq->net.sslContext, opq->net.socket)){
											PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, could not negotiate ssl to '%s:%d'.\n", opq->cfg.server, opq->cfg.port);
										} else {
											BOOL errFnd = FALSE;
											//Send PREFACE
											if(!errFnd){
												const UI32 prefaceLen = NBString_strLenBytes(http2Preface);
												NBASSERT(prefaceLen == 24)
												if(!NBSsl_write(opq->net.ssl, http2Preface, prefaceLen)){
													PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, send PREFACE failed.\n");
													errFnd = TRUE;
												} else {
													PRINTF_INFO("TSApplePushServicesClt, PREFACE sent.\n");
												}
											}
											//Send SETTINGS frame
											if(!errFnd){
												if(!NBHttp2Peer_sendSettings(&opq->net.peer, opq->net.ssl, 0x0 /*mask*/)){
													PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, NBHttp2Parser_settingsToBuff failed.\n");
													errFnd = TRUE;
												} else {
													PRINTF_INFO("TSApplePushServicesClt, SETTINGS sent.\n");
													//Receive SEETINGS
													if(!NBHttp2Peer_receiveSettings(&opq->net.peer, opq->net.ssl)){
														PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, NBHttp2Peer_receiveSettings failed.\n");
														errFnd = TRUE;
													} else {
														PRINTF_INFO("TSApplePushServicesClt, SETTINGS received.\n");
														if(!NBHttp2Peer_receiveSettingsAck(&opq->net.peer, opq->net.ssl)){
															PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, NBHttp2Peer_receiveSettingsAck failed.\n");
															errFnd = TRUE;
														} else {
															PRINTF_INFO("TSApplePushServicesClt, SETTINGS ACK received.\n");
															if(!NBHttp2Peer_sendSettingsAck(&opq->net.peer, opq->net.ssl)){
																PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, NBHttp2Peer_sendSettingsAck failed.\n");
																errFnd = TRUE;
															} else {
																PRINTF_INFO("TSApplePushServicesClt, SETTINGS ACK sent.\n");
																opq->net.isConnected			= TRUE;
																opq->net.lstConnStartTime		= NBDatetime_getCurUTCTimestamp();
																opq->net.lstConnSecsDuration	= 0;
															}
														}
													}
												}
											}
										}
									}
									//Send notification
									if(opq->net.isConnected){
										BOOL errFnd = FALSE;
										attemptsAllowed--; //decrease attempt
										//Send HEADERS frame
										if(!errFnd){
											STNBHttpHeader list;
											NBHttpHeader_init(&list);
											NBString_empty(&payBuff);
											NBString_concat(&payBuff, "/3/device/");
											NBString_concat(&payBuff, j.deviceToken.str);
											{
												NBHttpHeader_addField(&list, ":method", "POST");
												NBHttpHeader_addField(&list, ":scheme", "https");
												NBHttpHeader_addField(&list, ":path", payBuff.str);
												NBHttpHeader_addField(&list, "host", opq->cfg.server);
												NBHttpHeader_addField(&list, "apns-topic", opq->cfg.apnsTopic);
												const BOOL simulateErr = (FALSE && attemptsAllowed > 0 && ((opq->net.notifsSentCount + opq->net.notifsErrCount) % 2) == 0);
												if(simulateErr){
													PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, simulating error condition on NBHttp2Peer_sendHeaders.\n");
												}
												if(simulateErr || !NBHttp2Peer_sendHeaders(&opq->net.peer, &list, opq->net.ssl, streamId)){
													PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, NBHttp2Peer_sendHeaders failed.\n");
													errFnd = TRUE;
												} else {
													PRINTF_INFO("TSApplePushServicesClt, HEADERS sent.\n");
												}
											}
											NBHttpHeader_release(&list);
										}
										//Send BODY frame
										if(!errFnd){
											NBString_empty(&payBuff);
											NBString_concat(&payBuff, "{");
											NBString_concat(&payBuff, " \"aps\" : { \"badge\": 1, \"sound\": \"default\", \"alert\": { \"body\": \"");
											NBString_concat(&payBuff, j.text.str);
											NBString_concat(&payBuff, "\" } }");
											if(j.extraDataName.length > 0 && j.extraDataValue.length > 0){
												NBString_concat(&payBuff, ", \""); NBString_concat(&payBuff, j.extraDataName.str); NBString_concat(&payBuff, "\": ");
												NBString_concatBytes(&payBuff, j.extraDataValue.str, j.extraDataValue.length);
											}
											NBString_concat(&payBuff, "}");
											if(!NBHttp2Peer_sendData(&opq->net.peer, payBuff.str, payBuff.length, opq->net.ssl, streamId)){
												PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, NBHttp2Peer_sendData failed.\n");
												errFnd = TRUE;
											} else {
												PRINTF_INFO("TSApplePushServicesClt, DATA sent.\n");
												attemptsAllowed = 0; //sucess, do not retry
											}
										}
										//Receive responses until END_STREAM is found
										while(!errFnd){
											STHttp2FrameHead head;
											NBMemory_setZeroSt(head, STHttp2FrameHead);
											NBString_empty(&payBuff);
											if(!NBHttp2Peer_receiveFrame(&opq->net.peer, opq->net.ssl, &head, &payBuff)){
												PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, NBHttp2Peer_receiveFrame failed.\n");
												errFnd = TRUE;
											} else if(!NBHttp2Peer_processFrame(&opq->net.peer, &head, payBuff.str, payBuff.length)){
												PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, NBHttp2Peer_processFrame failed.\n");
												errFnd = TRUE;
											} else {
												if(head.type == ENNBHttp2FrameType_SETTINGS){
													//Send SETTINGS ACK
													NBASSERT((head.flag & ENNBHttp2FrameFlag_ACK) == 0)
													if(!NBHttp2Peer_sendSettingsAck(&opq->net.peer, opq->net.ssl)){
														PRINTF_CONSOLE_ERROR("TSApplePushServicesClt, NBHttp2Peer_sendSettingsAck failed.\n");
														errFnd = TRUE;
													} else {
														PRINTF_INFO("TSApplePushServicesClt, NBHttp2Peer_sendSettingsAck sent.\n");
													}
												} else if(head.type == ENNBHttp2FrameType_HEADERS || head.type == ENNBHttp2FrameType_DATA){
													//Check END_STREAM flag
													if((head.flag & ENNBHttp2FrameFlag_END_STREAM) != 0){
														PRINTF_INFO("TSApplePushServicesClt, response END_STREAM found.\n");
														break;
													}
												} else {
													PRINTF_INFO("TSApplePushServicesClt, ignoring received frameType: '%d' (%d bytes).\n", head.type, head.len);
												}
											}
										}
										//Force a reconnect
										if(errFnd){
											opq->net.isConnected = FALSE;
											opq->net.notifsErrCount++;
											PRINTF_INFO("TSApplePushServicesClt, notifsSentCount(%d) notifsErrCount++(%d).\n", opq->net.notifsSentCount, opq->net.notifsErrCount);
										} else {
											streamId += 2; //New streamId (must be odd)
											opq->net.notifsSentCount++;
											PRINTF_INFO("TSApplePushServicesClt, notifsSentCount++(%d) notifsErrCount(%d).\n", opq->net.notifsSentCount, opq->net.notifsErrCount);
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

BOOL TSApplePushServicesClt_prepare(STTSApplePushServicesClt* obj){
	BOOL r = FALSE;
	STTSApplePushServicesCltOpq* opq = (STTSApplePushServicesCltOpq*)obj->opaque;
	{
		NBThreadMutex_lock(&opq->thread.mutex);
		if(opq->thread.isRunning){
			PRINTF_CONSOLE_ERROR("Could not start TSApplePushServicesCltThread, already running.\n");
		} else if(!opq->isConfigured){
			PRINTF_CONSOLE_ERROR("Could not start TSApplePushServicesCltThread, not configured yet.\n");
		} else {
			NBASSERT(!opq->net.isConnected)
			r = TRUE;
			//Start send thread
			if(r){
				STNBThread* t = NBMemory_allocType(STNBThread);
				NBThread_init(t);
				NBThread_setIsJoinable(t, FALSE);
				opq->thread.isRunning = TRUE;
				if(!NBThread_start(t, TSApplePushServicesClt_sendNotifsMethd_, opq, NULL)){
					PRINTF_CONSOLE_ERROR("Could not start TSApplePushServicesClt_sendNotifsMethd_.\n");
					opq->thread.isRunning = FALSE;
					r = FALSE;
				} else {
					PRINTF_INFO("TSApplePushServicesClt_sendNotifsMethd_ started.\n");
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
