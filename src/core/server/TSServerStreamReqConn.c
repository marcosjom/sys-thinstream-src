//
//  TSServerStreamReqConn.c
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#include "nb/NBFrameworkPch.h"
#include "nb/core/NBStruct.h"
#include "core/server/TSServerStreamReqConn.h"

//lstnr
UI32 TSServerStreamReqConn_streamConsumeUnits_(void* obj, STTSStreamUnit* units, const UI32 unitsSz, STTSStreamUnitsReleaser* optUnitsReleaser);
void TSServerStreamReqConn_streamEnded_(void* obj);

//

void TSServerStreamReqConn_init(STTSServerStreamReqConn* obj){
	NBMemory_setZeroSt(*obj, STTSServerStreamReqConn);
	NBThreadMutex_init(&obj->mutex);
	//seqHdrs
	{
		//missed
		{
			//
		}
	}
	//buffs
	{
		SI32 i; const SI32 count = (sizeof(obj->buffs) / sizeof(obj->buffs[0]));
		for(i = 0; i < count; i++){
			STNBArray* arr = &obj->buffs[i];
			NBArray_init(arr, sizeof(STTSStreamUnit), NULL);
		}
	}
	//garbage
	{
		NBArray_init(&obj->garbage.pend, sizeof(STTSStreamUnit), NULL);
	}
}

void TSServerStreamReqConn_release(STTSServerStreamReqConn* obj){
	NBThreadMutex_lock(&obj->mutex);
	{
		//live
		if(obj->live != NULL){
			TSClientRequesterConn_removeViewer(obj->live, obj, NULL);
			obj->live = NULL;
		}
		//storage
		if(obj->storage != NULL){
			TSStreamVersionStorage_removeViewer(obj->storage, obj, NULL);
			obj->storage = NULL;
		}
		//garbage
		{
			if(obj->garbage.pend.use > 0){
				STTSStreamUnit* u = NBArray_dataPtr(&obj->garbage.pend, STTSStreamUnit);
				TSStreamUnit_unitsReleaseGrouped(u, obj->garbage.pend.use, NULL);
			}
			NBArray_empty(&obj->garbage.pend);
			NBArray_release(&obj->garbage.pend);
		}
		//buffs
		{
			SI32 i; const SI32 count = (sizeof(obj->buffs) / sizeof(obj->buffs[0]));
			for(i = 0; i < count; i++){
				STNBArray* arr = &obj->buffs[i];
				if(arr->use > 0){
					STTSStreamUnit* u = NBArray_dataPtr(arr, STTSStreamUnit);
					TSStreamUnit_unitsReleaseGrouped(u, arr->use, NULL);
				}
				NBArray_empty(arr);
				NBArray_release(arr);
				NBMemory_setZeroSt(*arr, STNBArray);
			}
		}
		//seqHdrs
		{
			//missed
			{
				//
			}
		}
	}
	NBThreadMutex_unlock(&obj->mutex);
	NBThreadMutex_release(&obj->mutex);
}

//

BOOL TSServerStreamReqConn_isConnectedToLive(STTSServerStreamReqConn* obj, STTSClientRequesterConn* conn){
	//unlocked (quick)
	return (obj->live == conn);
}


BOOL TSServerStreamReqConn_isConnectedToVersionStorage(STTSServerStreamReqConn* obj, STTSStreamVersionStorage* conn){
	//unlocked (quick)
	return (obj->storage == conn);
}

BOOL TSServerStreamReqConn_conectToLive(STTSServerStreamReqConn* obj, STTSClientRequesterConn* conn){
	BOOL r = FALSE;
	NBThreadMutex_lock(&obj->mutex);
	{
		//remove from current
		if(obj->live != NULL){
			TSClientRequesterConn_removeViewer(obj->live, obj, NULL);
			obj->live = NULL;
		}
		//add to new
		if(conn != NULL){
			STTSStreamLstnr lstnr;
			NBMemory_setZeroSt(lstnr, STTSStreamLstnr);
			lstnr.param					= obj;
			lstnr.streamConsumeUnits	= TSServerStreamReqConn_streamConsumeUnits_;
			lstnr.streamEnded			= TSServerStreamReqConn_streamEnded_;
			r = TSClientRequesterConn_addViewer(conn, &lstnr, NULL);
		} else {
			r = TRUE;
		}
		obj->live = conn;
	}
	NBThreadMutex_unlock(&obj->mutex);
	return r;
}

