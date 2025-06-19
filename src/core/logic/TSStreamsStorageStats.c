//
//  TSStreamsStorageStats.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSStreamsStorageStats.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBThreadMutex.h"
//
#include "nb/core/NBStructMaps.h"
#include "nb/core/NBMngrStructMaps.h"

// TSStreamsStorageStatsStreams

STNBStructMapsRec TSStreamsStorageStatsStreams_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsStorageStatsStreams_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamsStorageStatsStreams_sharedStructMap);
    if(TSStreamsStorageStatsStreams_sharedStructMap.map == NULL){
        STTSStreamsStorageStatsStreams s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsStorageStatsStreams);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addUIntM(map, s, loaded);
        NBStructMap_addUIntM(map, s, added);
        NBStructMap_addUIntM(map, s, removed);
        TSStreamsStorageStatsStreams_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamsStorageStatsStreams_sharedStructMap);
    return TSStreamsStorageStatsStreams_sharedStructMap.map;
}

// TSStreamsStorageStatsStreamsVersions

STNBStructMapsRec TSStreamsStorageStatsStreamsVersions_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsStorageStatsStreamsVersions_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamsStorageStatsStreamsVersions_sharedStructMap);
    if(TSStreamsStorageStatsStreamsVersions_sharedStructMap.map == NULL){
        STTSStreamsStorageStatsStreamsVersions s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsStorageStatsStreamsVersions);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addUIntM(map, s, loaded);
        NBStructMap_addUIntM(map, s, added);
        NBStructMap_addUIntM(map, s, removed);
        TSStreamsStorageStatsStreamsVersions_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamsStorageStatsStreamsVersions_sharedStructMap);
    return TSStreamsStorageStatsStreamsVersions_sharedStructMap.map;
}

// TSStreamsStorageStatsStreamsVersionsGroups

STNBStructMapsRec TSStreamsStorageStatsStreamsVersionsGroups_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsStorageStatsStreamsVersionsGroups_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamsStorageStatsStreamsVersionsGroups_sharedStructMap);
    if(TSStreamsStorageStatsStreamsVersionsGroups_sharedStructMap.map == NULL){
        STTSStreamsStorageStatsStreamsVersionsGroups s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsStorageStatsStreamsVersionsGroups);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addUIntM(map, s, loaded);
        NBStructMap_addUIntM(map, s, added);
        NBStructMap_addUIntM(map, s, removed);
        TSStreamsStorageStatsStreamsVersionsGroups_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamsStorageStatsStreamsVersionsGroups_sharedStructMap);
    return TSStreamsStorageStatsStreamsVersionsGroups_sharedStructMap.map;
}

// TSStreamsStorageStatsFolders

STNBStructMapsRec TSStreamsStorageStatsFolders_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsStorageStatsFolders_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamsStorageStatsFolders_sharedStructMap);
    if(TSStreamsStorageStatsFolders_sharedStructMap.map == NULL){
        STTSStreamsStorageStatsFolders s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsStorageStatsFolders);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, streams, TSStreamsStorageStatsStreams_getSharedStructMap());
        NBStructMap_addStructM(map, s, streamsVersions, TSStreamsStorageStatsStreamsVersions_getSharedStructMap());
        NBStructMap_addStructM(map, s, streamsVersionsGroups, TSStreamsStorageStatsStreamsVersionsGroups_getSharedStructMap());
        TSStreamsStorageStatsFolders_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamsStorageStatsFolders_sharedStructMap);
    return TSStreamsStorageStatsFolders_sharedStructMap.map;
}

// TSStreamsStorageStatsFileCounts

STNBStructMapsRec TSStreamsStorageStatsFileCounts_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsStorageStatsFileCounts_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamsStorageStatsFileCounts_sharedStructMap);
    if(TSStreamsStorageStatsFileCounts_sharedStructMap.map == NULL){
        STTSStreamsStorageStatsFileCounts s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsStorageStatsFileCounts);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addUIntM(map, s, count);
        NBStructMap_addUIntM(map, s, bytesCount);
        TSStreamsStorageStatsFileCounts_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamsStorageStatsFileCounts_sharedStructMap);
    return TSStreamsStorageStatsFileCounts_sharedStructMap.map;
}

// TSStreamsStorageStatsFileIdxCounts

