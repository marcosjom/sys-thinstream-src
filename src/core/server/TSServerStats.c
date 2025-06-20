//
//  TSServerStats.c
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSServerStats.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBMngrStructMaps.h"

//TSServerStats

STNBStructMapsRec TSServerStats_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSServerStats_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSServerStats_sharedStructMap);
	if(TSServerStats_sharedStructMap.map == NULL){
		STTSServerStats s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSServerStats);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addUIntM(map, s, timeStart);
		NBStructMap_addUIntM(map, s, timeEnd);
		NBStructMap_addIntM(map, s, starts);
		NBStructMap_addIntM(map, s, shutdowns);
		NBStructMap_addStructM(map, s, stats, NBHttpStatsData_getSharedStructMap());
		TSServerStats_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSServerStats_sharedStructMap);
	return TSServerStats_sharedStructMap.map;
}

//Tools

BOOL TSServerStats_loadCurrentAsServiceStart(STTSContext* context, STTSServerStats* dst){
	BOOL r = FALSE;
	//Load
	{
		STNBStorageRecordWrite ref = TSContext_getStorageRecordForWrite(context, "stats/", "_current", FALSE, NULL, TSServerStats_getSharedStructMap());
		if(ref.data != NULL){
			STTSServerStats* priv = (STTSServerStats*)ref.data->priv.stData;
			if(priv != NULL){
				priv->starts++;
				if(dst != NULL){
					NBStruct_stRelease(TSServerStats_getSharedStructMap(), dst, sizeof(*dst));
					NBStruct_stClone(TSServerStats_getSharedStructMap(), priv, sizeof(*priv), dst, sizeof(*dst));
				}
				ref.privModified = TRUE;
				r = TRUE;
			}
		}
		TSContext_returnStorageRecordFromWrite(context, &ref);
	}
	if(dst != NULL){
		if(dst->timeStart <= 0){
			dst->timeStart = dst->timeEnd = NBDatetime_getCurUTCTimestamp();
		}
	}
	return r;
}

BOOL TSServerStats_saveCurrentAsServiceShutdown(STTSContext* context, STTSServerStats* srcAndDst){
	BOOL r = FALSE;
	srcAndDst->timeEnd = NBDatetime_getCurUTCTimestamp();
	srcAndDst->shutdowns++;
	//Load
	{
		STNBStorageRecordWrite ref = TSContext_getStorageRecordForWrite(context, "stats/", "_current", FALSE, NULL, TSServerStats_getSharedStructMap());
		if(ref.data != NULL){
			STTSServerStats* priv = (STTSServerStats*)ref.data->priv.stData;
			if(priv != NULL){
				NBStruct_stRelease(TSServerStats_getSharedStructMap(), priv, sizeof(*priv));
				NBStruct_stClone(TSServerStats_getSharedStructMap(), srcAndDst, sizeof(*srcAndDst), priv, sizeof(*priv));
				ref.privModified = TRUE;
				r = TRUE;
			}
		}
		TSContext_returnStorageRecordFromWrite(context, &ref);
	}
	return r;
}

BOOL TSServerStats_saveCurrentStats(STTSContext* context, STTSServerStats* srcAndDst){
	BOOL r = FALSE;
	srcAndDst->timeEnd = NBDatetime_getCurUTCTimestamp();
	//Save current
	{
		STNBStorageRecordWrite ref = TSContext_getStorageRecordForWrite(context, "stats/", "_current", TRUE, NULL, TSServerStats_getSharedStructMap());
		if(ref.data != NULL){
			STTSServerStats* priv = (STTSServerStats*)ref.data->priv.stData;
			if(priv != NULL){
				NBStruct_stRelease(TSServerStats_getSharedStructMap(), priv, sizeof(*priv));
				NBStruct_stClone(TSServerStats_getSharedStructMap(), srcAndDst, sizeof(*srcAndDst), priv, sizeof(*priv));
				ref.privModified = TRUE;
				r = TRUE;
			}
		}
		TSContext_returnStorageRecordFromWrite(context, &ref);
	}
	return r;
}

BOOL TSServerStats_saveAndOpenNewStats(STTSContext* context, STTSServerStats* srcAndDst){
	BOOL r = FALSE;
	srcAndDst->timeEnd = NBDatetime_getCurUTCTimestamp();
	//Save range
	{
		const STNBDatetime time0 = NBDatetime_getUTCFromTimestamp(srcAndDst->timeStart);
		const STNBDatetime time1 = NBDatetime_getUTCFromTimestamp(srcAndDst->timeEnd);
		STNBString filename;
		NBString_init(&filename);
		NBString_concatDateTimeCompact(&filename, time0);
		NBString_concatByte(&filename, '_');
		NBString_concatDateTimeCompact(&filename, time1);
		{
			STNBStorageRecordWrite ref = TSContext_getStorageRecordForWrite(context, "stats/", filename.str, TRUE, NULL, TSServerStats_getSharedStructMap());
			if(ref.data != NULL){
				STTSServerStats* priv = (STTSServerStats*)ref.data->priv.stData;
				if(priv != NULL){
					NBStruct_stRelease(TSServerStats_getSharedStructMap(), priv, sizeof(*priv));
					NBStruct_stClone(TSServerStats_getSharedStructMap(), srcAndDst, sizeof(*srcAndDst), priv, sizeof(*priv));
					ref.privModified = TRUE;
				}
			}
			TSContext_returnStorageRecordFromWrite(context, &ref);
		}
		NBString_release(&filename);
	}
	//Reset current
	{
		NBStruct_stRelease(TSServerStats_getSharedStructMap(), srcAndDst, sizeof(*srcAndDst));
		srcAndDst->timeStart = srcAndDst->timeEnd = NBDatetime_getCurUTCTimestamp();
		//Save
		{
			STNBStorageRecordWrite ref = TSContext_getStorageRecordForWrite(context, "stats/", "_current", TRUE, NULL, TSServerStats_getSharedStructMap());
			if(ref.data != NULL){
				STTSServerStats* priv = (STTSServerStats*)ref.data->priv.stData;
				if(priv != NULL){
					NBStruct_stRelease(TSServerStats_getSharedStructMap(), priv, sizeof(*priv));
					NBStruct_stClone(TSServerStats_getSharedStructMap(), srcAndDst, sizeof(*srcAndDst), priv, sizeof(*priv));
					ref.privModified = TRUE;
					r = TRUE;
				}
			}
			TSContext_returnStorageRecordFromWrite(context, &ref);
		}
	}
	return r;
}

