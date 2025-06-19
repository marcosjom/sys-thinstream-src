//
//  TSServerStreamReq.c
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSServerStreamReq.h"
#include "core/server/TSServerOpq.h"
//
#include "nb/core/NBNumParser.h"

//stream req

void TSServerStreamReq_sourcesSyncLocked_(STTSServerStreamReq* obj);
BOOL TSServerStreamReq_moveToAnyStreamLocked_(STTSServerStreamReq* obj, STTSStreamUnit* dstUnit);
BOOL TSServerStreamReq_getNextBestLiveUnitLocked_(STTSServerStreamReq* obj, STTSStreamUnit* dstUnit, BOOL* dstNewCompatibleBlockAvailable);
void TSServerStreamReq_setCurStreamLocked_(STTSServerStreamReq* obj, STTSServerStreamReqConn* conn, const UI32 iFrameSeq);

//garbage

void TSServerStreamReq_unitsReleaseGrouped_(STTSStreamUnit* objs, const UI32 size, STNBDataPtrReleaser* optPtrsReleaser, void* usrData);

//TSServerStreamReq

void TSServerStreamReq_init(STTSServerStreamReq* obj){
	STNBTimestampMicro time = NBTimestampMicro_getMonotonicFast();
	NBMemory_setZeroSt(*obj, STTSServerStreamReq);
	NBThreadMutex_init(&obj->mutex);
	//cfg
	{
		NBUrl_init(&obj->cfg.uri);
		NBArray_init(&obj->cfg.srcs, sizeof(STTSServerStreamPath), NULL);
	}
	//time
	{
		obj->state.tick.last = time;
	}
	//net
	{
		//send
		{
			//h.264, mp4 stream builder
			{
				obj->net.send.queue = TSStreamBuilder_alloc(NULL);
				TSStreamBuilder_setFormatAndUnitsPrefix(obj->net.send.queue, ENTSStreamBuilderFmt_H264Raw, ENTSStreamBuilderUnitsPrefix_0x00000001);
			}
		}
	}
	//reqs
	{
		NBArray_init(&obj->reqs.arr, sizeof(STTSServerStreamReqConn*), NULL);
	}
	//garbage
	{
		NBArray_init(&obj->garbage.arr, sizeof(STTSStreamUnit), NBCompare_TSStreamUnit_byDescPtrOnly);
	}
	//mp4
	{
		//
	}
	//state
	{
		//accum
		{
			obj->state.stream.units.lastTime = time;
			obj->state.stream.frames.lastTime = time;
		}
	}
}
	
void TSServerStreamReq_release(STTSServerStreamReq* obj){
	NBThreadMutex_lock(&obj->mutex);
	{
		//reqs
		{
			SI32 i; for(i = 0; i < obj->reqs.arr.use; i++){
				STTSServerStreamReqConn* req = NBArray_itmValueAtIndex(&obj->reqs.arr, STTSServerStreamReqConn*, i);
				TSServerStreamReqConn_release(req);
				NBMemory_free(req);
				req = NULL;
			}
			NBArray_empty(&obj->reqs.arr);
			NBArray_release(&obj->reqs.arr);
			//
			obj->reqs.cur.req = NULL;
            //fmt
            /*{
                if(obj->reqs.fmt.mergedFmt != NULL){
                    NBStruct_stRelease(NBAvcPicDescBase_getSharedStructMap(), obj->reqs.fmt.mergedFmt, sizeof(*obj->reqs.fmt.mergedFmt));
                    NBMemory_free(obj->reqs.fmt.mergedFmt);
                    obj->reqs.fmt.mergedFmt = NULL;
                }
            }*/
		}
		//garbage
		{
			if(obj->garbage.arr.use > 0){
				STTSStreamUnit* u = NBArray_dataPtr(&obj->garbage.arr, STTSStreamUnit);
				TSStreamUnit_unitsReleaseGrouped(u, obj->garbage.arr.use, NULL);
				NBArray_empty(&obj->garbage.arr);
			}
			NBArray_release(&obj->garbage.arr);
		}
		//cfg
		{
			//srcs
			{
				SI32 i; for(i = 0; i < obj->cfg.srcs.use; i++){
					STTSServerStreamPath* src = NBArray_itmPtrAtIndex(&obj->cfg.srcs, STTSServerStreamPath, i);
					TSServerStreamPath_release(src);
				}
				NBArray_empty(&obj->cfg.srcs);
				NBArray_release(&obj->cfg.srcs);
			}
			//uri
			{
				NBUrl_release(&obj->cfg.uri);
			}
		}
		//net
		{
            //rawLnk
            {
                if(NBHttpServiceRespRawLnk_isSet(&obj->net.rawLnk)){
                    NBHttpServiceRespRawLnk_close(&obj->net.rawLnk);
                }
                NBMemory_setZeroSt(obj->net.rawLnk, STNBHttpServiceRespRawLnk);
            }
			if(NBWebSocket_isSet(obj->net.websocket)){
				NBWebSocket_release(&obj->net.websocket);
				NBWebSocket_null(&obj->net.websocket);
			}
			//send
			{
				if(TSStreamBuilder_isSet(obj->net.send.queue)){
					TSStreamBuilder_release(&obj->net.send.queue);
					TSStreamBuilder_null(&obj->net.send.queue);
				}
			}
		}
		//mp4
		{
			//
		}
		//state
		{
			//desc
			{
				//NBAvcPicDescBase_release(&obj->state.stream.desc.base);
			}
		}
	}
	NBThreadMutex_unlock(&obj->mutex);
	NBThreadMutex_release(&obj->mutex);
}

