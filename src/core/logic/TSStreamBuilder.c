//
//  TSStreamBuilder.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSStreamBuilder.h"
//
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBArray.h"
#include "nb/core/NBLinkedList.h"

struct STTSStreamBuilderOpq_;

//TSStreamBuilderWriterItf

typedef struct STTSStreamBuilderWriterItf_ {
	void (*bldrConcatBytes)(const void* data, const UI32 dataSz, void* usrData);
} STTSStreamBuilderWriterItf;

//TSStreamBuilderWriter

typedef struct STTSStreamBuilderWriter_ {
	STTSStreamBuilderWriterItf	itf;
	void*						usrData;
} STTSStreamBuilderWriter;

//

void TSStreamBuilderWriter_bldrConcatBytes_nbString_(const void* data, const UI32 dataSz, void* usrData){
	NBString_concatBytes((STNBString*)usrData, data, dataSz);
}

void TSStreamBuilderWriter_bldrConcatBytes_NBHttpServiceRespRef_(const void* data, const UI32 dataSz, void* usrData){
    STNBHttpServiceRespRawLnk* lnk = (STNBHttpServiceRespRawLnk*)usrData;
    NBHttpServiceRespRawLnk_sendBytes(lnk, data, dataSz);
}

//stream unitsGrp ('moof')

typedef struct STTSStreamBuilderMoofState_ {
	UI32		nextSeqNum;		//next 'moof' seqNum
	UI64		nextBaseScaledTime;	//next 'moof' baseTime (scaled time)
	//unit
	struct {
		UI64	globalCount;	//global amount of units added
		UI32	accumCount;		//current grp amount of units added
	} units;
} STTSStreamBuilderMoofState;

void TSStreamBuilderMoofState_init(STTSStreamBuilderMoofState* obj);
void TSStreamBuilderMoofState_release(STTSStreamBuilderMoofState* obj);
void TSStreamBuilderMoofState_setAsOther(STTSStreamBuilderMoofState* obj, const STTSStreamBuilderMoofState* other);

//mp4

/*
typedef struct STTSStreamBuilderMp4_ {
	struct STTSStreamBuilderOpq_* opq;	//parent
	STNBMp4FileTypeBox*		ftyp;		//if format is mp4.
	//tmp
	struct {
		STNBString			str;
	} tmp;
	//'moov'
	struct {
		STNBMp4MovieBox*	box;		//if format is mp4.
		STNBString			sps;
		STNBString			pps;
		STNBAvcPicDescBase* picProps;	//'moov' picProps (for detection of compatible streams)
		//trak
		struct {
			//tkhd
			struct {
				STNBMp4TrackHeaderBox* box;	//owned by 'moov.box', do not release
				//mdia
				struct {
					//minf
					struct {
						//stbl
						struct {
							//stsd
							struct {
								//avc1
								struct {
									STNBMp4AVCSampleEntry* box; //owned by 'moov.box', do not release
									//avcC
									struct {
										STNBMp4AVCConfigurationBox* box; //owned by 'moov.box', do not release
									} avcC;
								} avc1;
							} stsd;
						} stbl;
					} minf;
				} mdia;
			} tkhd;
		} trak;
	} moov;
	//moof
	struct {
		STNBMp4MovieFragmentBox*	box;	//if format is mp4.
		STTSStreamBuilderMoofState	accum;	//accum in current 'moof'
		STTSStreamBuilderMoofState	sent;	//sent in last 'moof'
		//'mfhd'
		struct {
			STNBMp4MovieFragmentHeaderBox* box; //owned by moof.box (do not release)
		} mfhd;
		//'traf'
		struct {
			//'tfdt'
			struct {
				STNBMp4TrackFragmentBaseMediaDecodeTimeBox* box; //owned by moof.box (do not release)
			} tfdt;
			//'tfhd'
			struct {
				STNBMp4TrackFragmentHeaderBox* box; //owned by moof.box (do not release)
			} tfhd;
			//'trun'
			struct {
				STNBMp4TrackRunBox* box; //owned by moof.box (do not release)
			} trun;
		} traf;
	} moof;
	//mdat
	struct {
		STNBMp4BoxHdr*		hdr;		//if format is mp4.
		//units
		struct {
			STNBArray		arr;			//STTSStreamUnit
			UI32			payBytesTotal;	//
			UI32			scaledDuration;	//milliseconds
			//consumed
			struct {
				UI32		count;			//amount of units already synced to 'moof'
				UI32		payBytesCount;	//amount of bytes already synced to 'moof'
			} consumed;
		} units;
	} mdat;
} STTSStreamBuilderMp4;
 //
void TSStreamBuilderMp4_init(STTSStreamBuilderMp4* obj);
void TSStreamBuilderMp4_release(STTSStreamBuilderMp4* obj);
void TSStreamBuilderMp4_allocMdat(STTSStreamBuilderMp4* obj);
BOOL TSStreamBuilderMp4_isCompatible(STTSStreamBuilderMp4* obj, const STTSStreamUnit* unitFnd);
BOOL TSStreamBuilderMp4_isFirstOrNonCompatible(STTSStreamBuilderMp4* obj, const STTSStreamUnit* unitFnd);
void TSStreamBuilderMp4_setSps(STTSStreamBuilderMp4* obj, const STTSStreamUnit* unitFnd);
void TSStreamBuilderMp4_setPps(STTSStreamBuilderMp4* obj, const STTSStreamUnit* unitFnd);
void TSStreamBuilderMp4_buildFtyp(STTSStreamBuilderMp4* obj);
void TSStreamBuilderMp4_buildMoov(STTSStreamBuilderMp4* obj, const STTSStreamUnit* unitFnd);
BOOL TSStreamBuilderMp4_hasMoof(const STTSStreamBuilderMp4* obj);
void TSStreamBuilderMp4_buildMoof(STTSStreamBuilderMp4* obj, const STTSStreamUnit* unitFnd);
BOOL TSStreamBuilderMp4_mdatQueueIsEmpty(STTSStreamBuilderMp4* obj);
void TSStreamBuilderMp4_mdatQueueAddOwning(STTSStreamBuilderMp4* obj, const UI32 msUnitDuration, STTSStreamUnit* u, const UI32 extraPrefixSz);
BOOL TSStreamBuilderMp4_addPendUnitsToMoof(STTSStreamBuilderMp4* obj);
void TSStreamBuilderMp4_sendFtypAndMoov(STTSStreamBuilderMp4* obj, const STTSStreamUnit* u, STTSStreamBuilderWriter* dst);
void TSStreamBuilderMp4_sendMoofAndMdat(STTSStreamBuilderMp4* obj, STTSStreamUnitsReleaser* optUnitsReleaser, STTSStreamBuilderWriter* dst);
 */

//stream data sequence (the base of state machine model)

typedef struct STTSStreamBuilderOpq_ {
	STNBObject	prnt;			//parent
	//cfg
	struct {
		ENTSStreamBuilderFmt			fmt;
		ENTSStreamBuilderUnitsPrefix	unitsPrefix;
		ENTSStreamBuilderMessageWrapper	msgsWrapper;
		BOOL							realTimeOffsets;	//frame duration will be calculated using delay from previous frame (it introduce an one-frame delay on the stream).
	} cfg;
	//frames
	struct {
		//prev
		struct {
			STNBTimestampMicro	fillStart;	//last time queued
			STNBArray			pend;		//STTSStreamUnit, prev frame pend units (used when realTimeOffsets is enabled)
		} prev;
	} frames;
	//mp4
	//STTSStreamBuilderMp4 mp4;
} STTSStreamBuilderOpq;

NB_OBJREF_BODY(TSStreamBuilder, STTSStreamBuilderOpq, NBObject)

//

void TSStreamBuilder_initZeroed(STNBObject* obj){
	STTSStreamBuilderOpq* opq = (STTSStreamBuilderOpq*)obj;
	//cfg
	{
		//opq->cfg.realTimeOffsets = TRUE; //ToDo: remove this line
	}
	//frames
	{
		NBArray_init(&opq->frames.prev.pend, sizeof(STTSStreamUnit), NBCompare_TSStreamUnit_byDescPtrOnly); //prev frame pend units (used when realTime is enabled)
	}
	//mp4
	/*{
		TSStreamBuilderMp4_init(&opq->mp4);
		opq->mp4.opq = opq;
	}*/
}