BOOL TSServerStreamReqConn_conectToVersionStorage(STTSServerStreamReqConn* obj, STTSStreamVersionStorage* conn){
	BOOL r = FALSE;
	NBThreadMutex_lock(&obj->mutex);
	{
		//remove from current
		if(obj->storage != NULL){
			TSStreamVersionStorage_removeViewer(obj->storage, obj, NULL);
			obj->storage = NULL;
		}
		//add to new
		if(conn != NULL){
			STTSStreamLstnr lstnr;
			NBMemory_setZeroSt(lstnr, STTSStreamLstnr);
			lstnr.param					= obj;
			lstnr.streamConsumeUnits	= TSServerStreamReqConn_streamConsumeUnits_;
			lstnr.streamEnded			= TSServerStreamReqConn_streamEnded_;
			r = TSStreamVersionStorage_addViewer(conn, &lstnr, NULL);
		} else {
			r = TRUE;
		}
		obj->storage = conn;
	}
	NBThreadMutex_unlock(&obj->mutex);
	return r;
}

UI32 TSServerStreamReqConn_secsChangesWithoutUnits(STTSServerStreamReqConn* obj){
	//unlocked (quick)
	return obj->state.rcv.secsChangesNoUnits;
}

BOOL TSServerStreamReqConn_getStreamEndedFlag(STTSServerStreamReqConn* obj){
	//unlocked (quick)
	return obj->state.rcv.ended;
}

void TSServerStreamReqConn_tickSecChanged(STTSServerStreamReqConn* obj, UI32* dstFrameSeq){
	NBThreadMutex_lock(&obj->mutex);
	{
		if(dstFrameSeq != NULL){
			*dstFrameSeq = obj->state.rcv.iFrameSeq;
		}
		obj->state.rcv.secsChangesNoUnits++;
	}
	NBThreadMutex_unlock(&obj->mutex);
}

BOOL TSServerStreamReqConn_getStartUnitFromNewestBufferAndFlushOldersLocked_(STTSServerStreamReqConn* obj/*, const STNBAvcPicDescBase* optRequiredDescBase*/, STTSStreamUnit* dst, BOOL* dstNewBlockAvailable, BOOL* dstNewBlockIsCompatible){
	BOOL r = FALSE; BOOL hasNewBlock = FALSE, hasNewFrame = FALSE, newBlockIsCompatible = FALSE;
	{
		const SI32 count = (sizeof(obj->buffs) / sizeof(obj->buffs[0]));
		STNBArray* newestBuff = &obj->buffs[count - 1]; 
		if(newestBuff->use > 0){
			STTSStreamUnit* u = NBArray_itmPtrAtIndex(newestBuff, STTSStreamUnit, 0);
			if(u->desc->isBlockStart){
				hasNewBlock = TRUE;
				hasNewFrame = (u->desc->isKeyframe);
				//count extra frames
				{
					SI32 i2; for(i2 = 1; i2 < newestBuff->use; i2++){
						STTSStreamUnit* u2 = NBArray_itmPtrAtIndex(newestBuff, STTSStreamUnit, i2);
						if(u2->desc->isFramePayload){
							hasNewFrame = TRUE;
							break;
						}
					}
				}
				//apply
				if(hasNewFrame){
					newBlockIsCompatible = TRUE;
					/*if(optRequiredDescBase != NULL){
                        newBlockIsCompatible = NBAvcPicDescBase_isCompatible(optRequiredDescBase, &u->desc->picProps);
						/ *if(!newBlockIsCompatible){
							PRINTF_WARNING("TSServerStreamReqConn, start-frame found is not compatible.\n");
						}* /
					}*/
					if(newBlockIsCompatible){
						//flush older buffers
						{
							SI32 i;  for(i = 0; i < (count - 1); i++){
								STNBArray* arr = &obj->buffs[i];
								if(arr->use > 0){
									STTSStreamUnit* u = NBArray_dataPtr(arr, STTSStreamUnit);
									NBArray_addItems(&obj->garbage.pend, u, sizeof(*u), arr->use);
								}
								NBArray_empty(arr);
							}
						}
						//set results
						if(dst != NULL){
							*dst = *u;
							obj->garbage.balance++; //add to garbage balance
							if(u->desc != NULL && u->desc->origin != NULL){
								NBASSERT(obj->garbage.origin.last == NULL || obj->garbage.origin.last == u->desc->origin)
								obj->garbage.origin.last = u->desc->origin;
								//PRINTF_INFO("TSServerStreamReqConn, ReqConn(%lld) setting garbage-unit from origin(%lld).\n", (SI64)obj, (SI64)u->desc->origin);
							}
							NBArray_removeItemAtIndex(newestBuff, 0);
						}
						r = TRUE;
					}
				}
			}
		}
	}
	if(dstNewBlockAvailable != NULL){
		*dstNewBlockAvailable = hasNewBlock;
	}
	if(dstNewBlockIsCompatible != NULL){
		*dstNewBlockIsCompatible = newBlockIsCompatible;
	}
	return r;
}