// TSServerStatsThreadState

#ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
STNBStructMapsRec TSServerStatsThreadState_sharedStructMap = STNBStructMapsRec_empty;
#endif

#ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
const STNBStructMap* TSServerStatsThreadState_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSServerStatsThreadState_sharedStructMap);
    if(TSServerStatsThreadState_sharedStructMap.map == NULL){
        STTSServerStatsThreadState s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSServerStatsThreadState);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStrPtrM(map, s, name);
        NBStructMap_addStrPtrM(map, s, firstKnownMethod);
        NBStructMap_addEnumM(map, s, statsLevel, NBLogLevel_getSharedEnumMap());
        NBStructMap_addStructM(map, s, stats, NBMngrProcessStatsState_getSharedStructMap());
        TSServerStatsThreadState_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSServerStatsThreadState_sharedStructMap);
    return TSServerStatsThreadState_sharedStructMap.map;
}
#endif

#ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
void TSServerStatsThreadState_init(STTSServerStatsThreadState* obj){
    NBMemory_setZeroSt(*obj, STTSServerStatsThreadState);
}
#endif

#ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
void TSServerStatsThreadState_release(STTSServerStatsThreadState* obj){
    NBMngrProcessStatsState_release(&obj->stats);
}
#endif

// TSServerStatsProcessState

#ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
STNBStructMapsRec TSServerStatsProcessState_sharedStructMap = STNBStructMapsRec_empty;
#endif

#ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
const STNBStructMap* TSServerStatsProcessState_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSServerStatsProcessState_sharedStructMap);
    if(TSServerStatsProcessState_sharedStructMap.map == NULL){
        STTSServerStatsProcessState s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSServerStatsProcessState);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, stats, NBMngrProcessStatsState_getSharedStructMap());
        NBStructMap_addPtrToArrayOfStructM(map, s, threads, threadsSz, ENNBStructMapSign_Unsigned, TSServerStatsThreadState_getSharedStructMap());
        TSServerStatsProcessState_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSServerStatsProcessState_sharedStructMap);
    return TSServerStatsProcessState_sharedStructMap.map;
}
#endif

#ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
void TSServerStatsProcessState_init(STTSServerStatsProcessState* obj){
    NBMemory_setZeroSt(*obj, STTSServerStatsProcessState);
}
#endif

#ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
void TSServerStatsProcessState_release(STTSServerStatsProcessState* obj){
    NBMngrProcessStatsState_release(&obj->stats);
    //threads
    if(obj->threads != NULL){
        UI32 i; for(i = 0; i < obj->threadsSz; i++){
            STTSServerStatsThreadState* t = &obj->threads[i];
            TSServerStatsThreadState_release(t);
        }
        NBMemory_free(obj->threads);
        obj->threads = NULL;
        obj->threadsSz = 0;
    }
}
#endif

// TSServerStatsState

STNBStructMapsRec TSServerStatsState_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSServerStatsState_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSServerStatsState_sharedStructMap);
    if(TSServerStatsState_sharedStructMap.map == NULL){
        STTSServerStatsState s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSServerStatsState);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, buffs, NBDataPtrsStatsState_getSharedStructMap());
        NBStructMap_addStructM(map, s, rtsp, NBRtspClientStatsState_getSharedStructMap());
        NBStructMap_addStructM(map, s, reqs, TSClientRequesterConnStatsState_getSharedStructMap());
        NBStructMap_addStructM(map, s, storage, TSStreamsStorageStatsState_getSharedStructMap());
#       ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
        NBStructMap_addStructM(map, s, process, TSServerStatsProcessState_getSharedStructMap());
#       endif
        TSServerStatsState_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSServerStatsState_sharedStructMap);
    return TSServerStatsState_sharedStructMap.map;
}

// TSServerStatsData

STNBStructMapsRec TSServerStatsData_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSServerStatsData_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSServerStatsData_sharedStructMap);
    if(TSServerStatsData_sharedStructMap.map == NULL){
        STTSServerStatsData s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSServerStatsData);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, loaded, TSServerStatsState_getSharedStructMap());
        NBStructMap_addStructM(map, s, accum, TSServerStatsState_getSharedStructMap());
        NBStructMap_addStructM(map, s, total, TSServerStatsState_getSharedStructMap());
        TSServerStatsData_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSServerStatsData_sharedStructMap);
    return TSServerStatsData_sharedStructMap.map;
}

void TSServerStatsData_init(STTSServerStatsData* obj){
    NBMemory_setZeroSt(*obj, STTSServerStatsData);
}

void TSServerStatsData_release(STTSServerStatsData* obj){
    //process
#    ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
    {
        //loaded
        TSServerStatsProcessState_release(&obj->loaded.process);
        //accum
        TSServerStatsProcessState_release(&obj->accum.process);
        //total
        TSServerStatsProcessState_release(&obj->total.process);
    }
#    endif
}
