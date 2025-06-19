//
//  TSStreamsStorageLoader.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSStreamsStorageLoader_h
#define TSStreamsStorageLoader_h

#include "nb/core/NBLog.h"
#include "nb/core/NBThreadsPool.h"
#include "nb/files/NBFilesystem.h"
#include "core/TSContext.h"
#include "core/logic/TSFile.h"
#include "core/logic/TSStreamsStorageStats.h"
#include "core/logic/TSStreamStorageFiles.h"

#ifdef __cplusplus
extern "C" {
#endif

//Stream storage files grp header

#define TS_STREAM_STORAGE_FILES_GRP_HEADER_NAME     "_head.json"

typedef struct STTSStreamStorageFilesGrpHeader_ {
    char*    runId;            //current files-sequence-id (this helps to identify files from the same program-run)
    UI64    fillStartTime;    //first write timestamp
} STTSStreamStorageFilesGrpHeader;

const STNBStructMap* TSStreamStorageFilesGrpHeader_getSharedStructMap(void);

//STTSStreamsStorageLoadFolderRng

typedef struct STTSStreamsStorageLoadFolderRng_ {
    char*       path;       //path
    SI32        nameLen;    //last chars in path that form the file's name only
    STTSFile*   parent;     //parent folder
    STTSFile*   myNode;     //my STFile* node
    //subfiles
    struct {
        SI32    iStart;
        SI32    count;
    } subfiles;
} STTSStreamsStorageLoadFolderRng;

void TSStreamsStorageLoadFolderRng_init(STTSStreamsStorageLoadFolderRng* obj);
void TSStreamsStorageLoadFolderRng_release(STTSStreamsStorageLoadFolderRng* obj);
//
BOOL TSStreamsStorageLoadFolderRng_setPath(STTSStreamsStorageLoadFolderRng* obj, const char* path, const SI32 filenameLen);
BOOL TSStreamsStorageLoadFolderRng_setParent(STTSStreamsStorageLoadFolderRng* obj, STTSFile* parent);
BOOL TSStreamsStorageLoadFolderRng_setMyNode(STTSStreamsStorageLoadFolderRng* obj, STTSFile* myNode);
BOOL TSStreamsStorageLoadFolderRng_setSubfilesRng(STTSStreamsStorageLoadFolderRng* obj, const SI32 iStart, const SI32 count);

//STTSStreamsStorageLoadStream

typedef struct STTSStreamsStorageLoadStream_ {
    STTSStreamsStorageLoadFolderRng* myFldr; //my STTSStreamsStorageLoadFolderRng* node
    STNBArray   versions;   //STTSStreamsStorageLoadStreamVer*
    SI32        iNext;      //next version to be loaded
} STTSStreamsStorageLoadStream;

void TSStreamsStorageLoadStream_init(STTSStreamsStorageLoadStream* obj);
void TSStreamsStorageLoadStream_release(STTSStreamsStorageLoadStream* obj);

//STTSStreamsStorageLoadStreamVer

typedef struct STTSStreamsStorageLoadStreamVer_ {
    STTSStreamsStorageLoadStream* parent;
    STTSStreamsStorageLoadFolderRng* myFldr; //my STTSStreamsStorageLoadFolderRng* node
    STNBArray   grps;       //STTSStreamsStorageLoadStreamVerGrp*
    SI32        iNext;      //next version to be loaded
} STTSStreamsStorageLoadStreamVer;

void TSStreamsStorageLoadStreamVer_init(STTSStreamsStorageLoadStreamVer* obj);
void TSStreamsStorageLoadStreamVer_release(STTSStreamsStorageLoadStreamVer* obj);
//
void TSStreamsStorageLoadStreamVer_sortGrps(STTSStreamsStorageLoadStreamVer* obj); //by grp.seq's value

//STTSStreamsStorageLoadStreamVerGrp

typedef struct STTSStreamsStorageLoadStreamVerGrp_ {
    STTSStreamsStorageLoadStreamVer* parent;
    STTSStreamsStorageLoadFolderRng* myFldr; //my STTSStreamsStorageLoadFolderRng* node
    STNBArray   files;      //STTSStreamsStorageLoadStreamVerGrpFiles*
    SI32        iNext;      //next version to be loaded
    //
    UI32        seq;        //numeric prefix in filename
    STTSStreamStorageFilesGrpHeader* head;  //if loaded
} STTSStreamsStorageLoadStreamVerGrp;

void TSStreamsStorageLoadStreamVerGrp_init(STTSStreamsStorageLoadStreamVerGrp* obj);
void TSStreamsStorageLoadStreamVerGrp_release(STTSStreamsStorageLoadStreamVerGrp* obj);
//
void TSStreamsStorageLoadStreamVerGrp_sortFiles(STTSStreamsStorageLoadStreamVerGrp* obj); //by file.seq's value

//STTSStreamsStorageLoadStreamVerGrpFiles

typedef struct STTSStreamsStorageLoadStreamVerGrpFiles_ {
    STTSStreamsStorageLoadStreamVerGrp* parent;
    UI32 seq;
    STTSStreamsStorageStatsUpd upd;
    STTSStreamStorageFiles* payload;
} STTSStreamsStorageLoadStreamVerGrpFiles;

void TSStreamsStorageLoadStreamVerGrpFiles_init(STTSStreamsStorageLoadStreamVerGrpFiles* obj);
void TSStreamsStorageLoadStreamVerGrpFiles_release(STTSStreamsStorageLoadStreamVerGrpFiles* obj);
//

//STTSFilesCount

typedef struct STTSFilesTreeCounts_ {
    //folders
    struct {
        UI32       total;   //
        UI32       empty;   //zero subelements inside
    } folders;
    //files
    struct {
        UI32       total;   //
        UI32       empty;   //zero bytes size
        UI32       nulls;   //deleted records
        UI64       totalBytes;   //
    } files;
} STTSFilesTreeCounts;

void TSFilesTreeCounts_init(STTSFilesTreeCounts* obj);
void TSFilesTreeCounts_release(STTSFilesTreeCounts* obj);
//
void TSFilesTreeCounts_reset(STTSFilesTreeCounts* obj);

//

struct STTSStreamsStorageLoader_;

typedef void (*TSStreamsStorageLoaderEndedFunc)(const BOOL isSuccess, STTSFile* rootTrash, STTSStreamsStorageLoadStream* const* streams, const SI32 streamsSz, void* usrData);
	
typedef struct STTSStreamsStorageLoader_ {
	void* opaque;
} STTSStreamsStorageLoader;

void TSStreamsStorageLoader_init(STTSStreamsStorageLoader* obj);
void TSStreamsStorageLoader_release(STTSStreamsStorageLoader* obj);

//If 'optInitialLoadThreads' is greather than '1' and 'optThreadsPool' not NULL,
//  the function returns inmediatly and the load is
//  done in secondary threads untill error or completion.
//else
//  the load is done in the current thread and the
//  function blocks untill error or completion.
BOOL TSStreamsStorageLoader_start(STTSStreamsStorageLoader* obj, STTSContext* context, STNBFilesystem* fs, STTSStreamsStorageStats* provider, const char* rootPath, const BOOL forceLoadAll, const char** ignoreTrashFiles, const UI32 ignoreTrashFilesSz, const BOOL* externalStopFlag, TSStreamsStorageLoaderEndedFunc endCallback, void* endCallbackUsrData, STNBThreadsPoolRef optThreadsPool, const UI32 optInitialLoadThreads);

//Headers
BOOL TSStreamsStorageLoader_writeStreamVerGrpHeader(const char* folderPath, const STTSStreamStorageFilesGrpHeader* head);
BOOL TSStreamsStorageLoader_loadStreamVerGrpHeader(const char* folderPath, STTSStreamStorageFilesGrpHeader* dst, STNBArray* optDstFilenamesUsed /*STNBString*/);


#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSStreamsStorageLoader_h */
