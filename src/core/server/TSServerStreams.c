//
//  TSServerStreams.c
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSServerStreams.h"
//
#include <stdlib.h>	//for rand()
#include "nb/core/NBMemory.h"
#include "nb/core/NBNumParser.h"
//
#include "core/server/TSServerOpq.h"
#include "core/server/TSServerSessionReq.h"
#include "core/server/TSServerStreamReq.h"

//Streams

void TSServerStreams_init(STTSServerStreams* obj){
	NBThreadMutex_init(&obj->mutex);
	NBThreadCond_init(&obj->cond);
	//sessions
	{
		obj->sessions.iSeq = 0;
		NBArray_init(&obj->sessions.arr, sizeof(STTSServerSessionReq*), NULL);
	}
	//viewers
	{
		obj->viewers.iSeq = 0;
		NBArray_init(&obj->viewers.arr, sizeof(STTSServerStreamReq*), NULL);
	}
}

void TSServerStreams_stopFlagLocked_(STTSServerStreams* obj);
void TSServerStreams_stopLocked_(STTSServerStreams* obj);
STNBSha1HashHex TSServerStreams_getNewUid_(void);

void TSServerStreams_release(STTSServerStreams* obj){
	NBThreadMutex_lock(&obj->mutex);
	//stop
	{
		TSServerStreams_stopFlagLocked_(obj);
		TSServerStreams_stopLocked_(obj);
	}
	//sessions
	{	
		NBASSERT(obj->sessions.arr.use == 0)
		NBArray_empty(&obj->sessions.arr);
		NBArray_release(&obj->sessions.arr);
	}
	//viewers
	{
		NBASSERT(obj->viewers.arr.use == 0)
		NBArray_empty(&obj->viewers.arr);
		NBArray_release(&obj->viewers.arr);
	}
	NBThreadCond_release(&obj->cond);
	NBThreadMutex_unlock(&obj->mutex);
	NBThreadMutex_release(&obj->mutex);
}

//cfg

void TSServerStreams_setServerOpq(STTSServerStreams* obj, void* pOpq){
	obj->opq = pOpq;
}

//

void TSServerStreams_stopFlagLocked_(STTSServerStreams* obj){
	//set flag
	obj->state.stopFlag = TRUE;
	//Close all connections
	{
		SI32 i; for(i = 0; i < obj->sessions.arr.use; i++){
			STTSServerSessionReq* s = NBArray_itmValueAtIndex(&obj->sessions.arr, STTSServerSessionReq*, i);
			NBThreadMutex_lock(&s->mutex);
			{
				s->state.stopFlag = TRUE;
                if(NBHttpServiceRespRawLnk_isSet(&s->rawLnk)){
                    NBHttpServiceRespRawLnk_close(&s->rawLnk);
				}
                NBMemory_setZeroSt(s->rawLnk, STNBHttpServiceRespRawLnk);
				NBThreadCond_broadcast(&s->cond);
			}
			NBThreadMutex_unlock(&s->mutex);
		}
	}
	//stop viewers
	{
		SI32 i; for(i = 0; i < obj->viewers.arr.use; i++){
			STTSServerStreamReq* v = NBArray_itmValueAtIndex(&obj->viewers.arr, STTSServerStreamReq*, i);
			TSServerStreamReq_stopFlag(v);
		}
	}
}

void TSServerStreams_stopLocked_(STTSServerStreams* obj){
	TSServerStreams_stopFlagLocked_(obj);
	//Wait for all sessions and viewers
	while(obj->sessions.arr.use != 0 || obj->viewers.arr.use != 0) {
		NBThreadCond_wait(&obj->cond, &obj->mutex);
		TSServerStreams_stopFlagLocked_(obj);
	};
}


void TSServerStreams_stopFlag(STTSServerStreams* obj){
	NBThreadMutex_lock(&obj->mutex);
	{
		TSServerStreams_stopFlagLocked_(obj);
	}
	NBThreadMutex_unlock(&obj->mutex);
}

void TSServerStreams_stop(STTSServerStreams* obj){
	NBThreadMutex_lock(&obj->mutex);
	{
		TSServerStreams_stopLocked_(obj);
	}
	NBThreadMutex_unlock(&obj->mutex);
}

void TSServerStreams_tickSecChanged(STTSServerStreams* obj, const UI64 timestamp){
	NBThreadMutex_lock(&obj->mutex);
	{
		const UI32 secsTimeout = 10; 
		const UI64 curSec = NBDatetime_getCurUTCTimestamp();
		if(obj->state.lastSec != curSec){
			//Tick all sessions
			{
				UI32 removedCount = 0;
				SI32 i; for(i = (SI32)obj->sessions.arr.use - 1; i >= 0; i--){
					STTSServerSessionReq* s2 = NBArray_itmValueAtIndex(&obj->sessions.arr, STTSServerSessionReq*, i);
					if(s2->state.isDisconnected){
						s2->state.secsDisconnected++;
						if(s2->state.secsDisconnected > secsTimeout){
							PRINTF_INFO("TSServerStreams, session '%s' removed after %d secs disconected.\n", s2->uid, s2->state.secsDisconnected);
							//Release and remove
							removedCount++;
							NBArray_removeItemAtIndex(&obj->sessions.arr, i);
							TSServerSessionReq_release(s2);
							NBMemory_free(s2);
							s2 = NULL;
						}
					}
				}
				if(removedCount > 0){
					NBThreadCond_broadcast(&obj->cond);
				}
			}
			//save for next second change detection
			obj->state.lastSec = curSec;
		}
	}
	NBThreadMutex_unlock(&obj->mutex);
}


STNBSha1HashHex TSServerStreams_getNewUid_(void){
	STNBSha1HashHex r;
	NBMemory_setZeroSt(r, STNBSha1HashHex);
	{
		const STNBDatetime dateTime = NBDatetime_getCurUTC();
		STNBString str;
		NBString_init(&str);
		NBString_concat(&str, "TSServerStreams::");
		NBString_concatSqlDatetime(&str, dateTime);
		{
			SI32 i = 0; UI16 val = 0;
			while(i < 8){
				val = (rand() % 0xFFFF);
				NBString_concat(&str, "::");
				NBString_concatUI32(&str, val);
				i++;
			}
		}
		r = NBSha1_getHashHexBytes(str.str, str.length);
		NBString_release(&str);
	}
	return r;
}

BOOL TSServerStreams_cltRequestedSessionStart_(STTSServerStreams* obj, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk);
void TSServerStreams_cltRequestedSessionStart_httpReqOwnershipEnded_(const STNBHttpServiceRespCtx ctx, void* usrData);
BOOL TSServerStreams_cltRequestedSessionStart_httpReqConsumeBodyEnd_(const STNBHttpServiceRespCtx ctx, void* lparam);
void TSServerStreams_cltRequestedSessionStart_httpReqTick_(const STNBHttpServiceRespCtx ctx, const STNBTimestampMicro tickTime, const UI64 msCurTick, const UI32 msNextTick, UI32* dstMsNextTick, void* usrData);
//stream
BOOL TSServerStreams_cltRequestedStream_(STTSServerStreams* obj, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk);
void TSServerStreams_cltRequestedStream_httpReqOwnershipEnded_(const STNBHttpServiceRespCtx ctx, void* usrData);
BOOL TSServerStreams_cltRequestedStream_httpReqConsumeBodyEnd_(const STNBHttpServiceRespCtx ctx, void* lparam);
void TSServerStreams_cltRequestedStream_httpReqTick_(const STNBHttpServiceRespCtx ctx, const STNBTimestampMicro tickTime, const UI64 msCurTick, const UI32 msNextTick, UI32* dstMsNextTick, void* usrData);

BOOL TSServerStreams_cltRequested(STTSServerStreams* obj, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk){
	BOOL r = FALSE;
    if(NBString_strIndexOfLike(reqDesc.firstLine.target, "/session/start", 0) >= 0){
        //Start new session
        if(!NBString_strIsEqual(reqDesc.firstLine.method, "GET")){
            r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Method not supported for target")
            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Method not supported for target");
        } else {
            r = TSServerStreams_cltRequestedSessionStart_(obj, port, conn, reqDesc, reqLnk);
        }
    } else if(NBString_strIndexOfLike(reqDesc.firstLine.target, "/stream.h.264", 0) >= 0 || NBString_strIndexOfLike(reqDesc.firstLine.target, "/stream.mp4", 0) >= 0
              //ToDo: remove
              //|| NBString_strIsEqual(absPath, "/stream") || NBString_strIsEqual(absPath, "/stream/")
              )
    {
        //Start new stream
        if(!NBString_strIsEqual(reqDesc.firstLine.method, "GET")){
            r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Method not supported for target")
            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Method not supported for target");
        } else {
            r = TSServerStreams_cltRequestedStream_(obj, port, conn, reqDesc, reqLnk);
        }
    }
	return r;
}

