//
//  TSStreamStorage.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSStreamStorage.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBThreadMutex.h"

typedef struct STTSStreamStorageVersion_ {
	UI32						iSeq;	//unique-id
	STTSStreamVersionStorage*	version;
	UI32						retainCount;
} STTSStreamStorageVersion;

typedef struct STTSStreamStorageOpq_ {
	STNBThreadMutex		mutex;
	STTSContext*		context;
	STNBThreadsPoolRef	pool;
	//Cfg
	struct {
		STNBFilesystem*	fs;			//filesystem
		STNBString		runId;		//current files-sequence-id (this helps to identify files from the same program-run)
		UI32			iSeq;		//unique-id (numeric)
		STNBString		uid;		//unique-id (string)
		STNBString		parentPath;	//parent folder path
		STNBString		rootPath;	//my root-folder path (parentPath + "/" + uid)
	} cfg;
	//stats
	struct {
		STTSStreamsStorageStats* provider;
	} stats;
	//versions
	struct {
		UI32			seqCur;		//current seq
		STNBArray		arr;		//STTSStreamStorageVersion
	} versions;
} STTSStreamStorageOpq;


void TSStreamStorage_init(STTSStreamStorage* obj){
	STTSStreamStorageOpq* opq = obj->opaque = NBMemory_allocType(STTSStreamStorageOpq);
	NBMemory_setZeroSt(*opq, STTSStreamStorageOpq);
	NBThreadMutex_init(&opq->mutex);
	{
		//Cfg
		{
			NBString_init(&opq->cfg.runId);
			NBString_init(&opq->cfg.uid);
			NBString_init(&opq->cfg.parentPath);
			NBString_init(&opq->cfg.rootPath);
		}
		//versions
		{
			NBArray_init(&opq->versions.arr, sizeof(STTSStreamStorageVersion), NULL);
		}
	}
}