STNBStructMapsRec TSStreamsStorageStatsFileIdxCounts_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsStorageStatsFileIdxCounts_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamsStorageStatsFileIdxCounts_sharedStructMap);
    if(TSStreamsStorageStatsFileIdxCounts_sharedStructMap.map == NULL){
        STTSStreamsStorageStatsFileIdxCounts s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsStorageStatsFileIdxCounts);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addUIntM(map, s, count);
        NBStructMap_addUIntM(map, s, recordsCount);
        NBStructMap_addUIntM(map, s, bytesCount);
        TSStreamsStorageStatsFileIdxCounts_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamsStorageStatsFileIdxCounts_sharedStructMap);
    return TSStreamsStorageStatsFileIdxCounts_sharedStructMap.map;
}

// TSStreamsStorageStatsFile

STNBStructMapsRec TSStreamsStorageStatsFile_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsStorageStatsFile_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamsStorageStatsFile_sharedStructMap);
    if(TSStreamsStorageStatsFile_sharedStructMap.map == NULL){
        STTSStreamsStorageStatsFile s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsStorageStatsFile);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, loaded, TSStreamsStorageStatsFileCounts_getSharedStructMap());
        NBStructMap_addStructM(map, s, added, TSStreamsStorageStatsFileCounts_getSharedStructMap());
        NBStructMap_addStructM(map, s, removed, TSStreamsStorageStatsFileCounts_getSharedStructMap());
        TSStreamsStorageStatsFile_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamsStorageStatsFile_sharedStructMap);
    return TSStreamsStorageStatsFile_sharedStructMap.map;
}

// TSStreamsStorageStatsFileIdx

STNBStructMapsRec TSStreamsStorageStatsFileIdx_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsStorageStatsFileIdx_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamsStorageStatsFileIdx_sharedStructMap);
    if(TSStreamsStorageStatsFileIdx_sharedStructMap.map == NULL){
        STTSStreamsStorageStatsFileIdx s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsStorageStatsFileIdx);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, loaded, TSStreamsStorageStatsFileIdxCounts_getSharedStructMap());
        NBStructMap_addStructM(map, s, added, TSStreamsStorageStatsFileIdxCounts_getSharedStructMap());
        NBStructMap_addStructM(map, s, removed, TSStreamsStorageStatsFileIdxCounts_getSharedStructMap());
        TSStreamsStorageStatsFileIdx_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamsStorageStatsFileIdx_sharedStructMap);
    return TSStreamsStorageStatsFileIdx_sharedStructMap.map;
}

// TSStreamsStorageStatsFiles

STNBStructMapsRec TSStreamsStorageStatsFiles_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsStorageStatsFiles_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamsStorageStatsFiles_sharedStructMap);
    if(TSStreamsStorageStatsFiles_sharedStructMap.map == NULL){
        STTSStreamsStorageStatsFiles s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsStorageStatsFiles);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, payloads, TSStreamsStorageStatsFile_getSharedStructMap());
        NBStructMap_addStructM(map, s, indexes, TSStreamsStorageStatsFileIdx_getSharedStructMap());
        NBStructMap_addStructM(map, s, gaps, TSStreamsStorageStatsFileIdx_getSharedStructMap());
        NBStructMap_addStructM(map, s, trash, TSStreamsStorageStatsFile_getSharedStructMap());
        NBStructMap_addStructM(map, s, others, TSStreamsStorageStatsFile_getSharedStructMap());
        TSStreamsStorageStatsFiles_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamsStorageStatsFiles_sharedStructMap);
    return TSStreamsStorageStatsFiles_sharedStructMap.map;
}

// TSStreamsStorageStatsState

STNBStructMapsRec TSStreamsStorageStatsState_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsStorageStatsState_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamsStorageStatsState_sharedStructMap);
    if(TSStreamsStorageStatsState_sharedStructMap.map == NULL){
        STTSStreamsStorageStatsState s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsStorageStatsState);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, folders, TSStreamsStorageStatsFolders_getSharedStructMap());
        NBStructMap_addStructM(map, s, files, TSStreamsStorageStatsFiles_getSharedStructMap());
        NBStructMap_addUIntM(map, s, updCalls);
        TSStreamsStorageStatsState_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamsStorageStatsState_sharedStructMap);
    return TSStreamsStorageStatsState_sharedStructMap.map;
}

// TSStreamsStorageStatsData

