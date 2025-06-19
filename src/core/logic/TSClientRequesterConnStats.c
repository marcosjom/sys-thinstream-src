//
//  TSClientRequesterConnStats.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSClientRequesterConnStats.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBThreadMutex.h"
//
#include "nb/core/NBStructMaps.h"
#include "nb/core/NBMngrStructMaps.h"

// TSClientRequesterConnStatsTime

STNBStructMapsRec TSClientRequesterConnStatsTime_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSClientRequesterConnStatsTime_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSClientRequesterConnStatsTime_sharedStructMap);
    if(TSClientRequesterConnStatsTime_sharedStructMap.map == NULL){
        STTSClientRequesterConnStatsTime s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSClientRequesterConnStatsTime);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addUIntM(map, s, count);
        NBStructMap_addUIntM(map, s, msMin);
        NBStructMap_addUIntM(map, s, msMax);
        NBStructMap_addUIntM(map, s, msTotal);
        TSClientRequesterConnStatsTime_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSClientRequesterConnStatsTime_sharedStructMap);
    return TSClientRequesterConnStatsTime_sharedStructMap.map;
}

// TSClientRequesterConnStatsNaluTimes

STNBStructMapsRec TSClientRequesterConnStatsNaluTimes_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSClientRequesterConnStatsNaluTimes_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSClientRequesterConnStatsNaluTimes_sharedStructMap);
    if(TSClientRequesterConnStatsNaluTimes_sharedStructMap.map == NULL){
        STTSClientRequesterConnStatsNaluTimes s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSClientRequesterConnStatsNaluTimes);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, wait, TSClientRequesterConnStatsTime_getSharedStructMap());
        NBStructMap_addStructM(map, s, arrival, TSClientRequesterConnStatsTime_getSharedStructMap());
        TSClientRequesterConnStatsNaluTimes_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSClientRequesterConnStatsNaluTimes_sharedStructMap);
    return TSClientRequesterConnStatsNaluTimes_sharedStructMap.map;
}

// TSClientRequesterConnStatsNalu

STNBStructMapsRec TSClientRequesterConnStatsNalu_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSClientRequesterConnStatsNalu_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSClientRequesterConnStatsNalu_sharedStructMap);
    if(TSClientRequesterConnStatsNalu_sharedStructMap.map == NULL){
        STTSClientRequesterConnStatsNalu s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSClientRequesterConnStatsNalu);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addUIntM(map, s, count);
        NBStructMap_addUIntM(map, s, bytesCount);
        NBStructMap_addStructM(map, s, times, TSClientRequesterConnStatsNaluTimes_getSharedStructMap());
        TSClientRequesterConnStatsNalu_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSClientRequesterConnStatsNalu_sharedStructMap);
    return TSClientRequesterConnStatsNalu_sharedStructMap.map;
}

// TSClientRequesterConnStatsNalusBuffs

STNBStructMapsRec TSClientRequesterConnStatsNalusBuffs_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSClientRequesterConnStatsNalusBuffs_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSClientRequesterConnStatsNalusBuffs_sharedStructMap);
    if(TSClientRequesterConnStatsNalusBuffs_sharedStructMap.map == NULL){
        STTSClientRequesterConnStatsNalusBuffs s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSClientRequesterConnStatsNalusBuffs);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addUIntM(map, s, newAllocs);
        NBStructMap_addBoolM(map, s, populated);
        NBStructMap_addUIntM(map, s, min);
        NBStructMap_addUIntM(map, s, max);
        TSClientRequesterConnStatsNalusBuffs_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSClientRequesterConnStatsNalusBuffs_sharedStructMap);
    return TSClientRequesterConnStatsNalusBuffs_sharedStructMap.map;
}

// TSClientRequesterConnStatsNalus

