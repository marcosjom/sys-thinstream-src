//
//  TSStreamUnit.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSStreamUnit_h
#define TSStreamUnit_h

#include "nb/core/NBDataPtr.h"
#include "nb/core/NBArray.h"

#ifdef __cplusplus
extern "C" {
#endif

struct STTSStreamUnit_;

//TSStreamUnitBuffHdr
//stream unit header

typedef struct STTSStreamUnitBuffHdr_ {
	UI8				refIdc;		//
	UI8				type;		//Type of unit
	UI16			prefixSz;	//Prefix data size
	BYTE			prefix[4];	//Prefix data
	UI32			tsRtp;		//timestamp in first rtp package
} STTSStreamUnitBuffHdr;

//TSStreamUnitBuffDesc

typedef struct STTSStreamUnitBuffDesc_ {
	const void*			origin;			//unit's origin
	UI32				unitId;			//non-zero unit sequence number/id
	BOOL				isBlockStart;	//is NALU 7 (first of an stream-block)
	BOOL				isSeqReqHdr;	//is NALU 7 or 8 (sequence data required to render frames)
	BOOL				isFramePayload;	//is VCL unit (picture frame data)
	BOOL				isKeyframe;		//is IDR picture (full frame)
	STNBTimestampMicro	time;			//arrival time
	STTSStreamUnitBuffHdr hdr;			//header
	STNBDataPtr*		chunks;			//chunks
	UI32				chunksSz;		//chunksSz
	UI32				chunksTotalSz;	//all chunks total bytes 
} STTSStreamUnitBuffDesc;

//TSStreamUnitsReleaserItf
//For capturing packages before returning to pool.

typedef struct STTSStreamUnitsReleaserItf_ {
	void		(*unitsReleaseGrouped)(struct STTSStreamUnit_* objs, const UI32 size, STNBDataPtrReleaser* optPtrsReleaser, void* usrData);
} STTSStreamUnitsReleaserItf;

//TSStreamUnitsReleaser

typedef struct STTSStreamUnitsReleaser_ {
	STTSStreamUnitsReleaserItf	itf;
	void*						usrData;
	STNBDataPtrReleaser*		ptrsReleaser;
} STTSStreamUnitsReleaser;

//TSStreamUnitDef

typedef struct STTSStreamUnitDef_ {
	STNBObjRef				alloc;  //STTSStreamUnitBuffRef
} STTSStreamUnitDef;

//TSStreamUnit

typedef struct STTSStreamUnit_ {
	STTSStreamUnitBuffDesc*	desc;   //description
	STTSStreamUnitDef		def;    //allocator
} STTSStreamUnit;

#define TSStreamUnit_init(OBJ) NBMemory_setZeroSt(*(OBJ), STTSStreamUnit)
void TSStreamUnit_release(STTSStreamUnit* obj, STNBDataPtrReleaser* optPtrsReleaser);

BOOL NBCompare_TSStreamUnit_byDescPtrOnly(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz);

//
void TSStreamUnit_resignToData(STTSStreamUnit* obj);
void TSStreamUnit_setAsOther(STTSStreamUnit* obj, STTSStreamUnit* other);

//
void TSStreamUnit_unitsReleaseGrouped(STTSStreamUnit* objs, const UI32 objsSz, STTSStreamUnitsReleaser* optUnitsReleaser);

#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSStreamDefs_h */
