//
//  TSClientRequester.c
//  thinstream
//
//  Created by Marcos Ortega on 2/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSClientRequester.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/core/NBArray.h"
#include "nb/core/NBThread.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBJson.h"
#include "nb/core/NBJsonParser.h"
#include "nb/core/NBStruct.h"
#include "nb/net/NBSocket.h"
#include "nb/net/NBHttpHeader.h"
#include "nb/net/NBHttpBuilder.h"
#include "nb/net/NBRtspClient.h"

//----------------
//- TSClientRequesterCmdState
//----------------

void TSClientRequesterCmdState_init(STTSClientRequesterCmdState* obj){
	NBMemory_setZeroSt(*obj, STTSClientRequesterCmdState);
	NBRtspCmdState_init(&obj->rtsp);
}

void TSClientRequesterCmdState_release(STTSClientRequesterCmdState* obj){
	NBRtspCmdState_release(&obj->rtsp);
}

//

typedef struct STTSClientRequesterOpq_ {
	STNBObject			prnt;
	//cfg
	struct {
		STNBString		runId;
	} cfg;
	//state
	struct {
		BOOL			stopFlag;
	} state;
	//lstnrs
	struct {
		STNBArray		arr;	//STTSClientRequesterLstnr
	} lstnrs;
	//sources
	struct {
		STNBArray		arr;	//STTSClientRequesterDeviceRef
	} sources;
	//rtsp
	struct {
		STNBRtspClient	clt;
		ENNBRtspRessState stoppedState;
		STTSClientRequesterConnStats* reqsStats;
	} rtsp;
} STTSClientRequesterOpq;

void TSClientRequester_stopFlagLockedOpq_(STTSClientRequesterOpq* opq);
BOOL TSClientRequester_isBusyLockedOpq_(STTSClientRequesterOpq* opq);
//
void TSClientRequester_rtspStreamStateChanged_(void* param, struct STTSClientRequesterConn_* obj, const char* streamId, const char* versionId, const ENNBRtspRessState state);
//void TSClientRequester_rtspStreamVideoPropsChanged_(void* param, struct STTSClientRequesterConn_* obj, const char* streamId, const char* versionId, const STNBAvcPicDescBase* props); //samples = pixels
//
void TSClientRequester_notifySourcesUpdatedLockedOpq_(STTSClientRequesterOpq* opq);
//

NB_OBJREF_BODY(TSClientRequester, STTSClientRequesterOpq, NBObject)

void TSClientRequester_initZeroed(STNBObject* obj){
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)obj;
	//cfg
	{
		NBString_init(&opq->cfg.runId);
	}
	//lstnrs
	{
		NBArray_init(&opq->lstnrs.arr, sizeof(STTSClientRequesterLstnr), NULL);
	}
	//sources
	{
		NBArray_init(&opq->sources.arr, sizeof(STTSClientRequesterDeviceRef), NULL);
	}
	//rtsp
	{
		NBRtspClient_init(&opq->rtsp.clt);
	}
}

void TSClientRequester_uninitLocked(STNBObject* obj){
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)obj;
	//stop
	{
		TSClientRequester_stopFlagLockedOpq_(opq);
	}
	//lstnrs
	{
		NBArray_empty(&opq->lstnrs.arr);
		NBArray_release(&opq->lstnrs.arr);
	}
	//sources
	{
		SI32 i; for(i = 0; i < opq->sources.arr.use; i++){
			STTSClientRequesterDeviceRef dev = NBArray_itmValueAtIndex(&opq->sources.arr, STTSClientRequesterDeviceRef, i);
			NBASSERT(!TSClientRequesterDevice_isRunning(dev)) //should not be running
			TSClientRequesterDevice_release(&dev);
			TSClientRequesterDevice_null(&dev);
		}
		NBArray_empty(&opq->sources.arr);
		NBArray_release(&opq->sources.arr);
	}
	//rtps
	{
		//client
		NBRtspClient_release(&opq->rtsp.clt);
	}
	//cfg
	{
		NBString_release(&opq->cfg.runId);
	}
}

void TSClientRequester_stopFlagLockedOpq_(STTSClientRequesterOpq* opq){
	opq->state.stopFlag = TRUE;
	{
		NBRtspClient_stopFlag(&opq->rtsp.clt);
	}
	{
		SI32 i; for(i = 0; i < opq->sources.arr.use; i++){
			STTSClientRequesterDeviceRef grp = NBArray_itmValueAtIndex(&opq->sources.arr, STTSClientRequesterDeviceRef, i);
			TSClientRequesterDevice_stopFlag(grp);
		}
	}
}

void TSClientRequester_stopFlag(STTSClientRequesterRef ref){
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	{
		TSClientRequester_stopFlagLockedOpq_(opq);
	}
	NBObject_unlock(opq);
}

BOOL TSClientRequester_isBusyLockedOpq_(STTSClientRequesterOpq* opq){
	BOOL r = FALSE;
	if(!r && NBRtspClient_isBusy(&opq->rtsp.clt)){
		r = TRUE;
	}
	return r;
}

BOOL TSClientRequester_isBusy(STTSClientRequesterRef ref){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	{
		r = TSClientRequester_isBusyLockedOpq_(opq);
	}
	NBObject_unlock(opq);
	return r;
}

