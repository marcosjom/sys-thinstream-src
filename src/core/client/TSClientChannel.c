//
//  TSClientChannel.c
//  thinstream
//
//  Created by Marcos Ortega on 8/24/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/client/TSClientChannel.h"
//
#include "nb/core/NBStruct.h"
#include "nb/core/NBThread.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBString.h"
#include "nb/ssl/NBSslContext.h"
#include "nb/ssl/NBSsl.h"
#include "nb/net/NBSocket.h"
#include "nb/net/NBHttpBuilder.h"
#include "nb/net/NBHttpReqRules.h"
#include "core/TSRequestsMap.h"

struct STTSClientChannelOpq_;

typedef struct STTSClientChannelConn_ {
	BOOL			allowReconnect;	//allow to reconnect to retry request
	STNBSslRef		ssl;
	STNBSocketRef	socket;
} STTSClientChannelConn;

typedef struct STTSClientChannelOpq_ {
	BOOL					threadWorking;	//worker thread
	BOOL					shuttingDown;
	STNBThreadMutex			mutex;
	STNBThreadCond			cond;
	STNBHttpPortCfg		    cfg;
	const STNBX509*			caCert;
	UI64					reqsCurId;
	void*					clientOpq;
	const STTSRequestsMap* 	reqsMap;
	STNBArray				reqsQueue; //STTSClientChannelRequest*
	STNBArray				reqsToNotify; //STTSClientChannelRequest*
	STTSCypher*				cypher;
	struct {
		STNBSslContextRef	current;
		BOOL				isAuthenticated;
		UI32				seqVer;		//current sequence of context set
	} sslContext;
	struct {
		UI64				curReqUid;
		struct {
			UI32			connTryAccum;	//Acumulated ammout of attempts to connect (reset by request)
			UI32			connTryTotal;	//Total amount of attempts to connect (globaly)
		} stats;
		STTSClientChannelConn d;
		UI32				seqVer;		//current sequence of context used
	} conn;
	STNBString				strs;
} STTSClientChannelOpq;

void TSClientChannel_init(STTSClientChannel* obj){
	STTSClientChannelOpq* opq = obj->opaque = NBMemory_allocType(STTSClientChannelOpq);
	NBMemory_setZeroSt(*opq, STTSClientChannelOpq);
	NBThreadMutex_init(&opq->mutex);
	NBThreadCond_init(&opq->cond);
	NBArray_init(&opq->reqsQueue, sizeof(STTSClientChannelRequest*), NULL);
	NBArray_init(&opq->reqsToNotify, sizeof(STTSClientChannelRequest*), NULL);
	{
		opq->sslContext.seqVer			= 1;
		{
			opq->conn.d.allowReconnect	= TRUE;
		}
	}
	NBString_init(&opq->strs);
	NBString_concatByte(&opq->strs, '\0');
}