//

BOOL TSServerStreams_cltRequestedStream_(STTSServerStreams* obj, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk){
	BOOL r = FALSE;
	STTSServerOpq* opq	= (STTSServerOpq*)obj->opq;
    if(reqDesc.header != NULL && reqDesc.body != NULL){ //waiting for header  and body to arrive
        STNBString absPath, query, fragment;
        NBString_initWithSz(&absPath, 0, 64, 0.10f);
        NBString_initWithSz(&query, 0, 64, 0.10f);
        NBString_initWithSz(&fragment, 0, 64, 0.10f);
        if(opq == NULL){
            r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 500, "Internal error, no OPQ-PTR")
            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Internal error, no OPQ-PTR");
        } else if(!NBHttpHeader_strParseRequestTarget(reqDesc.firstLine.target, &absPath, &query, &fragment)){
            r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "No valid target")
            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "No valid target");
        } else if(query.length == 0){
            r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Bad request - query required")
            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Bad request - query required");
        } else {
            //Execute request
            STTSServerStreamReq* reqVwr = NBMemory_allocType(STTSServerStreamReq);
            TSServerStreamReq_init(reqVwr);
            if(!TSServerStreamReq_setCfg(reqVwr, obj, absPath.str, query.str, fragment.str, reqDesc)){
                r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Bad request - no query or sources")
                && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Bad request - no query or sources");
            } else {
                NBThreadMutex_lock(&obj->mutex);
                if(obj->state.stopFlag){
                    r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 500, "Server is stopping")
                    && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Server is stopping");
                } else {
                    STNBHttpServiceReqLstnrItf itf;
                    NBMemory_setZeroSt(itf, STNBHttpServiceReqLstnrItf);
                    itf.httpReqOwnershipEnded   = TSServerStreams_cltRequestedStream_httpReqOwnershipEnded_;
                    itf.httpReqConsumeBodyEnd   = TSServerStreams_cltRequestedStream_httpReqConsumeBodyEnd_;
                    itf.httpReqTick             = TSServerStreams_cltRequestedStream_httpReqTick_;
                    
                    /*
                    //required
                    void (*httpReqOwnershipEnded)       (const STNBHttpServiceRespCtx ctx, void* usrData);    //request ended and none of these callbacks will be called again, release resources (not necesary to call 'httpReqRespClose')
                    //Reading header (optional, if request real-time processing is implement for the request)
                    BOOL (*httpReqConsumeHeadFieldLine) (const STNBHttpServiceRespCtx ctx, const STNBHttpHeaderField* field, void* lparam);
                    BOOL (*httpReqConsumeHeadEnd)       (const STNBHttpServiceRespCtx ctx, UI32* dstBodyBuffSz, void* lparam);
                    //Reading body (optional, if request real-time processing is implement for the request)
                    BOOL (*httpReqConsumeBodyData)      (const STNBHttpServiceRespCtx ctx, const void* data, const UI64 dataSz, void* lparam);
                    BOOL (*httpReqConsumeBodyTrailerField)(const STNBHttpServiceRespCtx ctx, const STNBHttpBodyField* field, void* lparam);
                    BOOL (*httpReqConsumeBodyEnd)       (const STNBHttpServiceRespCtx ctx, void* lparam);
                    //Reading raw-bytes
                    SI32 (*httpReqRcvd)                 (const STNBHttpServiceRespCtx ctx, const void* data, const UI32 dataSz, void* usrData); //request rcvd data
                    //Ticks
                    void (*httpReqTick)                 (const STNBHttpServiceRespCtx ctx, const STNBTimestampMicro tickTime, const UI64 msCurTick, const UI32 msNextTick, UI32* dstMsNextTick, void* usrData);        //request poll-tick
                    */
                    //Add
                    {
                        obj->viewers.iSeq++;
                        NBArray_addValue(&obj->viewers.arr, reqVwr);
                    }
                    if(!NBHttpServiceReqArrivalLnk_setOwner(&reqLnk, &itf, reqVwr, 0 /*msPerTick, dinamically calculated*/)){
                        PRINTF_ERROR("TSServerStreams, NBHttpServiceReqArrivalLnk_setOwner failed.\n");
                        NBArray_removeItemAtIndex(&obj->viewers.arr, obj->viewers.arr.use - 1);
                        r = FALSE; //end-TCP
                    } else {
                        //consume
                        reqVwr = NULL;
                        r = TRUE; //keep TCP-conn
                    }
                }
                NBThreadMutex_unlock(&obj->mutex);
            }
            //release (if not consumed)
            if(reqVwr != NULL){
                TSServerStreamReq_release(reqVwr);
                NBMemory_free(reqVwr);
                reqVwr = NULL;
            }
        }
        NBString_release(&absPath);
        NBString_release(&query);
        NBString_release(&fragment);
    }
	return r;
}

void TSServerStreams_cltRequestedStream_httpReqOwnershipEnded_(const STNBHttpServiceRespCtx ctx, void* usrData){
    STTSServerStreamReq* reqVwr = (STTSServerStreamReq*)usrData;
    STTSServerStreams* obj = reqVwr->obj;
    NBThreadMutex_lock(&obj->mutex);
    {
        //Remove
        BOOL fnd = FALSE;
        SI32 i; for(i = 0; i < obj->viewers.arr.use; i++){
            STTSServerStreamReq* v = NBArray_itmValueAtIndex(&obj->viewers.arr, STTSServerStreamReq*, i);
            if(v == reqVwr){
                NBArray_removeItemAtIndex(&obj->viewers.arr, i);
                NBThreadCond_broadcast(&obj->cond);
                fnd = TRUE;
                break;
            }
        } NBASSERT(fnd) //must be found
    }
    NBThreadMutex_unlock(&obj->mutex);
    //release
    if(reqVwr != NULL){
        //remove rawLnk
        if(NBHttpServiceRespRawLnk_isSet(&reqVwr->net.rawLnk)){
            NBMemory_setZeroSt(reqVwr->net.rawLnk, STNBHttpServiceRespRawLnk);
        }
        TSServerStreamReq_release(reqVwr);
        NBMemory_free(reqVwr);
        reqVwr = NULL;
    }
}

