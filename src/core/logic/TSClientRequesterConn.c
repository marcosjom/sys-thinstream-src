//
//  TSClientRequesterConn.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSClientRequesterConn.h"
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/2d/NBAvc.h"
#include "nb/crypto/NBBase64.h"

//RtspClient responses

void TSClientRequesterConn_resRtspStateChanged_(void* obj, const char* uri, const ENNBRtspRessState state);
void TSClientRequesterConn_resRtpConsumeReqResult_(void* pObj, const char* uri, const char* method, ENNBRtspClientConnReqStatus status);
void TSClientRequesterConn_resRtpConsumeOptions_(void* obj, const char* uri, const STNBRtspOptions* options);
void TSClientRequesterConn_resRtpConsumeSessionDesc_(void* obj, const char* uri, const STNBSdpDesc* desc);
void TSClientRequesterConn_resRtpConsumeSetup_(void* obj, const char* uri, const STNBRtspSetup* setup, const UI16 ssrcId);
BOOL TSClientRequesterConn_resRtpIsBorderPresent_(void* obj, const char* uri, const STNBRtpHdrBasic* hdrs, const STNBDataPtr* chunks, const UI32 chunksSz);
void TSClientRequesterConn_resRtpConsumePackets_(void* obj, const char* uri, STNBRtpHdrBasic* hdrs, STNBDataPtr* chunks, const UI32 chunksSz, const UI64 curUtcTime, STNBDataPtrReleaser* optPtrsReleaser);
void TSClientRequesterConn_resRtpFlushStats_(void* obj);

//

void TSClientRequesterConn_fragsEmptyUnsafe_(STTSClientRequesterConn* obj, STNBDataPtrReleaser* optPtrsReleaser);

//

void TSClientRequesterConn_init(STTSClientRequesterConn* obj){
	NBMemory_setZeroSt(*obj, STTSClientRequesterConn);
	NBThreadMutex_init(&obj->mutex);
	NBThreadCond_init(&obj->cond);
	//state
	{
		//units-pool
		{
			NBArray_initWithSz(&obj->state.units.pool.buffs, sizeof(STTSStreamUnitBuffRef), NULL, 32, 8, 0.10f);
			NBArray_initWithSz(&obj->state.units.pool.data, sizeof(STTSStreamUnit), NULL, 32, 8, 0.10f);
		}
        //frames
        {
            //cur
            {
                NBArray_initWithSz(&obj->state.units.frames.cur.units, sizeof(STTSStreamUnit), NULL, 64, 8, 0.10f);
            }
            NBArray_initWithSz(&obj->state.units.frames.fromLastIDR, sizeof(STTSStreamUnit), NULL, 64, 8, 0.10f);
        }
		//fragments (cur-unit)
		{
			NBArray_initWithSz(&obj->state.units.frags.arr, sizeof(STNBDataPtr), NULL, 256, 128, 0.10f);
		}
	}
	//stats
	{
		TSClientRequesterConnStatsUpd_init(&obj->stats.upd);
	}
	//buffs
	{
		//
	}
	//rtsp
	{
		//
	}
	//viewers
	{
		NBArray_init(&obj->viewers.cur, sizeof(STTSClientRequesterConnViewer), NULL);
		//notify
		{
			NBArray_init(&obj->viewers.notify.cur, sizeof(STTSClientRequesterConnViewer), NULL);
		}
	}
}

void TSClientRequesterConn_release(STTSClientRequesterConn* obj){
	NBThreadMutex_lock(&obj->mutex);
	NBASSERT(!obj->dbg.inUnsafeAction)
	//Remove from client
	if(obj->rtsp.clt != NULL){
		//ToDo: implement
		//NBRtspClient_removeResource(obj->rtsp.clt, obj->cfg.address->uri);
	}
	//Config
	{
		if(obj->cfg.runId != NULL){
			NBMemory_free(obj->cfg.runId);
			obj->cfg.runId = NULL;
		}
		if(obj->cfg.groupId != NULL){
			NBMemory_free(obj->cfg.groupId);
			obj->cfg.groupId = NULL;
		}
		if(obj->cfg.groupName != NULL){
			NBMemory_free(obj->cfg.groupName);
			obj->cfg.groupName = NULL;
		}
		if(obj->cfg.streamId != NULL){
			NBMemory_free(obj->cfg.streamId);
			obj->cfg.streamId = NULL;
		}
		if(obj->cfg.streamName != NULL){
			NBMemory_free(obj->cfg.streamName);
			obj->cfg.streamName = NULL;
		}
		if(obj->cfg.versionId != NULL){
			NBMemory_free(obj->cfg.versionId);
			obj->cfg.versionId = NULL;
		}
		if(obj->cfg.versionName != NULL){
			NBMemory_free(obj->cfg.versionName);
			obj->cfg.versionName = NULL;
		}
		if(obj->cfg.address != NULL){
			NBStruct_stRelease(TSStreamAddress_getSharedStructMap(), obj->cfg.address, sizeof(*obj->cfg.address));
			NBMemory_free(obj->cfg.address);
			obj->cfg.address = NULL;
		}
		if(obj->cfg.params != NULL){
			NBStruct_stRelease(TSStreamParams_getSharedStructMap(), obj->cfg.params, sizeof(*obj->cfg.params));
			NBMemory_free(obj->cfg.params);
			obj->cfg.params = NULL;
		}
	}
	//stats
	{
		if(obj->stats.provider != NULL){
			TSClientRequesterConnStats_addUpd(obj->stats.provider, &obj->stats.upd);
			obj->stats.provider = NULL;
		}
		TSClientRequesterConnStatsUpd_release(&obj->stats.upd);
	}
	//state
	{
		//units
		{
			//parser
			{
				//NBAvcParser_release(&obj->state.units.parser);
			}
			//pool
			{
				//buffs
				{
					SI32 i; for(i = 0; i < obj->state.units.pool.buffs.use; i++){
						STTSStreamUnitBuffRef buff = NBArray_itmValueAtIndex(&obj->state.units.pool.buffs, STTSStreamUnitBuffRef, i);
						TSStreamUnitBuff_release(&buff);
						TSStreamUnitBuff_null(&buff);
					}
					NBArray_empty(&obj->state.units.pool.buffs);
					NBArray_release(&obj->state.units.pool.buffs);
				}
				//data
				{
					STTSStreamUnit* u = NBArray_dataPtr(&obj->state.units.pool.data, STTSStreamUnit);
					TSStreamUnit_unitsReleaseGrouped(u, obj->state.units.pool.data.use, NULL);
					NBArray_empty(&obj->state.units.pool.data);
					NBArray_release(&obj->state.units.pool.data);
				}
			}
            //frames
            {
                //cur
                {
                    STTSStreamUnit* u = NBArray_dataPtr(&obj->state.units.frames.cur.units, STTSStreamUnit);
                    TSStreamUnit_unitsReleaseGrouped(u, obj->state.units.frames.cur.units.use, NULL);
                    NBArray_empty(&obj->state.units.frames.cur.units);
                    NBArray_release(&obj->state.units.frames.cur.units);
                }
                //fromLastIDR
                {
                    STTSStreamUnit* u = NBArray_dataPtr(&obj->state.units.frames.fromLastIDR, STTSStreamUnit);
                    TSStreamUnit_unitsReleaseGrouped(u, obj->state.units.frames.fromLastIDR.use, NULL);
                    NBArray_empty(&obj->state.units.frames.fromLastIDR);
                    NBArray_release(&obj->state.units.frames.fromLastIDR);
                }
            }
			//fragments (cur-unit)
			{
				{
					TSClientRequesterConn_fragsEmptyUnsafe_(obj, NULL);
					NBASSERT(obj->state.units.frags.arr.use == 0 && obj->state.units.frags.bytesCount == 0)
				}
				NBArray_release(&obj->state.units.frags.arr);
			}
		}
		//rtsp
		{
			//pic desc
			/*{
				NBStruct_stRelease(NBAvcPicDescBase_getSharedStructMap(), &obj->state.rtsp.picDesc.cur, sizeof(obj->state.rtsp.picDesc.cur));
                NBStruct_stRelease(NBAvcPicDescBase_getSharedStructMap(), &obj->state.rtsp.picDesc.tmp, sizeof(obj->state.rtsp.picDesc.tmp));
			}*/
		}
	}
	//viewers
	{
		//cur
		{
			SI32 i; for(i = 0; i < obj->viewers.cur.use; i++){
                STTSClientRequesterConnViewer* v = NBArray_itmPtrAtIndex(&obj->viewers.cur, STTSClientRequesterConnViewer, i);
				if(v->lstnr.streamEnded != NULL){
					(*v->lstnr.streamEnded)(v->lstnr.param);
				}
			}
			NBArray_empty(&obj->viewers.cur);
			NBArray_release(&obj->viewers.cur);
		}
		//notify
		{
			NBArray_empty(&obj->viewers.notify.cur);
			NBArray_release(&obj->viewers.notify.cur);
		}
	}
	NBThreadCond_release(&obj->cond);
	NBThreadMutex_unlock(&obj->mutex);
	NBThreadMutex_release(&obj->mutex);
}

//Config

BOOL TSClientRequesterConn_setStatsProvider(STTSClientRequesterConn* obj, STTSClientRequesterConnStats* statsProvider){
	BOOL r = FALSE;
	NBThreadMutex_lock(&obj->mutex);
	if(!obj->viewers.registered){
		obj->stats.provider = statsProvider;
		r = TRUE;
	}
	NBThreadMutex_unlock(&obj->mutex);
	return r;
}

BOOL TSClientRequesterConn_setRtspClient(STTSClientRequesterConn* obj, STNBRtspClient* clt, const ENNBRtspRessState stoppedState){
	BOOL r = FALSE;
	NBThreadMutex_lock(&obj->mutex);
	if(!obj->viewers.registered){
		obj->rtsp.clt = clt;
		obj->rtsp.stoppedState = stoppedState;
		r = TRUE;
	}
	NBThreadMutex_unlock(&obj->mutex);
	return r;
}

