//
//  TSStreamStorageFilesGrp.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSStreamStorageFilesGrp_h
#define TSStreamStorageFilesGrp_h

#include "nb/core/NBStructMap.h"
#include "nb/files/NBFilesystem.h"
#include "nb/core/NBDataChunk.h"
//
#include "core/TSContext.h"
#include "core/logic/TSStreamDefs.h"
#include "core/logic/TSStreamStorageFiles.h"
#include "core/logic/TSStreamsStorageStats.h"
#include "core/logic/TSStreamsStorageLoader.h"
#include "core/logic/TSFile.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct STTSStreamStorageFilesGrp_ {
	void* opaque;
} STTSStreamStorageFilesGrp;

void TSStreamStorageFilesGrp_init(STTSStreamStorageFilesGrp* obj);
void TSStreamStorageFilesGrp_release(STTSStreamStorageFilesGrp* obj);

//Config
BOOL TSStreamStorageFilesGrp_setStatsProvider(STTSStreamStorageFilesGrp* obj, STTSStreamsStorageStats* provider);
BOOL TSStreamStorageFilesGrp_setFilesystem(STTSStreamStorageFilesGrp* obj, STNBFilesystem* fs);
BOOL TSStreamStorageFilesGrp_setRunId(STTSStreamStorageFilesGrp* obj, STTSContext* context, const char* runId);
BOOL TSStreamStorageFilesGrp_setUid(STTSStreamStorageFilesGrp* obj, const char* parentPath, const UI32 iSeq, const char* uid, BOOL* dstCreatedNow);
BOOL TSStreamStorageFilesGrp_setParams(STTSStreamStorageFilesGrp* obj, const STTSStreamParamsWrite* params, const BOOL useRefNotClone);

//Viewers
UI32 TSStreamStorageFilesGrp_viewersCount(STTSStreamStorageFilesGrp* obj);
BOOL TSStreamStorageFilesGrp_addViewer(STTSStreamStorageFilesGrp* obj, const STTSStreamLstnr* viewer, UI32* dstCount);
BOOL TSStreamStorageFilesGrp_removeViewer(STTSStreamStorageFilesGrp* obj, const STTSStreamLstnr* viewer, UI32* dstCount);

//State
//UI32 TSStreamStorageFilesGrp_payloadFilePos(STTSStreamStorageFilesGrp* obj);
 
//Actions
BOOL TSStreamStorageFilesGrp_loadSortedFilesOwning(STTSStreamStorageFilesGrp* obj, STTSStreamsStorageLoadStreamVerGrp* grp, STTSStreamsStorageLoadStreamVerGrpFiles** files, const SI32 filesSz);
//ToDo: remove
//STTSStreamFilesLoadResult TSStreamStorageFilesGrp_load(STTSStreamStorageFilesGrp* obj, const BOOL forceLoadAll, STNBString* optDstErrStr);
BOOL TSStreamStorageFilesGrp_createNextFiles(STTSStreamStorageFilesGrp* obj, const BOOL ignoreIfCurrentExists);
BOOL TSStreamStorageFilesGrp_closeFiles(STTSStreamStorageFilesGrp* obj, const BOOL deleteAnyIfEmpty, STTSStreamUnitsReleaser* optUnitsReleaser);
BOOL TSStreamStorageFilesGrp_appendBlocksUnsafe(STTSStreamStorageFilesGrp* obj, STTSStreamUnit* units, const UI32 unitsSz, STTSStreamUnitsReleaser* optUnitsReleaser);
BOOL TSStreamStorageFilesGrp_appendGapsUnsafe(STTSStreamStorageFilesGrp* obj, STTSStreamUnit* units, const UI32 unitsSz, STTSStreamUnitsReleaser* optUnitsReleaser);
//Files
UI32 TSStreamStorageFilesGrp_getFilesCount(STTSStreamStorageFilesGrp* obj);
void TSStreamStorageFilesGrp_addFilesDesc(STTSStreamStorageFilesGrp* obj, STNBArray* dst); //STTSStreamStorageFilesDesc
UI64 TSStreamStorageFilesGrp_deleteFile(STTSStreamStorageFilesGrp* obj, const STTSStreamStorageFilesDesc* d, STTSStreamsStorageStatsUpd* dstUpd);
BOOL TSStreamStorageFilesGrp_deleteFolder(STTSStreamStorageFilesGrp* obj, const BOOL onlyIfEmptyExceptHeaderAndEmptyFiles);


#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSStreamStorageFilesGrp_h */
