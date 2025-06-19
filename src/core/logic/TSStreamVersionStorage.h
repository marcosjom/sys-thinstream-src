//
//  TSStreamVersionStorage.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSStreamVersionStorage_h
#define TSStreamVersionStorage_h

#include "nb/core/NBStructMap.h"
#include "nb/files/NBFilesystem.h"
#include "nb/core/NBDataChunk.h"
#include "core/TSContext.h"
#include "core/logic/TSStreamDefs.h"
#include "core/logic/TSStreamStorageFilesGrp.h"
#include "core/logic/TSStreamsStorageStats.h"
#include "core/logic/TSStreamsStorageLoader.h"
#include "core/logic/TSFile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct STTSStreamVersionStorage_ {
	void* opaque;
} STTSStreamVersionStorage;

void TSStreamVersionStorage_init(STTSStreamVersionStorage* obj);
void TSStreamVersionStorage_release(STTSStreamVersionStorage* obj);

//Config
BOOL TSStreamVersionStorage_setStatsProvider(STTSStreamVersionStorage* obj, STTSStreamsStorageStats* provider);
BOOL TSStreamVersionStorage_setFilesystem(STTSStreamVersionStorage* obj, STNBFilesystem* fs);
BOOL TSStreamVersionStorage_setRunId(STTSStreamVersionStorage* obj, STTSContext* context, const char* runId);
const char* TSStreamVersionStorage_getUid(STTSStreamVersionStorage* obj);
BOOL TSStreamVersionStorage_setUid(STTSStreamVersionStorage* obj, const char* parentPath, const UI32 iSeq, const char* uid, BOOL* dstCreatedNow);
BOOL TSStreamVersionStorage_setParams(STTSStreamVersionStorage* obj, const STTSStreamParamsWrite* params);
BOOL TSStreamVersionStorage_isUid(STTSStreamVersionStorage* obj, const char* uid);

//Viewers
STTSStreamLstnr TSStreamVersionStorage_getItfAsStreamLstnr(STTSStreamVersionStorage* obj);
UI32 TSStreamVersionStorage_viewersCount(STTSStreamVersionStorage* obj);
BOOL TSStreamVersionStorage_addViewer(STTSStreamVersionStorage* obj, const STTSStreamLstnr* viewer, UI32* dstCount);
BOOL TSStreamVersionStorage_removeViewer(STTSStreamVersionStorage* obj, const void* param, UI32* dstCount);

//Actions
BOOL TSStreamVersionStorage_loadSortedGrpsOwning(STTSStreamVersionStorage* obj, STTSStreamsStorageLoadStreamVerGrp** grps, const SI32 grpsSz);
//ToDo: remove
//STTSStreamFilesLoadResult TSStreamVersionStorage_load(STTSStreamVersionStorage* obj, const BOOL forceLoadAll, const BOOL* externalStopFlag);
BOOL TSStreamVersionStorage_createNextFilesGrp(STTSStreamVersionStorage* obj, const BOOL ignoreIfCurrentExists);
//Files
void TSStreamVersionStorage_addFilesDesc(STTSStreamVersionStorage* obj, STNBArray* dst); //STTSStreamStorageFilesDesc
UI64 TSStreamVersionStorage_deleteFile(STTSStreamVersionStorage* obj, const STTSStreamStorageFilesDesc* d, STTSStreamsStorageStatsUpd* dstUpd);

#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSStreamVersionStorage_h */