BOOL TSClientRequesterConn_setConfig(STTSClientRequesterConn* obj, const char* runId, const char* groupId, const char* groupName, const char* streamId, const char* streamName, const char* versionId, const char* versionName, const STTSStreamAddress* address, const STTSStreamParams* params){
	BOOL r = FALSE;
	//Validate assumeConnDroppedAfter
	UI32 assumeConnDroppedAfterSecs = 0;
	if(params != NULL){
		if(!NBString_strIsEmpty(params->rtp.assumeConnDroppedAfter)){
			const SI64 secs = NBString_strToSecs(params->rtp.assumeConnDroppedAfter);
			//PRINTF_INFO("TSClientRequesterConn, '%s' parsed to %lld secs\n", params->rtp.assumeConnDroppedAfter, secs);
			if(secs < 0){
				PRINTF_CONSOLE_ERROR("TSClientRequesterConn, '%s' parsed to %lld secs (negative not valid).\n", params->rtp.assumeConnDroppedAfter, secs);
			} else if(secs > (60 * 60 * 12)){
				PRINTF_CONSOLE_ERROR("TSClientRequesterConn, '%s' parsed to %lld secs (max is 12h).\n", params->rtp.assumeConnDroppedAfter, secs);
			} else {
				assumeConnDroppedAfterSecs = (UI32)secs;
			}
		} else {
			assumeConnDroppedAfterSecs = params->rtp.assumeConnDroppedAfterSecs;
		}
	}
	if(assumeConnDroppedAfterSecs == 0){
		PRINTF_CONSOLE_ERROR("TSClientRequesterConn, 'assumeConnDroppedAfterSecs' is required.\n");
	} else {
		STTSClientRequesterConnStatsUpd upd;
		TSClientRequesterConnStatsUpd_init(&upd);
		NBThreadMutex_lock(&obj->mutex);
		if(!obj->viewers.registered){
			//type
			obj->cfg.isRtsp = FALSE;
			//address
			{
				if(obj->cfg.address != NULL){
					NBStruct_stRelease(TSStreamAddress_getSharedStructMap(), obj->cfg.address, sizeof(*obj->cfg.address));
					NBMemory_free(obj->cfg.address);
					obj->cfg.address = NULL;
				}
				if(address != NULL){
					obj->cfg.address = NBMemory_allocType(STTSStreamAddress);
					NBMemory_setZeroSt(*obj->cfg.address, STTSStreamAddress);;
					NBStruct_stClone(TSStreamAddress_getSharedStructMap(), address, sizeof(*address), obj->cfg.address, sizeof(*obj->cfg.address));
					if(obj->cfg.address->uri != NULL){
						const char* uri = obj->cfg.address->uri;
						if(uri[0] == 'R' || uri[0] == 'r'){
							if(uri[1] == 'T' || uri[1] == 't'){
								if(uri[2] == 'S' || uri[2] == 's'){
									if(uri[3] == 'P' || uri[3] == 'p'){
										if(uri[4] == ':'){
											obj->cfg.isRtsp = TRUE;
										}
									}
								}
							}
						}
					}
				}
			}
			//params
			{
				//ids
				{
					NBString_strFreeAndNewBuffer(&obj->cfg.runId, runId);
					NBString_strFreeAndNewBuffer(&obj->cfg.groupId, groupId);
					NBString_strFreeAndNewBuffer(&obj->cfg.groupName, groupName);
					NBString_strFreeAndNewBuffer(&obj->cfg.streamId, streamId);
					NBString_strFreeAndNewBuffer(&obj->cfg.streamName, streamName);
					NBString_strFreeAndNewBuffer(&obj->cfg.versionId, versionId);
					NBString_strFreeAndNewBuffer(&obj->cfg.versionName, versionName);
				}
				if(obj->cfg.params != NULL){
					NBStruct_stRelease(TSStreamParams_getSharedStructMap(), obj->cfg.params, sizeof(*obj->cfg.params));
					NBMemory_free(obj->cfg.params);
					obj->cfg.params = NULL;
				}
				if(params != NULL){
					obj->cfg.params = NBMemory_allocType(STTSStreamParams);
					NBMemory_setZeroSt(*obj->cfg.params, STTSStreamParams);;
					NBStruct_stClone(TSStreamParams_getSharedStructMap(), params, sizeof(*params), obj->cfg.params, sizeof(*obj->cfg.params));
					//Set parsed values
					obj->cfg.params->rtp.assumeConnDroppedAfterSecs = assumeConnDroppedAfterSecs;
					//allocate units buffs
					while(obj->state.units.pool.buffs.use < params->buffers.units.minAlloc){
						STTSStreamUnitBuffRef buff = TSStreamUnitBuff_alloc(NULL);
						NBArray_addValue(&obj->state.units.pool.buffs, buff);
						upd.buffs.newAllocs++;
					}
				}
			}
			//buffs stats
			if(upd.buffs.populated){
				if(upd.buffs.min > obj->state.units.pool.buffs.use){ upd.buffs.min = obj->state.units.pool.buffs.use; }
				if(upd.buffs.max < obj->state.units.pool.buffs.use){ upd.buffs.max = obj->state.units.pool.buffs.use; }
			} else {
				upd.buffs.min = upd.buffs.max = obj->state.units.pool.buffs.use;
			}
			//
			r = TRUE;
		}
		NBThreadMutex_unlock(&obj->mutex);
		//Apply telemetry (unlocked)
		if(obj->stats.provider != NULL){
			TSClientRequesterConnStats_addUpd(obj->stats.provider, &upd);
		}
		TSClientRequesterConnStatsUpd_release(&upd);
	}
	return r;
}

BOOL TSClientRequesterConn_setLstnr(STTSClientRequesterConn* obj, STTSClientRequesterConnLstnr* lstnr){
	BOOL r = TRUE;
	NBThreadMutex_lock(&obj->mutex);
	if(!obj->viewers.registered){
		if(lstnr == NULL){
			NBMemory_setZeroSt(obj->cfg.lstnr, STTSClientRequesterConnLstnr);
		} else {
			obj->cfg.lstnr = *lstnr; 
		}
		r = TRUE;
	}
	NBThreadMutex_unlock(&obj->mutex);
	return r; 
}

BOOL TSClientRequesterConn_start(STTSClientRequesterConn* obj){
	BOOL r = FALSE;
	NBThreadMutex_lock(&obj->mutex);
	if(!obj->viewers.registered){
		//Register
		const STTSStreamAddress* address = obj->cfg.address;
		STNBRtspClientCfgStream cfg;
		STNBRtspClientResLstnr lstnr;
		{
			NBMemory_setZero(cfg);
			cfg.redirMode	= ENNBRtspClientConnRedirMode_None;
			cfg.server		= address->server;
			cfg.port		= address->port;
			cfg.useSSL		= address->useSSL;
			if(address->user != NULL){
				cfg.user	= address->user;
				cfg.userSz	= NBString_strLenBytes(address->user);
			}
			if(address->pass != NULL){
				cfg.pass	= address->pass;
				cfg.passSz	= NBString_strLenBytes(address->pass);
			}
			if(obj->cfg.params != NULL){
				STTSStreamParams* params = obj->cfg.params;
				cfg.client.rtp.port		= params->rtsp.ports.rtp;
				cfg.client.rtcp.port	= params->rtsp.ports.rtcp;
				cfg.client.rtp.assumeConnDroppedAfterSecs = params->rtp.assumeConnDroppedAfterSecs;
			}
		}
		{
			NBMemory_setZero(lstnr);
			lstnr.obj = obj;
			lstnr.resRtspStateChanged		= TSClientRequesterConn_resRtspStateChanged_;
			lstnr.resRtpConsumeReqResult	= TSClientRequesterConn_resRtpConsumeReqResult_;
			lstnr.resRtpConsumeOptions		= TSClientRequesterConn_resRtpConsumeOptions_;
			lstnr.resRtpConsumeSessionDesc	= TSClientRequesterConn_resRtpConsumeSessionDesc_;
			lstnr.resRtpConsumeSetup		= TSClientRequesterConn_resRtpConsumeSetup_;
			lstnr.resRtpIsBorderPresent		= TSClientRequesterConn_resRtpIsBorderPresent_;
			lstnr.resRtpConsumePackets		= TSClientRequesterConn_resRtpConsumePackets_;
			lstnr.resRtpFlushStats			= TSClientRequesterConn_resRtpFlushStats_;
		}
		if(!NBRtspClient_addResource(obj->rtsp.clt, address->uri, &lstnr, &cfg, obj->viewers.cur.use != 0 ? ENNBRtspRessState_Playing : obj->rtsp.stoppedState)){
			PRINTF_WARNING("TSClientRequesterConn, first NBRtspClient_addResource failed for: '%s'.\n", address->uri);
		} else {
            STNBRtspClientCfgStream streamCfg;
            NBMemory_setZeroSt(streamCfg, STNBRtspClientCfgStream);
            if(!NBRtspClient_getResourceCfg(obj->rtsp.clt, address->uri, &streamCfg)){
                PRINTF_WARNING("TSClientRequesterConn, first NBRtspClient_getResourceCfg failed for: '%s'.\n", address->uri);
            } else {
                if(streamCfg.client.rtp.assumeConnDroppedAfterSecs <= 0){
                    PRINTF_WARNING("TSClientRequesterConn, rtp.assumeConnDroppedAfterSecs is '%d' for '%s'.\n", streamCfg.client.rtp.assumeConnDroppedAfterSecs, address->uri);
                }
                //PRINTF_INFO("TSClientRequesterConn, NBRtspClient_addResource: '%s'.\n", address->uri);
            }
			obj->viewers.registered = TRUE;
			r = TRUE;
		}
	}
	NBThreadMutex_unlock(&obj->mutex);
	return r; 
}

BOOL TSClientRequesterConn_isAddress(STTSClientRequesterConn* obj, const STTSStreamAddress* address){
	BOOL r = FALSE;
	NBThreadMutex_lock(&obj->mutex);
	{
		r = TSStreamAddress_isEqual(obj->cfg.address, address);
	}
	NBThreadMutex_unlock(&obj->mutex);
	return r; 
}

//Viewers

UI32 TSClientRequesterConn_viewersCount(STTSClientRequesterConn* obj){
	return obj->viewers.cur.use; 
}

BOOL TSClientRequesterConn_addViewer(STTSClientRequesterConn* obj, const STTSStreamLstnr* viewer, UI32* dstCount){
	BOOL r = FALSE;
	if(viewer != NULL){
		NBThreadMutex_lock(&obj->mutex);
		{
			//verify added only once
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			{
				SI32 i; for(i = 0; i < obj->viewers.cur.use; i++){
                    STTSClientRequesterConnViewer* v = NBArray_itmPtrAtIndex(&obj->viewers.cur, STTSClientRequesterConnViewer, i);
					NBASSERT(v->lstnr.param != viewer->param) //Should not be repeated
				}
			}
#			endif
			//add
			{
                STTSClientRequesterConnViewer vwr;
                NBMemory_setZeroSt(vwr, STTSClientRequesterConnViewer);
                vwr.lstnr = *viewer;
				NBArray_addValue(&obj->viewers.cur, vwr);
				obj->viewers.iSeq++; r = TRUE;
			}
			//First viewer (add resource to RtspClient)
			if(obj->viewers.registered && obj->rtsp.clt != NULL && obj->viewers.cur.use == 1){
				//set state
				if(!NBRtspClient_setResourceState(obj->rtsp.clt, obj->cfg.address->uri, ENNBRtspRessState_Playing)){
					PRINTF_WARNING("TSClientRequesterConn, NBRtspClient_setResourceState reactivation failed for: '%s'.\n", obj->cfg.address->uri);
				} else {
					PRINTF_INFO("TSClientRequesterConn, NBRtspClient_setResourceState(playing): '%s'.\n", obj->cfg.address->uri);
				}
			}
			if(dstCount != NULL){
				*dstCount = obj->viewers.cur.use;
			}
		}
		NBThreadMutex_unlock(&obj->mutex);
	}
	return r;
}

BOOL TSClientRequesterConn_removeViewer(STTSClientRequesterConn* obj, void* param, UI32* dstCount){
	BOOL r = FALSE;
	if(param != NULL){
		BOOL notify = FALSE;
		STTSStreamLstnr notifyLstnr;
		NBThreadMutex_lock(&obj->mutex);
		NBASSERT(obj->viewers.registered)
		if(obj->viewers.registered){
			IF_NBASSERT(BOOL fnd = FALSE;)
			SI32 i; for(i = 0; i < obj->viewers.cur.use; i++){
                STTSClientRequesterConnViewer* v = NBArray_itmPtrAtIndex(&obj->viewers.cur, STTSClientRequesterConnViewer, i);
				if(v->lstnr.param == param){
					notify = TRUE;
					notifyLstnr = v->lstnr;
					//remove
					{
						NBArray_removeItemAtIndex(&obj->viewers.cur, i);
						obj->viewers.iSeq++; r = TRUE;
					}
					IF_NBASSERT(fnd = TRUE;)
					break;
				}
			} NBASSERT(fnd) //should be fund
			//
			if(dstCount != NULL){
				*dstCount = obj->viewers.cur.use;
			}
			//Desactivate resource
			if(obj->viewers.cur.use == 0){
				//set state
				if(!NBRtspClient_setResourceState(obj->rtsp.clt, obj->cfg.address->uri, obj->rtsp.stoppedState)){
					PRINTF_WARNING("TSClientRequesterConn, NBRtspClient_setResourceState deactivation failed for: '%s'.\n", obj->cfg.address->uri);
				} else {
					PRINTF_INFO("TSClientRequesterConn, NBRtspClient_setResourceState(stopped): '%s'.\n", obj->cfg.address->uri);
				}
			}
		}
		NBThreadMutex_unlock(&obj->mutex);
		//notify (unlocked)
		if(notify){
			if(notifyLstnr.streamEnded != NULL){
				(*notifyLstnr.streamEnded)(notifyLstnr.param);
			}
		}
	}
	return r;
}

//Actions

//ToDo: remove (this is a non-compliant validation).
/*BOOL TSClientRequesterConn_mustIgnoreNALU_(STTSClientRequesterConn* obj, const UI8 nalType){
	BOOL ignore = FALSE;
	if(nalType == 7){
		NBMemory_setZero(obj->state.units.state);
		obj->state.units.state.nal7Found = TRUE;
	} else if(nalType == 5){
		if(!obj->state.units.state.nal7Found){
			ignore = TRUE;
		} else {
			obj->state.units.state.nal5Found = TRUE;
		}
	} else if(nalType == 1){
		if(!obj->state.units.state.nal5Found){
			ignore = TRUE;
		}
	}
	return ignore;
}*/