STNBStructMapsRec TSStreamsStorageStatsData_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsStorageStatsData_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamsStorageStatsData_sharedStructMap);
    if(TSStreamsStorageStatsData_sharedStructMap.map == NULL){
        STTSStreamsStorageStatsData s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsStorageStatsData);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStructM(map, s, loaded, TSStreamsStorageStatsState_getSharedStructMap());
        NBStructMap_addStructM(map, s, accum, TSStreamsStorageStatsState_getSharedStructMap());
        NBStructMap_addStructM(map, s, total, TSStreamsStorageStatsState_getSharedStructMap());
        TSStreamsStorageStatsData_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamsStorageStatsData_sharedStructMap);
    return TSStreamsStorageStatsData_sharedStructMap.map;
}

//TSStreamsStorageStatsOpq

typedef struct STTSStreamsStorageStatsOpq_ {
	STNBThreadMutex					mutex;
	STTSStreamsStorageStatsData		data;
} STTSStreamsStorageStatsOpq;


void TSStreamsStorageStats_init(STTSStreamsStorageStats* obj){
	STTSStreamsStorageStatsOpq* opq = obj->opaque = NBMemory_allocType(STTSStreamsStorageStatsOpq);
	NBMemory_setZeroSt(*opq, STTSStreamsStorageStatsOpq);
	NBThreadMutex_init(&opq->mutex);
	{
		//
	}
}