void TSStreamBuilder_uninitLocked(STNBObject* obj){
	STTSStreamBuilderOpq* opq = (STTSStreamBuilderOpq*)obj;
	//cfg
	{
		//
	}
	//frames
	{
		if(opq->frames.prev.pend.use > 0){
			STTSStreamUnit* u = NBArray_dataPtr(&opq->frames.prev.pend, STTSStreamUnit);
			TSStreamUnit_unitsReleaseGrouped(u, opq->frames.prev.pend.use, NULL);
			NBArray_empty(&opq->frames.prev.pend);
		}
		NBArray_release(&opq->frames.prev.pend);
	}
	//mp4
	{
		//TSStreamBuilderMp4_release(&opq->mp4);
	}
}

//cfg

BOOL TSStreamBuilder_setFormatAndUnitsPrefix(STTSStreamBuilderRef ref, const ENTSStreamBuilderFmt fmt, const ENTSStreamBuilderUnitsPrefix unitsPrefix){
	BOOL r = FALSE;
	STTSStreamBuilderOpq* opq = (STTSStreamBuilderOpq*)ref.opaque; NBASSERT(TSStreamBuilder_isClass(ref));
	NBObject_lock(opq);
	if(
	   fmt >= 0 && fmt < ENTSStreamBuilderFmt_Count
	   && unitsPrefix >= 0 && unitsPrefix < ENTSStreamBuilderUnitsPrefix_Count
	   )
	{
		/*const BOOL wasMp4 = (opq->cfg.fmt == ENTSStreamBuilderFmt_Mp4Fragmented || opq->cfg.fmt == ENTSStreamBuilderFmt_Mp4FullyFragmented);
		const BOOL isMp4 = (fmt == ENTSStreamBuilderFmt_Mp4Fragmented || fmt == ENTSStreamBuilderFmt_Mp4FullyFragmented);
		if(!wasMp4 && isMp4){
			TSStreamBuilderMp4_release(&opq->mp4);
			TSStreamBuilderMp4_init(&opq->mp4);
			TSStreamBuilderMp4_allocMdat(&opq->mp4);
			opq->mp4.opq = opq;
		}*/
		opq->cfg.fmt			= fmt;
		opq->cfg.unitsPrefix	= unitsPrefix;
		r = TRUE;
	}
	NBObject_unlock(opq);
	return r;
}

BOOL TSStreamBuilder_setMsgsWrapper(STTSStreamBuilderRef ref, const ENTSStreamBuilderMessageWrapper msgsWrapper){
	BOOL r = FALSE;
	STTSStreamBuilderOpq* opq = (STTSStreamBuilderOpq*)ref.opaque; NBASSERT(TSStreamBuilder_isClass(ref));
	NBObject_lock(opq);
	if(msgsWrapper >= 0 && msgsWrapper < ENTSStreamBuilderMessageWrapper_Count){
		opq->cfg.msgsWrapper = msgsWrapper;
		r = TRUE;
	}
	NBObject_unlock(opq);
	return r;
}

BOOL TSStreamBuilder_setRealTimeOffsets(STTSStreamBuilderRef ref, const BOOL realTimeOffsets){
	BOOL r = FALSE;
	STTSStreamBuilderOpq* opq = (STTSStreamBuilderOpq*)ref.opaque; NBASSERT(TSStreamBuilder_isClass(ref));
	NBObject_lock(opq);
	{
		opq->cfg.realTimeOffsets = realTimeOffsets;
		r = TRUE;
	}
	NBObject_unlock(opq);
	return r;
}

void TSStreamBuilder_queueUnitPayOpq_(STTSStreamBuilderOpq* opq, STTSStreamUnit* u, STTSStreamBuilderWriter* dst){
	//prefix
	if(u->desc->hdr.prefixSz > 0){
		(*dst->itf.bldrConcatBytes)(u->desc->hdr.prefix, u->desc->hdr.prefixSz, dst->usrData);
	}
	//chunks
	{
		STNBDataPtr* c = u->desc->chunks;
		const STNBDataPtr* cAfterEnd = c + u->desc->chunksSz;
		while(c < cAfterEnd){
			if(c->use > 0){
				(*dst->itf.bldrConcatBytes)(c->ptr, c->use, dst->usrData);
			}
			//next
			c++;
		}
	}
}
	
void TSStreamBuilder_queueUnitSlotLockedOpq_(STTSStreamBuilderOpq* opq, STTSStreamUnit* u, STTSStreamBuilderWriter* dst){
	//msgHeader
	switch (opq->cfg.msgsWrapper) {
		case ENTSStreamBuilderMessageWrapper_None:
			break;
		case ENTSStreamBuilderMessageWrapper_WebSocket:
			{
				char hdr[14];
				const UI32 hdrSz = NBWebSocketFrame_writeHeader(ENNBWebSocketFrameOpCode_Binary, 0, 4 + u->desc->hdr.prefixSz + u->desc->chunksTotalSz, TRUE, TRUE, hdr);
				if(hdrSz > 0){
					(*dst->itf.bldrConcatBytes)(hdr, hdrSz, dst->usrData);
				}
			} 
			break;	
		default:
			NBASSERT(FALSE) //implementation error
			break;
	}
	//nalPrefix
	{
		switch (opq->cfg.unitsPrefix) {
			case ENTSStreamBuilderUnitsPrefix_0x00000001:
				{
					const BYTE hdr[] = { 0x00, 0x00, 0x00, 0x01 };
					(*dst->itf.bldrConcatBytes)(hdr, (sizeof(hdr) / sizeof(hdr[0])), dst->usrData);
				}
				break;
			case ENTSStreamBuilderUnitsPrefix_UnitSize32:
				{
					UI32 hdr; const UI32 unitSz = u->desc->hdr.prefixSz + u->desc->chunksTotalSz;
					NBMp4_writeUI32At(&unitSz, &hdr);
					(*dst->itf.bldrConcatBytes)(&hdr, sizeof(hdr), dst->usrData);
				}
				break;
			default:
				NBASSERT(FALSE) //implementation error
				break;
		}
	}
	//payload
	TSStreamBuilder_queueUnitPayOpq_(opq, u, dst);
}
	
void TSStreamBuilder_queueUnitSlotWithPrefixOpq_(STTSStreamBuilderOpq* opq, STTSStreamUnit* u, const void* prefix, const UI32 prefixSz, STTSStreamBuilderWriter* dst){	
	if(prefixSz > 0){
		(*dst->itf.bldrConcatBytes)(prefix, prefixSz, dst->usrData);
	}
	//payload
	TSStreamBuilder_queueUnitPayOpq_(opq, u, dst);
}

void TSStreamBuilder_queueStringWithPrefixOpq_(STTSStreamBuilderOpq* opq, STNBString* str, const void* prefix, const UI32 prefixSz, STTSStreamBuilderWriter* dst){
	if(prefixSz > 0){
		(*dst->itf.bldrConcatBytes)(prefix, prefixSz, dst->usrData);
	}
	if(str->length > 0){
		(*dst->itf.bldrConcatBytes)(str->str, str->length, dst->usrData);
	}
}

/*
void TSStreamBuilder_mp4ConcatUnitToWriterOwning2_(STTSStreamBuilderOpq* opq, const UI32 msUnitDuration, STTSStreamUnit* u, STTSStreamUnitsReleaser* optUnitsReleaser, STTSStreamBuilderWriter* dst){
	if(!u->desc->isFramePayload){
		//accum only (wating for frame payload)
		TSStreamBuilderMp4_mdatQueueAddOwning(&opq->mp4, msUnitDuration, u, 0);
	} else {
        //isFramePayload
		//flush 'moof' to start a new one with this keyFrame
		if(u->desc->isKeyframe){
			TSStreamBuilderMp4_sendMoofAndMdat(&opq->mp4, optUnitsReleaser, dst);
		}
		//build 'ftyp' + 'moov' header
		if(TSStreamBuilderMp4_isFirstOrNonCompatible(&opq->mp4, u)){
            //start stream
			TSStreamBuilderMp4_sendFtypAndMoov(&opq->mp4, u, dst);
			NBASSERT(TSStreamBuilderMp4_mdatQueueIsEmpty(&opq->mp4)) //must be empty
		}
		//build 'moof' header (fragment, if necesary)
		if(!TSStreamBuilderMp4_hasMoof(&opq->mp4)){
			TSStreamBuilderMp4_buildMoof(&opq->mp4, u);
		}
		//Add pendUnits and current one to 'moof'
		{
			//accum to 'moof'
			TSStreamBuilderMp4_mdatQueueAddOwning(&opq->mp4, msUnitDuration, u, 4); //+4 for nalSize header
			//feed 'moof'
			TSStreamBuilderMp4_addPendUnitsToMoof(&opq->mp4);
		}
		//flush 'moof'
		if(opq->cfg.fmt == ENTSStreamBuilderFmt_Mp4FullyFragmented){
			TSStreamBuilderMp4_sendMoofAndMdat(&opq->mp4, optUnitsReleaser, dst);
		}
	}
}*/
	