//Callbacks

void TSClientRequesterConn_resRtspStateChanged_(void* pObj, const char* uri, const ENNBRtspRessState state){
	if(pObj != NULL){
		STTSClientRequesterConn* obj = (STTSClientRequesterConn*)pObj;
		BOOL notify = FALSE; STTSClientRequesterConnLstnr lstnr;
		NBThreadMutex_lock(&obj->mutex);
		{
			NBASSERT(!obj->dbg.inUnsafeAction)
			IF_NBASSERT(obj->dbg.inUnsafeAction = TRUE;)
			if(obj->state.rtsp.state != state){
				//set
				obj->state.rtsp.state = state;
				//prepare notification
				lstnr	= obj->cfg.lstnr;
				notify	= TRUE; 
			}
            //Reset times
            {
                const UI64 curUtcTime = NBDatetime_getCurUTCTimestamp();
                obj->state.rtp.packets.utcTime.start = obj->state.rtp.packets.utcTime.last = curUtcTime;
                obj->state.units.utcTime.start = obj->state.units.utcTime.last = curUtcTime;
            }
			NBASSERT(obj->dbg.inUnsafeAction)
			IF_NBASSERT(obj->dbg.inUnsafeAction = FALSE;)
		}
		NBThreadMutex_unlock(&obj->mutex);
		//notify (unlocked)
		if(notify && lstnr.itf.rtspStreamStateChanged != NULL){
			(*lstnr.itf.rtspStreamStateChanged)(lstnr.param, obj, obj->cfg.streamId, obj->cfg.versionId, state);
		}
	}
}

//

void TSClientRequesterConn_fragsEmptyUnsafe_(STTSClientRequesterConn* obj, STNBDataPtrReleaser* optPtrsReleaser){
	if(obj->state.units.frags.arr.use > 0){
		STNBDataPtr* chunks = NBArray_dataPtr(&obj->state.units.frags.arr, STNBDataPtr);
		if(optPtrsReleaser != NULL && optPtrsReleaser->itf.ptrsReleaseGrouped != NULL){
			(*optPtrsReleaser->itf.ptrsReleaseGrouped)(chunks, obj->state.units.frags.arr.use, optPtrsReleaser->usrData);
		} else {
			NBDataPtr_ptrsReleaseGrouped(chunks, obj->state.units.frags.arr.use);
		}
		NBArray_empty(&obj->state.units.frags.arr);
	}
	obj->state.units.frags.bytesCount = 0;
}

//

void TSClientRequesterConn_resRtpConsumeReqResult_(void* pObj, const char* uri, const char* method, ENNBRtspClientConnReqStatus status){
	if(pObj != NULL){
		STTSClientRequesterConn* obj = (STTSClientRequesterConn*)pObj;
		NBThreadMutex_lock(&obj->mutex);
		{
			NBASSERT(!obj->dbg.inUnsafeAction)
			IF_NBASSERT(obj->dbg.inUnsafeAction = TRUE;)
			//
			NBASSERT(obj->dbg.inUnsafeAction)
			IF_NBASSERT(obj->dbg.inUnsafeAction = FALSE;)
            //Reset times
            {
                const UI64 curUtcTime = NBDatetime_getCurUTCTimestamp();
                obj->state.rtp.packets.utcTime.start = obj->state.rtp.packets.utcTime.last = curUtcTime;
                obj->state.units.utcTime.start = obj->state.units.utcTime.last = curUtcTime;
            }
		}
		NBThreadMutex_unlock(&obj->mutex);
	}
}



void TSClientRequesterConn_resRtpConsumeOptions_(void* pObj, const char* uri, const STNBRtspOptions* options){
	if(pObj != NULL){
		STTSClientRequesterConn* obj = (STTSClientRequesterConn*)pObj;
		NBThreadMutex_lock(&obj->mutex);
		{
			NBASSERT(!obj->dbg.inUnsafeAction)
			IF_NBASSERT(obj->dbg.inUnsafeAction = TRUE;)
			//
			NBASSERT(obj->dbg.inUnsafeAction)
			IF_NBASSERT(obj->dbg.inUnsafeAction = FALSE;)
            //Reset times
            {
                const UI64 curUtcTime = NBDatetime_getCurUTCTimestamp();
                obj->state.rtp.packets.utcTime.start = obj->state.rtp.packets.utcTime.last = curUtcTime;
                obj->state.units.utcTime.start = obj->state.units.utcTime.last = curUtcTime;
            }
		}
		NBThreadMutex_unlock(&obj->mutex);
	}
}

/*
https://datatracker.ietf.org/doc/html/rfc6184
RTP Payload Format for H.264 Video
8.2.1.  Mapping of Payload Type Parameters to SDP
- The media name in the "m=" line of SDP MUST be video
- The encoding name in the "a=rtpmap" line of SDP MUST be H264 (the
 media subtype).
- The clock rate in the "a=rtpmap" line MUST be 90000.
... parameters are expressed as a media type string, in the form of a semicolon-separated list of parameter=value pairs.
*/

typedef struct STNBStreamRequesterVideoParam_ {
	STNBRangeI name;
	STNBRangeI value;
} STNBStreamRequesterVideoParam;

BOOL TSClientRequesterConn_getNextVideoParam_(const char* pay, const UI32 payLen, const UI32 iStart, STNBStreamRequesterVideoParam* dst){
	BOOL r = FALSE;
	STNBStreamRequesterVideoParam rr;
	NBMemory_setZeroSt(rr, STNBStreamRequesterVideoParam);
	//ignore initial spaces
	UI32 iStartName = iStart;
	while(iStartName < payLen && pay[iStartName] == ' '){
		iStartName++;
	}
	//find name end
	if(iStartName < payLen){
		SI32 posEqq = NBString_strIndexOfBytes(pay, payLen, "=", iStartName);
		SI32 posEndd = NBString_strIndexOfBytes(pay, payLen, ";", iStartName);
		SI32 posMin, posMax;
		if(posEqq < 0) posEqq = payLen;
		if(posEndd < 0) posEndd = payLen;
		posMin = (posEqq < posEndd ? posEqq : posEndd);
		posMax = (posEqq > posEndd ? posEqq : posEndd);
		if(posMin >= 0){
			rr.name.start	= iStartName;
			rr.name.size	= posMin - iStartName;
			if(posMin < posMax){
				rr.value.start	= posMin + 1;
				rr.value.size	= posMax - posMin - 1;
			} else {
				rr.value.start	= payLen;
				rr.value.size	= 0;
			}
			r = TRUE;
		}
	}
	if(dst != NULL){
		*dst = rr;
	}
	return r;
}

void TSClientRequesterConn_resRtpStreamDetectPicDescChangesLocked_(STTSClientRequesterConn* obj){
    /*const UI32 picDescSeqPrev = obj->state.rtsp.picDesc.iSeq;
    //PRINTF_INFO("TSClientRequesterConn, '%s/%s' NBAvcParser_feedNALU(%d) OK, %d chunks.\n", obj->cfg.streamId, obj->cfg.versionId, type, chunksSz);
    if(NBAvcParser_getPicDescBase(&obj->state.units.parser, obj->state.rtsp.picDesc.iSeq, &obj->state.rtsp.picDesc.tmp, &obj->state.rtsp.picDesc.iSeq)){
        //print
        SI32 propsChagesCount = 0;
        if(picDescSeqPrev == 0){
            PRINTF_CONSOLE("TSClientRequesterConn, '%s/%s' pic-props inited: to v%u = %u x %u luma; %ufps %s;.\n", obj->cfg.streamId, obj->cfg.versionId, obj->state.rtsp.picDesc.iSeq, obj->state.rtsp.picDesc.cur.w, obj->state.rtsp.picDesc.cur.h, obj->state.rtsp.picDesc.cur.fpsMax, obj->state.rtsp.picDesc.cur.isFixedFps ? "fixed" : "non-fixed");
        } else {
            PRINTF_CONSOLE("TSClientRequesterConn, '%s/%s' pic-props changed:.\n", obj->cfg.streamId, obj->cfg.versionId, obj->state.rtsp.picDesc.iSeq, obj->state.rtsp.picDesc.cur.w, obj->state.rtsp.picDesc.cur.h, obj->state.rtsp.picDesc.cur.fpsMax, obj->state.rtsp.picDesc.cur.isFixedFps ? "fixed" : "non-fixed");
        }
        //width
        if(picDescSeqPrev == 0 || obj->state.rtsp.picDesc.tmp.w != obj->state.rtsp.picDesc.cur.w){
            PRINTF_CONSOLE("TSClientRequesterConn,                  width: %u.\n", obj->state.rtsp.picDesc.tmp.w);
            propsChagesCount++;
        }
        //height
        if(picDescSeqPrev == 0 || obj->state.rtsp.picDesc.tmp.h != obj->state.rtsp.picDesc.cur.h){
            PRINTF_CONSOLE("TSClientRequesterConn,                 height: %u luma.\n", obj->state.rtsp.picDesc.tmp.h);
            propsChagesCount++;
        }
        //fps + fpsIsFixed
        if(picDescSeqPrev == 0 || obj->state.rtsp.picDesc.tmp.fpsMax != obj->state.rtsp.picDesc.cur.fpsMax || obj->state.rtsp.picDesc.tmp.isFixedFps != obj->state.rtsp.picDesc.cur.isFixedFps){
            PRINTF_CONSOLE("TSClientRequesterConn,                    fps: %ufps %s.\n", obj->state.rtsp.picDesc.tmp.fpsMax, obj->state.rtsp.picDesc.tmp.isFixedFps ? "fixed" : "non-fixed");
            propsChagesCount++;
        }
        //avcProfile
        if(picDescSeqPrev == 0 || obj->state.rtsp.picDesc.tmp.avcProfile != obj->state.rtsp.picDesc.cur.avcProfile){
            PRINTF_CONSOLE("TSClientRequesterConn,             avcProfile: %u.\n", obj->state.rtsp.picDesc.tmp.avcProfile);
            propsChagesCount++;
        }
        //profConstraints
        if(picDescSeqPrev == 0 || obj->state.rtsp.picDesc.tmp.profConstraints != obj->state.rtsp.picDesc.cur.profConstraints){
            PRINTF_CONSOLE("TSClientRequesterConn,         profContraints: %u.\n", obj->state.rtsp.picDesc.tmp.profConstraints);
            propsChagesCount++;
        }
        //profLevel
        if(picDescSeqPrev == 0 || obj->state.rtsp.picDesc.tmp.profLevel != obj->state.rtsp.picDesc.cur.profLevel){
            PRINTF_CONSOLE("TSClientRequesterConn,              profLevel: %u.\n", obj->state.rtsp.picDesc.tmp.profLevel);
            propsChagesCount++;
        }
        //chromaFmt
        if(picDescSeqPrev == 0 || obj->state.rtsp.picDesc.tmp.chromaFmt != obj->state.rtsp.picDesc.cur.chromaFmt){
            PRINTF_CONSOLE("TSClientRequesterConn,              chromaFmt: %u.\n", obj->state.rtsp.picDesc.tmp.chromaFmt);
            propsChagesCount++;
        }
        //bitDepthLuma
        if(picDescSeqPrev == 0 || obj->state.rtsp.picDesc.tmp.bitDepthLuma != obj->state.rtsp.picDesc.cur.bitDepthLuma){
            PRINTF_CONSOLE("TSClientRequesterConn,           bitDepthLuma: %u.\n", obj->state.rtsp.picDesc.tmp.bitDepthLuma);
            propsChagesCount++;
        }
        //bitDepthChroma
        if(picDescSeqPrev == 0 || obj->state.rtsp.picDesc.tmp.bitDepthChroma != obj->state.rtsp.picDesc.cur.bitDepthChroma){
            PRINTF_CONSOLE("TSClientRequesterConn,         bitDepthChroma: %u.\n", obj->state.rtsp.picDesc.tmp.bitDepthChroma);
            propsChagesCount++;
        }
        //nothing printed
        if(propsChagesCount == 0){
            PRINTF_CONSOLE("TSClientRequesterConn,         unknown-changes-by-current-source-code (internal error).\n");
        }
        //apply
        NBStruct_stRelease(NBAvcPicDescBase_getSharedStructMap(), &obj->state.rtsp.picDesc.cur, sizeof(obj->state.rtsp.picDesc.cur));
        NBStruct_stClone(NBAvcPicDescBase_getSharedStructMap(), &obj->state.rtsp.picDesc.tmp, sizeof(obj->state.rtsp.picDesc.tmp), &obj->state.rtsp.picDesc.cur, sizeof(obj->state.rtsp.picDesc.cur));
        obj->state.rtsp.picDesc.changed = TRUE;
    }*/
}