/*
BOOL TSServerStreamReqConn_getStartUnitFmtFromNewestBuffer(STTSServerStreamReqConn* obj, STNBAvcPicDescBase* dst){
    BOOL r = FALSE;
    BOOL hasNewBlock = FALSE, hasNewFrame = FALSE;
    NBThreadMutex_lock(&obj->mutex);
    {
        const SI32 count = (sizeof(obj->buffs) / sizeof(obj->buffs[0]));
        STNBArray* newestBuff = &obj->buffs[count - 1];
        if(newestBuff->use > 0){
            STTSStreamUnit* u = NBArray_itmPtrAtIndex(newestBuff, STTSStreamUnit, 0);
            if(u->desc->isBlockStart){
                hasNewBlock = TRUE;
                hasNewFrame = (u->desc->isKeyframe);
                //count extra frames
                {
                    SI32 i2; for(i2 = 1; i2 < newestBuff->use; i2++){
                        STTSStreamUnit* u2 = NBArray_itmPtrAtIndex(newestBuff, STTSStreamUnit, i2);
                        if(u2->desc->isFramePayload){
                            hasNewFrame = TRUE;
                            break;
                        }
                    }
                }
                //apply
                if(hasNewFrame){
                    if(dst != NULL){
                        NBStruct_stRelease(NBAvcPicDescBase_getSharedStructMap(), dst, sizeof(*dst));
                        NBStruct_stClone(NBAvcPicDescBase_getSharedStructMap(), &u->desc->picProps, sizeof(u->desc->picProps), dst, sizeof(*dst));
                    }
                    r = TRUE;
                }
            }
        }
    }
    NBThreadMutex_unlock(&obj->mutex);
    return r;
}
*/

BOOL TSServerStreamReqConn_getStartUnitFromNewestBufferAndFlushOlders(STTSServerStreamReqConn* obj/*, const STNBAvcPicDescBase* optRequiredDescBase*/, STTSStreamUnit* dst, UI32* dstFrameSeq, BOOL* dstNewBlockAvailable, BOOL* dstNewBlockIsCompatible){
	BOOL r = FALSE;
	NBThreadMutex_lock(&obj->mutex);
	{
		r = TSServerStreamReqConn_getStartUnitFromNewestBufferAndFlushOldersLocked_(obj/*, optRequiredDescBase*/, dst, dstNewBlockAvailable, dstNewBlockIsCompatible);
		//
		if(dstFrameSeq != NULL){
			*dstFrameSeq = obj->state.rcv.iFrameSeq;
		}
	}
	NBThreadMutex_unlock(&obj->mutex);
	return r;
}

