//
//  TSRequestsMap.c
//  thinstream
//
//  Created by Marcos Ortega on 8/24/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/TSRequestsMap.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBCompare.h"
#include "nb/core/NBString.h"
#include "nb/core/NBArraySorted.h"

//Logics
#include "core/logic/TSHello.h"

//enumMap

STNBEnumMapRecord TSRequestId_sharedEnumMapRecs[] = {
	//Hello
	{ ENTSRequestId_Hello, "ENTSRequestId_Hello", "/hello" }
	//
	, {	ENTSRequestId_Count, "ENTSRequestId_Count", "" }
};

STNBEnumMap TSRequestId_sharedEnumMap = {
	"ENTSRequestId"
	, TSRequestId_sharedEnumMapRecs
	, (sizeof(TSRequestId_sharedEnumMapRecs) / sizeof(TSRequestId_sharedEnumMapRecs[0]))
};

const STNBEnumMap* TSRequestId_getSharedEnumMap(void){
	return &TSRequestId_sharedEnumMap;
}

BOOL NBCompare_SSRequestsMapRecordRes(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz){
	NBASSERT(dataSz == sizeof(STNBEnumMapRecord))
	if(dataSz == sizeof(STNBEnumMapRecord)){
		const STNBEnumMapRecord* o1 = (const STNBEnumMapRecord*)data1;
		const STNBEnumMapRecord* o2 = (const STNBEnumMapRecord*)data2;
		switch (mode) {
			case ENCompareMode_Equal:
				return NBString_strIsEqual(o1->strValue, o2->strValue) ? TRUE : FALSE;
			case ENCompareMode_Lower:
				return NBString_strIsLower(o1->strValue, o2->strValue) ? TRUE : FALSE;
			case ENCompareMode_LowerOrEqual:
				return NBString_strIsLowerOrEqual(o1->strValue, o2->strValue) ? TRUE : FALSE;
			case ENCompareMode_Greater:
				return NBString_strIsGreater(o1->strValue, o2->strValue) ? TRUE : FALSE;
			case ENCompareMode_GreaterOrEqual:
				return NBString_strIsGreaterOrEqual(o1->strValue, o2->strValue) ? TRUE : FALSE;
			default:
				NBASSERT(FALSE)
				break;
		}
	}
	return FALSE;
}

typedef struct STTSRequestsMapOpq_ {
	STTSRequestsMapItm	itms[ENTSRequestId_Count];
	STNBArraySorted		idxByRes;
} STTSRequestsMapOpq;

void TSRequestsMap_init(STTSRequestsMap* obj){
	STTSRequestsMapOpq* opq = obj->opaque = NBMemory_allocType(STTSRequestsMapOpq);
	NBMemory_setZeroSt(*opq, STTSRequestsMapOpq);
	{
		SI32 i; for(i = 0; i < ENTSRequestId_Count; i++){
			opq->itms[i].reqId		= (ENTSRequestId)i;
			opq->itms[i].resource	= TSRequestId_sharedEnumMapRecs[i].strValue;
		}
	}
	NBArraySorted_init(&opq->idxByRes, sizeof(STNBEnumMapRecord), NBCompare_SSRequestsMapRecordRes);
	{
		const UI32 arrSz = (sizeof(TSRequestId_sharedEnumMapRecs) / sizeof(TSRequestId_sharedEnumMapRecs[0]));
		NBASSERT(arrSz == (ENTSRequestId_Count + 1)) //Map is incomplete or older ".h" version was loaded
		UI32 i; for(i = 0; i < arrSz; i++){
			NBASSERT(TSRequestId_sharedEnumMapRecs[i].intValue == i) //Map is not ordered; ordered is prefered.
			NBArraySorted_addValue(&opq->idxByRes, TSRequestId_sharedEnumMapRecs[i]);
		}
	}
	//-------------------------------
	//-- Register individual actions
	//-------------------------------
	{
		//Hello
		TSRequestsMap_setPropsById(obj, ENTSRequestId_Hello, TSHello_srvProcess, NULL, NULL, NULL, NULL, NULL, NULL);
	}
}

void TSRequestsMap_release(STTSRequestsMap* obj){
	STTSRequestsMapOpq* opq = (STTSRequestsMapOpq*)obj->opaque;
	NBArraySorted_release(&opq->idxByRes);
	NBMemory_free(obj->opaque);
	obj->opaque = NULL;
}

ENTSRequestId TSRequestsMap_getRequestIdByRes(const STTSRequestsMap* obj, const char* resource){
	ENTSRequestId r = ENTSRequestId_Count;
	STTSRequestsMapOpq* opq = (STTSRequestsMapOpq*)obj->opaque;
	STNBEnumMapRecord srch;
	srch.strValue = resource;
	const SI32 idx = NBArraySorted_indexOf(&opq->idxByRes, &srch, sizeof(srch), NULL);
	if(idx >= 0){
		r = NBArraySorted_itmPtrAtIndex(&opq->idxByRes, STNBEnumMapRecord, idx)->intValue;
	}
	return r;
}

STNBEnumMapRecord TSRequestsMap_getRequestRecordById(const STTSRequestsMap* obj, const ENTSRequestId reqId){
	NBASSERT(TSRequestId_sharedEnumMapRecs[reqId].intValue == reqId)
	return TSRequestId_sharedEnumMapRecs[reqId];
}

//


void TSRequestsMap_setPropsById(STTSRequestsMap* obj, const ENTSRequestId reqId, TSServerRequestMethod reqMethod, const STNBStructMap* reqBodyMap, TSClientResponseMethod respMethod, const STNBStructMap* respBodyMap, TSClientStreamStartMethod respStartMethod, TSClientStreamPeekMethod respPeekMethod, TSClientStreamEndMethod respEndMethod){
	STTSRequestsMapOpq* opq = (STTSRequestsMapOpq*)obj->opaque;
	if(reqId >= 0 && reqId < ENTSRequestId_Count){
		STTSRequestsMapItm* itm = &opq->itms[reqId];
		//Request struct
		itm->reqMethod 			= reqMethod;
		itm->reqBodyMap			= reqBodyMap;
		//Response struct
		itm->respMethod			= respMethod;
		itm->respBodyMap		= respBodyMap;
		//Response stream read
		itm->respStartMethod	= respStartMethod;
		itm->respPeekMethod		= respPeekMethod;
		itm->respEndMethod		= respEndMethod;
	}
}

const STTSRequestsMapItm* TSRequestsMap_getPropsById(const STTSRequestsMap* obj, const ENTSRequestId reqId){
	const STTSRequestsMapItm* r = NULL;
	STTSRequestsMapOpq* opq = (STTSRequestsMapOpq*)obj->opaque;
	if(reqId >= 0 && reqId < ENTSRequestId_Count){
		r = &opq->itms[reqId];
		NBASSERT(r->reqId == reqId);
	}
	return r;
}

/*UI32 TSRequestsMap_getCrc32Signature(const STTSRequestsMap* obj){
	UI32 r = 0;
	STTSRequestsMapOpq* opq = (STTSRequestsMapOpq*)obj->opaque;
	STNBCrc32 crc;
	NBCrc32_init(&crc);
	NBCrc32_feed(&crc, opq->itms, sizeof(opq->itms));
	NBCrc32_finish(&crc);
	r  = crc.hash;
	NBCrc32_release(&crc);
	return r;
}*/