/*BOOL TSClientRequesterConn_resRtpStreamParseUnitForPicDescLocked_(STTSClientRequesterConn* obj, const UI8 refIdc, const UI8 type, const void* prefix, const UI32 prefixSz, const void* pay, const UI32 paySz){
	BOOL r = FALSE;
	if(pay != NULL && paySz > 0){
		const BOOL picDescBaseOnly = TRUE; //quick-parse only necesary data for 'NBAvcParser_getPicDescBase()'
		//PRINTF_INFO("TSClientRequesterConn, '%s/%s' parsing picDesc from bytes-payload refIdc(%d) type(%d).\n", obj->cfg.streamId, obj->cfg.versionId, refIdc, type);
		if(!NBAvcParser_feedStart(&obj->state.units.parser, refIdc, type)){
			//PRINTF_CONSOLE_ERROR("TSClientRequesterConn, '%s/%s' NBAvcParser_feedStart failed for refIdc(%d) type(%d).\n", obj->cfg.streamId, obj->cfg.versionId, refIdc, type);
		} else if(!NBAvcParser_feedNALU(&obj->state.units.parser, prefix, prefixSz, pay, paySz, picDescBaseOnly)){
			PRINTF_CONSOLE_ERROR("TSClientRequesterConn, '%s/%s' NBAvcParser_feedNALU(%d) failed for refIdc(%d).\n", obj->cfg.streamId, obj->cfg.versionId, type, refIdc);
		} else if(!NBAvcParser_feedEnd(&obj->state.units.parser)){
			PRINTF_CONSOLE_ERROR("TSClientRequesterConn, '%s/%s' NBAvcParser_feedEnd(%d) failed for refIdc(%d).\n", obj->cfg.streamId, obj->cfg.versionId, type, refIdc);
		} else {
			//PRINTF_INFO("TSClientRequesterConn, '%s/%s' NBAvcParser_feedNALU(%d) OK.\n", obj->cfg.streamId, obj->cfg.versionId, type);
            //Detect picDesc changes
            TSClientRequesterConn_resRtpStreamDetectPicDescChangesLocked_(obj);
			r = TRUE;
		}
	}
	return r;
}*/


void TSClientRequesterConn_resRtpConsumeSessionDesc_(void* pObj, const char* uri, const STNBSdpDesc* desc){
	if(pObj != NULL){
		STTSClientRequesterConn* obj = (STTSClientRequesterConn*)pObj;
		/*
		Example:
		v=0
		o=- 1001 1 IN IP4 192.168.0.222
		s=VCP IPC Realtime stream
		m=video 0 RTP/AVP 105
		c=IN IP4 192.168.0.222
		a=control:rtsp://192.168.0.222/media/video2/video
		a=rtpmap:105 H264/90000
		a=fmtp:105 profile-level-id=4d401f; packetization-mode=1; sprop-parameter-sets=Z01AH5WgLASabIAAAAMAgAAAGUI=,aO48gA==
		a=recvonly
		m=application 0 RTP/AVP 107
		c=IN IP4 192.168.0.222
		a=control:rtsp://192.168.0.222/media/video2/metadata
		a=rtpmap:107 vnd.onvif.metadata/90000
		a=fmtp:107 DecoderTag=h3c-v3 RTCP=0
		a=recvonly
		*/
		/*
		 Example-2:
		 v=0
		 o=- 1628966255381873 1628966255381873 IN IP4 192.168.0.221
		 s=Media Presentation
		 e=NONE
		 b=AS:5050
		 t=0 0
		 a=control:rtsp://192.168.0.221/Streaming/channels/102/trackID=1/
		 m=video 0 RTP/AVP 96
		 c=IN IP4 0.0.0.0
		 b=AS:5000
		 a=recvonly
		 a=x-dimensions:640,480
		 a=control:rtsp://192.168.0.221/Streaming/channels/102/trackID=1/trackID=1
		 a=rtpmap:96 H264/90000
		 a=fmtp:96 profile-level-id=420029; packetization-mode=1; sprop-parameter-sets=Z01AHo2NQFAe//+APwAvtwEBAUAAAPoAADDUOhgB6cAAJiWrvLjQwA9OAAExLV3lwoA=,aO44gA==
		 a=Media_header:MEDIAINFO=494D4B48010300000400000100000000000000000000000000000000000000000000000000000000;
		 a=appversion:1.0
		*/
		/*
		https://datatracker.ietf.org/doc/html/rfc6184
		RTP Payload Format for H.264 Video
		8.2.1.  Mapping of Payload Type Parameters to SDP
		- The media name in the "m=" line of SDP MUST be video
		- The encoding name in the "a=rtpmap" line of SDP MUST be H264 (the
		 media subtype).
		- The clock rate in the "a=rtpmap" line MUST be 90000.
		... parameters are expressed as a media type string, in the form of a semicolon-separated list of parameter=value pairs.
		*/
		/*
		 https://datatracker.ietf.org/doc/html/rfc6184
		 sprop-parameter-sets:
		 The value of the parameter is a comma-separated (',') list of base64 [7] representations of parameter set NAL units.
		 */
		//BOOL notify = FALSE;
        //STTSClientRequesterConnLstnr lstnr;
		NBThreadMutex_lock(&obj->mutex);
		NBASSERT(obj->cfg.address != NULL)
		NBASSERT(NBString_strIsEqual(obj->cfg.address->uri, uri))
		NBASSERT(!obj->dbg.inUnsafeAction)
		IF_NBASSERT(obj->dbg.inUnsafeAction = TRUE;)
		if(desc != NULL){
			if(desc->sessions != NULL && desc->sessionsSz > 0){
				UI32 i; for(i = 0; i < desc->sessionsSz; i++){
					const STNBSdpSessionDesc* s = &desc->sessions[i];
					if(s->mediaDescs != NULL && s->mediaDescsSz > 0){
						UI32 i; for(i = 0; i < s->mediaDescsSz; i++){
							STNBSdpMediaDesc* m = &s->mediaDescs[i];
							const char* nameAddr = &desc->str[m->nameAddr.iStart];
							if(NBString_strStartsWith(nameAddr, "video")){
								STNBString tmp, tmp2;
								NBString_init(&tmp);
								NBString_init(&tmp2);
								{
									NBString_setBytes(&tmp, nameAddr, m->nameAddr.len);
									//PRINTF_INFO("TSClientRequesterConn, media: '%s'.\n", tmp.str);
								}
								if(m->attribs != NULL && m->attribsSz > 0){
									BOOL isH264And9000 = FALSE;
									UI32 i; for(i = 0; i < m->attribsSz; i++){
										STNBSdpLine* a = &m->attribs[i];
										const char* ln = &desc->str[a->iStart];
										NBString_setBytes(&tmp, ln, a->len);
										//PRINTF_INFO("TSClientRequesterConn, attrib: '%s'.\n", tmp.str);
										if(NBString_strStartsWith(ln, "rtpmap:")){
											const UI32 prefixLen = NBString_strLenBytes("rtpmap:");
											const char* pay		= &ln[prefixLen];
											const UI32 payLen	= a->len - prefixLen;
											STNBSdpAttribRtpMap rtpMap;
											if(!NBSdpDesc_parseAttribRtpMap(pay, payLen, &rtpMap)){
												PRINTF_CONSOLE_ERROR("TSClientRequesterConn, 'rtpmap:' attib could not be parsed.\n");
											} else if(rtpMap.clockRate == 90000 && NBString_strIsEqualStrBytes("H264", &pay[rtpMap.encName.start], rtpMap.encName.size)){
												//PRINTF_INFO("TSClientRequesterConn, video is H264/9000 (ok).\n");
												isH264And9000 = TRUE;
											}
										} else if(NBString_strStartsWith(ln, "fmtp:")){
											if(!isH264And9000){
												PRINTF_WARNING("TSClientRequesterConn, ignoring 'fmtp:' attrib (only 'video/H264/9000' are processed).\n");
											} else {
												const UI32 prefixLen = NBString_strLenBytes("fmtp:");
												const char* pay		= &ln[prefixLen];
												const UI32 payLen	= a->len - prefixLen;
												STNBSdpAttribFmtp fmtp;
												if(!NBSdpDesc_parseAttribFmtp(pay, payLen, &fmtp)){
													PRINTF_CONSOLE_ERROR("TSClientRequesterConn, 'fmtp:' attib could not be parsed.\n");
												} else {
													const char* params = &pay[fmtp.params.start];
													UI32 iNextPos = 0;
													STNBStreamRequesterVideoParam param;
													NBString_setBytes(&tmp, params, fmtp.params.size);
													//PRINTF_INFO("TSClientRequesterConn, fmtp-params: '%s'.\n", tmp.str);
													do {
														if(!TSClientRequesterConn_getNextVideoParam_(params, fmtp.params.size, iNextPos, &param)){
															PRINTF_CONSOLE_ERROR("TSClientRequesterConn, TSClientRequesterConn_getNextVideoParam_ failed.\n");
															break;
														} else {
															NBString_setBytes(&tmp, &params[param.name.start], param.name.size);
															NBString_setBytes(&tmp2, &params[param.value.start], param.value.size);
															//PRINTF_INFO("TSClientRequesterConn, fmtp-param: '%s' = '%s'.\n", tmp.str, tmp2.str);
															//param
															/*if(NBString_strIsEqualStrBytes("sprop-parameter-sets", &params[param.name.start], param.name.size)){
																//sprop-parameter-sets
																const char* values = &params[param.value.start];
																const UI32 valuesLen = param.value.size;
																UI32 iPosNext = 0;
																STNBString naluBin;
																NBString_initWithSz(&naluBin, 4096, 4096, 0.1f);
																do {
																	SI32 sep = NBString_strIndexOfBytes(values, valuesLen, ",", iPosNext);
																	if(sep < 0) sep = valuesLen;
																	if(iPosNext < sep){
																		NBString_setBytes(&tmp, &values[iPosNext], sep - iPosNext);
																		//PRINTF_INFO("TSClientRequesterConn, sprop-parameter-sets NALU: '%s'.\n", tmp.str, tmp.str);
																		NBString_empty(&naluBin);
																		if(!NBBase64_decodeBytes(&naluBin, &values[iPosNext], sep - iPosNext)){
																			PRINTF_CONSOLE_ERROR("TSClientRequesterConn, sprop-parameter-sets NALU NBBase64_decodeBytes failed: '%s'.\n", tmp.str, tmp.str);
																		} else {
																			const BYTE* pckt = (const BYTE*)naluBin.str;
																			const STNBAvcNaluHdr desc = NBAvc_getNaluHdr(pckt[0]);
																			TSClientRequesterConn_resRtpStreamParseUnitForPicDescLocked_(obj, desc.refIdc, desc.type, NULL, 0, naluBin.str, naluBin.length);
																			//PRINTF_INFO("TSClientRequesterConn, NALU forbiddenZeroBit(%d) refIdc(%d) type(%d).\n", desc.forbiddenZeroBit, desc.refIdc, desc.type);
																		}
																	}
																	//
																	iPosNext = sep + 1;
																} while(iPosNext < valuesLen);
																NBString_release(&naluBin);
															}*/
															iNextPos = param.value.start + param.value.size + 1;
														}
													} while(iNextPos < fmtp.params.size);
												}
											}
										}
									}
								}
								NBString_release(&tmp);
								NBString_release(&tmp2);
							}
						}
					}  
				}
			} 
		}
		//notify (will be unlocked)
		/*if(obj->state.rtsp.picDesc.changed){
			obj->state.rtsp.picDesc.changed = FALSE;
			//prepare notification
			lstnr	= obj->cfg.lstnr;
			notify	= TRUE;
		}*/
        //Reset times
        {
            const UI64 curUtcTime = NBDatetime_getCurUTCTimestamp();
            obj->state.rtp.packets.utcTime.start = obj->state.rtp.packets.utcTime.last = curUtcTime;
            obj->state.units.utcTime.start = obj->state.units.utcTime.last = curUtcTime;
        }
		NBASSERT(obj->dbg.inUnsafeAction)
		IF_NBASSERT(obj->dbg.inUnsafeAction = FALSE;)
		NBThreadMutex_unlock(&obj->mutex);
		//notify (unlocked)
		/*if(notify && lstnr.itf.rtspStreamVideoPropsChanged != NULL){
			(*lstnr.itf.rtspStreamVideoPropsChanged)(lstnr.param, obj, obj->cfg.streamId, obj->cfg.versionId, &obj->state.rtsp.picDesc.cur);
		}*/
	}
}

