//
//  TSStreamsStorage.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSStreamsStorage.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBArraySorted.h"
#include "nb/crypto/NBSha1.h"
//
#include "core/TSContext.h"
#include "core/logic/TSFile.h"
#include "core/logic/TSStreamsStorageLoader.h"
//
#include <stdlib.h>	//for rand()

//

typedef struct STTSStreamsStorageStream_ {
	UI32				iSeq;		//unique-id
	STTSStreamStorage*	stream;
	UI32				retainCount;
} STTSStreamsStorageStream;

typedef struct STTSStreamsStorageOpq_ {
	STNBThreadMutex			mutex;
	STTSContext*			context;
	//cfg
	struct {
		STNBFilesystem*		fs;			//filesystem
		STNBString			rootPath;	//root-folder path
	} cfg;
    //load
    struct {
        STTSStreamsStorageLoader* cur;   //current process
        TSStreamsStorageLoadEndedFunc endCallback;
        void*               endCallbackUsrData;
        BOOL                isError;
        BOOL                isCompleted;
    } load;
	//stats
	struct {
		STTSStreamsStorageStats* provider;
	} stats;
	//streams
	struct {
		UI32				seqCur;		//current seq
		STNBString			runId;		//current files-sequence-id (this helps to identify files from the same program-run)
		STNBArray			arr;		//STTSStreamsStorageStream
	} streams;
	//trash-tree
	struct {
		STTSFile			root;		//root file for trash (unloaded files)
	} trash;
} STTSStreamsStorageOpq;


void TSStreamsStorage_init(STTSStreamsStorage* obj){
	STTSStreamsStorageOpq* opq = obj->opaque = NBMemory_allocType(STTSStreamsStorageOpq);
	NBMemory_setZeroSt(*opq, STTSStreamsStorageOpq);
	NBThreadMutex_init(&opq->mutex);
	{
		//Cfg
		{
			NBString_init(&opq->cfg.rootPath);
		}
		//streams
		{
			NBArray_init(&opq->streams.arr, sizeof(STTSStreamsStorageStream), NULL);
			NBString_init(&opq->streams.runId);
		}
		//trash-tree
		{
			TSFile_init(&opq->trash.root);
		}
	}
}

