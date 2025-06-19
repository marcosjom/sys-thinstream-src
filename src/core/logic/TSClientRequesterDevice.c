//
//  TSClientRequesterDevice.c
//  thinstream
//
//  Created by Marcos Ortega on 2/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSClientRequesterDevice.h"
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

//Device

NB_OBJREF_BODY(TSClientRequesterDevice, STTSClientRequesterDeviceOpq, NBObject)

void TSClientRequesterDevice_stopFlagLockedOpq_(STTSClientRequesterDeviceOpq* opq);
void TSClientRequesterDevice_stopLockedOpq_(STTSClientRequesterDeviceOpq* opq);
//notification
void TSClientRequesterDevice_notifyConnStateChangeLockedOpq_(STTSClientRequesterDeviceOpq* opq);
void TSClientRequesterDevice_notifyDescChangedLockedOpq_(STTSClientRequesterDeviceOpq* opq);
//
SI64 TSClientRequesterDevice_syncReceiverThread_(STNBThread* t, void* param);

void TSClientRequesterDevice_initZeroed(STNBObject* obj){
	STTSClientRequesterDeviceOpq* opq = (STTSClientRequesterDeviceOpq*)obj;
	NBThreadCond_init(&opq->cond);
	{
		//lstnr
		{
			NBArray_init(&opq->lstnrs.arr, sizeof(STTSClientRequesterDeviceLstnr), NULL);
		}
		//rtsp
		{
			NBArray_init(&opq->rtsp.reqs, sizeof(STTSClientRequesterConn*), NULL);
		}
		//syncing
		{
			opq->syncing.socket	= NBSocket_alloc(NULL);
		}
	}
}

void TSClientRequesterDevice_uninitLocked(STNBObject* obj){
	STTSClientRequesterDeviceOpq* opq = (STTSClientRequesterDeviceOpq*)obj;
	//stop
	{
		TSClientRequesterDevice_stopFlagLockedOpq_(opq);
		TSClientRequesterDevice_stopLockedOpq_(opq);
	}
	//cfg
	{
		if(opq->cfg.server.server != NULL){
			NBMemory_free(opq->cfg.server.server);
			opq->cfg.server.server = NULL;
		}
	}
	//lstnr
	{
		NBArray_empty(&opq->lstnrs.arr);
		NBArray_release(&opq->lstnrs.arr);
	}
	//rtsp
	{
		//reqs
		{
			SI32 i; for(i = 0; i < opq->rtsp.reqs.use; i++){
				STTSClientRequesterConn* req = NBArray_itmValueAtIndex(&opq->rtsp.reqs, STTSClientRequesterConn*, i);
				TSClientRequesterConn_release(req);
				NBMemory_free(req);
			}
			NBArray_empty(&opq->rtsp.reqs);
			NBArray_release(&opq->rtsp.reqs);
		}
	}
	//syncing
	{
		NBASSERT(!opq->syncing.isRunning)
		{
			NBStruct_stRelease(TSStreamsServiceDesc_getSharedStructMap(), &opq->remoteDesc, sizeof(opq->remoteDesc));
		}
		if(NBSocket_isSet(opq->syncing.socket)){
			NBSocket_release(&opq->syncing.socket);
			NBSocket_null(&opq->syncing.socket);
		}
	}
	NBThreadCond_release(&opq->cond);
}

void TSClientRequesterDevice_stopFlagLockedOpq_(STTSClientRequesterDeviceOpq* opq){
	opq->syncing.stopFlag = TRUE;
	if(NBSocket_isSet(opq->syncing.socket)){
		NBSocket_close(opq->syncing.socket);
	}
}

void TSClientRequesterDevice_stopLockedOpq_(STTSClientRequesterDeviceOpq* opq){
	//stop flag
	TSClientRequesterDevice_stopFlagLockedOpq_(opq);
	//wait for all
	while(opq->syncing.isRunning) {
		if(NBSocket_isSet(opq->syncing.socket)){
			NBSocket_close(opq->syncing.socket);
		}
		NBThreadCond_waitObj(&opq->cond, opq);
	}
}

void TSClientRequesterDevice_stopFlag(STTSClientRequesterDeviceRef ref){
	STTSClientRequesterDeviceOpq* opq = (STTSClientRequesterDeviceOpq*)ref.opaque; NBASSERT(TSClientRequesterDevice_isClass(ref));
	NBObject_lock(opq);
	{
		TSClientRequesterDevice_stopFlagLockedOpq_(opq);
	}
	NBObject_unlock(opq);
}

void TSClientRequesterDevice_stop(STTSClientRequesterDeviceRef ref){
	STTSClientRequesterDeviceOpq* opq = (STTSClientRequesterDeviceOpq*)ref.opaque; NBASSERT(TSClientRequesterDevice_isClass(ref));
	NBObject_lock(opq);
	{
		TSClientRequesterDevice_stopLockedOpq_(opq);
	}
	NBObject_unlock(opq);
}

BOOL TSClientRequesterDevice_isRunning(STTSClientRequesterDeviceRef ref){
	STTSClientRequesterDeviceOpq* opq = (STTSClientRequesterDeviceOpq*)ref.opaque; NBASSERT(TSClientRequesterDevice_isClass(ref));
	return (
			//rtsp condition
			opq->connState != ENTSClientRequesterDeviceConnState_Disconnected
			//thinstream condition
			|| opq->syncing.isRunning
			);
}

//