void TSStreamStorage_release(STTSStreamStorage* obj){
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	{
		//versions
		{
			{
				SI32 i; for(i = 0; i < opq->versions.arr.use; i++){
					STTSStreamStorageVersion* v = NBArray_itmPtrAtIndex(&opq->versions.arr, STTSStreamStorageVersion, i);
					TSStreamVersionStorage_release(v->version);
					NBMemory_free(v->version);
					v->version = NULL;
				}
				NBArray_empty(&opq->versions.arr);
				NBArray_release(&opq->versions.arr);
			}
		}
		//Cfg
		{
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

BOOL TSStreamStorage_setStatsProvider(STTSStreamStorage* obj, STTSStreamsStorageStats* provider){
	BOOL r = FALSE;
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	if(opq->versions.arr.use == 0){
		opq->stats.provider = provider;
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamStorage_setFilesystem(STTSStreamStorage* obj, STNBFilesystem* fs){
	BOOL r = FALSE;
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	if(opq->versions.arr.use == 0){
		opq->cfg.fs = fs;
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamStorage_setRunId(STTSStreamStorage* obj, STTSContext* context, const char* runId){
	BOOL r = FALSE;
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	if(opq->versions.arr.use == 0){
		NBString_set(&opq->cfg.runId, runId);
		//context
		opq->context = context;
		//
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

const char* TSStreamStorage_getUid(STTSStreamStorage* obj){
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque;
	return opq->cfg.uid.str;
}

BOOL TSStreamStorage_setUid(STTSStreamStorage* obj, const char* parentPath, const UI32 iSeq, const char* uid, BOOL* dstCreatedNow){
	BOOL r = FALSE;
	BOOL createdNow = FALSE;
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	if(!NBString_strIsEmpty(parentPath) && !NBString_strIsEmpty(uid) && opq->cfg.fs != NULL && opq->versions.arr.use == 0){
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
					createdNow = TRUE;
					r = TRUE;
				}
			}
			NBString_release(&path);
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	if(dstCreatedNow != NULL){
		*dstCreatedNow = createdNow;
	}
	return  r;
}

BOOL TSStreamStorage_isUid(STTSStreamStorage* obj, const char* uid){
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque; 
	return  NBString_strIsEqual(opq->cfg.uid.str, uid);
}

//Versions

STTSStreamVersionStorage* TSStreamStorage_getVersionRetainedLockedOpq_(STTSStreamStorageOpq* opq, const char* uid, const BOOL createIfNecesary, BOOL* dstIsNewFolder){
    BOOL isNewFolder = FALSE;
    STTSStreamVersionStorage* r = NULL;
    SI32 i; for(i = 0; i < opq->versions.arr.use; i++){
        STTSStreamStorageVersion* v = NBArray_itmPtrAtIndex(&opq->versions.arr, STTSStreamStorageVersion, i);
        if(v->version != NULL){
            if(TSStreamVersionStorage_isUid(v->version, uid)){
                v->retainCount++;
                r = v->version;
                break;
            }
        }
    }
    if(r == NULL && createIfNecesary){
        STTSStreamVersionStorage* vv = NBMemory_allocType(STTSStreamVersionStorage);
        TSStreamVersionStorage_init(vv);
        if(!TSStreamVersionStorage_setStatsProvider(vv, opq->stats.provider)){
            NBASSERT(FALSE)
        } else if(!TSStreamVersionStorage_setFilesystem(vv, opq->cfg.fs)){
            NBASSERT(FALSE)
        } else if(!TSStreamVersionStorage_setRunId(vv, opq->context, opq->cfg.runId.str)){
            NBASSERT(FALSE)
        } else if(!TSStreamVersionStorage_setUid(vv, opq->cfg.rootPath.str, (opq->versions.seqCur + 1), uid, &isNewFolder)){
            NBASSERT(FALSE)
        } else {
            //Add
            STTSStreamStorageVersion v;
            NBMemory_setZeroSt(v, STTSStreamStorageVersion);
            v.iSeq            = ++opq->versions.seqCur;
            v.retainCount    = 1;
            v.version        = r = vv; vv = NULL;
            NBArray_addValue(&opq->versions.arr, v);
            if(opq->stats.provider != NULL){
                STTSStreamsStorageStatsUpd upd;
                NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
                upd.folders.streamsVersions.added++;
                TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
            }
        }
        //Release (if not consumed)
        if(vv != NULL){
            TSStreamVersionStorage_release(vv);
            NBMemory_free(vv);
            vv = NULL;
        }
    }
    if(dstIsNewFolder != NULL){
        *dstIsNewFolder = isNewFolder;
    }
    return r;
}

STTSStreamVersionStorage* TSStreamStorage_getVersionRetained(STTSStreamStorage* obj, const char* uid, const BOOL createIfNecesary){
	STTSStreamVersionStorage* r = NULL;
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	{
        r = TSStreamStorage_getVersionRetainedLockedOpq_(opq, uid, createIfNecesary, NULL);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

STTSStreamVersionStorage* TSStreamStorage_getVersionByIdxRetained(STTSStreamStorage* obj, const SI32 idx){
	STTSStreamVersionStorage* r = NULL;
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	if(idx >= 0 && idx < opq->versions.arr.use){
		STTSStreamStorageVersion* v = NBArray_itmPtrAtIndex(&opq->versions.arr, STTSStreamStorageVersion, idx);
		v->retainCount++;
		r = v->version;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSStreamStorage_returnVersionLockedOpq_(STTSStreamStorageOpq* opq, STTSStreamVersionStorage* version){
    BOOL r = FALSE;
    SI32 i; for(i = 0; i < opq->versions.arr.use; i++){
        STTSStreamStorageVersion* v = NBArray_itmPtrAtIndex(&opq->versions.arr, STTSStreamStorageVersion, i);
        if(v->version != NULL && v->version == version){
            NBASSERT(v->retainCount > 0)
            if(v->retainCount > 0){
                v->retainCount--;
                r = TRUE;
                break;
            }
        }
    }
    NBASSERT(r) //Must be found
    return r;
}

BOOL TSStreamStorage_returnVersion(STTSStreamStorage* obj, STTSStreamVersionStorage* version){
	BOOL r = FALSE;
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	{
        r = TSStreamStorage_returnVersionLockedOpq_(opq, version);
		NBASSERT(r) //Must be found
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

//Actions

BOOL TSStreamStorage_loadVersionsOwning(STTSStreamStorage* obj, STTSStreamsStorageLoadStreamVer** vers, const SI32 versSz){
    BOOL r = TRUE;
    STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque;
    NBThreadMutex_lock(&opq->mutex);
    {
        //apply results
        if(vers != NULL && versSz > 0){
            SI32 i; for(i = 0; i < versSz && r; i++){
                STTSStreamsStorageLoadStreamVer* s = vers[i];
                NBASSERT(s != NULL)
                NBASSERT(s->myFldr != NULL)
                NBASSERT(s->myFldr->path != NULL)
                if(s != NULL && s->myFldr != NULL && s->myFldr->path != NULL){
                    const SI32 pathLen = NBString_strLenBytes(s->myFldr->path);
                    NBASSERT(pathLen >= s->myFldr->nameLen)
                    if(pathLen >= s->myFldr->nameLen){
                        const char* verId = &s->myFldr->path[pathLen - s->myFldr->nameLen];
                        const BOOL createIfNecesary = TRUE;
                        BOOL isNewFolder = FALSE;
                        STTSStreamVersionStorage* ss = TSStreamStorage_getVersionRetainedLockedOpq_(opq, verId, createIfNecesary, &isNewFolder);
                        NBASSERT(ss != NULL)
                        if(ss != NULL){
                            NBASSERT(!isNewFolder) //folder was just loaded (deleted or fake?)
                            if(isNewFolder){
                                r = FALSE;
                                PRINTF_ERROR("TSStreamStorage, version folder created (expected to exists, marked as error for security): '%s'.\n", verId);
                            } else {
                                //Add grps
                                if(s->grps.use > 0){
                                    //sort and feed
                                    TSStreamsStorageLoadStreamVer_sortGrps(s);
                                    if(!TSStreamVersionStorage_loadSortedGrpsOwning(ss, NBArray_dataPtr(&s->grps, STTSStreamsStorageLoadStreamVerGrp*), s->grps.use)){
                                        r = FALSE;
                                        PRINTF_ERROR("TSStreamStorage, TSStreamStorage_loadGrps failed for: '%s'.\n", verId);
                                    }
                                }
                            }
                            TSStreamStorage_returnVersionLockedOpq_(opq, ss);
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
STTSStreamFilesLoadResult TSStreamStorage_load(STTSStreamStorage* obj, const BOOL forceLoadAll, const BOOL* externalStopFlag){
	STTSStreamFilesLoadResult r;
	NBMemory_setZeroSt(r, STTSStreamFilesLoadResult);
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	if(opq->cfg.fs != NULL && opq->versions.arr.use == 0 && opq->cfg.rootPath.length > 0){
		STNBArray subFiles;
		STNBString subStrs;
		NBArray_init(&subFiles, sizeof(STNBFilesystemFile), NULL);
		NBString_initWithSz(&subStrs, 0, 1024, 0.10f);
		if(!NBFilesystem_getFolders(opq->cfg.fs, opq->cfg.rootPath.str, &subStrs, &subFiles)){
			//PRINTF_CONSOLE_ERROR("TSStreamStorage, NBFilesystem_getFolders failed for '%s'.\n", opq->cfg.rootPath.str);
		} else {
			SI32 i; for(i = 0; i < subFiles.use && (externalStopFlag == NULL || !(*externalStopFlag)); i++){
				STNBFilesystemFile* f = NBArray_itmPtrAtIndex(&subFiles, STNBFilesystemFile, i);
				if(!f->isSymLink){
					BOOL isNewFolder = FALSE;
					const char* uid	= &subStrs.str[f->name];
					STTSStreamVersionStorage* vv = NBMemory_allocType(STTSStreamVersionStorage);
					TSStreamVersionStorage_init(vv);
					if(!TSStreamVersionStorage_setStatsProvider(vv, opq->stats.provider)){
						NBASSERT(FALSE)
					} else if(!TSStreamVersionStorage_setFilesystem(vv, opq->cfg.fs)){
						NBASSERT(FALSE)
					} else if(!TSStreamVersionStorage_setRunId(vv, opq->context, opq->cfg.runId.str)){
						NBASSERT(FALSE)
					} else if(!TSStreamVersionStorage_setUid(vv, opq->cfg.rootPath.str, (opq->versions.seqCur + 1), uid, &isNewFolder)){
						NBASSERT(FALSE)
					} else {
						//Load
						NBASSERT(!isNewFolder)
						if(!isNewFolder){
							const BOOL forceLoadAll = FALSE;
							const STTSStreamFilesLoadResult result = TSStreamVersionStorage_load(vv, forceLoadAll, externalStopFlag); 
							if(result.initErrFnd || (result.nonEmptyCount > 0 && result.successCount == 0)){
								PRINTF_WARNING("TSStreamStorage, TSStreamVersionStorage_load failed for '%s'.\n", uid);
							}
							if(result.nonEmptyCount > 0){
								r.nonEmptyCount++;
							}
							if(result.successCount > 0){
								r.successCount++;
							}
						}
						//Add
						{
							STTSStreamStorageVersion v;
							NBMemory_setZeroSt(v, STTSStreamStorageVersion);
							v.iSeq			= ++opq->versions.seqCur;
							v.retainCount	= 0;
							v.version		= vv; vv = NULL; 
							NBArray_addValue(&opq->versions.arr, v);
							if(opq->stats.provider != NULL){
								STTSStreamsStorageStatsUpd upd;
								NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
								upd.folders.streamsVersions.added++;
								TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
							}
						}
					}
					//Release (if not consumed)
					if(vv != NULL){
						TSStreamVersionStorage_release(vv);
						NBMemory_free(vv);
						vv = NULL;
					}
				}
			}
		}
		NBArray_release(&subFiles);
		NBString_release(&subStrs);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}*/

//Files

void TSStreamStorage_addFilesDesc(STTSStreamStorage* obj, STNBArray* dst){ //STTSStreamStorageFilesDesc
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque; 
	if(dst != NULL){
		NBThreadMutex_lock(&opq->mutex);
		{
			SI32 i = dst->use;
			//Add
			{
				SI32 i; for(i = 0; i < opq->versions.arr.use; i++){
					STTSStreamStorageVersion* v = NBArray_itmPtrAtIndex(&opq->versions.arr, STTSStreamStorageVersion, i);
					TSStreamVersionStorage_addFilesDesc(v->version, dst);
				}
			}
			//Update new
			for(; i < dst->use; i++){
				STTSStreamStorageFilesDesc* d = NBArray_itmPtrAtIndex(dst, STTSStreamStorageFilesDesc, i);
				NBASSERT(d->filesId != 0)
				NBASSERT(d->groupId != 0)
				NBASSERT(d->versionId != 0)
				NBASSERT(d->streamId == 0)
				d->streamId = opq->cfg.iSeq;
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
}

UI64 TSStreamStorage_deleteFile(STTSStreamStorage* obj, const STTSStreamStorageFilesDesc* d, STTSStreamsStorageStatsUpd* dstUpd){
	UI64 r = 0;
	STTSStreamStorageOpq* opq = (STTSStreamStorageOpq*)obj->opaque; 
	if(d != NULL){
		NBThreadMutex_lock(&opq->mutex);
		{
			SI32 i; for(i = 0; i < opq->versions.arr.use; i++){
				STTSStreamStorageVersion* v = NBArray_itmPtrAtIndex(&opq->versions.arr, STTSStreamStorageVersion, i);
				NBASSERT(d->streamId == opq->cfg.iSeq)
				if(v->iSeq == d->versionId){
					r += TSStreamVersionStorage_deleteFile(v->version, d, dstUpd);
					break;
				}
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	return r;
}
