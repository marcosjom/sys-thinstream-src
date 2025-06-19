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
#include "nb/2d/NBAvc.h"
//
#include "core/logic/TSStreamUnitBuff.h"

//TSStreamUnit

void TSStreamUnit_release(STTSStreamUnit* obj, STNBDataPtrReleaser* optPtrsReleaser){
	//desc
	obj->desc = NULL;
	//release cur-buff
	if(TSStreamUnitBuff_isSet(obj->def.alloc)){
		TSStreamUnitBuff_release(&obj->def.alloc);
		TSStreamUnitBuff_null(&obj->def.alloc);
	}
}

BOOL NBCompare_TSStreamUnit_byDescPtrOnly(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz){
	NBASSERT(dataSz == sizeof(STTSStreamUnit))
	if(dataSz == sizeof(STTSStreamUnit)){
		switch (mode) {
			case ENCompareMode_Equal:
				return (((STTSStreamUnit*)data1)->desc == ((STTSStreamUnit*)data2)->desc);
			case ENCompareMode_Lower:
				return (((STTSStreamUnit*)data1)->desc < ((STTSStreamUnit*)data2)->desc);
			case ENCompareMode_LowerOrEqual:
				return (((STTSStreamUnit*)data1)->desc <= ((STTSStreamUnit*)data2)->desc);
			case ENCompareMode_Greater:
				return (((STTSStreamUnit*)data1)->desc > ((STTSStreamUnit*)data2)->desc);
			case ENCompareMode_GreaterOrEqual:
				return (((STTSStreamUnit*)data1)->desc >= ((STTSStreamUnit*)data2)->desc);
			default:
				NBASSERT(FALSE)
				break;
		}
	}
	NBASSERT(FALSE)
	return FALSE;
}

//

void TSStreamUnit_resignToData(STTSStreamUnit* obj){
	//desc
	obj->desc = NULL;
	//def
	TSStreamUnitBuff_null(&obj->def.alloc);
}

void TSStreamUnit_setAsOther(STTSStreamUnit* obj, STTSStreamUnit* other){
	//release cur-buff
	if(TSStreamUnitBuff_isSet(obj->def.alloc)){
		TSStreamUnitBuff_null(&obj->def.alloc);
		TSStreamUnitBuff_release(&obj->def.alloc);
	}
	//set
	if(other == NULL){
		NBMemory_setZeroSt(*obj, STTSStreamUnit);
	} else {
		//set and retain
		obj->desc	= other->desc;
		obj->def	= other->def;
		//retain
		if(TSStreamUnitBuff_isSet(obj->def.alloc)){
			TSStreamUnitBuff_retain(obj->def.alloc);
		}	
	}
}

//

void TSStreamUnit_unitsReleaseGrouped(STTSStreamUnit* objs, const UI32 objsSz, STTSStreamUnitsReleaser* optUnitsReleaser){
	if(optUnitsReleaser != NULL && optUnitsReleaser->itf.unitsReleaseGrouped != NULL){
		(*optUnitsReleaser->itf.unitsReleaseGrouped)(objs, objsSz, optUnitsReleaser->ptrsReleaser, optUnitsReleaser->usrData);
	} else {
		const STTSStreamUnit* objsAfterEnd = objs + objsSz;
		while(objs < objsAfterEnd){
			TSStreamUnit_release(objs, NULL);
			//next
			objs++;
		}
	}
}