BOOL TSServerStreams_cltRequestedStream_httpReqConsumeBodyEnd_(const STNBHttpServiceRespCtx ctx, void* lparam){
    BOOL r = FALSE;
    STTSServerStreamReq* reqVwr = (STTSServerStreamReq*)lparam; NBASSERT(reqVwr != NULL)
    NBASSERT(ctx.req.header.ref != NULL)
    //Detect "Upgrade" de "Websocket" request
#   ifdef NB_CONFIG_INCLUDE_ASSERTS
    /*{
     STNBString str;
     NBString_init(&str);
     NBHttpHeader_concatHeader(header, &str);
     PRINTF_INFO("TSServerStreams, received request:\n%s\n", str.str);
     NBString_release(&str);
     }*/
#   endif
    if(reqVwr == NULL){
        r = NBHttpServiceRespLnk_setResponseCode(&ctx.resp.lnk, 500, "Internal error - unlinked from parent streams")
        && NBHttpServiceRespLnk_concatBody(&ctx.resp.lnk, "Internal error - unlinked from parent streams");
    } else {
        BOOL connIsValid = FALSE;
        STNBHttpServiceRespRawLnk rawLnk;
        NBMemory_setZeroSt(rawLnk, STNBHttpServiceRespRawLnk);
        if(!NBHttpServiceRespLnk_upgradeToRaw(&ctx.resp.lnk, &rawLnk, FALSE /*includeRawRead*/)){
            connIsValid = FALSE;
            r = NBHttpServiceRespLnk_setResponseCode(&ctx.resp.lnk, 500, "Internal error - could not upgrade connection")
            && NBHttpServiceRespLnk_concatBody(&ctx.resp.lnk, "Internal error - could not upgrade connection");
        } else {
            BOOL connIsValid = TRUE;
            //upgrade to websocket (if requested)
            if(NBHttpHeader_hasFieldValue(ctx.req.header.ref, "Connection", "Upgrade", FALSE)){
                connIsValid = FALSE;
                if(NBHttpHeader_hasFieldValue(ctx.req.header.ref, "Upgrade", "websocket", FALSE)){
                    STNBHttpHeader resp;
                    NBHttpHeader_init(&resp);
                    reqVwr->net.websocket = NBWebSocket_alloc(NULL);
                    if(NBWebSocket_concatResponseHeader(reqVwr->net.websocket, ctx.req.header.ref, &resp)){
    #                   ifdef NB_CONFIG_INCLUDE_ASSERTS
                        {
                            STNBString str;
                            NBString_init(&str);
                            NBHttpHeader_concatHeader(&resp, &str);
                            PRINTF_INFO("TSServerStreams, sending response to upgrade:\n%s\n", str.str);
                            NBString_release(&str);
                        }
    #                   endif
                        if(NBHttpServiceRespRawLnk_sendHeader(&rawLnk, &resp, FALSE /*doNotSendHeaderEnd*/)){
                            connIsValid = TRUE;
                        }
                    }
                    NBHttpHeader_release(&resp);
                } else {
                    connIsValid = FALSE;
                    r = NBHttpServiceRespLnk_setResponseCode(&ctx.resp.lnk, 400, "Bad request - unsupported upgrade type")
                    && NBHttpServiceRespLnk_concatBody(&ctx.resp.lnk, "Bad request - unsupported upgrade type");
                }
            }
            //process request
            if(!connIsValid){
                r = FALSE; //end TCP-conn
            } else if(!TSServerStreamReq_prepare(reqVwr, &rawLnk)){
                PRINTF_ERROR("TSServerStreams, TSServerStreamReq_prepare failed for: '%s'\n", NBHttpHeader_getRequestTarget(ctx.req.header.ref));
                r = FALSE; //end TCP-conn
            } else {
                //connection succesfully upgraded to raw, start sending response
                r = TRUE;
                //consume link (owned by request)
                NBMemory_setZeroSt(rawLnk, STNBHttpServiceRespRawLnk);
            }
        }
        //close (if not consumed)
        if(NBHttpServiceRespRawLnk_isSet(&rawLnk)){
            NBHttpServiceRespRawLnk_close(&rawLnk);
            NBMemory_setZeroSt(rawLnk, STNBHttpServiceRespRawLnk);
        }
    }
    return r;
}

void TSServerStreams_cltRequestedStream_httpReqTick_(const STNBHttpServiceRespCtx ctx, const STNBTimestampMicro tickTime, const UI64 msCurTick, const UI32 msNextTick, UI32* dstMsNextTick, void* usrData){
    STTSServerStreamReq* reqVwr = (STTSServerStreamReq*)usrData; NBASSERT(reqVwr != NULL)
    if(NBHttpServiceRespRawLnk_isSet(&reqVwr->net.rawLnk)){ //only after raw-link is negotiated
        NBASSERT(ctx.req.header.isCompleted)    //request should be completed
        NBASSERT(ctx.req.body.isCompleted)      //request should be completed
        TSServerStreamReq_httpReqTick(reqVwr, tickTime, msCurTick, msNextTick, dstMsNextTick);
    }
}

//session

BOOL TSServerStreams_cltRequestedSessionStart_(STTSServerStreams* obj, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk){
    BOOL r = FALSE;
    STTSServerOpq* opq = (STTSServerOpq*)obj->opq;
    if(reqDesc.header != NULL && reqDesc.body != NULL){ //waiting for header  and body to arrive
        STNBString absPath, query, fragment;
        NBString_initWithSz(&absPath, 0, 64, 0.10f);
        NBString_initWithSz(&query, 0, 64, 0.10f);
        NBString_initWithSz(&fragment, 0, 64, 0.10f);
        if(opq == NULL){
            r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 500, "Internal error, no OPQ-PTR")
            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Internal error, no OPQ-PTR");
        } else if(!NBHttpHeader_strParseRequestTarget(reqDesc.firstLine.target, &absPath, &query, &fragment)){
            r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "No valid target")
            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "No valid target");
        } else {
            //create new session
            const STNBSha1HashHex uid = TSServerStreams_getNewUid_();
            STTSServerSessionReq* s = NBMemory_allocType(STTSServerSessionReq);
            TSServerSessionReq_init(s);
            //cfg
            {
                s->obj  = obj;
                s->uid  = NBString_strNewBuffer(uid.v);
            }
            //add
            {
                NBThreadMutex_lock(&obj->mutex);
                if(obj->state.stopFlag){
                    r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 500, "Server is stopping")
                    && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Server is stopping");
                } else {
                    STNBHttpServiceReqLstnrItf itf;
                    NBMemory_setZeroSt(itf, STNBHttpServiceReqLstnrItf);
                    itf.httpReqOwnershipEnded   = TSServerStreams_cltRequestedSessionStart_httpReqOwnershipEnded_;
                    itf.httpReqConsumeBodyEnd   = TSServerStreams_cltRequestedSessionStart_httpReqConsumeBodyEnd_;
                    itf.httpReqTick             = TSServerStreams_cltRequestedSessionStart_httpReqTick_;
                    //
                    obj->sessions.iSeq++;
                    NBArray_addValue(&obj->sessions.arr, s);
                    NBThreadCond_broadcast(&obj->cond);
                    //
                    if(!NBHttpServiceReqArrivalLnk_setOwner(&reqLnk, &itf, s, 1000)){ //1 ticks per second
                        PRINTF_ERROR("TSServerStreams, NBHttpServiceReqArrivalLnk_setOwner failed.\n");
                        NBArray_removeItemAtIndex(&obj->sessions.arr, obj->sessions.arr.use - 1);
                        r = FALSE; //end-TCP
                    } else {
                        //consume
                        s = NULL;
                        r = TRUE; //keep TCP-conn
                    }
                }
                NBThreadMutex_unlock(&obj->mutex);
            }
            //release (if not consumed)
            if(s != NULL){
                TSServerSessionReq_release(s);
                NBMemory_free(s);
                s = NULL;
            }
        }
        NBString_release(&absPath);
        NBString_release(&query);
        NBString_release(&fragment);
    }
    return r;
}

void TSServerStreams_cltRequestedSessionStart_httpReqOwnershipEnded_(const STNBHttpServiceRespCtx ctx, void* usrData){
    STTSServerSessionReq* s = (STTSServerSessionReq*)usrData;
    NBASSERT(s != NULL && s->obj != NULL)
    if(s != NULL){
        //remove
        if(s->obj != NULL){
            STTSServerStreams* obj = (STTSServerStreams*)s->obj;
            NBThreadMutex_lock(&obj->mutex);
            {
                BOOL fnd = FALSE;
                SI32 i; for(i = 0; i < obj->sessions.arr.use; i++){
                    STTSServerSessionReq* s2 = NBArray_itmValueAtIndex(&obj->sessions.arr, STTSServerSessionReq*, i);
                    if(s == s2){
                        PRINTF_INFO("TSServerStreams, session '%s' removed.\n", s2->uid);
                        s2->state.isDisconnected = TRUE;
                        NBArray_removeItemAtIndex(&obj->sessions.arr, i);
                        NBThreadCond_broadcast(&obj->cond);
                        fnd = TRUE;
                        break;
                    }
                } NBASSERT(fnd)
            }
            NBThreadMutex_unlock(&obj->mutex);
        }
        //remove rawLnk
        if(NBHttpServiceRespRawLnk_isSet(&s->rawLnk)){
            NBMemory_setZeroSt(s->rawLnk, STNBHttpServiceRespRawLnk);
        }
        //release
        TSServerSessionReq_release(s);
        NBMemory_free(s);
        s = NULL;
    }
}