void TSStreamsStorageStats_release(STTSStreamsStorageStats* obj){
	STTSStreamsStorageStatsOpq* opq = (STTSStreamsStorageStatsOpq*)obj->opaque; 
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

STTSStreamsStorageStatsData TSStreamsStorageStats_getData(STTSStreamsStorageStats* obj, const BOOL resetAccum){
	STTSStreamsStorageStatsData r;
	STTSStreamsStorageStatsOpq* opq = (STTSStreamsStorageStatsOpq*)obj->opaque; 
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

UI64 TSStreamsStorageStats_totalLoadedBytes(STTSStreamsStorageStats* obj){
	UI64 r = 0;
	STTSStreamsStorageStatsOpq* opq = (STTSStreamsStorageStatsOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	{
		r = TSStreamsStorageStats_totalLoadedBytesData(&opq->data);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

UI64 TSStreamsStorageStats_totalLoadedBytesData(const STTSStreamsStorageStatsData* obj){
	return obj->loaded.files.payloads.loaded.bytesCount + obj->loaded.files.indexes.loaded.bytesCount + obj->loaded.files.gaps.loaded.bytesCount; 
}

UI64 TSStreamsStorageStats_totalBytesState(const STTSStreamsStorageStatsState* obj){
	return obj->files.payloads.loaded.bytesCount + obj->files.indexes.loaded.bytesCount + obj->files.gaps.loaded.bytesCount + obj->files.trash.loaded.bytesCount + obj->files.others.loaded.bytesCount;
}

void TSStreamsStorageStats_concatFormatedBytes_(const UI64 bytesCount, STNBString* dst){
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

void TSStreamsStorageStats_concat(STTSStreamsStorageStats* obj, const STTSCfgTelemetryFilesystem* cfg, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst, const BOOL resetAccum){
	STTSStreamsStorageStatsOpq* opq = (STTSStreamsStorageStatsOpq*)obj->opaque; 
	if(cfg != NULL && cfg->statsLevel > ENNBLogLevel_None){
		NBThreadMutex_lock(&opq->mutex);
		{
			TSStreamsStorageStats_concatData(&opq->data, cfg, loaded, accum, total, prefixFirst, prefixOthers, ignoreEmpties, dst);
			if(resetAccum){
				NBMemory_setZero(opq->data.accum);
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
}

void TSStreamsStorageStats_concatData(const STTSStreamsStorageStatsData* obj, const STTSCfgTelemetryFilesystem* cfg, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst){
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
			TSStreamsStorageStats_concatState(&obj->loaded, cfg, "", pre.str, ignoreEmpties, &str);
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
			TSStreamsStorageStats_concatState(&obj->accum, cfg, "", pre.str, ignoreEmpties, &str);
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
			TSStreamsStorageStats_concatState(&obj->total, cfg, "", pre.str, ignoreEmpties, &str);
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

void TSStreamsStorageStats_concatState(const STTSStreamsStorageStatsState* obj, const STTSCfgTelemetryFilesystem* cfg, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL && cfg != NULL && cfg->statsLevel > ENNBLogLevel_None){
		BOOL opened = FALSE;
		const char* preExtra = "       |- ";
		STNBString str, pre;
		NBString_initWithSz(&str, 0, 256, 0.10f);
		NBString_initWithSz(&pre, NBString_strLenBytes(prefixOthers) + NBString_strLenBytes(preExtra) + 1, 256, 0.10f);
		NBString_concat(&pre, prefixOthers);
		NBString_concat(&pre, preExtra);
		{
			NBString_empty(&str);
			TSStreamsStorageStats_concatFiles(&obj->files, cfg, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "  files:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		{
			NBString_empty(&str);
			TSStreamsStorageStats_concatFolders(&obj->folders, cfg, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "folders:  ");
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

void TSStreamsStorageStats_concatFiles(const STTSStreamsStorageStatsFiles* obj, const STTSCfgTelemetryFilesystem* cfg, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL && cfg != NULL && cfg->statsLevel > ENNBLogLevel_None){
		BOOL opened = FALSE;
		STNBString str;
		NBString_initWithSz(&str, 0, 256, 0.10f);
		{
			NBString_empty(&str);
			TSStreamsStorageStats_concatFile(&obj->payloads, cfg, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "payload: ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		{
			NBString_empty(&str);
			TSStreamsStorageStats_concatFileIdx(&obj->indexes, cfg, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "indexes: ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		{
			NBString_empty(&str);
			TSStreamsStorageStats_concatFileIdx(&obj->gaps, cfg, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "   gaps: ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		{
			NBString_empty(&str);
			TSStreamsStorageStats_concatFile(&obj->trash, cfg, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "  trash: ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
        {
            NBString_empty(&str);
            TSStreamsStorageStats_concatFile(&obj->others, cfg, ignoreEmpties, &str);
            if(str.length > 0){
                if(opened){
                    NBString_concat(dst, "\n");
                    NBString_concat(dst, prefixOthers);
                } else {
                    NBString_concat(dst, prefixFirst);
                }
                NBString_concat(dst,  "  others: ");
                NBString_concat(dst, str.str);
                opened = TRUE;
            }
        }
		NBString_release(&str);
	}
}

void TSStreamsStorageStats_concatFile(const STTSStreamsStorageStatsFile* obj, const STTSCfgTelemetryFilesystem* cfg, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL && cfg != NULL && cfg->statsLevel > ENNBLogLevel_None){
		BOOL opened = FALSE;
		//Loaded
		{
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->loaded.bytesCount > 0)){
				if(opened) NBString_concat(dst, ", ");
				TSStreamsStorageStats_concatFormatedBytes_(obj->loaded.bytesCount, dst);
				opened = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->loaded.count > 0)){
				if(opened) NBString_concat(dst, "/");
				NBString_concatUI32(dst, obj->loaded.count);
				opened = TRUE;
			}
		}
		//Added
		{
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->added.bytesCount > 0)){
				NBString_concat(dst, opened ? ", (+)" : "(+)");
				TSStreamsStorageStats_concatFormatedBytes_(obj->added.bytesCount, dst);
				opened = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->added.count > 0)){
				if(opened) NBString_concat(dst, "/");
				NBString_concatUI32(dst, obj->added.count);
				opened = TRUE;
			}
		}
		//Removed
		{
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->removed.bytesCount > 0)){
				NBString_concat(dst, opened ? ", (-)" : "(-)");
				TSStreamsStorageStats_concatFormatedBytes_(obj->removed.bytesCount, dst);
				opened = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->removed.count > 0)){
				if(opened) NBString_concat(dst, "/");
				NBString_concatUI32(dst, obj->removed.count);
				opened = TRUE;
			}
		}
	}
}

void TSStreamsStorageStats_concatFileIdx(const STTSStreamsStorageStatsFileIdx* obj, const STTSCfgTelemetryFilesystem* cfg, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL && cfg != NULL && cfg->statsLevel > ENNBLogLevel_None){
		BOOL opened = FALSE;
		//Loaded
		{
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->loaded.bytesCount > 0)){
				if(opened) NBString_concat(dst, ", ");
				TSStreamsStorageStats_concatFormatedBytes_(obj->loaded.bytesCount, dst);
				opened = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->loaded.count > 0)){
				if(opened) NBString_concat(dst, "/");
				NBString_concatUI32(dst, obj->loaded.count);
				opened = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->loaded.recordsCount > 0)){
				if(opened) NBString_concat(dst, ", ");
				NBString_concat(dst, "records: ");
				NBString_concatUI32(dst, obj->loaded.recordsCount);
				opened = TRUE;
			}
		}
		//Added
		{
			BOOL opened2 = opened;
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->added.bytesCount > 0)){
				NBString_concat(dst, opened ? ", added(" : "added(");
				TSStreamsStorageStats_concatFormatedBytes_(obj->added.bytesCount, dst);
				opened = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->added.count > 0)){
				if(opened) NBString_concat(dst, "/");
				NBString_concatUI32(dst, obj->added.count);
				opened = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->added.recordsCount > 0)){
				NBString_concat(dst, opened ? ", added(" : "added(");
				NBString_concat(dst, "records: ");
				NBString_concatUI32(dst, obj->added.recordsCount);
				opened = TRUE;
			}
			if(!opened2 && opened){
				NBString_concat(dst, ")");
			}
		}
		//Removed
		{
			BOOL opened2 = opened;
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->removed.bytesCount > 0)){
				NBString_concat(dst, opened ? ", removed(" : "removed(");
				TSStreamsStorageStats_concatFormatedBytes_(obj->removed.bytesCount, dst);
				opened = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->removed.count > 0)){
				if(opened) NBString_concat(dst, "/");
				NBString_concatUI32(dst, obj->removed.count);
				opened = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->removed.recordsCount > 0)){
				NBString_concat(dst, opened ? ", removed(" : "removed(");
				NBString_concat(dst, "records: ");
				NBString_concatUI32(dst, obj->removed.recordsCount);
				opened = TRUE;
			}
			if(!opened2 && opened){
				NBString_concat(dst, ")");
			}
		}
	}
}

