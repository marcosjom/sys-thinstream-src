//
//  DRStMailboxes.c
//  thinstream
//
//  Created by Marcos Ortega on 1/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSMailClt.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/net/NBHttpRequest.h"
#include "nb/net/NBHttpClient.h"
#include "nb/net/NBSocket.h"
#include "nb/net/NBSmtpHeader.h"
#include "nb/net/NBSmtpBody.h"
#include "nb/net/NBSmtpClient.h"
#include "nb/net/NBMsExchangeClt.h"
#include "nb/ssl/NBSsl.h"
#include "nb/ssl/NBSslContext.h"

typedef struct STTSMailReq_ {
	STNBString		to;
	STNBString		subject;
	STNBString		body;
	STNBString		attachFilename;
	STNBString		attachMime;
	STNBString		attachData;
} STTSMailReq;

typedef struct STTSMailCltOpq_ {
	BOOL				isConfigured;
	STTSCfgMail			cfg;
	//Network
	struct {
		BOOL			isConnected;
		UI64			lstConnStartTime;
		UI64			lstConnSecsDuration;
		UI32			mailSentCount;
		UI32			mailErrCount;
		//STNBPKey		key;
		//STNBX509		cert;
		STNBSslContextRef sslContext;
		STNBSslRef		ssl;
		STNBSocketRef	socket;
		STNBMsExchangeClt* exchClt;
		STNBThreadMutex	mutex;
	} net;
	//Thread
	struct {
		STNBArray		jobs;			//STTSMailReq
		STNBThreadMutex	mutex;
		STNBThreadCond	cond;
		BOOL			stopFlag;
		UI32			isRunning;
	} thread;
} STTSMailCltOpq;

void TSMailClt_init(STTSMailClt* obj){
	STTSMailCltOpq* opq = obj->opaque = NBMemory_allocType(STTSMailCltOpq);
	NBMemory_setZeroSt(*opq, STTSMailCltOpq);
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
		NBArray_init(&opq->thread.jobs, sizeof(STTSMailReq), NULL);
		NBThreadMutex_init(&opq->thread.mutex);
		NBThreadCond_init(&opq->thread.cond);
		opq->thread.stopFlag	= FALSE;
		opq->thread.isRunning	= FALSE;
	}
}

void TSMailClt_stopLockedOpq(STTSMailCltOpq* opq);

