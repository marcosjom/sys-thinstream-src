//
//  TSStreamStorageFilesGrp.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSStreamStorageFilesGrp.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBArraySorted.h"
#include "nb/files/NBFile.h"
#include "nb/crypto/NBSha1.h"
#include "nb/net/NBRtp.h"

//

typedef struct STTSStreamStorageFilesGrpItm_ {
	UI32					iSeq;	//unique-id
	STTSStreamStorageFiles*	file;
} STTSStreamStorageFilesGrpItm;

typedef struct STTSStreamStorageFilesGrpOpq_ {
	STNBThreadMutex		mutex;
	STTSContext*		context;
	//Cfg
	struct {
		STNBFilesystem*	fs;			//filesystem
		STNBString		runId;		//current files-sequence-id (this helps to identify files from the same program-run)
		UI32			iSeq;		//unique-id (numeric)
		STNBString		uid;		//unique-id (string)
		STNBString		parentPath;	//parent folder path
		STNBString		rootPath;	//my root-folder path (parentPath + "/" + uid)
		STTSStreamParamsWrite	params;		//params
		BOOL					paramsNotCloned;	//params
	} cfg;
	//stats
	struct {
		STTSStreamsStorageStats* provider;
	} stats;
	//Viewers
	struct {
		STNBArray		arr;		//STTSStreamLstnr, loaded and current
	} viewers;
	//state
	struct {
		STTSStreamStorageFilesGrpHeader head;
		UI32					seqCur;			//current seq
		//cur
		struct {
			STTSStreamStorageFiles*	files;		//curr file writting
			STTSStreamFilesPos	pos;
		} cur;
		STNBArray				files;			//STTSStreamStorageFilesGrpItm
		STNBTimestampMicro		timeLast;		//last unit
		struct {
			UI32				ops;			//how many write operations
			BOOL				filesChangePnd;	//file change is pending
			//rtp timestamp
			struct {
				UI32			first;
				UI32			last;
			} tsRtp;
			//time-divblock (time divisor)
			struct {
				UI64			last;
			} timediv;
		} written;
		//change
		struct {
			BOOL				isPend;		//change of grp pending
			BOOL				isQueued;	//change of grp was queued for execution
            STNBTimestampMicro  waitUntill; //retry change of grp at this time
			STNBArray			units;		//STTSStreamUnit, units accums for next grp
		} change;
	} state;
	//dbg
#	ifdef NB_CONFIG_INCLUDE_ASSERTS
	struct {
		BOOL		inUnsafeAction;	//currently executing unsafe code
	} dbg;
#	endif
} STTSStreamStorageFilesGrpOpq;

//

void TSStreamStorageFilesGrp_init(STTSStreamStorageFilesGrp* obj){
	STTSStreamStorageFilesGrpOpq* opq = obj->opaque = NBMemory_allocType(STTSStreamStorageFilesGrpOpq);
	NBMemory_setZeroSt(*opq, STTSStreamStorageFilesGrpOpq);
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
			NBArray_init(&opq->state.files, sizeof(STTSStreamStorageFilesGrpItm), NULL);
			NBArray_init(&opq->state.change.units, sizeof(STTSStreamUnit), NULL);
		}
	}
}