BOOL TSServerStreams_cltRequestedSessionStart_httpReqConsumeBodyEnd_(const STNBHttpServiceRespCtx ctx, void* lparam){
    BOOL r = FALSE;
    STTSServerSessionReq* s = (STTSServerSessionReq*)lparam;
    NBASSERT(s != NULL && s->obj != NULL)
    if(s == NULL || s->obj == NULL){
        r = NBHttpServiceRespLnk_setResponseCode(&ctx.resp.lnk, 500, "Internal error, no OBJ-PTR")
            && NBHttpServiceRespLnk_concatBody(&ctx.resp.lnk, "Internal error, no OBJ-PTR");
    } else {
        STTSServerStreams* obj = (STTSServerStreams*)s->obj;
        BOOL connIsValid = FALSE;
        STNBHttpServiceRespRawLnk rawLnk;
        NBMemory_setZeroSt(rawLnk, STNBHttpServiceRespRawLnk);
        if(!NBHttpServiceRespLnk_upgradeToRaw(&ctx.resp.lnk, &rawLnk, FALSE /*includeRawRead*/)){
            connIsValid = FALSE;
            r = NBHttpServiceRespLnk_setResponseCode(&ctx.resp.lnk, 500, "Internal error - could not upgrade connection")
            && NBHttpServiceRespLnk_concatBody(&ctx.resp.lnk, "Internal error - could not upgrade connection");
        } else {
            BOOL connIsValid = TRUE;
            //upgrade to websocket (if requested)
            if(NBHttpHeader_hasFieldValue(ctx.req.header.ref, "Connection", "Upgrade", FALSE)){
                connIsValid = FALSE;
                if(NBHttpHeader_hasFieldValue(ctx.req.header.ref, "Upgrade", "websocket", FALSE)){
                    STNBHttpHeader resp;
                    NBHttpHeader_init(&resp);
                    s->net.websocket = NBWebSocket_alloc(NULL);
                    if(NBWebSocket_concatResponseHeader(s->net.websocket, ctx.req.header.ref, &resp)){
#                       ifdef NB_CONFIG_INCLUDE_ASSERTS
                        {
                            STNBString str;
                            NBString_init(&str);
                            NBHttpHeader_concatHeader(&resp, &str);
                            PRINTF_INFO("TSServerStreams, sending response to upgrade:\n%s\n", str.str);
                            NBString_release(&str);
                        }
#                       endif
                        NBHttpHeader_addField(&resp, "sessionId", s->uid);
                        if(NBHttpServiceRespRawLnk_sendHeader(&rawLnk, &resp, FALSE /*doNotSendHeaderEnd*/)){
                            connIsValid = TRUE;
                        }
                    }
                    NBHttpHeader_release(&resp);
                } else {
                    connIsValid = FALSE;
                    r = NBHttpServiceRespLnk_setResponseCode(&ctx.resp.lnk, 400, "Bad request - unsupported upgrade type")
                    && NBHttpServiceRespLnk_concatBody(&ctx.resp.lnk, "Bad request - unsupported upgrade type");
                }
            } else {
                STNBHttpHeader header;
                NBHttpHeader_init(&header);
                NBHttpHeader_setStatusLine(&header, 1, 1, 200, "OK");
                NBHttpHeader_addField(&header, "sessionId", s->uid);
                //no content-lenght, connection will be a "infinite" download-stream
                if(!NBHttpServiceRespRawLnk_sendHeader(&rawLnk, &header, FALSE /*doNotSendHeaderEnd*/)){
                    PRINTF_CONSOLE_ERROR("TSServerStreams, session '%s' could not send header.\n", s->uid);
                    connIsValid = FALSE;
                }
                NBHttpHeader_release(&header);
            }
            //process request
            if(!connIsValid){
                r = FALSE; //end TCP-conn
            } else {
                //connection succesfully upgraded to raw, start sending response
                r = TRUE;
                //consume link (owned by request)
                s->rawLnk = rawLnk;
                NBMemory_setZeroSt(rawLnk, STNBHttpServiceRespRawLnk);
                //affect other sessions
                {
                    NBThreadMutex_lock(&obj->mutex);
                    //Overtake previous session connections (if provided)
                    {
                        const char* reqSessionId = NBHttpHeader_getField(ctx.req.header.ref, "sessionId");
                        if(!NBString_strIsEmpty(reqSessionId)){
                            SI32 i; for(i = 0; i < obj->sessions.arr.use; i++){
                                STTSServerSessionReq* s2 = NBArray_itmValueAtIndex(&obj->sessions.arr, STTSServerSessionReq*, i);
                                if(s2 != s && !s2->state.stopFlag && NBString_strIsEqual(reqSessionId, s2->uid)){
                                    PRINTF_INFO("TSServerStreams, session killed by new-incoming-one.\n");
                                    //Kill previous session
                                    {
                                        s2->state.stopFlag = TRUE;
                                        if(NBHttpServiceRespRawLnk_isSet(&s2->rawLnk)){
                                            NBHttpServiceRespRawLnk_close(&s2->rawLnk);
                                            NBMemory_setZeroSt(s2->rawLnk, STNBHttpServiceRespRawLnk);
                                        }
                                    }
                                    //Overtake previous session connections
                                    {
                                        //ToDo: implement
                                    }
                                    break;
                                }
                            }
                        }
                    }
                    NBThreadMutex_unlock(&obj->mutex);
                }
            }
        }
        //close (if not consumed)
        if(NBHttpServiceRespRawLnk_isSet(&rawLnk)){
            NBHttpServiceRespRawLnk_close(&rawLnk);
            NBMemory_setZeroSt(rawLnk, STNBHttpServiceRespRawLnk);
        }
    }
    return r;
}

//Session msg sync

STTSStreamVerDesc* TSServerStreams_StreamGetStreamVerLocked_(STTSStreamDesc* stream, const char* versionId){
	STTSStreamVerDesc* r = NULL;
	//Search
	if(stream->versions != NULL && stream->versionsSz > 0){
		UI32 i; for(i = 0; i < stream->versionsSz; i++){
			STTSStreamVerDesc* v = &stream->versions[i];
			if(NBString_strIsEqual(v->uid, versionId)){
				r = v;
				break;
			}
		}
	}
	return r;
}

STTSStreamDesc* TSServerStreams_streamsGrpGetStreamLocked_(STTSStreamsGrpDesc* grp, const char* streamId){
	STTSStreamDesc* r = NULL;
	//Search
	if(grp->streams != NULL && grp->streamsSz > 0){
		UI32 i; for(i = 0; i < grp->streamsSz; i++){
			STTSStreamDesc* d = &grp->streams[i];
			if(NBString_strIsEqual(d->uid, streamId)){
				r = d;
				break;
			}
		}
	}
	return r;
}

STTSStreamDesc* TSServerStreams_streamServiceGetStreamPriorizingOtherGroupsLocked_(STTSStreamsServiceDesc* service, const char* streamId, const char* groupIdUnderPriorize, STTSStreamsGrpDesc** dstGrp){
	STTSStreamDesc* r = NULL;
	STTSStreamsGrpDesc* rGrp = NULL;
	//Search
	if(service->streamsGrps != NULL && service->streamsGrpsSz > 0){
		UI32 i; for(i = 0; i < service->streamsGrpsSz; i++){
			STTSStreamsGrpDesc* grp = &service->streamsGrps[i]; 
			if(grp->streams != NULL && grp->streamsSz > 0){
				UI32 i; for(i = 0; i < grp->streamsSz; i++){
					STTSStreamDesc* d = &grp->streams[i];
					if(
					   NBString_strIsEqual(d->uid, streamId)
					   && (rGrp == NULL || NBString_strIsEqual(rGrp->uid, groupIdUnderPriorize))
					   ){
						rGrp = grp;
						r = d;
					}
				}
			}
		}
	}
	if(dstGrp != NULL){
		*dstGrp = rGrp;
	}
	return r;
}

STTSStreamsGrpDesc* TSServerStreams_streamServiceGetStreamGrpLocked_(STTSStreamsServiceDesc* service, const char* grpId){
	STTSStreamsGrpDesc* r = NULL;
	//Search
	if(service->streamsGrps != NULL && service->streamsGrpsSz > 0){
		UI32 i; for(i = 0; i < service->streamsGrpsSz; i++){
			STTSStreamsGrpDesc* d = &service->streamsGrps[i];
			if(NBString_strIsEqual(d->uid, grpId)){
				r = d;
				break;
			}
		}
	}
	return r;
}

STTSStreamsServiceDesc* TSServerStreams_streamServiceGetStreamServiceLocked_(STTSStreamsServiceDesc* service, const char* runId){
	STTSStreamsServiceDesc* r = NULL;
	//Search
	if(service->subServices != NULL && service->subServicesSz > 0){
		UI32 i; for(i = 0; i < service->subServicesSz; i++){
			STTSStreamsServiceDesc* d = &service->subServices[i];
			if(NBString_strIsEqual(d->runId, runId)){
				r = d;
				break;
			}
		}
	}
	return r;
}