BOOL TSServerStreamReqConn_getNextBestLiveUnitLocked_(STTSServerStreamReqConn* obj/*, const STNBAvcPicDescBase* optRequiredDescBase*/, STTSStreamUnit* dst, UI32* dstFrameSeq, BOOL* dstNewBlockAvailable, BOOL* dstNewBlockIsCompatible){
	BOOL r = FALSE;
	//find
	{
		BOOL isNewBlockAvail = FALSE, hasNewFrame = FALSE, newBlockIsCompatible = FALSE;
		if(TSServerStreamReqConn_getStartUnitFromNewestBufferAndFlushOldersLocked_(obj/*, optRequiredDescBase*/, dst, &isNewBlockAvail, &newBlockIsCompatible)){
			r = TRUE;
		} else if(!isNewBlockAvail){
			//No new block started, return oldest frame
			SI32 iArrFrst = -1;
			const SI32 count = (sizeof(obj->buffs) / sizeof(obj->buffs[0]));
			SI32 i; for(i = 0; i < count; i++){
				STNBArray* arr = &obj->buffs[i];
				if(arr->use > 0){
					SI32 i2; for(i2 = 0; i2 < arr->use; i2++){
						STTSStreamUnit* u = NBArray_itmPtrAtIndex(arr, STTSStreamUnit, i2);
						if(u->desc->isFramePayload){
							hasNewFrame = TRUE;
							break;
						}
					}
					if(iArrFrst == -1){
						iArrFrst = i;
					}
				}
			}
			//Apply
			if(hasNewFrame){
				NBASSERT(iArrFrst >= 0)
				STNBArray* arr = &obj->buffs[iArrFrst];
				STTSStreamUnit* u = NBArray_itmPtrAtIndex(arr, STTSStreamUnit, 0);
				newBlockIsCompatible = TRUE;
				/*if(optRequiredDescBase != NULL){
                    newBlockIsCompatible = NBAvcPicDescBase_isCompatible(optRequiredDescBase, &u->desc->picProps);
					/ *if(!newBlockIsCompatible){
						PRINTF_WARNING("TSServerStreamReqConn, oldest-frame found is not compatible.\n");
					}* /
				}*/
				if(newBlockIsCompatible){
					if(dst != NULL){
						*dst = *u;
						obj->garbage.balance++; //add to garbage balance
						if(u->desc != NULL && u->desc->origin != NULL){
							NBASSERT(obj->garbage.origin.last == NULL || obj->garbage.origin.last == u->desc->origin)
							obj->garbage.origin.last = u->desc->origin;
							//PRINTF_INFO("TSServerStreamReqConn, ReqConn(%lld) setting garbage-unit from origin(%lld).\n", (SI64)obj, (SI64)u->desc->origin);
						}
						NBArray_removeItemAtIndex(arr, 0);
					}
					r = TRUE;
				}
			}
		}
		if(dstNewBlockAvailable != NULL){
			*dstNewBlockAvailable = isNewBlockAvail;
		}
		if(dstNewBlockIsCompatible != NULL){
			*dstNewBlockIsCompatible = newBlockIsCompatible;
		}
	}
	if(dstFrameSeq != NULL){
		*dstFrameSeq = obj->state.rcv.iFrameSeq;
	}
	return r;
}

void TSServerStreamReqConn_consumeGarbageUnits(STTSServerStreamReqConn* obj, STNBArray* garbageUnits){
	//gargabe balance
	if(garbageUnits != NULL && obj->garbage.balance > 0 && garbageUnits->use > 0){
		SI32 i, csmd = 0/*, useStart = garbageUnits->use*/; STTSStreamUnit* u;
		for(i = (SI32)garbageUnits->use - 1; i >= 0; i--){
			u = NBArray_itmPtrAtIndex(garbageUnits, STTSStreamUnit, i);
			if(u->desc != NULL && u->desc->origin == obj->garbage.origin.last){
				//PRINTF_INFO("TSServerStreamReqConn, ReqConn(%lld) taking garbage-unit from origin(%lld).\n", (SI64)obj, (SI64)u->desc->origin);
				NBArray_addValue(&obj->garbage.pend, *u);
				NBArray_removeItemAtIndex(garbageUnits, i);
				csmd++;
			}
		}
		NBASSERT(csmd <= obj->garbage.balance)
		if(csmd <= obj->garbage.balance){
			obj->garbage.balance -= csmd;
		} else {
			obj->garbage.balance = 0;
		}
		//PRINTF_INFO("TSServerStreamReqConn_consumeGarbageUnits, %d units taken from req-garbage to conn-garbage (%d remain).\n", csm, garbageUnits->use);
	}
}
	
BOOL TSServerStreamReqConn_getNextBestLiveUnit(STTSServerStreamReqConn* obj/*, const STNBAvcPicDescBase* optRequiredDescBase*/, STTSStreamUnit* dst, UI32* dstFrameSeq, BOOL* dstNewBlockAvailable, BOOL* dstNewBlockIsCompatible){
	BOOL r = FALSE;
	NBThreadMutex_lock(&obj->mutex);
	{
		r = TSServerStreamReqConn_getNextBestLiveUnitLocked_(obj/*, optRequiredDescBase*/, dst, dstFrameSeq, dstNewBlockAvailable, dstNewBlockIsCompatible);
	}
	NBThreadMutex_unlock(&obj->mutex);
	return r;
}