void TSStreamStorageFilesGrp_release(STTSStreamStorageFilesGrp* obj){
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	{
		//viewers
		{
			//arr
			{
				NBASSERT(opq->viewers.arr.use == 0) //Should be empty at this point
				//SI32 i; for(i = 0; i < opq->viewers.arr.use; i++){
				//	STTSStreamLstnr v = NBArray_itmPtrAtIndex(&opq->viewers.arr, STTSStreamLstnr, i);
				//}
				NBArray_empty(&opq->viewers.arr);
				NBArray_release(&opq->viewers.arr);
			}
		}
		//state
		{
			if(opq->state.cur.files != NULL){
				STTSStreamsStorageStatsUpd upd;
				NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
				//close
				{
					TSStreamStorageFiles_closeFiles(opq->state.cur.files, TRUE, &upd, NULL);
					opq->state.cur.files = NULL;
				}
				//stats
				if(opq->stats.provider != NULL){
					TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
				}
			}
			//files
			{
				SI32 i; for(i = 0; i < opq->state.files.use; i++){
					STTSStreamStorageFilesGrpItm* files = NBArray_itmPtrAtIndex(&opq->state.files, STTSStreamStorageFilesGrpItm, i);
					if(files->file != NULL){
						TSStreamStorageFiles_release(files->file);
						NBMemory_free(files->file);
						files->file = NULL;
					}
				}
				NBArray_empty(&opq->state.files);
				NBArray_release(&opq->state.files);
			}
			//header
			{
				NBStruct_stRelease(TSStreamStorageFilesGrpHeader_getSharedStructMap(), &opq->state.head, sizeof(opq->state.head));
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
			NBString_release(&opq->cfg.runId);
			NBString_release(&opq->cfg.uid);
			NBString_release(&opq->cfg.parentPath);
			NBString_release(&opq->cfg.rootPath);
			{
				if(opq->cfg.paramsNotCloned){
					NBMemory_setZero(opq->cfg.params);
				} else {
					NBStruct_stRelease(TSStreamParamsWrite_getSharedStructMap(), &opq->cfg.params, sizeof(opq->cfg.params));
				}
			}
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	NBThreadMutex_release(&opq->mutex);
	NBMemory_free(opq);
	obj->opaque = NULL;
}

//Config

BOOL TSStreamStorageFilesGrp_setStatsProvider(STTSStreamStorageFilesGrp* obj, STTSStreamsStorageStats* provider){
	BOOL r = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(opq->state.cur.files == NULL){
		opq->stats.provider = provider;
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamStorageFilesGrp_setFilesystem(STTSStreamStorageFilesGrp* obj, STNBFilesystem* fs){
	BOOL r = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(opq->state.cur.files == NULL){
		opq->cfg.fs = fs;
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamStorageFilesGrp_setRunId(STTSStreamStorageFilesGrp* obj, STTSContext* context, const char* runId){
	BOOL r = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(opq->state.cur.files == NULL){
		NBString_set(&opq->cfg.runId, runId);
		//context
		opq->context = context;
		//
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamStorageFilesGrp_setUid(STTSStreamStorageFilesGrp* obj, const char* parentPath, const UI32 iSeq, const char* uid, BOOL* dstCreatedNow){
	BOOL r = FALSE;
	BOOL createdNow = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(!NBString_strIsEmpty(parentPath) && !NBString_strIsEmpty(uid) && opq->cfg.fs != NULL && opq->state.cur.files == NULL){
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
					//PRINTF_INFO("TSStreamStorageFilesGrp, folder created '%s'.\n", path.str);
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

BOOL TSStreamStorageFilesGrp_setParams(STTSStreamStorageFilesGrp* obj, const STTSStreamParamsWrite* params, const BOOL useRefNotClone){
	BOOL r = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	{
		const STNBStructMap* paramsMap = TSStreamParamsWrite_getSharedStructMap();
		r = TRUE;
		//Validate
		if(params != NULL){
			//
		}
		//Apply
		if(r){
			//release
			{
				if(opq->cfg.paramsNotCloned){
					NBMemory_setZero(opq->cfg.params);
				} else {
					NBStruct_stRelease(paramsMap, &opq->cfg.params, sizeof(opq->cfg.params));
				}
			}
			//clone
			if(params != NULL){
				if(useRefNotClone){
					opq->cfg.params = *params;
					opq->cfg.paramsNotCloned = TRUE;
				} else {
					NBStruct_stClone(paramsMap, params, sizeof(*params), &opq->cfg.params, sizeof(opq->cfg.params));
					opq->cfg.paramsNotCloned = FALSE;
				}
			}
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamStorageFilesGrp_isUid(STTSStreamStorageFilesGrp* obj, const char* uid){
	BOOL r = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(NBString_strIsEqual(opq->cfg.uid.str, uid)){
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

//Viewers

UI32 TSStreamStorageFilesGrp_viewersCount(STTSStreamStorageFilesGrp* obj){
	UI32 r = 0;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	{
		r = opq->viewers.arr.use;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r; 
}

BOOL TSStreamStorageFilesGrp_addViewer(STTSStreamStorageFilesGrp* obj, const STTSStreamLstnr* viewer, UI32* dstCount){
	BOOL r = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque;
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

BOOL TSStreamStorageFilesGrp_removeViewer(STTSStreamStorageFilesGrp* obj, const STTSStreamLstnr* viewer, UI32* dstCount){
	BOOL r = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque;
	if(viewer != NULL){
		NBThreadMutex_lock(&opq->mutex);
		NBASSERT(!opq->dbg.inUnsafeAction)
		{
			SI32 i; for(i = (SI32)opq->viewers.arr.use - 1; i >= 0; i--){
				STTSStreamLstnr* v = NBArray_itmPtrAtIndex(&opq->viewers.arr, STTSStreamLstnr, i);
				if(v->param == viewer->param){
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

typedef struct STTSStreamStorageFilesGrpLoadItm_ {
	UI32		seq;
	UI32		iSubfile;
	const char* filename;
	UI16		lenWithoutExt;
} STTSStreamStorageFilesGrpLoadItm;

BOOL NBCompare_TSStreamStorageFilesGrpLoadItm(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz){
	const STTSStreamStorageFilesGrpLoadItm* o1 = (const STTSStreamStorageFilesGrpLoadItm*)data1;
	const STTSStreamStorageFilesGrpLoadItm* o2 = (const STTSStreamStorageFilesGrpLoadItm*)data2;
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

BOOL TSStreamStorageFilesGrp_loadSortedFilesOwning(STTSStreamStorageFilesGrp* obj, STTSStreamsStorageLoadStreamVerGrp* grp, STTSStreamsStorageLoadStreamVerGrpFiles** files, const SI32 filesSz){
    BOOL r = FALSE;
    STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque;
    NBThreadMutex_lock(&opq->mutex);
    if(grp->head != NULL){
        r = TRUE;
        //apply results
        if(files != NULL && filesSz > 0){
            SI32 i; for(i = 0; i < filesSz && r; i++){
                STTSStreamsStorageLoadStreamVerGrpFiles* s = files[i];
                NBASSERT(s != NULL)
                NBASSERT(s->payload != NULL)
                if(s != NULL && s->payload != NULL){
                    if(opq->state.seqCur < s->seq){
                        opq->state.seqCur = s->seq;
                    }
                    {
                        STTSStreamStorageFilesGrpItm itm2;
                        NBMemory_setZeroSt(itm2, STTSStreamStorageFilesGrpItm);
                        itm2.iSeq = s->seq;
                        itm2.file = s->payload; s->payload = NULL; //consume
                        NBArray_addValue(&opq->state.files, itm2);
                    }
                }
            }
        }
        //
        if(r){
            //Set header
            {
                NBStruct_stRelease(TSStreamStorageFilesGrpHeader_getSharedStructMap(), &opq->state.head, sizeof(opq->state.head));
                NBStruct_stClone(TSStreamStorageFilesGrpHeader_getSharedStructMap(), grp->head, sizeof(*grp->head), &opq->state.head, sizeof(opq->state.head));
            }
            //Set runId
            {
                NBString_set(&opq->cfg.runId, grp->head->runId);
            }
        }
    }
    NBThreadMutex_unlock(&opq->mutex);
    return r;
}

//ToDo: remove
/*
STTSStreamFilesLoadResult TSStreamStorageFilesGrp_load(STTSStreamStorageFilesGrp* obj, const BOOL forceLoadAll, STNBString* optDstErrStr){
	STTSStreamFilesLoadResult r;
	NBMemory_setZeroSt(r, STTSStreamFilesLoadResult);
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(!(opq->cfg.fs != NULL && opq->state.seqCur == 0 && opq->state.cur.files == NULL && opq->cfg.rootPath.length > 0)){
		r.initErrFnd = TRUE;
		if(optDstErrStr != NULL){
			NBString_set(optDstErrStr, "User params error");
		}
	} else {
		STTSStreamStorageFilesGrpHeader head;
		NBMemory_setZeroSt(head, STTSStreamStorageFilesGrpHeader);
		if(!TSStreamsStorageLoader_loadStreamVerGrpHeader(opq->cfg.rootPath.str, &head, NULL)){
			//Nothing
			r.initErrFnd = TRUE;
			if(optDstErrStr != NULL){
				NBString_set(optDstErrStr, "Could not load header");
			}
		} else if(head.fillStartTime <= 0){
			//Nothing
			r.initErrFnd = TRUE;
			if(optDstErrStr != NULL){
				NBString_set(optDstErrStr, "Header start-time is empty");
			}
		} else if(NBString_strIsEmpty(head.runId)){
			//Nothing
			r.initErrFnd = TRUE;
			if(optDstErrStr != NULL){
				NBString_set(optDstErrStr, "Header runId is empty");
			}
		} else {
			//Header
			STTSStreamFilesLoadResult resutls;
			STNBArraySorted sorted;
			STNBArray subFiles;
			STNBString strTmp, subStrs;
			NBArraySorted_init(&sorted, sizeof(STTSStreamStorageFilesGrpLoadItm), NBCompare_TSStreamStorageFilesGrpLoadItm);
			NBArray_init(&subFiles, sizeof(STNBFilesystemFile), NULL);
			NBString_init(&strTmp);
			NBString_initWithSz(&subStrs, 0, 1024, 0.10f);
			NBMemory_setZeroSt(resutls, STTSStreamFilesLoadResult);
			//Files
			if(!NBFilesystem_getFiles(opq->cfg.fs, opq->cfg.rootPath.str, FALSE, &subStrs, &subFiles)){
				r.initErrFnd = TRUE;
				if(optDstErrStr != NULL){
					NBString_set(optDstErrStr, "Could not get files list");
				}
				//PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, NBFilesystem_getFolders failed for '%s'.\n", opq->cfg.rootPath.str);
			} else {
				//Load headers (sorted)
				{
					SI32 i;
					const char* prefix	= "avp-";
					const UI32 prefixLen = NBString_strLenBytes(prefix);
					STTSStreamStorageFilesGrpLoadItm itmTmplt;
					NBMemory_setZeroSt(itmTmplt, STTSStreamStorageFilesGrpLoadItm);
					for(i = 0; i < subFiles.use; i++){
						STNBFilesystemFile* f = NBArray_itmPtrAtIndex(&subFiles, STNBFilesystemFile, i);
						if(!f->isSymLink){
							//Expected '[prefix]uid-YYYYMMDD-hhmmss.ext
							const char* name	= &subStrs.str[f->name];
							const UI32 nameLen	= NBString_strLenBytes(name);
							if(nameLen > prefixLen && NBString_strStartsWith(name, prefix)){
								const char* extChar = name + nameLen;
								const char* seqStart = &name[prefixLen];
								const char* seqAfterEnd = seqStart;
								while(*seqAfterEnd >= '0' && *seqAfterEnd <= '9'){
									seqAfterEnd++;
								}
								while(name < extChar && *extChar != '.'){
									extChar--;
								}
								//Add seq
								if(seqStart < seqAfterEnd && seqAfterEnd <= extChar){
									if(NBString_strIsEqual(extChar, ".264")){
										//add
										NBASSERT(NBString_strIsIntegerBytes(seqStart, (UI32)(seqAfterEnd - seqStart)))
										NBASSERT(*extChar == '.')
										itmTmplt.seq		= (UI32)NBString_strToSI32Bytes(seqStart, (UI32)(seqAfterEnd - seqStart));
										itmTmplt.iSubfile	= i;
										itmTmplt.filename	= name;
										itmTmplt.lenWithoutExt = (UI32)(extChar - name);
										NBArraySorted_addValue(&sorted, itmTmplt);
									}
								}
							}
						}
					}
				}
				//Load in order by runId+iSeq
				NBASSERT(opq->state.seqCur == 0)
				{
					const STTSStreamStorageFilesGrpLoadItm* itm = NBArraySorted_dataPtr(&sorted, STTSStreamStorageFilesGrpLoadItm);
					const STTSStreamStorageFilesGrpLoadItm* itmAfterEnd = itm + sorted.use;
					STNBString strPref;
					NBString_init(&strPref);
					while(itm < itmAfterEnd){
						STTSStreamStorageFiles* files = NBMemory_allocType(STTSStreamStorageFiles);
						TSStreamStorageFiles_init(files);
						//load
						//PRINTF_INFO("TSStreamStorageFilesGrp, processing file: '%s'.\n", itm->filename);
						//filepath-base
						{
							NBString_set(&strPref, opq->cfg.rootPath.str);
							if(strPref.length > 0){ //
								if(strPref.str[strPref.length - 1] != '/' && strPref.str[strPref.length - 1] != '\\'){
									NBString_concatByte(&strPref, '/');
								}
							}
							NBString_concatBytes(&strPref, itm->filename, itm->lenWithoutExt);
						}
						//
						if(!TSStreamStorageFiles_setFilesystem(files, opq->cfg.fs)){
							PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, TSStreamStorageFiles_setFilesystem failed for: '%s' / '%s'.\n", opq->cfg.uid.str, itm->filename);
							NBASSERT(FALSE)
						} else if(!TSStreamStorageFiles_setRunId(files, opq->context, head.runId)){
							PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, TSStreamStorageFiles_setFilesystem failed for: '%s' / '%s'.\n", opq->cfg.uid.str, itm->filename);
							NBASSERT(FALSE)
						} else if(!TSStreamStorageFiles_setParams(files, &opq->cfg.params, TRUE)){
							PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, TSStreamStorageFiles_setParams failed for: '%s' / '%s'.\n", opq->cfg.uid.str, itm->filename);
							NBASSERT(FALSE)
						} else if(!TSStreamStorageFiles_setFilepathsWithoutExtension(files, itm->seq, strPref.str)){
							PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, TSStreamStorageFiles_setFilepathsWithoutExtension failed for: '%s' / '%s'.\n", opq->cfg.uid.str, itm->filename);
							NBASSERT(FALSE)
						} else {
							STTSStreamsStorageStatsUpd upd;
							NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
							{
								BOOL isPayloadEmpty = FALSE;
								STNBString errStr;
								NBString_init(&errStr);
								if(!TSStreamStorageFiles_loadFiles(files, head.runId, forceLoadAll, &isPayloadEmpty, &upd, NULL, &errStr)){
									if(!isPayloadEmpty){
										r.nonEmptyCount++;
										PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, TSStreamStorageFiles_loadFiles failed for: '%s' / '%s': '%s'.\n", opq->cfg.uid.str, itm->filename, errStr.str);
									}
								} else {
									//apply results
									r.successCount++;
									if(!isPayloadEmpty){
										r.nonEmptyCount++;
									}
									//PRINTF_INFO("TSStreamStorageFilesGrp, loaded: '%s' / '%s'.\n", opq->cfg.uid.str, itm->filename);
									if(opq->state.seqCur < itm->seq){
										opq->state.seqCur = itm->seq;
									}
									{
										STTSStreamStorageFilesGrpItm itm2;
										NBMemory_setZeroSt(itm2, STTSStreamStorageFilesGrpItm);
										itm2.iSeq = itm->seq;
										itm2.file = files; files = NULL; //consume
										NBArray_addValue(&opq->state.files, itm2);
									}
									//stats
									if(opq->stats.provider != NULL){
										TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
									}
								}
								NBString_release(&errStr);
							}
						}
						//release (if not consumed)
						if(files != NULL){
							TSStreamStorageFiles_release(files);
							NBMemory_free(files);
							files = NULL;
						}
						//next
						itm++;
					}
					NBString_release(&strPref); 
				}
			}
			//results
			{
				if(resutls.nonEmptyCount != 0){
					r.nonEmptyCount++;
				}
				if(resutls.successCount != 0){
					r.successCount++;
				}
			}
			NBArraySorted_release(&sorted);
			NBArray_release(&subFiles);
			NBString_release(&strTmp);
			NBString_release(&subStrs);
		}
		//Set results
		if(r.successCount > 0){
			//Set header
			{
				NBStruct_stRelease(TSStreamStorageFilesGrpHeader_getSharedStructMap(), &opq->state.head, sizeof(opq->state.head));
				NBStruct_stClone(TSStreamStorageFilesGrpHeader_getSharedStructMap(), &head, sizeof(head), &opq->state.head, sizeof(opq->state.head));
			}
			//Set runId
			{
				NBString_set(&opq->cfg.runId, head.runId); 
			}
		}
		NBStruct_stRelease(TSStreamStorageFilesGrpHeader_getSharedStructMap(), &head, sizeof(head));
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r; 
}*/

BOOL TSStreamStorageFilesGrp_createNextFilesLockedOpq(STTSStreamStorageFilesGrpOpq* opq, STTSStreamUnitsReleaser* optUnitsReleaser){
	BOOL r = FALSE;
	NBASSERT(opq->cfg.params.files.payload.limitBytes > 0)
	if(opq->cfg.params.files.payload.limitBytes > 0){
		//Open new files
		NBASSERT(!opq->state.change.isPend || opq->state.change.isQueued)
		++opq->state.seqCur;
		r = TRUE;
		//Build prefix
		{
			UI32 iFileNameStart = 0;
			const char* namePrefix = "avp";
			STNBString str;
			NBString_init(&str);
			NBString_set(&str, opq->cfg.rootPath.str);
			//Concat slash (if necesary)
			if(str.str[str.length - 1] != '/' && str.str[str.length - 1] != '\\'){
				NBString_concatByte(&str, '/');
			}
			iFileNameStart = str.length;
			//prefix
			NBString_concat(&str, namePrefix);
			//sequential
			NBString_concatByte(&str, '-');
			{
				if(opq->cfg.params.files.nameDigits > 0){
					UI64 baseVal = 1;
					//build 'baseVal'
					UI8 digits = (opq->cfg.params.files.nameDigits < 12 ? opq->cfg.params.files.nameDigits : 12);
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
			}
			//date
			{
				const STNBDatetime now = NBDatetime_getCurLocal();
				NBString_concatByte(&str, '-');
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
				//Sep
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
			//
			{
				STTSStreamStorageFiles* files = NBMemory_allocType(STTSStreamStorageFiles);
				TSStreamStorageFiles_init(files);
				if(!TSStreamStorageFiles_setFilesystem(files, opq->cfg.fs)){
					PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, TSStreamStorageFiles_setFilesystem failed for: '%s'.\n", str.str);
					r = FALSE; NBASSERT(FALSE)
				} else if(!TSStreamStorageFiles_setRunId(files, opq->context, opq->cfg.runId.str)){
					PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, TSStreamStorageFiles_setRunId failed for: '%s'.\n", str.str);
					r = FALSE; NBASSERT(FALSE)
				} else if(!TSStreamStorageFiles_setParams(files, &opq->cfg.params, TRUE)){
					PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, TSStreamStorageFiles_setParams failed for: '%s'.\n", str.str);
					r = FALSE; NBASSERT(FALSE)
				} else if(!TSStreamStorageFiles_setFilepathsWithoutExtension(files, opq->state.seqCur, str.str)){
					PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, TSStreamStorageFiles_setFilepathsWithoutExtension failed for: '%s'.\n", str.str);
					r = FALSE; NBASSERT(FALSE)
				} else if(!TSStreamStorageFiles_createFiles(files, opq->cfg.runId.str)){
					PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, TSStreamStorageFiles_createFiles failed for: '%s'.\n", str.str);
					r = FALSE;
				} else {
					//previous file
					if(opq->state.cur.files != NULL){
						STTSStreamsStorageStatsUpd upd;
						NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
						//close
						{
							TSStreamStorageFiles_closeFiles(opq->state.cur.files, TRUE, &upd, optUnitsReleaser);
							opq->state.cur.files = NULL;
						}
						//stats
						if(opq->stats.provider != NULL){
							TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
						}
					}
					//Consume
					{
						STTSStreamStorageFilesGrpItm itm;
						NBMemory_setZeroSt(itm, STTSStreamStorageFilesGrpItm);
						itm.iSeq = opq->state.seqCur;
						itm.file = files;
						NBArray_addValue(&opq->state.files, itm);
					}
					//set
					{
						opq->state.cur.files	= files;
						if(opq->state.cur.files != NULL){
							opq->state.cur.pos	= TSStreamStorageFiles_getFilesPos(opq->state.cur.files);
						}
					}
					NBMemory_setZero(opq->state.written); //(opq->state.written.filesChangePnd = FALSE)
					files = NULL;
					//write pend units
					{
						if(opq->state.change.units.use > 0){
							//PRINTF_INFO("TSStreamStorageFilesGrp, writting unit-type(%d, +%d) at new files.\n", NBArray_itmPtrAtIndex(&opq->state.change.units, STTSStreamUnit, 0)->hdr.type, opq->state.change.units.use - 1);
							SI32 i; for(i = 0;  i < opq->state.change.units.use; i++){
								STTSStreamUnit* unit = NBArray_itmPtrAtIndex(&opq->state.change.units, STTSStreamUnit, i);
								BOOL streamChanged = FALSE;
								const UI32 written = TSStreamStorageFiles_appendBlocksUnsafe(opq->state.cur.files, unit, 1, optUnitsReleaser, &opq->state.cur.pos, &streamChanged);
								//Stream changed
								if(streamChanged){
									opq->state.written.filesChangePnd = TRUE;
								}
								//Apply written
								if(written > 0){
									//write header (if necesary)
									const BOOL needsUid		= NBString_strIsEmpty(opq->state.head.runId);
									const BOOL needsStart	= (opq->state.head.fillStartTime <= 0);
									if(needsUid || needsStart){
										//Set uid
										if(needsUid && opq->cfg.runId.length > 0){ 
											NBString_strFreeAndNewBuffer(&opq->state.head.runId, opq->cfg.runId.str);
										}
										//Set first
										if(needsStart){
											opq->state.head.fillStartTime = NBDatetime_getCurUTCTimestamp();
										}
										//Save
										if(!TSStreamsStorageLoader_writeStreamVerGrpHeader(opq->cfg.rootPath.str, &opq->state.head)){
											PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, writeCurHeader failed for: '%s' / '%s'.\n", opq->cfg.rootPath.str, opq->cfg.uid.str);
											r = FALSE;
										}
									}
									//update state
									{
										if(opq->state.written.ops == 0){
											opq->state.written.tsRtp.first = unit->desc->hdr.tsRtp;
										}
										opq->state.written.tsRtp.last = unit->desc->hdr.tsRtp;
										opq->state.written.ops++;
									}
								}
							}
							//release units (grouped)
							{
								STTSStreamUnit* u = NBArray_dataPtr(&opq->state.change.units, STTSStreamUnit);
								TSStreamUnit_unitsReleaseGrouped(u, opq->state.change.units.use, optUnitsReleaser);
							}
							NBArray_empty(&opq->state.change.units);
						}
						opq->state.change.isPend = FALSE;
						opq->state.change.isQueued = FALSE;
                        opq->state.change.waitUntill = (STNBTimestampMicro)NBTimestampMicro_Zero;
					}
				}
				//Release (if not consumed)
				if(files != NULL){
					TSStreamStorageFiles_release(files);
					NBMemory_free(files);
					files = NULL;
				}
			}
			NBString_release(&str);
		}
	}
	return r;
}
	
BOOL TSStreamStorageFilesGrp_createNextFiles(STTSStreamStorageFilesGrp* obj, const BOOL ignoreIfCurrentExists){
	BOOL r = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(ignoreIfCurrentExists && opq->state.cur.files != NULL){
		r = TRUE;
	} else {
		r = TSStreamStorageFilesGrp_createNextFilesLockedOpq(opq, NULL);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSStreamStorageFilesGrp_closeFiles(STTSStreamStorageFilesGrp* obj, const BOOL deleteAnyIfEmpty, STTSStreamUnitsReleaser* optUnitsReleaser){
	BOOL r = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	{
		if(opq->state.cur.files != NULL){
			STTSStreamsStorageStatsUpd upd;
			NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
			//close
			{
				TSStreamStorageFiles_closeFiles(opq->state.cur.files, deleteAnyIfEmpty, &upd, optUnitsReleaser);
				opq->state.cur.files = NULL;
			}
			//stats
			if(opq->stats.provider != NULL){
				TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
			}
		}
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSStreamStorageFilesGrp_appendBlocksUnsafe(STTSStreamStorageFilesGrp* obj, STTSStreamUnit* units, const UI32 unitsSz, STTSStreamUnitsReleaser* optUnitsReleaser){
	BOOL r = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque;
	NBASSERT(!opq->dbg.inUnsafeAction)
	IF_NBASSERT(opq->dbg.inUnsafeAction = TRUE;)
	//Write in logical blocks
	if(opq->state.cur.files != NULL){
		r = TRUE;
		//write
		if(units != NULL && unitsSz > 0){
			//write payload
			STTSStreamUnit* unit = units;
			const STTSStreamUnit* unitAfterEnd = unit + unitsSz;
			const UI32 payloadLimitPos = (UI32)opq->cfg.params.files.payload.limitBytes;
			NBASSERT(opq->cfg.params.files.payload.limitBytes > 0 && opq->cfg.params.files.payload.limitBytes <= 0xFFFFFFFF)
			//Write
			while(r && unit < unitAfterEnd){
				//analyze change of second
				if(opq->state.timeLast.timestamp != unit->desc->time.timestamp){
					//time second changed event (optimization, instead of internal constant timestamp request)
					if(opq->state.cur.files != NULL && opq->state.written.ops > 0){
						if(unit->desc->time.timestamp < opq->state.timeLast.timestamp){
							//Clock changed backwards (start new file)
							opq->state.written.filesChangePnd = TRUE;
						} else if(opq->cfg.params.files.payload.dividerSecs > 0){
							//time-divider boundary changed (start new file)
							const UI64 curTimeDiv = (unit->desc->time.timestamp / opq->cfg.params.files.payload.dividerSecs);
							if(opq->state.written.timediv.last != 0 && opq->state.written.timediv.last != curTimeDiv){
								opq->state.written.filesChangePnd = TRUE;
							}
							opq->state.written.timediv.last = curTimeDiv;
						}
					}
				}
				//analyze change of file
				if(
				   unit->desc->isBlockStart
				   && (
					   //real-time-div limiter
					   opq->state.written.filesChangePnd
					   //size-div limiter
					   || (payloadLimitPos != 0 && opq->state.cur.pos.payload >= payloadLimitPos))
				   ){
					//Flag file change
					opq->state.change.isPend = TRUE;
				}
				//process
				if(opq->state.change.isQueued){
					//just add to queue
					STTSStreamUnit unit2;
					TSStreamUnit_init(&unit2);
					TSStreamUnit_setAsOther(&unit2, unit);
					NBArray_addValue(&opq->state.change.units, unit2);
				} else if(opq->state.change.isPend){
                    if(NBTimestampMicro_isGreaterToNow(&opq->state.change.waitUntill)){
                        //wait-time still active (avoid retrying too much)
                    } else {
                        NBThreadMutex_lock(&opq->mutex);
                        {
                            opq->state.change.isQueued = TRUE;
                            //queue
                            {
                                STTSStreamUnit unit2;
                                TSStreamUnit_init(&unit2);
                                TSStreamUnit_setAsOther(&unit2, unit);
                                NBArray_addValue(&opq->state.change.units, unit2);
                            }
                            //create next files
                            if(TSStreamStorageFilesGrp_createNextFilesLockedOpq(opq, optUnitsReleaser)){
                                opq->state.change.isPend = FALSE;
                                opq->state.change.waitUntill = (STNBTimestampMicro)NBTimestampMicro_Zero;
                                NBASSERT(opq->state.cur.files != NULL)
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
				} else {
					//ToDo: optimize writting multiple units
					BOOL streamChanged = FALSE;
					const UI32 written = TSStreamStorageFiles_appendBlocksUnsafe(opq->state.cur.files, unit, 1, optUnitsReleaser, &opq->state.cur.pos, &streamChanged);
					//Stream changed
					if(streamChanged){
						opq->state.written.filesChangePnd = TRUE;
					}
					//Apply written
					if(written > 0){
						//write header (if necesary)
						const BOOL needsUid		= NBString_strIsEmpty(opq->state.head.runId);
						const BOOL needsStart	= (opq->state.head.fillStartTime <= 0);
						if(needsUid || needsStart){
							//Set uid
							if(needsUid && opq->cfg.runId.length > 0){ 
								NBString_strFreeAndNewBuffer(&opq->state.head.runId, opq->cfg.runId.str);
							}
							//Set first
							if(needsStart){
								opq->state.head.fillStartTime = NBDatetime_getCurUTCTimestamp();
							}
							//Save
							if(!TSStreamsStorageLoader_writeStreamVerGrpHeader(opq->cfg.rootPath.str, &opq->state.head)){
								PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, writeCurHeader failed for: '%s' / '%s'.\n", opq->cfg.rootPath.str, opq->cfg.uid.str);
								r = FALSE;
							}
						}
						//update state
						{
							if(opq->state.written.ops == 0){
								opq->state.written.tsRtp.first = unit->desc->hdr.tsRtp;
							}
							opq->state.written.tsRtp.last = unit->desc->hdr.tsRtp;
							opq->state.written.ops++;
						}
						//ToDo: remove
						//write to files
						//opq->state.cur.pos.payload += unit->desc->hdr.prefixSz + unit->desc->chunksTotalSz;
					}
				}
				//next
				opq->state.timeLast = unit->desc->time;
				unit++;
			} //while
		}
	}
	NBASSERT(opq->dbg.inUnsafeAction)
	IF_NBASSERT(opq->dbg.inUnsafeAction = FALSE;)
	return  r;
}

BOOL TSStreamStorageFilesGrp_appendGapsUnsafe(STTSStreamStorageFilesGrp* obj, STTSStreamUnit* units, const UI32 unitsSz, STTSStreamUnitsReleaser* optUnitsReleaser){
	BOOL r = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque;
	NBASSERT(!opq->dbg.inUnsafeAction)
	IF_NBASSERT(opq->dbg.inUnsafeAction = TRUE;)
	if(opq->state.cur.files != NULL){
		//
	}
	NBASSERT(opq->dbg.inUnsafeAction)
	IF_NBASSERT(opq->dbg.inUnsafeAction = FALSE;)
	return  r;
}

//Files

UI32 TSStreamStorageFilesGrp_getFilesCount(STTSStreamStorageFilesGrp* obj){
	UI32 r = 0;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	{
		r = opq->state.files.use;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

void TSStreamStorageFilesGrp_addFilesDesc(STTSStreamStorageFilesGrp* obj, STNBArray* dst){
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque; 
	if(dst != NULL){
		NBThreadMutex_lock(&opq->mutex);
		NBASSERT(!opq->dbg.inUnsafeAction)
		{
			SI32 i; for(i = 0; i < opq->state.files.use; i++){
				STTSStreamStorageFilesGrpItm* files = NBArray_itmPtrAtIndex(&opq->state.files, STTSStreamStorageFilesGrpItm, i);
				if(files->file != NULL){
					const STTSStreamFilesPos pos = TSStreamStorageFiles_getFilesPos(files->file);
					STTSStreamStorageFilesDesc desc;
					NBMemory_setZeroSt(desc, STTSStreamStorageFilesDesc);
					desc.groupId		= opq->cfg.iSeq;
					desc.filesId		= files->iSeq;
					desc.fillStartTime	= pos.fillStartTime;
					desc.filesSize		= (pos.payload + pos.index + pos.gaps);
					NBArray_addValue(dst, desc);
				}
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
}

UI64 TSStreamStorageFilesGrp_deleteFile(STTSStreamStorageFilesGrp* obj, const STTSStreamStorageFilesDesc* d, STTSStreamsStorageStatsUpd* dstUpd){
	UI64 r = 0;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque; 
	if(d != NULL){
		NBThreadMutex_lock(&opq->mutex);
		NBASSERT(!opq->dbg.inUnsafeAction)
		{
			SI32 i; for(i = 0; i < opq->state.files.use; i++){
				STTSStreamStorageFilesGrpItm* files = NBArray_itmPtrAtIndex(&opq->state.files, STTSStreamStorageFilesGrpItm, i);
				if(files->iSeq == d->filesId){
					if(files->file != NULL && files->file != opq->state.cur.files){
						const UI64 bytesRm = TSStreamStorageFiles_deleteFiles(files->file, dstUpd);
						if(bytesRm > 0){
							//remove
							{
								TSStreamStorageFiles_release(files->file);
								NBMemory_free(files->file);
								files->file = NULL;
								//remove
								NBArray_removeItemAtIndex(&opq->state.files, i);
							}
							//result
							r += bytesRm;
						} 
					}
					break;
				}
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	return r;
}

BOOL TSStreamStorageFilesGrp_deleteFolder(STTSStreamStorageFilesGrp* obj, const BOOL onlyIfEmptyExceptHeaderAndEmptyFiles){
	BOOL r = FALSE;
	STTSStreamStorageFilesGrpOpq* opq = (STTSStreamStorageFilesGrpOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)
	if(opq->cfg.fs != NULL){
		BOOL deleteFolder = TRUE;
		//Search other files
		if(onlyIfEmptyExceptHeaderAndEmptyFiles){
			STNBArray found;
			STNBString str, path;
			NBArray_init(&found, sizeof(STNBFilesystemFile), NULL);
			NBString_initWithSz(&str, 0, 1024, 0.10f);
			NBString_init(&path);
			//Search subfolders
			if(deleteFolder){
				NBArray_empty(&found);
				NBString_empty(&str);
				if(!NBFilesystem_getFolders(opq->cfg.fs, opq->cfg.rootPath.str, &str, &found)){
					//PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, NBFilesystem_getFolders failed for '%s'.\n", opq->cfg.rootPath.str);
				} else if(found.use > 0){
					//do not delete (contains subfolders)
					deleteFolder = FALSE;
				}
			}
			//Search files
			if(deleteFolder){
				NBArray_empty(&found);
				NBString_empty(&str);
				if(!NBFilesystem_getFiles(opq->cfg.fs, opq->cfg.rootPath.str, FALSE, &str, &found)){
					//PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, NBFilesystem_getFiles failed for '%s'.\n", opq->cfg.rootPath.str);
				} else {
					SI32 i; for(i = 0; i < found.use; i++){
						STNBFilesystemFile* f = NBArray_itmPtrAtIndex(&found, STNBFilesystemFile, i);
						const char* name	= &str.str[f->name];
						if(NBString_strIsEqual(name, TS_STREAM_STORAGE_FILES_GRP_HEADER_NAME)){
							//ignore header
						} else if(f->isSymLink){
							//do not delete (contains symlink-files)
							deleteFolder = FALSE;
							break;
						} else {
							BOOL isEmpty = FALSE;
							NBString_set(&path, opq->cfg.rootPath.str);
							//Concat slash (if necesary)
							if(path.length > 0){
								if(path.str[path.length - 1] != '/' && path.str[path.length - 1] != '\\'){
									NBString_concatByte(&path, '/');
								}
							}
							NBString_concat(&path, name);
							{
                                STNBFileRef file = NBFile_alloc(NULL);
								if(NBFilesystem_open(opq->cfg.fs, path.str, ENNBFileMode_Read, file)){
									NBFile_lock(file);
									if(NBFile_seek(file, 0, ENNBFileRelative_End)){
										isEmpty = (NBFile_curPos(file) == 0);
									}
									NBFile_unlock(file);
								}
								NBFile_release(&file);
							}
							if(!isEmpty){
								//do not delete (contains non-empty-files)
								deleteFolder = FALSE;
								break;
							}
						}
					}
				}
			}
			NBArray_release(&found);
			NBString_release(&str);
			NBString_release(&path);
		}
		//Apply
		if(deleteFolder){
			r = NBFilesystem_deleteFolder(opq->cfg.fs, opq->cfg.rootPath.str);
			//if(r){
			//	PRINTF_INFO("TSStreamStorageFilesGrp, folder removed: '%s'.\n", opq->cfg.rootPath.str);
			//}
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}