BOOL TSClientRequesterDevice_rtspStreamStateChanged(STTSClientRequesterDeviceRef ref, const char* streamId, const char* versionId, const ENNBRtspRessState state){
	BOOL r = FALSE;
	STTSClientRequesterDeviceOpq* grpOpq = (STTSClientRequesterDeviceOpq*)ref.opaque;
	NBASSERT(grpOpq->cfg.type != ENTSClientRequesterDeviceType_Local || (grpOpq->remoteDesc.streamsGrpsSz == 1 && grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1))
	if(grpOpq->cfg.type == ENTSClientRequesterDeviceType_Local
	   && (
		grpOpq->remoteDesc.streamsGrpsSz == 1
		&& grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1
		&& NBString_strIsEqual(grpOpq->remoteDesc.streamsGrps[0].streams[0].uid, streamId)
		)
	   )
	{
		NBObject_lock(grpOpq);
		{
			//NBASSERT(!NBString_strIsEmpty(grpOpq->remoteDesc.runId) && NBString_strIsEqual(grpOpq->remoteDesc.runId, opq->cfg.runId.str))
			NBASSERT(grpOpq->remoteDesc.streamsGrps != NULL && grpOpq->remoteDesc.streamsGrpsSz == 1)
			BOOL descChanged = FALSE, connChanged = FALSE;
			STTSStreamsServiceDesc* desc = &grpOpq->remoteDesc; 
			if(desc->streamsGrps != NULL && desc->streamsGrpsSz > 0){
				NBASSERT(desc->streamsGrpsSz == 1)
				STTSStreamsGrpDesc* grpDesc = &desc->streamsGrps[0];
				if(grpDesc->streams != NULL && grpDesc->streamsSz > 0){
					UI32 i; for(i = 0; i < grpDesc->streamsSz; i++){
						STTSStreamDesc* strDesc = &grpDesc->streams[i];
						if(NBString_strIsEqual(strDesc->uid, streamId)){
							if(strDesc->versions != NULL && strDesc->versionsSz > 0){
								UI32 i; for(i = 0; i < strDesc->versionsSz; i++){
									STTSStreamVerDesc* verDesc = &strDesc->versions[i];
									if(NBString_strIsEqual(verDesc->uid, versionId)){
										const BOOL isLiveBefore = verDesc->live.isOnline; 
										if(verDesc->live.isOnline && state < ENNBRtspRessState_Described){
											verDesc->live.isOnline = FALSE;
										} else if(!verDesc->live.isOnline && state >= ENNBRtspRessState_Described){
											verDesc->live.isOnline = TRUE;
										}
										descChanged = (isLiveBefore != verDesc->live.isOnline);
										r = TRUE;
										break;
									}
								}
							}
							break;
						}
					}
				}
			}
			//Count live streams to determine connection state
			{
				UI32 ammVersionsAlive = 0;
				if(desc->streamsGrps != NULL && desc->streamsGrpsSz > 0){
					UI32 i; for(i = 0; i < desc->streamsGrpsSz; i++){
						const STTSStreamsGrpDesc* grpDesc = &desc->streamsGrps[i];
						if(grpDesc->streams != NULL && grpDesc->streamsSz > 0){
							UI32 i; for(i = 0; i < grpDesc->streamsSz; i++){
								const STTSStreamDesc* strDesc = &grpDesc->streams[i];
								if(strDesc->versions != NULL && strDesc->versionsSz > 0){
									UI32 i; for(i = 0; i < strDesc->versionsSz; i++){
										const STTSStreamVerDesc* verDesc = &strDesc->versions[i];
										if(verDesc->live.isOnline){
											ammVersionsAlive++;
										}
									}
								}
							}
						}
					}
				}
				if(grpOpq->connState == ENTSClientRequesterDeviceConnState_Disconnected && ammVersionsAlive != 0){
					grpOpq->connState = ENTSClientRequesterDeviceConnState_Connected;
					connChanged = TRUE;
				} else if(grpOpq->connState == ENTSClientRequesterDeviceConnState_Connected && ammVersionsAlive == 0){
					grpOpq->connState = ENTSClientRequesterDeviceConnState_Disconnected;
					connChanged = TRUE;
				}
			}
			if(descChanged || connChanged){
				NBThreadCond_broadcast(&grpOpq->cond);
				if(connChanged){
					TSClientRequesterDevice_notifyConnStateChangeLockedOpq_(grpOpq);
				}
				if(descChanged){
					TSClientRequesterDevice_notifyDescChangedLockedOpq_(grpOpq);
				}
			}
		}
		NBObject_unlock(grpOpq);
	}
	return r;
}