void TSStreamsStorageStats_concatFolders(const STTSStreamsStorageStatsFolders* obj, const STTSCfgTelemetryFilesystem* cfg, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL && cfg != NULL && cfg->statsLevel > ENNBLogLevel_None){
		BOOL opened = FALSE;
		//Streams
		{
			BOOL opened2 = FALSE;
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->streams.loaded > 0)){
				if(opened) NBString_concat(dst, ", ");
				if(!opened2) NBString_concat(dst, "streams: ");
				NBString_concatUI32(dst, obj->streams.loaded);
				opened = opened2 = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->streams.added > 0)){
				if(opened) NBString_concat(dst, ", ");
				if(!opened2) NBString_concat(dst, "streams: ");
				NBString_concat(dst, "+");
				NBString_concatUI32(dst, obj->streams.added);
				opened = opened2 = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->streams.removed > 0)){
				if(opened) NBString_concat(dst, ", ");
				if(!opened2) NBString_concat(dst, "streams: ");
				NBString_concat(dst, "-");
				NBString_concatUI32(dst, obj->streams.removed);
				opened = opened2 = TRUE;
			}
		}
		//StreamsVersions
		{
			BOOL opened2 = FALSE;
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->streamsVersions.loaded > 0)){
				if(opened) NBString_concat(dst, ", ");
				if(!opened2) NBString_concat(dst, "substreams: ");
				NBString_concatUI32(dst, obj->streamsVersions.loaded);
				opened = opened2 = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->streamsVersions.added > 0)){
				if(opened) NBString_concat(dst, ", ");
				if(!opened2) NBString_concat(dst, "substreams: ");
				NBString_concat(dst, "+");
				NBString_concatUI32(dst, obj->streamsVersions.added);
				opened = opened2 = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->streamsVersions.removed > 0)){
				if(opened) NBString_concat(dst, ", ");
				if(!opened2) NBString_concat(dst, "substreams: ");
				NBString_concat(dst, "-");
				NBString_concatUI32(dst, obj->streamsVersions.removed);
				opened = opened2 = TRUE;
			}
		}
		//StreamsVersionsGrps
		{
			BOOL opened2 = FALSE;
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->streamsVersionsGroups.loaded > 0)){
				if(opened) NBString_concat(dst, ", ");
				if(!opened2) NBString_concat(dst, "subfolders: ");
				NBString_concatUI32(dst, obj->streamsVersionsGroups.loaded);
				opened = opened2 = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->streamsVersionsGroups.added > 0)){
				if(opened) NBString_concat(dst, ", ");
				if(!opened2) NBString_concat(dst, "subfolders: ");
				NBString_concat(dst, "+");
				NBString_concatUI32(dst, obj->streamsVersionsGroups.added);
				opened = opened2 = TRUE;
			}
			if(cfg->statsLevel >= ENNBLogLevel_Info && (!ignoreEmpties || obj->streamsVersionsGroups.removed > 0)){
				if(opened) NBString_concat(dst, ", ");
				if(!opened2) NBString_concat(dst, "subfolders: ");
				NBString_concat(dst, "-");
				NBString_concatUI32(dst, obj->streamsVersionsGroups.removed);
				opened = opened2 = TRUE;
			}
		}
	}
}