void TSServerStreams_streamSyncLocked_(STTSStreamDesc* desc, STTSClientRequesterDeviceRef reqStream, STTSStreamStorage* strgStream){
	//Add versions from requester
	STTSClientRequesterDeviceOpq* reqStreamOpq = (STTSClientRequesterDeviceOpq*)reqStream.opaque;
	if(TSClientRequesterDevice_isSet(reqStream) && reqStreamOpq->rtsp.reqs.use > 0){
		if(desc->versions == NULL || desc->versionsSz == 0){
			//init array
			{
				if(desc->versions != NULL){
					NBMemory_free(desc->versions);
					desc->versions = NULL;
				}
				desc->versionsSz = reqStreamOpq->rtsp.reqs.use;
				desc->versions = NBMemory_allocTypes(STTSStreamVerDesc, desc->versionsSz);
			}
			//Add all versions
			{
				SI32 i; for(i = 0; i < reqStreamOpq->rtsp.reqs.use; i++){
					STTSClientRequesterConn* req = NBArray_itmValueAtIndex(&reqStreamOpq->rtsp.reqs, STTSClientRequesterConn*, i);
					STTSStreamVerDesc* ver = &desc->versions[i]; NBASSERT(i < desc->versionsSz);
					NBMemory_setZeroSt(*ver, STTSStreamVerDesc);
					ver->syncFnd	= TRUE;
					ver->iSeq		= 1;
					ver->uid		= NBString_strNewBuffer(req->cfg.versionId);
					ver->name		= NBString_strNewBuffer(req->cfg.versionName);
					ver->live.isOnline = TRUE;
					//NBStruct_stClone(NBAvcPicDescBase_getSharedStructMap(), &req->state.rtsp.picDesc.cur, sizeof(req->state.rtsp.picDesc.cur), &ver->live.props, sizeof(ver->live.props));
				}
			}
		} else {
			//Add versions one by one
			SI32 i; for(i = 0; i < reqStreamOpq->rtsp.reqs.use; i++){
				STTSClientRequesterConn* req = NBArray_itmValueAtIndex(&reqStreamOpq->rtsp.reqs, STTSClientRequesterConn*, i);
				STTSStreamVerDesc* ver = TSServerStreams_StreamGetStreamVerLocked_(desc, req->cfg.versionId);
				if(ver == NULL){
					STTSStreamVerDesc* nArr = NBMemory_allocTypes(STTSStreamVerDesc, desc->versionsSz + 1);
					if(desc->versions != NULL){
						if(desc->versionsSz > 0){
							NBMemory_copy(nArr, desc->versions, sizeof(desc->versions[0]) * desc->versionsSz);
						}
						NBMemory_free(desc->versions);
					}
					desc->versions	= nArr;
					ver				= &desc->versions[desc->versionsSz++]; 
					//
					NBMemory_setZeroSt(*ver, STTSStreamVerDesc);
					ver->iSeq		= 1;
					ver->uid		= NBString_strNewBuffer(req->cfg.versionId);
					ver->name		= NBString_strNewBuffer(req->cfg.versionName);
					ver->live.isOnline = TRUE;
					//NBStruct_stClone(NBAvcPicDescBase_getSharedStructMap(), &req->state.rtsp.picDesc.cur, sizeof(req->state.rtsp.picDesc.cur), &ver->live.props, sizeof(ver->live.props));
				} else {
					NBASSERT(NBString_strIsEqual(ver->uid, req->cfg.versionId))
                    /*const STNBStructMap* propsMap = NBAvcPicDescBase_getSharedStructMap();
					if(!NBStruct_stIsEqualByCrc(propsMap, &req->state.rtsp.picDesc.cur, sizeof(req->state.rtsp.picDesc.cur), &ver->live.props, sizeof(ver->live.props))){
						NBStruct_stRelease(propsMap, &ver->live.props, sizeof(ver->live.props));
						NBStruct_stClone(propsMap, &req->state.rtsp.picDesc.cur, sizeof(req->state.rtsp.picDesc.cur), &ver->live.props, sizeof(ver->live.props));
						ver->iSeq++;
						desc->iSeq++;
					}*/
					if(!ver->live.isOnline){
						ver->live.isOnline = TRUE;
						ver->iSeq++;
						desc->iSeq++;
					}
				}
				//flag as 'found'
				ver->syncFnd = TRUE;
			}
		}
	}
	//Add versions from storage
	if(strgStream != NULL){
		SI32 strgVersionIdx = 0;
		STTSStreamVersionStorage* strgVersion = TSStreamStorage_getVersionByIdxRetained(strgStream, strgVersionIdx);
		while(strgVersion){
			const char* uid = TSStreamVersionStorage_getUid(strgVersion);
			STTSStreamVerDesc* ver = TSServerStreams_StreamGetStreamVerLocked_(desc, uid);
			if(ver == NULL){
				STTSStreamVerDesc* nArr = NBMemory_allocTypes(STTSStreamVerDesc, desc->versionsSz + 1);
				if(desc->versions != NULL){
					if(desc->versionsSz > 0){
						NBMemory_copy(nArr, desc->versions, sizeof(desc->versions[0]) * desc->versionsSz);
					}
					NBMemory_free(desc->versions);
				}
				desc->versions	= nArr;
				ver				= &desc->versions[desc->versionsSz++]; 
				//
				NBMemory_setZeroSt(*ver, STTSStreamVerDesc);
				ver->iSeq		= 1;
				ver->uid		= NBString_strNewBuffer(uid);
				ver->name		= NBString_strNewBuffer(uid);
				ver->storage.isOnline = TRUE;
			} else {
				NBASSERT(NBString_strIsEqual(ver->uid, uid))
				if(!ver->storage.isOnline){
					ver->storage.isOnline = TRUE;
					ver->iSeq++;
					desc->iSeq++;
				}
			}
			//flag as 'found'
			ver->syncFnd = TRUE;
			//Next
			strgVersionIdx++;
			TSStreamStorage_returnVersion(strgStream, strgVersion);
			strgVersion = TSStreamStorage_getVersionByIdxRetained(strgStream, strgVersionIdx);
		}
	}
	//flag as 'found'
	desc->syncFnd = TRUE;
}

STTSStreamDesc* TSServerStreams_streamsGrpAddNewStreamLocked_(STTSStreamsGrpDesc* grp, const char* streamId, const char* streamName, STTSClientRequesterDeviceRef stream, STTSStreamStorage* strgStream){
	STTSStreamDesc* r = NULL;
	{
		STTSStreamDesc* nArr = NBMemory_allocTypes(STTSStreamDesc, grp->streamsSz + 1);
		if(grp->streams != NULL){
			if(grp->streamsSz > 0){
				NBMemory_copy(nArr, grp->streams, sizeof(grp->streams[0]) * grp->streamsSz);
			}
			NBMemory_free(grp->streams);
		}
		grp->streams = nArr;
		r = &grp->streams[grp->streamsSz++];
		grp->iSeq++;
		//populate
		NBMemory_setZeroSt(*r, STTSStreamDesc);
		r->syncFnd	= TRUE;
		r->iSeq	 	= 1;
		r->uid		= NBString_strNewBuffer(streamId);
		r->name		= NBString_strNewBuffer(streamName);
		TSServerStreams_streamSyncLocked_(r, stream, strgStream);
	}
	return r;
}

