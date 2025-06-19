//
//  TSStreamStorage.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSStreamStorage_h
#define TSStreamStorage_h

#include "nb/core/NBLog.h"
#include "nb/files/NBFilesystem.h"
#include "core/logic/TSStreamDefs.h"
#include "core/logic/TSStreamVersionStorage.h"
#include "core/logic/TSStreamsStorageStats.h"
#include "core/logic/TSStreamsStorageLoader.h"
#include "core/logic/TSFile.h"

#ifdef __cplusplus
extern "C" {
#endif
	
typedef struct STTSStreamStorage_ {
	void* opaque;
} STTSStreamStorage;

void TSStreamStorage_init(STTSStreamStorage* obj);
void TSStreamStorage_release(STTSStreamStorage* obj);

//Config
BOOL TSStreamStorage_setStatsProvider(STTSStreamStorage* obj, STTSStreamsStorageStats* provider);
BOOL TSStreamStorage_setFilesystem(STTSStreamStorage* obj, STNBFilesystem* fs);
BOOL TSStreamStorage_setRunId(STTSStreamStorage* obj, STTSContext* context, const char* runId);
const char* TSStreamStorage_getUid(STTSStreamStorage* obj);
BOOL TSStreamStorage_setUid(STTSStreamStorage* obj, const char* parentPath, const UI32 iSeq, const char* uid, BOOL* dstCreatedNow);
BOOL TSStreamStorage_isUid(STTSStreamStorage* obj, const char* uid);

//Versions
STTSStreamVersionStorage* TSStreamStorage_getVersionRetained(STTSStreamStorage* obj, const char* uid, const BOOL createIfNecesary);
STTSStreamVersionStorage* TSStreamStorage_getVersionByIdxRetained(STTSStreamStorage* obj, const SI32 idx);
BOOL TSStreamStorage_returnVersion(STTSStreamStorage* obj, STTSStreamVersionStorage* version);

//Actions
BOOL TSStreamStorage_loadVersionsOwning(STTSStreamStorage* obj, STTSStreamsStorageLoadStreamVer** vers, const SI32 versSz);
//ToDo: remove
//STTSStreamFilesLoadResult TSStreamStorage_load(STTSStreamStorage* obj, const BOOL forceLoadAll, const BOOL* externalStopFlag);

//Files
void TSStreamStorage_addFilesDesc(STTSStreamStorage* obj, STNBArray* dst); //STTSStreamStorageFilesDesc
UI64 TSStreamStorage_deleteFile(STTSStreamStorage* obj, const STTSStreamStorageFilesDesc* d, STTSStreamsStorageStatsUpd* dstUpd);

#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSStreamStorage_h */
