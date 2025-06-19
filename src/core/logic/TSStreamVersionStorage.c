//
//  TSStreamVersionStorage.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSStreamVersionStorage.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBArraySorted.h"
#include "nb/files/NBFile.h"

typedef struct STTSStreamVersionStorageItm_ {
	UI32						iSeq;	//unique-id
	STTSStreamStorageFilesGrp*	grp;
} STTSStreamVersionStorageItm;

//

typedef struct STTSStreamVersionStorageOpq_ {
	STNBThreadMutex				mutex;
	STTSContext*				context;
	//Cfg
	struct {
		STNBFilesystem*			fs;			//filesystem
		STNBString				runId;		//current files-sequence-id (this helps to identify files from the same program-run)
		UI32					iSeq;		//unique-id (numeric)
		STNBString				uid;		//unique-id (string)
		STNBString				parentPath;	//parent folder path
		STNBString				rootPath;	//my root-folder path (parentPath + "/" + uid)
		STTSStreamParamsWrite*	params;		//params
	} cfg;
	//stats
	struct {
		STTSStreamsStorageStats* provider;
	} stats;
	//Viewers
	struct {
		STNBArray				arr;	//STTSStreamLstnr
	} viewers;
	//state
	struct {
		UI32						seqCur;		//current seq
		STNBDatetime				timeGrp;	//cur grp time
		STTSStreamStorageFilesGrp*	curGrp;		//curr file grp
		STNBArray					grps;		//STTSStreamVersionStorageItm, grps loaded and current
		//change
		struct {
			BOOL					isPend;		//change of grp pending
			BOOL					isQueued;	//change of grp was queued for execution
            STNBTimestampMicro      waitUntill; //retry change of grp at this time
			STNBArray				units;		//STTSStreamUnit, units accums for next grp
		} change;
	} state;
	//dbg
#	ifdef NB_CONFIG_INCLUDE_ASSERTS
	struct {
		BOOL		inUnsafeAction;	//currently executing unsafe code
	} dbg;
#	endif
} STTSStreamVersionStorageOpq;


void TSStreamVersionStorage_init(STTSStreamVersionStorage* obj){
	STTSStreamVersionStorageOpq* opq = obj->opaque = NBMemory_allocType(STTSStreamVersionStorageOpq);
	NBMemory_setZeroSt(*opq, STTSStreamVersionStorageOpq);
	NBThreadMutex_init(&opq->mutex);
	{
		//Cfg
		{
			NBString_init(&opq->cfg.runId);
			NBString_init(&opq->cfg.uid);
			NBString_init(&opq->cfg.parentPath);
			NBString_init(&opq->cfg.rootPath);
		}
		//viewers
		{
			NBArray_init(&opq->viewers.arr, sizeof(STTSStreamLstnr), NULL);
		}
		//state
		{
			NBArray_init(&opq->state.grps, sizeof(STTSStreamVersionStorageItm), NULL);
			NBArray_init(&opq->state.change.units, sizeof(STTSStreamUnit), NULL);
		}
	}
}

