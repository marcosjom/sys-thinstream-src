//
//  TSStreamsStorageStats.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSStreamsStorageStats_h
#define TSStreamsStorageStats_h

#include "nb/core/NBMemory.h"
#include "nb/core/NBString.h"
#include "nb/core/NBLog.h"
#include "core/config/TSCfgTelemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

//Folders

//TSStreamsStorageStatsStreams

typedef struct STTSStreamsStorageStatsStreams_ {
	UI32	loaded;     //automatically calculated
	UI32	added;
	UI32	removed;
} STTSStreamsStorageStatsStreams;

const STNBStructMap* TSStreamsStorageStatsStreams_getSharedStructMap(void);

//TSStreamsStorageStatsStreamsVersions

typedef struct STTSStreamsStorageStatsStreamsVersions_ {
	UI32	loaded;     //automatically calculated
	UI32	added;
	UI32	removed;
} STTSStreamsStorageStatsStreamsVersions;

const STNBStructMap* TSStreamsStorageStatsStreamsVersions_getSharedStructMap(void);

//TSStreamsStorageStatsStreamsVersionsGroups

typedef struct STTSStreamsStorageStatsStreamsVersionsGroups_ {
	UI32	loaded;     //automatically calculated
	UI32	added;
	UI32	removed;
} STTSStreamsStorageStatsStreamsVersionsGroups;

const STNBStructMap* TSStreamsStorageStatsStreamsVersionsGroups_getSharedStructMap(void);

//

typedef struct STTSStreamsStorageStatsFolders_ {
	STTSStreamsStorageStatsStreams					streams; //streams count
	STTSStreamsStorageStatsStreamsVersions			streamsVersions; //substreams count
	STTSStreamsStorageStatsStreamsVersionsGroups	streamsVersionsGroups; //substreams-subfolders count
} STTSStreamsStorageStatsFolders;

const STNBStructMap* TSStreamsStorageStatsFolders_getSharedStructMap(void);

//Files

//TSStreamsStorageStatsFileCounts

typedef struct STTSStreamsStorageStatsFileCounts_ {
	UI32	count;
	UI64	bytesCount;
} STTSStreamsStorageStatsFileCounts;

const STNBStructMap* TSStreamsStorageStatsFileCounts_getSharedStructMap(void);

//TSStreamsStorageStatsFileIdxCounts

typedef struct STTSStreamsStorageStatsFileIdxCounts_ {
	UI32	count;
	UI32	recordsCount;
	UI64	bytesCount;
} STTSStreamsStorageStatsFileIdxCounts;

const STNBStructMap* TSStreamsStorageStatsFileIdxCounts_getSharedStructMap(void);

//TSStreamsStorageStatsFile

typedef struct STTSStreamsStorageStatsFile_ {
	STTSStreamsStorageStatsFileCounts	loaded;
	STTSStreamsStorageStatsFileCounts	added;
	STTSStreamsStorageStatsFileCounts	removed;
} STTSStreamsStorageStatsFile;

const STNBStructMap* TSStreamsStorageStatsFile_getSharedStructMap(void);

//TSStreamsStorageStatsFileIdx

typedef struct STTSStreamsStorageStatsFileIdx_ {
	STTSStreamsStorageStatsFileIdxCounts	loaded;
	STTSStreamsStorageStatsFileIdxCounts	added;
	STTSStreamsStorageStatsFileIdxCounts	removed;
} STTSStreamsStorageStatsFileIdx;

const STNBStructMap* TSStreamsStorageStatsFileIdx_getSharedStructMap(void);

//TSStreamsStorageStatsFiles

typedef struct STTSStreamsStorageStatsFiles_ {
	STTSStreamsStorageStatsFile		payloads;
	STTSStreamsStorageStatsFileIdx	indexes;
	STTSStreamsStorageStatsFileIdx	gaps;
	STTSStreamsStorageStatsFile		trash;	//files not loaded, but listed to deletion if space is necesary
    STTSStreamsStorageStatsFile     others; //files not loaded, but not listed for deleteion (ignore-list)
} STTSStreamsStorageStatsFiles;

const STNBStructMap* TSStreamsStorageStatsFiles_getSharedStructMap(void);

//TSStreamsStorageStatsState

typedef struct STTSStreamsStorageStatsState_ {
	STTSStreamsStorageStatsFolders	folders;
	STTSStreamsStorageStatsFiles	files;
	UI64							updCalls;
} STTSStreamsStorageStatsState;

const STNBStructMap* TSStreamsStorageStatsState_getSharedStructMap(void);

//TSStreamsStorageStatsData