void TSClientRequesterConn_resRtpConsumeSetup_(void* pObj, const char* uri, const STNBRtspSetup* setup, const UI16 ssrcId){
	if(pObj != NULL){
		STTSClientRequesterConn* obj = (STTSClientRequesterConn*)pObj;
		NBThreadMutex_lock(&obj->mutex);
		{
			NBASSERT(!obj->dbg.inUnsafeAction)
			IF_NBASSERT(obj->dbg.inUnsafeAction = TRUE;)
			if(obj->state.rtp.ssrc != ssrcId){
				//set
				{
					obj->state.rtp.ssrc = ssrcId;
				}
			}
            if(setup != NULL){
                //session
                PRINTF_CONSOLE("TSClientRequesterConn, '%s/%s' setup/session: timeout(%u).\n", obj->cfg.streamId, obj->cfg.versionId, setup->session.parsed.timeout);
                //transport
                {
                    SI32 i; for(i = 0; i < setup->transport.specs.use; i++){
                        const STNBRtspTransportSpec* s = NBArray_itmPtrAtIndex(&setup->transport.specs, STNBRtspTransportSpec, i);
                        STNBString str;
                        NBString_init(&str);
                        NBString_concat(&str, "setup/transport: protocol('");
                        NBString_concat(&str, s->protocol);
                        NBString_concat(&str, "') profile('");
                        NBString_concat(&str, s->profile);
                        NBString_concat(&str, "') lowerTransport('");
                        NBString_concat(&str, s->lowerTransport);
                        NBString_concat(&str, "')");
                        {
                            SI32 i2; for(i2 = 0; i2 < s->params.use; i2++){
                                const STNBRtspParam* p = NBArray_itmPtrAtIndex(&s->params, STNBRtspParam, i2);
                                NBString_concat(&str, " '"); NBString_concat(&str, p->name);
                                NBString_concat(&str, "'('"); NBString_concat(&str, p->value); NBString_concat(&str, "')");
                            }
                        }
                        PRINTF_CONSOLE("TSClientRequesterConn, '%s/%s' %s.\n", obj->cfg.streamId, obj->cfg.versionId, str.str);
                        NBString_release(&str);
                    }
                }
            }
            //Reset times
            {
                const UI64 curUtcTime = NBDatetime_getCurUTCTimestamp();
                obj->state.rtp.packets.utcTime.start = obj->state.rtp.packets.utcTime.last = curUtcTime;
                obj->state.units.utcTime.start = obj->state.units.utcTime.last = curUtcTime;
            }
			NBASSERT(obj->dbg.inUnsafeAction)
			IF_NBASSERT(obj->dbg.inUnsafeAction = FALSE;)
		}
		NBThreadMutex_unlock(&obj->mutex);
	}
}

BOOL TSClientRequesterConn_resRtpIsBorderPresent_(void* pObj, const char* uri, const STNBRtpHdrBasic* hdrs, const STNBDataPtr* chunks, const UI32 chunksSz){
	if(pObj != NULL && hdrs != NULL && chunks != NULL && chunksSz > 0){
		const STNBDataPtr* chunk = chunks;
		const STNBRtpHdrBasic* hdr = hdrs;
		const STNBRtpHdrBasic* hdrAfterEnd = hdr + chunksSz;
		UI8 type = 0;
		while(hdr < hdrAfterEnd){
			if(chunk->ptr != NULL && chunk->use != 0){
				type = (((const BYTE*)chunk->ptr)[hdr->payload.iStart] & 0x1F);
				if(
				   (type > 0 && type < 24) //single fragment NALU
				   || (
					   (type == 28 || type == 29) && hdr->payload.sz > 1 //framented NALU
					   && ((((const BYTE*)chunk->ptr)[hdr->payload.iStart + 1] >> 6) & 0x1) //isFragEnd
					   )
				   ){
					//Single-chunk NALU (notify)
					return TRUE;
					break;
				}
			}
			//Next
			hdr++;
			chunk++;
		}
	}
	return FALSE;
}

/*BOOL TSClientRequesterConn_resRtpStreamParseUnitForPicDescChunckedLocked_(STTSClientRequesterConn* obj, const UI8 refIdc, const UI8 type, const void* prefix, const UI32 prefixSz, const STNBDataPtr* chunks, const SI32 chunksSz){
	BOOL r = FALSE;
	if(chunks != NULL && chunksSz > 0){
		const BOOL picDescBaseOnly = TRUE; //quick-parse only necesary data for 'NBAvcParser_getPicDescBase()'
		//PRINTF_INFO("TSClientRequesterConn, '%s/%s' parsing picDesc from chuncked-unit refIdc(%d) type(%d).\n", obj->cfg.streamId, obj->cfg.versionId, refIdc, type);
		if(!NBAvcParser_feedStart(&obj->state.units.parser, refIdc, type)){
			//PRINTF_CONSOLE_ERROR("TSClientRequesterConn, '%s/%s' NBAvcParser_feedStart failed for refIdc(%d) type(%d).\n", obj->cfg.streamId, obj->cfg.versionId, refIdc, type);
		} else if(!NBAvcParser_feedNALUChunked(&obj->state.units.parser, prefix, prefixSz, chunks, chunksSz, picDescBaseOnly)){
			PRINTF_CONSOLE_ERROR("TSClientRequesterConn, '%s/%s' NBAvcParser_feedNALUChunked(%d) failed for refIdc(%d), %d chunks.\n", obj->cfg.streamId, obj->cfg.versionId, type, refIdc, chunksSz);
		} else if(!NBAvcParser_feedEnd(&obj->state.units.parser)){
			PRINTF_CONSOLE_ERROR("TSClientRequesterConn, '%s/%s' NBAvcParser_feedEnd(%d) failed for refIdc(%d), %d chunks.\n", obj->cfg.streamId, obj->cfg.versionId, type, refIdc, chunksSz);
		} else {
            //Detect picDesc changes
            TSClientRequesterConn_resRtpStreamDetectPicDescChangesLocked_(obj);
			r = TRUE;
		}
	}
	return r;
}*/

void TSClientRequesterConn_resRtpStreamAddUnitUnsafe_(STTSClientRequesterConn* obj, STTSClientRequesterConnStatsUpd* dstUpd, STNBDataPtrReleaser* optPtrsReleaser){
	UI32 msWait = 0, msArrival = 0;
	STTSStreamUnitBuffRef unit = NB_OBJREF_NULL;
	//get unit
	if(obj->state.units.pool.buffs.use > 0){
		//reuse
		unit = NBArray_itmValueAtIndex(&obj->state.units.pool.buffs, STTSStreamUnitBuffRef, obj->state.units.pool.buffs.use - 1);
		NBArray_removeItemAtIndex(&obj->state.units.pool.buffs, obj->state.units.pool.buffs.use - 1);
		//PRINTF_INFO("TSClientRequesterConn, stream-unit-buff reused (%d remain).\n", obj->state.units.pool.buffs.use);
	} else {
		//allocate
		unit = TSStreamUnitBuff_alloc(NULL);
		if(dstUpd != NULL){
			dstUpd->buffs.newAllocs++;
		}
	}
	//picProps
	/*{
		//parse
		if(obj->state.units.frags.arr.use > 0){
			const STTSStreamUnitBuffHdr* hdr = &obj->state.units.frags.hdr;
			STNBDataPtr* arr = NBArray_dataPtr(&obj->state.units.frags.arr, STNBDataPtr);
			TSClientRequesterConn_resRtpStreamParseUnitForPicDescChunckedLocked_(obj, hdr->refIdc, hdr->type, hdr->prefix, hdr->prefixSz, arr, obj->state.units.frags.arr.use);
		}
	}*/
	//add unitBuff as pre-resigned-unitData
	{
		STTSStreamUnit unitData = TSStreamUnitBuff_setDataAsOwningUnit(unit, obj, ++obj->state.units.iSeq, &obj->state.units.frags.hdr, &obj->state.units.frags.arr, TRUE /*swapBuffers*/ /*, &obj->state.rtsp.picDesc.cur*/, optPtrsReleaser);
		NBASSERT(unitData.desc->hdr.type > 0)
		NBArray_addValue(&obj->state.units.pool.data, unitData);
	}
	//update final state
	TSClientRequesterConnStatsUpd_naluProcessed(dstUpd, obj->state.units.frags.hdr.type, obj->state.units.frags.bytesCount, msWait, msArrival);
}

void TSClientRequesterConn_unitsReleaseGroupedLocked_(STTSStreamUnit* objs, const UI32 size, STNBDataPtrReleaser* optPtrsReleaser, void* usrData){
	STTSClientRequesterConn* obj = (STTSClientRequesterConn*)usrData;
	NBASSERT(obj->dbg.inConsumeAction) //must be insde the unsafe-action and consume-action
	const SI32 maxKeep = (obj->cfg.params != NULL ? obj->cfg.params->buffers.units.maxKeep : 0);
	const STTSStreamUnit* objsAfterEnd = objs + size;
	while(objs < objsAfterEnd){
		//ignore units that were retained forward (not owned by this TSStreamUnit anymore)
		NBASSERT(!TSStreamUnitBuff_isSet(objs->def.alloc) || TSStreamUnitBuff_retainCount(objs->def.alloc) > 0)
		if(TSStreamUnitBuff_isSet(objs->def.alloc) && obj->state.units.pool.buffs.use < maxKeep && TSStreamUnitBuff_retainCount(objs->def.alloc) == 1){
			//keep unit-buff (I'm the only retainer)
			NBArray_addValue(&obj->state.units.pool.buffs, objs->def.alloc);
			//PRINTF_INFO("TSClientRequesterConn, stream-unit-buff kept for reutilization (%d total).\n", obj->state.units.pool.buffs.use);
			TSStreamUnit_resignToData(objs);
		}
		//release unit-buff
		TSStreamUnit_release(objs, optPtrsReleaser);
		//next
		objs++;
	}
}