void TSServerStreams_sessionSyncCurStreamsLockedOpq_(STTSStreamsServiceDesc* cur, STTSClientRequesterRef reqMngr, STTSStreamsStorage* storgMngr){
	//flag all as 'not-synced'
	{
		if(cur->streamsGrps != NULL && cur->streamsGrpsSz > 0){
			UI32 i; for(i = 0; i < cur->streamsGrpsSz; i++){
				STTSStreamsGrpDesc* grp = &cur->streamsGrps[i];
				grp->syncFnd = FALSE;
				if(grp->streams != NULL && grp->streamsSz > 0){
					UI32 i; for(i = 0; i < grp->streamsSz; i++){
						STTSStreamDesc* strm = &grp->streams[i];
						strm->syncFnd = FALSE;
						if(strm->versions != NULL && strm->versionsSz > 0){
							UI32 i; for(i = 0; i < strm->versionsSz; i++){
								STTSStreamVerDesc* ver = &strm->versions[i];
								ver->syncFnd = FALSE;
							}
						}
					}
				}
			}
		}
		if(cur->subServices != NULL && cur->subServicesSz > 0){
			UI32 i; for(i = 0; i < cur->subServicesSz; i++){
				STTSStreamsServiceDesc* sub = &cur->subServices[i];
				sub->syncFnd = FALSE;
			}
		}
	}
	//compare live-streams
	{
		SI32 use = 0;
		STTSClientRequesterDeviceRef* arr = TSClientRequester_getGrpsAndLock(reqMngr, &use);
		if(arr != NULL){
			SI32 i; for(i = 0; i < use; i++){
				STTSClientRequesterDeviceRef grp = arr[i];
				STTSClientRequesterDeviceOpq* grpOpq = (STTSClientRequesterDeviceOpq*)grp.opaque;
				NBASSERT(grpOpq->cfg.type != ENTSClientRequesterDeviceType_Local || (grpOpq->remoteDesc.streamsGrpsSz == 1 && grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1))
				if(
				   grpOpq->cfg.type == ENTSClientRequesterDeviceType_Local
				   && (
					grpOpq->remoteDesc.streamsGrpsSz == 1
					&& grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1 
					)
				   ){
					const char* groupId		= grpOpq->remoteDesc.streamsGrps[0].uid;
					const char* groupName	= grpOpq->remoteDesc.streamsGrps[0].name;
					const char* streamId	= grpOpq->remoteDesc.streamsGrps[0].streams[0].uid;
					const char* streamName	= grpOpq->remoteDesc.streamsGrps[0].streams[0].name;
					STTSStreamStorage* strgStream = TSStreamsStorage_getStreamRetained(storgMngr, streamId, FALSE);
					STTSStreamsGrpDesc* curGrp = TSServerStreams_streamServiceGetStreamGrpLocked_(cur, groupId);
					if(curGrp == NULL){
						//New group (create-group and add, create-stream and add)
						STTSStreamsGrpDesc* nArr = NBMemory_allocTypes(STTSStreamsGrpDesc, cur->streamsGrpsSz + 1);
						if(cur->streamsGrps != NULL){
							if(cur->streamsGrpsSz > 0){
								NBMemory_copy(nArr, cur->streamsGrps, sizeof(cur->streamsGrps[0]) * cur->streamsGrpsSz);
							}
							NBMemory_free(cur->streamsGrps);
						}
						cur->streamsGrps = nArr;
						curGrp = &cur->streamsGrps[cur->streamsGrpsSz++];
						NBMemory_setZeroSt(*curGrp, STTSStreamsGrpDesc);
						curGrp->iSeq	= 1;
						curGrp->uid 	= NBString_strNewBuffer(groupId);
						curGrp->name 	= NBString_strNewBuffer(groupName);
						//New stream (create-stream and add)
						{
							IF_NBASSERT(const UI32 iSeqBefore = curGrp->iSeq;)
							TSServerStreams_streamsGrpAddNewStreamLocked_(curGrp, streamId, streamName, grp, strgStream);
							NBASSERT(iSeqBefore != curGrp->iSeq) //should have changed
							cur->iSeq++;
						}
					} else {
						STTSStreamDesc* curDesc = TSServerStreams_streamsGrpGetStreamLocked_(curGrp, streamId);
						if(curDesc != NULL){
							//Existing stream
							NBASSERT(NBString_strIsEqual(curDesc->uid, streamId))
							const UI32 iSeqBefore = curDesc->iSeq;
							TSServerStreams_streamSyncLocked_(curDesc, grp, strgStream);
							if(iSeqBefore != curDesc->iSeq){
								//Change detected
								cur->iSeq++;
							}
						} else {
							//New stream (create-stream and add)
							IF_NBASSERT(const UI32 iSeqBefore = curGrp->iSeq;)
							TSServerStreams_streamsGrpAddNewStreamLocked_(curGrp, streamId, streamName, grp, strgStream);
							NBASSERT(iSeqBefore != curGrp->iSeq) //should have changed
							cur->iSeq++;
						}
					}
					//flag as 'found'
					curGrp->syncFnd = TRUE;
					NBASSERT(!NBString_strIsEmpty(curGrp->uid))
					//return stream
					if(strgStream != NULL){
						TSStreamsStorage_returnStream(storgMngr, strgStream);
						strgStream = NULL;
					}
				}
			}
			TSClientRequester_returnGrpsAndUnlock(reqMngr, arr);
		}
	}
	//compare storages-streams
	{
		SI32 strgStreamIdx = 0;
		STTSStreamStorage* strgStream = TSStreamsStorage_getStreamByIdxRetained(storgMngr, strgStreamIdx);
		while(strgStream != NULL){
			const char* groupIdDef = "storage";	//ungrouped sources
			const char* groupNameDef = "Recorded";	//ungrouped sources
			const char* streamId = TSStreamStorage_getUid(strgStream);
			STTSStreamsGrpDesc* curGrp = NULL;
			STTSStreamDesc* curDesc = TSServerStreams_streamServiceGetStreamPriorizingOtherGroupsLocked_(cur, streamId, groupIdDef, &curGrp);
			if(curDesc != NULL){
				//Existing stream
				NBASSERT(NBString_strIsEqual(curDesc->uid, streamId))
				const UI32 iSeqBefore = curDesc->iSeq; 
				TSServerStreams_streamSyncLocked_(curDesc, NB_OBJREF_NULL, strgStream);
				if(iSeqBefore != curDesc->iSeq){
					//Change detected
					cur->iSeq++;
				}
			} else {
				curGrp = TSServerStreams_streamServiceGetStreamGrpLocked_(cur, groupIdDef);
				if(curGrp == NULL){
					//New group (create-group and add, create-stream and add)
					STTSStreamsGrpDesc* nArr = NBMemory_allocTypes(STTSStreamsGrpDesc, cur->streamsGrpsSz + 1);
					if(cur->streamsGrps != NULL){
						if(cur->streamsGrpsSz > 0){
							NBMemory_copy(nArr, cur->streamsGrps, sizeof(cur->streamsGrps[0]) * cur->streamsGrpsSz);
						}
						NBMemory_free(cur->streamsGrps);
					}
					cur->streamsGrps = nArr;
					curGrp = &cur->streamsGrps[cur->streamsGrpsSz++];
					NBMemory_setZeroSt(*curGrp, STTSStreamsGrpDesc);
					curGrp->iSeq	= 1;
					curGrp->uid 	= NBString_strNewBuffer(groupIdDef);
					curGrp->name 	= NBString_strNewBuffer(groupNameDef);
				}
				//New stream (create and add)
				{
					IF_NBASSERT(const UI32 iSeqBefore = curGrp->iSeq;)
					TSServerStreams_streamsGrpAddNewStreamLocked_(curGrp, streamId, streamId, NB_OBJREF_NULL, strgStream);
					NBASSERT(iSeqBefore != curGrp->iSeq) //should have changed
					cur->iSeq++;
				}
			}
			//flag as 'found'
			NBASSERT(curGrp != NULL)
			curGrp->syncFnd = TRUE;
			//Next
			strgStreamIdx++;
			TSStreamsStorage_returnStream(storgMngr, strgStream);
			strgStream = TSStreamsStorage_getStreamByIdxRetained(storgMngr, strgStreamIdx);
		}
	}
	//ToDo: sync remote
	{
		//ToDo
	}
	//remove not found
	{
		if(cur->streamsGrps != NULL && cur->streamsGrpsSz > 0){
			SI32 i; for(i = (SI32)cur->streamsGrpsSz - 1; i >= 0; i--){
				STTSStreamsGrpDesc* grp = &cur->streamsGrps[i];
				if(!grp->syncFnd){
					//remove
					PRINTF_INFO("TSServerStreams, sync group('%s') removed.\n", grp->uid);
					NBStruct_stRelease(TSStreamsGrpDesc_getSharedStructMap(), grp, sizeof(*grp));
					cur->streamsGrpsSz--;
					{
						UI32 i2; for(i2 = (UI32)i; i2 < cur->streamsGrpsSz; i2++){
							cur->streamsGrps[i2] = cur->streamsGrps[i2 + 1]; 
						}
					}
				} else if(grp->streams != NULL && grp->streamsSz > 0){
					SI32 i; for(i = (SI32)grp->streamsSz - 1; i >= 0; i--){
						STTSStreamDesc* strm = &grp->streams[i];
						if(!strm->syncFnd){
							//remove
							PRINTF_INFO("TSServerStreams, sync group('%s') stream('%s') removed.\n", grp->uid, strm->uid);
							NBStruct_stRelease(TSStreamDesc_getSharedStructMap(), strm, sizeof(*strm));
							grp->streamsSz--;
							{
								UI32 i2; for(i2 = (UI32)i; i2 < grp->streamsSz; i2++){
									grp->streams[i2] = grp->streams[i2 + 1]; 
								}
							}
						} else if(strm->versions != NULL && strm->versionsSz > 0){
							SI32 i; for(i = (SI32)strm->versionsSz - 1; i >= 0; i--){
								STTSStreamVerDesc* ver = &strm->versions[i];
								if(!ver->syncFnd){
									//remove
									PRINTF_INFO("TSServerStreams, sync group('%s') stream('%s') ver('%s') removed.\n", grp->uid, strm->uid, ver->uid);
									NBStruct_stRelease(TSStreamVerDesc_getSharedStructMap(), ver, sizeof(*ver));
									strm->versionsSz--;
									{
										UI32 i2; for(i2 = (UI32)i; i2 < strm->versionsSz; i2++){
											strm->versions[i2] = strm->versions[i2 + 1]; 
										}
									}
								}
							}
						}
					}
				}
			}
		}
		if(cur->subServices != NULL && cur->subServicesSz > 0){
			SI32 i; for(i = (SI32)cur->subServicesSz - 1; i >= 0; i--){
				STTSStreamsServiceDesc* sub = &cur->subServices[i];
				if(!sub->syncFnd){
					//remove
					PRINTF_INFO("TSServerStreams, sync sub-service('%s') removed.\n", sub->runId);
					NBStruct_stRelease(TSStreamsServiceDesc_getSharedStructMap(), sub, sizeof(*sub));
					sub->subServicesSz--;
					{
						UI32 i2; for(i2 = (UI32)i; i2 < sub->subServicesSz; i2++){
							sub->subServices[i2] = sub->subServices[i2 + 1]; 
						}
					}
				}
			}
		}
	}
}