typedef struct STTSStreamsStorageStatsData_ {
	STTSStreamsStorageStatsState	loaded;		//loaded
	STTSStreamsStorageStatsState	accum;		//accum
	STTSStreamsStorageStatsState	total;		//total
} STTSStreamsStorageStatsData;

const STNBStructMap* TSStreamsStorageStatsData_getSharedStructMap(void);

//Update

//TSStreamsStorageStatsUpdFolderAct

typedef struct STTSStreamsStorageStatsUpdFolderAct_ {
	UI32	added;
	UI32	removed;
} STTSStreamsStorageStatsUpdFolderAct;

typedef struct STTSStreamsStorageStatsUpdFile_ {
	UI32	count;
	UI64	bytesCount;
} STTSStreamsStorageStatsUpdFile;

typedef struct STTSStreamsStorageStatsUpdFileAct_ {
	STTSStreamsStorageStatsUpdFile	added;
	STTSStreamsStorageStatsUpdFile	removed;
} STTSStreamsStorageStatsUpdFileAct;

typedef struct STTSStreamsStorageStatsUpdFileIdx_ {
	UI32	count;
	UI32	recordsCount;
	UI64	bytesCount;
} STTSStreamsStorageStatsUpdFileIdx;

typedef struct STTSStreamsStorageStatsUpdFileIdxAct_ {
	STTSStreamsStorageStatsUpdFileIdx	added;
	STTSStreamsStorageStatsUpdFileIdx	removed;
} STTSStreamsStorageStatsUpdFileIdxAct;

typedef struct STTSStreamsStorageStatsUpd_ {
	//folders
	struct {
		STTSStreamsStorageStatsUpdFolderAct		streams;
		STTSStreamsStorageStatsUpdFolderAct		streamsVersions;
		STTSStreamsStorageStatsUpdFolderAct		streamsVersionsGroups;
	} folders;
	//files
	struct {
		STTSStreamsStorageStatsUpdFileAct		payloads;
		STTSStreamsStorageStatsUpdFileIdxAct	indexes;
		STTSStreamsStorageStatsUpdFileIdxAct	gaps;
		STTSStreamsStorageStatsUpdFileAct		trash;  //files not loaded, but listed to deletion if space is necesary
        STTSStreamsStorageStatsUpdFileAct       others; //files not loaded, but not listed for deleteion (ignore-list)
	} files;
} STTSStreamsStorageStatsUpd;

//Stats

typedef struct STTSStreamsStorageStats_ {
	void* opaque;
} STTSStreamsStorageStats;

void TSStreamsStorageStats_init(STTSStreamsStorageStats* obj);
void TSStreamsStorageStats_release(STTSStreamsStorageStats* obj);

//Data
STTSStreamsStorageStatsData TSStreamsStorageStats_getData(STTSStreamsStorageStats* obj, const BOOL resetAccum);
UI64 TSStreamsStorageStats_totalLoadedBytes(STTSStreamsStorageStats* obj);
UI64 TSStreamsStorageStats_totalLoadedBytesData(const STTSStreamsStorageStatsData* obj);
UI64 TSStreamsStorageStats_totalBytesState(const STTSStreamsStorageStatsState* obj);
void TSStreamsStorageStats_concat(STTSStreamsStorageStats* obj, const STTSCfgTelemetryFilesystem* cfg, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst, const BOOL resetAccum);
void TSStreamsStorageStats_concatData(const STTSStreamsStorageStatsData* obj, const STTSCfgTelemetryFilesystem* cfg, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst);
void TSStreamsStorageStats_concatState(const STTSStreamsStorageStatsState* obj, const STTSCfgTelemetryFilesystem* cfg, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst);
void TSStreamsStorageStats_concatFiles(const STTSStreamsStorageStatsFiles* obj, const STTSCfgTelemetryFilesystem* cfg, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst);
void TSStreamsStorageStats_concatFile(const STTSStreamsStorageStatsFile* obj, const STTSCfgTelemetryFilesystem* cfg, const BOOL ignoreEmpties, STNBString* dst);
void TSStreamsStorageStats_concatFileIdx(const STTSStreamsStorageStatsFileIdx* obj, const STTSCfgTelemetryFilesystem* cfg, const BOOL ignoreEmpties, STNBString* dst);
void TSStreamsStorageStats_concatFolders(const STTSStreamsStorageStatsFolders* obj, const STTSCfgTelemetryFilesystem* cfg, const BOOL ignoreEmpties, STNBString* dst);

//Update
void TSStreamsStorageStats_applyUpdate(STTSStreamsStorageStats* obj, const STTSStreamsStorageStatsUpd* upd); //applies the update to the main counters

#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSStreamsStorageStats_h */
