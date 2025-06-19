//
//  TSStreamsStorageLoader.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSStreamsStorageLoader.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBArraySorted.h"
#include "nb/crypto/NBSha1.h"
//
#include "core/TSContext.h"
#include "core/logic/TSFile.h"
//
#include <stdlib.h>	//for rand()

//TSStreamStorageFilesGrpHeader

//Stream storage files grp header

STNBStructMapsRec TSStreamStorageFilesGrpHeader_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamStorageFilesGrpHeader_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamStorageFilesGrpHeader_sharedStructMap);
    if(TSStreamStorageFilesGrpHeader_sharedStructMap.map == NULL){
        STTSStreamStorageFilesGrpHeader s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamStorageFilesGrpHeader);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addStrPtrM(map, s, runId);
        NBStructMap_addUIntM(map, s, fillStartTime);
        TSStreamStorageFilesGrpHeader_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamStorageFilesGrpHeader_sharedStructMap);
    return TSStreamStorageFilesGrpHeader_sharedStructMap.map;
}


//STTSStreamsStorageLoaderOpq

typedef struct STTSStreamsStorageLoaderOpq_ {
    SI32                    retainCount;
	STNBThreadMutex			mutex;
    STNBThreadCond          cond;
	STTSContext*			context;
    //params
    struct {
        STTSContext*        context;
        STNBFilesystem*     fs;
        STTSStreamsStorageStats* provider;
        char*               rootPath;
        BOOL                forceLoadAll;
        char**              ignoreFilenames;
        UI32                ignoreFilenamesSz;
        const BOOL*         externalStopFlag;   //readOnly
        STNBThreadsPoolRef  optThreadsPool;
        UI32                optInitialLoadThreads;
        //
        TSStreamsStorageLoaderEndedFunc endCallback;
        void* endCallbackUsrData;
    } params;
    //threads
    struct {
        SI32                runningCount;
        SI32                waitingCount;    //to determine if action ended (waitingCount >= runningCount)
    } threads;
    //
    BOOL                    isErr;
    BOOL                    isCompleted;
    UI64                    timeStartLoad;
    //mapping
    struct {
        BOOL        isCompleted;
        UI64        timeStart;
        UI64        timeLast;
        UI64        secsAccum;
        SI32        iFileNext;  //next record in 'files' to be processed
        STNBArray   folders;    //STTSStreamsStorageLoadFolderRng*, flat folders list
        STNBArray   files;      //STTSFile*, flat files list (first file is root)
        //counts
        struct {
            STTSFilesTreeCounts tree; //after mapping
        } counts;
    } mapping;
    //loading
    struct {
        BOOL        isCompleted;   //all threads completedx
        UI64        timeStart;
        UI64        timeLast;
        UI64        secsAccum;
        STNBArray   streams;      //STTSStreamsStorageLoadStream*, flat streams list
        SI32        iStreamNext;  //next record in 'streams' to be processed
        //counts (tree elements, lineal counts)
        struct {
            SI32    versions;
            SI32    groups;
            STTSFilesTreeCounts tree; //after loading (remaining trash files)
        } counts;
    } loading;
    //loaded
    struct {
        //counts (tree elements, lineal counts)
        struct {
            SI32    groups;
        } counts;
    } loaded;
} STTSStreamsStorageLoaderOpq;

void TSStreamsStorageLoader_init(STTSStreamsStorageLoader* obj){
	STTSStreamsStorageLoaderOpq* opq = obj->opaque = NBMemory_allocType(STTSStreamsStorageLoaderOpq);
	NBMemory_setZeroSt(*opq, STTSStreamsStorageLoaderOpq);
    //
    opq->retainCount = 1;
    //
	NBThreadMutex_init(&opq->mutex);
    NBThreadCond_init(&opq->cond);
    //mapping
    {
        NBArray_initWithSz(&opq->mapping.folders, sizeof(STTSStreamsStorageLoadFolderRng*), NULL, 0, 1024, 0.1f);
        NBArray_initWithSz(&opq->mapping.files, sizeof(STTSFile*), NULL, 0, 1024, 0.1f);
        //counts
        {
            TSFilesTreeCounts_init(&opq->mapping.counts.tree);
        }
    }
    //load
    {
        NBArray_initWithSz(&opq->loading.streams, sizeof(STTSStreamsStorageLoadStream*), NULL, 0, 16, 0.1f);
        //counts
        {
            TSFilesTreeCounts_init(&opq->loading.counts.tree);
        }
    }
}