/*BOOL TSClientRequesterDevice_rtspStreamVideoPropsChanged(STTSClientRequesterDeviceRef ref, const char* streamId, const char* versionId, const STNBAvcPicDescBase* pros){
	BOOL r = FALSE;
	STTSClientRequesterDeviceOpq* grpOpq = (STTSClientRequesterDeviceOpq*)ref.opaque;
	NBASSERT(grpOpq->cfg.type != ENTSClientRequesterDeviceType_Local || (grpOpq->remoteDesc.streamsGrpsSz == 1 && grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1))
	if(grpOpq->cfg.type == ENTSClientRequesterDeviceType_Local
	   && (
		grpOpq->remoteDesc.streamsGrpsSz == 1
		&& grpOpq->remoteDesc.streamsGrps[0].streamsSz == 1
		&& NBString_strIsEqual(grpOpq->remoteDesc.streamsGrps[0].streams[0].uid, streamId)
		)
	   )
	{
		NBObject_lock(grpOpq);
		{
			//NBASSERT(!NBString_strIsEmpty(grpOpq->remoteDesc.runId) && NBString_strIsEqual(grpOpq->remoteDesc.runId, opq->cfg.runId.str))
			NBASSERT(grpOpq->remoteDesc.streamsGrps != NULL && grpOpq->remoteDesc.streamsGrpsSz == 1)
			BOOL descChanged = FALSE;
			STTSStreamsServiceDesc* desc = &grpOpq->remoteDesc; 
			if(desc->streamsGrps != NULL && desc->streamsGrpsSz > 0){
				NBASSERT(desc->streamsGrpsSz == 1)
				STTSStreamsGrpDesc* grpDesc = &desc->streamsGrps[0];
				if(grpDesc->streams != NULL && grpDesc->streamsSz > 0){
					UI32 i; for(i = 0; i < grpDesc->streamsSz; i++){
						STTSStreamDesc* strDesc = &grpDesc->streams[i];
						if(NBString_strIsEqual(strDesc->uid, streamId)){
							if(strDesc->versions != NULL && strDesc->versionsSz > 0){
								UI32 i; for(i = 0; i < strDesc->versionsSz; i++){
									STTSStreamVerDesc* verDesc = &strDesc->versions[i];
									if(NBString_strIsEqual(verDesc->uid, versionId)){
										const STNBStructMap* propsMap = NBAvcPicDescBase_getSharedStructMap();
										if(!NBStruct_stIsEqualByCrc(propsMap, pros, sizeof(*pros), &verDesc->live.props, sizeof(verDesc->live.props))){
											NBStruct_stRelease(propsMap, &verDesc->live.props, sizeof(verDesc->live.props));
											NBStruct_stClone(propsMap, pros, sizeof(*pros), &verDesc->live.props, sizeof(verDesc->live.props));
											descChanged = TRUE;
										}
										r = TRUE;
										break;
									}
								}
							}
							break;
						}
					}
				}
			}
			if(descChanged){
				NBThreadCond_broadcast(&grpOpq->cond);
				if(descChanged){
					TSClientRequesterDevice_notifyDescChangedLockedOpq_(grpOpq);
				}
			}
		}
		NBObject_unlock(grpOpq);
	}
	return r;
}*/

//lstnrs