void TSServerStreams_sessionNewStreamsUpdateCurVsSentRecursive_(STTSStreamsServiceUpd** dstUpd, const STTSStreamsServiceDesc* srvcCur, const STTSStreamsServiceDesc* srvcSent){
	//detect new and updated
	{
		UI32 i; for(i = 0; i < srvcCur->streamsGrpsSz; i++){
			STTSStreamsGrpDesc* grp = &srvcCur->streamsGrps[i];
			STTSStreamsGrpUpd* grpDst = NULL;
			{
				UI32 i; for(i = 0; i < grp->streamsSz; i++){
					const STTSStreamDesc* src = &grp->streams[i];
					const STTSStreamDesc* dst = NULL;
					{
						UI32 i; for(i = 0; i < srvcSent->streamsGrpsSz && dst == NULL; i++){
							const STTSStreamsGrpDesc* grp2 = &srvcSent->streamsGrps[i];
							if(NBString_strIsEqual(grp2->uid, grp->uid)){
								UI32 i; for(i = 0; i < grp2->streamsSz; i++){
									const STTSStreamDesc* dst2 = &grp2->streams[i];
									if(NBString_strIsEqual(dst2->uid, src->uid)){
										dst = dst2;
										break;
									}
								}
							}
						}
					}
					if(dst == NULL || dst->iSeq != src->iSeq){
						//new of modified stream
						if((*dstUpd) == NULL){
							(*dstUpd) = NBMemory_allocType(STTSStreamsServiceUpd);
							NBMemory_setZeroSt(**dstUpd, STTSStreamsServiceUpd);
						}
						//new grp-action
						if(grpDst == NULL){
							STTSStreamsGrpUpd* nArr = NBMemory_allocTypes(STTSStreamsGrpUpd, (*dstUpd)->grpsSz + 1);
							if((*dstUpd)->grps != NULL){
								if((*dstUpd)->grpsSz > 0){
									NBMemory_copy(nArr, (*dstUpd)->grps, sizeof((*dstUpd)->grps[0]) * (*dstUpd)->grpsSz);
								}
								NBMemory_free((*dstUpd)->grps);
							}
							(*dstUpd)->grps = nArr;
							grpDst = &(*dstUpd)->grps[(*dstUpd)->grpsSz++]; 
							NBMemory_setZeroSt(*grpDst, STTSStreamsGrpUpd);
							grpDst->uid		= NBString_strNewBuffer(grp->uid);
							grpDst->name	= NBString_strNewBuffer(grp->name);
						}
						//new stream-action
						{
							STTSStreamUpd* nArr = NBMemory_allocTypes(STTSStreamUpd, grpDst->streamsSz + 1);
							if(grpDst->streams != NULL){
								if(grpDst->streamsSz > 0){
									NBMemory_copy(nArr, grpDst->streams, sizeof(grpDst->streams[0]) * grpDst->streamsSz);
								}
								NBMemory_free(grpDst->streams);
							}
							grpDst->streams = nArr;
							{
								STTSStreamUpd* rec = &grpDst->streams[grpDst->streamsSz++];
								NBMemory_setZeroSt(*rec, STTSStreamUpd);
								rec->action = (dst == NULL ? ENTSStreamUpdAction_New : ENTSStreamUpdAction_Changed);
								NBStruct_stClone(TSStreamDesc_getSharedStructMap(), src, sizeof(*src), &rec->desc, sizeof(rec->desc));
							}
						}
					}
				}
			}
		}
	}
	//detect removed
	{
		UI32 i; for(i = 0; i < srvcSent->streamsGrpsSz; i++){
			STTSStreamsGrpDesc* grp = &srvcSent->streamsGrps[i];
			STTSStreamsGrpUpd* grpDst = NULL;
			{
				UI32 i; for(i = 0; i < grp->streamsSz; i++){
					const STTSStreamDesc* src = &grp->streams[i];
					const STTSStreamDesc* dst = NULL;
					{
						UI32 i; for(i = 0; i < srvcCur->streamsGrpsSz && dst == NULL; i++){
							STTSStreamsGrpDesc* grp2 = &srvcCur->streamsGrps[i];
							if(NBString_strIsEqual(grp2->uid, grp->uid)){
								UI32 i; for(i = 0; i < grp2->streamsSz; i++){
									const STTSStreamDesc* dst2 = &grp2->streams[i];
									if(NBString_strIsEqual(dst2->uid, src->uid)){
										dst = dst2;
										break;
									}
								}
							}
						}
					}
					if(dst == NULL){
						//Removed stream
						if((*dstUpd) == NULL){
							(*dstUpd) = NBMemory_allocType(STTSStreamsServiceUpd);
							NBMemory_setZeroSt(**dstUpd, STTSStreamsServiceUpd);
						}
						//new grp-action
						if(grpDst == NULL){
							STTSStreamsGrpUpd* nArr = NBMemory_allocTypes(STTSStreamsGrpUpd, (*dstUpd)->grpsSz + 1);
							if((*dstUpd)->grps != NULL){
								if((*dstUpd)->grpsSz > 0){
									NBMemory_copy(nArr, (*dstUpd)->grps, sizeof((*dstUpd)->grps[0]) * (*dstUpd)->grpsSz);
								}
								NBMemory_free((*dstUpd)->grps);
							}
							(*dstUpd)->grps = nArr;
							grpDst = &(*dstUpd)->grps[(*dstUpd)->grpsSz++]; 
							NBMemory_setZeroSt(*grpDst, STTSStreamsGrpUpd);
							grpDst->uid		= NBString_strNewBuffer(grp->uid);
							grpDst->name	= NBString_strNewBuffer(grp->name);
						}
						//new stream-action
						{
							STTSStreamUpd* nArr = NBMemory_allocTypes(STTSStreamUpd, grpDst->streamsSz + 1);
							if(grpDst->streams != NULL){
								if(grpDst->streamsSz > 0){
									NBMemory_copy(nArr, grpDst->streams, sizeof(grpDst->streams[0]) * grpDst->streamsSz);
								}
								NBMemory_free(grpDst->streams);
							}
							grpDst->streams = nArr;
							{
								STTSStreamUpd* rec = &grpDst->streams[grpDst->streamsSz++];
								NBMemory_setZeroSt(*rec, STTSStreamUpd);
								rec->action = ENTSStreamUpdAction_Removed;
								NBStruct_stClone(TSStreamDesc_getSharedStructMap(), src, sizeof(*src), &rec->desc, sizeof(rec->desc));
							}
						}
					}
				}
			}
		}
	}
}