//Commands

void TSClientRequester_statsFlushStart(STTSClientRequesterRef ref, STTSClientRequesterCmdState* dst){
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBASSERT(opq != NULL)
	NBObject_lock(opq);
	{
		NBRtspClient_statsFlushStart(&opq->rtsp.clt, (dst != NULL ? &dst->rtsp : NULL));
		//result
		if(dst != NULL){
			dst->isPend = (dst->rtsp.isPend);
		}
	}
	NBObject_unlock(opq);
}

BOOL TSClientRequester_statsFlushIsPend(STTSClientRequesterRef ref, STTSClientRequesterCmdState* dst){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBASSERT(opq != NULL)
	if(dst != NULL){
		NBObject_lock(opq);
		{
			NBRtspClient_statsFlushIsPend(&opq->rtsp.clt, &dst->rtsp);
			//result
			r = dst->isPend = (dst->rtsp.isPend);
		}
		NBObject_unlock(opq);
	}
	return r;
}

void TSClientRequester_statsGet(STTSClientRequesterRef ref, STNBArray* dstPortsStats /*STNBRtpClientPortStatsData*/, STNBArray* dstSsrcsStats /*STNBRtpClientSsrcStatsData*/, const BOOL resetAccum){
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBASSERT(opq != NULL)
	NBObject_lock(opq);
	{
		NBRtspClient_statsGet(&opq->rtsp.clt, dstPortsStats, dstSsrcsStats, resetAccum);
	}
	NBObject_unlock(opq);
}

//cfg

BOOL TSClientRequester_setStatsProvider(STTSClientRequesterRef ref, STNBRtspClientStats* rtspStats, STTSClientRequesterConnStats* reqsStats){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	{
		if(NBRtspClient_setStatsProvider(&opq->rtsp.clt, rtspStats)){
			opq->rtsp.reqsStats = reqsStats;
			r = TRUE;
		}
	}
	NBObject_unlock(opq);
	return r;
}

BOOL TSClientRequester_setExternalBuffers(STTSClientRequesterRef ref, STNBDataPtrsPoolRef pool){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	{
		r = NBRtspClient_setExternalBuffers(&opq->rtsp.clt, pool);
	}
	NBObject_unlock(opq);
	return r;
}

BOOL TSClientRequester_setCfg(STTSClientRequesterRef ref, const STNBRtspClientCfg* cfg, const char* runId, const ENNBRtspRessState stoppedState){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	if(!NBString_strIsEmpty(runId)){
		NBObject_lock(opq);
		{
			if(NBRtspClient_setCfg(&opq->rtsp.clt, cfg)){
                NBString_set(&opq->cfg.runId, runId);
                opq->rtsp.stoppedState = stoppedState;
				r = TRUE;
			}
		}
		NBObject_unlock(opq);
	}
	return r;
}

BOOL TSClientRequester_setPollster(STTSClientRequesterRef ref, STNBIOPollsterRef pollster, STNBIOPollsterSyncRef pollSync){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	{
		//rtspClient
		if(NBRtspClient_setPollster(&opq->rtsp.clt, pollster, pollSync)){
			r = TRUE;
		}
	}
	NBObject_unlock(opq);
	return r;
}

BOOL TSClientRequester_setPollstersProvider(STTSClientRequesterRef ref, STNBIOPollstersProviderRef provider){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	{
		//rtspClient
		if(NBRtspClient_setPollstersProvider(&opq->rtsp.clt, provider)){
			r = TRUE;
		}
	}
	NBObject_unlock(opq);
	return r;
}

void TSClientRequester_tickSecChanged(STTSClientRequesterRef ref, const UI64 timestamp){
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
    STNBRtspClient clt;
    NBObject_lock(opq);
	{
        //retain
        clt = opq->rtsp.clt;
        NBRtspClient_retain(&clt);
	}
	NBObject_unlock(opq);
    //analyze and release (unlocked, notifications may occur)
    {
        const UI32 onlyIfSecsElapsed = 2;
        NBRtspClient_tickAnalyzeQuick(&clt, onlyIfSecsElapsed);
        NBRtspClient_release(&clt);
    }
}

//lstnrs

BOOL TSClientRequester_addLstnr(STTSClientRequesterRef ref, STTSClientRequesterLstnr* lstnr){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	if(lstnr != NULL){
		NBObject_lock(opq);
		{
			//Search
			SI32 i; for(i = 0; i < opq->lstnrs.arr.use; i++){
				const STTSClientRequesterLstnr* lstnr2 = NBArray_itmPtrAtIndex(&opq->lstnrs.arr, STTSClientRequesterLstnr, i);
				if(lstnr2->param == lstnr->param){
					break;
				}
			} NBASSERT(i == opq->lstnrs.arr.use)
			//add
			if(i == opq->lstnrs.arr.use){
				NBArray_addValue(&opq->lstnrs.arr, *lstnr);
			}
		}
		NBObject_unlock(opq);
	}
	return r;
}