void TSClientChannel_release(STTSClientChannel* obj){
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	//Wait for worker thread
	{
		TSClientChannel_startShutdown(obj);
		TSClientChannel_waitForAll(obj);
	}
	opq->caCert		= NULL;
	{
		opq->sslContext.current			= NB_OBJREF_NULL;
		opq->sslContext.isAuthenticated	= FALSE;
		//stream
		{
			if(NBSsl_isSet(opq->conn.d.ssl)){
				NBSsl_release(&opq->conn.d.ssl);
				NBSsl_null(&opq->conn.d.ssl);
			}
			if(NBSocket_isSet(opq->conn.d.socket)){
				NBSocket_shutdown(opq->conn.d.socket, NB_IO_BITS_RDWR);
				NBSocket_release(&opq->conn.d.socket);
				NBSocket_null(&opq->conn.d.socket);
			}
		}
	}
	opq->cypher		= NULL;
	opq->clientOpq	= NULL;
	opq->reqsMap	= NULL;
	{
		NBThreadMutex_lock(&opq->mutex);
		UI32 i; for(i = 0; i < opq->reqsQueue.use; i++){
			STTSClientChannelRequest* req = NBArray_itmValueAtIndex(&opq->reqsQueue, STTSClientChannelRequest*, i);
			NBASSERT(!req->active)
			TSClientChannelRequest_release(req);
			NBMemory_free(req);
		}
		NBArray_empty(&opq->reqsQueue);
		NBArray_release(&opq->reqsQueue);
		for(i = 0; i < opq->reqsToNotify.use; i++){
			STTSClientChannelRequest* req = NBArray_itmValueAtIndex(&opq->reqsToNotify, STTSClientChannelRequest*, i);
			NBASSERT(!req->active)
			TSClientChannelRequest_release(req);
			NBMemory_free(req);
		}
		NBArray_empty(&opq->reqsToNotify);
		NBArray_release(&opq->reqsToNotify);
		NBThreadMutex_unlock(&opq->mutex);
	}
	NBStruct_stRelease(NBHttpPortCfg_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
	NBThreadCond_release(&opq->cond);
	NBThreadMutex_release(&opq->mutex);
	NBString_release(&opq->strs);
	NBMemory_free(obj->opaque);
	obj->opaque = NULL;
}

//Stats

UI32 TSClientChannel_getStatsTryConnAccum(STTSClientChannel* obj, const BOOL resetAccum){
	UI32 r = 0;
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	{
		r = opq->conn.stats.connTryAccum;
		if(resetAccum){
			opq->conn.stats.connTryAccum = 0;
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

//

const STNBHttpPortCfg* TSClientChannel_getCfg(STTSClientChannel* obj){
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	return &opq->cfg;
}

//

void TSClientChannel_connect_(STTSClientChannelOpq* opq, BOOL* dstWasCanceled){
	NBASSERT(!NBSsl_isSet(opq->conn.d.ssl) && !NBSocket_isSet(opq->conn.d.socket))
	if(!NBSsl_isSet(opq->conn.d.ssl) && !NBSocket_isSet(opq->conn.d.socket) && opq->cfg.server != NULL && opq->cfg.port != 0){
		STNBSocketRef socket	= NBSocket_alloc(NULL);
		STNBSslRef ssl			= NB_OBJREF_NULL;
		STNBSslContextRef sslCtxtRef = NB_OBJREF_NULL;
		NBSocket_createHnd(socket);
		NBSocket_setNoSIGPIPE(socket, TRUE);
		NBSocket_setCorkEnabled(socket, FALSE);
		NBSocket_setDelayEnabled(socket, FALSE);
		if(!NBSocket_connect(socket, opq->cfg.server, opq->cfg.port)){
			PRINTF_CONSOLE_ERROR("TSClientChannel, Could not connect to '%s:%d'.\n", opq->cfg.server, opq->cfg.port);
			if(NBSocket_isSet(socket)){
				//PRINTF_INFO("TSClientChannel_connect_, releasing tmp socket.\n");
				NBSocket_shutdown(socket, NB_IO_BITS_RDWR);
				NBSocket_release(&socket);
				NBSocket_null(&socket);
			}
		} else {
			//Load ref to context (locked)
			{
				NBThreadMutex_lock(&opq->mutex);
				if(NBSslContext_isSet(opq->sslContext.current)){
					sslCtxtRef = NBSslContext_alloc(NULL);
				}
				NBThreadMutex_unlock(&opq->mutex);
			}
			//Attempt connection (unlocked, to avoid freezing the thread)
			if(NBSslContext_isSet(sslCtxtRef)){
				ssl = NBSsl_alloc(NULL);
				if(!NBSsl_connect(ssl, sslCtxtRef, socket)){
					PRINTF_CONSOLE_ERROR("TSClientChannel, Connected to '%s:%d' but ssl handshake failed.\n", opq->cfg.server, opq->cfg.port);
					if(NBSsl_isSet(ssl)){
						NBSsl_release(&ssl);
						NBSsl_null(&ssl);
					}
					if(NBSocket_isSet(socket)){
						//PRINTF_INFO("TSClientChannel_connect_, releasing tmp socket after NBSsl_connect failed.\n");
						NBSocket_shutdown(socket, NB_IO_BITS_RDWR);
						NBSocket_release(&socket);
						NBSocket_null(&socket);
					}
				} else {
					//PRINTF_INFO("TSClientChannel, Connected to '%s:%d' and success ssl handshake.\n", opq->cfg.server, opq->cfg.port);
				}
			} else {
				//PRINTF_INFO("TSClientChannel, Connected to '%s:%d'.\n", opq->cfg.server, opq->cfg.port);
			}
		}
		//Set results (locked)
		{
			NBThreadMutex_lock(&opq->mutex);
			{
				const BOOL cancelConn = (opq->conn.seqVer != opq->sslContext.seqVer ? TRUE : FALSE);
				if(cancelConn){
					//PRINTF_CONSOLE_ERROR("TSClientChannel, SslContext changed while connecting to '%s:%d' (discarting connections).\n", opq->cfg.server, opq->cfg.port);
					if(NBSsl_isSet(ssl)){
						NBSsl_release(&ssl);
						NBSsl_null(&ssl);
					}
					if(NBSocket_isSet(socket)){
						NBSocket_shutdown(socket, NB_IO_BITS_RDWR);
						NBSocket_release(&socket);
						NBSocket_null(&socket);
					}
				}
				if(dstWasCanceled != NULL){
					*dstWasCanceled = cancelConn;
				}
				//
				opq->conn.d.socket	= socket;
				opq->conn.d.ssl		= ssl;
				//
				opq->conn.stats.connTryAccum++;
				opq->conn.stats.connTryTotal++;
			}
			NBThreadMutex_unlock(&opq->mutex);
		}
		//
		if(NBSslContext_isSet(sslCtxtRef)){
			NBSslContext_release(&sslCtxtRef);
			NBSslContext_null(&sslCtxtRef);
		}
	}
}

void TSClientChannel_disconnectLocked_(STTSClientChannelOpq* opq){
	if(NBSsl_isSet(opq->conn.d.ssl)){
		NBSsl_release(&opq->conn.d.ssl);
		NBSsl_null(&opq->conn.d.ssl);
	}
	if(NBSocket_isSet(opq->conn.d.socket)){
		//PRINTF_INFO("TSClientChannel_disconnectLocked_, releasing socket.\n");
		NBSocket_shutdown(opq->conn.d.socket, NB_IO_BITS_RDWR);
		NBSocket_release(&opq->conn.d.socket);
		NBSocket_null(&opq->conn.d.socket);
	}
}

BOOL TSClientChannel_sendBytes_(STTSClientChannelOpq* opq, const char* data, int dataSz){
	BOOL r = FALSE;
	NBASSERT(NBSocket_isSet(opq->conn.d.socket))
	if(NBSocket_isSet(opq->conn.d.socket)){
		if(!NBSsl_isSet(opq->conn.d.ssl)){
			const SI32 rr = NBSocket_send(opq->conn.d.socket, data, dataSz); 
			r = (rr == dataSz);
		} else {
			r = NBSsl_write(opq->conn.d.ssl, data, dataSz);
		}
	}
	return r;
}

BOOL TSClientChannel_sendRequest_(STTSClientChannelOpq* opq, const char* header, const UI32 headerSz, const char* body, const UI32 bodySz){
	BOOL r = FALSE;
	if(TSClientChannel_sendBytes_(opq, header, headerSz)){
		BOOL bodySent = TRUE;
		if(bodySz > 0){
			if(!TSClientChannel_sendBytes_(opq, body, bodySz)){
				bodySent = FALSE;
			}
		}
		r = bodySent;
	}
	return r;
}

SI32 TSClientChannel_receive_(STTSClientChannelOpq* opq, char* buff, int buffSz){
	SI32 r = 0;
	NBASSERT(NBSocket_isSet(opq->conn.d.socket))
	if(NBSocket_isSet(opq->conn.d.socket)){
		if(NBSsl_isSet(opq->conn.d.ssl)){
			r = NBSsl_read(opq->conn.d.ssl, buff, buffSz);
			//if(r <= 0){
			//	PRINTF_INFO("TSClientChannel, NBSsl_read failed.\n");
			//}
		} else {
			r = NBSocket_recv(opq->conn.d.socket, buff, buffSz);
			//if(r <= 0){
			//	PRINTF_INFO("TSClientChannel, NBSocket_recv failed.\n");
			//}
		}
		if(r <= 0){
			//PRINTF_INFO("TSClientChannel, Receive failed.\n");
			//Disconnect
			{
				NBThreadMutex_lock(&opq->mutex);
				TSClientChannel_disconnectLocked_(opq);
				NBThreadMutex_unlock(&opq->mutex);
			}
		}
	}
	return r;
}
	
void TSClientChannel_processRequestAndMoveOrRelease_(STTSClientChannelOpq* opq, STTSClientChannelRequest* req){
	NBASSERT((!NBSocket_isSet(opq->conn.d.socket) && !NBSsl_isSet(opq->conn.d.ssl)) || (NBSocket_isSet(opq->conn.d.socket) && NBSsl_isSet(opq->conn.d.ssl)))
	STNBHttpMessage* httpResp = NULL;
	//Disconnect if context changed
	{
		NBThreadMutex_lock(&opq->mutex);
		if(opq->conn.seqVer != opq->sslContext.seqVer){
			//PRINTF_INFO("TSClientChannel, Channel's sslContext changed, forcing new connection before request-conn...\n");
			TSClientChannel_disconnectLocked_(opq);
			NBASSERT(!NBSocket_isSet(opq->conn.d.socket))
			opq->conn.seqVer = opq->sslContext.seqVer;
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	//Connect
	if(!NBSocket_isSet(opq->conn.d.socket)){
		//PRINTF_INFO("TSClientChannel, Connecting channel to server...\n");
		BOOL wasCanceled = FALSE;
		TSClientChannel_connect_(opq, &wasCanceled);
		opq->conn.d.allowReconnect = FALSE;
		if(!NBSocket_isSet(opq->conn.d.socket)){
			if(!wasCanceled){
				PRINTF_CONSOLE_ERROR("TSClientChannel, Could not connect channel to server.\n");
			}
		} else {
			//PRINTF_INFO("TSClientChannel, Channel connected to server.\n");
		}
	}
	//Process
	if(NBSocket_isSet(opq->conn.d.socket)){
		//Send request
		STNBString str;
		STNBHttpBuilder bldr;
		NBString_init(&str);
		NBHttpBuilder_init(&bldr);
		//
		const char* reqMethod = "POST";
		const STNBEnumMapRecord reqRec = TSRequestsMap_getRequestRecordById(opq->reqsMap, req->reqId);
		NBHttpBuilder_addRequestLine(&bldr, &str, reqMethod, reqRec.strValue, 1, 1);
		NBHttpBuilder_addHost(&bldr, &str, opq->cfg.server, opq->cfg.port);
		NBHttpBuilder_addContentLength(&bldr, &str, req->body.length);
		{
			UI32 i; for(i = 0; i < req->paramsIdx.use; i += 2){
				const UI32 nn = NBArray_itmValueAtIndex(&req->paramsIdx, UI32, i);
				const UI32 vv = NBArray_itmValueAtIndex(&req->paramsIdx, UI32, i + 1);
				const char* n = &req->paramsStrs.str[nn];
				const char* v = &req->paramsStrs.str[vv];
				NBHttpBuilder_addHeaderField(&bldr, &str, n, v);
			}
		}
		NBHttpBuilder_addHeaderEnd(&bldr, &str);
		//
		{
			BOOL reqSent = FALSE;
			//First attempt
			if(TSClientChannel_sendRequest_(opq, str.str, str.length, req->body.str, req->body.length)){
				reqSent = TRUE;
			} else if(opq->conn.d.allowReconnect){
				opq->conn.d.allowReconnect = FALSE;
				//Retry request (second attempt)
				{
					//Diconnect
					{
						NBThreadMutex_lock(&opq->mutex);
						TSClientChannel_disconnectLocked_(opq);
						NBThreadMutex_unlock(&opq->mutex);
					}
					//Connect
					TSClientChannel_connect_(opq, NULL);
					if(NBSocket_isSet(opq->conn.d.socket)){
						if(TSClientChannel_sendRequest_(opq, str.str, str.length, req->body.str, req->body.length)){
							reqSent = TRUE;
						}
					}
				}
			}
			//Receive response
			if(reqSent){
				const STTSRequestsMapItm* reqProps = NULL;
				BOOL headrIsComple = FALSE;
				SI32 streamDiscardData = FALSE;
				void* streamRdrParam = NULL;
				//Receive response message
				httpResp = NBMemory_allocType(STNBHttpMessage);
				NBHttpMessage_init(httpResp);
				NBHttpMessage_feedStartResponseOf(httpResp, 4096, 4096, reqMethod);
				char buff[4096]; UI32 totalRcvd = 0;
				do {
					const SI32 rcvd = TSClientChannel_receive_(opq, buff, sizeof(buff));
					if(rcvd <= 0){
						BOOL errSolved = FALSE;
						if(opq->conn.d.allowReconnect && totalRcvd == 0){
							opq->conn.d.allowReconnect = FALSE;
							//Retry request (second attempt)
							{
								//Disconnect
								{
									NBThreadMutex_lock(&opq->mutex);
									TSClientChannel_disconnectLocked_(opq);
									NBThreadMutex_unlock(&opq->mutex);
								}
								TSClientChannel_connect_(opq, NULL);
								if(NBSocket_isSet(opq->conn.d.socket)){
									if(TSClientChannel_sendRequest_(opq, str.str, str.length, req->body.str, req->body.length)){
										errSolved = TRUE;
									}
								}
							}
						}
						//Stop
						if(!errSolved){
							break;
						}
					} else {
						/*{
						 STNBString str;
						 NBString_init(&str);
						 NBString_concatBytes(&str, buff, rcvd);
						 NBString_replace(&str, "\r", "-");
						 PRINTF_INFO("TSClientChannel, Client, received %d bytes:\nstart-->%s<--end\n", str.length, str.str);
						 NBString_release(&str);
						 }*/
						totalRcvd += rcvd;
						const UI32 csmd = NBHttpMessage_feedBytes(httpResp, buff, rcvd);
						if(csmd < rcvd){
							PRINTF_INFO("TSClientChannel, Response consumed only %d of %d bytes received.\n", csmd, rcvd);
							break;
						} else {
							//Read header
							if(!headrIsComple){
								if(NBHttpHeader_feedIsComplete(&httpResp->header)){
									STNBHttpHeaderStatusLine* statLn = httpResp->header.statusLine;
									if(statLn == NULL){
										PRINTF_INFO("TSClientChannel, Expected status line at http response with %d bytes.\n", totalRcvd);
									} else {
										reqProps = TSRequestsMap_getPropsById(opq->reqsMap, req->reqId); NBASSERT(reqProps != NULL)
										if(reqProps != NULL){
											if(reqProps->respStartMethod != NULL){
												//Send start signal
												(reqProps->respStartMethod)(opq->clientOpq, &streamRdrParam, &streamDiscardData);
												//Send current acumulated body
												if(reqProps->respPeekMethod != NULL){
													STNBString strBody;
													NBString_initWithSz(&strBody, NBHttpBody_chunksTotalBytes(&httpResp->body) + 32, 1024, 0.10f);
													NBHttpBody_chunksConcatAll(&httpResp->body, &strBody);
													if(strBody.length > 0){
														(reqProps->respPeekMethod)(opq->clientOpq, streamRdrParam, strBody.str, strBody.length);
													}
													NBString_release(&strBody);
												}
												//Start ignoring acumulated data
												if(streamDiscardData){
													NBHttpBody_feedSetStorageBuffSize(&httpResp->body, 0); //Stop acumulating data
													NBHttpBody_emptyChuncksData(&httpResp->body); //Clear acumulated data
												}
											}
										}
									}
									headrIsComple = TRUE;
								}
							} else {
								//Feed to stream-reader
								if(reqProps->respPeekMethod != NULL){
									if(rcvd > 0){
										(reqProps->respPeekMethod)(opq->clientOpq, streamRdrParam, buff, rcvd);
									}
								}
							}
							//Determine if header was read
							if(NBHttpMessage_feedIsComplete(httpResp)){
								break;
							}
						}
					}
				} while(TRUE);
				//Send end-signal
				if(reqProps != NULL){
					if(reqProps->respEndMethod != NULL){
						(reqProps->respEndMethod)(opq->clientOpq, streamRdrParam);
						streamRdrParam = NULL;
					}
				}
				//Http result
				//PRINTF_INFO("TSClientChannel, Client, received-end.\n");
				if(!NBHttpMessage_feedEnd(httpResp)){
					if(totalRcvd == 0){
						//Server disconnected with no partial message
					} else {
						//PRINTF_INFO("TSClientChannel, Incomplete http response received after %d bytes.\n", totalRcvd);
					}
					NBHttpMessage_release(httpResp);
					NBMemory_free(httpResp);
					httpResp = NULL;
				} else {
					STNBHttpHeaderStatusLine* statLn = httpResp->header.statusLine;
					if(statLn == NULL){
						PRINTF_INFO("TSClientChannel, Expected status line at http response with %d bytes.\n", totalRcvd);
						NBHttpMessage_release(httpResp);
						NBMemory_free(httpResp);
						httpResp = NULL;
					} else {
						//Analyze and process result
						//reqProps = TSRequestsMap_getPropsById(opq->reqsMap, req->reqId); NBASSERT(reqProps != NULL)
						if(reqProps != NULL){
							if(reqProps->respMethod != NULL){
								void* stData = NULL;
								//Create struct
								if(reqProps->respBodyMap != NULL && !streamDiscardData){
									STNBString strBody;
									NBString_initWithSz(&strBody, NBHttpBody_chunksTotalBytes(&httpResp->body) + 32, 1024, 0.10f);
									NBHttpBody_chunksConcatAll(&httpResp->body, &strBody);
									if(strBody.length > 0){
										stData = NBMemory_alloc(reqProps->respBodyMap->stSize);
										NBMemory_set(stData, 0, reqProps->respBodyMap->stSize);
										if(!NBStruct_stReadFromJsonStr(strBody.str, strBody.length, reqProps->respBodyMap, stData, reqProps->respBodyMap->stSize)){
											NBMemory_free(stData);
											stData = NULL;
										}
									}
									NBString_release(&strBody);
								}
								//Call method
								(*reqProps->respMethod)(opq->clientOpq, httpResp, stData, reqProps->respBodyMap, req->lstnrParam);
								//Release struct
								if(stData != NULL){
									NBStruct_stRelease(reqProps->respBodyMap, stData, reqProps->respBodyMap->stSize);
									NBMemory_free(stData);
									stData = NULL;
								}
							}
						}
					}
				}
			}
		}
		NBHttpBuilder_release(&bldr);
		NBString_release(&str);
	}
	//Move request from queue to resolved
	{
		NBThreadMutex_lock(&opq->mutex);
		{
			BOOL moved = FALSE;
			NBASSERT(req->active)
			UI32 i; for(i = 0; i < opq->reqsQueue.use; i++){
				STTSClientChannelRequest* reqq = NBArray_itmValueAtIndex(&opq->reqsQueue, STTSClientChannelRequest*, i);
				if(reqq == req){
					NBArray_removeItemAtIndex(&opq->reqsQueue, i);
					NBASSERT(req->response == NULL);
					req->response	= httpResp;
					req->active		= FALSE;
					NBArray_addValue(&opq->reqsToNotify, req);
					moved = TRUE;
					break;
				}
			}
			if(!moved){
				//Maybe the listener removed himself while the request was active
				NBASSERT(req->response == NULL);
				if(httpResp != NULL){
					NBHttpMessage_release(httpResp);
					NBMemory_free(httpResp);
					httpResp = NULL;
				}
				TSClientChannelRequest_release(req);
				NBMemory_free(req);
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
}

SI64 TSClientChannel_runAndRelease_(STNBThread* thread, void* param){
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)param;
	//Start
	{
		NBThreadMutex_lock(&opq->mutex);
		opq->threadWorking = TRUE;
		NBThreadMutex_unlock(&opq->mutex);
	}
	//Run
	{
		NBThreadMutex_lock(&opq->mutex);
		while(!opq->shuttingDown){
			//Search for a request
			STTSClientChannelRequest* req = NULL;
			const UI64 curTime = NBDatetime_getCurUTCTimestamp();
			{
				UI32 i; for(i = 0; i < opq->reqsQueue.use; i++){
					STTSClientChannelRequest* rr = NBArray_itmValueAtIndex(&opq->reqsQueue, STTSClientChannelRequest*, i);
					NBASSERT(!rr->active)
					if(rr->notBefore <= curTime){
						req = rr;
						break;
					}
				}
			}
			if(req == NULL){
				//Wait until a request is added to the queue (or any other signal)
				NBThreadCond_wait(&opq->cond, &opq->mutex);
			} else {
				//Consume request
				NBASSERT(!req->active)
				req->active = TRUE;
				opq->conn.curReqUid = req->uid;
				NBThreadMutex_unlock(&opq->mutex);
				{
					opq->conn.d.allowReconnect = TRUE;
					//PRINTF_INFO("Request processing by channel (port %d): '%s'.\n", opq->cfg.port, TSRequestId_getSharedEnumMap()->records[req->reqId].varName);
					TSClientChannel_processRequestAndMoveOrRelease_(opq, req);
				}
				NBThreadMutex_lock(&opq->mutex);
				opq->conn.curReqUid = 0;
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	//Exit
	{
		NBThreadMutex_lock(&opq->mutex);
		opq->threadWorking = FALSE;
		NBThreadCond_broadcast(&opq->cond);
		NBThreadMutex_unlock(&opq->mutex);
	}
	//Release thread
	if(thread != NULL){
		NBThread_release(thread);
		NBMemory_free(thread);
		thread = NULL;
	}
	return 0;
}

BOOL TSClientChannel_prepare(STTSClientChannel* obj, void* clientOpq, STTSContext* context, STTSCypher* cypher, STNBSslContextRef defSslCtxt, const BOOL defSslCtxtIsAuthenticated, const STNBX509* caCert, const STTSRequestsMap* reqsMap, const STNBHttpPortCfg* cfg){
	BOOL r = TRUE;
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	{
		if(r){
			if(cfg->server == NULL){
				PRINTF_CONSOLE_ERROR("TSClientChannel, Client requires a 'server' name.\n");
				r = FALSE;
			} else if(cfg->server[0] == '\0' || cfg->port == 0){
				PRINTF_CONSOLE_ERROR("TSClientChannel, Client requires a valid 'server' name and port.\n");
				r = FALSE;
			} else if(!NBStruct_stClone(NBHttpPortCfg_getSharedStructMap(), cfg, sizeof(*cfg), &opq->cfg, sizeof(opq->cfg))){
				PRINTF_CONSOLE_ERROR("TSClientChannel, Could not set client-channel config.\n");
				r = FALSE;
			}
		}
		if(r){
			opq->sslContext.current			= defSslCtxt;
			opq->sslContext.isAuthenticated	= defSslCtxtIsAuthenticated;
			opq->sslContext.seqVer++; //Force new connection
		}
	}
	if(r){
        //ToDo: implement
		/*if(!NBHttpReqRules_loadFromConfig(&opq->reqsRules, &cfg->reqsRules)){
			PRINTF_CONSOLE_ERROR("TSClientChannel, could not NBHttpReqRules_loadFromConfig.\n");
			r = FALSE;
		}*/
	}
	if(r){
		opq->clientOpq	= clientOpq;
		opq->caCert		= caCert;
		opq->reqsMap	= reqsMap;
		opq->cypher		= cypher;
	}
	if(r){
		STNBThread* thread = NBMemory_allocType(STNBThread);
		NBThread_init(thread);
		NBThread_setIsJoinable(thread, FALSE); //Release thread resources after exit
		opq->threadWorking = TRUE;
		if(!NBThread_start(thread, TSClientChannel_runAndRelease_, opq, NULL)){
			PRINTF_CONSOLE_ERROR("TSClientChannel, Could not start client's thread.\n");
			opq->threadWorking = FALSE;
			NBThread_release(thread);
			NBMemory_free(thread);
			thread = NULL;
			r = FALSE;
		}
	}
	return r;
}

//Connection will be reset
BOOL TSClientChannel_sslContextSet(STTSClientChannel* obj, STNBSslContextRef sslCtxt, const BOOL isAuthenticated){
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	{
		opq->sslContext.current			= sslCtxt;
		opq->sslContext.isAuthenticated	= isAuthenticated;
		opq->sslContext.seqVer++;		//force new connection
		//Shutdown current connection
		if(NBSocket_isSet(opq->conn.d.socket)){
			//PRINTF_INFO("TSClientChannel_sslContextSet, shutting down socket.\n");
			NBSocket_shutdown(opq->conn.d.socket, NB_IO_BITS_RDWR);
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return TRUE;
}

//Force to reset the current connection
BOOL TSClientChannel_sslContextTouch(STTSClientChannel* obj){
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	{
		opq->sslContext.seqVer++;		//force new connection
		//Shutdown current connection
		if(NBSocket_isSet(opq->conn.d.socket)){
			//PRINTF_INFO("TSClientChannel_sslContextTouch, shutting down socket.\n");
			NBSocket_shutdown(opq->conn.d.socket, NB_IO_BITS_RDWR);
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return TRUE;
}

UI64 TSClientChannel_addRequest(STTSClientChannel* obj, const UI64* optUniqueId, const ENTSRequestId reqId, const char* lang, const char** paramsPairsAndNull, const char* body, const UI32 bodySz, const UI64 secsWait, TSClientChannelReqCallbackFunc lstnr, void* lstnrParam){
	UI64 r = 0;
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(reqId < ENTSRequestId_Count && (!opq->cfg.ssl.cert.isRequired || opq->sslContext.isAuthenticated)){
		const STNBEnumMap* enMap = TSRequestId_getSharedEnumMap();
        //ToDo: imeplement
		//STNBHttpReqRule rule = NBHttpReqRules_ruleForRequestTarget(&opq->reqsRules, enMap->records[reqId].strValue);
		//if(rule.action == ENNBHttpReqRule_Accept)
        {
			STTSClientChannelRequest* req = NBMemory_allocType(STTSClientChannelRequest);
			NBMemory_setZeroSt(*req, STTSClientChannelRequest);
			req->uid		= ++opq->reqsCurId; if(optUniqueId != NULL) req->uid = *optUniqueId;
			req->notBefore	= (secsWait == 0 ? 0 : NBDatetime_getCurUTCTimestamp() + secsWait);
			req->active		= FALSE;
			req->reqId		= reqId;
			NBArray_init(&req->paramsIdx, sizeof(UI32), NULL);
			NBString_init(&req->paramsStrs);
			//Add lang as header parameter
			if(!NBString_strIsEmpty(lang)){
				const UI32 nn = req->paramsStrs.length; NBString_concat(&req->paramsStrs, "lang"); NBString_concatByte(&req->paramsStrs, '\0');
				const UI32 vv = req->paramsStrs.length; NBString_concat(&req->paramsStrs, lang); NBString_concatByte(&req->paramsStrs, '\0');
				NBArray_addValue(&req->paramsIdx, nn);
				NBArray_addValue(&req->paramsIdx, vv);
			}
			//Add header parameter
			if(paramsPairsAndNull != NULL){
				UI32 i = 0;
				do {
					const char* n = paramsPairsAndNull[i++];
					if(n == NULL){
						break;
					} else {
						const char* v = paramsPairsAndNull[i++];
						if(v == NULL){
							break;
						} else {
							const UI32 nn = req->paramsStrs.length; NBString_concat(&req->paramsStrs, n); NBString_concatByte(&req->paramsStrs, '\0');
							const UI32 vv = req->paramsStrs.length; NBString_concat(&req->paramsStrs, v); NBString_concatByte(&req->paramsStrs, '\0');
							NBArray_addValue(&req->paramsIdx, nn);
							NBArray_addValue(&req->paramsIdx, vv);
						}
					}
				} while(TRUE);
			}
			NBString_initWithStrBytes(&req->body, (body != NULL ? body : ""), bodySz);
			req->lstnr		= lstnr;
			req->lstnrParam = lstnrParam;
			req->response	= NULL;
			NBArray_addValue(&opq->reqsQueue, req);
			NBThreadCond_broadcast(&opq->cond);
			//PRINTF_INFO("TSClientChannel, Request added to queue.\n");
			r = req->uid;
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

void TSClientChannel_removeRequestsByListenerParam(STTSClientChannel* obj, void* lstnrParam){
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	{
		UI32 totalActive = 0;
		SI32 i; for(i = ((SI32)opq->reqsQueue.use - 1); i >= 0; i--){
			STTSClientChannelRequest* req = NBArray_itmValueAtIndex(&opq->reqsQueue, STTSClientChannelRequest*, i);
			if(req->lstnrParam == lstnrParam){
				if(req->active){
					totalActive++; //do not free, worker-thread is using it
				} else {
					TSClientChannelRequest_release(req);
					NBMemory_free(req);
				}
				NBArray_removeItemAtIndex(&opq->reqsQueue, i);
			}
		}
		for(i = ((SI32)opq->reqsToNotify.use - 1); i >= 0; i--){
			STTSClientChannelRequest* req = NBArray_itmValueAtIndex(&opq->reqsToNotify, STTSClientChannelRequest*, i);
			NBASSERT(!req->active)
			if(req->lstnrParam == lstnrParam){
				TSClientChannelRequest_release(req);
				NBMemory_free(req);
				NBArray_removeItemAtIndex(&opq->reqsToNotify, i);
			}
		}
		NBASSERT(totalActive <= 1)
	}
	NBThreadMutex_unlock(&opq->mutex);
}

void TSClientChannel_getRequestsResultsToNotifyAndRelease(STTSClientChannel* obj, STNBArray* dst){
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	UI32 i; for(i = 0; i < opq->reqsToNotify.use; i++){
		STTSClientChannelRequest* req = NBArray_itmValueAtIndex(&opq->reqsToNotify, STTSClientChannelRequest*, i);
		NBArray_addValue(dst, req);
	}
	NBArray_empty(&opq->reqsToNotify);
	NBThreadMutex_unlock(&opq->mutex);
}

void TSClientChannel_triggerTimedRequests(STTSClientChannel* obj){
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(!opq->shuttingDown){
		//Search for a request
		const UI64 curTime = NBDatetime_getCurUTCTimestamp();
		UI32 i; for(i = 0; i < opq->reqsQueue.use; i++){
			STTSClientChannelRequest* rr = NBArray_itmValueAtIndex(&opq->reqsQueue, STTSClientChannelRequest*, i);
			if(!rr->active && rr->notBefore != 0 && rr->notBefore <= curTime){
				//Notify request availibility
				NBThreadCond_broadcast(&opq->cond);
				break;
			}
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
}

void TSClientChannel_disconnect(STTSClientChannel* obj, const UI64* filterReqUid){
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	{
		BOOL disconnect = TRUE;
		if(filterReqUid != NULL){
			if(opq->conn.curReqUid != *filterReqUid){
				disconnect = FALSE;
			}
		}
		if(disconnect){
			if(NBSocket_isSet(opq->conn.d.socket)){
				opq->conn.d.allowReconnect = FALSE;
				//PRINTF_INFO("TSClientChannel_disconnect, disconnecting socket.\n");
				NBSocket_shutdown(opq->conn.d.socket, NB_IO_BITS_RDWR);
				NBSocket_close(opq->conn.d.socket);
			}
		}
	}
	NBThreadCond_broadcast(&opq->cond);
	NBThreadMutex_unlock(&opq->mutex);
}


void TSClientChannel_startShutdown(STTSClientChannel* obj){
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	opq->shuttingDown = TRUE;
	NBThreadCond_broadcast(&opq->cond);
	NBThreadMutex_unlock(&opq->mutex);
}

void TSClientChannel_waitForAll(STTSClientChannel* obj){
	STTSClientChannelOpq* opq = (STTSClientChannelOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	while(opq->threadWorking){
		NBThreadCond_broadcast(&opq->cond);
		NBThreadCond_wait(&opq->cond, &opq->mutex);
	}
	NBThreadMutex_unlock(&opq->mutex);
}