void TSStreamsStorageLoader_release(STTSStreamsStorageLoader* obj){
	STTSStreamsStorageLoaderOpq* opq = (STTSStreamsStorageLoaderOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
    NBASSERT(opq->retainCount > 0)
    opq->retainCount--;
    if(opq->retainCount > 0){
        //retained
        NBThreadMutex_unlock(&opq->mutex);
    } else {
        //release
        //threads
        while(opq->threads.runningCount > 0){
            //ToDo: implement stopFlag.
            NBThreadCond_broadcast(&opq->cond);
            NBThreadCond_wait(&opq->cond, &opq->mutex);
        }
        //loading
        {
            {
                SI32 i; for(i = 0; i < opq->loading.streams.use; i++){
                    STTSStreamsStorageLoadStream* s = NBArray_itmValueAtIndex(&opq->loading.streams, STTSStreamsStorageLoadStream*, i);
                    TSStreamsStorageLoadStream_release(s);
                    NBMemory_free(s);
                }
                NBArray_empty(&opq->loading.streams);
                NBArray_release(&opq->loading.streams);
            }
            //counts
            {
                TSFilesTreeCounts_release(&opq->loading.counts.tree);
            }
        }
        //mapping
        {
            {
                SI32 i; for(i = 0; i < opq->mapping.folders.use; i++){
                    STTSStreamsStorageLoadFolderRng* f =  NBArray_itmValueAtIndex(&opq->mapping.folders, STTSStreamsStorageLoadFolderRng*, i);
                    TSStreamsStorageLoadFolderRng_release(f);
                    NBMemory_free(f);
                }
                NBArray_empty(&opq->mapping.folders);
                NBArray_release(&opq->mapping.folders);
            }
            {
                SI32 i; for(i = 0; i < opq->mapping.files.use; i++){
                    STTSFile* f =  NBArray_itmValueAtIndex(&opq->mapping.files, STTSFile*, i);
                    if(f != NULL){
                        TSFile_release(f);
                        NBMemory_free(f);
                    }
                }
                NBArray_empty(&opq->mapping.files);
                NBArray_release(&opq->mapping.files);
            }
            //counts
            {
                TSFilesTreeCounts_release(&opq->mapping.counts.tree);
            }
        }
        //params
        {
            if(NBThreadsPool_isSet(opq->params.optThreadsPool)){
                NBThreadsPool_release(&opq->params.optThreadsPool);
                NBThreadsPool_null(&opq->params.optThreadsPool);
            }
            if(opq->params.rootPath != NULL){
                NBMemory_free(opq->params.rootPath);
                opq->params.rootPath = NULL;
            }
        }
        NBThreadCond_release(&opq->cond);
        NBThreadMutex_unlock(&opq->mutex);
        NBThreadMutex_release(&opq->mutex);
        NBMemory_free(opq);
        obj->opaque = NULL;
    }
}

SI64 TSStreamsStorageLoader_NBThreadPoolRoutinePtr_(void* param); //task (will run once)
void TSStreamsStorageLoader_tick_(STTSStreamsStorageLoaderOpq* opq);
void TSStreamsStorageLoader_addTreeCountsLockedOpq_(STTSStreamsStorageLoaderOpq* opq, STTSFilesTreeCounts* dst);
void TSStreamsStorageLoader_doPostEndNotificationLockedOpq_(STTSStreamsStorageLoaderOpq* opq);

void TSStreamsStorageLoader_addTreeCountsLockedOpq_(STTSStreamsStorageLoaderOpq* opq, STTSFilesTreeCounts* dst){
    if(dst != NULL){
        //count folders
        {
            SI32 i; for(i = 0; i < opq->mapping.folders.use; i++){
                STTSStreamsStorageLoadFolderRng* f =  NBArray_itmValueAtIndex(&opq->mapping.folders, STTSStreamsStorageLoadFolderRng*, i);
                dst->folders.total++;
                if(f->subfiles.count == 0){
                    dst->folders.empty++;
                }
            }
        }
        {
            SI32 i; for(i = 0; i < opq->mapping.files.use; i++){
                STTSFile* f =  NBArray_itmValueAtIndex(&opq->mapping.files, STTSFile*, i);
                if(f == NULL){
                    dst->files.nulls++;
                } else {
                    dst->files.total++;
                    dst->files.totalBytes += f->props.bytesCount;
                    if(f->props.bytesCount == 0){
                        dst->files.empty++;
                    }
                }
            }
        }
    }
}

//If 'optInitialLoadThreads' is greather than '1',
//  the function returns inmediatly and the load is
//  done in secondary threads untill error or completion.
//else
//  the load is done in the current thread and the
//  function blocks untill error or completion.
BOOL TSStreamsStorageLoader_start(STTSStreamsStorageLoader* obj, STTSContext* context, STNBFilesystem* fs, STTSStreamsStorageStats* provider, const char* rootPath, const BOOL forceLoadAll, const char** ignoreTrashFiles, const UI32 ignoreTrashFilesSz, const BOOL* externalStopFlag, TSStreamsStorageLoaderEndedFunc endCallback, void* endCallbackUsrData, STNBThreadsPoolRef optThreadsPool, const UI32 optInitialLoadThreads){
    BOOL r = TRUE, notifyEnd = FALSE, notifyIsSucess = FALSE;
    STTSStreamsStorageLoaderOpq* opq = (STTSStreamsStorageLoaderOpq*)obj->opaque;
    NBThreadMutex_lock(&opq->mutex);
    //set params.ignoreFilenames
    if(r){
        //release
        {
            if(opq->params.ignoreFilenames != NULL){
                SI32 i; for(i = 0; i < opq->params.ignoreFilenamesSz; i++){
                    char* c = opq->params.ignoreFilenames[i];
                    NBMemory_free(c);
                }
                NBMemory_free(opq->params.ignoreFilenames);
                opq->params.ignoreFilenames = NULL;
            }
            opq->params.ignoreFilenamesSz = 0;
        }
        //copy
        if(r && ignoreTrashFiles != NULL && ignoreTrashFilesSz > 0){
            opq->params.ignoreFilenames = NBMemory_allocTypes(char*, ignoreTrashFilesSz);
            if(opq->params.ignoreFilenames == NULL){
                r = FALSE;
            } else {
                SI32 i; for(i = 0; i < ignoreTrashFilesSz; i++){
                    const char* c = ignoreTrashFiles[i];
                    if(c != NULL && c[0] != '\0'){
                        const UI32 len = NBString_strLenBytes(c);
                        char* dst = NBMemory_allocTypes(char, len + 1);
                        if(dst == NULL){
                            r = FALSE;
                            break;
                        } else {
                            NBMemory_copy(dst, c, len + 1);
                            opq->params.ignoreFilenames[opq->params.ignoreFilenamesSz] = dst;
                            opq->params.ignoreFilenamesSz++;
                        }
                    }
                }
            }
        }
    }
    //set params.optThreadsPool
    if(r){
        NBThreadsPool_set(&opq->params.optThreadsPool, &optThreadsPool);
    }
    //set params.other
    if(r){
        opq->params.forceLoadAll            = forceLoadAll;
        opq->params.externalStopFlag        = externalStopFlag;
        opq->params.optInitialLoadThreads   = optInitialLoadThreads;
        opq->params.context                 = context;
        opq->params.fs                      = fs;
        opq->params.provider                = provider;
        opq->params.endCallback             = endCallback;
        opq->params.endCallbackUsrData      = endCallbackUsrData;
        {
            if(opq->params.rootPath != NULL){
                NBMemory_free(opq->params.rootPath);
                opq->params.rootPath = NULL;
            }
            if(rootPath != NULL && rootPath[0] != '\0'){
                opq->params.rootPath = NBString_strNewBuffer(rootPath);
            }
        }
        //
        if(opq->params.fs == NULL || opq->params.rootPath == NULL){
            r = FALSE;
        }
    }
    //scan root folder
    if(r){
        NBASSERT(opq->mapping.files.use == 0)
        NBASSERT(opq->mapping.iFileNext == 0)
        NBASSERT(!opq->mapping.isCompleted)
        NBASSERT(opq->threads.waitingCount == 0)
        opq->mapping.secsAccum = 0;
        opq->timeStartLoad = opq->mapping.timeStart = opq->mapping.timeLast = NBDatetime_getCurUTCTimestamp();
        //add root file
        {
            STTSFile* f = NBMemory_allocType(STTSFile);
            TSFile_init(f);
            TSFile_setName(f, ".", ENTSFileType_Folder, 0); //"." to reference to self.
            {
                STTSStreamsStorageLoadFolderRng* fldr = NBMemory_allocType(STTSStreamsStorageLoadFolderRng);
                TSStreamsStorageLoadFolderRng_init(fldr);
                if(!TSStreamsStorageLoadFolderRng_setPath(fldr, opq->params.rootPath, 1)){ //1 = len(".")
                    //error
                    r = FALSE;
                } else if(!TSStreamsStorageLoadFolderRng_setMyNode(fldr, f)){
                    //error
                    r = FALSE;
                } else {
                    //add
                    NBArray_addValue(&opq->mapping.folders, fldr);
                    NBArray_addValue(&opq->mapping.files, f);
                    fldr = NULL; //consume
                    f = NULL; //consume
                }
                //release (if not consumed)
                if(fldr != NULL){
                    TSStreamsStorageLoadFolderRng_release(fldr);
                    NBMemory_free(fldr);
                    fldr = NULL;
                }
            }
            //release (if not consumed)
            if(f != NULL){
                TSFile_release(f);
                NBMemory_free(f);
                f = NULL;
            }
        }
    }
    //execute
    if(r){
        //create threads
        if(NBObjRef_isSet(opq->params.optThreadsPool) && opq->params.optInitialLoadThreads > 1){
            //NBThreadsPool_threadsGroupStart(opq->params.optThreadsPool, opq->params.optInitialLoadThreads);
            {
                SI32 i; for(i = 0; i < opq->params.optInitialLoadThreads; i++){
                    opq->threads.runningCount++;
                    if(!NBThreadsPool_startTask(opq->params.optThreadsPool, ENNBThreadsTaskType_Unknown, ENNBThreadsTaskPriority_Default, TSStreamsStorageLoader_NBThreadPoolRoutinePtr_, NULL, opq, NULL)){
                        //error
                        opq->threads.runningCount--;
                    } else {
                        //started
                    }
                }
            }
            //NBThreadsPool_threadsGroupEnd(opq->params.optThreadsPool, opq->params.optInitialLoadThreads);
            //trigger all threads
            NBThreadCond_broadcast(&opq->cond); //notify
        }
        //execute here (if no thread)
        if(opq->threads.runningCount == 0){
            while(!opq->isErr && !opq->isCompleted && (opq->params.externalStopFlag == NULL || !(*opq->params.externalStopFlag))){
                NBThreadMutex_unlock(&opq->mutex);
                {
                    TSStreamsStorageLoader_tick_(opq);
                }
                NBThreadMutex_lock(&opq->mutex);
            }
            notifyEnd = TRUE;
            notifyIsSucess = (!opq->isErr && opq->isCompleted);
        }
    }
    //notify (unloked)
    if(notifyEnd){
        //retain myself (in case the notification releases me)
        opq->retainCount++;
        NBThreadMutex_unlock(&opq->mutex);
        {
            if(opq->params.endCallback != NULL){
                STTSFile* rootTrash = (opq->mapping.files.use > 0 ? NBArray_itmValueAtIndex(&opq->mapping.files, STTSFile*, 0) : NULL);
                const SI32 streamsSz = opq->loading.streams.use;
                STTSStreamsStorageLoadStream** streams = NBArray_dataPtr(&opq->loading.streams, STTSStreamsStorageLoadStream*);
                (*opq->params.endCallback)(notifyIsSucess, rootTrash, streams, streamsSz, opq->params.endCallbackUsrData);
            }
        }
        //revert unconsumed updates
        NBThreadMutex_lock(&opq->mutex);
        {
            TSStreamsStorageLoader_doPostEndNotificationLockedOpq_(opq);
        }
    }
    NBThreadMutex_unlock(&opq->mutex);
    if(notifyEnd){
        //release myself (if previously retained)
        STTSStreamsStorageLoader ref;
        ref.opaque = opq;
        TSStreamsStorageLoader_release(&ref);
    }
    return r;
}

void TSStreamsStorageLoader_doPostEndNotificationLockedOpq_(STTSStreamsStorageLoaderOpq* opq){
    //move unconsumed files stats from 'loaded' files to 'others'.
    if(opq->params.provider != NULL){
        SI32 i; for(i = 0; i < opq->loading.streams.use; i++){
            const STTSStreamsStorageLoadStream* s = NBArray_itmValueAtIndex(&opq->loading.streams, STTSStreamsStorageLoadStream*, i);
            if(s != NULL){
                SI32 i2; for(i2 = 0; i2 < s->versions.use; i2++){
                    const STTSStreamsStorageLoadStreamVer* v = NBArray_itmValueAtIndex(&s->versions, STTSStreamsStorageLoadStreamVer*, i2);
                    if(v != NULL){
                        SI32 i3; for(i3 = 0; i3 < v->grps.use; i3++){
                            const STTSStreamsStorageLoadStreamVerGrp* g = NBArray_itmValueAtIndex(&v->grps, STTSStreamsStorageLoadStreamVerGrp*, i3);
                            if(g != NULL){
                                SI32 i4; for(i4 = 0; i4 < g->files.use; i4++){
                                    const STTSStreamsStorageLoadStreamVerGrpFiles* p = NBArray_itmValueAtIndex(&g->files, STTSStreamsStorageLoadStreamVerGrpFiles*, i4);
                                    if(p != NULL && p->payload != NULL){
                                        //unconsumed files, move loaded files to 'others'
                                        STTSStreamsStorageStatsUpd upd;
                                        NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
                                        //unconsumed previously counted payload, indexes and gaps files are moved to 'others'.
                                        TSStreamStorageFiles_getStatsUpdAsOthers(p->payload, &upd);
                                        //apply
                                        TSStreamsStorageStats_applyUpdate(opq->params.provider, &upd);
                                        //
                                        PRINTF_WARNING("TSStreamsStorageLoader, files were loaded but not consumed by the storage: '%s'.\n", g->myFldr->path);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    //print stats
    if(opq->params.provider != NULL){
        STNBString str;
        NBString_init(&str);
        {
            BOOL loaded = TRUE, accum = FALSE, total = FALSE, ignoreEmpties = TRUE, resetAccum = FALSE;
            STTSCfgTelemetryFilesystem cfg;
            NBMemory_setZeroSt(cfg, STTSCfgTelemetryFilesystem);
            cfg.statsLevel = ENNBLogLevel_Info;
            TSStreamsStorageStats_concat(opq->params.provider, &cfg, loaded, accum, total, "", "", ignoreEmpties, &str, resetAccum);
            PRINTF_CONSOLE("TSStreamsStorageLoader, ---------------------------------------\n");
            PRINTF_CONSOLE("TSStreamsStorageLoader, storage-stats after load (%d secs):\n%s\n", (SI32)(NBDatetime_getCurUTCTimestamp() - opq->mapping.timeStart), str.str);
            PRINTF_CONSOLE("TSStreamsStorageLoader, ---------------------------------------\n");
        }
        NBString_release(&str);
    }
}

void TSStreamsStorageLoader_appendSubfilesToFlatArrayOpq_(STTSStreamsStorageLoaderOpq* opq, STTSStreamsStorageLoadFolderRng* fldr, STTSFile* dst){
    //Header
    SI32 filesCount = 0, foldersCount = 0;
    STNBArray subPtrs; //STTSFile*
    STNBArray subFiles;
    STNBString strTmp, subStrs;
    NBArray_initWithSz(&subPtrs, sizeof(STTSFile*), NULL, 64, 512, 0.1f);
    NBArray_initWithSz(&subFiles, sizeof(STNBFilesystemFile), NULL, 64, 512, 0.1f);
    NBString_init(&strTmp);
    NBString_initWithSz(&subStrs, 0, 1024, 0.10f);
    //Files
    if(!NBFilesystem_getFiles(opq->params.fs, fldr->path, FALSE, &subStrs, &subFiles)){
        //PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, NBFilesystem_getFolders failed for '%s'.\n", opq->params.rootPath);
    } else {
        //add files
        SI32 i; for(i = 0; i < subFiles.use && !opq->isErr && (opq->params.externalStopFlag == NULL || !(*opq->params.externalStopFlag)); i++){
            STNBFilesystemFile* f = NBArray_itmPtrAtIndex(&subFiles, STNBFilesystemFile, i);
            if(!f->isSymLink){
                const char* name = &subStrs.str[f->name];
                NBString_set(&strTmp, fldr->path);
                if(strTmp.length > 0){
                    if(strTmp.str[strTmp.length - 1] != '/' && strTmp.str[strTmp.length - 1] != '\\'){
                        NBString_concatByte(&strTmp, '/');
                    }
                }
                NBString_concat(&strTmp, name);
                {
                    const SI64 fileSz = NBFile_getSize(strTmp.str);
                    STTSFile* sub = NBMemory_allocType(STTSFile);
                    TSFile_init(sub);
                    TSFile_setName(sub, name, ENTSFileType_File, fileSz);
                    NBArray_addValue(&subPtrs, sub);
                    filesCount++;
                }
            }
        }
    }
    //Subfolders
    {
        NBArray_empty(&subFiles);
        NBString_empty(&subStrs);
        if(!NBFilesystem_getFolders(opq->params.fs, fldr->path, &subStrs, &subFiles)){
            //PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, NBFilesystem_getFolders failed for '%s'.\n", opq->params.rootPath);
        } else {
            //add folders
            SI32 i; for(i = 0; i < subFiles.use && !opq->isErr && (opq->params.externalStopFlag == NULL || !(*opq->params.externalStopFlag)); i++){
                STNBFilesystemFile* f = NBArray_itmPtrAtIndex(&subFiles, STNBFilesystemFile, i);
                if(!f->isSymLink){
                    const char* name = &subStrs.str[f->name];
                    NBString_set(&strTmp, fldr->path);
                    if(strTmp.length > 0){
                        if(strTmp.str[strTmp.length - 1] != '/' && strTmp.str[strTmp.length - 1] != '\\'){
                            NBString_concatByte(&strTmp, '/');
                        }
                    }
                    NBString_concat(&strTmp, name);
                    {
                        STTSFile* sub = NBMemory_allocType(STTSFile);
                        TSFile_init(sub);
                        TSFile_setName(sub, name, ENTSFileType_Folder, 0);
                        NBArray_addValue(&subPtrs, sub);
                        foldersCount++;
                    }
                }
            }
        }
    }
    //append all found (locked)
    if(subPtrs.use > 0){
        NBThreadMutex_lock(&opq->mutex);
        {
            //add folders
            {
                SI32 foldersAdded = 0;
                SI32 i; for(i = 0; i < subPtrs.use; i++){
                    STTSFile* f = NBArray_itmValueAtIndex(&subPtrs, STTSFile*, i);
                    if(f->props.type == ENTSFileType_Folder){
                        STTSStreamsStorageLoadFolderRng* fldrN = NBMemory_allocType(STTSStreamsStorageLoadFolderRng);
                        TSStreamsStorageLoadFolderRng_init(fldrN);
                        {
                            NBString_set(&strTmp, fldr->path);
                            if(strTmp.length > 0){
                                if(strTmp.str[strTmp.length - 1] != '/' && strTmp.str[strTmp.length - 1] != '\\'){
                                    NBString_concatByte(&strTmp, '/');
                                }
                            }
                            NBString_concat(&strTmp, f->name);
                            if(!TSStreamsStorageLoadFolderRng_setPath(fldrN, strTmp.str, NBString_strLenBytes(f->name))){
                                //flag as error
                                opq->isErr = TRUE;
                            } else if(!TSStreamsStorageLoadFolderRng_setMyNode(fldrN, f)){
                                //flag as error
                                opq->isErr = TRUE;
                            } else if(!TSStreamsStorageLoadFolderRng_setParent(fldrN, dst)){
                                //flag as error
                                opq->isErr = TRUE;
                            } else {
                                //PRINTF_INFO("TSStreamsStorageLoader, added subfolder, parent(%lld) myNode(%lld)...\n", (SI64)fldrN->parent, (SI64)fldrN->myNode);
                                NBArray_addValue(&opq->mapping.folders, fldrN);
                                fldrN = NULL; //consume
                                foldersAdded++;
                            }
                        }
                        //release (if not consumed)
                        if(fldrN != NULL){
                            TSStreamsStorageLoadFolderRng_release(fldrN);
                            NBMemory_free(fldrN);
                            fldrN = NULL;
                        }
                    } else {
                        //PRINTF_INFO("TSStreamsStorageLoader, added subfile: %s.\n", f->name);
                    }
                }
                NBASSERT(opq->isErr || foldersAdded == foldersCount)
            }
            //add files and folders
            {
                const SI32 iStart = opq->mapping.files.use;
                //move files from stack to array
                {
                    SI32 i; for(i = 0; i < subPtrs.use; i++){
                        STTSFile* f = NBArray_itmValueAtIndex(&subPtrs, STTSFile*, i);
                        NBArray_addValue(&opq->mapping.files, f);
                    }
                }
                //update parent folder
                if(!TSStreamsStorageLoadFolderRng_setSubfilesRng(fldr, iStart, subPtrs.use)){
                    //flag as error
                    opq->isErr = TRUE;
                } else {
                    NBASSERT(fldr->subfiles.iStart >= 0 && fldr->subfiles.iStart < opq->mapping.files.use && (fldr->subfiles.iStart + fldr->subfiles.count) <= opq->mapping.files.use) //must be in the files range
                    //PRINTF_INFO("TSStreamsStorageLoader, added folder %d subfiles, %d subfolders: '%s'.\n", filesCount, foldersCount, fldr->path);
                }
                //
                NBArray_empty(&subPtrs);
            }
        }
        NBThreadMutex_unlock(&opq->mutex);
    }
    //
    if(opq->params.externalStopFlag != NULL && *opq->params.externalStopFlag){
        //flag as error
        opq->isErr = TRUE;
    }
    NBArray_release(&subPtrs);
    NBArray_release(&subFiles);
    NBString_release(&strTmp);
    NBString_release(&subStrs);
}

STTSStreamsStorageLoadFolderRng* TSStreamsStorageLoader_getFolderRngBy_(STTSStreamsStorageLoaderOpq* opq, const STTSFile* folderNode);
//
void TSStreamsStorageLoader_tickMappingNextLocked_(STTSStreamsStorageLoaderOpq* opq);
void TSStreamsStorageLoader_tickMappingWaitOrCompleteLockedOpq_(STTSStreamsStorageLoaderOpq* opq);
//
void TSStreamsStorageLoader_tickLoadingNextLockedOpq_(STTSStreamsStorageLoaderOpq* opq);

void TSStreamsStorageLoader_tick_(STTSStreamsStorageLoaderOpq* opq){
    //mapping
    NBThreadMutex_lock(&opq->mutex);
    {
        if(!opq->mapping.isCompleted){
            NBASSERT(opq->mapping.iFileNext <= opq->mapping.files.use)
            NBASSERT(opq->mapping.files.use > 0) //at least root file should be in array
            if(opq->mapping.iFileNext < opq->mapping.files.use){
                //something pending to work
                TSStreamsStorageLoader_tickMappingNextLocked_(opq);
            } else {
                //mapping grows in parallel, reactivate if something new was added
                //end mapping if all threads entered the waiting-state.
                TSStreamsStorageLoader_tickMappingWaitOrCompleteLockedOpq_(opq);
            }
        } else if(!opq->loading.isCompleted){
            //loading doesnt grow in parallel
            //end loading if all threads entered the waiting-state.
            TSStreamsStorageLoader_tickLoadingNextLockedOpq_(opq);
        }
    }
    NBASSERT((opq->threads.runningCount == 0 && opq->threads.waitingCount == 1) || opq->threads.waitingCount <= opq->threads.runningCount)
    NBThreadMutex_unlock(&opq->mutex);
}

void TSStreamsStorageLoader_tickMappingNextLocked_(STTSStreamsStorageLoaderOpq* opq){
    //continue mapping
    STTSFile* f = NULL;
    do {
        f = NBArray_itmValueAtIndex(&opq->mapping.files, STTSFile*, opq->mapping.iFileNext);
        if(f->props.type != ENTSFileType_Folder){
            opq->mapping.iFileNext++; //move to next
            NBThreadCond_broadcast(&opq->cond); //notify
        } else {
            STTSStreamsStorageLoadFolderRng* fldr = TSStreamsStorageLoader_getFolderRngBy_(opq, f);
            NBASSERT(fldr != NULL)
            if(fldr == NULL){
                //error
                opq->isErr = TRUE;
                NBThreadCond_broadcast(&opq->cond); //notify
            } else {
                //process folder
                //PRINTF_INFO("TSStreamsStorageLoader, next #%d/%d (processing folder).\n", opq->mapping.iFileNext + 1, opq->mapping.files.use);
                opq->mapping.iFileNext++; //move to next
                NBThreadCond_broadcast(&opq->cond); //notify
                NBThreadMutex_unlock(&opq->mutex);
                {
                    TSStreamsStorageLoader_appendSubfilesToFlatArrayOpq_(opq, fldr, f);
                }
                NBThreadMutex_lock(&opq->mutex);
            }
        }
    } while(opq->mapping.iFileNext < opq->mapping.files.use && f->props.type != ENTSFileType_Folder);
    //progress (locked)
    {
        UI64 timeCur = NBDatetime_getCurUTCTimestamp();
        if(timeCur != opq->mapping.timeLast){
            opq->mapping.secsAccum  += (timeCur - opq->mapping.timeLast);
            opq->mapping.timeLast   = timeCur;
            if(opq->mapping.secsAccum >= 10){ //print every XX seconds
                PRINTF_CONSOLE("TSStreamsStorageLoader, mapping %d secs: %d folders, %d files found.\n", (timeCur - opq->mapping.timeStart), opq->mapping.folders.use, opq->mapping.files.use);
                opq->mapping.secsAccum = 0;
            }
        }
    }
}

void TSStreamsStorageLoader_moveFilesFromFlatArrayToTreeRecursiveLockedOpq_(STTSStreamsStorageLoaderOpq* opq, STTSStreamsStorageLoadFolderRng* fldr){
    STTSFile** f = NBArray_itmPtrAtIndex(&opq->mapping.files, STTSFile*, fldr->subfiles.iStart);
    STTSFile** fAfterEnd = f + fldr->subfiles.count;
    while(f < fAfterEnd && !opq->isErr && (opq->params.externalStopFlag == NULL || !(*opq->params.externalStopFlag))){
        if(*f != NULL){
            if((*f)->props.type != ENTSFileType_Folder){
                //just add
                TSFile_addSubfileObj(fldr->myNode, *f);
            } else {
                //recursive call, then add
                STTSStreamsStorageLoadFolderRng* fldr2 = NULL;
                //search file's folder
                {
                    SI32 i; for(i = 0; i < opq->mapping.folders.use; i++){
                        STTSStreamsStorageLoadFolderRng* fldr3 =  NBArray_itmValueAtIndex(&opq->mapping.folders, STTSStreamsStorageLoadFolderRng*, i);
                        if(fldr3->myNode == *f){
                            fldr2 = fldr3;
                            break;
                        }
                    }
                }
                NBASSERT(fldr2 != NULL)
                if(fldr2 == NULL){
                    //error
                    opq->isErr = TRUE;
                    NBThreadCond_broadcast(&opq->cond); //notify
                } else {
                    TSStreamsStorageLoader_moveFilesFromFlatArrayToTreeRecursiveLockedOpq_(opq, fldr2);
                    //add after all children were added
                    TSFile_addSubfileObj(fldr->myNode, *f);
                }
            }
        }
        //next
        f++;
    }
}


//

void TSStreamsStorageLoader_tickMappingWaitOrCompleteLockedOpq_(STTSStreamsStorageLoaderOpq* opq){
    opq->threads.waitingCount++;
    NBASSERT((opq->threads.runningCount == 0 && opq->threads.waitingCount == 1) || opq->threads.waitingCount <= opq->threads.runningCount)
    if(opq->threads.waitingCount < opq->threads.runningCount){
        //wait for signal
        NBThreadCond_broadcast(&opq->cond); //notify
        NBThreadCond_wait(&opq->cond, &opq->mutex);
        NBASSERT(opq->threads.waitingCount > 0)
        if(opq->threads.waitingCount > 0){
            opq->threads.waitingCount--;
        }
    } else {
        //finish mapping
        opq->mapping.isCompleted = TRUE;
        NBASSERT(opq->threads.waitingCount > 0)
        if(opq->threads.waitingCount > 0){
            opq->threads.waitingCount--;
        }
        //build files-tree from flat arrays
        if(!opq->isErr && (opq->params.externalStopFlag == NULL || !(*opq->params.externalStopFlag))){
            opq->loading.secsAccum = 0;
            opq->loading.timeStart = opq->loading.timeLast = NBDatetime_getCurUTCTimestamp();
            //
            TSFilesTreeCounts_reset(&opq->mapping.counts.tree);
            TSStreamsStorageLoader_addTreeCountsLockedOpq_(opq, &opq->mapping.counts.tree);
            PRINTF_CONSOLE("TSStreamsStorageLoader, root-folder-mapping %d:%d secs completed: %d folders (%d empty), %d files (%d empty, %d MBs); building tree ...\n", (SI32)(NBDatetime_getCurUTCTimestamp() - opq->mapping.timeStart) / 60, (SI32)(NBDatetime_getCurUTCTimestamp() - opq->mapping.timeStart) % 60, opq->mapping.counts.tree.folders.total, opq->mapping.counts.tree.folders.empty, opq->mapping.counts.tree.files.total, opq->mapping.counts.tree.files.empty, ((opq->mapping.counts.tree.files.totalBytes + (1024 * 1024) - 1) / (1024 * 1024)));
        }
        //create streams tree
        if(!opq->isErr && (opq->params.externalStopFlag == NULL || !(*opq->params.externalStopFlag))){
            if(opq->mapping.folders.use > 0){
                STTSStreamsStorageLoadFolderRng* fldr = NBArray_itmValueAtIndex(&opq->mapping.folders, STTSStreamsStorageLoadFolderRng*, 0);
                SI32 i; for(i = 0; i < fldr->subfiles.count && !opq->isErr && (opq->params.externalStopFlag == NULL || !(*opq->params.externalStopFlag)); i++){
                    STTSFile* f = NBArray_itmValueAtIndex(&opq->mapping.files, STTSFile*, fldr->subfiles.iStart + i);
                    if(f->props.type == ENTSFileType_Folder){
                        STTSStreamsStorageLoadFolderRng* fldr2 = TSStreamsStorageLoader_getFolderRngBy_(opq, f);
                        NBASSERT(fldr2 != NULL)
                        if(fldr2 == NULL){
                            //error
                            opq->isErr = TRUE;
                        } else {
                            STTSStreamsStorageLoadStream* ss = NBMemory_allocType(STTSStreamsStorageLoadStream);
                            TSStreamsStorageLoadStream_init(ss);
                            ss->myFldr = fldr2;
                            NBArray_addValue(&opq->loading.streams, ss);
                            //add versions
                            {
                                STTSStreamsStorageLoadFolderRng* fldr = ss->myFldr;
                                SI32 i; for(i = 0; i < fldr->subfiles.count && !opq->isErr && (opq->params.externalStopFlag == NULL || !(*opq->params.externalStopFlag)); i++){
                                    STTSFile* f = NBArray_itmValueAtIndex(&opq->mapping.files, STTSFile*, fldr->subfiles.iStart + i);
                                    if(f->props.type == ENTSFileType_Folder){
                                        STTSStreamsStorageLoadFolderRng* fldr2 = TSStreamsStorageLoader_getFolderRngBy_(opq, f);
                                        NBASSERT(fldr2 != NULL)
                                        if(fldr2 == NULL){
                                            //error
                                            opq->isErr = TRUE;
                                        } else {
                                            STTSStreamsStorageLoadStreamVer* vv = NBMemory_allocType(STTSStreamsStorageLoadStreamVer);
                                            TSStreamsStorageLoadStreamVer_init(vv);
                                            vv->parent  = ss;
                                            vv->myFldr  = fldr2;
                                            NBArray_addValue(&ss->versions, vv);
                                            opq->loading.counts.versions++;
                                            //add grps
                                            {
                                                STTSStreamsStorageLoadFolderRng* fldr = vv->myFldr;
                                                SI32 i; for(i = 0; i < fldr->subfiles.count && !opq->isErr && (opq->params.externalStopFlag == NULL || !(*opq->params.externalStopFlag)); i++){
                                                    STTSFile* f = NBArray_itmValueAtIndex(&opq->mapping.files, STTSFile*, fldr->subfiles.iStart + i);
                                                    if(f->props.type == ENTSFileType_Folder){
                                                        STTSStreamsStorageLoadFolderRng* fldr2 = TSStreamsStorageLoader_getFolderRngBy_(opq, f);
                                                        NBASSERT(fldr2 != NULL)
                                                        if(fldr2 == NULL){
                                                            //error
                                                            opq->isErr = TRUE;
                                                        } else {
                                                            STTSStreamsStorageLoadStreamVerGrp* gg = NBMemory_allocType(STTSStreamsStorageLoadStreamVerGrp);
                                                            TSStreamsStorageLoadStreamVerGrp_init(gg);
                                                            gg->parent  = vv;
                                                            gg->myFldr  = fldr2;
                                                            NBArray_addValue(&vv->grps, gg);
                                                            opq->loading.counts.groups++;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        //end-of-tree-build
        if(!opq->isErr && (opq->params.externalStopFlag == NULL || !(*opq->params.externalStopFlag))){
            PRINTF_CONSOLE("TSStreamsStorageLoader, tree-built %d secs: %d streams, %d versions, %d grps; loading ...\n", (SI32)(NBDatetime_getCurUTCTimestamp() - opq->loading.timeStart), opq->loading.streams.use, opq->loading.counts.versions, opq->loading.counts.groups);
        }
    }
}
    
void TSStreamsStorageLoader_tickLoadingNextLockedOpq_(STTSStreamsStorageLoaderOpq* opq){
    if(opq->loading.iStreamNext >= opq->loading.streams.use){
        //nothing to load
        opq->threads.waitingCount++;
        NBASSERT((opq->threads.runningCount == 0 && opq->threads.waitingCount == 1) || opq->threads.waitingCount <= opq->threads.runningCount)
        if(opq->threads.waitingCount < opq->threads.runningCount){
            //wait for signal
            NBThreadCond_broadcast(&opq->cond); //notify
            NBThreadCond_wait(&opq->cond, &opq->mutex);
            NBASSERT(opq->threads.waitingCount > 0)
            if(opq->threads.waitingCount > 0){
                opq->threads.waitingCount--;
            }
        } else {
            STTSStreamsStorageStatsUpd upd;
            NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
            //this is the last thread to add to 'waitingCount'.
            //finish loading
            opq->isCompleted = opq->loading.isCompleted = TRUE;
            NBASSERT(opq->threads.waitingCount > 0)
            if(opq->threads.waitingCount > 0){
                opq->threads.waitingCount--;
            }
            //remove 'ignoreFilenames' from remainig files
            {
                SI32 i; for(i = 0; i < opq->params.ignoreFilenamesSz; i++){
                    const char* c = opq->params.ignoreFilenames[i];
                    SI32 i2; for(i2 = 0; i2 < opq->mapping.files.use; i2++){
                        STTSFile** f = NBArray_itmPtrAtIndex(&opq->mapping.files, STTSFile*, i2);
                        if(*f != NULL && NBString_strIsEqual(c, (*f)->name)){
                            if((*f)->props.type != ENTSFileType_Folder){
                                upd.files.others.added.count++;
                                upd.files.others.added.bytesCount += (*f)->props.bytesCount;
                            }
                            TSFile_release(*f);
                            NBMemory_free(*f);
                            *f = NULL;
                        }
                    }
                }
            }
            //count remaining files as trash
            {
                SI32 i2; for(i2 = 0; i2 < opq->mapping.files.use; i2++){
                    STTSFile** f = NBArray_itmPtrAtIndex(&opq->mapping.files, STTSFile*, i2);
                    if(*f != NULL && (*f)->props.type != ENTSFileType_Folder){
                        upd.files.trash.added.count++;
                        upd.files.trash.added.bytesCount += (*f)->props.bytesCount;
                    }
                }
            }
            //print remaining files
            IF_NBASSERT(
            {
                SI32 i, iFnd = 0; for(i = 0; i < opq->mapping.folders.use; i++){
                    STTSStreamsStorageLoadFolderRng* f =  NBArray_itmValueAtIndex(&opq->mapping.folders, STTSStreamsStorageLoadFolderRng*, i);
                    SI32 i2; for(i2 = 0; i2 < f->subfiles.count; i2++){
                        STTSFile* ff =  NBArray_itmValueAtIndex(&opq->mapping.files, STTSFile*, f->subfiles.iStart + i2);
                        if(ff != NULL && ff->props.type != ENTSFileType_Folder && ff->props.bytesCount > 0){
                            iFnd++;
                            if(ff->props.bytesCount > (1024 * 1024)){
                                PRINTF_INFO("TSStreamsStorageLoader, file as trash #%d: '%s' (%d MBs).\n", iFnd, ff->name, (ff->props.bytesCount / (1024 * 1024)));
                            } else if(ff->props.bytesCount > (1024)){
                                PRINTF_INFO("TSStreamsStorageLoader, file as trash #%d: '%s' (%d KBs).\n", iFnd, ff->name, (ff->props.bytesCount / 1024));
                            } else {
                                PRINTF_INFO("TSStreamsStorageLoader, file as trash #%d: '%s' (%d bytes).\n", iFnd, ff->name, ff->props.bytesCount);
                            }
                        }
                    }
                }
            }
            ) //IF_NBASSERT
            //final counts
            {
                TSFilesTreeCounts_reset(&opq->loading.counts.tree);
                TSStreamsStorageLoader_addTreeCountsLockedOpq_(opq, &opq->loading.counts.tree);
            }
            //build tree from flat arrays (final trash files tree)
            if(opq->mapping.folders.use > 0 && opq->mapping.files.use > 0){
                STTSStreamsStorageLoadFolderRng* fldr =  NBArray_itmValueAtIndex(&opq->mapping.folders, STTSStreamsStorageLoadFolderRng*, 0);
                TSStreamsStorageLoader_moveFilesFromFlatArrayToTreeRecursiveLockedOpq_(opq, fldr);
                //release files pointers (linear arrays indexes are invalid from this point on)
                {
                    //after this point, all TSFile* objects internal payload is owned by the root-node (the first).
                    SI32 i; for(i = 1; i < opq->mapping.files.use; i++){
                        STTSFile** f =  NBArray_itmPtrAtIndex(&opq->mapping.files, STTSFile*, i);
                        if(*f != NULL){
                            //TSFile_release(*f); //Do not release.
                            NBMemory_free(*f);
                            *f = NULL;
                        }
                    }
                    //Only the first (root) node' internal data belongs to me.
                    //When the tree was built, the nodes references were copied.
                    NBArray_truncateBuffSize(&opq->mapping.files, 1);
                }
                //unset linear arrays indexes (are invalid from this point on)
                //keep folders objects, still referenced by the load-results
                {
                    SI32 i; for(i = 0; i < opq->mapping.folders.use; i++){
                        STTSStreamsStorageLoadFolderRng* f =  NBArray_itmValueAtIndex(&opq->mapping.folders, STTSStreamsStorageLoadFolderRng*, i);
                        f->parent = f->myNode = NULL;
                        f->subfiles.iStart = f->subfiles.count = 0;
                    }
                    NBArray_empty(&opq->mapping.folders);
                }
            }
            //
            IF_NBASSERT(
                if(opq->mapping.files.use > 0){
                    const BOOL ignoreEmptyFolders = TRUE;
                    const BOOL ignoreEmptyFiles = TRUE;
                    STTSFile* root =  NBArray_itmValueAtIndex(&opq->mapping.files, STTSFile*, 0);
                    STNBString tmp;
                    NBString_initWithSz(&tmp, 0, 1024 * 64, 0.1f);
                    TSFile_concatTreeRecursive(root, ignoreEmptyFolders, ignoreEmptyFiles, &tmp);
                    PRINTF_CONSOLE("TSStreamsStorageLoader, non-empties-trash-tree: -->\n%s\n<--\n", tmp.str);
                    NBString_release(&tmp);
                }
            ) //IF_NBASSERT
            PRINTF_CONSOLE("TSStreamsStorageLoader, %d of %d (%d of %d MBs) non-empty files remain as trash, +%d empty files.\n"
                           , (opq->loading.counts.tree.files.total - opq->loading.counts.tree.files.empty), (opq->mapping.counts.tree.files.total - opq->mapping.counts.tree.files.empty)
                           , ((opq->loading.counts.tree.files.totalBytes + (1024 * 1024) - 1) / (1024 * 1024)), ((opq->mapping.counts.tree.files.totalBytes + (1024 * 1024) - 1) / (1024 * 1024))
                           , opq->loading.counts.tree.files.empty
                           );
            PRINTF_CONSOLE("TSStreamsStorageLoader, %d folders (%d empty).\n", opq->loading.counts.tree.folders.total, opq->loading.counts.tree.folders.empty);
            PRINTF_CONSOLE("TSStreamsStorageLoader, %d:%d secs to complete storage load.\n", (SI32)(NBDatetime_getCurUTCTimestamp() - opq->mapping.timeStart) / 60, (SI32)(NBDatetime_getCurUTCTimestamp() - opq->mapping.timeStart) % 60);
            //
            if(opq->params.provider != NULL){
                TSStreamsStorageStats_applyUpdate(opq->params.provider, &upd);
            }
        }
    } else {
        //load next
        STTSStreamsStorageLoadStream* ss = NBArray_itmValueAtIndex(&opq->loading.streams, STTSStreamsStorageLoadStream*, opq->loading.iStreamNext);
        //progress (locked)
        {
            UI64 timeCur = NBDatetime_getCurUTCTimestamp();
            if(timeCur != opq->loading.timeLast){
                opq->loading.secsAccum  += (timeCur - opq->loading.timeLast);
                opq->loading.timeLast   = timeCur;
                if(opq->loading.secsAccum >= 10){ //print every XX seconds
                    PRINTF_CONSOLE("TSStreamsStorageLoader, loading %d:%d secs (%d%%, %d of %d groups).\n", (SI32)(timeCur - opq->loading.timeStart) / 60, (SI32)(timeCur - opq->loading.timeStart) % 60, (opq->loading.counts.groups <= 0 ? 0 : 100 * opq->loaded.counts.groups / opq->loading.counts.groups), opq->loaded.counts.groups, opq->loading.counts.groups);
                    opq->loading.secsAccum = 0;
                }
            }
        }
        if(ss->iNext >= ss->versions.use){
            //move to next stream
            opq->loading.iStreamNext++;
            NBThreadCond_broadcast(&opq->cond); //notify
        } else {
            STTSStreamsStorageLoadStreamVer* vv = NBArray_itmValueAtIndex(&ss->versions, STTSStreamsStorageLoadStreamVer*, ss->iNext);
            if(vv->iNext >= vv->grps.use){
                //move to next stream-ver
                ss->iNext++;
                NBThreadCond_broadcast(&opq->cond); //notify
            } else {
                STTSStreamsStorageLoadStreamVerGrp* gg = NBArray_itmValueAtIndex(&vv->grps, STTSStreamsStorageLoadStreamVerGrp*, vv->iNext);
                if(gg->iNext >= gg->myFldr->subfiles.count){
                    //move to next stream-ver-grp
                    vv->iNext++; opq->loaded.counts.groups++;
                    NBThreadCond_broadcast(&opq->cond); //notify
                } else {
                    if(gg->head == NULL){
                        //try to load header (keep the lock)
                        const char* prefix      = "";
                        const UI32 prefixLen    = NBString_strLenBytes(prefix);
                        const char* name        = gg->myFldr->myNode->name;
                        const UI32 nameLen      = NBString_strLenBytes(name);
                        if(nameLen > prefixLen && NBString_strStartsWith(name, prefix)){
                            const char* seqStart = &name[prefixLen];
                            const char* seqAfterEnd = seqStart;
                            while(*seqAfterEnd >= '0' && *seqAfterEnd <= '9'){
                                seqAfterEnd++;
                            }
                            //Add seq
                            if(seqStart < seqAfterEnd){
                                //load header
                                STNBArray filenamesUsed;
                                NBArray_initWithSz(&filenamesUsed, sizeof(STNBString), NULL, 0, 8, 01.f);
                                {
                                    STTSStreamStorageFilesGrpHeader* head = NBMemory_allocType(STTSStreamStorageFilesGrpHeader);
                                    NBMemory_setZeroSt(*head, STTSStreamStorageFilesGrpHeader);
                                    NBASSERT(NBString_strIsIntegerBytes(seqStart, (UI32)(seqAfterEnd - seqStart)))
                                    //
                                    gg->seq = NBString_strToUI32Bytes(seqStart, (UI32)(seqAfterEnd - seqStart));
                                    //
                                    if(!TSStreamsStorageLoader_loadStreamVerGrpHeader(gg->myFldr->path, head, &filenamesUsed)){
                                        //not a stream folder?
                                    } else if(head->runId == NULL){
                                        NBStruct_stRelease(TSStreamStorageFilesGrpHeader_getSharedStructMap(), &head, sizeof(head));
                                        NBASSERT(FALSE) //debugging
                                    } else {
                                        //consume
                                        gg->head = head; head = NULL; //consume
                                    }
                                    //release (if not consumed)
                                    if(head != NULL){
                                        NBStruct_stRelease(TSStreamStorageFilesGrpHeader_getSharedStructMap(), head, sizeof(*head));
                                        NBMemory_free(head);
                                        head = NULL;
                                    }
                                }
                                //remove used files from the flat-array
                                {
                                    SI32 i; for(i = 0; i < filenamesUsed.use; i++){
                                        STNBString* str = NBArray_itmPtrAtIndex(&filenamesUsed, STNBString, i);
                                        SI32 i2; for(i2 = 0; i2 < gg->myFldr->subfiles.count; i2++){
                                            STTSFile** f = NBArray_itmPtrAtIndex(&opq->mapping.files, STTSFile*, gg->myFldr->subfiles.iStart + i2);
                                            if(*f != NULL && NBString_isEqual(str, (*f)->name)){
                                                TSFile_release(*f);
                                                NBMemory_free(*f);
                                                *f = NULL;
                                                break;
                                            }
                                        } NBASSERT(i2 < gg->myFldr->subfiles.count) //should be found
                                        NBString_release(str);
                                    }
                                    NBArray_empty(&filenamesUsed);
                                    NBArray_release(&filenamesUsed);
                                }
                            }
                        }
                        if(gg->head == NULL){
                            //header coudl not be loaded
                            //move to next stream-ver-grp
                            vv->iNext++;
                            opq->loaded.counts.groups++;
                            NBThreadCond_broadcast(&opq->cond); //notify
                            if(gg->seq > 0){
                                //PRINTF_WARNING("TSStreamsStorageLoader, folder with valid-name but without-header: '%s'.\n", gg->myFldr->myNode->name);
                            } else {
                                //PRINTF_WARNING("TSStreamsStorageLoader, folder without valid-name or header: '%s'.\n", gg->myFldr->myNode->name);
                            }
                        }
                    } else {
                        STTSFile* f = NBArray_itmValueAtIndex(&opq->mapping.files, STTSFile*, gg->myFldr->subfiles.iStart + gg->iNext);
                        gg->iNext++;
                        NBThreadCond_broadcast(&opq->cond); //notify
                        if(f != NULL){
                            const char* prefix      = "avp-";
                            const UI32 prefixLen    = NBString_strLenBytes(prefix);
                            const char* name        = f->name;
                            const UI32 nameLen      = NBString_strLenBytes(name);
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
                                        STTSStreamsStorageLoadStreamVerGrpFiles* ff = NULL; //to-add-after-locking
                                        STNBArray filenamesUsed;
                                        STNBString pathNoExt;
                                        NBASSERT(NBString_strIsIntegerBytes(seqStart, (UI32)(seqAfterEnd - seqStart)))
                                        NBASSERT(*extChar == '.')
                                        NBArray_initWithSz(&filenamesUsed, sizeof(STNBString), NULL, 0, 8, 01.f);
                                        NBString_init(&pathNoExt);
                                        NBString_concat(&pathNoExt, gg->myFldr->path);
                                        if(pathNoExt.length > 0){
                                            if(pathNoExt.str[pathNoExt.length - 1] != '/' && pathNoExt.str[pathNoExt.length - 1] != '\\'){
                                                NBString_concatByte(&pathNoExt, '/');
                                            }
                                        }
                                        NBString_concatBytes(&pathNoExt, f->name, (UI32)(extChar - name)); //4 from '.264'
                                        //load file (unlocked)
                                        NBThreadMutex_unlock(&opq->mutex);
                                        {
                                            STTSStreamStorageFiles* pay = NBMemory_allocType(STTSStreamStorageFiles);
                                            TSStreamStorageFiles_init(pay);
                                            if(!TSStreamStorageFiles_setFilesystem(pay, opq->params.fs)){
                                                //error
                                            } else if(!TSStreamStorageFiles_setRunId(pay, opq->params.context, gg->head->runId)){
                                                //error
                                            } else if(!TSStreamStorageFiles_setFilepathsWithoutExtension(pay, gg->seq, pathNoExt.str)){
                                                //error
                                            } else {
                                                BOOL isPayloadEmpty = FALSE;
                                                STNBString errStr;
                                                STTSStreamsStorageStatsUpd upd;
                                                NBString_init(&errStr);
                                                NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
                                                if(!TSStreamStorageFiles_loadFiles(pay, gg->head->runId, opq->params.forceLoadAll, &isPayloadEmpty, &upd, &filenamesUsed, &errStr)){
                                                    if(!isPayloadEmpty){
                                                        //r.nonEmptyCount++;
                                                        //PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, TSStreamStorageFiles_loadFiles failed for: '%s' / '%s': '%s'.\n", opq->cfg.uid.str, itm->filename, errStr.str);
                                                        PRINTF_CONSOLE_ERROR("TSStreamsStorageLoader, TSStreamStorageFiles_loadFiles failed for: '%s' ('%s').\n", pathNoExt.str, errStr.str);
                                                    }
                                                } else {
                                                    ff = NBMemory_allocType(STTSStreamsStorageLoadStreamVerGrpFiles);
                                                    TSStreamsStorageLoadStreamVerGrpFiles_init(ff);
                                                    ff->parent  = gg;
                                                    ff->seq     = (UI32)NBString_strToSI32Bytes(seqStart, (UI32)(seqAfterEnd - seqStart));
                                                    ff->upd     = upd;
                                                    ff->payload = pay; pay = NULL; //consume
                                                    //PRINTF_INFO("TSStreamsStorageLoader, TSStreamStorageFiles_loadFiles success for: '%s'.\n", pathNoExt.str);
                                                    if(opq->params.provider != NULL){
                                                        TSStreamsStorageStats_applyUpdate(opq->params.provider, &upd);
                                                    }
                                                }
                                                NBString_release(&errStr);
                                            }
                                            //release (if not consumed)
                                            if(pay != NULL){
                                                TSStreamStorageFiles_release(pay);
                                                NBMemory_free(pay);
                                                pay = NULL;
                                            }
                                        }
                                        NBThreadMutex_lock(&opq->mutex);
                                        //add (locked)
                                        if(ff != NULL){
                                            NBArray_addValue(&gg->files, ff);
                                        }
                                        //remove used files from the flat-array
                                        {
                                            SI32 i; for(i = 0; i < filenamesUsed.use; i++){
                                                STNBString* str = NBArray_itmPtrAtIndex(&filenamesUsed, STNBString, i);
                                                SI32 i2; for(i2 = 0; i2 < gg->myFldr->subfiles.count; i2++){
                                                    STTSFile** f = NBArray_itmPtrAtIndex(&opq->mapping.files, STTSFile*, gg->myFldr->subfiles.iStart + i2);
                                                    if(*f != NULL && NBString_isEqual(str, (*f)->name)){
                                                        TSFile_release(*f);
                                                        NBMemory_free(*f);
                                                        *f = NULL;
                                                        break;
                                                    }
                                                } NBASSERT(i2 < gg->myFldr->subfiles.count) //should be found
                                                NBString_release(str);
                                            }
                                            NBArray_empty(&filenamesUsed);
                                            NBArray_release(&filenamesUsed);
                                        }
                                        NBString_release(&pathNoExt);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

SI64 TSStreamsStorageLoader_NBThreadPoolRoutinePtr_(void* param){ //task (will run once)
    SI64 r = 0; BOOL notifyEnd = FALSE, notifyIsSucess = FALSE;
    STTSStreamsStorageLoaderOpq* opq = (STTSStreamsStorageLoaderOpq*)param;
    NBThreadMutex_lock(&opq->mutex);
    {
        while(!opq->isErr && !opq->isCompleted && (opq->params.externalStopFlag == NULL || !(*opq->params.externalStopFlag))){
            NBThreadMutex_unlock(&opq->mutex);
            {
                TSStreamsStorageLoader_tick_(opq);
            }
            NBThreadMutex_lock(&opq->mutex);
        }
        //end-of-thread
        NBASSERT(opq->threads.runningCount > 0)
        if(opq->threads.runningCount > 0){
            opq->threads.runningCount--;
            NBThreadCond_broadcast(&opq->cond); //notify
            if(opq->threads.runningCount == 0){
                notifyEnd = TRUE;
                notifyIsSucess = (!opq->isErr && opq->isCompleted);
            }
        }
    }
    //notify (unloked)
    if(notifyEnd){
        //retain myself (in case the notification releases me)
        opq->retainCount++;
        NBThreadMutex_unlock(&opq->mutex);
        {
            if(opq->params.endCallback != NULL){
                STTSFile* rootTrash = (opq->mapping.files.use > 0 ? NBArray_itmValueAtIndex(&opq->mapping.files, STTSFile*, 0) : NULL);
                const SI32 streamsSz = opq->loading.streams.use;
                STTSStreamsStorageLoadStream** streams = NBArray_dataPtr(&opq->loading.streams, STTSStreamsStorageLoadStream*);
                (*opq->params.endCallback)(notifyIsSucess, rootTrash, streams, streamsSz, opq->params.endCallbackUsrData);
            }
        }
        //revert unconsumed updates
        NBThreadMutex_lock(&opq->mutex);
        {
            TSStreamsStorageLoader_doPostEndNotificationLockedOpq_(opq);
        }
    }
    NBThreadMutex_unlock(&opq->mutex);
    if(notifyEnd){
        //release myself (if previously retained)
        STTSStreamsStorageLoader ref;
        ref.opaque = opq;
        TSStreamsStorageLoader_release(&ref);
    }
    return r;
}

//STTSStreamsStorageLoadFolderRng

void TSStreamsStorageLoadFolderRng_init(STTSStreamsStorageLoadFolderRng* obj){
    NBMemory_setZeroSt(*obj, STTSStreamsStorageLoadFolderRng);
}

void TSStreamsStorageLoadFolderRng_release(STTSStreamsStorageLoadFolderRng* obj){
    if(obj->path != NULL){
        NBMemory_free(obj->path);
        obj->path = NULL;
    }
}

//

BOOL TSStreamsStorageLoadFolderRng_setPath(STTSStreamsStorageLoadFolderRng* obj, const char* path, const SI32 filenameLen){
    BOOL r = FALSE;
    //release
    if(obj->path != NULL){
        NBMemory_free(obj->path);
        obj->path = NULL;
        obj->nameLen = 0;
    }
    //set
    if(path == NULL){
        r = TRUE;
    } else {
        obj->path = NBString_strNewBuffer(path);
        obj->nameLen = filenameLen;
        r = (obj->path != NULL);
    }
    return r;
}

BOOL TSStreamsStorageLoadFolderRng_setParent(STTSStreamsStorageLoadFolderRng* obj, STTSFile* parent){
    obj->parent = parent;
    return TRUE;
}

BOOL TSStreamsStorageLoadFolderRng_setMyNode(STTSStreamsStorageLoadFolderRng* obj, STTSFile* myNode){
    obj->myNode = myNode;
    return TRUE;
}

BOOL TSStreamsStorageLoadFolderRng_setSubfilesRng(STTSStreamsStorageLoadFolderRng* obj, const SI32 iStart, const SI32 count){
    obj->subfiles.iStart = iStart;
    obj->subfiles.count = count;
    return TRUE;
}

//STTSStreamsStorageLoadStream

void TSStreamsStorageLoadStream_init(STTSStreamsStorageLoadStream* obj){
    NBMemory_setZeroSt(*obj, STTSStreamsStorageLoadStream);
    NBArray_initWithSz(&obj->versions, sizeof(STTSStreamsStorageLoadStreamVer*), NULL, 0, 4, 0.1f);
}

void TSStreamsStorageLoadStream_release(STTSStreamsStorageLoadStream* obj){
    {
        SI32 i; for(i = 0; i < obj->versions.use; i++){
            STTSStreamsStorageLoadStreamVer* rr = NBArray_itmValueAtIndex(&obj->versions, STTSStreamsStorageLoadStreamVer*, i);
            if(rr != NULL){
                TSStreamsStorageLoadStreamVer_release(rr);
                NBMemory_free(rr);
            }
        }
        NBArray_empty(&obj->versions);
        NBArray_release(&obj->versions);
    }
}

//STTSStreamsStorageLoadStreamVer

void TSStreamsStorageLoadStreamVer_init(STTSStreamsStorageLoadStreamVer* obj){
    NBMemory_setZeroSt(*obj, STTSStreamsStorageLoadStreamVer);
    NBArray_initWithSz(&obj->grps, sizeof(STTSStreamsStorageLoadStreamVer*), NULL, 0, 32, 0.1f);
}

void TSStreamsStorageLoadStreamVer_release(STTSStreamsStorageLoadStreamVer* obj){
    {
        SI32 i; for(i = 0; i < obj->grps.use; i++){
            STTSStreamsStorageLoadStreamVerGrp* grp = NBArray_itmValueAtIndex(&obj->grps, STTSStreamsStorageLoadStreamVerGrp*, i);
            if(grp != NULL){
                TSStreamsStorageLoadStreamVerGrp_release(grp);
                NBMemory_free(grp);
            }
        }
        NBArray_empty(&obj->grps);
        NBArray_release(&obj->grps);
    }
}

void TSStreamsStorageLoadStreamVer_sortGrps(STTSStreamsStorageLoadStreamVer* obj){
    //by grp.seq's value
    if(obj->grps.use > 1){
        SI32 i, i2, count2 = obj->grps.use; STTSStreamsStorageLoadStreamVerGrp cpy;
        for(i = 1; i < obj->grps.use; i++){ //do (n-1) times
            STTSStreamsStorageLoadStreamVerGrp* prev = NBArray_itmValueAtIndex(&obj->grps, STTSStreamsStorageLoadStreamVerGrp*, 0);
            for(i2 = 1; i2 < count2; i2++){
                STTSStreamsStorageLoadStreamVerGrp* ff = NBArray_itmValueAtIndex(&obj->grps, STTSStreamsStorageLoadStreamVerGrp*, i2);
                if(prev->seq > ff->seq){
                    //swap values
                    cpy = *ff;
                    *ff = *prev;
                    *prev = cpy;
                }
                prev = ff;
            }
            count2--; //another biggest element was pushed to the end
        }
    }
    //validate
    IF_NBASSERT({
        STTSStreamsStorageLoadStreamVerGrp* prev = NULL;
        SI32 i; for(i = 0; i < obj->grps.use; i++){
            STTSStreamsStorageLoadStreamVerGrp* ff = NBArray_itmValueAtIndex(&obj->grps, STTSStreamsStorageLoadStreamVerGrp*, i);
            NBASSERT(prev != ff) //duplicated
            NBASSERT(prev == NULL || prev->seq <= ff->seq)
            prev = ff;
        }
    })
}

//STTSStreamsStorageLoadStreamVerGrp

void TSStreamsStorageLoadStreamVerGrp_init(STTSStreamsStorageLoadStreamVerGrp* obj){
    NBMemory_setZeroSt(*obj, STTSStreamsStorageLoadStreamVerGrp);
    NBArray_initWithSz(&obj->files, sizeof(STTSStreamsStorageLoadStreamVerGrpFiles*), NULL, 0, 64, 0.1f);
}

void TSStreamsStorageLoadStreamVerGrp_release(STTSStreamsStorageLoadStreamVerGrp* obj){
    {
        SI32 i; for(i = 0; i < obj->files.use; i++){
            STTSStreamsStorageLoadStreamVerGrpFiles* ff = NBArray_itmValueAtIndex(&obj->files, STTSStreamsStorageLoadStreamVerGrpFiles*, i);
            if(ff != NULL){
                TSStreamsStorageLoadStreamVerGrpFiles_release(ff);
                NBMemory_free(ff);
            }
        }
        NBArray_empty(&obj->files);
        NBArray_release(&obj->files);
    }
    if(obj->head != NULL){
        NBStruct_stRelease(TSStreamStorageFilesGrpHeader_getSharedStructMap(), obj->head, sizeof(*obj->head));
        NBMemory_free(obj->head);
        obj->head = NULL;
    }
}

void TSStreamsStorageLoadStreamVerGrp_sortFiles(STTSStreamsStorageLoadStreamVerGrp* obj){
    //by file.seq's value
    if(obj->files.use > 1){
        SI32 i, i2, count2 = obj->files.use; STTSStreamsStorageLoadStreamVerGrpFiles cpy;
        for(i = 1; i < obj->files.use; i++){ //do (n-1) times
            STTSStreamsStorageLoadStreamVerGrpFiles* prev = NBArray_itmValueAtIndex(&obj->files, STTSStreamsStorageLoadStreamVerGrpFiles*, 0);
            for(i2 = 1; i2 < count2; i2++){
                STTSStreamsStorageLoadStreamVerGrpFiles* ff = NBArray_itmValueAtIndex(&obj->files, STTSStreamsStorageLoadStreamVerGrpFiles*, i2);
                if(prev->seq > ff->seq){
                    //swap values
                    cpy = *ff;
                    *ff = *prev;
                    *prev = cpy;
                }
                prev = ff;
            }
            count2--; //another biggest element was pushed to the end
        }
    }
    //validate
    IF_NBASSERT({
        STTSStreamsStorageLoadStreamVerGrpFiles* prev = NULL;
        SI32 i; for(i = 0; i < obj->files.use; i++){
            STTSStreamsStorageLoadStreamVerGrpFiles* ff = NBArray_itmValueAtIndex(&obj->files, STTSStreamsStorageLoadStreamVerGrpFiles*, i);
            NBASSERT(prev != ff) //duplicated
            NBASSERT(prev == NULL || prev->seq <= ff->seq)
            prev = ff;
        }
    })
}

//STTSStreamsStorageLoadStreamVerGrpFiles

void TSStreamsStorageLoadStreamVerGrpFiles_init(STTSStreamsStorageLoadStreamVerGrpFiles* obj){
    NBMemory_setZeroSt(*obj, STTSStreamsStorageLoadStreamVerGrpFiles);
}

void TSStreamsStorageLoadStreamVerGrpFiles_release(STTSStreamsStorageLoadStreamVerGrpFiles* obj){
    if(obj->payload != NULL){
        TSStreamStorageFiles_release(obj->payload);
        obj->payload = NULL;
    }
}

//

STTSStreamsStorageLoadFolderRng* TSStreamsStorageLoader_getFolderRngBy_(STTSStreamsStorageLoaderOpq* opq, const STTSFile* folderNode){ //the folder's node, not a child
    STTSStreamsStorageLoadFolderRng* r = NULL;
    SI32 i; for(i = 0; i < opq->mapping.folders.use; i++){
        STTSStreamsStorageLoadFolderRng* fldr2 =  NBArray_itmValueAtIndex(&opq->mapping.folders, STTSStreamsStorageLoadFolderRng*, i);
        if(fldr2->myNode == folderNode){
            r = fldr2;
            break;
        }
    }
    return r;
}

//STTSFilesCount

void TSFilesTreeCounts_init(STTSFilesTreeCounts* obj){
    NBMemory_setZeroSt(*obj, STTSFilesTreeCounts);
}

void TSFilesTreeCounts_release(STTSFilesTreeCounts* obj){
    //nothing
}

void TSFilesTreeCounts_reset(STTSFilesTreeCounts* obj){
    NBMemory_setZeroSt(*obj, STTSFilesTreeCounts);
}

//header

BOOL TSStreamsStorageLoader_writeStreamVerGrpHeader(const char* folderPath, const STTSStreamStorageFilesGrpHeader* head){
    BOOL r = FALSE;
    if(folderPath != NULL){
        STNBString path;
        NBString_init(&path);
        NBString_concat(&path, folderPath);
        //Concat slash (if necesary)
        if(path.length > 0){
            if(path.str[path.length - 1] != '/' && path.str[path.length - 1] != '\\'){
                NBString_concatByte(&path, '/');
            }
        }
        NBString_concat(&path, TS_STREAM_STORAGE_FILES_GRP_HEADER_NAME);
        if(!NBStruct_stWriteToJsonFilepath(path.str, TSStreamStorageFilesGrpHeader_getSharedStructMap(), head, sizeof(*head))){
            PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, NBStruct_stWriteToJsonFilepath failed for: '%s'.\n", path.str);
        } else {
            r = TRUE;
        }
        NBString_release(&path);
    }
    return r;
}

BOOL TSStreamsStorageLoader_loadStreamVerGrpHeader(const char* folderPath, STTSStreamStorageFilesGrpHeader* dst, STNBArray* optDstFilenamesUsed /*STNBString*/){
    BOOL r = FALSE;
    const SI32 optDstFilenamesUsedSzBefore = (optDstFilenamesUsed != NULL ? optDstFilenamesUsed->use : 0);
    if(folderPath != NULL && dst != NULL){
        STNBString path;
        NBString_init(&path);
        NBString_concat(&path, folderPath);
        //Concat slash (if necesary)
        if(path.length > 0){
            if(path.str[path.length - 1] != '/' && path.str[path.length - 1] != '\\'){
                NBString_concatByte(&path, '/');
            }
        }
        NBString_concat(&path, TS_STREAM_STORAGE_FILES_GRP_HEADER_NAME);
        //Release
        NBStruct_stRelease(TSStreamStorageFilesGrpHeader_getSharedStructMap(), dst, sizeof(*dst));
        //Load
        if(!NBStruct_stReadFromJsonFilepath(path.str, TSStreamStorageFilesGrpHeader_getSharedStructMap(), dst, sizeof(*dst))){
            //PRINTF_CONSOLE_ERROR("TSStreamStorageFilesGrp, NBStruct_stReadFromJsonFilepath failed for: '%s'.\n", path.str);
        } else {
            r = TRUE;
            //add filename
            if(optDstFilenamesUsed != NULL){
                STNBString str;
                NBString_initWithStr(&str, TS_STREAM_STORAGE_FILES_GRP_HEADER_NAME);
                NBArray_addValue(optDstFilenamesUsed, str);
            }
        }
        NBString_release(&path);
    }
    //revert filenames added
    if(!r && optDstFilenamesUsed != NULL){
        while(optDstFilenamesUsed->use > optDstFilenamesUsedSzBefore){
            STNBString* str = NBArray_itmPtrAtIndex(optDstFilenamesUsed, STNBString, optDstFilenamesUsed->use - 1);
            NBString_release(str);
            NBArray_removeItemAtIndex(optDstFilenamesUsed, optDstFilenamesUsed->use - 1);
        }
    }
    return r;
}

