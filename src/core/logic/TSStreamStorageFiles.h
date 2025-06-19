//
//  TSStreamStorageFiles.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSStreamStorageFiles_h
#define TSStreamStorageFiles_h

#include "nb/core/NBDataChunk.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBThreadsPool.h"
#include "nb/files/NBFilesystem.h"
//
#include "core/TSContext.h"
#include "core/logic/TSStreamDefs.h"
#include "core/logic/TSStreamsStorageStats.h"
#include "core/logic/TSFile.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TS_STREAM_FILE_IDX_HEADER			0x55	//0101-0101
#define TS_STREAM_FILE_IDX_FOOTER			0xAA	//1010-1010
#define TS_STREAM_FILE_IDX_VERSION			0
#define TS_STREAM_FILE_IDX_HEAD_VERSION		0
#define TS_STREAM_FILE_IDX_0_RECORDS_SZ		16		//HVTR-RRRP-PPPS-SSSF (H=header, V=version,  T=nalType, R=relTimestampRtp, P=filePos, S=nalSize, F=footer)
													//0123-4567-8901-2345

//Load coordinator (optimization, optional multithread load)

typedef struct STTSStreamFilesLoadLock_ {
	STNBThreadMutex		mutex;
	STNBThreadCond		cond;
} STTSStreamFilesLoadLock;

//Load result

typedef struct STTSStreamFilesLoadResult_ {
    BOOL        isCompleted;    //load action is completed
	BOOL		initErrFnd;		//error before load of unit started
	UI32		nonEmptyCount;	//amount of non-empty files found
	UI32		successCount;	//amount of files success loaded
} STTSStreamFilesLoadResult;

//Stream storage files grp header

typedef struct STTSStreamStorageFilesDesc_ {
	//Ref
	UI32		streamId;		//seqId
	UI32		versionId;		//seqId
	UI32		groupId;		//seqId
	UI32		filesId;		//seqId
	//Data
	UI64		fillStartTime;	//first write timestamp
	UI32		fillTsRtp;		//first rtp timestamp
	UI32		filesSize;		//total files sizes (payload + index + gaps)
} STTSStreamStorageFilesDesc;

//

typedef struct STTSStreamFilesHeader_ {
	UI8			head;			//TS_STREAM_FILE_IDX_HEADER
	UI8			version;		//format version
	char		runId[40];		//files-sequence-id (this helps to identify files from the same program-run)
	UI64		fillStartTime;	//first write timestamp
	UI32		fillTsRtp;		//first rtp timestamp
	UI8			foot;			//TS_STREAM_FILE_IDX_FOOTER
} STTSStreamFilesHeader;

typedef struct STTSStreamFilesSizes_ {
	UI64		payload;		//position/size of payload file
	UI64		index;			//position/size of index file
	UI64		gaps;			//position/size of gaps file
} STTSStreamFilesSizes;

typedef struct STTSStreamFilesPos_ {
	UI32		payload;		//position/size of payload file
	UI32		index;			//position/size of index file
	UI32		gaps;			//position/size of gaps file
	UI64		fillStartTime;	//first write timestamp
	UI32		fillTsRtp;		//first rtp timestamp
	UI32		fillTimeRtp;	//relative rtp time filled into file
} STTSStreamFilesPos;

typedef struct STTSStreamFileIdx_ {
	UI32		tsRtpRel;		//rtp timestamp relative to this file
	UI32		pos;			//pos-at-file
	UI32		nalTypeAndsize;	//8-bits(nalType) + 24-bits(size-on-file)
} STTSStreamFileIdx;

typedef struct STTSStreamStorageFiles_ {
	void*		opaque;
} STTSStreamStorageFiles;

void TSStreamStorageFiles_init(STTSStreamStorageFiles* obj);
void TSStreamStorageFiles_release(STTSStreamStorageFiles* obj);

//Config
BOOL TSStreamStorageFiles_setFilesystem(STTSStreamStorageFiles* obj, STNBFilesystem* fs);
BOOL TSStreamStorageFiles_setRunId(STTSStreamStorageFiles* obj, STTSContext* context, const char* runId);
BOOL TSStreamStorageFiles_setFilepathsWithoutExtension(STTSStreamStorageFiles* obj, const UI32 iSeq, const char* filepathWithoutExt);
BOOL TSStreamStorageFiles_setParams(STTSStreamStorageFiles* obj, const STTSStreamParamsWrite* params, const BOOL useRefNotClone);

//State
STTSStreamFilesPos TSStreamStorageFiles_getFilesPos(STTSStreamStorageFiles* obj);

//Actions
BOOL TSStreamStorageFiles_loadFiles(STTSStreamStorageFiles* obj, const char* filterRunId, const BOOL forceLoadAll, BOOL* dstIsEmptyPayload, STTSStreamsStorageStatsUpd* dstUpd, STNBArray* optDstFilenamesUsed /*STNBString*/, STNBString* optDstErrStr);
void TSStreamStorageFiles_getStatsUpdAsOthers(STTSStreamStorageFiles* obj, STTSStreamsStorageStatsUpd* dstUpd); //moves a previous upd applied from 'TSStreamStorageFiles_loadFiles' from 'payload' to 'others'.
BOOL TSStreamStorageFiles_createFiles(STTSStreamStorageFiles* obj, const char* runId);
BOOL TSStreamStorageFiles_closeFiles(STTSStreamStorageFiles* obj, const BOOL deleteAnyIfEmpty, STTSStreamsStorageStatsUpd* dstUpd, STTSStreamUnitsReleaser* optUnitsReleaser);
UI64 TSStreamStorageFiles_deleteFiles(STTSStreamStorageFiles* obj, STTSStreamsStorageStatsUpd* dstUpd);
UI32 TSStreamStorageFiles_appendBlocksUnsafe(STTSStreamStorageFiles* obj, STTSStreamUnit* units, const UI32 unitsSz, STTSStreamUnitsReleaser* optUnitsReleaser, STTSStreamFilesPos* dstCurPos, BOOL* dstStreamChanged);
BOOL TSStreamStorageFiles_appendGapsUnsafe(STTSStreamStorageFiles* obj, STTSStreamUnit* units, const UI32 unitsSz, UI32* dstPayloadCurPos);

#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSStreamStorageFiles_h */