BOOL TSServerStreamReq_setCfg(STTSServerStreamReq* obj, struct STTSServerStreams_* streamServer, const char* absPath, const char* query, const char* fragment, const STNBHttpServiceReqDesc reqDesc){
	BOOL r = FALSE;
	NBThreadMutex_lock(&obj->mutex);
	{
		BOOL uriIsValid = FALSE;
		obj->obj = streamServer;
		//cfg
		{
			//parse uri
			{
				STNBString strUri;
				NBString_init(&strUri);
				NBString_concat(&strUri, "lcl://lcl");
				NBString_concat(&strUri, absPath);
				if(!NBString_strIsEmpty(query)){
					NBString_concatByte(&strUri, '?');
					NBString_concat(&strUri, query);
				}
				if(!NBString_strIsEmpty(fragment)){
					NBString_concatByte(&strUri, '#');
					NBString_concat(&strUri, fragment);
				}
				if(NBUrl_parse(&obj->cfg.uri, strUri.str)){
					uriIsValid = TRUE;
				} else {
					PRINTF_CONSOLE_ERROR("TSServerStreams, uri is now valid: '%s'.\n", strUri.str);
				}
				NBString_release(&strUri);
			}
			//parse query
			if(uriIsValid){
				SI32 iParam = 0;
				const char* name = NULL; 
				const char* value = NULL;
				while(NBUrl_getQueryAtIndex(&obj->cfg.uri, iParam, &name, &value)){
					//PRINTF_INFO("TSServerStreams, uri param #%d: '%s' = '%s'.\n", (iParam + 1), name, value);
					if(NBString_strIsLike(name, "src")){
						//RunId		: service
						//Group		: cameras-group
						//Device	: camera
						//Version	: stream inside camera
						//Source	: live-view or storage-tail
						STTSServerStreamPath src;
						TSServerStreamPath_init(&src);
						if(!TSServerStreamPath_parserSrc(&src, value)){
							PRINTF_CONSOLE_ERROR("TSServerStreams, clt src not valid: '%s'.\n", value);
							TSServerStreamPath_release(&src);
						} else {
							PRINTF_INFO("TSServerStreams, clt added src run('%s') group('%s') stream('%s') version('%s') source('%s').\n", src.parts.runId, src.parts.groupId, src.parts.streamId, src.parts.versionId, src.parts.sourceId);
							NBArray_addValue(&obj->cfg.srcs, src);
						}
					} else if(NBString_strIsLike(name, "secsPerStream")){
						BOOL rr = FALSE;
						const SI32 secsPerStream = NBNumParser_toSI32(value, &rr);
						if(rr && secsPerStream >= 0){
							if(obj->cfg.params.secsPerStream < secsPerStream){
								obj->cfg.params.secsPerStream = secsPerStream;
								PRINTF_INFO("TSServerStreams, clt secsPerStream set to: %u.\n", secsPerStream);
							}
						}
					} else if(NBString_strIsLike(name, "skipFramesMax")){
						BOOL rr = FALSE;
						const SI32 skipFramesMax = NBNumParser_toSI32(value, &rr);
						if(rr && skipFramesMax >= 0){
							if(obj->cfg.params.skipFramesMax < skipFramesMax){
								obj->cfg.params.skipFramesMax = skipFramesMax;
								PRINTF_INFO("TSServerStreams, clt skipFramesMax set to: %u.\n", skipFramesMax);
							}
						}
                    } else if(NBString_strIsLike(name, "sameProfileOnly")){
                        PRINTF_INFO("TSServerStreams, param '%s' = '%s'.\n", name, value);
                        obj->cfg.params.sameProfileOnly    = !(NBString_strIsEmpty(value) || NBString_strIsLike(value, "0") || NBString_strIsLike(value, "false"));
					} else if(NBString_strIsLike(name, "mp4FullyFrag")){
						PRINTF_INFO("TSServerStreams, param '%s' = '%s'.\n", name, value);
						obj->cfg.params.mp4FullyFrag	= !(NBString_strIsEmpty(value) || NBString_strIsLike(value, "0") || NBString_strIsLike(value, "false")); 
					} else if(NBString_strIsLike(name, "mp4MultiFiles")){
						PRINTF_INFO("TSServerStreams, param '%s' = '%s'.\n", name, value);
						obj->cfg.params.mp4MultiFiles	= !(NBString_strIsEmpty(value) || NBString_strIsLike(value, "0") || NBString_strIsLike(value, "false"));
					} else if(NBString_strIsLike(name, "mp4RealTime")){
						PRINTF_INFO("TSServerStreams, param '%s' = '%s'.\n", name, value);
						obj->cfg.params.mp4RealTime	= !(NBString_strIsEmpty(value) || NBString_strIsLike(value, "0") || NBString_strIsLike(value, "false"));
					}
					//next
					iParam++;
				}
				//result
				if(obj->cfg.srcs.use > 0){
					r = TRUE;
				}
			}
		}
		//Mp4 mode
		/*{
			obj->mp4.isCurFmt = FALSE;
			//init
			{
				const UI32 absPathLen = NBString_strLenBytes(absPath);
				if(absPathLen > 4){
					if(
					   absPath[absPathLen - 4] == '.'
					   && (absPath[absPathLen - 3] == 'm' || absPath[absPathLen - 3] == 'M')
					   && (absPath[absPathLen - 2] == 'p' || absPath[absPathLen - 2] == 'P')
					   && absPath[absPathLen - 1] == '4'
					   )
					{
						obj->mp4.isCurFmt = TRUE;
					}
				}
			}
		}*/
		//set format
		TSStreamBuilder_setFormatAndUnitsPrefix(obj->net.send.queue, /*obj->mp4.isCurFmt ? (obj->cfg.params.mp4FullyFrag ? ENTSStreamBuilderFmt_Mp4FullyFragmented : ENTSStreamBuilderFmt_Mp4Fragmented) :*/ ENTSStreamBuilderFmt_H264Raw, ENTSStreamBuilderUnitsPrefix_0x00000001);
	}
	NBThreadMutex_unlock(&obj->mutex);
	return r;
}

BOOL TSServerStreamReq_prepare(STTSServerStreamReq* obj, const STNBHttpServiceRespRawLnk* rawLnk){
	BOOL r = FALSE;
    if(rawLnk != NULL){
        NBThreadMutex_lock(&obj->mutex);
        //config
        {
            TSStreamBuilder_setMsgsWrapper(obj->net.send.queue, NBWebSocket_isSet(obj->net.websocket) ? ENTSStreamBuilderMessageWrapper_WebSocket : ENTSStreamBuilderMessageWrapper_None);
        }
        //first sync streams with filters
        {
            TSServerStreamReq_sourcesSyncLocked_(obj);
        }
        //stream(s) found send header and data
        if(obj->reqs.arr.use <= 0){
            //obj->cfg.uri.str, does not match any stream url-descriptor
            //PRINTF_CONSOLE_ERROR("TSServerStreamReq, (%llu) no current-stream match for '%s'.\n", (UI64)obj, obj->cfg.uri.str);
        } else {
            BOOL connIsValid = FALSE;
            if(NBWebSocket_isSet(obj->net.websocket)){
                connIsValid = TRUE;
            } else {
                //send http header
                STNBHttpHeader hdr;
                NBHttpHeader_init(&hdr);
                NBHttpHeader_setStatusLine(&hdr, 1, 1, 200, "OK");
                /*if(obj->mp4.isCurFmt){
                    NBHttpHeader_addField(&hdr, "Content-Type", "video/mp4");
                } else
                */
                {
                    NBHttpHeader_addField(&hdr, "Content-Type", "video/h264");
                }
                if(!NBHttpServiceRespRawLnk_sendHeader(rawLnk, &hdr, FALSE /*doNotSendHeaderEnd*/)){
                    PRINTF_CONSOLE_ERROR("TSServerStreamReq, (%llu) NBHttpServiceRespRawLnk_sendHeader failed.\n", (UI64)obj);
                } else {
                    connIsValid = TRUE;
                }
                NBHttpHeader_release(&hdr);
            }
            if(connIsValid){
                obj->net.rawLnk = *rawLnk;
                r = TRUE;
            }
        }
        NBThreadMutex_unlock(&obj->mutex);
    }
	return r;
}