void TSMailClt_release(STTSMailClt* obj){
	STTSMailCltOpq* opq = (STTSMailCltOpq*)obj->opaque;
	//Wait for jobs
	{
		NBThreadMutex_lock(&opq->thread.mutex);
		TSMailClt_stopLockedOpq(opq);
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
		if(opq->net.exchClt != NULL){
			NBMsExchangeClt_release(opq->net.exchClt);
			NBMemory_free(opq->net.exchClt);
			opq->net.exchClt = NULL;
		}
		//Release all pending jobs
		{
			SI32 i; for(i = 0; i < opq->thread.jobs.use; i++){
				STTSMailReq* j = NBArray_itmPtrAtIndex(&opq->thread.jobs, STTSMailReq, i);
				NBString_release(&j->to);
				NBString_release(&j->subject);
				NBString_release(&j->body);
				NBString_release(&j->attachFilename);
				NBString_release(&j->attachMime);
				NBString_release(&j->attachData);
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
	NBStruct_stRelease(TSCfgMail_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
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

BOOL TSMailClt_loadFromConfig(STTSMailClt* obj, const STTSCfgMail* cfg){
	BOOL r = FALSE;
	STTSMailCltOpq* opq = (STTSMailCltOpq*)obj->opaque;
	if(cfg != NULL && !opq->isConfigured){
		if(!(NBString_strIsEmpty(cfg->server) || cfg->port <= 0 || NBString_strIsEmpty(cfg->user) || NBString_strIsEmpty(cfg->pass))){
			if(cfg->isMsExchange){
				STNBMsExchangeClt* exch = NBMemory_allocType(STNBMsExchangeClt);
				NBMsExchangeClt_init(exch);
				if(!NBMsExchangeClt_configure(exch, cfg->server, cfg->port, cfg->target, cfg->user, cfg->pass, NULL, NULL)){
					PRINTF_CONSOLE_ERROR("TSMailClt, NBMsExchangeClt_configure failed.\n");
				} else {
					opq->net.exchClt = exch; exch = NULL;
					r = TRUE;
				}
				//Release (if not consumed)
				if(exch != NULL){
					NBMsExchangeClt_release(exch);
					NBMemory_free(exch);
					exch = NULL;
				}
			} else {
				r = TRUE;
				if(cfg->useSSL){
					r = FALSE;
					if(!NBSslContext_create(opq->net.sslContext, NBSslContext_getClientMode)){
						PRINTF_CONSOLE_ERROR("TSMailClt, could not create the ssl context.\n");
					} else {
						r = TRUE;
					}
					/*if(!NBString_strIsEmpty(cfg->cert.key.path) && !NBString_strIsEmpty(cfg->cert.key.pass)){
						r = FALSE;
						{
							STNBPkcs12 p12;
							NBPkcs12_init(&p12);
							if(!NBPkcs12_createFromDERFile(&p12, cfg->cert.key.path)){
								PRINTF_CONSOLE_ERROR("TSMailClt, could not load p12: '%s'.\n", cfg->cert.key.path);
							} else if(!NBPkcs12_getCertAndKey(&p12, &opq->net.key, &opq->net.cert, cfg->cert.key.pass)){
								PRINTF_CONSOLE_ERROR("TSMailClt, could not extract cert/key from p12: '%s'.\n", cfg->cert.key.path);
							} else if(!NBSslContext_create(&opq->net.sslContext, NBSslContext_getClientMode)){
								PRINTF_CONSOLE_ERROR("TSMailClt, could not create the ssl context: '%s'.\n", cfg->cert.key.path);
							} else if(!NBSslContext_attachCertAndkey(&opq->net.sslContext, &opq->net.cert, &opq->net.key)){
								PRINTF_CONSOLE_ERROR("TSMailClt, could not attach cert/key to ssl context: '%s'.\n", cfg->cert.key.path);
							} else {
								PRINTF_INFO("TSMailClt, loaded with key: '%s'.\n", cfg->cert.key.path);
								r = TRUE;
							}
							NBPkcs12_release(&p12);
						}
					}*/
				}
			}
			//
			if(r){
				NBStruct_stRelease(TSCfgMail_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
				NBStruct_stClone(TSCfgMail_getSharedStructMap(), cfg, sizeof(*cfg), &opq->cfg, sizeof(opq->cfg));
				opq->isConfigured = TRUE;
			}
		}
	}
	return r;
}

BOOL TSMailClt_addRequest(STTSMailClt* obj, const char* to, const char* subject, const char* body, const char* attachFilename, const char* attachMime, const void* attachData, const UI32 attachDataSz){
	BOOL r = FALSE;
	STTSMailCltOpq* opq = (STTSMailCltOpq*)obj->opaque;
	if(opq->isConfigured && !NBString_strIsEmpty(to) && !NBString_strIsEmpty(body)){
		NBThreadMutex_lock(&opq->thread.mutex);
		{
			STTSMailReq n;
			NBMemory_setZeroSt(n, STTSMailReq);
			NBString_initWithStr(&n.to, to);
			NBString_initWithStr(&n.subject, subject);
			NBString_initWithStr(&n.body, body);
			NBString_initWithStr(&n.attachFilename, attachFilename);
			NBString_initWithStr(&n.attachMime, attachMime);
			NBString_initWithStrBytes(&n.attachData, (const char*)attachData, attachDataSz);
			NBArray_addValue(&opq->thread.jobs, n);
			r = TRUE;
		}
		NBThreadCond_broadcast(&opq->thread.cond);
		NBThreadMutex_unlock(&opq->thread.mutex);
	}
	return r;
}

void TSMailClt_stopFlag(STTSMailClt* obj){
	STTSMailCltOpq* opq = (STTSMailCltOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->thread.mutex);
	{
		opq->thread.stopFlag = TRUE;
		NBThreadCond_broadcast(&opq->thread.cond);
	}
	NBThreadMutex_unlock(&opq->thread.mutex);
}

void TSMailClt_stop(STTSMailClt* obj){
	STTSMailCltOpq* opq = (STTSMailCltOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->thread.mutex);
	TSMailClt_stopLockedOpq(opq);
	NBThreadMutex_unlock(&opq->thread.mutex);
}

void TSMailClt_stopLockedOpq(STTSMailCltOpq* opq){
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

SI64 TSMailClt_sendMailsMethd_(STNBThread* t, void* param){
	SI64 r = 0;
	//
	if(param != NULL){
		STTSMailCltOpq* opq = (STTSMailCltOpq*)param;
		NBASSERT(opq->thread.isRunning)
		NBASSERT(!opq->net.isConnected)
		NBASSERT(opq->net.lstConnStartTime == 0)
		NBASSERT(opq->net.lstConnSecsDuration == 0)
		NBASSERT(opq->net.mailSentCount == 0)
		NBASSERT(opq->net.mailErrCount == 0)
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
					STTSMailReq j = NBArray_itmValueAtIndex(&opq->thread.jobs, STTSMailReq, 0);
					//Remove job (will be released after processed)
					NBArray_removeItemAtIndex(&opq->thread.jobs, 0);
					//Process job (unlocked)
					{
						NBThreadMutex_unlock(&opq->thread.mutex);
						{
							STNBSmtpHeader head;
							STNBSmtpBody body;
							NBSmtpHeader_init(&head);
							NBSmtpBody_init(&body);
							NBSmtpHeader_setFROM(&head, opq->cfg.from);
							NBSmtpHeader_addTO(&head, j.to.str);
							NBSmtpHeader_setSUBJECT(&head, j.subject.str);
							NBSmtpBody_setMimeType(&body, "text/html");
							NBSmtpBody_setContent(&body, j.body.str);
							if(j.attachFilename.length > 0 && j.attachData.length > 0){
								NBSmtpBody_addAttach(&body, j.attachMime.str, NULL, NULL, j.attachFilename.str, j.attachData.str, j.attachData.length);
							}
							if(opq->net.exchClt != NULL){
								//Send with Exchange client
								if(!NBMsExchangeClt_mailSendSync(opq->net.exchClt, &head, &body)){
									PRINTF_CONSOLE_ERROR("TSMailClt, NBMsExchangeClt_mailSendSync send failed.\n");
								} else {
									PRINTF_INFO("TSMailClt, mail sent (MsExchange)'%s:%d'.\n", opq->cfg.server, opq->cfg.port);
								}
							} else {
								//Send with SMTP
								//ToDo: use persistent conns.
								STNBSmtpClient smpt;
								NBSmtpClient_init(&smpt);
								if(!NBSmtpClient_connect(&smpt, opq->cfg.server, opq->cfg.port, opq->cfg.useSSL, NULL)){
									PRINTF_CONSOLE_ERROR("TSMailClt, could not connect to: '%s':%d (ssl %d).\n", opq->cfg.server, opq->cfg.port, opq->cfg.useSSL);
								} else {
									if(!NBSmtpClient_sendHeader(&smpt, &head, opq->cfg.user, opq->cfg.pass)){
										PRINTF_CONSOLE_ERROR("TSMailClt, could not send smtp-header to: '%s':%d (ssl %d).\n", opq->cfg.server, opq->cfg.port, opq->cfg.useSSL);
									} else if(!NBSmtpClient_sendBody(&smpt, &body)){
										PRINTF_CONSOLE_ERROR("TSMailClt, could not send smtp-body to: '%s':%d (ssl %d).\n", opq->cfg.server, opq->cfg.port, opq->cfg.useSSL);
									} else {
										PRINTF_INFO("TSMailClt, mail sent (SMTP) '%s:%d'.\n", opq->cfg.server, opq->cfg.port);
									}
									NBSmtpClient_disconnect(&smpt);
								}
								NBSmtpClient_release(&smpt);
							}
							NBSmtpHeader_release(&head);
							NBSmtpBody_release(&body);
						}
						NBThreadMutex_lock(&opq->thread.mutex);
					}
					//Release job
					{
						NBString_release(&j.to);
						NBString_release(&j.subject);
						NBString_release(&j.body);
						NBString_release(&j.attachFilename);
						NBString_release(&j.attachMime);
						NBString_release(&j.attachData);
					}
				}
			}
			NBASSERT(opq->thread.isRunning)
			opq->thread.isRunning = FALSE;
			NBThreadCond_broadcast(&opq->thread.cond);
			NBThreadMutex_unlock(&opq->thread.mutex);
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

BOOL TSMailClt_prepare(STTSMailClt* obj){
	BOOL r = FALSE;
	STTSMailCltOpq* opq = (STTSMailCltOpq*)obj->opaque;
	{
		NBThreadMutex_lock(&opq->thread.mutex);
		if(opq->thread.isRunning){
			PRINTF_CONSOLE_ERROR("Could not start TSMailCltThread, already running.\n");
		} else if(!opq->isConfigured){
			PRINTF_CONSOLE_ERROR("Could not start TSMailCltThread, not configured yet.\n");
		} else {
			NBASSERT(!opq->net.isConnected)
			r = TRUE;
			//Start send thread
			if(r){
				STNBThread* t = NBMemory_allocType(STNBThread);
				NBThread_init(t);
				NBThread_setIsJoinable(t, FALSE);
				opq->thread.isRunning = TRUE;
				if(!NBThread_start(t, TSMailClt_sendMailsMethd_, opq, NULL)){
					PRINTF_CONSOLE_ERROR("Could not start TSMailClt_sendMailsMethd_.\n");
					opq->thread.isRunning = FALSE;
					r = FALSE;
				} else {
					PRINTF_INFO("TSMailClt_sendMailsMethd_ started.\n");
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