STNBStructMapsRec TSClientRequesterConnStatsNalus_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSClientRequesterConnStatsNalus_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSClientRequesterConnStatsNalus_sharedStructMap);
    if(TSClientRequesterConnStatsNalus_sharedStructMap.map == NULL){
        STTSClientRequesterConnStatsNalus s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSClientRequesterConnStatsNalus);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, buffs, TSClientRequesterConnStatsNalusBuffs_getSharedStructMap());
        NBStructMap_addArrayOfStructM(map, s, byNalu, TSClientRequesterConnStatsNalu_getSharedStructMap());
        NBStructMap_addArrayOfStructM(map, s, ignored, TSClientRequesterConnStatsNalu_getSharedStructMap());
        NBStructMap_addArrayOfStructM(map, s, malform, TSClientRequesterConnStatsNalu_getSharedStructMap());
        NBStructMap_addStructM(map, s, malformUnkw, TSClientRequesterConnStatsNalu_getSharedStructMap());
        TSClientRequesterConnStatsNalus_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSClientRequesterConnStatsNalus_sharedStructMap);
    return TSClientRequesterConnStatsNalus_sharedStructMap.map;
}

// TSClientRequesterConnStatsState

STNBStructMapsRec TSClientRequesterConnStatsState_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSClientRequesterConnStatsState_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSClientRequesterConnStatsState_sharedStructMap);
    if(TSClientRequesterConnStatsState_sharedStructMap.map == NULL){
        STTSClientRequesterConnStatsState s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSClientRequesterConnStatsState);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, nalus, TSClientRequesterConnStatsNalus_getSharedStructMap());
        NBStructMap_addUIntM(map, s, updCalls);
        TSClientRequesterConnStatsState_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSClientRequesterConnStatsState_sharedStructMap);
    return TSClientRequesterConnStatsState_sharedStructMap.map;
}

// TSClientRequesterConnStatsData

STNBStructMapsRec TSClientRequesterConnStatsData_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSClientRequesterConnStatsData_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSClientRequesterConnStatsData_sharedStructMap);
    if(TSClientRequesterConnStatsData_sharedStructMap.map == NULL){
        STTSClientRequesterConnStatsData s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSClientRequesterConnStatsData);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, loaded, TSClientRequesterConnStatsState_getSharedStructMap());
        NBStructMap_addStructM(map, s, accum, TSClientRequesterConnStatsState_getSharedStructMap());
        NBStructMap_addStructM(map, s, total, TSClientRequesterConnStatsState_getSharedStructMap());
        TSClientRequesterConnStatsData_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSClientRequesterConnStatsData_sharedStructMap);
    return TSClientRequesterConnStatsData_sharedStructMap.map;
}

//TSClientRequesterConnStatsOpq

typedef struct STTSClientRequesterConnStatsOpq_ {
	STNBThreadMutex					    mutex;
	STTSClientRequesterConnStatsData	data;
} STTSClientRequesterConnStatsOpq;


void TSClientRequesterConnStats_init(STTSClientRequesterConnStats* obj){
	STTSClientRequesterConnStatsOpq* opq = obj->opaque = NBMemory_allocType(STTSClientRequesterConnStatsOpq);
	NBMemory_setZeroSt(*opq, STTSClientRequesterConnStatsOpq);
	NBThreadMutex_init(&opq->mutex);
	{
		//
	}
}