void TSStreamsStorage_release(STTSStreamsStorage* obj){
	STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	{
        //load
        if(opq->load.cur != NULL){
            TSStreamsStorageLoader_release(opq->load.cur);
            NBMemory_free(opq->load.cur);
            opq->load.cur = NULL;
        }
		//cfg
		{
			NBString_release(&opq->cfg.rootPath);
		}
		//streams
		{
			{
				SI32 i; for(i = 0; i < opq->streams.arr.use; i++){
					STTSStreamsStorageStream* s = NBArray_itmPtrAtIndex(&opq->streams.arr, STTSStreamsStorageStream, i);
					TSStreamStorage_release(s->stream);
					NBMemory_free(s->stream);
					s->stream = NULL;
				}
				NBArray_empty(&opq->streams.arr);
				NBArray_release(&opq->streams.arr);
			}
			NBString_release(&opq->streams.runId);
		}
		//trash-tree
		{
			TSFile_release(&opq->trash.root);
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	NBThreadMutex_release(&opq->mutex);
	NBMemory_free(opq);
	obj->opaque = NULL;
}

//Config

BOOL TSStreamsStorage_setStatsProvider(STTSStreamsStorage* obj, STTSStreamsStorageStats* provider){
	BOOL r = FALSE;
	STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	if(opq->streams.arr.use == 0){
		opq->stats.provider = provider;
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamsStorage_setFilesystem(STTSStreamsStorage* obj, STNBFilesystem* fs){
	BOOL r = FALSE;
	STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	if(opq->streams.arr.use == 0){
		opq->cfg.fs = fs;
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamsStorage_setRootPath(STTSStreamsStorage* obj, const char* rootPath){
	BOOL r = FALSE;
	STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	if(!NBString_strIsEmpty(rootPath) && opq->cfg.fs != NULL && opq->streams.arr.use == 0){
		if(!NBString_strIsEqual(opq->cfg.rootPath.str, rootPath)){
			if(NBFilesystem_folderExists(opq->cfg.fs, rootPath)){
				NBString_set(&opq->cfg.rootPath, rootPath);
				r = TRUE;
			} else {
				if(NBFilesystem_createFolderPath(opq->cfg.fs, rootPath)){
					NBString_set(&opq->cfg.rootPath, rootPath);
					r = TRUE;
				}
			}
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamsStorage_setRunId(STTSStreamsStorage* obj, STTSContext* context, const char* runId){
	BOOL r = FALSE;
	STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	if(!NBString_strIsEmpty(runId) && opq->streams.runId.length == 0 && opq->cfg.fs != NULL && opq->streams.arr.use == 0){
		NBString_set(&opq->streams.runId, runId);
		//context
		opq->context = context;
		//
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

//Streams

STTSStreamStorage* TSStreamsStorage_getStreamRetainedLockedOpq_(STTSStreamsStorageOpq* opq, const char* uid, const BOOL createIfNecesary, BOOL* dstIsNewFolder){
    STTSStreamStorage* r = NULL;
    BOOL isNewFolder = FALSE;
    if(opq->streams.runId.length > 0){
        SI32 i; for(i = 0; i < opq->streams.arr.use; i++){
            STTSStreamsStorageStream* s = NBArray_itmPtrAtIndex(&opq->streams.arr, STTSStreamsStorageStream, i);
            if(s->stream != NULL){
                if(TSStreamStorage_isUid(s->stream, uid)){
                    s->retainCount++;
                    r = s->stream;
                    break;
                }
            }
        }
        if(r == NULL && createIfNecesary){
            STTSStreamStorage* ss = NBMemory_allocType(STTSStreamStorage);
            TSStreamStorage_init(ss);
            if(!TSStreamStorage_setStatsProvider(ss, opq->stats.provider)){
                NBASSERT(FALSE)
            } else if(!TSStreamStorage_setFilesystem(ss, opq->cfg.fs)){
                NBASSERT(FALSE)
            } else if(!TSStreamStorage_setRunId(ss, opq->context, opq->streams.runId.str)){
                NBASSERT(FALSE)
            } else if(!TSStreamStorage_setUid(ss, opq->cfg.rootPath.str, (opq->streams.seqCur + 1), uid, &isNewFolder)){
                NBASSERT(FALSE)
            } else {
                //Add
                STTSStreamsStorageStream s;
                NBMemory_setZeroSt(s, STTSStreamsStorageStream);
                s.iSeq            = ++opq->streams.seqCur;
                s.retainCount    = 1;
                s.stream        = r = ss; ss = NULL;
                NBArray_addValue(&opq->streams.arr, s);
                if(opq->stats.provider != NULL){
                    STTSStreamsStorageStatsUpd upd;
                    NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
                    upd.folders.streams.added++;
                    TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
                }
            }
            //Release (if not consumed)
            if(ss != NULL){
                TSStreamStorage_release(ss);
                NBMemory_free(ss);
                ss = NULL;
            }
        }
    }
    if(dstIsNewFolder != NULL){
        *dstIsNewFolder = isNewFolder;
    }
    return r;
}

STTSStreamStorage* TSStreamsStorage_getStreamRetained(STTSStreamsStorage* obj, const char* uid, const BOOL createIfNecesary){
	STTSStreamStorage* r = NULL;
	STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	if(!opq->load.isError && opq->load.isCompleted){
        r = TSStreamsStorage_getStreamRetainedLockedOpq_(opq, uid, createIfNecesary, NULL);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

STTSStreamStorage* TSStreamsStorage_getStreamByIdxRetained(STTSStreamsStorage* obj, const SI32 idx){
	STTSStreamStorage* r = NULL;
	STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	if(opq->streams.runId.length > 0 && idx >= 0 && idx < opq->streams.arr.use && !opq->load.isError && opq->load.isCompleted){
		STTSStreamsStorageStream* s = NBArray_itmPtrAtIndex(&opq->streams.arr, STTSStreamsStorageStream, idx);
		s->retainCount++;
		r = s->stream;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSStreamsStorage_returnStreamLockedOpq_(STTSStreamsStorageOpq* opq, STTSStreamStorage* stream){
    BOOL r = FALSE;
    SI32 i; for(i = 0; i < opq->streams.arr.use; i++){
        STTSStreamsStorageStream* s = NBArray_itmPtrAtIndex(&opq->streams.arr, STTSStreamsStorageStream, i);
        if(s->stream != NULL && s->stream == stream){
            NBASSERT(s->retainCount > 0)
            if(s->retainCount > 0){
                s->retainCount--;
                r = TRUE;
                break;
            }
        }
    }
    NBASSERT(r) //Must be found
    return r;
}

BOOL TSStreamsStorage_returnStream(STTSStreamsStorage* obj, STTSStreamStorage* stream){
	BOOL r = FALSE;
	STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	{
        r = TSStreamsStorage_returnStreamLockedOpq_(opq, stream);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

//Actions

void TSStreamsStorage_TSStreamsStorageLoaderEndedFunc_(const BOOL isSuccess, STTSFile* rootTrash, STTSStreamsStorageLoadStream* const* streams, const SI32 streamsSz, void* usrData){
    STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)usrData;
    STTSStreamsStorageLoader* loaderToRelease = NULL;
    TSStreamsStorageLoadEndedFunc endCallback = NULL;
    void* endCallbackUsrData = NULL;
    BOOL endCallbackIsSuccess = FALSE;
    NBThreadMutex_lock(&opq->mutex);
    {
        if(!isSuccess){
            opq->load.isError = TRUE;
        } else {
            opq->load.isError = FALSE;
            opq->load.isCompleted = FALSE;
            //apply results
            if(streams != NULL && streamsSz > 0){
                SI32 i; for(i = 0; i < streamsSz && !opq->load.isError; i++){
                    const STTSStreamsStorageLoadStream* s = streams[i];
                    NBASSERT(s != NULL)
                    NBASSERT(s->myFldr != NULL)
                    NBASSERT(s->myFldr->path != NULL)
                    if(s != NULL && s->myFldr != NULL && s->myFldr->path != NULL){
                        const SI32 pathLen = NBString_strLenBytes(s->myFldr->path);
                        NBASSERT(pathLen >= s->myFldr->nameLen)
                        if(pathLen >= s->myFldr->nameLen){
                            const char* streamId = &s->myFldr->path[pathLen - s->myFldr->nameLen];
                            const BOOL createIfNecesary = TRUE;
                            BOOL isNewFolder = FALSE;
                            STTSStreamStorage* ss = TSStreamsStorage_getStreamRetainedLockedOpq_(opq, streamId, createIfNecesary, &isNewFolder);
                            NBASSERT(ss != NULL)
                            if(ss != NULL){
                                NBASSERT(!isNewFolder) //folder was just loaded (deleted or fake?)
                                if(isNewFolder){
                                    opq->load.isError = TRUE;
                                    PRINTF_ERROR("TSStreamsStorage, stream folder created (expected to exists, marked as error for security): '%s'.\n", streamId);
                                } else {
                                    //Add versions
                                    if(s->versions.use > 0){
                                        if(!TSStreamStorage_loadVersionsOwning(ss, NBArray_dataPtr(&s->versions, STTSStreamsStorageLoadStreamVer*), s->versions.use)){
                                            opq->load.isError = TRUE;
                                            PRINTF_ERROR("TSStreamsStorage, TSStreamStorage_loadVersionsOwning failed for: '%s'.\n", streamId);
                                        }
                                    }
                                }
                                TSStreamsStorage_returnStreamLockedOpq_(opq, ss);
                            }
                        }
                    }
                }
            }
            //trash (unused) files
            if(rootTrash != NULL){
                //swap
                STTSFile cpy = *rootTrash;
                *rootTrash = opq->trash.root;
                opq->trash.root = cpy;
            }
            //flag as completed
            opq->load.isCompleted = TRUE;
        }
        //
        loaderToRelease = opq->load.cur;
        endCallback = opq->load.endCallback;
        endCallbackUsrData = opq->load.endCallbackUsrData;
        endCallbackIsSuccess = (!opq->load.isError && opq->load.isCompleted);
        opq->load.cur = NULL;
        opq->load.endCallback = NULL;
        opq->load.endCallbackUsrData = NULL;
        
    }
    NBThreadMutex_unlock(&opq->mutex);
    //notify
    if(endCallback != NULL){
        (*endCallback)(endCallbackIsSuccess, endCallbackUsrData);
    }
    //release loader
    if(loaderToRelease != NULL){
        TSStreamsStorageLoader_release(loaderToRelease);
        NBMemory_free(loaderToRelease);
        loaderToRelease = NULL;
    }
}

BOOL TSStreamsStorage_loadStart(STTSStreamsStorage* obj, const BOOL forceLoadAll, const char** ignoreTrashFiles, const UI32 ignoreTrashFilesSz, const BOOL* externalStopFlag, TSStreamsStorageLoadEndedFunc endCallback, void* endCallbackUsrData, STNBThreadsPoolRef optThreadsPool, const UI32 optInitialLoadThreads){
    BOOL r = FALSE;
    STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)obj->opaque;
    NBThreadMutex_lock(&opq->mutex);
    {
        if(opq->load.cur != NULL || opq->load.isError || opq->load.isCompleted){
            //only once
        } else {
            STTSStreamsStorageLoader* cur = NBMemory_allocType(STTSStreamsStorageLoader);
            TSStreamsStorageLoader_init(cur);
            opq->load.cur = cur;
            opq->load.endCallback = endCallback;
            opq->load.endCallbackUsrData = endCallbackUsrData;
            NBThreadMutex_unlock(&opq->mutex);
            {
                //start unlocked (culd notify inmediatly)
                if(!TSStreamsStorageLoader_start(cur, opq->context, opq->cfg.fs, opq->stats.provider, opq->cfg.rootPath.str, forceLoadAll, ignoreTrashFiles, ignoreTrashFilesSz, externalStopFlag, TSStreamsStorage_TSStreamsStorageLoaderEndedFunc_, opq, optThreadsPool, optInitialLoadThreads)){
                    //error
                } else {
                    r = TRUE;
                }
            }
            NBThreadMutex_lock(&opq->mutex);
            //release (if not consumed)
            if(!r){
                opq->load.cur = NULL;
                opq->load.endCallback = NULL;
                opq->load.endCallbackUsrData = NULL;
                TSStreamsStorageLoader_release(cur);
                NBMemory_free(cur);
                cur = NULL;
            }
        }
    }
    NBThreadMutex_unlock(&opq->mutex);
    return r;
}

STTSStreamFilesLoadResult TSStreamsStorage_load(STTSStreamsStorage* obj, const BOOL forceLoadAll, const char** ignoreTrashFiles, const UI32 ignoreTrashFilesSz, const BOOL* externalStopFlag, STNBThreadsPoolRef optThreadsPool, const UI32 optInitialLoadThreads){
	STTSStreamFilesLoadResult r;
	NBMemory_setZeroSt(r, STTSStreamFilesLoadResult);
    /*STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(!(opq->cfg.fs != NULL && opq->streams.runId.length > 0 && opq->streams.arr.use == 0 && opq->cfg.rootPath.length > 0)){
		r.initErrFnd = TRUE;
        PRINTF_ERROR("TSStreamsStorage, load called without configuring and preparing first.\n");
	} else {
        const UI64 timeStartLoad = NBDatetime_getCurUTCTimestamp();
		STTSStreamsStorageStatsUpd upd;
		NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
		{
			STNBArray found;
			STNBString subStrs, strTmp;
            NBArray_initWithSz(&found, sizeof(STNBFilesystemFile), NULL, 64, 256, 0.1f);
			NBString_initWithSz(&subStrs, 0, 1024, 0.10f);
			NBString_init(&strTmp);
			if(!NBFilesystem_getFolders(opq->cfg.fs, opq->cfg.rootPath.str, &subStrs, &found)){
				r.initErrFnd = TRUE;
                PRINTF_ERROR("TSStreamsStorage, NBFilesystem_getFolders failed for '%s'.\n", opq->cfg.rootPath.str);
			} else {
				STTSStreamFilesGrpLoadThreads* loadThreads = NULL;
				STTSStreamsStorageStatsUpd upd;
				STTSFileProps trashFilesBef, trashFilesAft;
				NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
				NBMemory_setZeroSt(trashFilesBef, STTSFileProps);
				NBMemory_setZeroSt(trashFilesAft, STTSFileProps);
				//Create threads (optional)
				if(NBObjRef_isSet(optThreadsPool) && optInitialLoadThreads > 1){
					loadThreads = NBMemory_allocType(STTSStreamFilesGrpLoadThreads);
					NBMemory_setZeroSt(*loadThreads, STTSStreamFilesGrpLoadThreads);
					//externalStopFlag
					{
						loadThreads->externalStopFlag = externalStopFlag;
					}
					//lock
					{
						NBThreadMutex_init(&loadThreads->lock.mutex);
						NBThreadCond_init(&loadThreads->lock.cond);
					}
					//threads
					{
						loadThreads->threads.pool = optThreadsPool;
						loadThreads->threads.runningLimit = optInitialLoadThreads;
					}
				}
				//Map all files (in parallel tasks)
				if(found.use > 0){
					UI32 asycNodesUse = 0;
					STTSStreamsStorageAppendTreeNode* asycNodes = NULL;
					STTSStreamsStorageAppendTreeParams* pool = NULL;
					const UI64 timeStart = NBDatetime_getCurUTCTimestamp();
					//Prepare
					if(loadThreads != NULL){
						NBASSERT(loadThreads->threads.runningCount == 0)
						asycNodes = NBMemory_allocTypes(STTSStreamsStorageAppendTreeNode, found.use);
						//pool
						pool = NBMemory_allocType(STTSStreamsStorageAppendTreeParams);
						NBMemory_setZeroSt(*pool, STTSStreamsStorageAppendTreeParams);
						pool->fs			= opq->cfg.fs;
						pool->loadThreads	= loadThreads;
						pool->stats			= opq->stats.provider;
						//Reset progress
						NBMemory_setZero(loadThreads->progress);
					}
					//Add files or start tasks
					{
						SI32 i; for(i = 0; i < found.use && (externalStopFlag == NULL || !(*externalStopFlag)); i++){
							STNBFilesystemFile* f = NBArray_itmPtrAtIndex(&found, STNBFilesystemFile, i);
							if(!f->isSymLink){
								const char* uid	= &subStrs.str[f->name];
								//Build path
								{
									NBString_set(&strTmp, opq->cfg.rootPath.str);
									if(strTmp.length > 0){
										if(strTmp.str[strTmp.length - 1] != '/' && strTmp.str[strTmp.length - 1] != '\\'){
											NBString_concatByte(&strTmp, '/');
										}
									}
									NBString_concat(&strTmp, uid);
								}
								//Append files
								if(loadThreads == NULL){
									//call on this thread
									STTSFile* subF = TSFile_addSubfile(&opq->trash.root, uid, ENTSFileType_Folder, 0);
									TSStreamsStorage_appendFilesTree_(opq->cfg.fs, strTmp.str, opq->stats.provider, subF, NULL);
								} else {
									//create thread node
									NBASSERT(asycNodesUse < found.use)
									STTSStreamsStorageAppendTreeNode* node = &asycNodes[asycNodesUse++];
									TSStreamsStorageAppendTreeNode_init(node);
									TSStreamsStorageAppendTreeNode_setWithoutPrnt(node, pool, uid, strTmp.str);
									node->isAlloc = FALSE;
									TSStreamsStorage_appendFilesTree_(opq->cfg.fs, strTmp.str, opq->stats.provider, node->node, node);
								}
							}
						}
					}
					//Wait for all threads 
					if(loadThreads != NULL){
						UI64 secsAccum = 0, timeCur, timeLast = NBDatetime_getCurUTCTimestamp();
						NBThreadMutex_lock(&loadThreads->lock.mutex);
						while(loadThreads->threads.runningCount > 0){
							NBThreadCond_wait(&loadThreads->lock.cond, &loadThreads->lock.mutex);
							//
							timeCur		= NBDatetime_getCurUTCTimestamp();
							secsAccum	+= (timeCur - timeLast);
							timeLast	= timeCur;
							if(secsAccum >= 10){ //print every XX seconds
								if(loadThreads->progress.queuedCount > 0){
									PRINTF_INFO("TSStreamsStorage, filesTree mapping %d%% (%d of %d tasks remain).\n", ((100 * loadThreads->progress.completedCount) / loadThreads->progress.queuedCount), (loadThreads->progress.queuedCount - loadThreads->progress.completedCount), loadThreads->progress.queuedCount);
								}
								secsAccum = 0;
							}
						}
						NBThreadMutex_unlock(&loadThreads->lock.mutex);
						//Append files to root
						{
							SI32 i; for(i = 0; i < asycNodesUse; i++){
								STTSStreamsStorageAppendTreeNode* node = &asycNodes[i];
								IF_NBASSERT(STTSFile* subF2 =) TSFile_addSubfileObj(&opq->trash.root, node->node); NBASSERT(subF2 != NULL)
								TSStreamsStorageAppendTreeNode_resignToNode(node); //free without calling 'release'
							}
						}
					}
					//Release
					{
						if(asycNodes != NULL){
							SI32 i; for(i = 0; i < asycNodesUse; i++){
								STTSStreamsStorageAppendTreeNode* node = &asycNodes[i];
								TSStreamsStorageAppendTreeNode_release(node);
							}
							NBMemory_free(asycNodes);
							asycNodes = 0;
							asycNodesUse = 0;
						}
						if(pool != NULL){
							NBMemory_free(pool);
							pool = NULL;
						}
					}
					//results
					trashFilesBef = TSFile_getRecursiveSubfilesCount(&opq->trash.root, ENTSFileType_File);
					PRINTF_CONSOLE("TSStreamsStorage, filesTree mapped (%d secs), loading...\n", (SI32)(NBDatetime_getCurUTCTimestamp() - timeStart));
				}
				//Load streams (in parallel tasks)
				{
                    const UI64 timeStart = NBDatetime_getCurUTCTimestamp();
					SI32 i; for(i = 0; i < found.use && (externalStopFlag == NULL || !(*externalStopFlag)); i++){
						STNBFilesystemFile* f = NBArray_itmPtrAtIndex(&found, STNBFilesystemFile, i);
						if(!f->isSymLink){
							const char* uid	= &subStrs.str[f->name];
							STTSFile* subF	= TSFile_getSubfile(&opq->trash.root, uid); NBASSERT(subF != NULL)
							BOOL isNewFolder = FALSE;
							STTSStreamStorage* ss = NBMemory_allocType(STTSStreamStorage);
							TSStreamStorage_init(ss);
							if(!TSStreamStorage_setStatsProvider(ss, opq->stats.provider)){
								NBASSERT(FALSE)
							} else if(!TSStreamStorage_setFilesystem(ss, opq->cfg.fs)){
								NBASSERT(FALSE)
							} else if(!TSStreamStorage_setRunId(ss, opq->context, opq->streams.runId.str)){
								NBASSERT(FALSE)
							} else if(!TSStreamStorage_setUid(ss, opq->cfg.rootPath.str, (opq->streams.seqCur + 1), uid, &isNewFolder)){
								NBASSERT(FALSE)
							} else {
								//Load
								NBASSERT(!isNewFolder) //should be an existing folder
								if(!isNewFolder){
									const BOOL forceLoadAll = FALSE;
									const STTSStreamFilesLoadResult result = TSStreamStorage_load(ss, forceLoadAll, externalStopFlag);
									if(result.initErrFnd || (result.nonEmptyCount > 0 && result.successCount == 0)){
										PRINTF_WARNING("TSStreamStorage, TSStreamStorage_load failed for '%s'.\n", uid);
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
									STTSStreamsStorageStream s;
									NBMemory_setZeroSt(s, STTSStreamsStorageStream);
									s.iSeq			= ++opq->streams.seqCur;
									s.retainCount	= 1;
									s.stream		= ss; ss = NULL; 
									NBArray_addValue(&opq->streams.arr, s);
									//stats
									upd.folders.streams.added++;
								}
							}
							//Release (if not consumed)
							if(ss != NULL){
								TSStreamStorage_release(ss);
								NBMemory_free(ss);
								ss = NULL;
							}
						}
					}
					trashFilesAft = TSFile_getRecursiveSubfilesCount(&opq->trash.root, ENTSFileType_File);
					PRINTF_CONSOLE("TSStreamsStorage, filesTree loaded (%d secs).\n", (SI32)(NBDatetime_getCurUTCTimestamp() - timeStart));
				}
				//Release threads
				if(loadThreads != NULL){
					//lock
					{
						NBThreadMutex_release(&loadThreads->lock.mutex);
						NBThreadCond_release(&loadThreads->lock.cond);
					}
					NBMemory_free(loadThreads);
					loadThreads = NULL;
				}
				if(opq->stats.provider != NULL){
					TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
				}
				PRINTF_CONSOLE("TSStreamsStorage, loading, trashFiles: %u (%lld MBs) before vs %u (%lld MBs) after.\n", trashFilesBef.count, (trashFilesBef.bytesCount / (1024 * 1024)), trashFilesAft.count, (trashFilesAft.bytesCount / (1024 * 1024)));
			}
			//Maps files in root (listed as trash)
			if(externalStopFlag == NULL || !(*externalStopFlag)){
				NBString_empty(&subStrs);
				NBArray_empty(&found);
				if(!NBFilesystem_getFiles(opq->cfg.fs, opq->cfg.rootPath.str, FALSE, &subStrs, &found)){
					//PRINTF_CONSOLE_ERROR("TSStreamsStorage, NBFilesystem_getFiles failed for '%s'.\n", opq->cfg.rootPath.str);
				} else {
					SI32 i; for(i = 0; i < found.use && (externalStopFlag == NULL || !(*externalStopFlag)); i++){
						STNBFilesystemFile* f = NBArray_itmPtrAtIndex(&found, STNBFilesystemFile, i);
						if(!f->isSymLink){
							const char* name = &subStrs.str[f->name];
							BOOL isAtIgnoreList = FALSE;
							//Ignore-list
							if(ignoreTrashFiles != NULL && ignoreTrashFilesSz > 0){
								UI32 i; for(i = 0; i < ignoreTrashFilesSz; i++){
									if(NBString_strIsEqual(ignoreTrashFiles[i], name)){
										PRINTF_CONSOLE("TSStreamsStorage, trash-file found at ignore-list: '%s'.\n", name);
										isAtIgnoreList = TRUE;
										break;
									}
								}
							}
							if(!isAtIgnoreList){
								//Build path
								{
									NBString_set(&strTmp, opq->cfg.rootPath.str);
									if(strTmp.length > 0){
										if(strTmp.str[strTmp.length - 1] != '/' && strTmp.str[strTmp.length - 1] != '\\'){
											NBString_concatByte(&strTmp, '/');
										}
									}
									NBString_concat(&strTmp, name);
								}
								//Add file
								{
									const SI64 fileSz = NBFile_getSize(strTmp.str);
									if(fileSz >= 0){
										upd.files.trash.added.count++;
										upd.files.trash.added.bytesCount += fileSz;
										TSFile_addSubfile(&opq->trash.root, name, ENTSFileType_File, fileSz);
									}
								}
							}
						}
					}
				}
			}
			NBArray_release(&found);
			NBString_release(&subStrs);
			NBString_release(&strTmp);
		}
        PRINTF_CONSOLE("TSStreamsStorage, load ended.\n", (SI32)(NBDatetime_getCurUTCTimestamp() - timeStartLoad));
		//stats
		if(opq->stats.provider != NULL){
			TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
		}
	}
	NBThreadMutex_unlock(&opq->mutex);*/
	return r;
}

//Files

void TSStreamsStorage_addFilesDesc(STTSStreamsStorage* obj, STNBArray* dst){ //STTSStreamStorageFilesDesc
	STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)obj->opaque;
	if(dst != NULL){
		NBThreadMutex_lock(&opq->mutex);
		{
			SI32 i; for(i = 0; i < opq->streams.arr.use; i++){
				STTSStreamsStorageStream* s = NBArray_itmPtrAtIndex(&opq->streams.arr, STTSStreamsStorageStream, i);
				TSStreamStorage_addFilesDesc(s->stream, dst);
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
}

BOOL NBCompare_TSStreamsStorageStreamByFillStartTime(const ENCompareMode mode, const void* data1, const void* data2, const UI32 dataSz){
	const STTSStreamStorageFilesDesc* o1 = (const STTSStreamStorageFilesDesc*)data1;
	const STTSStreamStorageFilesDesc* o2 = (const STTSStreamStorageFilesDesc*)data2;
	NBASSERT(dataSz == sizeof(*o1))
	if(dataSz == sizeof(*o1)){
		switch (mode) {
			case ENCompareMode_Equal:
				return o1->fillStartTime == o2->fillStartTime ? TRUE : FALSE;
			case ENCompareMode_Lower:
				return o1->fillStartTime < o2->fillStartTime ? TRUE : FALSE;
			case ENCompareMode_LowerOrEqual:
				return o1->fillStartTime <= o2->fillStartTime ? TRUE : FALSE;
			case ENCompareMode_Greater:
				return o1->fillStartTime > o2->fillStartTime ? TRUE : FALSE;
			case ENCompareMode_GreaterOrEqual:
				return o1->fillStartTime >= o2->fillStartTime ? TRUE : FALSE;
			default:
				NBASSERT(FALSE)
				break;
		}
	}
	return FALSE;
}

UI64 TSStreamsStorage_deleteTrashFiles_(STTSStreamsStorageOpq* opq, STNBString* folderPath, STTSFile* folder, const UI64 maxBytesToDelete, STTSStreamsStorageStatsUpd* dstUpd){
	UI64 r = 0;
	const UI32 lenOrg = folderPath->length;
	NBASSERT(folder->name == NULL || folder->props.type == ENTSFileType_Folder) //Must be root or Folder
	if(folder->subfiles != NULL){
		SI32 i; for(i = 0; i < folder->subfiles->use && r < maxBytesToDelete; i++){
			STTSFile* sub = NBArray_itmPtrAtIndex(folder->subfiles, STTSFile, i);
			//reset path
			NBString_removeLastBytes(folderPath, folderPath->length - lenOrg);
			//Add slash
			if(folderPath->length > 0){
				if(folderPath->str[folderPath->length - 1] != '/' && folderPath->str[folderPath->length - 1] != '\\'){
					NBString_concatByte(folderPath, '/');
				}
			}
			//Add sub path
			NBString_concat(folderPath, sub->name);
			//Delete
			if(sub->props.type == ENTSFileType_Folder){
				//Process subfolder
				r += TSStreamsStorage_deleteTrashFiles_(opq, folderPath, sub, (maxBytesToDelete - r), dstUpd);
				//Remove subfolder
				if(sub->subfiles == NULL || sub->subfiles->use == 0){
					//PRINTF_INFO("TSStreamsStorage, deleting trash folder: '%s'.\n", folderPath->str);
					//Delete folder
					BOOL deleteFolder = TRUE;
					//Search other files
					{
						STNBArray found;
						STNBString str, path;
						NBArray_init(&found, sizeof(STNBFilesystemFile), NULL);
						NBString_initWithSz(&str, 0, 1024, 0.10f);
						NBString_init(&path);
						//Search subfolders
						if(deleteFolder){
							NBArray_empty(&found);
							NBString_empty(&str);
							if(!NBFilesystem_getFolders(opq->cfg.fs, folderPath->str, &str, &found)){
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
							if(!NBFilesystem_getFiles(opq->cfg.fs, folderPath->str, FALSE, &str, &found)){
								//PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, NBFilesystem_getFiles failed for '%s'.\n", opq->cfg.rootPath.str);
							} else if(found.use > 0){
								//do not delete (contains subfiles)
								deleteFolder = FALSE;
							}
						}
						NBArray_release(&found);
						NBString_release(&str);
						NBString_release(&path);
					}
					//Apply
					if(deleteFolder){
						NBFilesystem_deleteFolder(opq->cfg.fs, folderPath->str); 
					}
					//Remove record
					TSFile_release(sub);
					NBArray_removeItemAtIndex(folder->subfiles, i);
					i--;
				}
			} else {
				//Delete file
				//PRINTF_INFO("TSStreamsStorage, deleting trash file: '%s' (%lld MBs).\n", folderPath->str, (sub->props.bytesCount / (1024 * 1024)));
				if(NBFilesystem_deleteFile(opq->cfg.fs, folderPath->str)){
					if(dstUpd != NULL){
						dstUpd->files.trash.removed.count += sub->props.count;
						dstUpd->files.trash.removed.bytesCount += sub->props.bytesCount;
					}
					//Remove record
					TSFile_release(sub);
					NBArray_removeItemAtIndex(folder->subfiles, i);
					i--;
				}
			}
		}
	}
	return r;
}

UI64 TSStreamsStorage_deleteOldestFiles(STTSStreamsStorage* obj, const UI64 maxBytesToDelete, const BOOL keepUnknownFiles){
	UI64 r = 0;
	STTSStreamsStorageOpq* opq = (STTSStreamsStorageOpq*)obj->opaque;
	if(maxBytesToDelete > 0){
		STTSStreamsStorageStatsUpd upd;
		NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
		NBThreadMutex_lock(&opq->mutex);
		{
			//Delete trash files
			if(r < maxBytesToDelete && !keepUnknownFiles){
				STNBString str;
				NBString_init(&str);
				NBString_concat(&str, opq->cfg.rootPath.str);
				while(str.length > 0 && (str.str[str.length - 1] == '/' || str.str[str.length - 1] == '\\')){
					NBString_removeLastBytes(&str, 1);
				}
				r += TSStreamsStorage_deleteTrashFiles_(opq, &str, &opq->trash.root, maxBytesToDelete, &upd);
				NBString_release(&str);
			}
			//Delete stream files
			if(r < maxBytesToDelete){
				STNBArray files; //STTSStreamStorageFilesDesc
				NBArray_init(&files, sizeof(STTSStreamStorageFilesDesc), NULL);
				//Add files
				{
					SI32 i; for(i = 0; i < opq->streams.arr.use; i++){
						STTSStreamsStorageStream* s = NBArray_itmPtrAtIndex(&opq->streams.arr, STTSStreamsStorageStream, i);
						TSStreamStorage_addFilesDesc(s->stream, &files);
					}
				}
				//Sort by size
				if(files.use > 0){
					STNBArraySorted sorted;
					NBArraySorted_init(&sorted, sizeof(STTSStreamStorageFilesDesc), NBCompare_TSStreamsStorageStreamByFillStartTime);
					{
						const STTSStreamStorageFilesDesc* arr = NBArray_dataPtr(&files, STTSStreamStorageFilesDesc);
						NBArraySorted_addItems(&sorted, arr, sizeof(STTSStreamStorageFilesDesc), files.use);
						//delete
						{
							SI32 i; for(i = 0; i < sorted.use && r < maxBytesToDelete; i++){
								const STTSStreamStorageFilesDesc* d = NBArraySorted_itmPtrAtIndex(&sorted, STTSStreamStorageFilesDesc, i);
								{
									SI32 i; for(i = 0; i < opq->streams.arr.use; i++){
										STTSStreamsStorageStream* s = NBArray_itmPtrAtIndex(&opq->streams.arr, STTSStreamsStorageStream, i);
										if(s->iSeq == d->streamId){
											r += TSStreamStorage_deleteFile(s->stream, d, &upd);
											break;
										}
									}
								}
							}
						}
					}
					NBArraySorted_release(&sorted);
				}
				NBArray_release(&files);
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
		//stats
		if(opq->stats.provider != NULL){
			TSStreamsStorageStats_applyUpdate(opq->stats.provider, &upd);
		}
	}
	return r;
}