//Actions

void TSStreamsStorageStats_applyUpdate(STTSStreamsStorageStats* obj, const STTSStreamsStorageStatsUpd* upd){
	STTSStreamsStorageStatsOpq* opq = (STTSStreamsStorageStatsOpq*)obj->opaque;
	if(upd != NULL){
		NBThreadMutex_lock(&opq->mutex);
		{
			//loaded
			{
				//folders
				NBASSERT(opq->data.loaded.folders.streams.loaded + upd->folders.streams.added >= upd->folders.streams.removed)
				NBASSERT(opq->data.loaded.folders.streamsVersions.loaded + upd->folders.streamsVersions.added >= upd->folders.streamsVersions.removed)
				NBASSERT(opq->data.loaded.folders.streamsVersionsGroups.loaded + upd->folders.streamsVersionsGroups.added >= upd->folders.streamsVersionsGroups.removed)
				//payloads
				NBASSERT(opq->data.loaded.files.payloads.loaded.count + upd->files.payloads.added.count >= upd->files.payloads.removed.count)
				NBASSERT(opq->data.loaded.files.payloads.loaded.bytesCount + upd->files.payloads.added.bytesCount >= upd->files.payloads.removed.bytesCount)
				//indexes
				NBASSERT(opq->data.loaded.files.indexes.loaded.count + upd->files.indexes.added.count >= upd->files.indexes.removed.count)
				NBASSERT(opq->data.loaded.files.indexes.loaded.bytesCount + upd->files.indexes.added.bytesCount >= upd->files.indexes.removed.bytesCount)
				NBASSERT(opq->data.loaded.files.indexes.loaded.recordsCount + upd->files.indexes.added.recordsCount >= upd->files.indexes.removed.recordsCount)
				//gaps
				NBASSERT(opq->data.loaded.files.gaps.loaded.count + upd->files.gaps.added.count >= upd->files.gaps.removed.count)
				NBASSERT(opq->data.loaded.files.gaps.loaded.bytesCount + upd->files.gaps.added.bytesCount >= upd->files.gaps.removed.bytesCount)
				NBASSERT(opq->data.loaded.files.gaps.loaded.recordsCount + upd->files.gaps.added.recordsCount >= upd->files.gaps.removed.recordsCount)
				//trash
				NBASSERT(opq->data.loaded.files.trash.loaded.count + upd->files.trash.added.count >= upd->files.trash.removed.count)
				NBASSERT(opq->data.loaded.files.trash.loaded.bytesCount + upd->files.trash.added.bytesCount >= upd->files.trash.removed.bytesCount)
                //others
                NBASSERT(opq->data.loaded.files.others.loaded.count + upd->files.others.added.count >= upd->files.others.removed.count)
                NBASSERT(opq->data.loaded.files.others.loaded.bytesCount + upd->files.others.added.bytesCount >= upd->files.others.removed.bytesCount)
				//folders
				opq->data.loaded.folders.streams.loaded					= opq->data.loaded.folders.streams.loaded + upd->folders.streams.added - upd->folders.streams.removed;
				opq->data.loaded.folders.streamsVersions.loaded			= opq->data.loaded.folders.streamsVersions.loaded + upd->folders.streamsVersions.added - upd->folders.streamsVersions.removed;
				opq->data.loaded.folders.streamsVersionsGroups.loaded	= opq->data.loaded.folders.streamsVersionsGroups.loaded + upd->folders.streamsVersionsGroups.added - upd->folders.streamsVersionsGroups.removed;
				//payloads
				opq->data.loaded.files.payloads.loaded.count		= opq->data.loaded.files.payloads.loaded.count + upd->files.payloads.added.count - upd->files.payloads.removed.count;
				opq->data.loaded.files.payloads.loaded.bytesCount	= opq->data.loaded.files.payloads.loaded.bytesCount + upd->files.payloads.added.bytesCount - upd->files.payloads.removed.bytesCount;
				//indexes
				opq->data.loaded.files.indexes.loaded.count			= opq->data.loaded.files.indexes.loaded.count + upd->files.indexes.added.count - upd->files.indexes.removed.count;
				opq->data.loaded.files.indexes.loaded.bytesCount	= opq->data.loaded.files.indexes.loaded.bytesCount + upd->files.indexes.added.bytesCount - upd->files.indexes.removed.bytesCount;
				opq->data.loaded.files.indexes.loaded.recordsCount	= opq->data.loaded.files.indexes.loaded.recordsCount + upd->files.indexes.added.recordsCount - upd->files.indexes.removed.recordsCount;
				//gaps
				opq->data.loaded.files.gaps.loaded.count		= opq->data.loaded.files.gaps.loaded.count + upd->files.gaps.added.count - upd->files.gaps.removed.count;
				opq->data.loaded.files.gaps.loaded.bytesCount	= opq->data.loaded.files.gaps.loaded.bytesCount + upd->files.gaps.added.bytesCount - upd->files.gaps.removed.bytesCount;
				opq->data.loaded.files.gaps.loaded.recordsCount = opq->data.loaded.files.gaps.loaded.recordsCount + upd->files.gaps.added.recordsCount - upd->files.gaps.removed.recordsCount;
				//trash
				opq->data.loaded.files.trash.loaded.count		= opq->data.loaded.files.trash.loaded.count + upd->files.trash.added.count - upd->files.trash.removed.count;
				opq->data.loaded.files.trash.loaded.bytesCount	= opq->data.loaded.files.trash.loaded.bytesCount + upd->files.trash.added.bytesCount - upd->files.trash.removed.bytesCount;
                //others
                opq->data.loaded.files.others.loaded.count      = opq->data.loaded.files.others.loaded.count + upd->files.others.added.count - upd->files.others.removed.count;
                opq->data.loaded.files.others.loaded.bytesCount = opq->data.loaded.files.others.loaded.bytesCount + upd->files.others.added.bytesCount - upd->files.others.removed.bytesCount;
			}
			//accum
			{
				//folders
				opq->data.accum.folders.streams.added			+= upd->folders.streams.added;
				opq->data.accum.folders.streams.removed			+= upd->folders.streams.removed;
				//
				opq->data.accum.folders.streamsVersions.added	+= upd->folders.streamsVersions.added;
				opq->data.accum.folders.streamsVersions.removed	+= upd->folders.streamsVersions.removed;
				//
				opq->data.accum.folders.streamsVersionsGroups.added += upd->folders.streamsVersionsGroups.added;
				opq->data.accum.folders.streamsVersionsGroups.removed += upd->folders.streamsVersionsGroups.removed;
				//payloads
				opq->data.accum.files.payloads.added.count			+= upd->files.payloads.added.count;
				opq->data.accum.files.payloads.added.bytesCount		+= upd->files.payloads.added.bytesCount;
				opq->data.accum.files.payloads.removed.count		+= upd->files.payloads.removed.count;
				opq->data.accum.files.payloads.removed.bytesCount	+= upd->files.payloads.removed.bytesCount;
				//index
				opq->data.accum.files.indexes.added.count			+= upd->files.indexes.added.count;
				opq->data.accum.files.indexes.added.bytesCount		+= upd->files.indexes.added.bytesCount;
				opq->data.accum.files.indexes.added.recordsCount	+= upd->files.indexes.added.recordsCount;
				opq->data.accum.files.indexes.removed.count			+= upd->files.indexes.removed.count;
				opq->data.accum.files.indexes.removed.bytesCount	+= upd->files.indexes.removed.bytesCount;
				opq->data.accum.files.indexes.removed.recordsCount	+= upd->files.indexes.removed.recordsCount;
				//gaps
				opq->data.accum.files.gaps.added.count			+= upd->files.gaps.added.count;
				opq->data.accum.files.gaps.added.bytesCount		+= upd->files.gaps.added.bytesCount;
				opq->data.accum.files.gaps.added.recordsCount	+= upd->files.gaps.added.recordsCount;
				opq->data.accum.files.gaps.removed.count		+= upd->files.gaps.removed.count;
				opq->data.accum.files.gaps.removed.bytesCount	+= upd->files.gaps.removed.bytesCount;
				opq->data.accum.files.gaps.removed.recordsCount	+= upd->files.gaps.removed.recordsCount;
				//trash
				opq->data.accum.files.trash.added.count			+= upd->files.trash.added.count;
				opq->data.accum.files.trash.added.bytesCount	+= upd->files.trash.added.bytesCount;
				opq->data.accum.files.trash.removed.count		+= upd->files.trash.removed.count;
				opq->data.accum.files.trash.removed.bytesCount	+= upd->files.trash.removed.bytesCount;
                //others
                opq->data.accum.files.others.added.count        += upd->files.others.added.count;
                opq->data.accum.files.others.added.bytesCount   += upd->files.others.added.bytesCount;
                opq->data.accum.files.others.removed.count      += upd->files.others.removed.count;
                opq->data.accum.files.others.removed.bytesCount += upd->files.others.removed.bytesCount;
			}
			//total
			{
				//folders
				opq->data.total.folders.streams.added			+= upd->folders.streams.added;
				opq->data.total.folders.streams.removed			+= upd->folders.streams.removed;
				//
				opq->data.total.folders.streamsVersions.added	+= upd->folders.streamsVersions.added;
				opq->data.total.folders.streamsVersions.removed	+= upd->folders.streamsVersions.removed;
				//
				opq->data.total.folders.streamsVersionsGroups.added += upd->folders.streamsVersionsGroups.added;
				opq->data.total.folders.streamsVersionsGroups.removed += upd->folders.streamsVersionsGroups.removed;
				//payloads
				opq->data.total.files.payloads.added.count			+= upd->files.payloads.added.count;
				opq->data.total.files.payloads.added.bytesCount		+= upd->files.payloads.added.bytesCount;
				opq->data.total.files.payloads.removed.count		+= upd->files.payloads.removed.count;
				opq->data.total.files.payloads.removed.bytesCount	+= upd->files.payloads.removed.bytesCount;
				//index
				opq->data.total.files.indexes.added.count			+= upd->files.indexes.added.count;
				opq->data.total.files.indexes.added.bytesCount		+= upd->files.indexes.added.bytesCount;
				opq->data.total.files.indexes.added.recordsCount	+= upd->files.indexes.added.recordsCount;
				opq->data.total.files.indexes.removed.count			+= upd->files.indexes.removed.count;
				opq->data.total.files.indexes.removed.bytesCount	+= upd->files.indexes.removed.bytesCount;
				opq->data.total.files.indexes.removed.recordsCount	+= upd->files.indexes.removed.recordsCount;
				//gaps
				opq->data.total.files.gaps.added.count			+= upd->files.gaps.added.count;
				opq->data.total.files.gaps.added.bytesCount		+= upd->files.gaps.added.bytesCount;
				opq->data.total.files.gaps.added.recordsCount	+= upd->files.gaps.added.recordsCount;
				opq->data.total.files.gaps.removed.count		+= upd->files.gaps.removed.count;
				opq->data.total.files.gaps.removed.bytesCount	+= upd->files.gaps.removed.bytesCount;
				opq->data.total.files.gaps.removed.recordsCount	+= upd->files.gaps.removed.recordsCount;
				//trash
				opq->data.total.files.trash.added.count			+= upd->files.trash.added.count;
				opq->data.total.files.trash.added.bytesCount	+= upd->files.trash.added.bytesCount;
				opq->data.total.files.trash.removed.count		+= upd->files.trash.removed.count;
				opq->data.total.files.trash.removed.bytesCount	+= upd->files.trash.removed.bytesCount;
                //others
                opq->data.total.files.others.added.count        += upd->files.others.added.count;
                opq->data.total.files.others.added.bytesCount   += upd->files.others.added.bytesCount;
                opq->data.total.files.others.removed.count      += upd->files.others.removed.count;
                opq->data.total.files.others.removed.bytesCount += upd->files.others.removed.bytesCount;
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