BOOL TSClientRequester_removeLstnr(STTSClientRequesterRef ref, void* param){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	if(param != NULL){
		NBObject_lock(opq);
		{
			BOOL fnd = FALSE;
			//Search
			SI32 i; for(i = 0; i < opq->lstnrs.arr.use; i++){
				const STTSClientRequesterLstnr* lstnr2 = NBArray_itmPtrAtIndex(&opq->lstnrs.arr, STTSClientRequesterLstnr, i);
				if(lstnr2->param == param){
					NBArray_removeItemAtIndex(&opq->lstnrs.arr, i);
					fnd = TRUE;
					break;
				}
			} NBASSERT(fnd) //should be found
		}
		NBObject_unlock(opq);
	}
	return r;
}

//

BOOL TSClientRequester_prepare(STTSClientRequesterRef ref){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	{
		r = NBRtspClient_prepare(&opq->rtsp.clt);
	}
	NBObject_unlock(opq);
	return r;
}

BOOL TSClientRequester_start(STTSClientRequesterRef ref){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	{
		r = NBRtspClient_start(&opq->rtsp.clt);
	}
	NBObject_unlock(opq);
	return r;
}

//notification

void TSClientRequester_notifySourcesUpdatedLockedOpq_(STTSClientRequesterOpq* opq){
	STTSClientRequesterRef reqRef = TSClientRequester_fromOpqPtr(opq);
	STNBArray arr;
	NBArray_init(&arr, sizeof(STTSClientRequesterLstnr), NULL);
	//retain (locked)
	{
		SI32 i; for(i = 0; i < opq->lstnrs.arr.use; i++){
			STTSClientRequesterLstnr* lstnr = NBArray_itmPtrAtIndex(&opq->lstnrs.arr, STTSClientRequesterLstnr, i);
			if(lstnr->itf.requesterSourcesUpdated != NULL){
				if(lstnr->itf.retain != NULL && lstnr->itf.release != NULL){
					(*lstnr->itf.retain)(lstnr->param);
				}
				NBArray_addValue(&arr, *lstnr);
			}
		}
	}
	//notify and release(unlocked)
	if(arr.use > 0){
		NBObject_unlock(opq);
		{ 
			SI32 i; for(i = 0; i < arr.use; i++){
				STTSClientRequesterLstnr* lstnr = NBArray_itmPtrAtIndex(&arr, STTSClientRequesterLstnr, i);
				if(lstnr->itf.requesterSourcesUpdated != NULL){
					(*lstnr->itf.requesterSourcesUpdated)(lstnr->param, reqRef);
				}
				if(lstnr->itf.retain != NULL && lstnr->itf.release != NULL){
					(*lstnr->itf.release)(lstnr->param);
				}
			}
		}
		NBObject_lock(opq);
	}
	NBArray_release(&arr);
}

//rtsp