STTSStreamsServiceUpd* TSServerStreams_sessionNewStreamsUpdateCurVsSent_(STTSServerSessionReq* s){
	STTSStreamsServiceUpd* r = NULL;
	{
		TSServerStreams_sessionNewStreamsUpdateCurVsSentRecursive_(&r, &s->sync.cur, &s->sync.sent);
	}
	return r;
}

void TSServerStreams_cltRequestedSessionStart_httpReqTick_(const STNBHttpServiceRespCtx ctx, const STNBTimestampMicro tickTime, const UI64 msCurTick, const UI32 msNextTick, UI32* dstMsNextTick, void* usrData){
	STTSServerSessionReq* s = (STTSServerSessionReq*)usrData;
    if(s != NULL && s->obj != NULL){
        STTSServerStreams* obj = (STTSServerStreams*)s->obj;
        NBThreadMutex_lock(&s->mutex);
        if(NBHttpServiceRespRawLnk_isSet(&s->rawLnk) && obj->opq != NULL){
            STTSServerOpq* opq = obj->opq;
            const char* runId = TSContext_getRunId(opq->context.cur);
            //
            const UI64 secsMaxWithoutMsg = 5; //cfg
            BOOL sendKeepAliveMsg = FALSE;
            STTSStreamsServiceUpd* streamsUpds = NULL;
            //streamsChanges (sync and populate)
            {
                //sync (locked)
                {
                    TSServerStreams_sessionSyncCurStreamsLockedOpq_(&s->sync.cur, opq->streams.reqs.mngr, &opq->streams.storage.mngr);
                    s->sync.cur.syncFnd = TRUE;
                }
                //compare (unlcoked)
                {
                    streamsUpds = TSServerStreams_sessionNewStreamsUpdateCurVsSent_(s);
                    //apply changes
                    if(streamsUpds != NULL){
                        //print
#				        ifdef NB_CONFIG_INCLUDE_ASSERTS
                        {
                            STNBString str;
                            NBString_init(&str);
                            {
                                STNBStructConcatFormat fmt;
                                NBMemory_setZeroSt(fmt, STNBStructConcatFormat);
                                fmt.objectsInNewLine = TRUE;
                                fmt.tabChar = " "; fmt.tabCharLen = 1;
                                NBString_concat(&str, "sent:\n");
                                NBStruct_stConcatAsJsonWithFormat(&str, TSStreamsServiceDesc_getSharedStructMap(), &s->sync.sent, sizeof(s->sync.sent), &fmt);
                                NBString_concat(&str, "\n\nvs cur:\n");
                                NBStruct_stConcatAsJsonWithFormat(&str, TSStreamsServiceDesc_getSharedStructMap(), &s->sync.cur, sizeof(s->sync.cur), &fmt);
                                NBString_concat(&str, "\n\nupdate-cmds:\n");
                                NBStruct_stConcatAsJsonWithFormat(&str, TSStreamsServiceUpd_getSharedStructMap(), streamsUpds, sizeof(*streamsUpds), &fmt);
                                PRINTF_INFO("TSClientRequesterDevice, session change:\n\n%s\n\n", str.str);
                            }
                            NBString_release(&str);
                        }
#					    endif
                        NBString_strFreeAndNewBuffer(&streamsUpds->runId, runId);
                        //sync cur and sent
                        {
                            NBStruct_stRelease(TSStreamsServiceDesc_getSharedStructMap(), &s->sync.sent, sizeof(s->sync.sent));
                            NBStruct_stClone(TSStreamsServiceDesc_getSharedStructMap(), &s->sync.cur, sizeof(s->sync.cur), &s->sync.sent, sizeof(s->sync.sent));
                        }
                        //Print changed state
                        {
                            STNBString str;
                            NBString_initWithSz(&str, 4096, 4096, 0.10f);
                            {
                                STNBStructConcatFormat fmt;
                                NBMemory_setZeroSt(fmt, STNBStructConcatFormat);
                                fmt.objectsInNewLine = TRUE;
                                fmt.tabChar = " "; fmt.tabCharLen = 1;
                                NBStruct_stConcatAsJsonWithFormat(&str, TSStreamsServiceUpd_getSharedStructMap(), streamsUpds, sizeof(*streamsUpds), &fmt);
                                PRINTF_INFO("TSServerStreams, session desc changed:\n%s\n", str.str);
                            }
                            NBString_release(&str);
                        }
                    }
                }
            }
            //Keep alive
            if(secsMaxWithoutMsg > 0 && (NBDatetime_getCurUTCTimestamp() - s->state.lastMsgTime) >= secsMaxWithoutMsg){
                PRINTF_INFO("TSServerStreams, session sending keep-alive-packet.\n");
                sendKeepAliveMsg = TRUE;
            }
            //Send response
            if(sendKeepAliveMsg || streamsUpds != NULL){
                STTSStreamsSessionMsg msg;
                NBMemory_setZeroSt(msg, STTSStreamsSessionMsg);
                if(s->state.msgSeq == 0){
                    msg.sessionId = NBString_strNewBuffer(s->uid);
                }
                msg.runId	= NBString_strNewBuffer(runId);
                msg.time	= NBDatetime_getCurUTCTimestamp();
                msg.iSeq	= ++s->state.msgSeq;
                //consume streams
                if(streamsUpds != NULL){
                    msg.streamsUpds = streamsUpds;
                    streamsUpds = NULL;
                }
                //Send response (unlocked)
                {
                    NBThreadMutex_unlock(&s->mutex);
                    {
                        NBASSERT(s->buff.length == 0)
                        //build
                        {
                            NBStruct_stConcatAsJson(&s->buff, TSStreamsSessionMsg_getSharedStructMap(), &msg, sizeof(msg));
                        }
                        //send
                        {
                            if(NBWebSocket_isSet(s->net.websocket)){
                                NBASSERT(s->buff2.length == 0) //should be cleaned after used
                                PRINTF_INFO("TSServerStreams, sending ws-msg: '%s'.\n", s->buff.str)
                                BYTE hdr[14];
                                const UI32 hdsSz = NBWebSocketFrame_writeHeader(ENNBWebSocketFrameOpCode_Text, 0, s->buff.length, TRUE, TRUE, hdr);
                                if(hdsSz > 0){
                                    NBString_concatBytes(&s->buff2, hdr, hdsSz);
                                }
                                if(s->buff.length > 0){
                                    NBString_concatBytes(&s->buff2, s->buff.str, s->buff.length);
                                }
                                if(!NBHttpServiceRespRawLnk_sendBytes(&s->rawLnk, s->buff2.str, s->buff2.length)){
                                    obj->state.stopFlag = /*connErr =*/ TRUE;
                                } else {
                                    PRINTF_INFO("TSServerStreams, session message websocket-sent.\n");
                                }
                                NBString_empty(&s->buff2);
                            } else if(!NBHttpServiceRespRawLnk_sendBytes(&s->rawLnk, s->buff.str, s->buff.length)){
                                obj->state.stopFlag = /*connErr =*/ TRUE;
                            } else {
                                PRINTF_INFO("TSServerStreams, session message naked-sent.\n");
                            }
                        }
                        //set
                        NBString_empty(&s->buff);
                        s->state.lastMsgTime = NBDatetime_getCurUTCTimestamp();
                    }
                    NBThreadMutex_lock(&s->mutex);
                }
                NBStruct_stRelease(TSStreamsSessionMsg_getSharedStructMap(), &msg, sizeof(msg));
            }
            //Release
            if(streamsUpds != NULL){
                NBStruct_stRelease(TSStreamsServiceUpd_getSharedStructMap(), streamsUpds, sizeof(*streamsUpds));
                NBMemory_free(streamsUpds);
                streamsUpds = NULL;
            }
        }
        NBThreadMutex_unlock(&s->mutex);
    }
    //allways 1-second ticks
    if(dstMsNextTick != NULL){
        *dstMsNextTick = 1000;
    }
}