void TSServerStreamReq_stopFlag(STTSServerStreamReq* obj){
	obj->state.stopFlag = TRUE;
}

void TSServerStreamReq_sourcesSyncLocked_(STTSServerStreamReq* obj){
	STTSServerStreams* streams = obj->obj;
	STTSServerOpq* opq		= (STTSServerOpq*)streams->opq;
	const char* runId		= TSContext_getRunId(opq->context.cur);
	BOOL sourcesChanged		= FALSE;
	SI32 i; for(i = 0; i < obj->cfg.srcs.use; i++){
		STTSServerStreamPath* src = NBArray_itmPtrAtIndex(&obj->cfg.srcs, STTSServerStreamPath, i);
		//Add local sources
		if(
		   NBString_strIsEqual("*", src->parts.runId) //all runId (remote and local)
		   || NBString_strIsEqual("_", src->parts.runId) //current local runId
		   || NBString_strIsEqual(runId, src->parts.runId) //explicit current runId
		   )
		{
			//Add live streams
			if(
			   NBString_strIsEmpty(src->parts.sourceId) //default (~ "live")
			   || NBString_strIsEqual("*", src->parts.sourceId) //all sourceId (remote and local)
			   || NBString_strIsEqual("_", src->parts.sourceId) //all local sourceId
			   || NBString_strIsEqual("live", src->parts.sourceId) //explicit "live"
			   )
			{
				SI32 use = 0;
				STTSClientRequesterDeviceRef* arr = TSClientRequester_getGrpsAndLock(opq->streams.reqs.mngr, &use);
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
							const char* streamId	= grpOpq->remoteDesc.streamsGrps[0].streams[0].uid;
							if(
							   //groupId
							   (
								NBString_strIsEqual("*", src->parts.groupId) //all groupId (remote and local)
								|| (NBString_strIsEmpty(src->parts.groupId) || NBString_strIsEqual("_", src->parts.groupId)) //all local groupId
								|| NBString_strIsEqual(groupId, src->parts.groupId) //explicit groupId
								)
							   &&
							   //streamId
							   (
								NBString_strIsEqual("*", src->parts.streamId) //all streamId (remote and local)
								|| (NBString_strIsEmpty(src->parts.streamId) || NBString_strIsEqual("_", src->parts.streamId)) //all local streamId
								|| NBString_strIsEqual(streamId, src->parts.streamId) //explicit streamId
								)
							)
							{
								if(grpOpq->rtsp.reqs.use > 0){
									SI32 i; for(i = 0; i < grpOpq->rtsp.reqs.use; i++){
										STTSClientRequesterConn* req = NBArray_itmValueAtIndex(&grpOpq->rtsp.reqs, STTSClientRequesterConn*, i);
										const char* versionId = req->cfg.versionId;
										NBASSERT(NBString_strIsEqual(req->cfg.groupId, groupId));
										NBASSERT(NBString_strIsEqual(req->cfg.streamId, streamId));
										if(
										   NBString_strIsEqual("*", src->parts.versionId) //all versionId (remote and local)
										   || (NBString_strIsEmpty(src->parts.versionId) || NBString_strIsEqual("_", src->parts.versionId)) //all local versionId
										   || NBString_strIsEqual(versionId, src->parts.versionId) //explicit versionId
										   )
										{
											BOOL fnd = FALSE;
											//search
											{
												SI32 i; for(i = 0; i < obj->reqs.arr.use; i++){
													STTSServerStreamReqConn* reqq = NBArray_itmValueAtIndex(&obj->reqs.arr, STTSServerStreamReqConn*, i);
													if(TSServerStreamReqConn_isConnectedToLive(reqq, req)){
														fnd = TRUE;
														break;
													}
												}
											}
											//Add
											if(!fnd){
												STTSServerStreamReqConn* reqq = NBMemory_allocType(STTSServerStreamReqConn);
												TSServerStreamReqConn_init(reqq);
												if(!TSServerStreamReqConn_conectToLive(reqq, req)){
													PRINTF_CONSOLE_ERROR("TSServerStreamReq, (%llu) could not add live '%s' / '%s' / '%s'.\n", (UI64)obj, req->cfg.groupId, req->cfg.streamId, req->cfg.versionId)
												} else {
													//Consume
													PRINTF_INFO("TSServerStreamReq, (%llu) added live '%s' / '%s' / '%s'.\n", (UI64)obj, req->cfg.groupId, req->cfg.streamId, req->cfg.versionId)
													NBArray_addValue(&obj->reqs.arr, reqq);
													sourcesChanged = TRUE;
													reqq = NULL;
												}
												//Release (if not consumed)
												if(reqq != NULL){
													TSServerStreamReqConn_release(reqq);
													NBMemory_free(reqq);
													reqq = NULL;
												}
											}
										}
									}
								}
							}
						}
					}
				}
				TSClientRequester_returnGrpsAndUnlock(opq->streams.reqs.mngr, arr);
			}
			//Add storage streams
			if(
			   NBString_strIsEqual("*", src->parts.sourceId) //all sourceId
			   || NBString_strIsEqual("storage", src->parts.sourceId) //explicit "storage"
			   )
			{
				SI32 strgStreamIdx = 0;
				STTSStreamStorage* strgStream = TSStreamsStorage_getStreamByIdxRetained(&opq->streams.storage.mngr, strgStreamIdx);
				while(strgStream != NULL){
					const char* streamId = TSStreamStorage_getUid(strgStream);
					if(
					   NBString_strIsEqual("*", src->parts.streamId) //all streamId (remote and local)
					   || (NBString_strIsEmpty(src->parts.streamId) || NBString_strIsEqual("_", src->parts.streamId)) //all local streamId
					   || NBString_strIsEqual(streamId, src->parts.streamId) //explicit streamId
					   ){
						//Filter without groupId
						BOOL isGroupMatch = (
											 NBString_strIsEqual("*", src->parts.groupId) //all groupId (remote and local)
											 || (NBString_strIsEmpty(src->parts.groupId) || NBString_strIsEqual("_", src->parts.groupId)) //all local groupId
											 );
						//Filter with groupId
						if(!isGroupMatch){
							isGroupMatch = TSClientRequester_isRtspReqStreamIdAtGroupId(opq->streams.reqs.mngr, streamId, src->parts.groupId);
						}
						//Process stream
						if(isGroupMatch){
							SI32 verIdx = 0;
							STTSStreamVersionStorage* ver = TSStreamStorage_getVersionByIdxRetained(strgStream, verIdx);
							while(ver != NULL){
								const char* versionId = TSStreamVersionStorage_getUid(ver);
								if(
								   NBString_strIsEqual("*", src->parts.versionId) //all versionId (remote and local)
								   || (NBString_strIsEmpty(src->parts.versionId) || NBString_strIsEqual("_", src->parts.versionId)) //all local versionId
								   || NBString_strIsEqual(versionId, src->parts.versionId) //explicit versionId
								   )
								{
									BOOL fnd = FALSE;
									//search
									{
										SI32 i; for(i = 0; i < obj->reqs.arr.use; i++){
											STTSServerStreamReqConn* reqq = NBArray_itmValueAtIndex(&obj->reqs.arr, STTSServerStreamReqConn*, i);
											if(TSServerStreamReqConn_isConnectedToVersionStorage(reqq, ver)){
												fnd = TRUE;
												break;
											}
										}
									}
									//Add
									if(!fnd){
										STTSServerStreamReqConn* reqq = NBMemory_allocType(STTSServerStreamReqConn);
										TSServerStreamReqConn_init(reqq);
										if(!TSServerStreamReqConn_conectToVersionStorage(reqq, ver)){
											PRINTF_CONSOLE_ERROR("TSServerStreamReq, (%llu) could not add storage '%s' / '%s'.\n", (UI64)obj, streamId, versionId);
										} else {
											//Consume
											PRINTF_INFO("TSServerStreamReq, (%llu) added storage '%s' / '%s'.\n", (UI64)obj, streamId, versionId);
											NBArray_addValue(&obj->reqs.arr, reqq);
											sourcesChanged = TRUE;
											reqq = NULL;
										}
										//Release (if not consumed)
										if(reqq != NULL){
											TSServerStreamReqConn_release(reqq);
											NBMemory_free(reqq);
											reqq = NULL;
										}
									}
								}
								//Next
								verIdx++;
								TSStreamStorage_returnVersion(strgStream, ver);
								ver = TSStreamStorage_getVersionByIdxRetained(strgStream, verIdx);
							}
						}
					}
					//Next
					strgStreamIdx++;
					TSStreamsStorage_returnStream(&opq->streams.storage.mngr, strgStream);
					strgStream = TSStreamsStorage_getStreamByIdxRetained(&opq->streams.storage.mngr, strgStreamIdx);
				}
			}
		} else {
			//ToDo: add remote sources
		}
	}
}