BOOL TSClientRequester_addDeviceRtspStreamLockedOpq_(STTSClientRequesterOpq* opq, STTSStreamLstnr* viewer, const char* groupId, const char* groupName, const char* streamId, const char* streamName, const char* versionId, const char* versionName, const STTSStreamAddress* address, const STTSStreamParams* params){
	BOOL r = FALSE;
	NBASSERT(params != NULL)
	if(params != NULL){
		if(!opq->state.stopFlag){
			STTSClientRequesterDeviceRef grpFnd = NB_OBJREF_NULL;
			//grp
			{
				//search 
				SI32 i; for(i = 0; i < opq->sources.arr.use; i++){
					STTSClientRequesterDeviceRef grp = NBArray_itmValueAtIndex(&opq->sources.arr, STTSClientRequesterDeviceRef, i);
					STTSClientRequesterDeviceOpq* grpOpq = (STTSClientRequesterDeviceOpq*)grp.opaque;
					NBASSERT(grpOpq->cfg.type != ENTSClientRequesterDeviceType_Local || (grpOpq->remoteDesc.streamsGrpsSz == 1 && grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1))
					if(grpOpq->cfg.type == ENTSClientRequesterDeviceType_Local
					   && grpOpq->cfg.server.port == address->port
					   && NBString_strIsEqual(grpOpq->cfg.server.server, address->server)
					   && (
						grpOpq->remoteDesc.streamsGrpsSz == 1
						&& grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1
						&& NBString_strIsEqual(grpOpq->remoteDesc.streamsGrps[0].streams[0].uid, streamId) 
						)
					   ){
						NBASSERT(!NBString_strIsEmpty(grpOpq->remoteDesc.runId) && NBString_strIsEqual(grpOpq->remoteDesc.runId, opq->cfg.runId.str))
						NBASSERT(grpOpq->remoteDesc.streamsGrps != NULL && grpOpq->remoteDesc.streamsGrpsSz == 1)
                        grpFnd = grp;
						break;
					}
				}
				//create
				if(!TSClientRequesterDevice_isSet(grpFnd)){
					STTSClientRequesterDeviceRef grp = TSClientRequesterDevice_alloc(NULL);
					if(TSClientRequesterDevice_startAsRtsp(grp, opq->cfg.runId.str, groupId, groupName, streamId, streamName, address->server, address->port)){
						//consume
						NBArray_addValue(&opq->sources.arr, grp);
                        //consume
                        grpFnd = grp; TSClientRequesterDevice_null(&grp);
					}
					//release (if not consumed)
					if(TSClientRequesterDevice_isSet(grp)){
						TSClientRequesterDevice_release(&grp);
						TSClientRequesterDevice_null(&grp);
					}
				}
			}
			//requester
			NBASSERT(!r)
			if(TSClientRequesterDevice_isSet(grpFnd)){
				STTSClientRequesterDeviceOpq* grppOpq = (STTSClientRequesterDeviceOpq*)grpFnd.opaque;
				STTSStreamsServiceDesc* desc = &grppOpq->remoteDesc; NBASSERT(desc->streamsGrps != NULL && desc->streamsGrpsSz == 1 && NBString_strIsEqual(desc->streamsGrps[0].uid, groupId));
				STTSStreamsGrpDesc* grpDesc = &desc->streamsGrps[0]; NBASSERT(grpDesc->streams != NULL && grpDesc->streamsSz == 1 && NBString_strIsEqual(grpDesc->streams[0].uid, streamId));
				STTSStreamDesc* strmDesc = &grpDesc->streams[0];
				//version
				{
					STTSStreamVerDesc* verDesc = NULL;
					//search
					if(strmDesc->versions != NULL && strmDesc->versionsSz > 0){
						UI32 i; for(i = 0; i < strmDesc->versionsSz; i++){
							STTSStreamVerDesc* verDesc2 = &strmDesc->versions[i];
							if(NBString_strIsEqual(verDesc2->uid, versionId)){
								verDesc = verDesc2;
								break;
							}
						}
					}
					//add
					if(verDesc == NULL){
						STTSStreamVerDesc* nArr = NBMemory_allocTypes(STTSStreamVerDesc, strmDesc->versionsSz + 1);
						if(strmDesc->versions != NULL){
							if(strmDesc->versionsSz > 0){
								NBMemory_copy(nArr, strmDesc->versions, sizeof(strmDesc->versions[0]) * strmDesc->versionsSz);
							}
							NBMemory_free(strmDesc->versions);
						}
						strmDesc->versions = nArr;
						verDesc = &strmDesc->versions[strmDesc->versionsSz++];
						NBMemory_setZeroSt(*verDesc, STTSStreamVerDesc);
						verDesc->iSeq	= 1;
						verDesc->uid	= NBString_strNewBuffer(versionId);
						verDesc->name	= NBString_strNewBuffer(versionName);
					}
				}
				//Requester
				{
					STTSClientRequesterConn* reqq = NULL;
					//search
					{
						SI32 i; for(i = 0; i < grppOpq->rtsp.reqs.use; i++){
							STTSClientRequesterConn* req2 = NBArray_itmValueAtIndex(&grppOpq->rtsp.reqs, STTSClientRequesterConn*, i);
							if(NBString_strIsEqual(req2->cfg.versionId, versionId)){
								//PRINTF_INFO("TSServer, requester found for viewer: '%s'.\n", address->uri);
								reqq = req2;
								break;
							}
						}
					}
					//add
					if(reqq != NULL){
						if(viewer == NULL){
							r = TRUE;
						} else {
							r = TSClientRequesterConn_addViewer(reqq, viewer, NULL);
						}
					} else {
						STTSClientRequesterConnLstnr lstnr;
						NBMemory_setZeroSt(lstnr, STTSClientRequesterConnLstnr);
						lstnr.param = opq;
						lstnr.itf.rtspStreamStateChanged = TSClientRequester_rtspStreamStateChanged_;
						//lstnr.itf.rtspStreamVideoPropsChanged = TSClientRequester_rtspStreamVideoPropsChanged_;
						{
							STTSClientRequesterConn* req2 = NBMemory_allocType(STTSClientRequesterConn);
							TSClientRequesterConn_init(req2);
							if(!TSClientRequesterConn_setStatsProvider(req2, opq->rtsp.reqsStats)){
								PRINTF_CONSOLE_ERROR("TSServer, TSClientRequesterConn_setStatsProvider failed.\n");
							} else if(!TSClientRequesterConn_setRtspClient(req2, &opq->rtsp.clt, opq->rtsp.stoppedState)){
								PRINTF_CONSOLE_ERROR("TSServer, TSClientRequesterConn_setRtspClient failed.\n");
							} else if(!TSClientRequesterConn_setConfig(req2, opq->cfg.runId.str, groupId, groupName, streamId, streamName, versionId, versionName, address, params)){
								PRINTF_CONSOLE_ERROR("TSServer, TSClientRequesterConn_setConfig failed.\n");
							} else if(!TSClientRequesterConn_setLstnr(req2, &lstnr)){
								PRINTF_CONSOLE_ERROR("TSServer, TSClientRequesterConn_setLstnr failed.\n");
							} else {
								//Consume
								NBArray_addValue(&grppOpq->rtsp.reqs, req2);
								//add viewer
								if(viewer == NULL){
									r = TRUE;
								} else {
									r = TSClientRequesterConn_addViewer(req2, viewer, NULL);
								}
								//start requester
								{
									TSClientRequesterConn_start(req2);
								}
								//PRINTF_INFO("TSServer, requester added for viewer: '%s' (%d in total).\n", address->uri, opq->streams.reqs.arr.use);
								reqq	= req2;
								req2	= NULL;
							}
							//Release (if not consumed)
							if(req2 != NULL){
								TSClientRequesterConn_release(req2);
								NBMemory_free(req2);
								req2 = NULL;
							}
						}
					}
				}
				//Notify
				if(r){
					TSClientRequester_notifySourcesUpdatedLockedOpq_(opq);
				}
			}
		}
	}
	return r;
}