void TSStreamBuilder_concatUnitToWriterOwning_(STTSStreamBuilderOpq* opq, const UI32 msUnitDuration, STTSStreamUnit* u, STTSStreamUnitsReleaser* optUnitsReleaser, STTSStreamBuilderWriter* dst){
	NBASSERT(u->desc != NULL) //released reference given as param
	NBObject_lock(opq);
	/*if(opq->cfg.fmt == ENTSStreamBuilderFmt_Mp4Fragmented || opq->cfg.fmt == ENTSStreamBuilderFmt_Mp4FullyFragmented){
		//'mp4' mode
		switch(u->desc->hdr.type) {
			case 7:
				//7=seq-params-set
				NBASSERT(!u->desc->isFramePayload) //should not be a frame
				TSStreamBuilderMp4_setSps(&opq->mp4, u);
				break;
			case 8:
				//8 = pic-params-set
				NBASSERT(!u->desc->isFramePayload) //should not be a frame
				TSStreamBuilderMp4_setPps(&opq->mp4, u); 
				break;
			default:
				if(opq->cfg.realTimeOffsets){
					const STNBTimestampMicro time = NBTimestampMicro_getMonotonicFast();
					//flush prev unit
					if(u->desc->isFramePayload){
						if(opq->frames.prev.pend.use != 0){
							const SI64 msPassed = NBTimestampMicro_getDiffInMs(&opq->frames.prev.fillStart, &time);
							STTSStreamUnit* u2 = NBArray_dataPtr(&opq->frames.prev.pend, STTSStreamUnit);
							const STTSStreamUnit* u2End = u2 + opq->frames.prev.pend.use - 1;
							while(u2 < u2End){
								TSStreamBuilder_mp4ConcatUnitToWriterOwning2_(opq, 0, u2, optUnitsReleaser, dst);
								//next
								u2++;
							}
							//last (with timestamp)
							NBASSERT(msPassed >= 0)
							TSStreamBuilder_mp4ConcatUnitToWriterOwning2_(opq, (msPassed < 0 ? 0 : (UI32)msPassed), u2, optUnitsReleaser, dst);
							PRINTF_INFO("TSStreamBuilder, sending frame after %lldms.\n", msPassed);
							//empty
							NBArray_empty(&opq->frames.prev.pend);
						}
					}
					//queue to prev.pend
					if(opq->frames.prev.pend.use == 0){
						opq->frames.prev.fillStart = NBTimestampMicro_getMonotonicFast();
					}
					NBArray_addValue(&opq->frames.prev.pend, *u);
					TSStreamUnit_resignToData(u);
				} else {
					//process
					TSStreamBuilder_mp4ConcatUnitToWriterOwning2_(opq, msUnitDuration, u, optUnitsReleaser, dst);
				}
				break;
		}
	} else
    */
    {
		//'raw-h264' mode
        //nalPrefix
        {
            switch (opq->cfg.unitsPrefix) {
                case ENTSStreamBuilderUnitsPrefix_0x00000001:
                    {
                        const BYTE hdr[] = { 0x00, 0x00, 0x00, 0x01 };
                        (*dst->itf.bldrConcatBytes)(hdr, (sizeof(hdr) / sizeof(hdr[0])), dst->usrData);
                    }
                    break;
                case ENTSStreamBuilderUnitsPrefix_UnitSize32:
                    {
                        UI32 hdr; const UI32 unitSz = u->desc->hdr.prefixSz + u->desc->chunksTotalSz;
                        NBMp4_writeUI32At(&unitSz, &hdr);
                        (*dst->itf.bldrConcatBytes)(&hdr, sizeof(hdr), dst->usrData);
                    }
                    break;
                default:
                    NBASSERT(FALSE) //implementation error
                    break;
            }
        }
		TSStreamBuilder_queueUnitPayOpq_(opq, u, dst);
	}
	NBObject_unlock(opq);
}

/*void TSStreamBuilder_concatUnitOwning(STTSStreamBuilderRef ref, const UI32 msUnitDuration, STTSStreamUnit* u, STNBString* dst, STTSStreamUnitsReleaser* optUnitsReleaser){
	STTSStreamBuilderOpq* opq = (STTSStreamBuilderOpq*)ref.opaque; NBASSERT(TSStreamBuilder_isClass(ref));
	STTSStreamBuilderWriter w;
	NBMemory_setZeroSt(w, STTSStreamBuilderWriter);
	w.itf.bldrConcatBytes	= TSStreamBuilderWriter_bldrConcatBytes_nbString_;
	w.usrData				= dst;
	NBASSERT(u->desc != NULL) //released reference given as param
	TSStreamBuilder_concatUnitToWriterOwning_(opq, msUnitDuration, u, optUnitsReleaser, &w);
}*/

void TSStreamBuilder_httpReqSendUnitOwning(STTSStreamBuilderRef ref, const UI32 msUnitDuration, STTSStreamUnit* u, STNBHttpServiceRespRawLnk dst, STTSStreamUnitsReleaser* optUnitsReleaser){
	STTSStreamBuilderOpq* opq = (STTSStreamBuilderOpq*)ref.opaque; NBASSERT(TSStreamBuilder_isClass(ref));
	STTSStreamBuilderWriter w;
	NBMemory_setZeroSt(w, STTSStreamBuilderWriter);
	w.itf.bldrConcatBytes	= TSStreamBuilderWriter_bldrConcatBytes_NBHttpServiceRespRef_;
	w.usrData				= &dst;
	NBASSERT(u->desc != NULL) //released reference given as param
	TSStreamBuilder_concatUnitToWriterOwning_(opq, msUnitDuration, u, optUnitsReleaser, &w);
}

//stream unitsGrp ('moof')

void TSStreamBuilderMoofState_init(STTSStreamBuilderMoofState* obj){
	NBMemory_setZeroSt(*obj, STTSStreamBuilderMoofState);
}

void TSStreamBuilderMoofState_release(STTSStreamBuilderMoofState* obj){
	//nothing
}

void TSStreamBuilderMoofState_setAsOther(STTSStreamBuilderMoofState* obj, const STTSStreamBuilderMoofState* other){
	*obj = *other;
}

//mp4 methods

