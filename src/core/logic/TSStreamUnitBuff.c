//
//  TSStreamUnitBuff.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSStreamUnitBuff.h"
//
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBDatetime.h"     //for timestamp
#include "nb/2d/NBAvc.h"

//stream unit

typedef struct STTSStreamUnitBuffOpq_ {
	STNBObject				prnt;		//parent
	STTSStreamUnitBuffDesc	desc;		//payload dscription (for STTSStreamUnit)
	//data
	struct {
		STNBArray			chunks;		//STNBDataPtr
	} buff;
} STTSStreamUnitBuffOpq;

NB_OBJREF_BODY(TSStreamUnitBuff, STTSStreamUnitBuffOpq, NBObject)

//

void TSStreamUnitBuff_emptyOpq_(STTSStreamUnitBuffOpq* opq, STNBDataPtrReleaser* optPtrsReleaser);

//TSStreamUnitBuff

void TSStreamUnitBuff_initZeroed(STNBObject* obj){
	STTSStreamUnitBuffOpq* opq = (STTSStreamUnitBuffOpq*)obj;
	//desc
	{
		//NBAvcPicDescBase_init(&opq->desc.picProps);
	}
	//state
	{
		//
	}
	//data
	{
		NBArray_init(&opq->buff.chunks, sizeof(STNBDataPtr), NULL);
	}
}

void TSStreamUnitBuff_uninitLocked(STNBObject* obj){
	STTSStreamUnitBuffOpq* opq = (STTSStreamUnitBuffOpq*)obj;
	//chunks
	{
		if(opq->buff.chunks.use > 0){
			STNBDataPtr* chunks = NBArray_dataPtr(&opq->buff.chunks, STNBDataPtr);
			NBDataPtr_ptrsReleaseGrouped(chunks, opq->buff.chunks.use);
			NBArray_empty(&opq->buff.chunks);
		}
		NBArray_release(&opq->buff.chunks);
	}
    //desc
    {
        //NBAvcPicDescBase_release(&opq->desc.picProps);
    }
}

//

STTSStreamUnit TSStreamUnitBuff_getDataLocked_(STTSStreamUnitBuffRef ref, STTSStreamUnitBuffOpq* opq){
	STTSStreamUnit r;
	NBMemory_setZeroSt(r, STTSStreamUnit);
	//update desc
	{
		//header
		{
			const STNBAvcNaluDef* nDef = NBAvc_getNaluDef(opq->desc.hdr.type);
			opq->desc.isBlockStart		= (opq->desc.hdr.type == 7); //7-seq-params-set is the first NALU, followed by 8-pic-params-set and 5-IDR-VCL
			opq->desc.isSeqReqHdr		= (opq->desc.hdr.type == 7 || opq->desc.hdr.type == 8); //7-seq-params-set, 8-pic-params-set (required to render frames)
			opq->desc.isFramePayload	= (nDef != NULL && nDef->group[0] == 'V' && nDef->group[1] == 'C' && nDef->group[2] == 'L' && nDef->group[3] == '\0');
			opq->desc.isKeyframe		= (opq->desc.hdr.type == 5); //5-IDR-picture (independent-frame fully renderizable)
		}
		//chunks
		if(opq->buff.chunks.use > 0){
			opq->desc.chunks	= NBArray_dataPtr(&opq->buff.chunks, STNBDataPtr);
			opq->desc.chunksSz	= opq->buff.chunks.use;
			opq->desc.chunksTotalSz	= 0;
			//count bytes
			{
				STNBDataPtr* c = NBArray_dataPtr(&opq->buff.chunks, STNBDataPtr);
				const STNBDataPtr* cAfterEnd = c + opq->buff.chunks.use;
				while(c < cAfterEnd){
					opq->desc.chunksTotalSz += c->use;
					c++;
				}
			}
		} else {
			opq->desc.chunks		= NULL;
			opq->desc.chunksSz		= 0;
			opq->desc.chunksTotalSz	= 0;
		}
	}
	//desc
	{
		r.desc = &opq->desc;
	}
	return r;
}