/*BOOL TSServerStreamReq_usesMergedFmtLocked_(STTSServerStreamReq* obj){
    return (obj->mp4.isCurFmt && !obj->cfg.params.mp4MultiFiles) || (!obj->mp4.isCurFmt && obj->cfg.params.sameProfileOnly);
}*/

BOOL TSServerStreamReq_moveToAnyStreamLocked_(STTSServerStreamReq* obj, STTSStreamUnit* dstUnit){
	BOOL r = FALSE;
	//search for any stream that started
	UI32 iFrameSeq = 0; //NBASSERT(!TSServerStreamReq_usesMergedFmtLocked_(obj) || obj->reqs.fmt.mergedFmt != NULL) //merged fmt should be already defined
	SI32 i; for(i = 0; i < obj->reqs.arr.use; i++){
		STTSServerStreamReqConn* req = NBArray_itmValueAtIndex(&obj->reqs.arr, STTSServerStreamReqConn*, i);
		if(TSServerStreamReqConn_getStreamEndedFlag(req)){
			NBASSERT(obj->reqs.cur.req != req)
			//PRINTF_INFO("TSServerStreamReq, (%llu) stream(#%d of %d) ended and removing.\n", (UI64)obj, (i + 1), obj->reqs.arr.use);
			//remove
			NBArray_removeItemAtIndex(&obj->reqs.arr, i);
			TSServerStreamReqConn_release(req);
			NBMemory_free(req);
			req = NULL;
			i--;
		} else if(TSServerStreamReqConn_getStartUnitFromNewestBufferAndFlushOlders(req/*, (TSServerStreamReq_usesMergedFmtLocked_(obj) ? obj->reqs.fmt.mergedFmt NULL : NULL)*/, dstUnit, &iFrameSeq, NULL, NULL)){
			//PRINTF_INFO("TSServerStreamReq, (%llu) stream(#%d of %d) set as current from none (+%d secs).\n", (UI64)obj, (i + 1), obj->reqs.arr.use, obj->reqs.cur.secsChanges);
			//PRINTF_INFO("TSServerStreamReq, (%llu) unit-frst %d(%d) iFrame (lastest frame %d).\n", (UI64)obj, dstUnit->iFrame, dstUnit->hdr.type, iFrameSeq);
			TSServerStreamReq_setCurStreamLocked_(obj, req, iFrameSeq);
			r = TRUE;
			break;
		}
	} NBASSERT((obj->reqs.cur.req == NULL && !r) || (obj->reqs.cur.req != NULL && r))
	return r;
}

BOOL TSServerStreamReq_getNextBestLiveUnitLocked_(STTSServerStreamReq* obj, STTSStreamUnit* dstUnit, BOOL* dstNewCompatibleBlockAvailable){
	BOOL r = FALSE;
	UI32 iFrameSeq = 0;
	BOOL isNewBlockAvailable = FALSE, isNewBlockIsCompatible = FALSE;
    //NBASSERT(!TSServerStreamReq_usesMergedFmtLocked_(obj) || obj->reqs.fmt.mergedFmt != NULL) //merged fmt should be already defined
	if(
	   !TSServerStreamReqConn_getStreamEndedFlag(obj->reqs.cur.req)
	   && TSServerStreamReqConn_getNextBestLiveUnit(obj->reqs.cur.req/*, (TSServerStreamReq_usesMergedFmtLocked_(obj) ? obj->reqs.fmt.mergedFmt NULL : NULL)*/, dstUnit, &iFrameSeq, &isNewBlockAvailable, &isNewBlockIsCompatible)
	   )
	{
		//PRINTF_INFO("TSServerStreamReq, (%llu) unit-fnd %d(%d) iFrame (filter %d/%d) (lastest frame %d).\n", (UI64)obj, dstUnit->iFrame, dstUnit->hdr.type, iFrameFilter, obj->reqs.cur.skipFramesMax, iFrameSeq);
		r = TRUE;
	} else {
		//PRINTF_INFO("TSServerStreamReq, (%llu) unit-not-found (filter %d) (lastest frame %d).\n", (UI64)obj, iFrameFilter, iFrameSeq);
	}
	if(dstNewCompatibleBlockAvailable != NULL){
		*dstNewCompatibleBlockAvailable = (isNewBlockAvailable && isNewBlockIsCompatible);
	}
	return r;
}