//TMP: writting NAL units to file for dbg
/*
#ifdef NB_CONFIG_INCLUDE_ASSERTS
void TSClientRequesterConn_resRtpConsumePacketsWriteFragmentsToDbgFile_(void* pObj, const UI8 payType, const STNBRtpHdrBasic* head, const STNBDataPtr* chunk){
    STTSClientRequesterConn* obj = (STTSClientRequesterConn*)pObj;
    //TMP: writting NAL units to file for dbg
    static void* _fileObj = NULL;
    static STNBFileRef _file = NB_OBJREF_NULL;
    static BOOL _fileNalFnd7 = FALSE; //(Sequence parameter set, NALU = 7)
    static BOOL _fileNalFnd8 = FALSE; //(Picture parameter set, NALU = 8)
    static BOOL _fileNalFnd5 = FALSE; //(IDR image, NALU = 5)
    //open
    if(!NBFile_isSet(_file)){
        STNBString path;
        NBString_init(&path);
        NBString_concat(&path, "/Volumes/Untitled/_stream_");
        NBString_concatUI64(&path, (UI64)pObj);
        NBString_concat(&path, ".264");
        {
            _file = NBFile_alloc(NULL);
            if(!NBFile_open(_file, path.str, ENNBFileMode_Write)){
                PRINTF_ERROR("TSClientRequesterConn, could not open file to write: '%s'.\n", path.str);
            } else {
                PRINTF_INFO("TSClientRequesterConn, opened file to write: '%s'.\n", path.str);
                _fileObj = pObj;
            }
        }
        NBString_release(&path);
    }
    //write
    if(_fileObj == pObj && NBFile_isSet(_file) && NBFile_isOpen(_file)){
        if(payType == 7) _fileNalFnd7 = TRUE;
        if(payType == 8) _fileNalFnd8 = TRUE;
        //missing sps and pps before image?
        if((payType == 5 || payType == 1) && (!_fileNalFnd7 || !_fileNalFnd8)){
            _fileNalFnd7 = FALSE;
            _fileNalFnd8 = FALSE;
        }
        //IDR found
        if(payType == 5 && _fileNalFnd7 && _fileNalFnd8){
            _fileNalFnd5 = TRUE;
        }
        //write
        if(
           (
               (payType == 5 && _fileNalFnd7 && _fileNalFnd8) //IDR accepted only after 7 and 8 arrived
               || (payType == 1 && _fileNalFnd5) //non-IDR acepted only after IDR arrived
               || (payType != 5 && payType != 1) //non-image accepted any time
           )
           && ((head != NULL && chunk != NULL) || obj->state.units.frags.arr.use > 0)
           )
        {
            NBFile_lock(_file);
            {
                //write 4-bytes hdr (needed by decoders)
                {
                    const BYTE hdr4[4] = {0x00, 0x00, 0x00, 0x01};
                    NBFile_write(_file, hdr4, sizeof(hdr4));
                }
                //write payload
                if(head != NULL && chunk != NULL){
                    //from chunk (not-fragmented unit)
                    NBFile_write(_file, (BYTE*)chunk->ptr + head->payload.iStart, head->payload.sz);
                } else {
                    //fro fragmented units
                    const STTSStreamUnitBuffHdr* hdr = &obj->state.units.frags.hdr;
                    const STNBDataPtr* chunk = NBArray_dataPtr(&obj->state.units.frags.arr, STNBDataPtr);
                    const STNBDataPtr* chunkAfterEnd = chunk + obj->state.units.frags.arr.use;
                    //write prefix
                    if(hdr->prefixSz > 0){
                        NBFile_write(_file, hdr->prefix, hdr->prefixSz);
                    }
                    //write chunks
                    while(chunk < chunkAfterEnd){
                        if(chunk->use > 0){
                            NBFile_write(_file, chunk->ptr, chunk->use);
                        }
                        chunk++;
                    }
                }
                PRINTF_CONSOLE("NAL %d writen to file.\n", payType);
                NBFile_flush(_file);
            }
            NBFile_unlock(_file);
        } else {
            PRINTF_CONSOLE("NAL %d ignored (found 7:%s 8:%s 5:%s).\n", payType, _fileNalFnd7 ? "yes" : "no", _fileNalFnd8 ? "yes" : "no", _fileNalFnd5 ? "yes" : "no");
        }
    }
}
#endif
*/

SI32 TSClientRequesterConn_getNalsCountOfGrp_(const STTSStreamUnit* units, const SI32 unitsSz, const char* group){
    SI32 r = 0;
    if(units != NULL && unitsSz > 0){
        const STTSStreamUnit* u = units;
        const STTSStreamUnit* unitsAfterEnd = units + unitsSz;
        while(u < unitsAfterEnd){
            if(u->desc != NULL && NBString_strIsEqual(NBAvc_getNaluDef(u->desc->hdr.type)->group, group)){
                r++;
            }
            //next
            u++;
        }
    }
    return r;
}