BOOL TSClientRequester_addDeviceRtspStream(STTSClientRequesterRef ref, const char* groupId, const char* groupName, const char* streamId, const char* streamName, const char* versionId, const char* versionName, const STTSStreamAddress* address, const STTSStreamParams* params){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	if(params != NULL){
		NBObject_lock(opq);
		{
			r = TSClientRequester_addDeviceRtspStreamLockedOpq_(opq, NULL, groupId, groupName, streamId, streamName, versionId, versionName, address, params);
		}
		NBObject_unlock(opq);
	}
	return r;
}

BOOL TSClientRequester_rtspAddStream(STTSClientRequesterRef ref, const char* groupId, const char* groupName, const char* streamId, const char* streamName, const char* versionId, const char* versionName, const STTSStreamAddress* address, const STTSStreamParams* params){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	if(params != NULL){
		NBObject_lock(opq);
		{
			r = TSClientRequester_addDeviceRtspStreamLockedOpq_(opq, NULL, groupId, groupName, streamId, streamName, versionId, versionName, address, params);
		}
		NBObject_unlock(opq);
	}
	return r;
}

BOOL TSClientRequester_rtspAddViewer(STTSClientRequesterRef ref, STTSStreamLstnr* viewer, const char* groupId, const char* groupName, const char* streamId, const char* streamName, const char* versionId, const char* versionName, const STTSStreamAddress* address, const STTSStreamParams* params){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	if(viewer != NULL && params != NULL){
		NBObject_lock(opq);
		{
			r = TSClientRequester_addDeviceRtspStreamLockedOpq_(opq, viewer, groupId, groupName, streamId, streamName, versionId, versionName, address, params);
		}
		NBObject_unlock(opq);
	}
	return r;
}

BOOL TSClientRequester_rtspRemoveViewer(STTSClientRequesterRef ref, STTSStreamLstnr* viewer){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	if(viewer != NULL){
		NBObject_lock(opq);
		{
			SI32 i; for(i = 0; i < opq->sources.arr.use; i++){
				STTSClientRequesterDeviceRef grp = NBArray_itmValueAtIndex(&opq->sources.arr, STTSClientRequesterDeviceRef, i);
				STTSClientRequesterDeviceOpq* grpOpq = (STTSClientRequesterDeviceOpq*)grp.opaque;
				NBASSERT(grpOpq->cfg.type != ENTSClientRequesterDeviceType_Local || (grpOpq->remoteDesc.streamsGrpsSz == 1 && grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1))
				if(grpOpq->cfg.type == ENTSClientRequesterDeviceType_Local){
					NBASSERT(!NBString_strIsEmpty(grpOpq->remoteDesc.runId) && NBString_strIsEqual(grpOpq->remoteDesc.runId, opq->cfg.runId.str))
					NBASSERT(grpOpq->remoteDesc.streamsGrps != NULL && grpOpq->remoteDesc.streamsGrpsSz == 1)
					SI32 i; for(i = 0; i < grpOpq->rtsp.reqs.use; i++){
						STTSClientRequesterConn* req2 = NBArray_itmValueAtIndex(&grpOpq->rtsp.reqs, STTSClientRequesterConn*, i);
						if(TSClientRequesterConn_removeViewer(req2, viewer->param, NULL)){
							r = TRUE;
						}
					}
				}
			}
			//Notify
			if(r){
				TSClientRequester_notifySourcesUpdatedLockedOpq_(opq);
			}
		}
		NBObject_unlock(opq);
	}
	return r;
}

void TSClientRequester_rtspStreamStateChanged_(void* param, struct STTSClientRequesterConn_* obj, const char* streamId, const char* versionId, const ENNBRtspRessState state){
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)param;
	if(opq != NULL){
		NBObject_lock(opq);
		{
			BOOL fnd = FALSE;
			SI32 i; for(i = 0; i < opq->sources.arr.use && !fnd; i++){
				STTSClientRequesterDeviceRef grp = NBArray_itmValueAtIndex(&opq->sources.arr, STTSClientRequesterDeviceRef, i);
				if(TSClientRequesterDevice_rtspStreamStateChanged(grp, streamId, versionId, state)){
					fnd = TRUE;
				}
			} NBASSERT(fnd) //must be found
		}
		NBObject_unlock(opq);
	}
}

//samples = pixels
/*void TSClientRequester_rtspStreamVideoPropsChanged_(void* param, struct STTSClientRequesterConn_* obj, const char* streamId, const char* versionId, const STNBAvcPicDescBase* props){
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)param;
	PRINTF_INFO("TSClientRequester, '%s/%s' props-changed width(%d) height(%d) maxFps(%d) isFixedFps('%s').\n", streamId, versionId, props->w, props->h, props->fpsMax, (props->isFixedFps ? "yes" : "no"));
	if(opq != NULL){
		NBObject_lock(opq);
		{
			BOOL fnd = FALSE;
			SI32 i; for(i = 0; i < opq->sources.arr.use && !fnd; i++){
				STTSClientRequesterDeviceRef grp = NBArray_itmValueAtIndex(&opq->sources.arr, STTSClientRequesterDeviceRef, i);
				if(TSClientRequesterDevice_rtspStreamVideoPropsChanged(grp, streamId, versionId, props)){
					fnd = TRUE;
				}
			} NBASSERT(fnd) //must be found
		}
		NBObject_unlock(opq);
	}
}*/