/*
void TSStreamBuilderMp4_init(STTSStreamBuilderMp4* obj){
	//tmp
	{
		NBString_initWithSz(&obj->tmp.str, 1024, 1024, 0.10f);
	}
	//'moov'
	{
		NBString_init(&obj->moov.sps);
		NBString_init(&obj->moov.pps);
	}
	//'moof'
	{
		TSStreamBuilderMoofState_init(&obj->moof.accum);
		TSStreamBuilderMoofState_init(&obj->moof.sent);
	}
	//'mdat'
	{
		//units
		NBArray_init(&obj->mdat.units.arr, sizeof(STTSStreamUnit), NULL);
	}
}

void TSStreamBuilderMp4_release(STTSStreamBuilderMp4* obj){
	if(obj->ftyp != NULL){
		NBMp4FileTypeBox_release(obj->ftyp);
		NBMemory_free(obj->ftyp);
		obj->ftyp = NULL;
	}
	//'moov'
	{
		if(obj->moov.box != NULL){
			NBMp4MovieBox_release(obj->moov.box);
			NBMemory_free(obj->moov.box);
			obj->moov.box = NULL;
		}
		{
			NBString_release(&obj->moov.sps);
			NBString_release(&obj->moov.pps);
		}
		if(obj->moov.picProps != NULL){
			NBAvcPicDescBase_release(obj->moov.picProps);
			NBMemory_free(obj->moov.picProps);
			obj->moov.picProps = NULL;
		}
	}
	//'moof'
	{
		if(obj->moof.box != NULL){
			NBMp4MovieFragmentBox_release(obj->moof.box);
			NBMemory_free(obj->moof.box);
			obj->moof.box				= NULL;
			obj->moof.traf.tfhd.box		= NULL;
			obj->moof.traf.trun.box		= NULL;
		}
		TSStreamBuilderMoofState_release(&obj->moof.accum);
		TSStreamBuilderMoofState_release(&obj->moof.sent);
	}
	//mdat
	{
		if(obj->mdat.hdr != NULL){
			NBMp4BoxHdr_release(obj->mdat.hdr);
			NBMemory_free(obj->mdat.hdr);
			obj->mdat.hdr = NULL;
		}
		//units
		{
			//arr
			{
				STTSStreamUnit* u = NBArray_dataPtr(&obj->mdat.units.arr, STTSStreamUnit);
				TSStreamUnit_unitsReleaseGrouped(u, obj->mdat.units.arr.use, NULL);
				NBArray_empty(&obj->mdat.units.arr);
				NBArray_release(&obj->mdat.units.arr);
			}
			//
			obj->mdat.units.payBytesTotal			= 0;
			obj->mdat.units.consumed.count			= 0;
			obj->mdat.units.consumed.payBytesCount	= 0;
		}
	}
	//tmp
	{
		NBString_release(&obj->tmp.str);
	}
}

void TSStreamBuilderMp4_allocMdat(STTSStreamBuilderMp4* obj){
	//hdr
	if(obj->mdat.hdr == NULL){
		obj->mdat.hdr = NBMemory_allocType(STNBMp4BoxHdr);
		NBMp4BoxHdr_init(obj->mdat.hdr);
		obj->mdat.hdr->type.str[0] = 'm'; 
		obj->mdat.hdr->type.str[1] = 'd';
		obj->mdat.hdr->type.str[2] = 'a';
		obj->mdat.hdr->type.str[3] = 't';
	}
}

BOOL TSStreamBuilderMp4_isCompatible(STTSStreamBuilderMp4* obj, const STTSStreamUnit* u){
	return (
			//both are NULL
			(obj->moov.picProps == NULL && u->desc->picProps.w == 0)
			//both are compatible
			|| (
				obj->moov.picProps != NULL
				&& u->desc->picProps.w != 0
                && NBAvcPicDescBase_isCompatible(obj->moov.picProps, &u->desc->picProps)
				)
			);
}

BOOL TSStreamBuilderMp4_isFirstOrNonCompatible(STTSStreamBuilderMp4* obj, const STTSStreamUnit* u){
	return (obj->ftyp == NULL || obj->moov.box == NULL || !TSStreamBuilderMp4_isCompatible(obj, u));
}

void TSStreamBuilderMp4_setSps(STTSStreamBuilderMp4* obj, const STTSStreamUnit* u){
	UI32 i; NBString_empty(&obj->moov.sps);
	if(u->desc->hdr.prefixSz > 0){
		NBString_concatBytes(&obj->moov.sps, u->desc->hdr.prefix, u->desc->hdr.prefixSz);
	}
	for(i = 0; i < u->desc->chunksSz; i++){
		const STNBDataPtr* ptr = &u->desc->chunks[i];
		NBString_concatBytes(&obj->moov.sps, ptr->ptr, ptr->use);
	}
}

void TSStreamBuilderMp4_setPps(STTSStreamBuilderMp4* obj, const STTSStreamUnit* u){
	UI32 i;
	NBString_empty(&obj->moov.pps);
	if(u->desc->hdr.prefixSz > 0){
		NBString_concatBytes(&obj->moov.pps, u->desc->hdr.prefix, u->desc->hdr.prefixSz);
	}
	for(i = 0; i < u->desc->chunksSz; i++){
		const STNBDataPtr* ptr = &u->desc->chunks[i];
		NBString_concatBytes(&obj->moov.pps, ptr->ptr, ptr->use);
	}
}

void TSStreamBuilderMp4_buildFtyp(STTSStreamBuilderMp4* obj){
	if(obj->ftyp == NULL){
		obj->ftyp = NBMemory_allocType(STNBMp4FileTypeBox);
		NBMp4FileTypeBox_init(obj->ftyp);
	}
	NBMp4FileTypeBox_setMajorBrand(obj->ftyp, "mp42", 1);
	NBMp4FileTypeBox_addCompatibleBrand(obj->ftyp, "mp41");
	NBMp4FileTypeBox_addCompatibleBrand(obj->ftyp, "mp42");
	NBMp4FileTypeBox_addCompatibleBrand(obj->ftyp, "isom");
	NBMp4FileTypeBox_addCompatibleBrand(obj->ftyp, "hlsf");
}

void TSStreamBuilderMp4_buildMoov(STTSStreamBuilderMp4* obj, const STTSStreamUnit* unitFnd){
	//create once
	if(obj->moov.box == NULL){
		STNBMp4TrackHeaderBox* tkhd = NULL;
		STNBMp4AVCSampleEntry* avc1 = NULL;
		STNBMp4AVCConfigurationBox* avcC = NULL;
		STNBMp4BoxRef moovRef;
		NBMp4BoxRef_init(&moovRef);
		if(NBMp4MovieBox_allocRef(&moovRef)){ 
			//'mvhd'
			{
				STNBMp4BoxRef mvhdRef;
				NBMp4BoxRef_init(&mvhdRef);
				if(NBMp4MovieHeaderBox_allocRef(&mvhdRef)){
					STNBMp4MovieHeaderBox* mvhd = (STNBMp4MovieHeaderBox*)mvhdRef.box;
					NBMp4MovieHeaderBox_setTimescale(mvhd, 1000ULL);
					NBMp4MovieHeaderBox_setNextTrackId(mvhd, 2);
					//add
					NBMp4Box_addChildBoxByOwning(moovRef.box, &mvhdRef);
				}
				NBMp4BoxRef_release(&mvhdRef);
			}
			//'trak'
			{
				STNBMp4BoxRef trakRef;
				NBMp4BoxRef_init(&trakRef);
				if(NBMp4TrackBox_allocRef(&trakRef)){
					//'tkhd'
					{
						STNBMp4BoxRef tkhdRef;
						NBMp4BoxRef_init(&tkhdRef);
						if(NBMp4TrackHeaderBox_allocRef(&tkhdRef)){
							tkhd = (STNBMp4TrackHeaderBox*)tkhdRef.box;
							NBMp4TrackHeaderBox_setIsEnabled(tkhd, TRUE);
							NBMp4TrackHeaderBox_setTrackId(tkhd, 1);
							//add
							NBMp4Box_addChildBoxByOwning(trakRef.box, &tkhdRef);
						}
						NBMp4BoxRef_release(&tkhdRef);
					}
					//'mdia'
					{
						STNBMp4BoxRef mdiaRef;
						NBMp4BoxRef_init(&mdiaRef);
						if(NBMp4MediaBox_allocRef(&mdiaRef)){
							//'mdhd'
							{
								STNBMp4BoxRef mdhdRef;
								NBMp4BoxRef_init(&mdhdRef);
								if(NBMp4MediaHeaderBox_allocRef(&mdhdRef)){
									STNBMp4MediaHeaderBox* mdhd = (STNBMp4MediaHeaderBox*)mdhdRef.box;
									NBMp4MediaHeaderBox_setTimescale(mdhd, 1000ULL);
									//add
									NBMp4Box_addChildBoxByOwning(mdiaRef.box, &mdhdRef);
								}
								NBMp4BoxRef_release(&mdhdRef);
							}
							//'hdlr'
							{
								STNBMp4BoxRef hdlrRef;
								NBMp4BoxRef_init(&hdlrRef);
								if(NBMp4HandlerBox_allocRef(&hdlrRef)){
									STNBMp4HandlerBox* hdlr = (STNBMp4HandlerBox*)hdlrRef.box;
									NBMp4HandlerBox_setHandlerType(hdlr, "vide", "Core Media Video");
									//add
									NBMp4Box_addChildBoxByOwning(mdiaRef.box, &hdlrRef);
								}
								NBMp4BoxRef_release(&hdlrRef);
							}
							//'minf'
							{
								STNBMp4BoxRef minfRef;
								NBMp4BoxRef_init(&minfRef);
								if(NBMp4MediaInformationBox_allocRef(&minfRef)){
									//'vmhd'
									{
										STNBMp4BoxRef vmhdRef;
										NBMp4BoxRef_init(&vmhdRef);
										if(NBMp4VideoMediaHeaderBox_allocRef(&vmhdRef)){
											//STNBMp4VideoMediaHeaderBox* vmhd = (STNBMp4VideoMediaHeaderBox*)vmhdRef.box;
											//add
											NBMp4Box_addChildBoxByOwning(minfRef.box, &vmhdRef);
										}
										NBMp4BoxRef_release(&vmhdRef);
									}
									//'dinf'
									{
										STNBMp4BoxRef dinfRef;
										NBMp4BoxRef_init(&dinfRef);
										if(NBMp4DataInformationBox_allocRef(&dinfRef)){
											//'dref'
											{
												STNBMp4BoxRef drefRef;
												NBMp4BoxRef_init(&drefRef);
												if(NBMp4DataReferenceBox_allocRef(&drefRef)){
													STNBMp4DataReferenceBox* dref = (STNBMp4DataReferenceBox*)drefRef.box;
													NBMp4DataReferenceBox_addEntry(dref, NULL, NULL);
													//add
													NBMp4Box_addChildBoxByOwning(dinfRef.box, &drefRef);
												}
												NBMp4BoxRef_release(&drefRef);
											}
											//add
											NBMp4Box_addChildBoxByOwning(minfRef.box, &dinfRef);
										}
										NBMp4BoxRef_release(&dinfRef);
									}
									//'stbl'
									{
										STNBMp4BoxRef stblRef;
										NBMp4BoxRef_init(&stblRef);
										if(NBMp4SampleTableBox_allocRef(&stblRef)){
											//'stsd'
											{
												STNBMp4BoxRef stsdRef;
												NBMp4BoxRef_init(&stsdRef);
												if(NBMp4SampleDescriptionBox_allocRef(&stsdRef)){
													STNBMp4SampleDescriptionBox* stsd = (STNBMp4SampleDescriptionBox*)stsdRef.box; 
													//'avc1'
													{
														STNBMp4BoxRef avc1Ref;
														NBMp4BoxRef_init(&avc1Ref);
														if(NBMp4AVCSampleEntry_allocRef(&avc1Ref)){
															avc1 = (STNBMp4AVCSampleEntry*)avc1Ref.box;
															NBMp4AVCSampleEntry_setDataRefIndex(avc1, 1);
															NBMp4AVCSampleEntry_setFrameCount(avc1, 1);
															NBMp4AVCSampleEntry_setCompressorName(avc1, "Elemental H.264");
															NBMp4AVCSampleEntry_setDepth(avc1, 24); //Todo: calculate
															//'pasp'
															{
																STNBMp4BoxRef paspRef;
																NBMp4BoxRef_init(&paspRef);
																if(NBMp4PixelAspectRatioBox_allocRef(&paspRef)){
																	STNBMp4PixelAspectRatioBox* pasp = (STNBMp4PixelAspectRatioBox*)paspRef.box;
																	NBMp4PixelAspectRatioBox_setHSpacing(pasp, 1);
																	NBMp4PixelAspectRatioBox_setVSpacing(pasp, 1);
																	//add
																	NBMp4AVCSampleEntry_setPaspByOwning(avc1, &paspRef);
																}
																NBMp4BoxRef_release(&paspRef);
															}
															//'avcC'
															{
																STNBMp4BoxRef avcCRef;
																NBMp4BoxRef_init(&avcCRef);
																if(NBMp4AVCConfigurationBox_allocRef(&avcCRef)){
																	avcC = (STNBMp4AVCConfigurationBox*)avcCRef.box;
																	//add
																	NBMp4AVCSampleEntry_setAvcCByOwning(avc1, &avcCRef);
																}
																NBMp4BoxRef_release(&avcCRef);
															}
															//'btrt' (optional)
															/ *{
															 STNBMp4BoxRef btrtRef;
															 NBMp4BoxRef_init(&btrtRef);
															 if(NBMp4BitRateBox_allocRef(&btrtRef)){
															 STNBMp4BitRateBox* btrt = (STNBMp4BitRateBox*)btrtRef.box;
															 //add
															 NBMp4AVCSampleEntry_setBtrtByOwning(avc1, &btrtRef);
															 }
															 NBMp4BoxRef_release(&btrtRef);
															 }* /
															//add
															NBMp4SampleDescriptionBox_addEntryByOwning(stsd, &avc1Ref);
														}
														NBMp4BoxRef_release(&avc1Ref);
													}
													//add
													NBMp4Box_addChildBoxByOwning(stblRef.box, &stsdRef);
												}
												NBMp4BoxRef_release(&stsdRef);
											}
											//'stts'
											{
												STNBMp4BoxRef sttsRef;
												NBMp4BoxRef_init(&sttsRef);
												if(NBMp4TimeToSampleBox_allocRef(&sttsRef)){
													//STNBMp4TimeToSampleBox* stts = (STNBMp4TimeToSampleBox*)sttsRef.box;
													//add
													NBMp4Box_addChildBoxByOwning(stblRef.box, &sttsRef);
												}
												NBMp4BoxRef_release(&sttsRef);
											}
											//'stsc'
											{
												STNBMp4BoxRef stscRef;
												NBMp4BoxRef_init(&stscRef);
												if(NBMp4SampleToChunkBox_allocRef(&stscRef)){
													//STNBMp4SampleToChunkBox* stsc = (STNBMp4SampleToChunkBox*)stscRef.box;
													//add
													NBMp4Box_addChildBoxByOwning(stblRef.box, &stscRef);
												}
												NBMp4BoxRef_release(&stscRef);
											}
											//'stsz'
											{
												STNBMp4BoxRef stszRef;
												NBMp4BoxRef_init(&stszRef);
												if(NBMp4SampleSizeBox_allocRef(&stszRef)){
													//STNBMp4SampleSizeBox* stsz = (STNBMp4SampleSizeBox*)stszRef.box;
													//add
													NBMp4Box_addChildBoxByOwning(stblRef.box, &stszRef);
												}
												NBMp4BoxRef_release(&stszRef);
											}
											//'stco'
											{
												STNBMp4BoxRef stcoRef;
												NBMp4BoxRef_init(&stcoRef);
												if(NBMp4ChunkOffsetBox_allocRef(&stcoRef)){
													//STNBMp4ChunkOffsetBox* stco = (STNBMp4ChunkOffsetBox*)stcoRef.box;
													//add
													NBMp4Box_addChildBoxByOwning(stblRef.box, &stcoRef);
												}
												NBMp4BoxRef_release(&stcoRef);
											}
											//add
											NBMp4Box_addChildBoxByOwning(minfRef.box, &stblRef);
										}
										NBMp4BoxRef_release(&stblRef);
									}
									//add
									NBMp4Box_addChildBoxByOwning(mdiaRef.box, &minfRef);
								}
								NBMp4BoxRef_release(&minfRef);
							}
							//add
							NBMp4Box_addChildBoxByOwning(trakRef.box, &mdiaRef);
						}
						NBMp4BoxRef_release(&mdiaRef);
					}
					//add
					NBMp4Box_addChildBoxByOwning(moovRef.box, &trakRef);
				}
				NBMp4BoxRef_release(&trakRef);
			}
			//'mvex' (required to enable fragmented mp4)
			{
				STNBMp4BoxRef mvexRef;
				NBMp4BoxRef_init(&mvexRef);
				if(NBMp4MovieExtendsBox_allocRef(&mvexRef)){
					//'trex'
					{
						STNBMp4BoxRef trexRef;
						NBMp4BoxRef_init(&trexRef);
						if(NBMp4TrackExtendsBox_allocRef(&trexRef)){
							STNBMp4TrackExtendsBox* trex = (STNBMp4TrackExtendsBox*)trexRef.box;
							NBMp4TrackExtendsBox_setTrackId(trex, 1);
							NBMp4TrackExtendsBox_setDefaultSampleDescriptionIndex(trex, 1);
							{
								STNBMp4SampleFlags flags;
								NBMp4SampleFlags_init(&flags);
								NBMp4SampleFlags_setSampleDependsOn(&flags, ENNBMp4SampleFlagsDependedsOn_Depends);
								NBMp4SampleFlags_setSampleIsNonSyncSample(&flags, TRUE);
								NBMp4TrackExtendsBox_setDefaultSampleFlags(trex, &flags);
								NBMp4SampleFlags_release(&flags);
							}
							//add
							NBMp4Box_addChildBoxByOwning(mvexRef.box, &trexRef);
						}
						NBMp4BoxRef_release(&trexRef);
					}
					//add
					NBMp4Box_addChildBoxByOwning(moovRef.box, &mvexRef);
				}
				NBMp4BoxRef_release(&mvexRef);
			}
			//set as current 'moov'
			{
				obj->moov.box = (STNBMp4MovieBox*)moovRef.box; moovRef.box = NULL; NBASSERT(obj->moov.box != NULL)
				obj->moov.trak.tkhd.box = tkhd; NBASSERT(tkhd != NULL)
				obj->moov.trak.tkhd.mdia.minf.stbl.stsd.avc1.box = avc1; NBASSERT(avc1 != NULL)
				obj->moov.trak.tkhd.mdia.minf.stbl.stsd.avc1.avcC.box = avcC; NBASSERT(avcC != NULL)
			}
		}
		NBMp4BoxRef_release(&moovRef);
	}
	//populate
	if(obj->moov.box != NULL){
		//tkhd
		if(obj->moov.trak.tkhd.box != NULL){
			STNBMp4TrackHeaderBox* tkhd = obj->moov.trak.tkhd.box;
			if(unitFnd->desc->picProps.w == 0){
				NBMp4TrackHeaderBox_setWidth(tkhd, 0);
				NBMp4TrackHeaderBox_setHeight(tkhd, 0);
			} else {
				NBMp4TrackHeaderBox_setWidth(tkhd, unitFnd->desc->picProps.w);
				NBMp4TrackHeaderBox_setHeight(tkhd, unitFnd->desc->picProps.h);
			}
		}
		//avc1
		if(obj->moov.trak.tkhd.mdia.minf.stbl.stsd.avc1.box != NULL){
			STNBMp4AVCSampleEntry* avc1 = obj->moov.trak.tkhd.mdia.minf.stbl.stsd.avc1.box;
			if(unitFnd->desc->picProps.w == 0){
				NBMp4AVCSampleEntry_setWidth(avc1, 0);
				NBMp4AVCSampleEntry_setHeight(avc1, 0);
			} else {
				NBMp4AVCSampleEntry_setWidth(avc1, unitFnd->desc->picProps.w);
				NBMp4AVCSampleEntry_setHeight(avc1, unitFnd->desc->picProps.h);
			}
		}
		//avcC
		if(obj->moov.trak.tkhd.mdia.minf.stbl.stsd.avc1.avcC.box != NULL){
			STNBMp4AVCConfigurationBox* avcC = obj->moov.trak.tkhd.mdia.minf.stbl.stsd.avc1.avcC.box;
			//
			if(unitFnd->desc->picProps.w == 0){
				NBMp4AVCConfigurationBox_setAVCProfileIndication(avcC, 0);
				NBMp4AVCConfigurationBox_setProfileCompatibility(avcC, 0);
				NBMp4AVCConfigurationBox_setProfileLevelIndication(avcC, 0);
			} else {
				NBMp4AVCConfigurationBox_setAVCProfileIndication(avcC, unitFnd->desc->picProps.avcProfile);
				NBMp4AVCConfigurationBox_setProfileCompatibility(avcC, unitFnd->desc->picProps.profConstraints);
				NBMp4AVCConfigurationBox_setProfileLevelIndication(avcC, unitFnd->desc->picProps.profLevel);
			}
			//
			NBMp4AVCConfigurationBox_setLengthSize(avcC, 4); //sizeof of length value injected before each NALU.
			//
			if(obj->moov.sps.length <= 0){
				NBMp4AVCConfigurationBox_emptySps(avcC);
			} else if(avcC->avcCfg.numOfSequenceParameterSets < 1){
				NBMp4AVCConfigurationBox_addSps(avcC, obj->moov.sps.str, obj->moov.sps.length);
			} else {
				NBMp4AVCConfigurationBox_updateSps(avcC, 0, obj->moov.sps.str, obj->moov.sps.length);
			}
			//
			if(obj->moov.pps.length <= 0){
				NBMp4AVCConfigurationBox_emptyPps(avcC);
			} else if(avcC->avcCfg.numOfPictureParameterSets < 1){
				NBMp4AVCConfigurationBox_addPps(avcC, obj->moov.pps.str, obj->moov.pps.length);
			} else {
				NBMp4AVCConfigurationBox_updatePps(avcC, 0, obj->moov.pps.str, obj->moov.pps.length);
			}
			//
			if(unitFnd->desc->picProps.w == 0){
				NBMp4AVCConfigurationBox_setChromaFormat(avcC, 0); //bit(2)
				NBMp4AVCConfigurationBox_setBitDepthLuma(avcC, 0); //bit(3) + 8
				NBMp4AVCConfigurationBox_setBitDepthChroma(avcC, 0); //bit(3) + 8
			} else {
				NBMp4AVCConfigurationBox_setChromaFormat(avcC, unitFnd->desc->picProps.chromaFmt); //bit(2)
				if(unitFnd->desc->picProps.bitDepthLuma >= 8){
					NBMp4AVCConfigurationBox_setBitDepthLuma(avcC, unitFnd->desc->picProps.bitDepthLuma); //bit(3) + 8
				}
				if(unitFnd->desc->picProps.bitDepthChroma >= 8){
					NBMp4AVCConfigurationBox_setBitDepthChroma(avcC, unitFnd->desc->picProps.bitDepthChroma); //bit(3) + 8
				}
			}
		}
		//picProps
		if(unitFnd->desc->picProps.w == 0){
			if(obj->moov.picProps != NULL){
				NBAvcPicDescBase_release(obj->moov.picProps);
				NBMemory_free(obj->moov.picProps);
				obj->moov.picProps = NULL;
			}
		} else {
			if(obj->moov.picProps == NULL){
				obj->moov.picProps = NBMemory_allocType(STNBAvcPicDescBase);
				NBAvcPicDescBase_init(obj->moov.picProps);
			}
			NBAvcPicDescBase_setAsOther(obj->moov.picProps, &unitFnd->desc->picProps);
		}
	}
}

BOOL TSStreamBuilderMp4_hasMoof(const STTSStreamBuilderMp4* obj){
	return (obj->moof.box != NULL); 
}

void TSStreamBuilderMp4_buildMoof(STTSStreamBuilderMp4* obj, const STTSStreamUnit* unitFnd){
	//alloc (if necesary)
	if(obj->moof.box == NULL){
		STNBMp4BoxRef moofRef;
		NBMp4BoxRef_init(&moofRef);
		if(NBMp4MovieFragmentBox_allocRef(&moofRef)){
			STNBMp4MovieFragmentHeaderBox* mfhd = NULL;
			STNBMp4TrackFragmentBaseMediaDecodeTimeBox* tfdt = NULL;
			STNBMp4TrackRunBox* trun = NULL;
			STNBMp4TrackFragmentHeaderBox* tfhd = NULL;
			//'mfhd'
			{
				STNBMp4BoxRef mfhdRef;
				NBMp4BoxRef_init(&mfhdRef);
				if(NBMp4MovieFragmentHeaderBox_allocRef(&mfhdRef)){
					mfhd = (STNBMp4MovieFragmentHeaderBox*)mfhdRef.box;
					//add
					NBMp4Box_addChildBoxByOwning(moofRef.box, &mfhdRef);
				}
				NBMp4BoxRef_release(&mfhdRef);
			}
			//'traf'
			{
				STNBMp4BoxRef trafRef;
				NBMp4BoxRef_init(&trafRef);
				if(NBMp4TrackFragmentBox_allocRef(&trafRef)){
					//'tfhd'
					{
						STNBMp4BoxRef tfhdRef;
						NBMp4BoxRef_init(&tfhdRef);
						if(NBMp4TrackFragmentHeaderBox_allocRef(&tfhdRef)){
							tfhd = (STNBMp4TrackFragmentHeaderBox*)tfhdRef.box;
							NBMp4TrackFragmentHeaderBox_setTrackId(tfhd, 1);
							NBMp4TrackFragmentHeaderBox_setDefaultBaseIsMoof(tfhd, TRUE); //all offsets must be calculated from moof-box's start.
							//add
							NBMp4Box_addChildBoxByOwning(trafRef.box, &tfhdRef);
						}
						NBMp4BoxRef_release(&tfhdRef);
					}
					//'tfdt'
					{
						STNBMp4BoxRef tfdtRef;
						NBMp4BoxRef_init(&tfdtRef);
						if(NBMp4TrackFragmentBaseMediaDecodeTimeBox_allocRef(&tfdtRef)){
							tfdt = (STNBMp4TrackFragmentBaseMediaDecodeTimeBox*)tfdtRef.box;
							//add
							NBMp4Box_addChildBoxByOwning(trafRef.box, &tfdtRef);
						}
						NBMp4BoxRef_release(&tfdtRef);
					}
					//'trun'
					{
						STNBMp4BoxRef trunRef;
						NBMp4BoxRef_init(&trunRef);
						if(NBMp4TrackRunBox_allocRef(&trunRef)){
							trun = (STNBMp4TrackRunBox*)trunRef.box;
							NBMp4TrackRunBox_setDataOffset(trun, 1); //reserve offset space (will be overwritten latter)
							//add
							NBMp4Box_addChildBoxByOwning(trafRef.box, &trunRef);
						}
						NBMp4BoxRef_release(&trunRef);
					}
					//add
					NBMp4Box_addChildBoxByOwning(moofRef.box, &trafRef);
				}
				NBMp4BoxRef_release(&trafRef);
			}
			//set as curretn 'moof'
			{
				obj->moof.box					= (STNBMp4MovieFragmentBox*)moofRef.box; moofRef.box = NULL; NBASSERT(obj->moof.box != NULL)
				obj->moof.mfhd.box				= mfhd; NBASSERT(mfhd != NULL)
				obj->moof.traf.tfdt.box			= tfdt; NBASSERT(tfdt != NULL)
				obj->moof.traf.tfhd.box			= tfhd; NBASSERT(tfhd != NULL)
				obj->moof.traf.trun.box			= trun; NBASSERT(trun != NULL)
			}
		}
		NBMp4BoxRef_release(&moofRef);
	}
	//populate
	if(obj->moof.box != NULL){
		//mfhd
		if(obj->moof.mfhd.box != NULL){
			STNBMp4MovieFragmentHeaderBox* mfhd = obj->moof.mfhd.box;
			NBMp4MovieFragmentHeaderBox_setSeqNumber(mfhd, obj->moof.sent.nextSeqNum);
		}
		//tfdt
		if(obj->moof.traf.tfdt.box != NULL){
			STNBMp4TrackFragmentBaseMediaDecodeTimeBox* tfdt = obj->moof.traf.tfdt.box;
			NBMp4TrackFragmentBaseMediaDecodeTimeBox_setBaseMediaDecodeTime(tfdt, obj->moof.sent.nextBaseScaledTime);
		}
		//tfhd
		if(obj->moof.traf.tfhd.box != NULL){
			//STNBMp4TrackFragmentHeaderBox* tfhd = obj->moof.traf.tfhd.box;
		}
		if(obj->moof.traf.trun.box != NULL){
			STNBMp4TrackRunBox* trun = obj->moof.traf.trun.box;
			NBMp4TrackRunBox_entriesEmpty(trun);
		}
		//continue relative to last unit sent
		TSStreamBuilderMoofState_setAsOther(&obj->moof.accum, &obj->moof.sent);
		obj->moof.accum.units.accumCount = 0;
	}
}

BOOL TSStreamBuilderMp4_mdatQueueIsEmpty(STTSStreamBuilderMp4* obj){
	NBASSERT((obj->mdat.units.arr.use == 0 && obj->mdat.units.payBytesTotal == 0) || (obj->mdat.units.arr.use != 0 && obj->mdat.units.payBytesTotal != 0))
	return (obj->mdat.units.arr.use == 0);
}

void TSStreamBuilderMp4_mdatQueueAddOwning(STTSStreamBuilderMp4* obj, const UI32 msUnitDuration, STTSStreamUnit* u, const UI32 extraPrefixSz){
#	ifdef NB_CONFIG_INCLUDE_ASSERTS
	{
		SI32 i; for(i = (SI32)obj->mdat.units.arr.use - 1; i >= 0; i--){
			STTSStreamUnit* u2 = NBArray_itmPtrAtIndex(&obj->mdat.units.arr, STTSStreamUnit, i);
			NBASSERT(u->desc != u2->desc) //repeated unit found
		}
	}
#	endif
	//arr
	{
		NBArray_addValue(&obj->mdat.units.arr, *u);
		obj->mdat.units.payBytesTotal	+= extraPrefixSz + u->desc->hdr.prefixSz + u->desc->chunksTotalSz;
		obj->mdat.units.scaledDuration	+= msUnitDuration;
	}
	//own data
	TSStreamUnit_resignToData(u);
}

BOOL TSStreamBuilderMp4_addPendUnitsToMoof(STTSStreamBuilderMp4* obj){
	BOOL r = FALSE;
	if(obj->moof.traf.tfhd.box != NULL && obj->moof.traf.trun.box != NULL && obj->mdat.hdr != NULL && obj->mdat.units.consumed.count < obj->mdat.units.arr.use){
		NBASSERT(obj->mdat.units.consumed.payBytesCount < obj->mdat.units.payBytesTotal)
		const STTSStreamUnit* u = NBArray_itmPtrAtIndex(&obj->mdat.units.arr, STTSStreamUnit, obj->mdat.units.arr.use - 1);
		NBASSERT(u->desc != NULL)
		//update 'moof' / 'traf' / 'tfhd' with default values
		if(obj->moof.accum.units.accumCount == 0){ //is first unit at group ('moof')
			if(u->desc->picProps.w != 0 && u->desc->picProps.fpsMax > 0){
				NBMp4TrackFragmentHeaderBox_setDefaultSampleDuration(obj->moof.traf.tfhd.box, (UI32)(1000U / u->desc->picProps.fpsMax));
			}
			NBMp4TrackFragmentHeaderBox_setDefaultSampleSize(obj->moof.traf.tfhd.box, (obj->mdat.units.payBytesTotal - obj->mdat.units.consumed.payBytesCount));
			{
				STNBMp4SampleFlags defaultSampleFlags;
				NBMp4SampleFlags_init(&defaultSampleFlags);
				NBMp4TrackFragmentHeaderBox_setDefaultSampleFlags(obj->moof.traf.tfhd.box, &defaultSampleFlags);
				NBMp4SampleFlags_release(&defaultSampleFlags);
			}
		}
		//concat to 'moof' / 'traf' / 'trun'
		{
			STNBMp4SampleFlags sampleFlags;
			NBMp4SampleFlags_init(&sampleFlags);
			if(u->desc->isFramePayload){
				NBMp4SampleFlags_setSampleDependsOn(&sampleFlags, u->desc->isKeyframe ? ENNBMp4SampleFlagsDependedsOn_Independent: ENNBMp4SampleFlagsDependedsOn_Depends);
				NBMp4SampleFlags_setSampleIsNonSyncSample(&sampleFlags, !u->desc->isKeyframe);
			}
			//add
			{
				NBASSERT(obj->mdat.units.consumed.payBytesCount <= obj->mdat.units.payBytesTotal)
				NBMp4TrackRunBox_entriesAdd(obj->moof.traf.trun.box, obj->mdat.units.scaledDuration, (obj->mdat.units.payBytesTotal - obj->mdat.units.consumed.payBytesCount), &sampleFlags, 0);
			}
			//save state (accum)
			{
				obj->moof.accum.nextBaseScaledTime		+= obj->mdat.units.scaledDuration; 
				obj->moof.accum.units.globalCount++;
				obj->moof.accum.units.accumCount++;
				//
				obj->mdat.units.consumed.payBytesCount	= obj->mdat.units.payBytesTotal;
				obj->mdat.units.consumed.count			= obj->mdat.units.arr.use;
				obj->mdat.units.scaledDuration			= 0;
			}
			NBMp4SampleFlags_release(&sampleFlags);
		}
		//result
		r = TRUE;
	}
	return r;
}

void TSStreamBuilderMp4_sendFtypAndMoov(STTSStreamBuilderMp4* obj, const STTSStreamUnit* u, STTSStreamBuilderWriter* dst){
	NBString_empty(&obj->tmp.str);
	//build 'ftyp' header (once)
	{
		if(obj->ftyp == NULL){
			TSStreamBuilderMp4_buildFtyp(obj);
		}
		//concat to buffer
		if(obj->ftyp != NULL){
			NBMp4FileTypeBox_writeBits(obj->ftyp, 0, NB_OBJREF_NULL, &obj->tmp.str);
		}
	}
	//build 'moov' header (once)
	{
		TSStreamBuilderMp4_buildMoov(obj, u);
		//concat to buffer
		if(obj->moov.box != NULL){
			NBMp4MovieBox_writeBits(obj->moov.box, 0, NB_OBJREF_NULL, &obj->tmp.str);
		}
	}
	//add string-slot
	{
		//PRINTF_WARNING("TSStreamBuilder, starting new 'moov' header.\n");
		BYTE prefix[24];
		UI32 prefixSz = 0;
		//msgHeader
		switch (obj->opq->cfg.msgsWrapper) {
			case ENTSStreamBuilderMessageWrapper_None:
				break;
			case ENTSStreamBuilderMessageWrapper_WebSocket:
				prefixSz += NBWebSocketFrame_writeHeader(ENNBWebSocketFrameOpCode_Binary, 0, obj->tmp.str.length, TRUE, TRUE, &prefix[prefixSz]);
				break;	
			default:
				NBASSERT(FALSE) //implementation error
				break;
		}
		TSStreamBuilder_queueStringWithPrefixOpq_(obj->opq, &obj->tmp.str, prefix, prefixSz, dst);
	}
}

void TSStreamBuilderMp4_sendMoofAndMdat(STTSStreamBuilderMp4* obj, STTSStreamUnitsReleaser* optUnitsReleaser, STTSStreamBuilderWriter* dst){
	if(obj->moof.box != NULL && obj->mdat.hdr != NULL && obj->mdat.units.arr.use > 0){
		BOOL msgIsOpen = FALSE; BYTE prefix[24]; UI32 prefixSz = 0;
		//build 'moof' and 'mdat' hdr
		{
			NBString_empty(&obj->tmp.str);
			//build
			{
				const UI32 moofPos = obj->tmp.str.length; NBASSERT(moofPos == 0)
				UI32 moofLen = 0, mdatHdrSz = 0;
				//write 'moof'
				NBMp4MovieFragmentBox_writeBits(obj->moof.box, 0, NB_OBJREF_NULL, &obj->tmp.str);
				moofLen = obj->tmp.str.length - moofPos;
				//write 'mdat'
				{
					const UI32 iBoxPos = obj->tmp.str.length;
					NBMp4BoxHdr_writeBits(obj->mdat.hdr, 0, NB_OBJREF_NULL, &obj->tmp.str);
					mdatHdrSz = (obj->tmp.str.length - iBoxPos);
					NBMp4BoxHdr_writeHdrFinalSizeExplicit(obj->mdat.hdr, 0, NB_OBJREF_NULL, &obj->tmp.str, obj->tmp.str.length + obj->mdat.units.consumed.payBytesCount);
				}
				//update 'moof' / 'traf' / 'trun' offset
				{
					NBMp4TrackRunBox_overwriteDataOffset(obj->moof.traf.trun.box, (SI32)(moofLen + mdatHdrSz), &obj->tmp.str);
				}	
			}
			//add string-slot
			{
				//PRINTF_WARNING("TSStreamBuilder, starting new 'moof' header.\n");
				//msgHeader
				prefixSz = 0;
				switch (obj->opq->cfg.msgsWrapper) {
					case ENTSStreamBuilderMessageWrapper_None:
						break;
					case ENTSStreamBuilderMessageWrapper_WebSocket:
						msgIsOpen	= (obj->mdat.units.consumed.count > 0);
						prefixSz	+= NBWebSocketFrame_writeHeader(ENNBWebSocketFrameOpCode_Binary, 0, obj->tmp.str.length, TRUE, !msgIsOpen, &prefix[prefixSz]);
						break;	
					default:
						NBASSERT(FALSE) //implementation error
						break;
				}
				TSStreamBuilder_queueStringWithPrefixOpq_(obj->opq, &obj->tmp.str, prefix, prefixSz, dst);
			}
		}
		//add units-slots
		{
			NBASSERT(obj->mdat.units.consumed.count <= obj->mdat.units.arr.use)
			NBASSERT(obj->mdat.units.consumed.payBytesCount <= obj->mdat.units.payBytesTotal)
			UI32 bytesAddedCount = 0; //dbg
			UI32 i; for(i = 0; i < obj->mdat.units.consumed.count && i < obj->mdat.units.arr.use; i++){
				STTSStreamUnit* u = NBArray_itmPtrAtIndex(&obj->mdat.units.arr, STTSStreamUnit, i);
				const UI32 len = u->desc->hdr.prefixSz + u->desc->chunksTotalSz;
				prefixSz = 0;
				//msg-wrapper
				switch (obj->opq->cfg.msgsWrapper) {
					case ENTSStreamBuilderMessageWrapper_None:
						break;
					case ENTSStreamBuilderMessageWrapper_WebSocket:
						NBASSERT(msgIsOpen)
						msgIsOpen	= ((i + 1) < obj->mdat.units.consumed.count);
						prefixSz	+= NBWebSocketFrame_writeHeader(ENNBWebSocketFrameOpCode_Binary, 0, 4 + u->desc->hdr.prefixSz + u->desc->chunksTotalSz, FALSE, !msgIsOpen, &prefix[prefixSz]);
						break;	
					default:
						NBASSERT(FALSE) //implementation error
						break;
				}
				//unit-prefix 
				NBMp4_writeUI32At(&len, &prefix[prefixSz]);
				prefixSz += 4;
				//
				bytesAddedCount += (4 + u->desc->hdr.prefixSz + u->desc->chunksTotalSz);
				TSStreamBuilder_queueUnitSlotWithPrefixOpq_(obj->opq, u, prefix, prefixSz, dst);
			}
			NBASSERT(!msgIsOpen) //should be closed
			NBASSERT(i == obj->mdat.units.consumed.count) //should be equal
			NBASSERT(bytesAddedCount == obj->mdat.units.consumed.payBytesCount) //should be equal
			//
			obj->mdat.units.payBytesTotal -= (obj->mdat.units.consumed.payBytesCount <= obj->mdat.units.payBytesTotal ? obj->mdat.units.consumed.payBytesCount : obj->mdat.units.payBytesTotal);
			//release
			if(obj->mdat.units.consumed.count > 0){
				STTSStreamUnit* u = NBArray_dataPtr(&obj->mdat.units.arr, STTSStreamUnit);
				TSStreamUnit_unitsReleaseGrouped(u, obj->mdat.units.consumed.count, optUnitsReleaser);
			}
			//remove
			if(obj->mdat.units.consumed.count < obj->mdat.units.arr.use){
				NBArray_removeItemsAtIndex(&obj->mdat.units.arr, 0, obj->mdat.units.consumed.count);
			} else {
				NBArray_empty(&obj->mdat.units.arr);
				NBASSERT(obj->mdat.units.payBytesTotal == 0)
			}
			obj->mdat.units.consumed.count = 0;
			obj->mdat.units.consumed.payBytesCount = 0;
		}
		//save state (sent)
		{
			TSStreamBuilderMoofState_setAsOther(&obj->moof.sent, &obj->moof.accum);
			obj->moof.sent.nextSeqNum++; 
		}
		//reset
		{
			if(obj->moof.box != NULL){
				NBMp4MovieFragmentBox_release(obj->moof.box);
				NBMemory_free(obj->moof.box);
				obj->moof.box				= NULL;
				obj->moof.traf.tfhd.box		= NULL;
				obj->moof.traf.trun.box		= NULL;
			}
		}
	}
}
*/