void TSStreamUnitBuff_setDataLocked_(STTSStreamUnitBuffOpq* opq, const void* origin, const UI32 unitId, const STTSStreamUnitBuffHdr* hdr, STNBArray* chunks, const BOOL swapBuffers/*, const STNBAvcPicDescBase* knownProps*/, STNBDataPtrReleaser* optPtrsReleaser){
	//release
	TSStreamUnitBuff_emptyOpq_(opq, optPtrsReleaser);
	//set values
	{
		opq->desc.origin	= origin;
		opq->desc.unitId	= unitId; //non-zero unit sequence number/id
		opq->desc.time		= NBTimestampMicro_getUTC();
		//apply header
		if(hdr != NULL){
			opq->desc.hdr = *hdr;
		}
		//apply chunks
		if(chunks != NULL && chunks->use > 0){
			if(swapBuffers){
				//optimized swap of buffers
				NBArray_swapBuffers(&opq->buff.chunks, chunks);
			} else {
				//copy data
				STNBDataPtr* arr = NBArray_dataPtr(chunks, STNBDataPtr);
				NBArray_addItems(&opq->buff.chunks, arr, sizeof(arr[0]), chunks->use);
			}
		}
		//apply picProps
        /*if(knownProps == NULL){
			NBAvcPicDescBase_release(&opq->desc.picProps);
			NBAvcPicDescBase_init(&opq->desc.picProps);
		} else {
			NBAvcPicDescBase_setAsOther(&opq->desc.picProps, knownProps);
		}*/
	}
}
	
//

STTSStreamUnit TSStreamUnitBuff_setDataAsOwningUnit(STTSStreamUnitBuffRef ref, const void* origin, const UI32 unitId, const STTSStreamUnitBuffHdr* hdr, STNBArray* chunks, const BOOL swapBuffers/*, const STNBAvcPicDescBase* knownProps*/, STNBDataPtrReleaser* optPtrsReleaser){
	STTSStreamUnit r;
	STTSStreamUnitBuffOpq* opq = (STTSStreamUnitBuffOpq*)ref.opaque;
	//set payload
	TSStreamUnitBuff_setDataLocked_(opq, origin, unitId, hdr, chunks, swapBuffers/*, knownProps*/, optPtrsReleaser);
	//data
	r = TSStreamUnitBuff_getDataLocked_(ref, opq);
	//set reference to myself (without retaining)
	r.def.alloc = ref;
	//
	return r;
}

//data

BOOL TSStreamUnitBuff_setData(STTSStreamUnitBuffRef ref, const void* origin, const UI32 unitId, const STTSStreamUnitBuffHdr* hdr, STNBArray* chunks, const BOOL swapBuffers/*, const STNBAvcPicDescBase* knownProps*/, STNBDataPtrReleaser* optPtrsReleaser){
	BOOL r = FALSE;
	STTSStreamUnitBuffOpq* opq = (STTSStreamUnitBuffOpq*)ref.opaque;
	NBObject_lock(opq);
	{
		TSStreamUnitBuff_setDataLocked_(opq, origin, unitId, hdr, chunks, swapBuffers/*, knownProps*/, optPtrsReleaser);
		r = TRUE;
	}
	NBObject_unlock(opq);
	return r;
}

void TSStreamUnitBuff_empty(STTSStreamUnitBuffRef ref, STNBDataPtrReleaser* optPtrsReleaser){
	STTSStreamUnitBuffOpq* opq = (STTSStreamUnitBuffOpq*)ref.opaque;
	TSStreamUnitBuff_emptyOpq_(opq, optPtrsReleaser);
}

void TSStreamUnitBuff_emptyOpq_(STTSStreamUnitBuffOpq* opq, STNBDataPtrReleaser* optPtrsReleaser){
	//header
	{
		NBMemory_setZeroSt(opq->desc.hdr, STTSStreamUnitBuffHdr);
	}
	//chunks
	if(opq->buff.chunks.use > 0){
		STNBDataPtr* chunks = NBArray_dataPtr(&opq->buff.chunks, STNBDataPtr);
		if(optPtrsReleaser != NULL && optPtrsReleaser->itf.ptrsReleaseGrouped != NULL){
			(*optPtrsReleaser->itf.ptrsReleaseGrouped)(chunks, opq->buff.chunks.use, optPtrsReleaser->usrData);
		} else {
			NBDataPtr_ptrsReleaseGrouped(chunks, opq->buff.chunks.use);
		}
		NBArray_empty(&opq->buff.chunks);
	}
}