void TSClientRequesterDevice_notifyConnStateChangeLockedOpq_(STTSClientRequesterDeviceOpq* opq){
	STTSClientRequesterDeviceRef devRef = TSClientRequesterDevice_fromOpqPtr(opq);
	const ENTSClientRequesterDeviceConnState connState = opq->connState;
	STNBArray arr;
	NBArray_init(&arr, sizeof(STTSClientRequesterDeviceLstnr), NULL);
	//retain (locked)
	{
		SI32 i; for(i = 0; i < opq->lstnrs.arr.use; i++){
			STTSClientRequesterDeviceLstnr* lstnr = NBArray_itmPtrAtIndex(&opq->lstnrs.arr, STTSClientRequesterDeviceLstnr, i);
			if(lstnr->itf.requesterSourceConnStateChanged != NULL){
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
				STTSClientRequesterDeviceLstnr* lstnr = NBArray_itmPtrAtIndex(&arr, STTSClientRequesterDeviceLstnr, i);
				if(lstnr->itf.requesterSourceConnStateChanged != NULL){
					(*lstnr->itf.requesterSourceConnStateChanged)(lstnr->param, devRef, connState);
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

void TSClientRequesterDevice_notifyDescChangedLockedOpq_(STTSClientRequesterDeviceOpq* opq){
	STTSClientRequesterDeviceRef devRef = TSClientRequesterDevice_fromOpqPtr(opq);
	STNBArray arr;
	NBArray_init(&arr, sizeof(STTSClientRequesterDeviceLstnr), NULL);
	//retain (locked)
	{
		SI32 i; for(i = 0; i < opq->lstnrs.arr.use; i++){
			STTSClientRequesterDeviceLstnr* lstnr = NBArray_itmPtrAtIndex(&opq->lstnrs.arr, STTSClientRequesterDeviceLstnr, i);
			if(lstnr->itf.requesterSourceDescChanged != NULL){
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
				STTSClientRequesterDeviceLstnr* lstnr = NBArray_itmPtrAtIndex(&arr, STTSClientRequesterDeviceLstnr, i);
				if(lstnr->itf.requesterSourceDescChanged != NULL){
					(*lstnr->itf.requesterSourceDescChanged)(lstnr->param, devRef);
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

BOOL TSClientRequesterDevice_addLstnr(STTSClientRequesterDeviceRef ref, STTSClientRequesterDeviceLstnr* lstnr){
	BOOL r = FALSE;
	STTSClientRequesterDeviceOpq* opq = (STTSClientRequesterDeviceOpq*)ref.opaque; NBASSERT(TSClientRequesterDevice_isClass(ref));
	if(lstnr != NULL){
		NBObject_lock(opq);
		{
			//Search
			SI32 i; for(i = 0; i < opq->lstnrs.arr.use; i++){
				const STTSClientRequesterDeviceLstnr* lstnr2 = NBArray_itmPtrAtIndex(&opq->lstnrs.arr, STTSClientRequesterDeviceLstnr, i);
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

BOOL TSClientRequesterDevice_removeLstnr(STTSClientRequesterDeviceRef ref, void* param){
	BOOL r = FALSE;
	STTSClientRequesterDeviceOpq* opq = (STTSClientRequesterDeviceOpq*)ref.opaque; NBASSERT(TSClientRequesterDevice_isClass(ref));
	if(param != NULL){
		NBObject_lock(opq);
		{
			BOOL fnd = FALSE;
			//Search
			SI32 i; for(i = 0; i < opq->lstnrs.arr.use; i++){
				const STTSClientRequesterDeviceLstnr* lstnr2 = NBArray_itmPtrAtIndex(&opq->lstnrs.arr, STTSClientRequesterDeviceLstnr, i);
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

BOOL TSClientRequesterDevice_startAsThinstream(STTSClientRequesterDeviceRef ref, const char* server, const UI16 port){
	BOOL r = FALSE;
	STTSClientRequesterDeviceOpq* opq = (STTSClientRequesterDeviceOpq*)ref.opaque; NBASSERT(TSClientRequesterDevice_isClass(ref));
	NBObject_lock(opq);
	if(!opq->syncing.isRunning){
		STNBThread* tin = NBMemory_allocType(STNBThread);
		NBThread_init(tin);
		NBThread_setIsJoinable(tin, FALSE);
		{
			//cfg
			opq->cfg.type = ENTSClientRequesterDeviceType_Native;
			opq->cfg.server.port = port;
			NBString_strFreeAndNewBuffer(&opq->cfg.server.server, server);
			//
			opq->syncing.isRunning = TRUE;
			if(NBThread_start(tin, TSClientRequesterDevice_syncReceiverThread_, opq, NULL)){
				tin = NULL;
				r = TRUE;
			} else {
				opq->syncing.isRunning = FALSE;
			}
		}
		//release (if not consumed)
		if(tin != NULL){
			NBThread_release(tin);
			NBMemory_free(tin);
			tin = NULL;
		}
	}
	NBObject_unlock(opq);
	return r;
}

BOOL TSClientRequesterDevice_startAsRtsp(STTSClientRequesterDeviceRef ref, const char* runId, const char* groupId, const char* groupName, const char* streamId, const char* streamName, const char* server, const UI16 port){
	BOOL r = FALSE;
	STTSClientRequesterDeviceOpq* opq = (STTSClientRequesterDeviceOpq*)ref.opaque; NBASSERT(TSClientRequesterDevice_isClass(ref));
	NBObject_lock(opq);
	if(!opq->syncing.isRunning){
		//cfg
		{
			opq->cfg.type = ENTSClientRequesterDeviceType_Local;
			opq->cfg.server.port = port;
			NBString_strFreeAndNewBuffer(&opq->cfg.server.server, server);
		}
		//remoteDesc
		{
			opq->remoteDesc.iSeq = 1;
			NBString_strFreeAndNewBuffer(&opq->remoteDesc.runId, runId);
			NBASSERT(opq->remoteDesc.streamsGrps == NULL && opq->remoteDesc.streamsGrpsSz == 0)
			{
				STTSStreamsGrpDesc* grpDesc = NBMemory_allocType(STTSStreamsGrpDesc);
				STTSStreamDesc* strmDesc = NBMemory_allocType(STTSStreamDesc);
				NBMemory_setZeroSt(*grpDesc, STTSStreamsGrpDesc);
				NBMemory_setZeroSt(*strmDesc, STTSStreamDesc);
				grpDesc->iSeq	= 1;
				grpDesc->uid	= NBString_strNewBuffer(groupId);
				grpDesc->name	= NBString_strNewBuffer(groupName);
				grpDesc->streams = strmDesc;
				grpDesc->streamsSz = 1;
				//
				{
					strmDesc->iSeq	= 1;
					strmDesc->uid	= NBString_strNewBuffer(streamId);
					strmDesc->name	= NBString_strNewBuffer(streamName);
				}
				//
				opq->remoteDesc.streamsGrps = grpDesc;
				opq->remoteDesc.streamsGrpsSz = 1;
			}
		}
		r = TRUE;
	}
	NBObject_unlock(opq);
	return r;
}

void TSClientRequesterDevice_syncStreamsDesc_(STTSStreamsServiceDesc* obj, const STTSStreamsServiceUpd* upd);

SI64 TSClientRequesterDevice_syncReceiverThread_(STNBThread* t, void* param){
	SI64 r = 0;
	STTSClientRequesterDeviceOpq* opq = (STTSClientRequesterDeviceOpq*)param;
	NBObject_lock(opq);
	//start
	{
		NBASSERT(opq->syncing.isRunning)
		opq->syncing.isRunning = TRUE;
		NBThreadCond_broadcast(&opq->cond);
		PRINTF_INFO("TSClientRequesterDevice, recvr started.\n");
	}
	//cycle
	{
		STNBSocketRef* socket = &opq->syncing.socket;
		{
			NBSocket_createHnd(*socket);
			NBSocket_setNoSIGPIPE(*socket, TRUE);
			NBSocket_setDelayEnabled(*socket, FALSE);
			NBSocket_setCorkEnabled(*socket, FALSE);
		}
		while(!opq->syncing.stopFlag){
			//create socket
			NBASSERT(NBSocket_isSet(*socket))
			//set 'connecting' state
			{
				NBASSERT(opq->connState == ENTSClientRequesterDeviceConnState_Disconnected);
				opq->connState = ENTSClientRequesterDeviceConnState_Connecting;
				NBThreadCond_broadcast(&opq->cond);
				TSClientRequesterDevice_notifyConnStateChangeLockedOpq_(opq);
			}
			NBObject_unlock(opq);
			{
				//connect
				const ENNBSocketResult rr = NBSocket_connect(*socket, opq->cfg.server.server, opq->cfg.server.port); 
				if(rr != ENNBSocketResult_Success){
					//PRINTF_CONSOLE_ERROR("TSClientRequesterDevice, recvr connect failed to '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
					//set 'disconnected' state
					{
						NBObject_lock(opq);
						{
							NBASSERT(opq->connState == ENTSClientRequesterDeviceConnState_Connecting);
							opq->connState = ENTSClientRequesterDeviceConnState_Disconnected;
							NBThreadCond_broadcast(&opq->cond);
							TSClientRequesterDevice_notifyConnStateChangeLockedOpq_(opq);
						}
						NBObject_unlock(opq);
					}
				} else {
					PRINTF_INFO("TSClientRequesterDevice, recvr connected to '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
					STNBString str;
					STNBHttpBuilder bldr;
					NBString_initWithSz(&str, 4096, 4096, 0.10f);
					NBHttpBuilder_init(&bldr);
					//set 'connected' state
					{
						NBObject_lock(opq);
						{
							NBASSERT(opq->connState == ENTSClientRequesterDeviceConnState_Connecting);
							opq->connState = ENTSClientRequesterDeviceConnState_Connected;
							NBThreadCond_broadcast(&opq->cond);
							TSClientRequesterDevice_notifyConnStateChangeLockedOpq_(opq);
						}
						NBObject_unlock(opq);
					}
					//sync-cycle
					{
						NBHttpBuilder_addRequestLine(&bldr, &str, "GET", "/session/start", 1, 1);
						NBHttpBuilder_addHost(&bldr, &str, opq->cfg.server.server, opq->cfg.server.port);
						NBHttpBuilder_addHeaderEnd(&bldr, &str);
						if(NBSocket_send(*socket, str.str, str.length) != str.length){
							PRINTF_CONSOLE_ERROR("TSClientRequesterDevice, recvr send-header failed to '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
						} else {
							//PRINTF_INFO("TSClientRequesterDevice, recvr header sent:\n%s\n", str.str);
                            BYTE buff[4096]; SI32 buffUse = 0, buffCsmd = 0, hdrSz = 0;
							STNBHttpHeader hdr;
							NBHttpHeader_init(&hdr);
							NBHttpHeader_feedStart(&hdr);
							//Receive header
							while(!opq->syncing.stopFlag){
								buffCsmd	= 0;
								buffUse		= NBSocket_recv(*socket, (char*)buff, sizeof(buff));
                                /*if(buffUse > 0){
                                    STNBString str;
                                    NBString_initWithStrBytes(&str, (const char*) buff, buffUse);
                                    PRINTF_INFO("TSClientRequesterDevice, %d bytes rcvd: '%s'.\n", buffUse, str.str);
                                    NBString_release(&str);
                                }*/
                                if(buffUse <= 0){
                                    PRINTF_ERROR("TSClientRequesterDevice, conn-lost reading http-response '%s:%d' after %d bytes.\n", opq->cfg.server.server, opq->cfg.server.port, hdrSz);
                                    break;
                                }
								buffCsmd	= NBHttpHeader_feedBytes(&hdr, buff, buffUse);
                                if(buffCsmd > 0) hdrSz += buffCsmd;
                                /*if(buffCsmd > 0){
                                    STNBString str;
                                    NBString_initWithStrBytes(&str, (const char*) buff, buffCsmd);
                                    PRINTF_INFO("TSClientRequesterDevice, %d bytes csmd: '%s'.\n", buffCsmd, str.str);
                                    NBString_release(&str);
                                }*/
								if(buffCsmd <= 0 || NBHttpHeader_feedIsComplete(&hdr)) break;
							}
                            //
							if(!NBHttpHeader_feedIsComplete(&hdr)){
								PRINTF_CONSOLE_ERROR("TSClientRequesterDevice, recvr response-header failed to '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
							} else if(hdr.statusLine == NULL){
								PRINTF_CONSOLE_ERROR("TSClientRequesterDevice, recvr response without status to '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
							} else if(hdr.statusLine->statusCode < 200 || hdr.statusLine->statusCode >= 299){
								PRINTF_CONSOLE_ERROR("TSClientRequesterDevice, recvr response status %d '%s' to '%s:%d'.\n", hdr.statusLine->statusCode, &hdr.strs.str[hdr.statusLine->reasonPhrase], opq->cfg.server.server, opq->cfg.server.port);
							} else if(NBHttpHeader_isContentLength(&hdr)){
								PRINTF_CONSOLE_ERROR("TSClientRequesterDevice, recvr expected no content-length stream to '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
							} else {
								PRINTF_INFO("TSClientRequesterDevice, recvr response status %d '%s' to '%s:%d'; reading messages sequence.\n", hdr.statusLine->statusCode, &hdr.strs.str[hdr.statusLine->reasonPhrase], opq->cfg.server.server, opq->cfg.server.port);
								BOOL logicErr = FALSE;
								UI32 msgsCount = 0;
								STNBString json;
								STNBJsonParser parser;
								NBString_initWithSz(&json, 4096, 4096, 0.10f);
								NBJsonParser_init(&parser);
								NBJsonParser_feedStart(&parser, NULL, NULL);
								while(!opq->syncing.stopFlag && !logicErr){
									//read
									if(buffUse == buffCsmd){
										buffCsmd	= 0;
										buffUse		= NBSocket_recv(*socket, (char*)buff, sizeof(buff));
										if(buffUse <= 0){
                                            //PRINTF_ERROR("TSClientRequesterDevice, conn-lost reading jsons-payload '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
											logicErr = TRUE;
											break;
										}
									}
									//consume
									NBASSERT(buffCsmd <= buffUse)
									if(buffCsmd < buffUse){
										const UI32 cmsnd = NBJsonParser_feed(&parser, &buff[buffCsmd], buffUse - buffCsmd, NULL, NULL);
										if(cmsnd <= 0){
											PRINTF_CONSOLE_ERROR("TSClientRequesterDevice, recvr json-parsing error to '%s:%d' (%d of %d bytes csmd).\n", opq->cfg.server.server, opq->cfg.server.port, cmsnd, (buffUse - buffCsmd));
                                            //PRINTF_CONSOLE_ERROR("TSClientRequesterDevice, accumulated-json: %s.\n", json.str);
											logicErr = TRUE;
											break;
										} else {
											NBString_concatBytes(&json, (const char*)&buff[buffCsmd], cmsnd);
											buffCsmd += cmsnd;
											if(NBJsonParser_feedIsComplete(&parser)){
												const STNBStructMap* stMap = TSStreamsSessionMsg_getSharedStructMap();
												STTSStreamsSessionMsg msg;
												NBMemory_setZeroSt(msg, STTSStreamsSessionMsg)
												if(!NBStruct_stReadFromJsonStr(json.str, json.length, stMap, &msg, sizeof(msg))){
													PRINTF_CONSOLE_ERROR("TSClientRequesterDevice, recvr json-to-struct error to '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
                                                    //PRINTF_CONSOLE_ERROR("TSClientRequesterDevice, accumulated-json: %s.\n", json.str);
													logicErr = TRUE;
													break;
												} else {
													//PRINTF_INFO("TSClientRequesterDevice, recvr msg #%d:\n%s\n", msgsCount, json.str);
													NBObject_lock(opq);
													//reset remote desc
													if(msgsCount == 0){
														//Flush previous desc
														NBStruct_stRelease(TSStreamsServiceDesc_getSharedStructMap(), &opq->remoteDesc, sizeof(opq->remoteDesc));
														//start new
														opq->remoteDesc.iSeq	= 1;
														opq->remoteDesc.runId	= NBString_strNewBuffer(msg.runId);
													}
													//Consume msg
													if(msg.streamsUpds != NULL){
														//print
#														ifdef NB_CONFIG_INCLUDE_ASSERTS
														/*{
															STNBString str;
															NBString_init(&str);
															{
																STNBStructConcatFormat fmt;
																NBMemory_setZeroSt(fmt, STNBStructConcatFormat);
																fmt.objectsInNewLine = TRUE;
																fmt.tabChar = " "; fmt.tabCharLen = 1;
																NBStruct_stConcatAsJsonWithFormat(&str, TSStreamsServiceUpd_getSharedStructMap(), msg.streamsUpds, sizeof(*msg.streamsUpds), &fmt);
																PRINTF_INFO("TSClientRequesterDevice, session '%s:%d' mesg-upd:\n\n%s\n\n", opq->cfg.server.server, opq->cfg.server.port, str.str);
															}
															NBString_release(&str);
														}*/
#														endif
														{
															TSClientRequesterDevice_syncStreamsDesc_(&opq->remoteDesc, msg.streamsUpds);
														}
														//print
#														ifdef NB_CONFIG_INCLUDE_ASSERTS
														/*{
															STNBString str;
															NBString_init(&str);
															{
																STNBStructConcatFormat fmt;
																NBMemory_setZeroSt(fmt, STNBStructConcatFormat);
																fmt.objectsInNewLine = TRUE;
																fmt.tabChar = " "; fmt.tabCharLen = 1;
																NBStruct_stConcatAsJsonWithFormat(&str, TSStreamsServiceDesc_getSharedStructMap(), &opq->remoteDesc, sizeof(opq->remoteDesc), &fmt);
																PRINTF_INFO("TSClientRequesterDevice, session '%s:%d' desc changed:\n\n%s\n\n", opq->cfg.server.server, opq->cfg.server.port, str.str);
															}
															NBString_release(&str);
														}*/
#														endif
														//Notify
														{
															NBASSERT(opq->connState == ENTSClientRequesterDeviceConnState_Connected);
															NBThreadCond_broadcast(&opq->cond);
															TSClientRequesterDevice_notifyDescChangedLockedOpq_(opq);
														}
													}
													NBObject_unlock(opq);
													msgsCount++;
												}
												NBStruct_stRelease(stMap, &msg, sizeof(msg));
												//restart
												NBJsonParser_feedStart(&parser, NULL, NULL);
												NBString_empty(&json);
											}
										}
									}
								}
                                if(!logicErr && opq->syncing.stopFlag){
                                    PRINTF_ERROR("TSClientRequesterDevice, syncReceiverThread stopped by signal '%s:%d'.\n", opq->cfg.server.server, opq->cfg.server.port);
                                }
								NBJsonParser_release(&parser);
								NBString_release(&json);
							}
							NBHttpHeader_release(&hdr);
						}
					}
					//set 'disconnected' state and reset descState
					{
						NBObject_lock(opq);
						{
							NBASSERT(opq->connState == ENTSClientRequesterDeviceConnState_Connected);
							opq->connState = ENTSClientRequesterDeviceConnState_Disconnected;
							NBThreadCond_broadcast(&opq->cond);
							TSClientRequesterDevice_notifyConnStateChangeLockedOpq_(opq);
						}
						NBObject_unlock(opq);
					}
					NBString_release(&str);
					NBHttpBuilder_release(&bldr);
				}
				//wait to reconnect
				{
					UI64 msAccum = 0;
					while(!opq->syncing.stopFlag && msAccum < 1000){
						NBThread_mSleep(50);
						msAccum += 50;
					}
				}
			}
			NBObject_lock(opq);
			//reset socket
			if(!opq->syncing.stopFlag){
				NBSocket_close(*socket);
				NBSocket_createHnd(*socket);
				NBSocket_setNoSIGPIPE(*socket, TRUE);
				NBSocket_setDelayEnabled(*socket, FALSE);
				NBSocket_setCorkEnabled(*socket, FALSE);
			}
		}
	}
	//end
	{
		NBASSERT(opq->syncing.isRunning)
		opq->syncing.isRunning = FALSE;
		NBThreadCond_broadcast(&opq->cond);
		PRINTF_INFO("TSClientRequesterDevice, recvr ended.\n");
	}
	NBObject_unlock(opq);
	if(t != NULL){
		NBThread_release(t);
		NBMemory_free(t);
		t = NULL;
	}
	return r;
}

void TSClientRequesterDevice_syncStreamsDesc_(STTSStreamsServiceDesc* obj, const STTSStreamsServiceUpd* upd){
	NBASSERT(obj != NULL && upd != NULL)
	NBASSERT(NBString_strIsEqual(obj->runId, upd->runId))
	//streamsGrps
	if(upd->grps != NULL && upd->grpsSz > 0){
		UI32 i; for(i = 0; i < upd->grpsSz; i++){
			const STTSStreamsGrpUpd* grpUpd = &upd->grps[i];
			STTSStreamsGrpDesc* grpDst = NULL;
			//Search group
			if(obj->streamsGrps != NULL && obj->streamsGrpsSz > 0){
				UI32 i; for(i = 0; i < obj->streamsGrpsSz; i++){
					STTSStreamsGrpDesc* grp = &obj->streamsGrps[i];
					if(NBString_strIsEqual(grp->uid, grpUpd->uid)){
						grpDst = grp;
						break;
					}
				}
			}
			//create group
			if(grpDst == NULL){
				STTSStreamsGrpDesc* nArr = NBMemory_allocTypes(STTSStreamsGrpDesc, obj->streamsGrpsSz + 1);
				if(obj->streamsGrps != NULL){
					if(obj->streamsGrpsSz > 0){
						NBMemory_copy(nArr, obj->streamsGrps, sizeof(obj->streamsGrps[0]) * obj->streamsGrpsSz);
					}
					NBMemory_free(obj->streamsGrps);
				}
				obj->streamsGrps = nArr;
				grpDst = &obj->streamsGrps[obj->streamsGrpsSz++];
				NBMemory_setZeroSt(*grpDst, STTSStreamsGrpDesc);
				grpDst->iSeq	= 1;
				grpDst->uid		= NBString_strNewBuffer(grpUpd->uid);
				grpDst->name	= NBString_strNewBuffer(grpUpd->name);
			}
			//streams
			NBASSERT(grpDst != NULL)
			if(grpUpd->streams != NULL && grpUpd->streamsSz > 0){
				UI32 i; for(i = 0; i < grpUpd->streamsSz; i++){
					const STTSStreamUpd* streamUpd = &grpUpd->streams[i];
					if(streamUpd->action == ENTSStreamUpdAction_Removed){
						//Find and remove
						BOOL fnd = FALSE;
						if(grpDst->streams != NULL && grpDst->streamsSz > 0){
							UI32 i; for(i = 0; i < grpDst->streamsSz; i++){
								STTSStreamDesc* stream = &grpDst->streams[i];
								if(NBString_strIsEqual(stream->uid, streamUpd->desc.uid)){
									//Release
									NBStruct_stRelease(TSStreamDesc_getSharedStructMap(), stream, sizeof(*stream));
									//Fill array
									grpDst->streamsSz--;
									for(; i < grpDst->streamsSz; i++){
										grpDst->streams[i] = grpDst->streams[i + 1];
									}
									fnd = TRUE;
									break;
								}
							}
						} NBASSERT(fnd) //should be found (sync-logic error)
					} else {
						NBASSERT(streamUpd->action == ENTSStreamUpdAction_New || streamUpd->action == ENTSStreamUpdAction_Changed)
						STTSStreamDesc* streamDst = NULL;
						if(grpDst->streams != NULL && grpDst->streamsSz > 0){
							UI32 i; for(i = 0; i < grpDst->streamsSz; i++){
								STTSStreamDesc* stream = &grpDst->streams[i];
								if(NBString_strIsEqual(stream->uid, streamUpd->desc.uid)){
									streamDst = stream;
									break;
								}
							}
						}
						if(streamDst == NULL){
							NBASSERT(streamUpd->action == ENTSStreamUpdAction_New) //should be new (sync-logic error)
							STTSStreamDesc* nArr = NBMemory_allocTypes(STTSStreamDesc, grpDst->streamsSz + 1);
							if(grpDst->streams != NULL){
								if(grpDst->streamsSz > 0){
									NBMemory_copy(nArr, grpDst->streams, sizeof(grpDst->streams[0]) * grpDst->streamsSz);
								}
								NBMemory_free(grpDst->streams);
							}
							grpDst->streams = nArr;
							streamDst		= &grpDst->streams[grpDst->streamsSz++];
							NBMemory_setZeroSt(*streamDst, STTSStreamDesc);
							NBStruct_stClone(TSStreamDesc_getSharedStructMap(), &streamUpd->desc, sizeof(streamUpd->desc), streamDst, sizeof(*streamDst));
						} else {
							NBASSERT(streamUpd->action == ENTSStreamUpdAction_Changed) //should be found (sync-logic error)
							NBStruct_stRelease(TSStreamDesc_getSharedStructMap(), streamDst, sizeof(*streamDst));
							NBStruct_stClone(TSStreamDesc_getSharedStructMap(), &streamUpd->desc, sizeof(streamUpd->desc), streamDst, sizeof(*streamDst));
						}
					}
				}
			}
		}
	}
	//sub-services
	if(upd->subServices != NULL && upd->subServicesSz > 0){
		UI32 i; for(i = 0; i < upd->subServicesSz; i++){
			const STTSStreamsServiceUpd* srvUpd = &upd->subServices[i];
			STTSStreamsServiceDesc* srvDst = NULL;
			//Search sub-service
			if(obj->subServices != NULL && obj->subServicesSz > 0){
				UI32 i; for(i = 0; i < obj->subServicesSz; i++){
					STTSStreamsServiceDesc* srv = &obj->subServices[i];
					if(NBString_strIsEqual(srv->runId, srvUpd->runId)){
						srvDst = srv;
						break;
					}
				}
			}
			//create group
			if(srvDst == NULL){
				STTSStreamsServiceDesc* nArr = NBMemory_allocTypes(STTSStreamsServiceDesc, obj->subServicesSz + 1);
				if(obj->subServices != NULL){
					if(obj->subServicesSz > 0){
						NBMemory_copy(nArr, obj->subServices, sizeof(obj->subServices[0]) * obj->subServicesSz);
					}
					NBMemory_free(obj->subServices);
				}
				obj->subServices = nArr;
				srvDst = &obj->subServices[obj->subServicesSz++];
				NBMemory_setZeroSt(*srvDst, STTSStreamsServiceDesc);
				srvDst->iSeq	= 1;
				srvDst->runId	= NBString_strNewBuffer(srvUpd->runId);
			}
			//sync
			{
				TSClientRequesterDevice_syncStreamsDesc_(srvDst, srvUpd);
			}
		}
	}
}

//

/*BOOL TSClientRequesterDevice_isBetterCandidateForStream_(const STTSStreamsServiceDesc* desc, const char* optRootId, const char* runId, const char* streamId, const char* versionId, const char* sourceId, STTSClientRequesterDeviceRef curBestGrp, const UI32 curBestGrpDepth, UI32 *dstBestDepth){
	BOOL r = FALSE;
	//local streams
	if(
	   //(NBString_strIsEmpty(optRootId) || NBString_strIsEqual(optRootId, desc->runId))
	   NBString_strIsEqual(runId, desc->runId)
	   )
	{
		if(desc->streamsGrps != NULL && desc->streamsGrpsSz > 0 ){
			UI32 i; for(i = 0; i < desc->streamsGrpsSz; i++){
				const STTSStreamsGrpDesc* grpDesc = &desc->streamsGrps[i];
				if(grpDesc->streams != NULL && grpDesc->streamsSz > 0){
					UI32 i; for(i = 0; i < grpDesc->streamsSz; i++){
						const STTSStreamDesc* strm = &grpDesc->streams[i];
						if(NBString_strIsEqual(strm->uid, streamId) && strm->versions != NULL && strm->versionsSz > 0){
							UI32 i; for(i = 0; i < strm->versionsSz; i++){
								const STTSStreamVerDesc* ver = &strm->versions[i];
								if(NBString_strIsEqual(ver->uid, versionId)){
									if(ver->live.isOnline && (NBString_strIsEmpty(sourceId) || NBString_strIsEqual(sourceId, "live"))){
										if(!TSClientRequesterDevice_isSet(curBestGrp) || grpDepth < curBestGrpDepth){
											r = TRUE;
										}
									} else if(ver->storage.isOnline && NBString_strIsEqual(sourceId, "storage")){
										if(!TSClientRequesterDevice_isSet(curBestGrp) || grpDepth < curBestGrpDepth){
											r = TRUE;
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
	return r;
}*/

/*BOOL TSClientRequesterDevice_isBetterCandidateForStream(STTSClientRequesterDeviceRef ref, const char* optRootId, const char* runId, const char* streamId, const char* versionId, const char* sourceId, STTSClientRequesterDeviceRef curBestGrp, const UI32 curBestGrpDepth, UI32 *dstBestDepth){
	BOOL r = FALSE;
	STTSClientRequesterDeviceOpq* opq = (STTSClientRequesterDeviceOpq*)ref.opaque; NBASSERT(TSClientRequesterDevice_isClass(ref));
	NBObject_lock(opq);
	{
		UI32 grpDepth = 1;
		TSClientRequesterDevice_isBetterCandidateForStream_(&grpOpq->remoteDesc, optRootId, runId, streamId, versionId, sourceId, curBestGrp, curBestGrpDepth, );
		if(
		   //(NBString_strIsEmpty(optRootId) || NBString_strIsEqual(optRootId, desc->runId))
		   NBString_strIsEqual(runId, desc->runId)
		   )
		{
			
			//subservices
			if(desc->subServices != NULL && desc->subServicesSz > 0){
				//ToDo: implement
			}
		}
	}
	NBObject_unlock(opq);
	return r;
}*/