UI32 TSServerStreamReqConn_streamConsumeUnits_(void* pObj, STTSStreamUnit* units, const UI32 unitsSz, STTSStreamUnitsReleaser* optUnitsReleaser){
	UI32 r = 0;
	STTSServerStreamReqConn* obj = (STTSServerStreamReqConn*)pObj;
	IF_NBASSERT(STNBTimestampMicro timeLastAction = NBTimestampMicro_getMonotonicFast(), timeCur; SI64 usDiff;)
	NBThreadMutex_lock(&obj->mutex);
	{
#		ifdef NB_CONFIG_INCLUDE_ASSERTS
		BOOL streamStartedBefore = FALSE;
#		endif
		const SI32 buffsCount = (sizeof(obj->buffs) / sizeof(obj->buffs[0]));
		STTSStreamUnit* u = units;
		STTSStreamUnit* uAfterLast = units + unitsSz;
		BOOL isFramePayload; UI32 chunksTotalSz;
		while(u < uAfterLast){
			NBASSERT(u->desc != NULL)
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			streamStartedBefore = obj->state.rcv.started;
#			endif
			isFramePayload	= u->desc->isFramePayload;
			chunksTotalSz	= u->desc->chunksTotalSz;
			//isFramePayload (unit 5 or 1, frame)
			if(u->desc->isFramePayload){
				obj->state.rcv.iFrameSeq++;
			}
			//process unit
			{
				//start of stream-block
				if(u->desc->isBlockStart){
					//empty oldest
					{
						STNBArray* arr = &obj->buffs[0];
						if(arr->use > 0){
							STTSStreamUnit* u = NBArray_dataPtr(arr, STTSStreamUnit);
							TSStreamUnit_unitsReleaseGrouped(u, arr->use, optUnitsReleaser);
						}
						NBArray_empty(arr);
					}
					//swap slots
					{
						STNBArray first = obj->buffs[0];
						SI32 i; for(i = 1; i < buffsCount; i++){
							obj->buffs[i - 1] = obj->buffs[i];
						}
						obj->buffs[buffsCount - 1] = first;
					}
					//start consumption
					if(!obj->state.rcv.started){
						obj->state.rcv.started = TRUE;
					}
					
				}
				//add to newest slot
				if(obj->state.rcv.started){
					STNBArray* arr = &obj->buffs[buffsCount - 1];
					NBASSERT(u->desc->chunks != NULL && u->desc->chunksSz > 0)
					STTSStreamUnit u2;
					TSStreamUnit_init(&u2);
					TSStreamUnit_setAsOther(&u2, u);
					NBArray_addValue(arr, u2);
				}
			}
			//next
			u++;
		}
		//flush garbage
		if(obj->garbage.pend.use > 0){
			STTSStreamUnit* u = NBArray_dataPtr(&obj->garbage.pend, STTSStreamUnit);
			//PRINTF_INFO("TSServerStreamReqConn_streamConsumeUnits_, %d units released from conn-garbage.\n", obj->garbage.pend.use);
			TSStreamUnit_unitsReleaseGrouped(u, obj->garbage.pend.use, optUnitsReleaser);
			NBArray_empty(&obj->garbage.pend);
		}
		//flags
		obj->state.rcv.secsChangesNoUnits = 0;
		//result
		NBASSERT(units <= u)
		r = (UI32)(u - units);
	}
	NBThreadMutex_unlock(&obj->mutex);
#	ifdef NB_CONFIG_INCLUDE_ASSERTS
	{
		timeCur	= NBTimestampMicro_getMonotonicFast();
		usDiff	= NBTimestampMicro_getDiffInUs(&timeLastAction, &timeCur);
		if(usDiff >= 1000ULL){
			PRINTF_INFO("TSServerStreamReqConn_streamConsumeUnits_ took %llu.%llu%llums.\n", (usDiff / 1000ULL), (usDiff % 1000ULL) % 100ULL, (usDiff % 100ULL) % 10ULL);
		}
		timeLastAction = timeCur;
	}
#	endif
	return r;
}

//

void TSServerStreamReqConn_streamEnded_(void* pObj){
	STTSServerStreamReqConn* obj = (STTSServerStreamReqConn*)pObj; 
	//set flag
	obj->state.rcv.ended = TRUE;
}
