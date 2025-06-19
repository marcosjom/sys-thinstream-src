//
//  TSStreamsStorage.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSStreamsStorage_h
#define TSStreamsStorage_h

#include "nb/core/NBLog.h"
#include "nb/core/NBThreadsPool.h"
#include "nb/files/NBFilesystem.h"
#include "core/logic/TSStreamStorage.h"
#include "core/logic/TSStreamsStorageStats.h"
#include "core/logic/TSStreamsStorageLoader.h"

#ifdef __cplusplus
extern "C" {
#endif
	
typedef struct STTSStreamsStorage_ {
	void* opaque;
} STTSStreamsStorage;

typedef void (*TSStreamsStorageLoadEndedFunc)(const BOOL isSuccess, void* usrData);

void						TSStreamsStorage_init(STTSStreamsStorage* obj);
void						TSStreamsStorage_release(STTSStreamsStorage* obj);

//Config
BOOL						TSStreamsStorage_setStatsProvider(STTSStreamsStorage* obj, STTSStreamsStorageStats* provider);
BOOL						TSStreamsStorage_setFilesystem(STTSStreamsStorage* obj, STNBFilesystem* fs);
BOOL						TSStreamsStorage_setRootPath(STTSStreamsStorage* obj, const char* rootPath);
BOOL						TSStreamsStorage_setRunId(STTSStreamsStorage* obj, STTSContext* context, const char* runId);

//Streams
STTSStreamStorage*			TSStreamsStorage_getStreamRetained(STTSStreamsStorage* obj, const char* uid, const BOOL createIfNecesary);
STTSStreamStorage*			TSStreamsStorage_getStreamByIdxRetained(STTSStreamsStorage* obj, const SI32 idx);
BOOL						TSStreamsStorage_returnStream(STTSStreamsStorage* obj, STTSStreamStorage* stream);

//Actions
BOOL                        TSStreamsStorage_loadStart(STTSStreamsStorage* obj, const BOOL forceLoadAll, const char** ignoreTrashFiles, const UI32 ignoreTrashFilesSz, const BOOL* externalStopFlag, TSStreamsStorageLoadEndedFunc endCallback, void* endCallbackUsrData, STNBThreadsPoolRef optThreadsPool, const UI32 optInitialLoadThreads);
//STTSStreamFilesLoadResult TSStreamsStorage_load(STTSStreamsStorage* obj, const BOOL forceLoadAll, const char** ignoreTrashFiles, const UI32 ignoreTrashFilesSz, const BOOL* externalStopFlag, STNBThreadsPoolRef optThreadsPool, const UI32 optInitialLoadThreads);

//Files
void						TSStreamsStorage_addFilesDesc(STTSStreamsStorage* obj, STNBArray* dst); //STTSStreamStorageFilesDesc
UI64						TSStreamsStorage_deleteOldestFiles(STTSStreamsStorage* obj, const UI64 maxBytesToDelete, const BOOL keepUnknownFiles);

#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSStreamsStorage_h */