void TSStreamVersionStorage_release(STTSStreamVersionStorage* obj){
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	{
		//viewers
		{
			//arr
			{
				NBASSERT(opq->viewers.arr.use == 0) //Should be empty at this point
				//Notify viewers
				{
					SI32 i; for(i = 0; i < opq->viewers.arr.use; i++){
						STTSStreamLstnr* v = NBArray_itmPtrAtIndex(&opq->viewers.arr, STTSStreamLstnr, i);
						if(v->streamEnded != NULL){
							(*v->streamEnded)(v->param);
						}
					}
				}
				NBArray_empty(&opq->viewers.arr);
				NBArray_release(&opq->viewers.arr);
			}
		}
		//state
		{
			if(opq->state.curGrp != NULL){
				TSStreamStorageFilesGrp_closeFiles(opq->state.curGrp, TRUE, NULL);
				opq->state.curGrp = NULL;
			}
			//Grps
			{
				SI32 i; for(i = 0; i < opq->state.grps.use; i++){
					STTSStreamVersionStorageItm* itm = NBArray_itmPtrAtIndex(&opq->state.grps, STTSStreamVersionStorageItm, i);
					if(itm->grp != NULL){
						TSStreamStorageFilesGrp_release(itm->grp);
						NBMemory_free(itm->grp);
						itm->grp = NULL;
					}
				}
				NBArray_empty(&opq->state.grps);
				NBArray_release(&opq->state.grps);
			}
			//change
			{
				STTSStreamUnit* u = NBArray_dataPtr(&opq->state.change.units, STTSStreamUnit);
				TSStreamUnit_unitsReleaseGrouped(u, opq->state.change.units.use, NULL);
				NBArray_empty(&opq->state.change.units);
				NBArray_release(&opq->state.change.units);
			}
		}
		//Cfg
		{
			if(opq->cfg.params != NULL){
				NBStruct_stRelease(TSStreamParamsWrite_getSharedStructMap(), opq->cfg.params, sizeof(*opq->cfg.params));
				NBMemory_free(opq->cfg.params);
				opq->cfg.params = NULL;
			}
			NBString_release(&opq->cfg.runId);
			NBString_release(&opq->cfg.uid);
			NBString_release(&opq->cfg.parentPath);
			NBString_release(&opq->cfg.rootPath);
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	NBThreadMutex_release(&opq->mutex);
	NBMemory_free(opq);
	obj->opaque = NULL;
}

//Config

BOOL TSStreamVersionStorage_setStatsProvider(STTSStreamVersionStorage* obj, STTSStreamsStorageStats* provider){
	BOOL r = FALSE;
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(opq->state.curGrp == NULL){
		opq->stats.provider = provider;
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamVersionStorage_setFilesystem(STTSStreamVersionStorage* obj, STNBFilesystem* fs){
	BOOL r = FALSE;
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(opq->state.curGrp == NULL){
		opq->cfg.fs = fs;
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamVersionStorage_setRunId(STTSStreamVersionStorage* obj, STTSContext* context, const char* runId){
	BOOL r = FALSE;
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(opq->state.curGrp == NULL){
		NBString_set(&opq->cfg.runId, runId);
		//context
		opq->context = context;
		//
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

const char* TSStreamVersionStorage_getUid(STTSStreamVersionStorage* obj){
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque;
	return opq->cfg.uid.str;
}

BOOL TSStreamVersionStorage_setUid(STTSStreamVersionStorage* obj, const char* parentPath, const UI32 iSeq, const char* uid, BOOL* dstCreatedNow){
	BOOL r = FALSE;
	BOOL createdNow = FALSE;
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(!NBString_strIsEmpty(parentPath) && !NBString_strIsEmpty(uid) && opq->cfg.fs != NULL && opq->state.curGrp == NULL){
		if(!NBString_strIsEqual(opq->cfg.parentPath.str, parentPath) || !NBString_strIsEqual(opq->cfg.uid.str, uid)){
			const char* uid2 = uid;
			STNBString path;
			NBString_init(&path);
			NBString_concat(&path, parentPath);
			if(path.length > 0){
				//Concat slash (if necesary)
				if(path.str[path.length - 1] != '/' && path.str[path.length - 1] != '\\'){
					NBString_concatByte(&path, '/');
				}
				//Ignore starting slashes
				while(*uid2 != '\0' && (*uid2 == '/' || *uid2 == '\\')){
					uid2++;
				}
			}
			NBString_concat(&path, uid2);
			if(NBFilesystem_folderExists(opq->cfg.fs, path.str)){
				NBString_set(&opq->cfg.rootPath, path.str);
				NBString_set(&opq->cfg.parentPath, parentPath);
				NBString_set(&opq->cfg.uid, uid);
				opq->cfg.iSeq = iSeq;
				r = TRUE;
			} else {
				if(NBFilesystem_createFolderPath(opq->cfg.fs, path.str)){
					NBString_set(&opq->cfg.rootPath, path.str);
					NBString_set(&opq->cfg.parentPath, parentPath);
					NBString_set(&opq->cfg.uid, uid);
					opq->cfg.iSeq = iSeq;
					r = createdNow = TRUE;
				}
			}
			NBString_release(&path);
		}
	}
	if(dstCreatedNow != NULL){
		*dstCreatedNow = createdNow;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamVersionStorage_setParams(STTSStreamVersionStorage* obj, const STTSStreamParamsWrite* params){
	BOOL r = FALSE;
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	NBASSERT(opq->state.curGrp == NULL)
	if(opq->state.curGrp == NULL){
		const STNBStructMap* paramsMap = TSStreamParamsWrite_getSharedStructMap();
		const STNBStructMap* files2Map = TSStreamParamsFiles_getSharedStructMap();
		STTSStreamParamsFiles files2;
		NBMemory_setZeroSt(files2, STTSStreamParamsFiles);
		r = TRUE;
		//Validate
		if(params != NULL){
			NBStruct_stClone(files2Map, &params->files, sizeof(params->files), &files2, sizeof(files2));
			//Limit (string to bytes)
			if(!NBString_strIsEmpty(files2.payload.limit)){
				const SI64 bytes = NBString_strToBytes(files2.payload.limit);
				//PRINTF_INFO("TSStreamStorageFilesGrp, '%s' parsed to %lld bytes\n", files2.payload.limit, bytes);
				if(bytes <= 0){
					PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, '%s' parsed to %lld bytes (value not valid).\n", files2.payload.limit, bytes);
					r = FALSE;
				} else if(bytes > 0xFFFFFFFFU){ //max: 32-bits value
					PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, '%s' parsed to %lld bytes (value not valid).\n", files2.payload.limit, bytes);
					r = FALSE;
				} else {
					files2.payload.limitBytes = (UI64)bytes;
				}
			} else if(files2.payload.limitBytes <= 0){
				PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, filesystem/streams/limitBytes (%lld bytes) not valid.\n", files2.payload.limitBytes);
				r = FALSE;
			} else if(files2.payload.limitBytes > 0xFFFFFFFFU){ //max: 32-bits value
				PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, filesystem/streams/limitBytes (%lld bytes) not valid.\n", files2.payload.limitBytes);
				r = FALSE;
			}
			//Divider (string to secs)
			if(!NBString_strIsEmpty(files2.payload.divider)){
				const SI64 secs = NBString_strToSecs(files2.payload.divider);
				//PRINTF_INFO("TSStreamStorageFilesGrp, '%s' parsed to %lld secs\n", files2.payload.divider, secs);
				if(secs < 0){
					PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, '%s' parsed to %lld secs (negative not valid).\n", files2.payload.divider, secs);
					r = FALSE;
				} else if(secs > (60 * 60 * 12)){ //tsRtp is 16 bits integer = ~18h (limiting to 12h)
					PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, '%s' parsed to %lld secs (max is 12h).\n", files2.payload.divider, secs);
					r = FALSE;
				} else {
					files2.payload.dividerSecs = (UI32)secs;
				}
			}
		}
		//Apply
		if(r){
			//release
			if(opq->cfg.params != NULL){
				NBStruct_stRelease(paramsMap, opq->cfg.params, sizeof(*opq->cfg.params));
				NBMemory_free(opq->cfg.params);
				opq->cfg.params = NULL;
			}
			//clone
			if(params != NULL){
				opq->cfg.params = NBMemory_allocType(STTSStreamParamsWrite);
				NBMemory_setZeroSt(*opq->cfg.params, STTSStreamParamsWrite);;
				NBStruct_stClone(paramsMap, params, sizeof(*params), opq->cfg.params, sizeof(*opq->cfg.params));
				//Apply interpreted values
				{
					NBStruct_stRelease(files2Map, &opq->cfg.params->files, sizeof(opq->cfg.params->files));
					NBStruct_stClone(files2Map, &files2, sizeof(files2), &opq->cfg.params->files, sizeof(opq->cfg.params->files));
					NBASSERT(opq->cfg.params->files.payload.limitBytes > 0 && opq->cfg.params->files.payload.limitBytes <= 0xFFFFFFFF)
				}
			}
		}
		NBStruct_stRelease(files2Map, &files2, sizeof(files2));
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamVersionStorage_isUid(STTSStreamVersionStorage* obj, const char* uid){
	BOOL r = FALSE;
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(NBString_strIsEqual(opq->cfg.uid.str, uid)){
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

//Viewers

UI32 TSStreamVersionStorage_streamConsumeUnits_(void* obj, STTSStreamUnit* units, const UI32 unitsSz, STTSStreamUnitsReleaser* optUnitsReleaser);
void TSStreamVersionStorage_streamEnded_(void* obj);

STTSStreamLstnr TSStreamVersionStorage_getItfAsStreamLstnr(STTSStreamVersionStorage* obj){
	STTSStreamLstnr r;
	NBMemory_setZero(r);
	r.param					= obj->opaque;
	r.streamConsumeUnits	= TSStreamVersionStorage_streamConsumeUnits_;
	r.streamEnded			= TSStreamVersionStorage_streamEnded_;
	return r;
}

UI32 TSStreamVersionStorage_viewersCount(STTSStreamVersionStorage* obj){
	UI32 r = 0;
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	{
		r = opq->viewers.arr.use;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r; 
}

BOOL TSStreamVersionStorage_addViewer(STTSStreamVersionStorage* obj, const STTSStreamLstnr* viewer, UI32* dstCount){
	BOOL r = FALSE;
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque;
	if(viewer != NULL){
		NBThreadMutex_lock(&opq->mutex);
		NBASSERT(!opq->dbg.inUnsafeAction)
		{
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			{
				SI32 i; for(i = 0; i < opq->viewers.arr.use; i++){
					STTSStreamLstnr* v = NBArray_itmPtrAtIndex(&opq->viewers.arr, STTSStreamLstnr, i);
					NBASSERT(v->param != viewer->param) //Should not be repeated
				}
			}
#			endif
			NBArray_addValue(&opq->viewers.arr, *viewer);
			if(dstCount != NULL){
				*dstCount = opq->viewers.arr.use;
			}
			r = TRUE;
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	return r;
}

BOOL TSStreamVersionStorage_removeViewer(STTSStreamVersionStorage* obj, const void* param, UI32* dstCount){
	BOOL r = FALSE;
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque;
	if(param != NULL){
		NBThreadMutex_lock(&opq->mutex);
		NBASSERT(!opq->dbg.inUnsafeAction)
		{
			SI32 i; for(i = (SI32)opq->viewers.arr.use - 1; i >= 0; i--){
				STTSStreamLstnr* v = NBArray_itmPtrAtIndex(&opq->viewers.arr, STTSStreamLstnr, i);
				if(v->param == param){
					NBASSERT(!r) //Should not be repeated
					NBArray_removeItemAtIndex(&opq->viewers.arr, i);
					r = TRUE;
				}
			}
			if(dstCount != NULL){
				*dstCount = opq->viewers.arr.use;
			}
		}
		NBASSERT(r) //Should be found
		NBThreadMutex_unlock(&opq->mutex);
	}
	return r;
}

//Actions

typedef struct STTSStreamVersionStorageLoadItm_ {
	UI32							seq;
	const char*						filename;
	UI32							iSubfile;
	STTSStreamStorageFilesGrpHeader head;
	//params
	struct {
		STTSStreamVersionStorageOpq* opqLocked;
		BOOL						forceLoadAll;
	} params;
	//results
	STTSStreamFilesLoadResult		result;
} STTSStreamVersionStorageLoadItm;

BOOL NBCompare_TSStreamVersionStorageLoadItm(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz){
	const STTSStreamVersionStorageLoadItm* o1 = (const STTSStreamVersionStorageLoadItm*)data1;
	const STTSStreamVersionStorageLoadItm* o2 = (const STTSStreamVersionStorageLoadItm*)data2;
	NBASSERT(dataSz == sizeof(*o1))
	if(dataSz == sizeof(*o1)){
		switch (mode) {
			case ENCompareMode_Equal:
				return (o1->seq == o2->seq);
			case ENCompareMode_Lower:
				return (o1->seq < o2->seq);
			case ENCompareMode_LowerOrEqual:
				return (o1->seq <= o2->seq);
			case ENCompareMode_Greater:
				return (o1->seq > o2->seq);
			case ENCompareMode_GreaterOrEqual:
				return (o1->seq >= o2->seq);
			default:
				NBASSERT(FALSE)
				break;
		}
	}
	return FALSE;
}

//ToDo: remove
/*
STTSStreamFilesLoadResult TSStreamVersionStorage_loadItmLockedOpq_(STTSStreamVersionStorageOpq* opq, const STTSStreamVersionStorageLoadItm* itm, const BOOL forceLoadAll){
	STTSStreamFilesLoadResult r;
	NBMemory_setZeroSt(r, STTSStreamFilesLoadResult);
	STTSStreamStorageFilesGrp* grp = NBMemory_allocType(STTSStreamStorageFilesGrp);
	TSStreamStorageFilesGrp_init(grp);
	if(!TSStreamStorageFilesGrp_setStatsProvider(grp, opq->stats.provider)){
		r.initErrFnd = TRUE;
		PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setStatsProvider failed for: '%s' / '%s'.\n", opq->cfg.uid.str, itm->filename);
		NBASSERT(FALSE)
	} else if(!TSStreamStorageFilesGrp_setFilesystem(grp, opq->cfg.fs)){
		r.initErrFnd = TRUE;
		PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setFilesystem failed for: '%s' / '%s'.\n", opq->cfg.uid.str, itm->filename);
		NBASSERT(FALSE)
	} else if(!TSStreamStorageFilesGrp_setRunId(grp, opq->context, opq->cfg.runId.str)){
		r.initErrFnd = TRUE;
		PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setRunId failed for: '%s' / '%s'.\n", opq->cfg.uid.str, itm->filename);
		NBASSERT(FALSE)
	} else if(!TSStreamStorageFilesGrp_setUid(grp, opq->cfg.rootPath.str, itm->seq, itm->filename, NULL)){
		r.initErrFnd = TRUE;
		PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setFilepathsWithoutExtension failed for: '%s' / '%s'.\n", opq->cfg.uid.str, itm->filename);
		NBASSERT(FALSE)
	} else if(!TSStreamStorageFilesGrp_setParams(grp, opq->cfg.params, TRUE)){
		r.initErrFnd = TRUE;
		PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setParams failed for: '%s' / '%s'.\n", opq->cfg.uid.str, itm->filename);
		NBASSERT(FALSE)
	} else {
		STNBString errStr;
		NBString_init(&errStr);
		r = TSStreamStorageFilesGrp_load(grp, forceLoadAll, &errStr);
		if(r.initErrFnd){
			PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_load failed for: '%s' / '%s': '%s'.\n", opq->cfg.uid.str, itm->filename, errStr.str);
		} else if(r.nonEmptyCount > 0){
			if(r.successCount <= 0){
				PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_load failed for: '%s' / '%s': '%s'.\n", opq->cfg.uid.str, itm->filename, errStr.str);
			} else {
				NBASSERT(opq->state.curGrp == NULL)
				//Consume
				//PRINTF_INFO("TSStreamVersionStorage, loaded: '%s' / '%s'.\n", opq->cfg.uid.str, itm->filename);
				if(opq->state.seqCur < itm->seq){
					opq->state.seqCur = itm->seq;
				}
				{
					STTSStreamVersionStorageItm itm2;
					NBMemory_setZeroSt(itm2, STTSStreamVersionStorageItm);
					itm2.iSeq	= itm->seq;
					itm2.grp	= grp; grp = NULL;
					NBArray_addValue(&opq->state.grps, itm2);
				}
				//Stats
				if(opq->stats.provider != NULL){
					STTSStreamsStorageStatsUpd upd;
					NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
					upd.folders.streamsVersionsGroups.added++;
					TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
				}
			}
		}
		NBString_release(&errStr);
	} 
	//Release (if not consumed)
	if(grp != NULL){
		TSStreamStorageFilesGrp_release(grp);
		NBMemory_free(grp);
		grp = NULL;
	}
	return r;
}*/

BOOL TSStreamVersionStorage_loadSortedGrpsOwning(STTSStreamVersionStorage* obj, STTSStreamsStorageLoadStreamVerGrp** grps, const SI32 grpsSz){
    BOOL r = TRUE;
    STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque;
    NBThreadMutex_lock(&opq->mutex);
    {
        //apply results
        if(grps != NULL && grpsSz > 0){
            SI32 i; for(i = 0; i < grpsSz && r; i++){
                STTSStreamsStorageLoadStreamVerGrp* s = grps[i];
                NBASSERT(s != NULL)
                NBASSERT(s->myFldr != NULL)
                NBASSERT(s->myFldr->path != NULL)
                if(s != NULL && s->myFldr != NULL && s->myFldr->path != NULL && s->head != NULL){
                    const SI32 pathLen = NBString_strLenBytes(s->myFldr->path);
                    NBASSERT(pathLen >= s->myFldr->nameLen)
                    if(pathLen >= s->myFldr->nameLen){
                        const char* grpId = &s->myFldr->path[pathLen - s->myFldr->nameLen];
                        STTSStreamStorageFilesGrp* grp = NBMemory_allocType(STTSStreamStorageFilesGrp);
                        TSStreamStorageFilesGrp_init(grp);
                        //sort
                        TSStreamsStorageLoadStreamVerGrp_sortFiles(s);
                        //feed
                        if(!TSStreamStorageFilesGrp_setStatsProvider(grp, opq->stats.provider)){
                            r = FALSE;
                            PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setStatsProvider failed for: '%s' / '%s'.\n", opq->cfg.uid.str, grpId);
                            NBASSERT(FALSE)
                        } else if(!TSStreamStorageFilesGrp_setFilesystem(grp, opq->cfg.fs)){
                            r = FALSE;
                            PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setFilesystem failed for: '%s' / '%s'.\n", opq->cfg.uid.str, grpId);
                            NBASSERT(FALSE)
                        } else if(!TSStreamStorageFilesGrp_setRunId(grp, opq->context, s->head->runId)){
                            r = FALSE;
                            PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setRunId failed for: '%s' / '%s'.\n", opq->cfg.uid.str, grpId);
                            NBASSERT(FALSE)
                        } else if(!TSStreamStorageFilesGrp_setUid(grp, opq->cfg.rootPath.str, s->seq, grpId, NULL)){
                            r = FALSE;
                            PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setFilepathsWithoutExtension failed for: '%s' / '%s'.\n", opq->cfg.uid.str, grpId);
                            NBASSERT(FALSE)
                        } else if(!TSStreamStorageFilesGrp_setParams(grp, opq->cfg.params, TRUE)){
                            r = FALSE;
                            PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setParams failed for: '%s' / '%s'.\n", opq->cfg.uid.str, grpId);
                            NBASSERT(FALSE)
                        } else if(!TSStreamStorageFilesGrp_loadSortedFilesOwning(grp, s, NBArray_dataPtr(&s->files, STTSStreamsStorageLoadStreamVerGrpFiles*), s->files.use)){
                            r = FALSE;
                            PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_loadSortedFilesOwning failed for: '%s' / '%s'.\n", opq->cfg.uid.str, grpId);
                            NBASSERT(FALSE)
                        } else {
                            if(opq->state.seqCur < s->seq){
                                opq->state.seqCur = s->seq;
                            }
                            {
                                STTSStreamVersionStorageItm itm2;
                                NBMemory_setZeroSt(itm2, STTSStreamVersionStorageItm);
                                itm2.iSeq   = s->seq;
                                itm2.grp    = grp; grp = NULL; //consume
                                NBArray_addValue(&opq->state.grps, itm2);
                            }
                            //Stats
                            if(opq->stats.provider != NULL){
                                STTSStreamsStorageStatsUpd upd;
                                NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
                                upd.folders.streamsVersionsGroups.added++;
                                TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
                            }
                        }
                        //Release (if not consumed)
                        if(grp != NULL){
                            TSStreamStorageFilesGrp_release(grp);
                            NBMemory_free(grp);
                            grp = NULL;
                        }
                    }
                }
            }
        }
    }
    NBThreadMutex_unlock(&opq->mutex);
    return r;
}

//ToDo: remove
/*
STTSStreamFilesLoadResult TSStreamVersionStorage_load(STTSStreamVersionStorage* obj, const BOOL forceLoadAll, const BOOL* externalStopFlag){
	STTSStreamFilesLoadResult r;
	NBMemory_setZeroSt(r, STTSStreamFilesLoadResult);
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(!(opq->cfg.fs != NULL && opq->state.seqCur == 0 && opq->state.curGrp == NULL && opq->cfg.rootPath.length > 0)){
		r.initErrFnd = TRUE;
	} else {
		STNBArraySorted sorted;
		STNBArray subFiles;
		STNBString subStrs, rootWithSlash, path;
		NBArraySorted_init(&sorted, sizeof(STTSStreamVersionStorageLoadItm), NBCompare_TSStreamVersionStorageLoadItm);
		NBArray_init(&subFiles, sizeof(STNBFilesystemFile), NULL);
		NBString_initWithSz(&subStrs, 0, 1024, 0.10f);
		NBString_init(&rootWithSlash);
		NBString_init(&path);
		//Build root
		{
			NBString_set(&rootWithSlash, opq->cfg.rootPath.str);
			if(rootWithSlash.length > 0){
				if(rootWithSlash.str[rootWithSlash.length - 1] != '/' && rootWithSlash.str[rootWithSlash.length - 1] != '\\'){
					NBString_concatByte(&rootWithSlash, '/');
				}
			}
		}
		//Folders
		if(!NBFilesystem_getFolders(opq->cfg.fs, opq->cfg.rootPath.str, &subStrs, &subFiles)){
			r.initErrFnd = TRUE;
			//PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, NBFilesystem_getFolders failed for '%s'.\n", opq->cfg.rootPath.str);
		} else {			
			//Load headers (sorted)
			{
				SI32 i;
				const char* prefix	= "";
				const UI32 prefixLen = NBString_strLenBytes(prefix);
				STTSStreamVersionStorageLoadItm itmTmplt;
				NBMemory_setZeroSt(itmTmplt, STTSStreamVersionStorageLoadItm);
				for(i = 0; i < subFiles.use && (externalStopFlag == NULL || !(*externalStopFlag)); i++){
					STNBFilesystemFile* f = NBArray_itmPtrAtIndex(&subFiles, STNBFilesystemFile, i);
					if(!f->isSymLink){
						//Expected '[prefix]uid-YYYYMMDD-hhmmss
						const char* name	= &subStrs.str[f->name];
						const UI32 nameLen	= NBString_strLenBytes(name);
						if(nameLen > prefixLen && NBString_strStartsWith(name, prefix)){
							const char* seqStart = &name[prefixLen];
							const char* seqAfterEnd = seqStart;
							while(*seqAfterEnd >= '0' && *seqAfterEnd <= '9'){
								seqAfterEnd++;
							}
							//Add seq
							if(seqStart < seqAfterEnd){
								//load header
								STTSStreamStorageFilesGrpHeader head;
								NBMemory_setZeroSt(head, STTSStreamStorageFilesGrpHeader);
								NBString_set(&path, rootWithSlash.str);
								NBString_concat(&path, name);
								NBASSERT(NBString_strIsIntegerBytes(seqStart, (UI32)(seqAfterEnd - seqStart)))
								if(!TSStreamsStorageLoader_loadStreamVerGrpHeader(path.str, &head, NULL)){
									//not a stream folder?
								} else if(head.runId == NULL){
									NBStruct_stRelease(TSStreamStorageFilesGrpHeader_getSharedStructMap(), &head, sizeof(head));
									NBASSERT(FALSE) //debugging
								} else {
									//add
									itmTmplt.seq		= (UI32)NBString_strToSI32Bytes(seqStart, (UI32)(seqAfterEnd - seqStart));
									itmTmplt.filename	= name;
									itmTmplt.head		= head;
									itmTmplt.iSubfile	= i;
									//params
									itmTmplt.params.opqLocked		= opq;
									itmTmplt.params.forceLoadAll	= forceLoadAll;
									NBArraySorted_addValue(&sorted, itmTmplt);
								}
							}
						}
					}
				}
			}
			//Load content
			{
				//Load in this thread
				STTSStreamVersionStorageLoadItm* itm = NBArraySorted_dataPtr(&sorted, STTSStreamVersionStorageLoadItm);
				const STTSStreamVersionStorageLoadItm* itmAfterEnd = itm + sorted.use;
				//Load in groups by seqNum + runId
				NBASSERT(opq->state.seqCur == 0)
				while(itm < itmAfterEnd && (externalStopFlag == NULL || !(*externalStopFlag))){
					STTSStreamVersionStorageLoadItm* cur = itm++;
					NBASSERT(cur->head.runId != NULL)
					cur->result = TSStreamVersionStorage_loadItmLockedOpq_(cur->params.opqLocked, cur, cur->params.forceLoadAll);
				}
			}
			//Process results
			{
				const STTSStreamVersionStorageLoadItm* itm = NBArraySorted_dataPtr(&sorted, STTSStreamVersionStorageLoadItm);
				const STTSStreamVersionStorageLoadItm* itmAfterEnd = itm + sorted.use;
				//Load in groups by seqNum + runId
				while(itm < itmAfterEnd && (externalStopFlag == NULL || !(*externalStopFlag))){
					const STTSStreamVersionStorageLoadItm* cur = itm++;
					NBASSERT(cur->head.runId != NULL)
					//Consume results
					{
						if(cur->result.initErrFnd || (cur->result.nonEmptyCount > 0 && cur->result.successCount == 0)){
							PRINTF_WARNING("TSStreamVersionStorage, could not load next in seq.\n");
						}
						if(cur->result.nonEmptyCount > 0){
							r.nonEmptyCount++;
						}
						if(cur->result.successCount > 0){
							r.successCount++;
						}
					}
					//Find next
					{
						const STTSStreamVersionStorageLoadItm* next = itm;
						while(next < itmAfterEnd && (externalStopFlag == NULL || !(*externalStopFlag))){
							NBASSERT(next->head.runId != NULL)
							if(NBString_strIsEqual(cur->head.runId, next->head.runId)){
								itm = next;
								break;
							}
							next++;
						}
#						ifdef NB_CONFIG_INCLUDE_ASSERTS
						if((cur + 1) < next){
							//PRINTF_WARNING("TSStreamVersionStorage, %d groups ignored between runId.\n", (SI32)(next - cur - 1));
						}
#						endif
					}
				}
#				ifdef NB_CONFIG_INCLUDE_ASSERTS
				if(itm < itmAfterEnd){
					//PRINTF_WARNING("TSStreamVersionStorage, %d groups not analyzed at the end.\n", (SI32)(itmAfterEnd - itm));
				}
#				endif
			}
			//Release headers
			{
				STTSStreamVersionStorageLoadItm* itm = NBArraySorted_dataPtr(&sorted, STTSStreamVersionStorageLoadItm);
				const STTSStreamVersionStorageLoadItm* itmAfterEnd = itm + sorted.use;
				while(itm < itmAfterEnd){
					NBStruct_stRelease(TSStreamStorageFilesGrpHeader_getSharedStructMap(), &itm->head, sizeof(itm->head));
					itm++;
				}
			}
		}
		NBArraySorted_release(&sorted);
		NBArray_release(&subFiles);
		NBString_release(&subStrs);
		NBString_release(&rootWithSlash);
		NBString_release(&path);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r; 
}*/

BOOL TSStreamVersionStorage_createNextFilesGrpLockedOpq(STTSStreamVersionStorageOpq* opq, STTSStreamUnitsReleaser* optUnitsReleaser){
	BOOL r = FALSE;
	NBASSERT(opq->cfg.params != NULL)
	if(opq->cfg.params != NULL){
		//Open new files
		++opq->state.seqCur;
		r = TRUE;
		//Build prefix
		{
			const STNBDatetime now = NBDatetime_getCurLocal();
			STNBString str;
			NBString_init(&str);
			{
				if(opq->cfg.params->files.nameDigits > 0){
					UI64 baseVal = 1;
					//build 'baseVal'
					UI8 digits = (opq->cfg.params->files.nameDigits < 12 ? opq->cfg.params->files.nameDigits : 12);
					while(digits > 1){
						baseVal *= 10;
						digits--;
					}
					//add zeroes
					while(opq->state.seqCur < baseVal){
						NBString_concatByte(&str, '0');
						baseVal /= 10;
					}
				}
				NBString_concatUI32(&str, opq->state.seqCur);
				NBString_concatByte(&str, '-');
			}
			{
				//Year
				if(now.year < 1000) NBString_concatByte(&str, '0');
				if(now.year < 100) NBString_concatByte(&str, '0');
				if(now.year < 10) NBString_concatByte(&str, '0');
				NBString_concatUI32(&str, now.year);
				//Month
				if(now.month < 10) NBString_concatByte(&str, '0');
				NBString_concatUI32(&str, now.month);
				//Day
				if(now.day < 10) NBString_concatByte(&str, '0');
				NBString_concatUI32(&str, now.day);
				//sep
				NBString_concatByte(&str, '-');
				//Hour
				if(now.hour < 10) NBString_concatByte(&str, '0');
				NBString_concatUI32(&str, now.hour);
				//Min
				if(now.min < 10) NBString_concatByte(&str, '0');
				NBString_concatUI32(&str, now.min);
				//Sec
				if(now.sec < 10) NBString_concatByte(&str, '0');
				NBString_concatUI32(&str, now.sec);
			}
			{
				const BOOL ignoreIfCurrentExists = FALSE;
				STTSStreamStorageFilesGrp* grp = NBMemory_allocType(STTSStreamStorageFilesGrp);
				TSStreamStorageFilesGrp_init(grp);
				if(!TSStreamStorageFilesGrp_setStatsProvider(grp, opq->stats.provider)){
					PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setStatsProvider failed for: '%s' / '%s'.\n", opq->cfg.uid.str, str.str);
					r = FALSE; NBASSERT(FALSE)
				} else if(!TSStreamStorageFilesGrp_setFilesystem(grp, opq->cfg.fs)){
					PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setFilesystem failed for: '%s' / '%s'.\n", opq->cfg.uid.str, str.str);
					r = FALSE; NBASSERT(FALSE)
				} else if(!TSStreamStorageFilesGrp_setRunId(grp, opq->context, opq->cfg.runId.str)){
					PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setRunId failed for: '%s' / '%s'.\n", opq->cfg.uid.str, str.str);
					NBASSERT(FALSE)
				} else if(!TSStreamStorageFilesGrp_setUid(grp, opq->cfg.rootPath.str, opq->state.seqCur, str.str, NULL)){
					PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setFilepathsWithoutExtension failed for: '%s' / '%s'.\n", opq->cfg.uid.str, str.str);
					r = FALSE; NBASSERT(FALSE)
				} else if(!TSStreamStorageFilesGrp_setParams(grp, opq->cfg.params, TRUE)){
					PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_setParams failed for: '%s' / '%s'.\n", opq->cfg.uid.str, str.str);
					r = FALSE; NBASSERT(FALSE)
				} else if(!TSStreamStorageFilesGrp_createNextFiles(grp, ignoreIfCurrentExists)){
					PRINTF_CONSOLE_ERROR("TSStreamVersionStorage, TSStreamStorageFilesGrp_createFiles failed for: '%s' / '%s'.\n", opq->cfg.uid.str, str.str);
					r = FALSE;
				} else {
					//Apply
					if(opq->state.curGrp != NULL){
						TSStreamStorageFilesGrp_closeFiles(opq->state.curGrp, TRUE, optUnitsReleaser);
						opq->state.curGrp = NULL;
					}
					//Consume
					opq->state.timeGrp		= now;
					opq->state.curGrp		= grp;
					//write pend
					{
						if(opq->state.change.units.use > 0){
							//PRINTF_INFO("TSStreamVersionStorage, writting unit-type(%d, +%d) at new grp.\n", NBArray_itmPtrAtIndex(&opq->state.change.units, STTSStreamUnit, 0)->hdr.type, opq->state.change.units.use - 1);
							STTSStreamUnit* units = NBArray_itmPtrAtIndex(&opq->state.change.units, STTSStreamUnit, 0);
							if(!TSStreamStorageFilesGrp_appendBlocksUnsafe(opq->state.curGrp, units, opq->state.change.units.use, optUnitsReleaser)){
								//
							} else {
								//Notify viewers
								SI32 i; for(i = 0; i < opq->viewers.arr.use; i++){
									STTSStreamLstnr* v = NBArray_itmPtrAtIndex(&opq->viewers.arr, STTSStreamLstnr, i);
									if(v->streamConsumeUnits != NULL){
										(*v->streamConsumeUnits)(v->param, units, opq->state.change.units.use, optUnitsReleaser);
									}
								}
							}
							//empty
							{
								STTSStreamUnit* u = NBArray_dataPtr(&opq->state.change.units, STTSStreamUnit);
								TSStreamUnit_unitsReleaseGrouped(u, opq->state.change.units.use, optUnitsReleaser);
								NBArray_empty(&opq->state.change.units);
							}
						}
						NBASSERT(!opq->state.change.isPend || opq->state.change.isQueued)
						opq->state.change.isPend	= FALSE;
						opq->state.change.isQueued	= FALSE;
                        opq->state.change.waitUntill = (STNBTimestampMicro)NBTimestampMicro_Zero;
					}
					{
						STTSStreamVersionStorageItm itm;
						NBMemory_setZeroSt(itm, STTSStreamVersionStorageItm);
						itm.iSeq	= opq->state.seqCur;
						itm.grp		= grp; grp = NULL;
						NBArray_addValue(&opq->state.grps, itm);
					}
					//Stats
					if(opq->stats.provider != NULL){
						STTSStreamsStorageStatsUpd upd;
						NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
						upd.folders.streamsVersionsGroups.added++;
						TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
					}
				}
				//Release (if not consumed)
				if(grp != NULL){
					TSStreamStorageFilesGrp_release(grp);
					NBMemory_free(grp);
					grp = NULL;
				}
			}
			NBString_release(&str);
		}
	}
	return r;
}
	
BOOL TSStreamVersionStorage_createNextFilesGrp(STTSStreamVersionStorage* obj, const BOOL ignoreIfCurrentExists){
	BOOL r = FALSE;
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(ignoreIfCurrentExists && opq->state.curGrp != NULL){
		r = TRUE;
	} else {
		r = TSStreamVersionStorage_createNextFilesGrpLockedOpq(opq, NULL);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSStreamVersionStorage_appendBlocksUnsafe_(STTSStreamVersionStorageOpq* opq, STTSStreamUnit* units, const UI32 unitsSz, STTSStreamUnitsReleaser* optUnitsReleaser){
	BOOL r = FALSE;
	NBASSERT(!opq->dbg.inUnsafeAction)
	IF_NBASSERT(opq->dbg.inUnsafeAction = TRUE;)
	if(opq->state.curGrp != NULL){
		r = TRUE;
		//write
		if(units != NULL && unitsSz > 0){
			NBASSERT(opq->state.timeGrp.year != 0)
			STTSStreamUnit* unit = units;
			const STTSStreamUnit* unitAfterEnd = unit + unitsSz;
			const STNBDatetime now = NBDatetime_getCurLocal(); //ToDo: optimize by removing this
			if(!opq->state.change.isPend){
				opq->state.change.isPend = (opq->state.timeGrp.year != 0
										 && (opq->state.timeGrp.day != now.day
//#										ifdef NB_CONFIG_INCLUDE_ASSERTS
//										|| opq->state.timeGrp.min != now.min	//testing "changing group every minute"
//#										endif
										  )
									  ); 
			}
			//add units
			if(!opq->state.change.isPend){
				//write to current file
				NBASSERT(unit <= unitAfterEnd)
				if(unit < unitAfterEnd){
					if(!TSStreamStorageFilesGrp_appendBlocksUnsafe(opq->state.curGrp, unit, (UI32)(unitAfterEnd - unit), optUnitsReleaser)){
						r = FALSE;
					} else {
						//Notify viewers
						SI32 i; for(i = 0; i < opq->viewers.arr.use; i++){
							STTSStreamLstnr* v = NBArray_itmPtrAtIndex(&opq->viewers.arr, STTSStreamLstnr, i);
							if(v->streamConsumeUnits != NULL){
								(*v->streamConsumeUnits)(v->param, unit, (UI32)(unitAfterEnd - unit), optUnitsReleaser);
							}
						}
					}
				}
			} else {
                //Flush-current and change-group
				if(!opq->state.change.isQueued){
					NBASSERT(opq->state.change.isPend)
					//Calculate start-of-next
					STTSStreamUnit* unit2 = unit; 
					while(unit2 < unitAfterEnd){
						if(unit2->desc->isBlockStart){
							unit = unit2;
							break;
						}
						//next
						unit2++;
					}
					//Flush to old file
					NBASSERT(unit <= unitAfterEnd)
					if(units < unit){
						if(!TSStreamStorageFilesGrp_appendBlocksUnsafe(opq->state.curGrp, units, (UI32)(unit - units), optUnitsReleaser)){
							r = FALSE;
						} else {
							//Notify viewers
							SI32 i; for(i = 0; i < opq->viewers.arr.use; i++){
								STTSStreamLstnr* v = NBArray_itmPtrAtIndex(&opq->viewers.arr, STTSStreamLstnr, i);
								if(v->streamConsumeUnits != NULL){
									(*v->streamConsumeUnits)(v->param, units, (UI32)(unit - units), optUnitsReleaser);
								}
							}
						}
					}
					//queue task
					if(unit < unitAfterEnd){
                        if(NBTimestampMicro_isGreaterToNow(&opq->state.change.waitUntill)){
                            //wait-time still active (avoid retrying too much)
                        } else {
                            NBThreadMutex_lock(&opq->mutex);
                            {
                                opq->state.change.isQueued = TRUE;
                                if(TSStreamVersionStorage_createNextFilesGrpLockedOpq(opq, optUnitsReleaser)){
                                    opq->state.change.isPend = FALSE;
                                    opq->state.change.waitUntill = (STNBTimestampMicro)NBTimestampMicro_Zero;
                                } else {
                                    //error, retry after a wait
                                    //ToDo: implement wait-time as input parameter
                                    opq->state.change.waitUntill = NBTimestampMicro_getMonotonicFast();
                                    opq->state.change.waitUntill.timestamp += 15; //retry in 15 secs
                                }
                                opq->state.change.isQueued = FALSE;
                            }
                            NBThreadMutex_unlock(&opq->mutex);
                        }
					}
				}
				//add remaining to pend array
				if(r && unit < unitAfterEnd){
					while(unit < unitAfterEnd){
						STTSStreamUnit unit2;
						TSStreamUnit_init(&unit2);
						TSStreamUnit_setAsOther(&unit2, unit);
						NBArray_addValue(&opq->state.change.units, unit2);
						//next
						unit++;
					}
				}
			}
			//PRINTF_INFO("TSStreamVersionStorage, %d units to add.\n", unitsSz);
		}
	}
	NBASSERT(opq->dbg.inUnsafeAction)
	IF_NBASSERT(opq->dbg.inUnsafeAction = FALSE;)
	return r;
}

//Files

void TSStreamVersionStorage_addFilesDesc(STTSStreamVersionStorage* obj, STNBArray* dst){ //STTSStreamStorageFilesDesc
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque;
	if(dst != NULL){
		NBThreadMutex_lock(&opq->mutex);
		NBASSERT(!opq->dbg.inUnsafeAction)
		{
			SI32 i = dst->use;
			//Add
			{
				SI32 i; for(i = 0; i < opq->state.grps.use; i++){
					STTSStreamVersionStorageItm* itm = NBArray_itmPtrAtIndex(&opq->state.grps, STTSStreamVersionStorageItm, i);
					if(itm->grp != NULL){
						TSStreamStorageFilesGrp_addFilesDesc(itm->grp, dst);
					}
				}
			}
			//Update new
			{
				for(; i < dst->use; i++){
					STTSStreamStorageFilesDesc* d = NBArray_itmPtrAtIndex(dst, STTSStreamStorageFilesDesc, i);
					NBASSERT(d->filesId != 0)
					NBASSERT(d->groupId != 0)
					NBASSERT(d->versionId == 0)
					d->versionId = opq->cfg.iSeq;
				}
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
}

UI64 TSStreamVersionStorage_deleteFile(STTSStreamVersionStorage* obj, const STTSStreamStorageFilesDesc* d, STTSStreamsStorageStatsUpd* dstUpd){
	UI64 r = 0;
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj->opaque;
	if(d != NULL){
		NBThreadMutex_lock(&opq->mutex);
		NBASSERT(!opq->dbg.inUnsafeAction)
		{
			SI32 i; for(i = 0; i < opq->state.grps.use; i++){
				STTSStreamVersionStorageItm* itm = NBArray_itmPtrAtIndex(&opq->state.grps, STTSStreamVersionStorageItm, i);
				NBASSERT(d->versionId == opq->cfg.iSeq)
				if(itm->iSeq == d->groupId){
					if(itm->grp != NULL){
						const UI64 deletedBytes = TSStreamStorageFilesGrp_deleteFile(itm->grp, d, dstUpd);
						//Remove folder
						if(deletedBytes > 0 && itm->grp != opq->state.curGrp){
							const UI32 filesRemain = TSStreamStorageFilesGrp_getFilesCount(itm->grp);
							if(filesRemain <= 0){
								const BOOL onlyIfEmptyExceptHeaderAndEmptyFiles = TRUE;
								if(TSStreamStorageFilesGrp_deleteFolder(itm->grp, onlyIfEmptyExceptHeaderAndEmptyFiles)){
									//stats
									if(dstUpd != NULL){
										dstUpd->folders.streamsVersionsGroups.removed++;
									}
									//Remove
									{
										TSStreamStorageFilesGrp_release(itm->grp);
										NBMemory_free(itm->grp);
										itm->grp = NULL;
										NBArray_removeItemAtIndex(&opq->state.grps, i);
									}
								}
							}
						}
						//result
						r += deletedBytes;
					}
					break;
				}
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	return r;
}

//consume stream units

UI32 TSStreamVersionStorage_streamConsumeUnits_(void* obj, STTSStreamUnit* units, const UI32 unitsSz, STTSStreamUnitsReleaser* optUnitsReleaser){
	UI32 r = 0;
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj;
	if(units != NULL && unitsSz > 0){
		//IF_NBASSERT(STNBTimestampMicro timeLastAction = NBTimestampMicro_getMonotonicFast(), timeCur; SI64 usDiff;)
		//
		TSStreamVersionStorage_appendBlocksUnsafe_(opq, units, unitsSz, optUnitsReleaser);
		//
        /*
#		ifdef NB_CONFIG_INCLUDE_ASSERTS
		{
			timeCur	= NBTimestampMicro_getMonotonicFast();
			usDiff	= NBTimestampMicro_getDiffInUs(&timeLastAction, &timeCur);
			if(usDiff >= 1000ULL){
				PRINTF_INFO("TSStreamVersionStorage_streamConsumeUnits_ took %llu.%llu%llums.\n", (usDiff / 1000ULL), (usDiff % 1000ULL) % 100ULL, (usDiff % 100ULL) % 10ULL);
			}
			timeLastAction = timeCur;
		}
#		endif
        */
	}
	return r;
}

void TSStreamVersionStorage_streamEnded_(void* obj){
	STTSStreamVersionStorageOpq* opq = (STTSStreamVersionStorageOpq*)obj;
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	{
		//Notify viewers
		SI32 i; for(i = 0; i < opq->viewers.arr.use; i++){
			STTSStreamLstnr* v = NBArray_itmPtrAtIndex(&opq->viewers.arr, STTSStreamLstnr, i);
			if(v->streamEnded != NULL){
				(*v->streamEnded)(v->param);
			}
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
}