void TSClientRequesterConnStats_release(STTSClientRequesterConnStats* obj){
	STTSClientRequesterConnStatsOpq* opq = (STTSClientRequesterConnStatsOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	{
		//
	}
	NBThreadMutex_unlock(&opq->mutex);
	NBThreadMutex_release(&opq->mutex);
	NBMemory_free(opq);
	obj->opaque = NULL;
}

//Data

STTSClientRequesterConnStatsData TSClientRequesterConnStats_getData(STTSClientRequesterConnStats* obj, const BOOL resetAccum){
	STTSClientRequesterConnStatsData r;
	STTSClientRequesterConnStatsOpq* opq = (STTSClientRequesterConnStatsOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	{
		r = opq->data;
		if(resetAccum){
			NBMemory_setZero(opq->data.accum);
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

void TSClientRequesterConnStats_concatFormatedBytes_(const UI64 bytesCount, STNBString* dst){
	if(dst != NULL){
		if(bytesCount >= (1024 * 1024 * 1024)){
			const UI64 div = (1024 * 1024 * 1024);
			const double p0 = (double)(bytesCount / div); 
			const double p1 = (double)(bytesCount % div) / (double)div;
			NBString_concatDouble(dst, p0 + p1, 1);
			NBString_concat(dst, "GB");
		} else if(bytesCount >= (1024 * 1024)){
			const UI64 div = (1024 * 1024);
			const double p0 = (double)(bytesCount / div); 
			const double p1 = (double)(bytesCount % div) / (double)div;
			NBString_concatDouble(dst, p0 + p1, 1);
			NBString_concat(dst, "MB");
		} else if(bytesCount >= (1024)){
			const UI64 div = (1024);
			const double p0 = (double)(bytesCount / div); 
			const double p1 = (double)(bytesCount % div) / (double)div;
			NBString_concatDouble(dst, p0 + p1, 1);
			NBString_concat(dst, "KB");
		} else {
			NBString_concatUI64(dst, bytesCount);
			NBString_concat(dst, "B");
		}
	}
}

void TSClientRequesterConnStats_concat(STTSClientRequesterConnStats* obj, const STTSCfgTelemetryStreams* cfg, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst, const BOOL resetAccum){
	STTSClientRequesterConnStatsOpq* opq = (STTSClientRequesterConnStatsOpq*)obj->opaque;
	if(cfg != NULL && cfg->statsLevel > ENNBLogLevel_None){
		NBThreadMutex_lock(&opq->mutex);
		{
			TSClientRequesterConnStats_concatData(&opq->data, cfg, loaded, accum, total, prefixFirst, prefixOthers, ignoreEmpties, dst);
			if(resetAccum){
				NBMemory_setZero(opq->data.accum);
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
}

void TSClientRequesterConnStats_concatData(const STTSClientRequesterConnStatsData* obj, const STTSCfgTelemetryStreams* cfg, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL && cfg != NULL && cfg->statsLevel > ENNBLogLevel_None){
		BOOL opened = FALSE;
		const char* preExtra = "        |- ";
		STNBString str, pre;
		NBString_initWithSz(&str, 0, 256, 0.10f);
		NBString_initWithSz(&pre, NBString_strLenBytes(prefixOthers) + NBString_strLenBytes(preExtra) + 1, 256, 0.10f);
		NBString_concat(&pre, prefixOthers);
		NBString_concat(&pre, preExtra);
		if(loaded){
			NBString_empty(&str);
			TSClientRequesterConnStats_concatState(&obj->loaded, cfg, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "loaded:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		if(accum){
			NBString_empty(&str);
			TSClientRequesterConnStats_concatState(&obj->accum, cfg, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  " accum:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		if(total){
			NBString_empty(&str);
			TSClientRequesterConnStats_concatState(&obj->total, cfg, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  " total:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		NBString_release(&pre);
		NBString_release(&str);
	}
}

void TSClientRequesterConnStats_concatState(const STTSClientRequesterConnStatsState* obj, const STTSCfgTelemetryStreams* cfg, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL && cfg != NULL && cfg->statsLevel > ENNBLogLevel_None){
		BOOL opened = FALSE;
		const char* preExtra = "       |- ";
		STNBString str, pre;
		NBString_initWithSz(&str, 0, 256, 0.10f);
		NBString_initWithSz(&pre, NBString_strLenBytes(prefixOthers) + NBString_strLenBytes(preExtra) + 1, 256, 0.10f);
		NBString_concat(&pre, prefixOthers);
		NBString_concat(&pre, preExtra);
		//nalus
		{
			NBString_empty(&str);
			TSClientRequesterConnStats_concatNalus(&obj->nalus, cfg, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "  nalus:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		//UpdCalls
		/*{
			if(!ignoreEmpties || obj->updCalls > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "updCall: ");
				NBString_concatUI64(dst, obj->updCalls);
				opened = TRUE;
			}
		}*/
		NBString_release(&pre);
		NBString_release(&str);
	}
}

void TSClientRequesterConnStats_concatNalus(const STTSClientRequesterConnStatsNalus* obj, const STTSCfgTelemetryStreams* cfg, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL && cfg != NULL && cfg->statsLevel > ENNBLogLevel_None){
		BOOL opened = FALSE;
		//buffers
		{
			BOOL opened2 = FALSE;
			if(cfg->statsLevel >= ENNBLogLevel_Warning && obj->buffs.newAllocs > 0){
				//prefix
				if(opened){
					NBString_concat(dst, ", ");
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concatUI32(dst, obj->buffs.newAllocs);
				NBString_concat(dst,  " newAllocs");
				opened = opened2 = TRUE;
			}
			if((opened2 || cfg->statsLevel >= ENNBLogLevel_Info) && obj->buffs.populated){
				//prefix
				if(opened){
					NBString_concat(dst, ", ");
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concatUI32(dst, obj->buffs.min);
				NBString_concat(dst,  "-");
				NBString_concatUI32(dst, obj->buffs.max);
				NBString_concat(dst,  " units-per-buff");
				opened = opened2 = TRUE;
			}
		}
		//processed
		if(cfg->statsLevel >= ENNBLogLevel_Info){
			SI32 i; const SI32 count = (sizeof(obj->byNalu) / sizeof(obj->byNalu[0]));
			UI32 totalCount = 0; UI64 totalBytesCount = 0;
			STTSClientRequesterConnStatsNalu total;
			NBMemory_setZeroSt(total, STTSClientRequesterConnStatsNalu);
			//Get total
			for(i = 0; i < count; i++){
				const STTSClientRequesterConnStatsNalu* n = &obj->byNalu[i];
				totalCount += n->count;
				totalBytesCount += n->bytesCount;
				//wait time
				if(n->times.wait.count != 0){
					if(total.times.wait.count == 0){
						total.times.wait.msMin = n->times.wait.msMin;
						total.times.wait.msMax = n->times.wait.msMax;
					} else {
						if(total.times.wait.msMin > n->times.wait.msMin) total.times.wait.msMin = n->times.wait.msMin;
						if(total.times.wait.msMax < n->times.wait.msMax) total.times.wait.msMax = n->times.wait.msMax;
					}
					total.times.wait.count += n->times.wait.count; 
					total.times.wait.msTotal += n->times.wait.msTotal;
				}
				//arrival time
				if(n->times.arrival.count != 0){
					if(total.times.arrival.count == 0){
						total.times.arrival.msMin = n->times.arrival.msMin;
						total.times.arrival.msMax = n->times.arrival.msMax;
					} else {
						if(total.times.arrival.msMin > n->times.arrival.msMin) total.times.arrival.msMin = n->times.arrival.msMin;
						if(total.times.arrival.msMax < n->times.arrival.msMax) total.times.arrival.msMax = n->times.arrival.msMax;
					}
					total.times.arrival.count += n->times.arrival.count; 
					total.times.arrival.msTotal += n->times.arrival.msTotal;
				}
			}
			if(!ignoreEmpties || totalCount > 0){
				//total
				if(totalBytesCount > 0){
					const STTSClientRequesterConnStatsNalu* n = &total;
					if(opened){
						NBString_concat(dst, "\n");
						NBString_concat(dst, prefixOthers);
					} else {
						NBString_concat(dst, prefixFirst);
					}
					NBString_concat(dst,  "total: ");
					TSClientRequesterConnStats_concatFormatedBytes_(totalBytesCount, dst);
					if(totalCount > 0){
						NBString_concat(dst, "/");
						NBString_concatUI32(dst, totalCount);
					}
					//wait times
					if(n->times.wait.count > 0){
						NBString_concat(dst, ", ");
						if(n->times.wait.msMin == n->times.wait.msMax){
							NBString_concatUI32(dst, n->times.wait.msMin);
						} else {
							NBString_concatUI32(dst, n->times.wait.msMin);
							NBString_concat(dst, "-");
							NBString_concatUI32(dst, n->times.wait.msTotal / n->times.wait.count);
							NBString_concat(dst, "-");
							NBString_concatUI32(dst, n->times.wait.msMax);
						}
						NBString_concat(dst, "ms/");
						NBString_concatUI32(dst, n->times.wait.count);
						NBString_concat(dst, " wait");
					}
					//arrival times
					if(n->times.arrival.count > 0){
						NBString_concat(dst, ", ");
						if(n->times.arrival.msMin == n->times.arrival.msMax){
							NBString_concatUI32(dst, n->times.arrival.msMin);
						} else {
							NBString_concatUI32(dst, n->times.arrival.msMin);
							NBString_concat(dst, "-");
							NBString_concatUI32(dst, n->times.arrival.msTotal / n->times.arrival.count);
							NBString_concat(dst, "-");
							NBString_concatUI32(dst, n->times.arrival.msMax);
						}
						NBString_concat(dst, "ms/");
						NBString_concatUI32(dst, n->times.arrival.count);
						NBString_concat(dst, " rcvDly");
					}
					opened = TRUE;
				}
				//Details
				for(i = 0; i < count; i++){
					const STTSClientRequesterConnStatsNalu* n = &obj->byNalu[i];
					if(!ignoreEmpties || n->count > 0){
						if(opened){
							NBString_concat(dst, "\n");
							NBString_concat(dst, prefixOthers);
						} else {
							NBString_concat(dst, prefixFirst);
						}
						{
							NBString_concat(dst,  "nal");
							if(i < 10){
								NBString_concat(dst,  "_");
							}
							NBString_concatSI32(dst,  i);
							NBString_concat(dst,  ": ");
						}
						if(n->bytesCount > 0){
							TSClientRequesterConnStats_concatFormatedBytes_(n->bytesCount, dst);
							NBString_concat(dst, "/");
						}
						NBString_concatUI32(dst, n->count);
						//wait times
						if(n->times.wait.count > 0){
							NBString_concat(dst, ", ");
							if(n->times.wait.msMin == n->times.wait.msMax){
								NBString_concatUI32(dst, n->times.wait.msMin);
							} else {
								NBString_concatUI32(dst, n->times.wait.msMin);
								NBString_concat(dst, "-");
								NBString_concatUI32(dst, n->times.wait.msTotal / n->times.wait.count);
								NBString_concat(dst, "-");
								NBString_concatUI32(dst, n->times.wait.msMax);
							}
							NBString_concat(dst, "ms/");
							NBString_concatUI32(dst, n->times.wait.count);
							NBString_concat(dst, " wait");
						}
						//arrival times
						if(n->times.arrival.count > 0){
							NBString_concat(dst, ", ");
							if(n->times.arrival.msMin == n->times.arrival.msMax){
								NBString_concatUI32(dst, n->times.arrival.msMin);
							} else {
								NBString_concatUI32(dst, n->times.arrival.msMin);
								NBString_concat(dst, "-");
								NBString_concatUI32(dst, n->times.arrival.msTotal / n->times.arrival.count);
								NBString_concat(dst, "-");
								NBString_concatUI32(dst, n->times.arrival.msMax);
							}
							NBString_concat(dst, "ms/");
							NBString_concatUI32(dst, n->times.arrival.count);
							NBString_concat(dst, " rcvDly");
						}
						opened = TRUE;
					}
				}
			}
		}
		//ignored
		if(cfg->statsLevel >= ENNBLogLevel_Warning){
			SI32 i; const SI32 count = (sizeof(obj->ignored) / sizeof(obj->ignored[0]));
			UI32 totalCount = 0; UI64 totalBytesCount = 0;
			//Get total
			for(i = 0; i < count; i++){
				const STTSClientRequesterConnStatsNalu* n = &obj->ignored[i];
				totalCount += n->count;
				totalBytesCount += n->bytesCount;
			}
			if(!ignoreEmpties || totalCount > 0){
				BOOL opened2 = FALSE;
				//prefix
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "ignored:   ");
				//total
				if(totalBytesCount > 0){
					if(opened2) NBString_concat(dst, ", ");
					TSClientRequesterConnStats_concatFormatedBytes_(totalBytesCount, dst);
					if(totalCount > 0){
						NBString_concat(dst, "/");
						NBString_concatUI32(dst, totalCount);
					}
					opened2 = TRUE;
				}
				//Details
				for(i = 0; i < count; i++){
					const STTSClientRequesterConnStatsNalu* n = &obj->ignored[i];
					if(!ignoreEmpties || n->count > 0){
						if(opened2) NBString_concat(dst, ", ");
						NBString_concatSI32(dst, i);
						NBString_concat(dst, "[");
						if(n->bytesCount > 0){
							TSClientRequesterConnStats_concatFormatedBytes_(n->bytesCount, dst);
							NBString_concat(dst, "/");
						}
						NBString_concatUI32(dst, n->count);
						NBString_concat(dst, "]");
						opened2 = TRUE;
					}
				}
				opened = TRUE;
			}
		}
		//malformed
		if(cfg->statsLevel >= ENNBLogLevel_Error){
			SI32 i; const SI32 count = (sizeof(obj->malform) / sizeof(obj->malform[0]));
			UI32 totalCount = 0; UI64 totalBytesCount = 0;
			//Get total
			{
				for(i = 0; i < count; i++){
					const STTSClientRequesterConnStatsNalu* n = &obj->malform[i];
					totalCount += n->count;
					totalBytesCount += n->bytesCount;
				}
				totalCount += obj->malformUnkw.count;
				totalBytesCount += obj->malformUnkw.bytesCount;
			}
			if(!ignoreEmpties || totalCount > 0){
				BOOL opened2 = FALSE;
				//prefix
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "malform:   ");
				//total
				if(totalBytesCount > 0){
					if(opened2) NBString_concat(dst, ", ");
					TSClientRequesterConnStats_concatFormatedBytes_(totalBytesCount, dst);
					if(totalCount > 0){
						NBString_concat(dst, "/");
						NBString_concatUI32(dst, totalCount);
					}
					opened2 = TRUE;
				}
				//Details (unknown)
				{
					const STTSClientRequesterConnStatsNalu* n = &obj->malformUnkw;
					if(!ignoreEmpties || n->count > 0){
						if(opened2) NBString_concat(dst, ", ");
						NBString_concat(dst, "unkn[");
						if(n->bytesCount > 0){
							TSClientRequesterConnStats_concatFormatedBytes_(n->bytesCount, dst);
							NBString_concat(dst, "/");
						}	
						NBString_concatUI32(dst, n->count);
						NBString_concat(dst, "]");
						opened2 = TRUE;
					}
				}
				//Details
				for(i = 0; i < count; i++){
					const STTSClientRequesterConnStatsNalu* n = &obj->malform[i];
					if(!ignoreEmpties || n->count > 0){
						if(opened2) NBString_concat(dst, ", ");
						NBString_concatSI32(dst, i);
						NBString_concat(dst, "[");
						if(n->bytesCount > 0){
							TSClientRequesterConnStats_concatFormatedBytes_(n->bytesCount, dst);
							NBString_concat(dst, "/");
						}
						NBString_concatUI32(dst, n->count);
						NBString_concat(dst, "]");
						opened2 = TRUE;
					}
				}
				opened = TRUE;
			}
		}
	}
}

//Actions

//Upd

void TSClientRequesterConnStats_addUpd(STTSClientRequesterConnStats* obj, const STTSClientRequesterConnStatsUpd* upd){
	if(upd != NULL){
		STTSClientRequesterConnStatsOpq* opq = (STTSClientRequesterConnStatsOpq*)obj->opaque;
		const BOOL buffs		= (upd->buffs.newAllocs != 0 || upd->buffs.populated);
		const BOOL processed	= (upd->processed.total != 0);
		const BOOL ignored		= (upd->ignored.total != 0);
		const BOOL malformed	= (upd->malformed.total.count != 0);
		if(buffs || processed || ignored || malformed){
			NBThreadMutex_lock(&opq->mutex);
			{
				UI32 i;
				//allocated
				if(buffs){
					opq->data.accum.nalus.buffs.newAllocs += upd->buffs.newAllocs;
					if(upd->buffs.populated){
						if(!opq->data.accum.nalus.buffs.populated){
							opq->data.accum.nalus.buffs.min = upd->buffs.min;
							opq->data.accum.nalus.buffs.max = upd->buffs.max;
							opq->data.accum.nalus.buffs.populated = TRUE;
						} else {
							if(opq->data.accum.nalus.buffs.min > upd->buffs.min){
								opq->data.accum.nalus.buffs.min = upd->buffs.min;
							}
							if(opq->data.accum.nalus.buffs.max < upd->buffs.max){
								opq->data.accum.nalus.buffs.max = upd->buffs.max;
							}
						}
					}
				}
				//processed
				if(processed){
					//by nalu
					for(i = 0; i < (sizeof(upd->processed.byNalu) / sizeof(upd->processed.byNalu[0])); i++){
						const STTSClientRequesterConnStatsNalu* src = &upd->processed.byNalu[i];
						STTSClientRequesterConnStatsNalu* acc = &opq->data.accum.nalus.byNalu[i];
						STTSClientRequesterConnStatsNalu* tot = &opq->data.total.nalus.byNalu[i];
						//alive
						//accum
						acc->count		+= src->count;
						acc->bytesCount	+= src->bytesCount;
						if(src->times.wait.count != 0){
							if(acc->times.wait.count == 0){
								acc->times.wait.msMin = src->times.wait.msMin; 
								acc->times.wait.msMax = src->times.wait.msMax;
							} else {
								if(acc->times.wait.msMin > src->times.wait.msMin) acc->times.wait.msMin = src->times.wait.msMin; 
								if(acc->times.wait.msMax < src->times.wait.msMax) acc->times.wait.msMax = src->times.wait.msMax;
							}
							acc->times.wait.count += src->times.wait.count; 
							acc->times.wait.msTotal += src->times.wait.msTotal;
						}
						if(src->times.arrival.count != 0){
							if(acc->times.arrival.count == 0){
								acc->times.arrival.msMin = src->times.arrival.msMin; 
								acc->times.arrival.msMax = src->times.arrival.msMax;
							} else {
								if(acc->times.arrival.msMin > src->times.arrival.msMin) acc->times.arrival.msMin = src->times.arrival.msMin; 
								if(acc->times.arrival.msMax < src->times.arrival.msMax) acc->times.arrival.msMax = src->times.arrival.msMax;
							}
							acc->times.arrival.count += src->times.arrival.count; 
							acc->times.arrival.msTotal += src->times.arrival.msTotal;
						}
						//total
						tot->count		+= src->count;
						tot->bytesCount	+= src->bytesCount;
						if(src->times.wait.count != 0){
							if(tot->times.wait.count == 0){
								tot->times.wait.msMin = src->times.wait.msMin; 
								tot->times.wait.msMax = src->times.wait.msMax;
							} else {
								if(tot->times.wait.msMin > src->times.wait.msMin) tot->times.wait.msMin = src->times.wait.msMin; 
								if(tot->times.wait.msMax < src->times.wait.msMax) tot->times.wait.msMax = src->times.wait.msMax;
							}
							tot->times.wait.count += src->times.wait.count; 
							tot->times.wait.msTotal += src->times.wait.msTotal;
						}
						if(src->times.arrival.count != 0){
							if(tot->times.arrival.count == 0){
								tot->times.arrival.msMin = src->times.arrival.msMin; 
								tot->times.arrival.msMax = src->times.arrival.msMax;
							} else {
								if(tot->times.arrival.msMin > src->times.arrival.msMin) tot->times.arrival.msMin = src->times.arrival.msMin; 
								if(tot->times.arrival.msMax < src->times.arrival.msMax) tot->times.arrival.msMax = src->times.arrival.msMax;
							}
							tot->times.arrival.count += src->times.arrival.count; 
							tot->times.arrival.msTotal += src->times.arrival.msTotal;
						}
					}
				}
				//ignored
				if(ignored){
					for(i = 0; i < (sizeof(upd->ignored.byNalu) / sizeof(upd->ignored.byNalu[0])); i++){
						const STTSClientRequesterConnStatsNalu* src = &upd->ignored.byNalu[i];
						STTSClientRequesterConnStatsNalu* acc = &opq->data.accum.nalus.ignored[i];
						STTSClientRequesterConnStatsNalu* tot = &opq->data.total.nalus.ignored[i];
						//alive
						//accum
						acc->count		+= src->count;
						acc->bytesCount	+= src->bytesCount;
						//total
						tot->count		+= src->count;
						tot->bytesCount	+= src->bytesCount;
					}
				}
				//malformed
				if(malformed){
					for(i = 0; i < (sizeof(upd->malformed.byNalu) / sizeof(upd->malformed.byNalu[0])); i++){
						const STTSClientRequesterConnStatsNalu* src = &upd->malformed.byNalu[i];
						STTSClientRequesterConnStatsNalu* acc = &opq->data.accum.nalus.malform[i];
						STTSClientRequesterConnStatsNalu* tot = &opq->data.total.nalus.malform[i];
						//alive
						//accum
						acc->count		+= src->count;
						acc->bytesCount	+= src->bytesCount;
						//total
						tot->count		+= src->count;
						tot->bytesCount	+= src->bytesCount;
					}
					//alive
					//accum
					opq->data.accum.nalus.malformUnkw.count += upd->malformed.unknown.count;
					opq->data.accum.nalus.malformUnkw.bytesCount += upd->malformed.unknown.bytesCount;
					//total
					opq->data.total.nalus.malformUnkw.count += upd->malformed.unknown.count;
					opq->data.total.nalus.malformUnkw.bytesCount += upd->malformed.unknown.bytesCount;
				}
			}
			//updCalls
			{
				//accum
				opq->data.accum.updCalls++;
				//total
				opq->data.total.updCalls++;
			}
			NBThreadMutex_unlock(&opq->mutex);
		}
	}
}

//Update

void TSClientRequesterConnStatsUpd_init(STTSClientRequesterConnStatsUpd* obj){
	NBMemory_setZeroSt(*obj, STTSClientRequesterConnStatsUpd);
}

void TSClientRequesterConnStatsUpd_release(STTSClientRequesterConnStatsUpd* obj){
	//Nothing
}

void TSClientRequesterConnStatsUpd_naluProcessed(STTSClientRequesterConnStatsUpd* obj, const UI8 iNalType, const UI64 bytesCount, const UI32 msWait, const UI32 msArrival){
	if(iNalType >= 0 && iNalType < (sizeof(obj->processed.byNalu) / sizeof(obj->processed.byNalu[0]))){
		//byNalu
		{
			STTSClientRequesterConnStatsNalu* n = &obj->processed.byNalu[iNalType]; 
			n->count++;
			n->bytesCount += bytesCount;
			if(msWait != 0){
				if(n->times.wait.count == 0){
					n->times.wait.msMin = n->times.wait.msMax = msWait;
				} else {
					if(n->times.wait.msMin > msWait) n->times.wait.msMin = msWait;
					if(n->times.wait.msMax < msWait) n->times.wait.msMax = msWait;
				}
				n->times.wait.msTotal += msWait;
				n->times.wait.count++;
			}
			if(msArrival != 0){
				if(n->times.arrival.count == 0){
					n->times.arrival.msMin = n->times.arrival.msMax = msArrival;
				} else {
					if(n->times.arrival.msMin > msArrival) n->times.arrival.msMin = msArrival;
					if(n->times.arrival.msMax < msArrival) n->times.arrival.msMax = msArrival;
				}
				n->times.arrival.msTotal += msArrival;
				n->times.arrival.count++;
			}
		}
		//total
		{ 
			obj->processed.total++;
		}
	}
}

void TSClientRequesterConnStatsUpd_naluIgnored(STTSClientRequesterConnStatsUpd* obj, const UI8 iNalType, const UI64 bytesCount){
	if(iNalType >= 0 && iNalType < (sizeof(obj->ignored.byNalu) / sizeof(obj->ignored.byNalu[0]))){
		//by nalu
		{
			STTSClientRequesterConnStatsNalu* n = &obj->ignored.byNalu[iNalType]; 
			n->count++;
			n->bytesCount += bytesCount;
		}
		//total
		{
			obj->ignored.total++;
		}
	}
}

void TSClientRequesterConnStatsUpd_naluMalformed(STTSClientRequesterConnStatsUpd* obj, const UI8 iNalType, const UI64 bytesCount){
	if(iNalType >= 0 && iNalType < (sizeof(obj->malformed.byNalu) / sizeof(obj->malformed.byNalu[0]))){
		//by nalu
		{
			STTSClientRequesterConnStatsNalu* n = &obj->malformed.byNalu[iNalType];
			n->count++;
			n->bytesCount += bytesCount;
		}
		//total
		{
			obj->malformed.total.count++;
			obj->malformed.total.bytesCount += bytesCount;
		}
	}
}

void TSClientRequesterConnStatsUpd_unknownMalformed(STTSClientRequesterConnStatsUpd* obj, const UI64 bytesCount){
	//unknown
	{
		obj->malformed.unknown.count++;
		obj->malformed.unknown.bytesCount += bytesCount;
	}
	//total
	{
		obj->malformed.total.count++;
		obj->malformed.total.bytesCount += bytesCount;
	}
}
