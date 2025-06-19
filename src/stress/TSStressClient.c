
#include "nb/NBFrameworkPch.h"
/*
#include "stress/TSStressClient.h"
#include "nb/core/NBThreadCond.h"

void TSStressClientRun_init(STTSStressClientRun* obj){
	obj->stopFlag				= FALSE;
	obj->isMainThreadRunning	= FALSE;
	{
		NBMemory_setZero(obj->stats);
	}
	NBArray_init(&obj->clients, sizeof(STTSStressClientClient*), NULL);
	//Meeting between all threads
	{
		obj->meeting.creating	= FALSE;
		NBString_init(&obj->meeting.meetingId);
	}
	NBThreadMutex_init(&obj->mutex);
	NBThreadCond_init(&obj->cond);
}

void TSStressClientRun_release(STTSStressClientRun* obj){
	//Wait for all threads
	{
		while(obj->stats.threads.running > 0 && obj->isMainThreadRunning) {
			obj->stopFlag	= TRUE;
		}
	}
	NBMemory_setZero(obj->stats);
	NBASSERT(obj->clients.use == 0)
	//Meeting between all threads
	{
		NBString_release(&obj->meeting.meetingId);
	}
	NBArray_release(&obj->clients);
	NBThreadCond_release(&obj->cond);
	NBThreadMutex_release(&obj->mutex);
}

//

void TSStressClientRun_setStopFlag(STTSStressClientRun* obj){
	NBThreadMutex_lock(&obj->mutex);
	obj->stopFlag	= TRUE;
	NBThreadCond_broadcast(&obj->cond);
	NBThreadMutex_unlock(&obj->mutex);
}

void TSStressClientRun_waitForAll(STTSStressClientRun* obj){
	//Wait for all threads
	{
		NBThreadMutex_lock(&obj->mutex);
		while(obj->stats.threads.running > 0) {
			NBThreadCond_wait(&obj->cond, &obj->mutex);
		}
		NBThreadMutex_unlock(&obj->mutex);
	}
}

SI64 TSStressClient_runThreadMain_(STNBThread* thread, void* param){
	SI64 r = 0;
	STTSStressClientRun* obj = (STTSStressClientRun*)param;
	if(obj != NULL){
		//Start
		{
			NBThreadMutex_lock(&obj->mutex);
			NBASSERT(obj->isMainThreadRunning)
			NBThreadMutex_unlock(&obj->mutex);
		}
		//Run
		{
			const UI32 msSleep = 100;
			UI32 msAccum = 0;
			BOOL exit = FALSE;
			while(!exit){
				NBThreadMutex_lock(&obj->mutex);
				{
					if(msAccum >= 1000){
						{
							STNBString str;
							NBString_init(&str);
							//Stats
							if(obj->stats.threads.cltInited >= obj->stats.threads.running){
								//Signups
								if(obj->stats.accum.sigupsCount > 0){
									if(str.length > 0) NBString_concat(&str, ",");
									NBString_concat(&str, " ");
									NBString_concatUI32(&str, obj->stats.accum.sigupsCount);
									NBString_concat(&str, " sigups/s");
									if(obj->stats.accum.sigupsCount > 0){
										NBString_concat(&str, " (");
										NBString_concatSI32(&str, (SI32)(((obj->stats.accum.sigupsClocks / obj->stats.accum.sigupsCount) * 1000) / NBThread_clocksPerSec()));
										NBString_concat(&str, "  ms avg)");
									}
									if(obj->stats.total.sigupsCount > 0){
										NBString_concat(&str, " (");
										NBString_concatUI32(&str, obj->stats.total.sigupsCount);
										NBString_concat(&str, " total)");
									}
								}
								//Seens
								if(obj->stats.accum.seensCount > 0){
									if(str.length > 0) NBString_concat(&str, ",");
									NBString_concat(&str, " ");
									NBString_concatUI32(&str, obj->stats.accum.seensCount);
									NBString_concat(&str, " seens/s");
									if(obj->stats.accum.seensCount > 0){
										NBString_concat(&str, " (");
										NBString_concatSI32(&str, (SI32)(((obj->stats.accum.seensClocks / obj->stats.accum.seensCount) * 1000) / NBThread_clocksPerSec()));
										NBString_concat(&str, "  ms avg)");
									}
									if(obj->stats.total.seensCount > 0){
										NBString_concat(&str, " (");
										NBString_concatUI32(&str, obj->stats.total.seensCount);
										NBString_concat(&str, " total)");
									}
								}
								//Syncing
								if(obj->stats.alive.syncsCount > 0){
									if(str.length > 0) NBString_concat(&str, ",");
									NBString_concat(&str, " ");
									NBString_concatUI32(&str, obj->stats.alive.syncsCount);
									NBString_concat(&str, " syncing");
									/ *if(obj->stats.total.syncsCount > 0){
										NBString_concat(&str, " (");
										NBString_concatUI32(&str, obj->stats.total.syncsCount);
										NBString_concat(&str, " total)");
									}* /
								}
								//Sleeping
								if(obj->stats.alive.sleepingCount > 0){
									if(str.length > 0) NBString_concat(&str, ",");
									NBString_concat(&str, " ");
									NBString_concatUI32(&str, obj->stats.alive.sleepingCount);
									NBString_concat(&str, " sleeping");
									/ *if(obj->stats.total.sleepingCount > 0){
										NBString_concat(&str, " (");
										NBString_concatUI32(&str, obj->stats.total.sleepingCount);
										NBString_concat(&str, " total)");
									}* /
								}
								//Reconnections
								if(obj->stats.total.reconnCount > 0){
									if(str.length > 0) NBString_concat(&str, ",");
									NBString_concat(&str, " ");
									NBString_concatUI32(&str, obj->stats.accum.reconnCount);
									NBString_concat(&str, " reconn/s");
									/ *if(obj->stats.total.reconnCount > 0){
										NBString_concat(&str, " (");
										NBString_concatUI32(&str, obj->stats.total.reconnCount);
										NBString_concat(&str, " total)");
									}* /
								}
							}
							//Default message
							if(str.length <= 0){
								NBString_concat(&str, " ");
								NBString_concatUI32(&str, obj->stats.threads.cltInited);
								NBString_concat(&str, "/");
								NBString_concatUI32(&str, obj->stats.threads.running);
								NBString_concat(&str, "  workers inited");
							}
							PRINTF_INFO("TSStressClientRun,%s.\n", str.str);
							NBString_release(&str);
						}
						NBMemory_setZero(obj->stats.accum);
						msAccum = 0;
					}
					if(obj->stopFlag && obj->stats.threads.running <= 0){
						exit = TRUE;
					}
				}
				NBThreadMutex_unlock(&obj->mutex);
				if(!exit){
					NBThread_mSleep(msSleep);
					msAccum += msSleep;
				}
			}
		}
		//End
		{
			NBThreadMutex_lock(&obj->mutex);
			NBASSERT(obj->isMainThreadRunning)
			obj->isMainThreadRunning = FALSE;
			NBThreadMutex_unlock(&obj->mutex);
		}
	}
	//Release thread
	if(thread != NULL){
		NBThread_release(thread);
		NBMemory_free(thread);
		thread = NULL;
	}
	return r;
}
	
typedef struct STTSStressClientThread_ {
	UI32					threadIdx;
	STTSStressClientRun*	obj;
	struct {
		BOOL				active;
		NBTHREAD_CLOCK		start;
		STNBThreadMutex		mutex;
		STNBThreadCond		cond;
	} sync;
	struct {
		BOOL				active;
		NBTHREAD_CLOCK		start;
		STNBThreadMutex		mutex;
		STNBThreadCond		cond;
	} req;
} STTSStressClientThread;

void TSStressClient_reqCallbackSignup_(const struct STTSClientChannelRequest_* req, void* lParam){
	STTSStressClientClient* clt = (STTSStressClientClient*)lParam;
	STTSStressClientThread* data = (STTSStressClientThread*)clt->data.thread;
	if(data != NULL){
		NBThreadMutex_lock(&data->req.mutex);
		{
			if(req != NULL){
				if(req->response == NULL){
					PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, no response to request (signup).\n", (data->threadIdx + 1));
				} else {
					STNBHttpMessage* resp = req->response;
					if(resp->header.statusLine == NULL){
						PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, no statusLine in response (signup).\n", (data->threadIdx + 1));
					} else {
						STNBHttpHeaderStatusLine* statusLine = resp->header.statusLine;
						if(statusLine->statusCode < 200 || statusLine->statusCode >= 300){
							PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, response (signup): %d '%s'.\n", (data->threadIdx + 1), statusLine->statusCode, &resp->header.strs.str[statusLine->reasonPhrase]);
						} else {
							//PRINTF_INFO("TSStressClient, thread #%d, response (signup): %d '%s'.\n", (data->threadIdx + 1), statusLine->statusCode, &resp->header.strs.str[statusLine->reasonPhrase]);
							STTSStressClientRun* obj = data->obj;
							NBTHREAD_CLOCK cocksDur = NBThread_clock() - data->req.start;
							{
								NBThreadMutex_lock(&obj->mutex);
								obj->stats.accum.sigupsCount++;
								obj->stats.accum.sigupsClocks += cocksDur;
								obj->stats.total.sigupsCount++;
								obj->stats.total.sigupsClocks += cocksDur;
								NBThreadMutex_unlock(&obj->mutex);
							}
						}
					}
				}
			}
			NBASSERT(data->req.active);
			if(data->req.active){
				data->req.active = FALSE;
			}
			NBThreadCond_broadcast(&data->req.cond);
		}
		NBThreadMutex_unlock(&data->req.mutex);
	}
}

void TSStressClient_reqCallbackMeetingNew_(const struct STTSClientChannelRequest_* req, void* lParam){
	STTSStressClientClient* clt = (STTSStressClientClient*)lParam;
	STTSStressClientThread* data = (STTSStressClientThread*)clt->data.thread;
	if(data != NULL){
		NBThreadMutex_lock(&data->req.mutex);
		{
			if(req != NULL){
				if(req->response == NULL){
					PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, no response to request (meeting creation).\n", (data->threadIdx + 1));
				} else {
					STNBHttpMessage* resp = req->response;
					if(resp->header.statusLine == NULL){
						PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, no statusLine in response (meeting creation).\n", (data->threadIdx + 1));
					} else {
						STNBHttpHeaderStatusLine* statusLine = resp->header.statusLine;
						if(statusLine->statusCode < 200 || statusLine->statusCode >= 300){
							PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, response (join): %d '%s'.\n", (data->threadIdx + 1), statusLine->statusCode, &resp->header.strs.str[statusLine->reasonPhrase]);
						} else {
							//PRINTF_INFO("TSStressClient, thread #%d, response (join): %d '%s'.\n", (data->threadIdx + 1), statusLine->statusCode, &resp->header.strs.str[statusLine->reasonPhrase]);
							STTSStressClientRun* obj = data->obj;
							NBTHREAD_CLOCK cocksDur = NBThread_clock() - data->req.start;
							{
								STNBString bodyStr;
								NBString_init(&bodyStr);
								NBHttpBody_chunksConcatAll(&resp->body, &bodyStr);
								{
									const STNBStructMap* respMap = DRMeetingNewResp_getSharedStructMap();
									STTSMeetingNewResp resp;
									NBMemory_setZeroSt(resp, STTSMeetingNewResp);
									if(!NBStruct_stReadFromJsonStr(bodyStr.str, bodyStr.length, respMap, &resp, sizeof(resp))){
										PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, response (meeting creation) is not valid JSON.\n", (data->threadIdx + 1));
									} else {
										PRINTF_INFO("TSStressClient, thread #%d, response (meeting creation): meetingId('%s') in %d ms.\n", (data->threadIdx + 1), resp.meetingId, (SI32)((cocksDur * 1000) / NBThread_clocksPerSec()));
										{
											NBThreadMutex_lock(&obj->mutex);
											NBString_set(&obj->meeting.meetingId, resp.meetingId);
											NBThreadMutex_unlock(&obj->mutex);
										}
									}
									NBStruct_stRelease(respMap, &resp, sizeof(resp));
								}
								NBString_release(&bodyStr);
							}
						}
					}
				}
			}
			NBASSERT(data->req.active);
			if(data->req.active){
				data->req.active = FALSE;
			}
			NBThreadCond_broadcast(&data->req.cond);
		}
		NBThreadMutex_unlock(&data->req.mutex);
	}
}

void TSStressClient_reqCallbackMeetingJoin_(const struct STTSClientChannelRequest_* req, void* lParam){
	STTSStressClientClient* clt = (STTSStressClientClient*)lParam;
	STTSStressClientThread* data = (STTSStressClientThread*)clt->data.thread;
	if(data != NULL){
		NBThreadMutex_lock(&data->req.mutex);
		{
			if(req != NULL){
				if(req->response == NULL){
					PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, no response to request (join).\n", (data->threadIdx + 1));
				} else {
					STNBHttpMessage* resp = req->response;
					if(resp->header.statusLine == NULL){
						PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, no statusLine in response (join).\n", (data->threadIdx + 1));
					} else {
						STNBHttpHeaderStatusLine* statusLine = resp->header.statusLine;
						if(statusLine->statusCode < 200 || statusLine->statusCode >= 300){
							PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, response (join): %d '%s'.\n", (data->threadIdx + 1), statusLine->statusCode, &resp->header.strs.str[statusLine->reasonPhrase]);
						} else {
							//PRINTF_INFO("TSStressClient, thread #%d, response (join): %d '%s'.\n", (data->threadIdx + 1), statusLine->statusCode, &resp->header.strs.str[statusLine->reasonPhrase]);
							//STTSStressClientRun* obj = data->obj;
							NBTHREAD_CLOCK cocksDur = NBThread_clock() - data->req.start;
							{
								STNBString bodyStr;
								NBString_init(&bodyStr);
								NBHttpBody_chunksConcatAll(&resp->body, &bodyStr);
								{
									const STNBStructMap* respMap = DRMeetingJoinResp_getSharedStructMap();
									STTSMeetingJoinResp resp;
									NBMemory_setZeroSt(resp, STTSMeetingJoinResp);
									if(!NBStruct_stReadFromJsonStr(bodyStr.str, bodyStr.length, respMap, &resp, sizeof(resp))){
										PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, response (meeting join) is not valid JSON.\n", (data->threadIdx + 1));
									} else {
										PRINTF_INFO("TSStressClient, thread #%d, response (meeting join): meetingId('%s') in %d ms.\n", (data->threadIdx + 1), resp.meetingId, (SI32)((cocksDur * 1000) / NBThread_clocksPerSec()));
										NBThreadMutex_lock(&clt->mutex);
										{
											NBASSERT(!clt->data.joined)
											clt->data.joined = TRUE;
										}
										NBThreadMutex_unlock(&clt->mutex);
									}
									NBStruct_stRelease(respMap, &resp, sizeof(resp));
								}
								NBString_release(&bodyStr);
							}
						}
					}
				}
			}
			NBASSERT(data->req.active);
			if(data->req.active){
				data->req.active = FALSE;
			}
			NBThreadCond_broadcast(&data->req.cond);
		}
		NBThreadMutex_unlock(&data->req.mutex);
	}
}

void TSStressClient_reqCallbackSyncMe_(const struct STTSClientChannelRequest_* req, void* lParam){
	STTSStressClientClient* clt = (STTSStressClientClient*)lParam;
	STTSStressClientThread* data = (STTSStressClientThread*)clt->data.thread;
	if(data != NULL){
		NBThreadMutex_lock(&data->sync.mutex);
		{
			if(req != NULL){
				if(req->response == NULL){
					PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, no response to request (sync).\n", (data->threadIdx + 1));
				} else {
					STNBHttpMessage* resp = req->response;
					if(resp->header.statusLine == NULL){
						PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, no statusLine in response (sync).\n", (data->threadIdx + 1));
					} else {
						STNBHttpHeaderStatusLine* statusLine = resp->header.statusLine;
						if(statusLine->statusCode < 200 || statusLine->statusCode >= 300){
							PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, response (sync): %d '%s'.\n", (data->threadIdx + 1), statusLine->statusCode, &resp->header.strs.str[statusLine->reasonPhrase]);
						} else {
							PRINTF_INFO("TSStressClient, thread #%d, response (sync): %d '%s'.\n", (data->threadIdx + 1), statusLine->statusCode, &resp->header.strs.str[statusLine->reasonPhrase]);
						}
					}
				}
			}
			NBASSERT(data->sync.active);
			if(data->sync.active){
				data->sync.active = FALSE;
			}
			NBThreadCond_broadcast(&data->sync.cond);
		}
		NBThreadMutex_unlock(&data->sync.mutex);
		//
		STTSStressClientRun* obj = data->obj;
		//NBTHREAD_CLOCK cocksDur = NBThread_clock() - data->sync.start;
		{
			NBThreadMutex_lock(&obj->mutex);
			NBASSERT(obj->stats.alive.syncsCount > 0)
			obj->stats.alive.syncsCount--;
			NBThreadMutex_unlock(&obj->mutex);
		}
	}
}

void TSStressClient_reqCallbackMeetingMsgSeen_(const struct STTSClientChannelRequest_* req, void* lParam){
	STTSStressClientClient* clt = (STTSStressClientClient*)lParam;
	STTSStressClientThread* data = (STTSStressClientThread*)clt->data.thread;
	if(data != NULL){
		NBThreadMutex_lock(&data->req.mutex);
		{
			if(req != NULL){
				if(req->response == NULL){
					PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, no response to request (seen).\n", (data->threadIdx + 1));
				} else {
					STNBHttpMessage* resp = req->response;
					if(resp->header.statusLine == NULL){
						PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, no statusLine in response (seen).\n", (data->threadIdx + 1));
					} else {
						STNBHttpHeaderStatusLine* statusLine = resp->header.statusLine;
						if(statusLine->statusCode < 200 || statusLine->statusCode >= 300){
							PRINTF_CONSOLE_ERROR("TSStressClient, thread #%d, response (seen): %d '%s'.\n", (data->threadIdx + 1), statusLine->statusCode, &resp->header.strs.str[statusLine->reasonPhrase]);
						} else {
							//PRINTF_INFO("TSStressClient, thread #%d, response (seen): %d '%s'.\n", (data->threadIdx + 1), statusLine->statusCode, &resp->header.strs.str[statusLine->reasonPhrase]);
							STTSStressClientRun* obj = data->obj;
							NBTHREAD_CLOCK cocksDur = NBThread_clock() - data->req.start;
							{
								NBThreadMutex_lock(&obj->mutex);
								obj->stats.accum.seensCount++;
								obj->stats.accum.seensClocks += cocksDur;
								obj->stats.total.seensCount++;
								obj->stats.total.seensClocks += cocksDur;
								NBThreadMutex_unlock(&obj->mutex);
							}
						}
					}
				}
			}
			NBASSERT(data->req.active);
			if(data->req.active){
				data->req.active = FALSE;
			}
			NBThreadCond_broadcast(&data->req.cond);
		}
		NBThreadMutex_unlock(&data->req.mutex);
	}
}

SI64 TSStressClient_runThread_(STNBThread* thread, void* param){
	SI64 r = 0;
	STTSStressClientThread* data = (STTSStressClientThread*)param;
	if(data != NULL){
		//
	}
	//Release obj
	if(data != NULL){
		{
			NBASSERT(!data->sync.active);
			NBThreadMutex_release(&data->sync.mutex);
			NBThreadCond_release(&data->sync.cond);
		}
		{
			NBASSERT(!data->req.active);
			NBThreadMutex_release(&data->req.mutex);
			NBThreadCond_release(&data->req.cond);
		}
		NBMemory_free(data);
		data = NULL;
	}
	//Release thread
	if(thread != NULL){
		NBThread_release(thread);
		NBMemory_free(thread);
		thread = NULL;
	}
	return r;
}

BOOL TSStressClientRun_start(STTSStressClientRun* obj, const UI32 ammThreads){
	BOOL r = TRUE;
	//Create mesasurement thread
	if(r){
		STNBThread* thread = NBMemory_allocType(STNBThread);
		NBThread_init(thread);
		NBThread_setIsJoinable(thread, FALSE);
		{
			NBThreadMutex_lock(&obj->mutex);
			NBASSERT(!obj->isMainThreadRunning)
			obj->isMainThreadRunning = TRUE;
			NBThreadMutex_unlock(&obj->mutex);
		}
		if(!NBThread_start(thread, TSStressClient_runThreadMain_, obj, NULL)){
			PRINTF_CONSOLE_ERROR("TSStressClient, could not create mian thread.\n");
			r = FALSE;
		} else {
			//Consume
			thread	= NULL;
		}
		//Release (if not consumed)
		if(thread != NULL){
			{
				NBThreadMutex_lock(&obj->mutex);
				NBASSERT(obj->isMainThreadRunning)
				obj->isMainThreadRunning = FALSE;
				NBThreadMutex_unlock(&obj->mutex);
			}
			NBThread_release(thread);
			NBMemory_free(thread);
			thread = NULL;
		}
	}
	//Create work threads
	if(r){
		{
			NBThreadMutex_lock(&obj->mutex);
			{
				obj->stats.threads.expected = ammThreads;
				NBThreadCond_broadcast(&obj->cond);
			}
			NBThreadMutex_unlock(&obj->mutex);
		}
		{
			SI32 i; for(i = 0; i < ammThreads && r; i++){
				STTSStressClientThread* wrkr = NBMemory_allocType(STTSStressClientThread);
				NBMemory_setZeroSt(*wrkr, STTSStressClientThread);
				wrkr->threadIdx		= i;
				wrkr->obj			= obj;
				{
					wrkr->sync.active = FALSE;
					NBThreadMutex_init(&wrkr->sync.mutex);
					NBThreadCond_init(&wrkr->sync.cond);
				}
				{
					wrkr->req.active = FALSE;
					NBThreadMutex_init(&wrkr->req.mutex);
					NBThreadCond_init(&wrkr->req.cond);
				}
				{
					STNBThread* thread = NBMemory_allocType(STNBThread);
					NBThread_init(thread);
					NBThread_setIsJoinable(thread, FALSE);
					{
						NBThreadMutex_lock(&obj->mutex);
						{
							NBASSERT(obj->stats.threads.running >= 0)
							obj->stats.threads.running++;
							NBThreadCond_broadcast(&obj->cond);
						}
						NBThreadMutex_unlock(&obj->mutex);
					}
					if(!NBThread_start(thread, TSStressClient_runThread_, wrkr, NULL)){
						PRINTF_CONSOLE_ERROR("TSStressClient, could not create thread #%d.\n", (i + 1));
						r = FALSE;
					} else {
						//Consume
						wrkr	= NULL;
						thread	= NULL;
					}
					//Release (if not consumed)
					if(thread != NULL){
						{
							NBThreadMutex_lock(&obj->mutex);
							{
								NBASSERT(obj->stats.threads.running > 0)
								obj->stats.threads.running--;
								NBThreadCond_broadcast(&obj->cond);
							}
							NBThreadMutex_unlock(&obj->mutex);
						}
						NBThread_release(thread);
						NBMemory_free(thread);
						thread = NULL;
					}
				}
				//Release (if not consumed)
				if(wrkr != NULL){
					{
						wrkr->sync.active = FALSE;
						NBThreadMutex_release(&wrkr->sync.mutex);
						NBThreadCond_release(&wrkr->sync.cond);
					}
					{
						wrkr->req.active = FALSE;
						NBThreadMutex_release(&wrkr->req.mutex);
						NBThreadCond_release(&wrkr->req.cond);
					}
					NBMemory_free(wrkr);
					wrkr = NULL;
				}
			}
		}
	}
	//Stop working thread
	if(!r){
		NBThreadMutex_lock(&obj->mutex);
		{
			obj->stopFlag = TRUE;
			obj->stats.threads.expected = 0;
			NBThreadCond_broadcast(&obj->cond);
		}
		NBThreadMutex_unlock(&obj->mutex);
	}
	return r;
}
*/