BOOL TSClientRequester_rtspConcatTargetBySsrc(STTSClientRequesterRef ref, const UI16 ssrc, STNBString* dst){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	{
		SI32 i; for(i = 0; i < opq->sources.arr.use && !r; i++){
			STTSClientRequesterDeviceRef grp = NBArray_itmValueAtIndex(&opq->sources.arr, STTSClientRequesterDeviceRef, i);
			STTSClientRequesterDeviceOpq* grpOpq = (STTSClientRequesterDeviceOpq*)grp.opaque;
			NBASSERT(grpOpq->cfg.type != ENTSClientRequesterDeviceType_Local || (grpOpq->remoteDesc.streamsGrpsSz == 1 && grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1))
			if(grpOpq->cfg.type == ENTSClientRequesterDeviceType_Local){
				NBASSERT(!NBString_strIsEmpty(grpOpq->remoteDesc.runId) && NBString_strIsEqual(grpOpq->remoteDesc.runId, opq->cfg.runId.str))
				NBASSERT(grpOpq->remoteDesc.streamsGrps != NULL && grpOpq->remoteDesc.streamsGrpsSz == 1)
				SI32 i; for(i = 0; i < grpOpq->rtsp.reqs.use; i++){
					STTSClientRequesterConn* req2 = NBArray_itmValueAtIndex(&grpOpq->rtsp.reqs, STTSClientRequesterConn*, i);
					if(req2->state.rtp.ssrc == ssrc){
						if(!NBString_strIsEmpty(req2->cfg.address->uri)){
							NBString_concat(dst, req2->cfg.address->uri);
							r = TRUE;
							break;
						}
					}
				}
			}
		}
	}
	NBObject_unlock(opq);
	return r;
}

BOOL TSClientRequester_rtspConcatStreamVersionIdBySsrc(STTSClientRequesterRef ref, const UI16 ssrc, STNBString* dst){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	{
		SI32 i; for(i = 0; i < opq->sources.arr.use && !r; i++){
			STTSClientRequesterDeviceRef grp = NBArray_itmValueAtIndex(&opq->sources.arr, STTSClientRequesterDeviceRef, i);
			STTSClientRequesterDeviceOpq* grpOpq = (STTSClientRequesterDeviceOpq*)grp.opaque;
			NBASSERT(grpOpq->cfg.type != ENTSClientRequesterDeviceType_Local || (grpOpq->remoteDesc.streamsGrpsSz == 1 && grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1))
			if(grpOpq->cfg.type == ENTSClientRequesterDeviceType_Local){
				NBASSERT(!NBString_strIsEmpty(grpOpq->remoteDesc.runId) && NBString_strIsEqual(grpOpq->remoteDesc.runId, opq->cfg.runId.str))
				NBASSERT(grpOpq->remoteDesc.streamsGrps != NULL && grpOpq->remoteDesc.streamsGrpsSz == 1)
				SI32 i; for(i = 0; i < grpOpq->rtsp.reqs.use; i++){
					STTSClientRequesterConn* req2 = NBArray_itmValueAtIndex(&grpOpq->rtsp.reqs, STTSClientRequesterConn*, i);
					if(req2->state.rtp.ssrc == ssrc){
						const UI32 lenBefore = dst->length;
						if(!NBString_strIsEmpty(req2->cfg.streamId)){
							if(lenBefore != dst->length){
								NBString_concatByte(dst, '/');
							}
							NBString_concat(dst, req2->cfg.streamId);
						}
						if(!NBString_strIsEmpty(req2->cfg.versionId)){
							if(lenBefore != dst->length){
								NBString_concatByte(dst, '/');
							}
							NBString_concat(dst, req2->cfg.versionId);
						}
						if(lenBefore != dst->length){
							r = TRUE;
							break;
						}
					}
				}
			}
		}
	}
	NBObject_unlock(opq);
	return r;
}

//

STTSClientRequesterDeviceRef* TSClientRequester_getGrpsAndLock(STTSClientRequesterRef ref, SI32* dstSz){
	STTSClientRequesterDeviceRef* r = NULL;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	if(dstSz != NULL){
		*dstSz = opq->sources.arr.use;
	}
	r = NBArray_dataPtr(&opq->sources.arr, STTSClientRequesterDeviceRef);
	if(r == NULL){
		NBObject_unlock(opq);
	}
	return r;
} 

void TSClientRequester_returnGrpsAndUnlock(STTSClientRequesterRef ref, STTSClientRequesterDeviceRef* grps){
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	if(grps != NULL){
		NBASSERT(NBArray_dataPtr(&opq->sources.arr, STTSClientRequesterDeviceRef) == grps)
		NBObject_unlock(opq);
	}
}

//