void TSClientRequesterConn_resRtpConsumePackets_(void* pObj, const char* uri, STNBRtpHdrBasic* hdrs, STNBDataPtr* chunks, const UI32 chunksSz, const UI64 curUtcTime, STNBDataPtrReleaser* optPtrsReleaser){
	if(pObj != NULL && hdrs != NULL && chunks != NULL && chunksSz > 0){
		STTSClientRequesterConn* obj = (STTSClientRequesterConn*)pObj;
        BOOL notifyUnits = FALSE;
        //BOOL notifyProps = FALSE; STNBAvcPicDescBase notifyPropsCur; //if notifyProps is TRUE
		STTSClientRequesterConnStatsUpd* upd = &obj->stats.upd;
		NBThreadMutex_lock(&obj->mutex);
		NBASSERT(obj->cfg.address != NULL)
		NBASSERT(NBString_strIsEqual(obj->cfg.address->uri, uri))
		NBASSERT(!obj->dbg.inUnsafeAction && !obj->dbg.inConsumeAction)
		IF_NBASSERT(obj->dbg.inUnsafeAction = obj->dbg.inConsumeAction = TRUE;)
		{
			//Set time
			{
				obj->state.rtp.packets.utcTime.last = curUtcTime;
			}
			//Populate initial buff state
			if(upd->buffs.populated){
				if(upd->buffs.min > obj->state.units.pool.buffs.use){ upd->buffs.min = obj->state.units.pool.buffs.use; } 
				if(upd->buffs.max < obj->state.units.pool.buffs.use){ upd->buffs.max = obj->state.units.pool.buffs.use; }
			} else {
				upd->buffs.populated = TRUE;
				upd->buffs.min = upd->buffs.max = obj->state.units.pool.buffs.use;
			}
			//Process packets
			{
				UI32 i; for(i = 0; i < chunksSz; i++){
					STNBRtpHdrBasic* head	= &hdrs[i];
					STNBDataPtr* chunk		= &chunks[i];
					const BOOL isGroupedGap	= (chunk->ptr == NULL || chunk->use <= 0);
					const BOOL isValidSeq	= (!obj->state.rtp.packets.seqNum.isFirstKnown || (head->head.seqNum == ((obj->state.rtp.packets.seqNum.lastSeqNum + 1) % 0xFFFF)));
					//Validate sequence
					if(!isValidSeq || isGroupedGap){
						//Packet(s) lost (groups of gaps are reported as one empty packet)
						//stats
						{
							if(obj->state.units.frags.arr.use > 0){
								TSClientRequesterConnStatsUpd_naluMalformed(upd, obj->state.units.frags.hdr.type, obj->state.units.frags.bytesCount);
							} else {
								TSClientRequesterConnStatsUpd_unknownMalformed(upd, chunk->use);
							}
						}
						//empty fragments
						if(obj->state.units.frags.arr.use > 0){
							TSClientRequesterConn_fragsEmptyUnsafe_(obj, optPtrsReleaser);
							NBASSERT(obj->state.units.frags.arr.use == 0 && obj->state.units.frags.bytesCount == 0)
						}
						//Reset sequence
						obj->state.rtp.packets.seqNum.isFirstKnown	= FALSE;
						obj->state.rtp.packets.seqNum.lastSeqNum	= 0;
					} else {
						//if(i == 0){
						//	const float ts = (float)((packet->head.head.timestamp >> 16) & 0xFFFF) + ((packet->head.head.timestamp & 0xFFFF) / (float)((int)0xFFFF)); 
						//	PRINTF_INFO("TSClientRequesterConn, timestamp %.2f.\n", ts);
						//}
						//Analyze NAL
						const BYTE* chunkStart		= (const BYTE*)chunk->ptr;
						const BYTE* nalDataStart	= &chunkStart[head->payload.iStart];
						const BYTE* nalDataAfterEnd	= nalDataStart + head->payload.sz;
						const BYTE* nalPos			= nalDataStart;
						const BYTE nalFirstByte		= *(nalPos++);
						const STNBAvcNaluHdr desc   = NBAvc_getNaluHdr(nalFirstByte);
						//Set sequence
						obj->state.rtp.packets.seqNum.isFirstKnown	= TRUE;
						obj->state.rtp.packets.seqNum.lastSeqNum	= head->head.seqNum;
						//
						//if(desc.type == 7){
						//	PRINTF_INFO("TSClientRequesterConn, nalType(%d).\n", desc.type);
						//}
						if(desc.type <= 0 || desc.type >= 30){
							//PRINTF_INFO("StreamRequester, NAL packet type(%d) is reserved.\n", desc.type);
							//stats
							TSClientRequesterConnStatsUpd_unknownMalformed(upd, chunk->use);
						} else if(desc.forbiddenZeroBit){
							//PRINTF_INFO("StreamRequester, NAL ignoring packet with active forbiddenZeroBit.\n");
							//stats
							TSClientRequesterConnStatsUpd_naluIgnored(upd, desc.type, chunk->use);
                            //Set time (even if no-viewers)
                            {
                                obj->state.units.utcTime.last = curUtcTime;
                            }
						} else {
#							ifdef NB_CONFIG_INCLUDE_ASSERTS
							/*{
							 const STNBAvcNaluDef* nTypeDef = NBAvc_getNaluDef(desc.type);
							 PRINTF_INFO("StreamRequester, (%lld) H.264 NAL packet type(%u, '%s', '%s').\n", (SI64)obj, desc.type, nTypeDef->group, nTypeDef->desc);
							 }*/
#							endif
							if(desc.type > 0 && desc.type < 24){
#								ifdef NB_CONFIG_INCLUDE_ASSERTS
								//Print
								/*{
								 const char* hexChars = "0123456789abcdef";
								 const BYTE* pData = (const BYTE*)hdr.payload.data;
								 const BYTE* pDataAfterEnd = pData + hdr.payload.sz;
								 STNBString str;
								 NBString_init(&str);
								 while(pData < pDataAfterEnd){
								 NBString_concatByte(&str, hexChars[(*pData >> 4) & 0xF]);
								 NBString_concatByte(&str, hexChars[*pData & 0xF]);
								 NBString_concatByte(&str, pDataAfterEnd != pData && ((pDataAfterEnd - pData) % 32) == 0 ? '\n' : ' ');
								 pData++;
								 }
								 PRINTF_INFO("StreamRequester, port(%d) NAL payload:\n------>\n%s\n<------\n", opq->port, str.str);
								 NBString_release(&str);
								 }*/
#								endif
								//Reset fragmented state (incomplete)
								if(obj->state.units.frags.arr.use > 0){
									//PRINTF_WARNING("StreamRequester, incomplete fragmented-NALU before unfragmented-NALU, discarting %d packets %u bytes.\n", obj->state.units.chunks.use, obj->state.units.frags.bytesCount);
									//stats
									{
										TSClientRequesterConnStatsUpd_naluMalformed(upd, obj->state.units.frags.hdr.type, obj->state.units.frags.bytesCount);
									}
									//empty fragments
									{
										TSClientRequesterConn_fragsEmptyUnsafe_(obj, optPtrsReleaser);
										NBASSERT(obj->state.units.frags.arr.use == 0 && obj->state.units.frags.bytesCount == 0)
									}
								}
                                //TMP: writting NAL units to file for dbg
                                /*
#                               ifdef NB_CONFIG_INCLUDE_ASSERTS
                                TSClientRequesterConn_resRtpConsumePacketsWriteFragmentsToDbgFile_(pObj, desc.type, head, chunk);
#                               endif
                                */
								//Notify
                                //ToDo: remove (this is a non-compliant validation).
                                const BOOL ignore = 0; //TSClientRequesterConn_mustIgnoreNALU_(obj, desc.type);
                                if(ignore){
                                    //stats
                                    TSClientRequesterConnStatsUpd_naluIgnored(upd, desc.type, head->payload.sz);
                                } else {
                                    if(obj->viewers.cur.use > 0){
                                        //Populate and add unit
                                        {
                                            //header
                                            {
                                                obj->state.units.frags.hdr.refIdc       = desc.refIdc;
                                                obj->state.units.frags.hdr.type         = desc.type;
                                                obj->state.units.frags.hdr.prefix[0]    = 0;
                                                obj->state.units.frags.hdr.prefixSz     = 0;
                                                obj->state.units.frags.hdr.tsRtp        = head->head.timestamp;
                                                NBASSERT(obj->state.units.frags.bytesCount == 0)
                                            }
                                            //populate
                                            {
                                                STNBDataPtr chunk2;
                                                NBDataPtr_init(&chunk2);
                                                NBDataPtr_setAsSubRng(&chunk2, chunk, head->payload.iStart, head->payload.sz);
                                                NBDataPtr_preResign(&chunk2); //preResign for listener (reduce locks and reallocations)
                                                NBASSERT(chunk2.use > 0)
                                                NBArray_addValue(&obj->state.units.frags.arr, chunk2); //will be released latter
                                                //fragment stats
                                                obj->state.units.frags.bytesCount += chunk2.use;
                                            }
                                            //add unit
                                            {
                                                TSClientRequesterConn_resRtpStreamAddUnitUnsafe_(obj, upd, optPtrsReleaser);
                                            }
                                            //empty unused fragments
                                            {
                                                TSClientRequesterConn_fragsEmptyUnsafe_(obj, optPtrsReleaser);
                                                NBASSERT(obj->state.units.frags.arr.use == 0 && obj->state.units.frags.bytesCount == 0)
                                            }
                                        }
                                    }
                                }
                                //Set time (all units)
                                {
                                    obj->state.units.utcTime.last = curUtcTime;
                                }
							} else if((desc.type == 28 || desc.type == 29) && nalPos < nalDataAfterEnd){
								BOOL isValid = TRUE, isFragStart = FALSE, isFragEnd = FALSE, reservedBit = FALSE;
								UI8 payType = 0; UI16 don = 0; 
								{
									const BYTE c	= *(nalPos++);
									isFragStart		= ((c >> 7) & 0x1);
									isFragEnd		= ((c >> 6) & 0x1);
									reservedBit		= ((c >> 5) & 0x1);
									payType			= (c & 0x1F);
									//if(isFragStart){ PRINTF_INFO("StreamRequester, port(%d) NAL fragment START.\n", opq->port); }
									//if(isFragEnd){ PRINTF_INFO("StreamRequester, port(%d) NAL fragment END.\n", opq->port); }
									if(desc.type == 29){
										if((nalDataAfterEnd - nalPos) < 2){
											PRINTF_WARNING("StreamRequester, H.264 ignoring NALU without DON data (truncated?).\n");
											isValid = FALSE;
										} else {
											don |= ((UI16)*(nalPos++) << 8); 
											don |= *(nalPos++);
										}
									}
									if(reservedBit != 0){
										PRINTF_WARNING("StreamRequester, ignoring NALU with active reservedBit.\n");
										isValid = FALSE;
									}
								}
								//Process
								if(!isValid){
									//stats
									{
										if(obj->state.units.frags.arr.use > 0){
											TSClientRequesterConnStatsUpd_naluMalformed(upd, obj->state.units.frags.hdr.type, obj->state.units.frags.bytesCount);
										}
										TSClientRequesterConnStatsUpd_unknownMalformed(upd, chunk->use);
									}
									//empty fragments
									if(obj->state.units.frags.arr.use > 0){
										TSClientRequesterConn_fragsEmptyUnsafe_(obj, optPtrsReleaser);
										NBASSERT(obj->state.units.frags.arr.use == 0 && obj->state.units.frags.bytesCount == 0)
									}
								} else {
									if(isFragStart){
										//validate missing start-chunk
										//if(obj->state.units.chunks.use > 0){
										//	start-chunk before end-chunk for previous NAL
										//	PRINTF_WARNING("StreamRequester, missing end-chunk before new start-chunk, discarting NALs %d packets %u bytes.\n", obj->state.units.chunks.use, obj->state.units.frags.bytesCount);
										//}
									} else {
										//validate missing start-chunk
										if(obj->state.units.frags.arr.use == 0){
											//missing start-chunk before this one
											//PRINTF_WARNING("StreamRequester, missing start-chunk before continuation-chunk, discarting NALs %d packets %u bytes.\n", obj->state.units.chunks.use, obj->state.units.frags.bytesCount);
											isValid = FALSE;
										}
									}
									//Reset payload
									if(!isValid || isFragStart){
										//empty fragments
										if(obj->state.units.frags.arr.use > 0){
											//stats
											{
												TSClientRequesterConnStatsUpd_naluMalformed(upd, obj->state.units.frags.hdr.type, obj->state.units.frags.bytesCount);
											}
											//empty fragments
											{
												TSClientRequesterConn_fragsEmptyUnsafe_(obj, optPtrsReleaser);
												NBASSERT(obj->state.units.frags.arr.use == 0 && obj->state.units.frags.bytesCount == 0)
											}
										}
										//add first fragment
										if(isFragStart){
											//unit
											obj->state.units.frags.hdr.type			= payType;
											obj->state.units.frags.hdr.prefix[0]	= (nalFirstByte & 0xE0) | payType;
											obj->state.units.frags.hdr.prefixSz		= 1;
											obj->state.units.frags.hdr.tsRtp		= head->head.timestamp;
											NBASSERT(obj->state.units.frags.bytesCount == 0)
											isValid = TRUE;
#											ifdef NB_CONFIG_INCLUDE_ASSERTS
											/*{
											 const STNBAvcNaluDef* nTypeDef = NBAvc_getNaluDef(payType);
											 PRINTF_INFO("StreamRequester, port(%d) H.264 NAL fragmented packet type(%u, '%s', '%s').\n", opq->port, payType, nTypeDef->group, nTypeDef->desc);
											 }*/
#											endif
										}
									}
									if(!isValid){
										//stats
										TSClientRequesterConnStatsUpd_unknownMalformed(upd, chunk->use);
									} else {
										//Concat fragment
										{
											STNBDataPtr chunk2;
											NBDataPtr_init(&chunk2);
											NBDataPtr_setAsSubRng(&chunk2, chunk, (UI32)(nalPos - chunkStart), (UI32)(nalDataAfterEnd - nalPos));
											NBDataPtr_preResign(&chunk2); //preResign for listener (reduce locks and reallocations)
											NBASSERT(chunk2.use > 0)
											NBArray_addValue(&obj->state.units.frags.arr, chunk2); //will be released latter
											//fragment stats
											obj->state.units.frags.bytesCount += chunk2.use;
										}
										//Feed payload
										if(isFragEnd){
											//PRINTF_INFO("StreamRequester, port(%d) feeding %u (%u bytes) accumulated fragments to H.264.\n", opq->port, s->state.nal.payloadFramesCount, s->state.nal.payload.length);
#											ifdef NB_CONFIG_INCLUDE_ASSERTS
											//Print
											/*{
											 const char* hexChars = "0123456789abcdef";
											 const BYTE* pData = (const BYTE*)s->state.nal.payload.str;
											 const BYTE* pDataAfterEnd = pData + s->state.nal.payload.length;
											 STNBString str;
											 NBString_init(&str);
											 while(pData < pDataAfterEnd){
											 NBString_concatByte(&str, hexChars[(*pData >> 4) & 0xF]);
											 NBString_concatByte(&str, hexChars[*pData & 0xF]);
											 NBString_concatByte(&str, pDataAfterEnd != pData && ((pDataAfterEnd - pData) % 32) == 0 ? '\n' : ' ');
											 pData++;
											 }
											 PRINTF_INFO("StreamRequester, port(%d) NAL payload:\n------>\n%s\n<------\n", opq->port, str.str);
											 NBString_release(&str);
											 }*/
#											endif
                                            //
                                            //TMP: writting NAL units to file for dbg
                                            /*
#                                           ifdef NB_CONFIG_INCLUDE_ASSERTS
                                            TSClientRequesterConn_resRtpConsumePacketsWriteFragmentsToDbgFile_(pObj, payType, NULL, NULL);
#                                           endif
                                            */
                                            //
											NBASSERT(obj->state.units.frags.arr.use > 0)
											if(obj->state.units.frags.arr.use > 0){
                                                //ToDo: remove (this is a non-compliant validation).
                                                const BOOL ignore = 0; //TSClientRequesterConn_mustIgnoreNALU_(obj, obj->state.units.frags.hdr.type);
                                                if(ignore){
                                                    //stats
                                                    TSClientRequesterConnStatsUpd_naluIgnored(upd, obj->state.units.frags.hdr.type, obj->state.units.frags.bytesCount);
                                                } else {
                                                    if(obj->viewers.cur.use > 0){
                                                        //Populate and add unit
                                                        TSClientRequesterConn_resRtpStreamAddUnitUnsafe_(obj, upd, optPtrsReleaser);
                                                    }
                                                }
                                                //Set time (even if no-viewers)
                                                {
                                                    obj->state.units.utcTime.last = curUtcTime;
                                                }
												//empty unused fragments
												{
													TSClientRequesterConn_fragsEmptyUnsafe_(obj, optPtrsReleaser);
													NBASSERT(obj->state.units.frags.arr.use == 0 && obj->state.units.frags.bytesCount == 0)
												}
											}
										}
									}
								}
							} else if(desc.type < 32 && nalPos < nalDataAfterEnd){
								//Ignored (not included yet)
								TSClientRequesterConnStatsUpd_naluIgnored(upd, desc.type, head->payload.sz);
                                //Set time (even if no-viewers)
                                {
                                    obj->state.units.utcTime.last = curUtcTime;
                                }
							} else { //nalType out-of-range, empty-data, ...
								//Malformed
								TSClientRequesterConnStatsUpd_unknownMalformed(upd, head->payload.sz);
							}
						}
					}
				}
			}
			//update buff state (before notification)
			{
				NBASSERT(upd->buffs.populated)
				if(upd->buffs.min > obj->state.units.pool.buffs.use){ upd->buffs.min = obj->state.units.pool.buffs.use; }
				if(upd->buffs.max < obj->state.units.pool.buffs.use){ upd->buffs.max = obj->state.units.pool.buffs.use; }
			}
			//notify
			{
				//lstnrs
                notifyUnits = (obj->state.units.pool.data.use > 0);
				if(notifyUnits){
					//sync notify array
					if(obj->viewers.notify.iSeq != obj->viewers.iSeq){
                        STTSClientRequesterConnViewer* lstnrs = NBArray_dataPtr(&obj->viewers.cur, STTSClientRequesterConnViewer);
						NBArray_empty(&obj->viewers.notify.cur);
						NBArray_addItems(&obj->viewers.notify.cur, lstnrs, sizeof(lstnrs[0]), obj->viewers.cur.use);
                        //flag new viewers as not-new
                        {
                            STTSClientRequesterConnViewer* l = lstnrs;
                            const STTSClientRequesterConnViewer* lAfterEnd = l + obj->viewers.cur.use;
                            while(l < lAfterEnd){
                                if(!l->isFirstNotifyDone){
                                    l->isFirstNotifyDone = TRUE;
                                }
                                //next
                                l++;
                            }
                        }
                        //
                        obj->viewers.notify.iSeq = obj->viewers.iSeq;
					}
				}
				//picProps
				/*if((notifyProps = obj->state.rtsp.picDesc.changed)){
					//clone picProps to notify
					NBMemory_setZeroSt(notifyPropsCur, STNBAvcPicDescBase);
					NBStruct_stClone(NBAvcPicDescBase_getSharedStructMap(), &obj->state.rtsp.picDesc.cur, sizeof(obj->state.rtsp.picDesc.cur), &notifyPropsCur, sizeof(notifyPropsCur));
				}*/
			}
			//obj->state.rtsp.picDesc.changed = FALSE;
		}
		//update buff state (after notification)
		/*{
		 NBASSERT(upd->buffs.populated)
		 if(upd->buffs.min > obj->state.units.pool.refs.use){
		 upd->buffs.min = obj->state.units.pool.refs.use;
		 }
		 if(upd->buffs.max < obj->state.units.pool.refs.use){
		 upd->buffs.max = obj->state.units.pool.refs.use;
		 }
		 }*/
		//
		NBASSERT(obj->dbg.inUnsafeAction)
		IF_NBASSERT(obj->dbg.inUnsafeAction = FALSE;)
		NBThreadMutex_unlock(&obj->mutex);
		//notify units (unlocked)
		if(notifyUnits && obj->state.units.pool.data.use > 0){
			STTSStreamUnit* units = NBArray_dataPtr(&obj->state.units.pool.data, STTSStreamUnit);
			const STTSStreamUnit* unitAfterEnd = units + obj->state.units.pool.data.use;
            //Notify last units only (non-new listeners)
            {
                STTSStreamUnitsReleaser unitsReleaser;
                SI32 i; for(i = 0; i < obj->viewers.notify.cur.use; i++){
                    STTSClientRequesterConnViewer* v = NBArray_itmPtrAtIndex(&obj->viewers.notify.cur, STTSClientRequesterConnViewer, i);
                    if(v->isFirstNotifyDone && v->lstnr.streamConsumeUnits != NULL){
                        NBMemory_setZeroSt(unitsReleaser, STTSStreamUnitsReleaser);
                        unitsReleaser.itf.unitsReleaseGrouped = TSClientRequesterConn_unitsReleaseGroupedLocked_;
                        unitsReleaser.usrData = obj;
                        unitsReleaser.ptrsReleaser = optPtrsReleaser;
                        (*v->lstnr.streamConsumeUnits)(v->lstnr.param, units, (UI32)(unitAfterEnd - units), &unitsReleaser);
                    }
                }
            }
            //Build frames.fromLastIDR
            {
                STTSStreamUnit* u = units;
                while(u < unitAfterEnd){
                    NBASSERT(u->desc != NULL)
                    if(u->desc != NULL){
                        const SI32 nalType = u->desc->hdr.type;
                        BOOL unitConsumed = FALSE;
                        BOOL startNewFrame = FALSE;
                        BOOL keepCurNalInCurFrame = FALSE;
                        //Validate start of new access unit
                        if(
                           nalType == 9 /*Access unit delimiter*/
                           ){
                               
                               NBASSERT((obj->state.units.frames.cur.state.nalsCountPerType[9] && obj->state.units.frames.cur.state.delimeter.isPresent) || (!obj->state.units.frames.cur.state.nalsCountPerType[9] && !obj->state.units.frames.cur.state.delimeter.isPresent)) //must match
                               if(!startNewFrame && obj->state.units.frames.cur.units.use > 0){
                                   //PRINTF_INFO("StreamContext, nal-type(%d) opening new frame ('Access unit delimiter' at non-empty frame).\n", nalType);
                                   startNewFrame = TRUE;
                                   keepCurNalInCurFrame = FALSE;
                               }
                           } else if(
                                     nalType == 7 /*Sequence parameter set*/
                                     || nalType == 8 /*Sequence parameter set*/
                                     || nalType == 6 /*SEI*/
                                     || (nalType >= 14 && nalType <= 18) /*...*/
                                     //ToDo: implement '7.4.1.2.4 Detection of the first VCL NAL unit of a primary coded picture'.
                                     //Asuming all VCL NALs are the first NAL of a primary coded.
                                     || NBString_strIsEqual(NBAvc_getNaluDef(nalType)->group, "VCL") /*first VCL NAL unit of a primary coded picture*/
                                     )
                           {
                               if(!startNewFrame && TSClientRequesterConn_getNalsCountOfGrp_(NBArray_dataPtr(&obj->state.units.frames.cur.units, STTSStreamUnit), obj->state.units.frames.cur.units.use, "VCL") > 0){
                                   //PRINTF_INFO("StreamContext, opening new frame (nalType %d after the last VCL NAL).\n", nalType);
                                   startNewFrame = TRUE;
                                   keepCurNalInCurFrame = FALSE;
                               }
                           }
                        //Safety, this should be previouly validated at 'isEndOfNAL'
                        NBASSERT(obj->state.units.frames.cur.state.nalsCountPerType[10] == 0)
                        if(!startNewFrame && obj->state.units.frames.cur.state.nalsCountPerType[10]){
                            //PRINTF_INFO("StreamContext, opening new frame ('End of sequence' already added).\n");
                            startNewFrame = TRUE;
                            keepCurNalInCurFrame = FALSE;
                        }
                        //constrains
                        if(nalType == 13 /*Sequence parameter set extension*/){
                            if(obj->state.units.frames.cur.state.lastCompletedNalType != 7 /*Sequence parameter set*/){
                                if(!obj->state.units.frames.cur.state.isInvalid){
                                    PRINTF_WARNING("StreamContext, invalidating frame ('Sequence parameter set extension' without inmediate-previous 'Sequence parameter set').\n");
                                    obj->state.units.frames.cur.state.isInvalid = 1;
                                }
                            }
                        } else if(nalType == 19 /*coded slice of an auxiliary coded picture without partitioning*/){
                            const int nalVCLCount = TSClientRequesterConn_getNalsCountOfGrp_(NBArray_dataPtr(&obj->state.units.frames.cur.units, STTSStreamUnit), obj->state.units.frames.cur.units.use, "VCL");
                            if(nalVCLCount == 0){
                                if(!obj->state.units.frames.cur.state.isInvalid){
                                    PRINTF_WARNING("StreamContext, invalidating frame ('auxiliary coded picture' without previous 'primary or redundant coded pictures').\n");
                                    obj->state.units.frames.cur.state.isInvalid = 1;
                                }
                            }
                        } else if(nalType == 0 || nalType == 12 || (nalType >= 20 && nalType <= 31)){
                            if(!obj->state.units.frames.cur.state.isInvalid && TSClientRequesterConn_getNalsCountOfGrp_(NBArray_dataPtr(&obj->state.units.frames.cur.units, STTSStreamUnit), obj->state.units.frames.cur.units.use, "VCL") == 0){
                                PRINTF_WARNING("StreamContext, invalidating frame (nalType %d shall not precede the first VCL of the primary coded picture).\n", nalType);
                                obj->state.units.frames.cur.state.isInvalid = 1;
                            }
                            
                        } else if(nalType == 10 /*End of sequence*/){
                            startNewFrame = TRUE;
                            keepCurNalInCurFrame = TRUE;
                        }
                        //
                        if(startNewFrame){
                            //add before moving frame to 'fromLastIDR'
                            if(keepCurNalInCurFrame){
                                NBASSERT(!unitConsumed)
                                if(!unitConsumed){
                                    obj->state.units.frames.cur.state.nalsCountPerType[nalType]++;
                                    obj->state.units.frames.cur.state.lastCompletedNalType = nalType;
                                    NBArray_addValue(&obj->state.units.frames.cur.units, *u);
                                    unitConsumed = TRUE;
                                }
                            }
                            //if cur frame is and IDR, flush 'fromLastIDR'
                            if(obj->state.units.frames.cur.state.nalsCountPerType[5] > 0){
                                //current frame is IDR, flush
                                STTSStreamUnitsReleaser unitsReleaser;
                                NBMemory_setZeroSt(unitsReleaser, STTSStreamUnitsReleaser);
                                unitsReleaser.itf.unitsReleaseGrouped = TSClientRequesterConn_unitsReleaseGroupedLocked_;
                                unitsReleaser.usrData = obj;
                                unitsReleaser.ptrsReleaser = optPtrsReleaser;
                                TSStreamUnit_unitsReleaseGrouped(NBArray_dataPtr(&obj->state.units.frames.fromLastIDR, STTSStreamUnit), obj->state.units.frames.fromLastIDR.use, &unitsReleaser);
                                //empty
                                NBArray_empty(&obj->state.units.frames.fromLastIDR);
                            }
                            //move current 'frames.cur.units' to 'frames.fromLastIDR'
                            {
                                NBArray_addItems(&obj->state.units.frames.fromLastIDR, NBArray_dataPtr(&obj->state.units.frames.cur.units, STTSStreamUnit), sizeof(STTSStreamUnit), obj->state.units.frames.cur.units.use);
                                NBArray_empty(&obj->state.units.frames.cur.units);
                                NBMemory_set(&obj->state.units.frames.cur.state, 0, sizeof(obj->state.units.frames.cur.state));
                            }
                        }
                        //add
                        if(!unitConsumed){
                            obj->state.units.frames.cur.state.nalsCountPerType[nalType]++;
                            obj->state.units.frames.cur.state.lastCompletedNalType = nalType;
                            NBArray_addValue(&obj->state.units.frames.cur.units, *u);
                            unitConsumed = TRUE;
                        }
                    }
                    //next
                    u++;
                }
            }
			//Notify 'frames.fromLastIDR' and 'frames.cur.units' (to new-listeners)
			{
				STTSStreamUnitsReleaser unitsReleaser;
                /*IF_NBASSERT(
                            {
                                SI32 i; for(i = 0; i < obj->state.units.frames.fromLastIDR.use; i++){
                                    const STTSStreamUnit* u1 = NBArray_itmPtrAtIndex(&obj->state.units.frames.fromLastIDR, STTSStreamUnit, i);
                                    SI32 i2; for(i2 = 0; i2 < obj->state.units.frames.cur.units.use; i2++){
                                        const STTSStreamUnit* u2 = NBArray_itmPtrAtIndex(&obj->state.units.frames.cur.units, STTSStreamUnit, i2);
                                        NBASSERT(u1->desc != u2->desc && u1->desc->unitId != u2->desc->unitId)
                                    }
                                }
                            }
                            )*/
				SI32 i; for(i = 0; i < obj->viewers.notify.cur.use; i++){
                    STTSClientRequesterConnViewer* v = NBArray_itmPtrAtIndex(&obj->viewers.notify.cur, STTSClientRequesterConnViewer, i);
                    if(!v->isFirstNotifyDone && v->lstnr.streamConsumeUnits != NULL){
                        PRINTF_INFO("TSClientRequesterConn, viewer, sending last frames since IDR.\n");
                        //notify 'frames.fromLastIDR' and 'frames.cur' (including last units)
                        NBMemory_setZeroSt(unitsReleaser, STTSStreamUnitsReleaser);
                        unitsReleaser.itf.unitsReleaseGrouped = TSClientRequesterConn_unitsReleaseGroupedLocked_;
                        unitsReleaser.usrData = obj;
                        unitsReleaser.ptrsReleaser = optPtrsReleaser;
                        (*v->lstnr.streamConsumeUnits)(v->lstnr.param, NBArray_dataPtr(&obj->state.units.frames.fromLastIDR, STTSStreamUnit), obj->state.units.frames.fromLastIDR.use, &unitsReleaser);
                        //
                        NBMemory_setZeroSt(unitsReleaser, STTSStreamUnitsReleaser);
                        unitsReleaser.itf.unitsReleaseGrouped = TSClientRequesterConn_unitsReleaseGroupedLocked_;
                        unitsReleaser.usrData = obj;
                        unitsReleaser.ptrsReleaser = optPtrsReleaser;
                        (*v->lstnr.streamConsumeUnits)(v->lstnr.param, NBArray_dataPtr(&obj->state.units.frames.cur.units, STTSStreamUnit), obj->state.units.frames.cur.units.use, &unitsReleaser);
                        //force the update of 'isFirstNotify' flags
                        obj->viewers.iSeq++;
                    }
				}
			}
			//Release
			{
				//empty without releasing (units are retained by 'current frame' and 'last frames from IDR')
				NBArray_empty(&obj->state.units.pool.data);
			}
		}
		//notif picProps (unlocked)
		/*if(notifyProps){
			if(obj->cfg.lstnr.itf.rtspStreamVideoPropsChanged != NULL){
				(*obj->cfg.lstnr.itf.rtspStreamVideoPropsChanged)(obj->cfg.lstnr.param, obj, obj->cfg.streamId, obj->cfg.versionId, &notifyPropsCur);
			}
			NBStruct_stRelease(NBAvcPicDescBase_getSharedStructMap(), &notifyPropsCur, sizeof(notifyPropsCur));
		}*/
		NBASSERT(obj->dbg.inConsumeAction)
		IF_NBASSERT(obj->dbg.inConsumeAction = FALSE;)
	}
}

void TSClientRequesterConn_resRtpFlushStats_(void* pObj){
	STTSClientRequesterConn* obj = (STTSClientRequesterConn*)pObj;
	NBThreadMutex_lock(&obj->mutex);
	{
		NBASSERT(!obj->dbg.inUnsafeAction)
		IF_NBASSERT(obj->dbg.inUnsafeAction = TRUE;)
		{
			//Apply telemetry (unlocked)
			if(obj->stats.provider != NULL){
				STTSClientRequesterConnStatsUpd* upd = &obj->stats.upd;
				TSClientRequesterConnStats_addUpd(obj->stats.provider, upd);
				//reset
				{
					TSClientRequesterConnStatsUpd_release(upd);
					TSClientRequesterConnStatsUpd_init(upd);
				}
			}
		}
		NBASSERT(obj->dbg.inUnsafeAction)
		IF_NBASSERT(obj->dbg.inUnsafeAction = FALSE;)
	}
	NBThreadMutex_unlock(&obj->mutex);
}