void TSServerStreamReq_setCurStreamLocked_(STTSServerStreamReq* obj, STTSServerStreamReqConn* conn, const UI32 iFrameSeq){
	//PRINTF_INFO("TSServerStreamReq, (%llu) setCurStreamLocked (%llu) iFrame(%d).\n", (UI64)obj, (UI64)conn, iFrameSeq);
	obj->reqs.cur.secsChanges	= 0;
	obj->reqs.cur.req			= conn;
	obj->reqs.cur.changeFlagsApplied = FALSE;
}

//execute

void TSServerStreamReq_httpReqTick(STTSServerStreamReq* obj, const STNBTimestampMicro timeMicro, const UI64 msCurTick, const UI32 msNextTick, UI32* dstMsNextTick){
	const BOOL secChanged = (obj->state.tick.last.timestamp != timeMicro.timestamp);
	UI32 msNextTickNew = 1, unitsFoundCount = 0, unitsSentCount = 0;
	NBThreadMutex_lock(&obj->mutex);
	//tick
	{
		BOOL availUnitFound = TRUE, frameUnitFound = FALSE;
        //define merged stream fmt (if not defined yet)
        /*if(TSServerStreamReq_usesMergedFmtLocked_(obj) && obj->reqs.fmt.mergedFmt == NULL){
            obj->reqs.fmt.msWaitAllFmtsRemain -= (obj->reqs.fmt.msWaitAllFmtsRemain < msCurTick ? obj->reqs.fmt.msWaitAllFmtsRemain : msCurTick);
            //analyze current streams fmts and define merged-fmt
            if(!obj->reqs.fmt.analyzedAtCurSec){
                obj->reqs.fmt.analyzedAtCurSec = TRUE;
                //analyze
                if(obj->reqs.arr.use > 0){
                    BOOL anyFmtMissing = FALSE;
                    STNBArray fmts;
                    NBArray_initWithSz(&fmts, sizeof(STNBAvcPicDescBase), NULL, obj->reqs.arr.use, 16, 0.1f);
                    {
                        SI32 i; for(i = 0; i < obj->reqs.arr.use; i++){
                            STTSServerStreamReqConn* req = NBArray_itmValueAtIndex(&obj->reqs.arr, STTSServerStreamReqConn*, i);
                            STNBAvcPicDescBase fmtDesc;
                            NBMemory_setZero(fmtDesc);
                            if(TSServerStreamReqConn_getStartUnitFmtFromNewestBuffer(req, &fmtDesc)){
                                NBArray_addValue(&fmts, fmtDesc);
                            } else {
                                NBStruct_stRelease(NBAvcPicDescBase_getSharedStructMap(), &fmtDesc, sizeof(fmtDesc));
                                anyFmtMissing = TRUE;
                                //stop analyzing if will be analyzed again
                                if(obj->reqs.fmt.msWaitAllFmtsRemain > 0){
                                    break;
                                }
                            }
                        }
                    }
                    //analyze
                    if(obj->reqs.arr.use > 0 && (!anyFmtMissing / *all-found* / || obj->reqs.fmt.msWaitAllFmtsRemain == 0 / *wait-timedout* /)){
                        STNBAvcPicDescBase mergedFmt;
                        NBMemory_setZeroSt(mergedFmt, STNBAvcPicDescBase);
                        //define the img size
                        {
                            STNBArray arrCountsByImgSz; //ammount of frames with the same image size
                            NBArray_init(&arrCountsByImgSz, sizeof(SI32), NULL);
                            //populate
                            {
                                SI32 i; for(i = 0; i < fmts.use; i++){
                                    const STNBAvcPicDescBase* desc = NBArray_itmPtrAtIndex(&fmts, STNBAvcPicDescBase, i);
                                    SI32 repeatedImgSz = 1; //1 is mine
                                    //count others
                                    {
                                        SI32 i2; for(i2 = i + 1; i2 < fmts.use; i2++){
                                            const STNBAvcPicDescBase* desc2 = NBArray_itmPtrAtIndex(&fmts, STNBAvcPicDescBase, i2);
                                            if(desc->w == desc2->w && desc->h == desc2->h){
                                                repeatedImgSz++;
                                            }
                                        }
                                    }
                                    //add count
                                    NBArray_addValue(&arrCountsByImgSz, repeatedImgSz);
                                }
                            }
                            //find the most repeated img sz
                            {
                                NBASSERT(arrCountsByImgSz.use == fmts.use)
                                SI32 repeatCurMax = 0;
                                SI32 i; for(i = 0; i < arrCountsByImgSz.use; i++){
                                    const SI32 repeatCount = NBArray_itmValueAtIndex(&arrCountsByImgSz, SI32, i);
                                    if(repeatCurMax < repeatCount){
                                        const STNBAvcPicDescBase* desc = NBArray_itmPtrAtIndex(&fmts, STNBAvcPicDescBase, i);
                                        //set as current img-sz
                                        mergedFmt.w = desc->w;
                                        mergedFmt.h = desc->h; //NOTE: luma height could missmatch by few pixels that are cropped in the final image
                                        repeatCurMax = repeatCount;
                                    }
                                }
                            }
                            NBArray_empty(&arrCountsByImgSz);
                            NBArray_release(&arrCountsByImgSz);
                        }
                        //find the highest AVC profile
                        if(mergedFmt.w > 0 && mergedFmt.h > 0){ //NOTE: luma height could missmatch by few pixels that are cropped in the final image
                            SI32 i; for(i = 0; i < fmts.use; i++){
                                const STNBAvcPicDescBase* desc = NBArray_itmPtrAtIndex(&fmts, STNBAvcPicDescBase, i);
                                if(mergedFmt.w == desc->w && mergedFmt.h == desc->h){ //NOTE: luma height could missmatch by few pixels that are cropped in the final image
                                    if(!NBAvcPicDescBase_isCompatibleProfile(&mergedFmt, desc)){
                                        NBStruct_stRelease(NBAvcPicDescBase_getSharedStructMap(), &mergedFmt, sizeof(mergedFmt));
                                        NBStruct_stClone(NBAvcPicDescBase_getSharedStructMap(), desc, sizeof(*desc), &mergedFmt, sizeof(mergedFmt));
                                    }
                                    //ToDo: remove
                                    / *if(mergedFmt.avcProfile < desc->avcProfile){
                                        //set as current highest profile
                                        mergedFmt.avcProfile = desc->avcProfile;
                                        mergedFmt.profConstraints = 0; //desc->profConstraints; //TMP, disable al contrainsts.
                                        mergedFmt.profLevel = desc->profLevel;
                                    } else if(mergedFmt.avcProfile == desc->avcProfile){
                                        //disable contrains flags
                                        mergedFmt.profConstraints &= desc->profConstraints;
                                        //define higher level
                                        if(mergedFmt.profLevel < desc->profLevel){
                                            mergedFmt.profLevel = desc->profLevel;
                                        }
                                    }* /
                                }
                            }
                            //set as current profile
                            {
                                if(obj->reqs.fmt.mergedFmt != NULL){
                                    NBStruct_stRelease(NBAvcPicDescBase_getSharedStructMap(), obj->reqs.fmt.mergedFmt, sizeof(*obj->reqs.fmt.mergedFmt));
                                    NBMemory_free(obj->reqs.fmt.mergedFmt);
                                    obj->reqs.fmt.mergedFmt = NULL;
                                }
                                obj->reqs.fmt.msWaitAllFmtsRemain = 0; //stop waiting
                                obj->reqs.fmt.mergedFmt = NBMemory_allocType(STNBAvcPicDescBase);
                                NBMemory_set(obj->reqs.fmt.mergedFmt, 0, sizeof(STNBAvcPicDescBase));
                                NBStruct_stClone(NBAvcPicDescBase_getSharedStructMap(), &mergedFmt, sizeof(mergedFmt), obj->reqs.fmt.mergedFmt, sizeof(*obj->reqs.fmt.mergedFmt));
                                //prinf profile
                                {
                                    SI32 compatibleCount = 0;
                                    SI32 i; for(i = 0; i < fmts.use; i++){
                                        const STNBAvcPicDescBase* desc = NBArray_itmPtrAtIndex(&fmts, STNBAvcPicDescBase, i);
                                        if(NBAvcPicDescBase_isCompatible(obj->reqs.fmt.mergedFmt, desc)){
                                            compatibleCount++;
                                        }
                                    } NBASSERT(compatibleCount > 0) //program logic error, at least one stream should be found as compatible.
                                    PRINTF_INFO("TSServerStreamReq, (%llu) merged-format defined as for %d of %d streams: %u x %u avc-profile(%u, contraints:%u, level:%u).\n", (UI64)obj, compatibleCount, fmts.use, obj->reqs.fmt.mergedFmt->w, obj->reqs.fmt.mergedFmt->h, obj->reqs.fmt.mergedFmt->avcProfile, obj->reqs.fmt.mergedFmt->profConstraints, obj->reqs.fmt.mergedFmt->profLevel);
                                }
                            }
                        }
                        //release
                        NBStruct_stRelease(NBAvcPicDescBase_getSharedStructMap(), &mergedFmt, sizeof(mergedFmt));
                    }
                    //release
                    {
                        SI32 i; for(i = 0; i < fmts.use; i++){
                            STNBAvcPicDescBase* desc = NBArray_itmPtrAtIndex(&fmts, STNBAvcPicDescBase, i);
                            NBStruct_stRelease(NBAvcPicDescBase_getSharedStructMap(), desc, sizeof(*desc));
                        }
                        NBArray_empty(&fmts);
                        NBArray_release(&fmts);
                    }
                } //if(obj->reqs.arr.use > 0)
                //info
                if(obj->reqs.fmt.mergedFmt == NULL){
                    PRINTF_INFO("TSServerStreamReq, (%llu) waiting for formats to define this stream merged-format.\n", (UI64)obj);
                }
            }
        }*/
        {
            //accumulate time since last pic
            if(obj->state.stream.frames.count != 0){
                obj->state.stream.frames.msWaited += msCurTick;
                obj->state.stream.playTime.msReal += msCurTick;
            }
            //process units
            while(availUnitFound && !frameUnitFound && !obj->state.stopFlag && !obj->state.netErr && obj->reqs.arr.use > 0){
                BOOL nextStreamNewBlockAvailable = FALSE, curStreamNewBlockAvailable = FALSE;
                STTSStreamUnit unitFnd; UI32 msStreamVirtualToAdd = 0;
                TSStreamUnit_init(&unitFnd);
                //reset flags
                availUnitFound = frameUnitFound = FALSE;
                //activate any stream (if none is active)
                if(
                   !obj->state.stopFlag && unitFnd.desc == NULL
                   && obj->reqs.cur.req == NULL && obj->reqs.arr.use > 0
                   )
                {
                    if(TSServerStreamReq_moveToAnyStreamLocked_(obj, &unitFnd)){
                        PRINTF_INFO("TSServerStreamReq, (%llu) moved to any-stream.\n", (UI64)obj);
                        //PRINTF_INFO("TSServerStreamReq, (%llu) moved to any-stream (%ux%u@%ufps).\n", (UI64)obj, unitFnd.desc->picProps.w, unitFnd.desc->picProps.h, unitFnd.desc->picProps.fpsMax);
                        NBASSERT(unitFnd.desc != NULL)
                    }
                }
                //search for unit (if not set yet and no new block is available)
                if(
                   !obj->state.stopFlag && unitFnd.desc == NULL
                   && obj->reqs.cur.req != NULL
                   )
                {
                    if(TSServerStreamReq_getNextBestLiveUnitLocked_(obj, &unitFnd, &curStreamNewBlockAvailable)){
                        //PRINTF_INFO("TSServerStreamReq, (%llu) moved to next-unit type(%d, %ux%u@%ufps).\n", (UI64)obj, unitFnd.desc->hdr.type, unitFnd.desc->picProps->w, unitFnd.desc->picProps->h, unitFnd.desc->picProps->fpsMax);
                        NBASSERT(unitFnd.desc != NULL)
                    }
                }
                //set populated flag
                if(unitFnd.desc != NULL){
                    availUnitFound = TRUE;
                    unitsFoundCount++;
                }
                //discard unit if new block is available and this unit is not the start
                if(
                   !obj->state.stopFlag && unitFnd.desc != NULL
                   && (nextStreamNewBlockAvailable || curStreamNewBlockAvailable)
                   && !unitFnd.desc->isBlockStart
                   )
                {
                    //PRINTF_INFO("TSServerStreamReq, (%llu) unitId(%u) skipped type(%d, %ux%u@%ufps) [new block available].\n", (UI64)obj, unitFnd.desc->unitId, unitFnd.desc->hdr.type, unitFnd.desc->picProps->w, unitFnd.desc->picProps->h, unitFnd.desc->picProps->fpsMax);
                    //add to garbage collection
                    {
                        NBASSERT(unitFnd.desc != NULL)
                        //PRINTF_INFO("TSServerStreamReq, (%llu) unit-desc(%llu) added to req-garbage at cycle-unused.\n", (UI64)obj, (SI64)unitFnd.desc);
                        NBASSERT(NBArray_indexOf(&obj->garbage.arr, &unitFnd, sizeof(unitFnd)) < 0) //unit already in garbage array
                        NBArray_addValue(&obj->garbage.arr, unitFnd);
                        TSStreamUnit_resignToData(&unitFnd);
                    }
                    TSStreamUnit_init(&unitFnd);
                    NBASSERT(unitFnd.desc == NULL)
                }
                //count and analyze unit
                if(unitFnd.desc != NULL){
                    //frame-payload
                    if(unitFnd.desc->isFramePayload){
                        //const SI64 usecsFrame = NBTimestampMicro_getDiffInUs(&obj->state.stream.frames.lastTime, &timeMicro);
                        //const SI64 usecsUnit = NBTimestampMicro_getDiffInUs(&obj->state.stream.units.lastTime, &timeMicro);
                        //PRINTF_INFO("TSServerStreamReq, (%llu) frame-type(%d) after %d.%d%dms (%d.%d%dms since prev unit).\n", (UI64)obj, unitFnd.desc->hdr.type, (usecsFrame / 1000ULL), ((usecsFrame % 1000) / 100), ((usecsFrame % 100) / 10), (usecsUnit / 1000ULL), ((usecsUnit % 1000) / 100), ((usecsUnit % 100) / 10));
                        //accum
                        obj->state.curSec.frames++;
                        obj->state.stream.frames.count++;
                        obj->state.stream.frames.lastTime = timeMicro;
                        //calculate next frame time
                        /*if(unitFnd.desc->picProps.fpsMax > 0){
                            const UI32 fpsVirtual	= (obj->cfg.params.skipFramesMax < unitFnd.desc->picProps.fpsMax ? unitFnd.desc->picProps.fpsMax - obj->cfg.params.skipFramesMax : 1);
                            const UI32 msPerFrameR	= (1000 / unitFnd.desc->picProps.fpsMax);
                            const UI32 msPerFrameV	= (1000 / fpsVirtual);
                            const UI32 msWaitToCsm	= ((obj->state.stream.frames.msWaited / msPerFrameV) * msPerFrameV);
                            //Reduce ms to take
                            NBASSERT(obj->state.stream.frames.msWaited >= msWaitToCsm)
                            obj->state.stream.frames.msWaited -= msWaitToCsm;
                            //Calculate next tick-time
                            NBASSERT(obj->state.stream.frames.msWaited < msPerFrameV)
                            msNextTickNew			= (msPerFrameV - obj->state.stream.frames.msWaited);
                            //set stream-ms-timestamp to advance
                            msStreamVirtualToAdd	= (msWaitToCsm < msPerFrameR ? msWaitToCsm : msPerFrameR);
                        }*/
                        frameUnitFound = TRUE;
                    } else {
                        //const SI64 usecsUnit = NBTimestampMicro_getDiffInUs(&obj->state.stream.units.lastTime, &timeMicro);
                        //PRINTF_INFO("TSServerStreamReq, (%llu) unit-type(%d) after %d.%d%dms.\n", (UI64)obj, unitFnd.desc->hdr.type, (usecsUnit / 1000ULL), ((usecsUnit % 1000) / 100), ((usecsUnit % 100) / 10));
                    }
                    obj->state.curSec.units++;
                    obj->state.stream.units.count++;
                    obj->state.stream.units.lastTime = timeMicro;
                }
                //send
                if(unitFnd.desc != NULL){
                    //send (unlocked)
                    //NBASSERT(!TSServerStreamReq_usesMergedFmtLocked_(obj) || obj->reqs.fmt.mergedFmt != NULL) //merged fmt should be already defined
                    NBThreadMutex_unlock(&obj->mutex);
                    //accumulate unit
                    unitsSentCount++;
                    //build and send output
                    {
                        NBASSERT(unitFnd.desc != NULL) //released reference given as param
                        {
                            STTSStreamUnitsReleaser unitsReleaser;
                            NBMemory_setZeroSt(unitsReleaser, STTSStreamUnitsReleaser);
                            unitsReleaser.itf.unitsReleaseGrouped = TSServerStreamReq_unitsReleaseGrouped_;
                            unitsReleaser.usrData = obj;
                            //PRINTF_INFO("TSServerStreamReq, (%llu) queing unit type(%d) unitId(%u).\n", (UI64)obj, unitFnd.desc->hdr.type, unitFnd.desc->unitId);
                            TSStreamBuilder_httpReqSendUnitOwning(obj->net.send.queue, msStreamVirtualToAdd, &unitFnd, obj->net.rawLnk, &unitsReleaser);
                        }
                        //release
                        if(unitFnd.desc != NULL){
                            //PRINTF_INFO("TSServerStreamReq, (%llu) unit-desc(%llu) added to req-garbage at cycle-send.\n", (UI64)obj, (SI64)unitFnd.desc);
                            //add to garbage collection
                            NBASSERT(NBArray_indexOf(&obj->garbage.arr, &unitFnd, sizeof(unitFnd)) < 0) //unit already in garbage array
                            NBArray_addValue(&obj->garbage.arr, unitFnd);
                            TSStreamUnit_resignToData(&unitFnd);
                        } else {
                            //release
                            TSStreamUnit_release(&unitFnd, NULL);
                        }
                    }
                    NBASSERT(unitFnd.desc == NULL) //should be released
                    NBThreadMutex_lock(&obj->mutex);
                } NBASSERT(unitFnd.desc == NULL) //should be released
                //accumulate stream virtual
                {
                    obj->state.stream.playTime.msVirtual += msStreamVirtualToAdd;
                    NBASSERT(obj->state.stream.playTime.msVirtual <= obj->state.stream.playTime.msReal)
                }
                //remove current stream (if ended)
                if(obj->reqs.cur.req != NULL && TSServerStreamReqConn_getStreamEndedFlag(obj->reqs.cur.req)){
                    BOOL fnd = FALSE;
                    SI32 i; for(i = 0; i < obj->reqs.arr.use; i++){
                        STTSServerStreamReqConn* req = NBArray_itmValueAtIndex(&obj->reqs.arr, STTSServerStreamReqConn*, i);
                        if(obj->reqs.cur.req == req){
                            PRINTF_INFO("TSServerStreamReq, (%llu) stream(#%d of %d) (current) ended and removing.\n", (UI64)obj, (i + 1), obj->reqs.arr.use);
                            NBArray_removeItemAtIndex(&obj->reqs.arr, i);
                            TSServerStreamReqConn_release(req);
                            NBMemory_free(req);
                            req = NULL;
                            fnd = TRUE;
                            break;
                        }
                    } NBASSERT(fnd) //should be found
                    //set current
                    if(fnd){
                        TSServerStreamReq_setCurStreamLocked_(obj, NULL, 0);
                    }
                    NBASSERT(obj->reqs.cur.req == NULL) //should be found
                }
                //release unit
                if(unitFnd.desc != NULL){
                    //PRINTF_INFO("TSServerStreamReq, (%llu) unit-desc(%llu) added to req-garbage at cycle-end.\n", (UI64)obj, (SI64)unitFnd.desc);
                    //add to garbage collection
                    NBASSERT(NBArray_indexOf(&obj->garbage.arr, &unitFnd, sizeof(unitFnd)) < 0) //unit already in garbage array
                    NBArray_addValue(&obj->garbage.arr, unitFnd);
                    TSStreamUnit_resignToData(&unitFnd);
                } else {
                    //release
                    TSStreamUnit_release(&unitFnd, NULL);
                }
            } //while
        }
	}
	//second changed
	if(secChanged){
		//sec-changed
		{
			//analyze sources without units
			STTSServerStreamReqConn** req = NBArray_dataPtr(&obj->reqs.arr, STTSServerStreamReqConn*);
			STTSServerStreamReqConn** reqAfterEnd = req + obj->reqs.arr.use;
			while(req < reqAfterEnd){
				UI32 iFrameSeq = 0;
				TSServerStreamReqConn_tickSecChanged(*req, &iFrameSeq);
				//next
				req++;
			}
			//secs current source
			obj->reqs.cur.secsChanges++;
            //trigger merged-fmt analysis action
            //obj->reqs.fmt.analyzedAtCurSec = FALSE;
			//per-sec stats
			/*{
				NBASSERT(obj->reqs.cur.secsChanges > 0)
				PRINTF_INFO("TSServerStreamReq, (%llu) curSec(%u frames, %u units); avg(%u frames, %u units); total(%d frames, %d units, %d secs); playTime virtual(%llu.%llu%llus) real(%llu.%llu%llus) delay(%llu.%llu%llus).\n"
                            , (UI64)obj
							, obj->state.curSec.frames, obj->state.curSec.units
							, (obj->state.stream.frames.count / obj->reqs.cur.secsChanges), (obj->state.stream.units.count / obj->reqs.cur.secsChanges)
							, obj->state.stream.frames.count, obj->state.stream.units.count, obj->reqs.cur.secsChanges
							, obj->state.stream.playTime.msVirtual / 1000ULL, (obj->state.stream.playTime.msVirtual % 1000ULL) / 100ULL, (obj->state.stream.playTime.msVirtual % 100ULL) / 10ULL
							, obj->state.stream.playTime.msReal / 1000ULL, (obj->state.stream.playTime.msReal % 1000ULL) / 100ULL, (obj->state.stream.playTime.msReal % 100ULL) / 10ULL
							, (obj->state.stream.playTime.msReal - obj->state.stream.playTime.msVirtual) / 1000ULL, ((obj->state.stream.playTime.msReal - obj->state.stream.playTime.msVirtual) % 1000ULL) / 100ULL, ((obj->state.stream.playTime.msReal - obj->state.stream.playTime.msVirtual) % 100ULL) / 10ULL
							);
				NBMemory_setZero(obj->state.curSec);
			}*/
		}
		//garbage-return
		if(obj->garbage.arr.use != 0){
			//IF_PRINTF(const SI32 useStart = obj->garbage.arr.use;)
			SI32 i; for(i = 0; i < obj->reqs.arr.use && obj->garbage.arr.use != 0; i++){
				STTSServerStreamReqConn* req = NBArray_itmValueAtIndex(&obj->reqs.arr, STTSServerStreamReqConn*, i);
				TSServerStreamReqConn_consumeGarbageUnits(req, &obj->garbage.arr);
			}
			//PRINTF_INFO("TSServerStreamReq, (%llu) %d units returned to conn, %d released as orphan.\n", (UI64)obj, (useStart - obj->garbage.arr.use), obj->garbage.arr.use);
			if(obj->garbage.arr.use > 0){
				STTSStreamUnit* u = NBArray_dataPtr(&obj->garbage.arr, STTSStreamUnit);
				TSStreamUnit_unitsReleaseGrouped(u, obj->garbage.arr.use, NULL);
				NBArray_empty(&obj->garbage.arr);
			}
		}
	}
	//set time
	/*{
		const SI64 usDiff = NBTimestampMicro_getDiffInUs(&obj->state.time.lastMicro, &timeMicro);
		PRINTF_INFO("TSServerStreamReq, (%llu) %d.%d%dms tick (%d units found, %d sent, %dms waiting for next).\n", (UI64)obj, (usDiff / 1000), ((usDiff % 1000) / 100), ((usDiff % 100) / 10), unitsFoundCount, unitsSentCount, obj->state.stream.frames.msWaited);
	}*/
	obj->state.tick.last = timeMicro;
	NBThreadMutex_unlock(&obj->mutex);
	//set wait-for-next-tick
	if(dstMsNextTick != NULL){
		*dstMsNextTick = msNextTickNew;
	}
	//PRINTF_INFO("TSServerStreamReq, (%llu) next-tick in %dms.\n", (UI64)obj, msNextTickNew);
}

//garbage

void TSServerStreamReq_unitsReleaseGrouped_(STTSStreamUnit* objs, const UI32 size, STNBDataPtrReleaser* optPtrsReleaser, void* usrData){
	STTSServerStreamReq* obj = (STTSServerStreamReq*)usrData;
	const STTSStreamUnit* objsAfterEnd = objs + size;
	while(objs < objsAfterEnd){
		NBASSERT(objs->desc != NULL) //should be filled
		//release unit
		if(objs->desc != NULL){
			//PRINTF_INFO("TSServerStreamReq, (%llu) unit-desc(%llu) added to req-garbage by callback.\n", (UI64)obj, (SI64)objs->desc);
			//add to garbage collection
			NBASSERT(NBArray_indexOf(&obj->garbage.arr, objs, sizeof(*objs)) < 0) //unit already in garbage array
			NBArray_addValue(&obj->garbage.arr, *objs);
			TSStreamUnit_resignToData(objs);
		} else {
			//release
			TSStreamUnit_release(objs, optPtrsReleaser);
		}
		//next
		objs++;
	}
}