BOOL TSClientRequester_isRtspReqStreamIdAtGroupId(STTSClientRequesterRef ref, const char* streamId, const char* groupId){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	{
		SI32 i; for(i = 0; i < opq->sources.arr.use; i++){
			STTSClientRequesterDeviceRef grp = NBArray_itmValueAtIndex(&opq->sources.arr, STTSClientRequesterDeviceRef, i);
			STTSClientRequesterDeviceOpq* grpOpq = (STTSClientRequesterDeviceOpq*)grp.opaque; 
			NBASSERT(grpOpq->cfg.type != ENTSClientRequesterDeviceType_Local || (grpOpq->remoteDesc.streamsGrpsSz == 1 && grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1))
			if(grpOpq->cfg.type == ENTSClientRequesterDeviceType_Local
			   && (
				grpOpq->remoteDesc.streamsGrpsSz == 1
				&& grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1
				&& NBString_strIsEqual(grpOpq->remoteDesc.streamsGrps[0].streams[0].uid, streamId)
				)
			   )
			{
				NBASSERT(!NBString_strIsEmpty(grpOpq->remoteDesc.runId) && NBString_strIsEqual(grpOpq->remoteDesc.runId, opq->cfg.runId.str))
				NBASSERT(grpOpq->remoteDesc.streamsGrps != NULL && grpOpq->remoteDesc.streamsGrpsSz == 1)
				r = NBString_strIsEqual(grpOpq->remoteDesc.streamsGrps[0].uid, groupId);
				break;
			}
		}
	}
	NBObject_unlock(opq);
	return r;
}

STTSClientRequesterConn* TSClientRequester_getRtspReqRetained(STTSClientRequesterRef ref, const char* streamId, const char* versionId){
	STTSClientRequesterConn* r = NULL;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	{
		SI32 i; for(i = 0; i < opq->sources.arr.use && r == NULL; i++){
			STTSClientRequesterDeviceRef grp = NBArray_itmValueAtIndex(&opq->sources.arr, STTSClientRequesterDeviceRef, i);
			STTSClientRequesterDeviceOpq* grpOpq = (STTSClientRequesterDeviceOpq*)grp.opaque; 
			NBASSERT(grpOpq->cfg.type != ENTSClientRequesterDeviceType_Local || (grpOpq->remoteDesc.streamsGrpsSz == 1 && grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1))
			if(grpOpq->cfg.type == ENTSClientRequesterDeviceType_Local
			   && (
				grpOpq->remoteDesc.streamsGrpsSz == 1
				&& grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1
				&& NBString_strIsEqual(grpOpq->remoteDesc.streamsGrps[0].streams[0].uid, streamId)
				)
			   )
			{
				NBASSERT(!NBString_strIsEmpty(grpOpq->remoteDesc.runId) && NBString_strIsEqual(grpOpq->remoteDesc.runId, opq->cfg.runId.str))
				NBASSERT(grpOpq->remoteDesc.streamsGrps != NULL && grpOpq->remoteDesc.streamsGrpsSz == 1)
				SI32 i; for(i = 0; i < grpOpq->rtsp.reqs.use; i++){
					STTSClientRequesterConn* req2 = NBArray_itmValueAtIndex(&grpOpq->rtsp.reqs, STTSClientRequesterConn*, i);
					NBASSERT(NBString_strIsEqual(streamId, req2->cfg.streamId))
					if(NBString_strIsEqual(versionId, req2->cfg.versionId)){
						//ToDo: implement retainer
						r = req2;
						break;
					}
				}
			}
		}
	}
	NBObject_unlock(opq);
	return r;
}

void TSClientRequester_returnRtspReq(STTSClientRequesterRef ref, STTSClientRequesterConn* req){
	//ToDo: implement release
}

//thinstream

BOOL TSClientRequester_addDeviceThinstream(STTSClientRequesterRef ref, const char* server, const UI16 port){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	if(!opq->state.stopFlag){
		//search
		SI32 i; for(i = 0; i < opq->sources.arr.use; i++){
			STTSClientRequesterDeviceRef grp = NBArray_itmValueAtIndex(&opq->sources.arr, STTSClientRequesterDeviceRef, i);
			STTSClientRequesterDeviceOpq* grpOpq = (STTSClientRequesterDeviceOpq*)grp.opaque;
			if(
			   grpOpq->cfg.type == ENTSClientRequesterDeviceType_Native
			   && grpOpq->cfg.server.port == port
			   && NBString_strIsEqual(grpOpq->cfg.server.server, server)
			   )
			{
				break;
			}
		}
		//add
		if(i == opq->sources.arr.use){
			STTSClientRequesterDeviceRef grp = TSClientRequesterDevice_alloc(NULL);
			if(TSClientRequesterDevice_startAsThinstream(grp, server, port)){
				//consume
				NBArray_addValue(&opq->sources.arr, grp);
				TSClientRequesterDevice_null(&grp);
				r = TRUE;
			}
			//release (if not consumed)
			if(TSClientRequesterDevice_isSet(grp)){
				TSClientRequesterDevice_release(&grp);
				TSClientRequesterDevice_null(&grp);
			}
		}
	}
	NBObject_unlock(opq);
	return r;
}

//viewers

/*BOOL TSClientRequester_addStreamViewer(STTSClientRequesterRef ref, const char* optRootId, const char* runId, const char* streamId, const char* versionId, const char* sourceId, STTSStreamLstnr* viewer){
	BOOL r = FALSE;
	STTSClientRequesterOpq* opq = (STTSClientRequesterOpq*)ref.opaque; NBASSERT(TSClientRequester_isClass(ref))
	NBObject_lock(opq);
	if(!opq->state.stopFlag){
		//search best candidates
		STTSClientRequesterDeviceRef grpBest = NB_OBJREF_NULL;
		UI32 grpBestDepth = 0;
		//
		SI32 i; for(i = 0; i < opq->sources.arr.use; i++){
			STTSClientRequesterDeviceRef grp = NBArray_itmValueAtIndex(&opq->sources.arr, STTSClientRequesterDeviceRef, i);
			
			if(grpIsBest){
				TSClientRequesterDevice_set(&grpBest, &grp);
				grpBestDepth = grpDepth;
			}
		}
		//process into best group found
		if(TSClientRequesterDevice_isSet(grpBest)){
			STTSClientRequesterDeviceOpq* grpOpq = (STTSClientRequesterDeviceOpq*)grpBest.opaque;
			NBObject_lock(grpOpq);
			{
				STTSClientRequesterConn* conn = NULL;
				//search current connection
				{
					SI32 i; for(i = 0; i < grpOpq->rtsp.reqs.use; i++){
						STTSClientRequesterConn* conn2 = NBArray_itmValueAtIndex(&grpOpq->rtsp.reqs, STTSClientRequesterConn*, i);
						if(
						   NBString_strIsEqual(conn2->cfg.runId, runId)
						   && NBString_strIsEqual(conn2->cfg.streamId, streamId)
						   && NBString_strIsEqual(conn2->cfg.versionId, versionId)
						   )
						{
							conn = conn2;
							break;
						}
					}
					NBASSERT(conn != NULL) //implement
					//create connection
					/ *if(conn == NULL){
						STTSClientRequesterConnLstnr lstnr;
						NBMemory_setZeroSt(lstnr, STTSClientRequesterConnLstnr);
						lstnr.param = opq;
						lstnr.itf.rtspStreamStateChanged = TSClientRequester_rtspStreamStateChanged_;
						lstnr.itf.rtspStreamVideoPropsChanged = TSClientRequester_rtspStreamVideoPropsChanged_;
						{
							STTSClientRequesterConn* conn2 = NBMemory_allocType(STTSClientRequesterConn);
							TSClientRequesterConn_init(conn2);
							if(!TSClientRequesterConn_setStatsProvider(conn2, opq->rtsp.reqsStats)){
								PRINTF_CONSOLE_ERROR("TSServer, TSClientRequesterConn_setStatsProvider failed.\n");
							} else if(!TSClientRequesterConn_setVirtualClock(req2, opq->vClock)){
								PRINTF_CONSOLE_ERROR("TSServer, TSClientRequesterConn_setVirtualClock failed.\n");
							} else if(!TSClientRequesterConn_setRtspClient(conn2, &opq->rtsp.clt, opq->rtsp.stoppedState)){
								PRINTF_CONSOLE_ERROR("TSServer, TSClientRequesterConn_setRtspClient failed.\n");
							} else if(!TSClientRequesterConn_setConfig(conn2, runId, groupId, groupName, streamId, streamName, versionId, versionName, address, params)){
								PRINTF_CONSOLE_ERROR("TSServer, TSClientRequesterConn_setConfig failed.\n");
							} else if(!TSClientRequesterConn_setLstnr(conn2, &lstnr)){
								PRINTF_CONSOLE_ERROR("TSServer, TSClientRequesterConn_setLstnr failed.\n");
							} else {
								//Consume
								NBArray_addValue(&grpOpq->rtsp.reqs, conn2);
								conn	= conn2;
								conn2	= NULL;
							}
							//Release (if not consumed)
							if(conn2 != NULL){
								TSClientRequesterConn_release(req2);
								NBMemory_free(conn2);
								conn2 = NULL;
							}
						}
					}* /
					//Search viwer
					if(conn != NULL){
						TSClientRequesterConn_addViewer(<#STTSClientRequesterConn *obj#>, <#const STTSStreamLstnr *viewer#>, <#UI32 *dstCount#>);
					}
				}
			}
			NBObject_unlock(grpOpq);
		}
		//add
		if(i == opq->sources.arr.use){
			STTSClientRequesterDeviceRef grp = TSClientRequesterDevice_alloc(NULL);
			if(TSClientRequesterDevice_startAsThinstream(grp, server, port)){
				//consume
				NBArray_addValue(&opq->sources.arr, grp);
				NBThreadCond_broadcast(&opq->cond);
				TSClientRequesterDevice_null(&grp);
				r = TRUE;
			}
			//release (if not consumed)
			if(TSClientRequesterDevice_isSet(grp)){
				TSClientRequesterDevice_release(&grp);
				TSClientRequesterDevice_null(&grp);
			}
		}
	}
	NBObject_unlock(opq);
	return r;
}*/

/*BOOL TSClientRequester_removeStreamViewer(STTSClientRequesterRef ref, const char* optRootId, const char* runId, const char* streamId, const char* versionId, const char* sourceId, void* param){
	
}*/
