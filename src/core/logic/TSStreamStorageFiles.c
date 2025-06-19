//
//  TSStreamStorageFiles.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSStreamStorageFiles.h"
//
#include <stdlib.h>	//for "rand()" 
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/net/NBRtp.h"
#include "nb/files/NBFile.h"

typedef struct STTSStreamStorageFilesOpq_ {
	STNBThreadMutex		mutex;
	STTSContext*		context;
	//cfg
	struct {
		STNBFilesystem*	fs;			//filesystem
		STNBString		runId;		//current files-sequence-id (this helps to identify files from the same program-run)
		UI32			iSeq;		//unique-id (numeric)
		STNBString		filepathBase;
		STTSStreamParamsWrite params;				//params
		BOOL			paramsNotCloned;	//params
	} cfg;
	//nalSeq
	struct {
		BOOL			isBlockStarted;		//is NALU 7 (first of an stream-block) found
		BOOL			isKeyFrameFound;	//is VCL unit (picture frame data) IDR picture (full frame) found
	} nalSeq;
	//index
	struct {
        SI32            rdrsCount;      //clients reading the index data (to load/unload on memory)
		STNBString		path;
		STNBString		buff;			//write buffer
		STNBFileRef		file;			//file to save index
		UI64			fillStartTime;	//first write timestamp
		UI32			fillTsRtp;		//first rtp packet timestamp
		UI32			fillTsRtpLast;	//last rtp timestamp, to calculate relative time
		UI32			fillTsRtpRelLast; //last rtp timestamp, to calculate relative time
		UI32			fileStart;		//file start pos without header
		UI32			filePos;		//file position (current size)
		STNBArray		arr;			//STTSStreamFileIdx
        //hdr
        struct {
            STTSStreamFileIdx   first;  //first record
            STTSStreamFileIdx   last;   //last record
            UI32                count;  //ammount of records
        } hdr;
	} index;
	//gaps
	struct {
        SI32            rdrsCount;  //clients reading the index data (to load/unload on memory)
		STNBString		path;
		STNBFileRef		file;		//file to save gaps only
		UI32			fileStart;	//file start pos without header
		UI32			filePos;	//file position (current size)
		STNBArray		arr;		//STTSStreamFileIdx
        //hdr
        struct {
            STTSStreamFileIdx   first;  //first record
            STTSStreamFileIdx   last;   //last record
            UI32                count;  //ammount of records
        } hdr;
	} gaps;
	//payload
	struct {
		STNBString		path;
		STNBArray		buff;		//STTSStreamUnit, write buffer
		STNBFileRef		file;		//file to save stream
		UI32			fileStart;	//file start pos without header
		UI32			filePos;
	} payload;
    //stats
    struct {
        STTSStreamsStorageStatsUpd upd;
    } stats;
	//dbg
#	ifdef NB_CONFIG_INCLUDE_ASSERTS
	struct {
		BOOL			inUnsafeAction;	//currently executing unsafe code
	} dbg;
#	endif
} STTSStreamStorageFilesOpq;

//

BOOL TSStreamStorageFiles_writeBuffsOpqUnsafe_(STTSStreamStorageFilesOpq* opq, STTSStreamUnitsReleaser* optUnitsReleaser);

//

void TSStreamStorageFiles_init(STTSStreamStorageFiles* obj){
	STTSStreamStorageFilesOpq* opq = obj->opaque = NBMemory_allocType(STTSStreamStorageFilesOpq);
	NBMemory_setZeroSt(*opq, STTSStreamStorageFilesOpq);
	NBThreadMutex_init(&opq->mutex);
	{
		//Cfg
		{
			NBString_init(&opq->cfg.runId);
			NBString_init(&opq->cfg.filepathBase);
		}
		//state
		{
			{
				NBString_init(&opq->index.path);
				NBString_initWithSz(&opq->index.buff, 0, 1024 * 10, 0.1f);
				NBArray_initWithSz(&opq->index.arr, sizeof(STTSStreamFileIdx), NULL, 0, 1024, 0.10f);
			}
			{
				NBString_init(&opq->gaps.path);
				NBArray_initWithSz(&opq->gaps.arr, sizeof(STTSStreamFileIdx), NULL, 0, 1024, 0.10f);
			}
			{
				NBString_init(&opq->payload.path);
				NBArray_init(&opq->payload.buff, sizeof(STTSStreamUnit), NULL);
			}
		}
	}
}

void TSStreamStorageFiles_release(STTSStreamStorageFiles* obj){
	STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)	//should not be called in parallel to feed thread
	{
		//flush
		{
			TSStreamStorageFiles_writeBuffsOpqUnsafe_(opq, NULL);
		}
		//state
		{
			//index
			{
                NBArray_empty(&opq->index.arr);
				NBArray_release(&opq->index.arr);
				NBString_release(&opq->index.buff);
				NBString_release(&opq->index.path);
				if(NBFile_isSet(opq->index.file)){
					NBFile_close(opq->index.file);
					NBFile_release(&opq->index.file);
                    NBFile_null(&opq->index.file);
				}
			}
			{
                NBArray_empty(&opq->gaps.arr);
				NBArray_release(&opq->gaps.arr);
				NBString_release(&opq->gaps.path);
				if(NBFile_isSet(opq->gaps.file)){
					NBFile_close(opq->gaps.file);
					NBFile_release(&opq->gaps.file);
                    NBFile_null(&opq->gaps.file);
				}
			}
			{
				//units
				{
					if(opq->payload.buff.use > 0){
						STTSStreamUnit* u = NBArray_dataPtr(&opq->payload.buff, STTSStreamUnit);
						TSStreamUnit_unitsReleaseGrouped(u, opq->payload.buff.use, NULL);
						NBArray_empty(&opq->payload.buff);
					}
					NBArray_release(&opq->payload.buff);
				}
				//path
				{
					NBString_release(&opq->payload.path);
				}
				//file
				if(NBFile_isSet(opq->payload.file)){
					NBFile_close(opq->payload.file);
					NBFile_release(&opq->payload.file);
                    NBFile_null(&opq->payload.file);
				}
			}
		}
		//Cfg
		{
			//release
			NBString_release(&opq->cfg.runId);
			NBString_release(&opq->cfg.filepathBase);
			{
				if(opq->cfg.paramsNotCloned){
					NBMemory_setZero(opq->cfg.params);
				} else {
					NBStruct_stRelease(TSStreamParamsWrite_getSharedStructMap(), &opq->cfg.params, sizeof(opq->cfg.params));
				}
			}
			opq->cfg.fs = NULL;
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	NBThreadMutex_release(&opq->mutex);
	NBMemory_free(opq);
	obj->opaque = NULL;
}

//Config

BOOL TSStreamStorageFiles_setFilesystem(STTSStreamStorageFiles* obj, STNBFilesystem* fs){
	BOOL r = FALSE;
	STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)	//should not be called in parallel to feed thread
	if(!NBFile_isSet(opq->payload.file)){
        //fs
		opq->cfg.fs = fs;
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamStorageFiles_setRunId(STTSStreamStorageFiles* obj, STTSContext* context, const char* runId){
	BOOL r = FALSE;
	STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)	//should not be called in parallel to feed thread
	if(!NBFile_isSet(opq->payload.file)){
		NBString_set(&opq->cfg.runId, runId);
		//context
		opq->context = context;
		//
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamStorageFiles_setFilepathsWithoutExtension(STTSStreamStorageFiles* obj, const UI32 iSeq, const char* filepathWithoutExt){
	BOOL r = FALSE;
	STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)	//should not be called in parallel to feed thread
	if(!NBFile_isSet(opq->payload.file)){
		opq->cfg.iSeq = iSeq;
		NBString_set(&opq->cfg.filepathBase, filepathWithoutExt);
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamStorageFiles_setParams(STTSStreamStorageFiles* obj, const STTSStreamParamsWrite* params, const BOOL useRefNotClone){
	BOOL r = FALSE;
	STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)	//should not be called in parallel to feed thread
	{
		r = TRUE;
		//Validate
		if(params != NULL){
			//
		}
		//Apply
		if(r){
			//release
			{
				if(opq->cfg.paramsNotCloned){
					NBMemory_setZero(opq->cfg.params);
				} else {
					NBStruct_stRelease(TSStreamParamsWrite_getSharedStructMap(), &opq->cfg.params, sizeof(opq->cfg.params));
				}
			}
			//clone
			if(params != NULL){
				if(useRefNotClone){
					opq->cfg.params = *params;
					opq->cfg.paramsNotCloned = TRUE;
				} else {
					NBStruct_stClone(TSStreamParamsWrite_getSharedStructMap(), params, sizeof(*params), &opq->cfg.params, sizeof(opq->cfg.params));
					opq->cfg.paramsNotCloned = FALSE;
				}
			}
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

//State

void TSStreamStorageFiles_getFilesPosLockedOpq_(STTSStreamStorageFilesOpq* opq, STTSStreamFilesPos* dst){
	dst->payload		= opq->payload.filePos;
	dst->index			= opq->index.filePos;
	dst->gaps			= opq->gaps.filePos;
	dst->fillStartTime	= opq->index.fillStartTime;
	dst->fillTsRtp		= opq->index.fillTsRtp;
	dst->fillTimeRtp	= opq->index.fillTsRtpRelLast;
}
	
STTSStreamFilesPos TSStreamStorageFiles_getFilesPos(STTSStreamStorageFiles* obj){
	STTSStreamFilesPos r;
	NBMemory_setZeroSt(r, STTSStreamFilesPos);
	STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)	//should not be called in parallel to feed thread
	{
		TSStreamStorageFiles_getFilesPosLockedOpq_(opq, &r);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

//Actions

BOOL TSStreamStorageFiles_loadFileIdx_(STNBFilesystem* fs, const char* fullfilepath, const UI32 payloadSz, const char* filterRunId, STTSStreamFilesHeader* dstHead, STNBArray* dstIdxs, UI32* dstFileStart, UI32* dstFilePos, STNBString* optDstErrStr, const BOOL mustBeSequential, const BOOL forceLoadAll){
	BOOL r = TRUE;
	UI32 fileStart = 0, fileCurPos = 0;
	STNBFileRef file = NBFile_alloc(NULL);
	//Open
	if(!NBFilesystem_open(fs, fullfilepath, ENNBFileMode_Read, file)){
		if(optDstErrStr != NULL){
			NBString_set(optDstErrStr, "Could not open index file");
		}
		r = FALSE;
	} else {
		NBFile_lock(file);
		//Header: HV... ((H=header, V=version ...)
		if(r && (dstHead != NULL || payloadSz > 0)){
			BYTE start[2];
            NBASSERT(NBFile_curPos(file) == 0)
			if(NBFile_read(file, start, sizeof(start)) != sizeof(start)){
				if(optDstErrStr != NULL){
					NBString_set(optDstErrStr, "Could not read index start-bytes");
				}
				r = FALSE;
			} else if(start[0] != TS_STREAM_FILE_IDX_HEADER){
				if(optDstErrStr != NULL){
                    NBString_set(optDstErrStr, "Index first-byte-header missmatch (");
                    NBString_concatSI32(optDstErrStr, start[0]);
                    NBString_concat(optDstErrStr, " vs ");
                    NBString_concatSI32(optDstErrStr, TS_STREAM_FILE_IDX_HEADER);
                    NBString_concat(optDstErrStr, ")");
				}
				r = FALSE; //NBASSERT(FALSE) //debugging only
			} else if(!NBFile_seek(file, -((SI32)sizeof(start)), ENNBFileRelative_CurPos)){
				if(optDstErrStr != NULL){
					NBString_set(optDstErrStr, "Could not rel-reset index file pos");
				}
				r = FALSE; //NBASSERT(FALSE) //debugging only
			} else {
				const BYTE ver = start[1];
				//Read header
				if(r && dstHead != NULL){
					if(ver == 0){
						STTSStreamFilesHeader head; //v=0
						if(NBString_strLenBytes(filterRunId) != sizeof(head.runId)){
							if(optDstErrStr != NULL){
								NBString_set(optDstErrStr, "RunId size missmatch");
							}
							r = FALSE; //NBASSERT(FALSE) //debugging only
						} else if(!NBFile_seek(file, 0, ENNBFileRelative_Start)){
							if(optDstErrStr != NULL){
								NBString_set(optDstErrStr, "Could not reset index file pos");
							}
							r = FALSE; //NBASSERT(FALSE) //debugging only
						} else if(NBFile_read(file, &head, sizeof(head)) != sizeof(head)){
							if(optDstErrStr != NULL){
								NBString_set(optDstErrStr, "Could not read index-header-struct");
							}
							r = FALSE; //NBASSERT(FALSE) //debugging only
						} else if(head.head != TS_STREAM_FILE_IDX_HEADER || head.version != ver || head.foot != TS_STREAM_FILE_IDX_FOOTER){
							if(optDstErrStr != NULL){
								NBString_set(optDstErrStr, "Index header/footer missmatch (");
                                NBString_concatSI32(optDstErrStr, head.head);
                                NBString_concat(optDstErrStr, " vs ");
                                NBString_concatSI32(optDstErrStr, TS_STREAM_FILE_IDX_HEADER);
                                NBString_concat(optDstErrStr, ") (");
                                NBString_concatSI32(optDstErrStr, head.version);
                                NBString_concat(optDstErrStr, " vs ");
                                NBString_concatSI32(optDstErrStr, ver);
                                NBString_concat(optDstErrStr, ") (");
                                NBString_concatSI32(optDstErrStr, head.foot);
                                NBString_concat(optDstErrStr, " vs ");
                                NBString_concatSI32(optDstErrStr, TS_STREAM_FILE_IDX_FOOTER);
                                NBString_concat(optDstErrStr, ")");
							}
							r = FALSE; //NBASSERT(FALSE) //debugging only
						} else if(!NBString_strIsEqualStrBytes(filterRunId, head.runId, sizeof(head.runId))){
							if(optDstErrStr != NULL){
								NBString_set(optDstErrStr, "Index from different runId");
							}
							r = FALSE; //NBASSERT(FALSE) //debugging only
						} else {
							*dstHead = head;
							fileStart = sizeof(head);
							fileCurPos += sizeof(head);
							NBASSERT(fileStart == fileCurPos)
							NBASSERT(fileStart == NBFile_curPos(file))
						}
					}
				} else {
					//Unxepected version
					if(optDstErrStr != NULL){
						NBString_set(optDstErrStr, "Unsupported index version");
					}
					r = FALSE;
				}
                //Read records
                if(r && payloadSz > 0){
                    if(ver == 0){
                        //Load records
                        BOOL continueReading = TRUE;
                        BYTE buff[4096]; BYTE* idx = NULL;
                        SI32 i, readItms = 0, readRemain = 0;
                        UI32 tsRel = 0, tsRelLast = 0, pos = 0, size = 0, payloadPos = 0; UI8 nalType = 0;
                        SI32 readBytes = 0;
                        STTSStreamFileIdx idxTmp;
                        NBMemory_setZeroSt(idxTmp, STTSStreamFileIdx);
                        while(r && continueReading && (readBytes = NBFile_read(file, buff, sizeof(buff))) > 0){
                            idx         = buff;
                            readItms    = readBytes / TS_STREAM_FILE_IDX_0_RECORDS_SZ;
                            readRemain  = readBytes % TS_STREAM_FILE_IDX_0_RECORDS_SZ;
                            //revert incomplete-record read
                            if(readRemain > 0){
                                if(!NBFile_seek(file, -((SI64)readRemain), ENNBFileRelative_CurPos)){
                                    if(optDstErrStr != NULL){
                                        NBString_set(optDstErrStr, "Could not rel-reset index file pos, after incomplete-idx-record read");
                                    }
                                    NBASSERT((fileCurPos + (readItms * TS_STREAM_FILE_IDX_0_RECORDS_SZ)) == NBFile_curPos(file))
                                    r = FALSE; //NBASSERT(FALSE) //debugging only
                                    break;
                                }
                            }
                            //process records
                            for(i = 0; i < readItms; i++){
                                //HVTR-RRRP-PPPS-SSSF (H=header, V=version,  T=nalType, R=relTimestampRtp, P=filePos, S=nalSize, F=footer)
                                //0123-4567-8901-2345
                                if(idx[0] != TS_STREAM_FILE_IDX_HEADER || idx[1] != TS_STREAM_FILE_IDX_VERSION || idx[15] != TS_STREAM_FILE_IDX_FOOTER){
                                    if(optDstErrStr != NULL){
                                        NBString_set(optDstErrStr, "Index record header/version/footer missmatch (");
                                        NBString_concatSI32(optDstErrStr, idx[0]);
                                        NBString_concat(optDstErrStr, " vs ");
                                        NBString_concatSI32(optDstErrStr, TS_STREAM_FILE_IDX_HEADER);
                                        NBString_concat(optDstErrStr, ") (");
                                        NBString_concatSI32(optDstErrStr, idx[1]);
                                        NBString_concat(optDstErrStr, " vs ");
                                        NBString_concatSI32(optDstErrStr, TS_STREAM_FILE_IDX_VERSION);
                                        NBString_concat(optDstErrStr, ") (");
                                        NBString_concatSI32(optDstErrStr, idx[15]);
                                        NBString_concat(optDstErrStr, " vs ");
                                        NBString_concatSI32(optDstErrStr, TS_STREAM_FILE_IDX_FOOTER);
                                        NBString_concat(optDstErrStr, ")");
                                    }
                                    r = FALSE; //NBASSERT(FALSE) //debugging only
                                    break;
                                } else {
                                    nalType = idx[2];
                                    tsRel   = ((UI32)idx[3] << 24) | ((UI32)idx[4] << 16) | ((UI32)idx[5] << 8) | idx[6];
                                    pos     = ((UI32)idx[7] << 24) | ((UI32)idx[8] << 16) | ((UI32)idx[9] << 8) | idx[10];
                                    size    = ((UI32)idx[11] << 24) | ((UI32)idx[12] << 16) | ((UI32)idx[13] << 8) | idx[14]; //size: max allowed size is 24-bits (~16MBs)
                                    //Validate
                                    if(
                                       (tsRel < tsRelLast)                //rel-time must move forward
                                       || (pos < payloadPos)            //indexes must move forward
                                       || (mustBeSequential && pos != payloadPos) //must be sequential
                                       || (size > 0xFFFFFF)                //size must fit into UI16
                                       || ((pos + size) < pos)            //overflows UI32
                                       || (pos >= payloadSz)            //beyond payload
                                       || ((pos + size) > payloadSz)    //beyond payload
                                       )
                                    {
                                        if(optDstErrStr != NULL){
                                            NBString_set(optDstErrStr, "Invalid index record after ");
                                            NBString_concatUI32(optDstErrStr, (UI32)(((UI64)payloadPos * (UI64)100) / (UI64)payloadSz));
                                            NBString_set(optDstErrStr, " percent of pay-pos");
                                        }
                                        PRINTF_WARNING("TSStreamStorageFiles, truncating indexes-list, %d%% of payload mapped, %d%% skipped.\n", (UI32)(((UI64)payloadPos * (UI64)100) / (UI64)payloadSz), (UI32)(((UI64)(payloadSz - payloadPos) * (UI64)100) / (UI64)payloadSz));
                                        //return position to precious record's end
                                        if(!NBFile_seek(file, (i - readItms) * TS_STREAM_FILE_IDX_0_RECORDS_SZ, ENNBFileRelative_CurPos)){
                                            if(optDstErrStr != NULL){
                                                NBString_set(optDstErrStr, "Could not rel-reset index file pos, after truncating to last valid record found");
                                            }
                                            NBASSERT(fileCurPos == NBFile_curPos(file))
                                            r = FALSE; //NBASSERT(FALSE) //debugging only
                                        }
                                        continueReading = FALSE;
                                        break;
                                    } else if(dstIdxs != NULL){
                                        idxTmp.tsRtpRel    = tsRel;
                                        idxTmp.pos        = pos;
                                        idxTmp.nalTypeAndsize = ((UI32)nalType << 24) | (size & 0xFFFFFF);
                                        NBArray_addValue(dstIdxs, idxTmp);
                                    }
                                    idx           += TS_STREAM_FILE_IDX_0_RECORDS_SZ;
                                    fileCurPos    += TS_STREAM_FILE_IDX_0_RECORDS_SZ;
                                    payloadPos    = pos + size;
                                    tsRelLast    = tsRel;
                                    NBASSERT(idx <= (buff + sizeof(buff)))
                                }
                            }
                        }
                        NBASSERT(!r || fileCurPos == NBFile_curPos(file))
                    } else {
                        //Unxepected version
                        if(optDstErrStr != NULL){
                            NBString_set(optDstErrStr, "Unsupported index version");
                        }
                        r = FALSE;
                    }
                }
			}
		}
        NBASSERT(!r || fileCurPos == NBFile_curPos(file))
		NBFile_unlock(file);
	}
	if(dstFileStart != NULL){
		*dstFileStart = fileStart;
	}
	if(dstFilePos != NULL){
		*dstFilePos = fileCurPos;
	}
	//Release (if not consumed)
	if(NBFile_isSet(file)){
		NBFile_release(&file);
        NBFile_null(&file);
	}
	return r;
}

//ToDo: return filenames used.
BOOL TSStreamStorageFiles_loadFiles(STTSStreamStorageFiles* obj, const char* filterRunId, const BOOL forceLoadAll, BOOL* dstIsEmptyPayload, STTSStreamsStorageStatsUpd* dstUpd, STNBArray* optDstFilenamesUsed /*STNBString*/, STNBString* optDstErrStr){
    BOOL r = FALSE, isEmptyPayload = FALSE;
    const SI32 optDstFilenamesUsedSzBefore = (optDstFilenamesUsed != NULL ? optDstFilenamesUsed->use : 0);
	STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque;
	STTSStreamsStorageStatsUpd upd;
	NBMemory_setZeroSt(upd, STTSStreamsStorageStatsUpd);
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)	//should not be called in parallel to feed thread
	if(!(opq->cfg.fs != NULL && !NBFile_isSet(opq->payload.file) && opq->cfg.filepathBase.length > 0)){
		if(optDstErrStr != NULL){
			NBString_set(optDstErrStr, "User params error");
		}
	} else {
		UI32 payStart = 0, payloadSz = 0, idxStart = 0, idxSz = 0, gapStart = 0, gapSz = 0;
		STNBString strPath; UI64 fillStartTime = 0; UI32 fillTsRtp = 0;
		STNBString payPath, idxPath, gapPath;
		STNBString nameNoExt, payName, idxName, gapName;
		//
		NBString_init(&strPath);
		//File paths
		NBString_init(&payPath);
		NBString_init(&idxPath);
		NBString_init(&gapPath);
		//File names
		NBString_init(&nameNoExt);
		NBString_init(&payName);
		NBString_init(&idxName);
		NBString_init(&gapName);
		NBASSERT(!NBFile_isSet(opq->payload.file))
		NBASSERT(opq->payload.path.length == 0)
		NBASSERT(!NBFile_isSet(opq->index.file))
		NBASSERT(opq->index.path.length == 0)
		NBASSERT(opq->index.arr.use == 0 && opq->index.hdr.count == opq->index.arr.use)
		NBASSERT(!NBFile_isSet(opq->gaps.file))
		NBASSERT(opq->gaps.path.length == 0)
		NBASSERT(opq->gaps.arr.use == 0 && opq->gaps.hdr.count == opq->gaps.arr.use)
		{
			SI32 lstSlash = NBString_strLastIndexOf(opq->cfg.filepathBase.str, "/", opq->cfg.filepathBase.length);
			if(lstSlash < 0){
				lstSlash = NBString_strLastIndexOf(opq->cfg.filepathBase.str, "\\", opq->cfg.filepathBase.length);
				if(lstSlash < 0){
					lstSlash = -1;
				}
			}
			NBString_set(&nameNoExt, &opq->cfg.filepathBase.str[lstSlash + 1]);
		}
		r = TRUE;
		//Load payload
		if(r){
			//Path
			NBString_set(&strPath, opq->cfg.filepathBase.str);
			NBString_concat(&strPath, ".264");
			//Name
			NBString_set(&payName, nameNoExt.str);
			NBString_concat(&payName, ".264");
			{
                STNBFileRef file = NBFile_alloc(NULL);
				//Open
				if(!NBFilesystem_open(opq->cfg.fs, strPath.str, ENNBFileMode_Read, file)){
					if(optDstErrStr != NULL){
						NBString_set(optDstErrStr, "Could not open payload file");
					}
					r = FALSE;
				} else {
					NBFile_lock(file);
					{
						if(!NBFile_seek(file, 0, ENNBFileRelative_End)){
							if(optDstErrStr != NULL){
								NBString_set(optDstErrStr, "Could not move to end-of-payload-file");
							}
							r = FALSE;
						} else {
							const SI64 sz = NBFile_curPos(file);
							if(sz < 0){
								if(optDstErrStr != NULL){
									NBString_set(optDstErrStr, "Could not read payload-file-size");
								}
								r = FALSE;
							} else if(sz == 0){
								if(optDstErrStr != NULL){
									NBString_set(optDstErrStr, "Payload-file is empty");
								}
								isEmptyPayload = TRUE;
								r = FALSE;
							} else {
								NBString_set(&payPath, strPath.str);
								payloadSz = (UI32)sz;
                                //add filename
                                if(optDstFilenamesUsed != NULL){
                                    STNBString str;
                                    NBString_initWithStr(&str, payName.str);
                                    NBArray_addValue(optDstFilenamesUsed, str);
                                }
							}
						}
					}
					NBFile_unlock(file);
					//Stats
					upd.files.payloads.added.count++;
					upd.files.payloads.added.bytesCount += payloadSz;
				}
				//Release (if not consumed)
				if(NBFile_isSet(file)){
					NBFile_release(&file);
                    NBFile_null(&file);
				}
			}
		}
		//Load idx
		if(r){
			const BOOL mustBeSequential = FALSE;
			UI32 fileStart = 0, filePos = 0;
			STNBArray arrIdx;
			NBArray_init(&arrIdx, sizeof(STTSStreamFileIdx), NULL);
			{
				NBString_set(&strPath, opq->cfg.filepathBase.str);
				NBString_concat(&strPath, ".idx");
				//
				NBString_set(&idxName, nameNoExt.str);
				NBString_concat(&idxName, ".idx");
			}
			{
				STTSStreamFilesHeader head;
				NBMemory_setZeroSt(head, STTSStreamFilesHeader);
				if(!TSStreamStorageFiles_loadFileIdx_(opq->cfg.fs, strPath.str, payloadSz, filterRunId, &head, &arrIdx, &fileStart, &filePos, optDstErrStr, mustBeSequential, forceLoadAll)){
					r = FALSE;
				} else {
                    //empty
                    {
                        NBArray_truncateBuffSize(&opq->index.arr, 0);
                        opq->index.hdr.count = 0;
                    }
                    //copy
					if(arrIdx.use > 0){
						STTSStreamFileIdx* data = NBArray_dataPtr(&arrIdx, STTSStreamFileIdx);
                        //add
                        {
                            //keep records
                            if(opq->index.rdrsCount > 0){
                                NBArray_addItems(&opq->index.arr, data, sizeof(STTSStreamFileIdx), arrIdx.use);
                            }
                            //stats
                            upd.files.indexes.added.count++;
                            upd.files.indexes.added.bytesCount += filePos;
                            upd.files.indexes.added.recordsCount += arrIdx.use;
                        }
                        //hdr
                        opq->index.hdr.count    = arrIdx.use;
                        opq->index.hdr.first    = data[0];
                        opq->index.hdr.last     = data[arrIdx.use - 1];
					}
					NBString_set(&idxPath, strPath.str);
					fillStartTime	= head.fillStartTime;
					fillTsRtp		= head.fillTsRtp;
					idxStart		= fileStart;
					idxSz			= filePos;
                    //add filename
                    if(optDstFilenamesUsed != NULL){
                        STNBString str;
                        NBString_initWithStr(&str, idxName.str);
                        NBArray_addValue(optDstFilenamesUsed, str);
                    }
				}
			}
			NBArray_release(&arrIdx);
		}
		//Load gaps (optional)
		if(r){
			const BOOL mustBeSequential = FALSE;
			UI32 fileStart = 0, filePos = 0;
			STNBArray arrIdx;
			NBArray_init(&arrIdx, sizeof(STTSStreamFileIdx), NULL);
			{
				NBString_set(&strPath, opq->cfg.filepathBase.str);
				NBString_concat(&strPath, ".gap");
				//
				NBString_set(&gapName, nameNoExt.str);
				NBString_concat(&gapName, ".gap");
			}
			if(!TSStreamStorageFiles_loadFileIdx_(opq->cfg.fs, strPath.str, payloadSz, filterRunId, NULL, &arrIdx, &fileStart, &filePos, NULL, mustBeSequential, forceLoadAll)){
				//nothing, is optional
			} else {
                //empty
                {
                    NBArray_truncateBuffSize(&opq->gaps.arr, 0);
                    opq->gaps.hdr.count = 0;
                }
                //copy
				if(arrIdx.use > 0){
					STTSStreamFileIdx* data = NBArray_dataPtr(&arrIdx, STTSStreamFileIdx);
                    //add
                    {
                        if(opq->gaps.rdrsCount > 0){
                            NBArray_addItems(&opq->gaps.arr, data, sizeof(STTSStreamFileIdx), arrIdx.use);
                        }
                        //stats
                        upd.files.gaps.added.count++;
                        upd.files.gaps.added.bytesCount += filePos;
                        upd.files.gaps.added.recordsCount += arrIdx.use;
                    }
                    //hdr
                    opq->gaps.hdr.count    = arrIdx.use;
                    opq->gaps.hdr.first    = data[0];
                    opq->gaps.hdr.last     = data[arrIdx.use - 1];
				}
				NBString_set(&gapPath, strPath.str);
				gapStart = fileStart;
				gapSz = filePos;
                //add filename
                if(optDstFilenamesUsed != NULL){
                    STNBString str;
                    NBString_initWithStr(&str, gapName.str);
                    NBArray_addValue(optDstFilenamesUsed, str);
                }
			}
			//PRINTF_INFO("TSStreamStorageFiles, loaded %u KBs payload, %u KBs idx, %u KB gaps.\n", (payloadSz / 1024), (idxSz / 1024), (gapSz / 1024));
			NBArray_release(&arrIdx);
		}
		//Results
		if(r){
			//
			NBString_set(&opq->payload.path, payPath.str);
			opq->payload.fileStart	= payStart;
			opq->payload.filePos	= payloadSz;
			//
			NBString_set(&opq->index.path, idxPath.str);
			opq->index.fillStartTime	= fillStartTime; NBASSERT(fillStartTime > 0)
			opq->index.fillTsRtp		= fillTsRtp;
			opq->index.fillTsRtpLast	= fillTsRtp; 
			opq->index.fillTsRtpRelLast = 0;
			opq->index.fileStart		= idxStart;
			opq->index.filePos			= idxSz;
			//
			if(opq->index.hdr.count > 0){
				opq->index.fillTsRtpLast	+= opq->index.hdr.last.tsRtpRel;
				opq->index.fillTsRtpRelLast	= opq->index.hdr.last.tsRtpRel;
			}
			//
			NBString_set(&opq->gaps.path, gapPath.str);
			opq->gaps.fileStart	= gapStart;
			opq->gaps.filePos	= gapSz;
            //
            opq->stats.upd = upd;
		}
		NBString_release(&payName);
		NBString_release(&idxName);
		NBString_release(&gapName);
		NBString_release(&payPath);
		NBString_release(&idxPath);
		NBString_release(&gapPath);
		NBString_release(&nameNoExt);
		NBString_release(&strPath);
	}
	NBThreadMutex_unlock(&opq->mutex);
    //revert filenames added
    if(!r && optDstFilenamesUsed != NULL){
        while(optDstFilenamesUsed->use > optDstFilenamesUsedSzBefore){
            STNBString* str = NBArray_itmPtrAtIndex(optDstFilenamesUsed, STNBString, optDstFilenamesUsed->use - 1);
            NBString_release(str);
            NBArray_removeItemAtIndex(optDstFilenamesUsed, optDstFilenamesUsed->use - 1);
        }
    }
	//Results
	{
        if(dstIsEmptyPayload != NULL){
            *dstIsEmptyPayload = isEmptyPayload;
        }
        if(dstUpd != NULL){
            *dstUpd = upd;
        }
	}
	return r;
}

//moves a previous upd applied from 'TSStreamStorageFiles_loadFiles' from 'payload' to 'others'.
void TSStreamStorageFiles_getStatsUpdAsOthers(STTSStreamStorageFiles* obj, STTSStreamsStorageStatsUpd* dstUpd){
    STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque;
    NBThreadMutex_lock(&opq->mutex);
    NBASSERT(!opq->dbg.inUnsafeAction)    //should not be called in parallel to feed thread
    if(dstUpd != NULL){
        //revert previously added
        {
            dstUpd->files.payloads.removed.count += opq->stats.upd.files.payloads.added.count;
            dstUpd->files.payloads.removed.bytesCount += opq->stats.upd.files.payloads.added.bytesCount;
            //
            dstUpd->files.gaps.removed.count += opq->stats.upd.files.gaps.added.count;
            dstUpd->files.gaps.removed.bytesCount += opq->stats.upd.files.gaps.added.bytesCount;
            dstUpd->files.gaps.removed.recordsCount += opq->stats.upd.files.gaps.added.recordsCount;
            //
            dstUpd->files.gaps.removed.count += opq->stats.upd.files.gaps.added.count;
            dstUpd->files.gaps.removed.bytesCount += opq->stats.upd.files.gaps.added.bytesCount;
            dstUpd->files.gaps.removed.recordsCount += opq->stats.upd.files.gaps.added.recordsCount;
        }
        //reasign as 'others'
        dstUpd->files.others.added.count += opq->stats.upd.files.payloads.added.count;
        dstUpd->files.others.added.bytesCount += opq->stats.upd.files.payloads.added.bytesCount;
        //
        dstUpd->files.others.added.count += opq->stats.upd.files.gaps.added.count;
        dstUpd->files.others.added.bytesCount += opq->stats.upd.files.gaps.added.bytesCount;
        //
        dstUpd->files.others.added.count += opq->stats.upd.files.gaps.added.count;
        dstUpd->files.others.added.bytesCount += opq->stats.upd.files.gaps.added.bytesCount;
    }
    NBThreadMutex_unlock(&opq->mutex);
}

BOOL TSStreamStorageFiles_createFiles(STTSStreamStorageFiles* obj, const char* runId){
	BOOL r = FALSE;
	STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)	//should not be called in parallel to feed thread
	if(opq->cfg.fs != NULL && !NBFile_isSet(opq->payload.file) && opq->cfg.filepathBase.length > 0){
		NBASSERT(!NBFile_isSet(opq->payload.file))
		NBASSERT(opq->payload.path.length == 0)
		NBASSERT(!NBFile_isSet(opq->index.file))
		NBASSERT(opq->index.path.length == 0)
		NBASSERT(opq->index.arr.use == 0 && opq->index.hdr.count == opq->index.arr.use)
		NBASSERT(!NBFile_isSet(opq->gaps.file))
		NBASSERT(opq->gaps.path.length == 0)
		NBASSERT(opq->gaps.arr.use == 0 && opq->gaps.hdr.count == opq->gaps.arr.use)
		STNBFileRef	newFiles[3]         = { NB_OBJREF_NULL, NB_OBJREF_NULL, NB_OBJREF_NULL };
		STNBFileRef* dstFiles[3]        = { &opq->payload.file, &opq->index.file, &opq->gaps.file };
		STNBString* paths[3]            = { &opq->payload.path, &opq->index.path, &opq->gaps.path };
		IF_NBASSERT(UI32* fStarts[3]    = { &opq->payload.fileStart, &opq->index.fileStart, &opq->gaps.fileStart };)
		IF_NBASSERT(UI32* fPoss[3]      = { &opq->payload.filePos, &opq->index.filePos, &opq->gaps.filePos };)
		const char* exts[3]             = { "264", "idx", "gap" };
		r = TRUE;
		//Create files
		if(r){
			STNBString strPath;
			NBString_init(&strPath);
			{
				const SI32 count = (sizeof(newFiles) / sizeof(newFiles[0]));
				SI32 i; for(i = 0; i < count && r; i++){
					NBString_set(&strPath, opq->cfg.filepathBase.str);
					NBString_concatByte(&strPath, '.');
					NBString_concat(&strPath, exts[i]);
					{
                        STNBFileRef file = NBFile_alloc(NULL);
						//Open
						if(!NBFilesystem_open(opq->cfg.fs, strPath.str, ENNBFileMode_Write, file)){
							r = FALSE;
						} else {
							NBASSERT(*fStarts[i] == 0)
							NBASSERT(*fPoss[i] == 0)
							NBASSERT(!NBFile_isSet(newFiles[i]))
							//Consume
							NBString_set(paths[i], strPath.str);
							newFiles[i] = file;
                            NBFile_null(&file); //consume
						}
						//Release (if not consumed)
						if(NBFile_isSet(file)){
							NBFile_release(&file);
							NBFile_null(&file);
						}
					}
				}
			}
			NBString_release(&strPath);
		}
		//Apply files
		if(r){
			const SI32 count = (sizeof(newFiles) / sizeof(newFiles[0]));
			SI32 i; for(i = 0; i < count; i++){
				STNBFileRef* dstFile = dstFiles[i];
				if(NBFile_isSet(*dstFile)){
					NBFile_release(dstFile);
                    NBFile_null(dstFile);
				}
				*dstFiles[i] = newFiles[i];
                NBFile_null(&newFiles[i]);
			}
		}
		//Release (if not consumed)
		{
            STNBString strPath;
            NBString_init(&strPath);
            {
                const SI32 count = (sizeof(newFiles) / sizeof(newFiles[0]));
                SI32 i; for(i = 0; i < count; i++){
                    STNBFileRef file = newFiles[i];
                    if(NBFile_isSet(file)){
                        NBFile_release(&file);
                        NBFile_null(&file);
                        NBFile_null(&newFiles[i]);
                        //delete created file
                        NBString_set(&strPath, opq->cfg.filepathBase.str);
                        NBString_concatByte(&strPath, '.');
                        NBString_concat(&strPath, exts[i]);
                        if(!NBFilesystem_deleteFile(opq->cfg.fs, strPath.str)){
                            //error
                        }
                    }
                }
            }
            NBString_release(&strPath);
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

BOOL TSStreamStorageFiles_closeFiles(STTSStreamStorageFiles* obj, const BOOL deleteAnyIfEmpty, STTSStreamsStorageStatsUpd* dstUpd, STTSStreamUnitsReleaser* optUnitsReleaser){
	BOOL r = FALSE;
	STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)	//should not be called in parallel to feed thread
	if(opq->cfg.fs != NULL){
		STNBFileRef* files[3]		= { &opq->payload.file, &opq->index.file, &opq->gaps.file };
		STNBString* paths[3]		= { &opq->payload.path, &opq->index.path, &opq->gaps.path };
		STNBArray*  fileRecs[3]		= { NULL, &opq->index.arr, &opq->gaps.arr };
		UI32*		fileStarts[3]	= { &opq->payload.fileStart, &opq->index.fileStart, &opq->gaps.fileStart };
		UI32*		filePoss[3]		= { &opq->payload.filePos, &opq->index.filePos, &opq->gaps.filePos };
		UI32*		updCounts[3]	= { NULL, NULL, NULL };
		UI64*		updBytesCounts[3]	= { NULL, NULL, NULL };
		UI32*		updRecordsCounts[3]	= { NULL, NULL, NULL };
        SI32*       updRecRdrsCounts[3] = { NULL, NULL, NULL };
		if(dstUpd != NULL){
			//
			updCounts[0] = &dstUpd->files.payloads.added.count;
			updCounts[1] = &dstUpd->files.indexes.added.count;
			updCounts[2] = &dstUpd->files.gaps.added.count;
			//
			updBytesCounts[0] = &dstUpd->files.payloads.added.bytesCount;
			updBytesCounts[1] = &dstUpd->files.indexes.added.bytesCount;
			updBytesCounts[2] = &dstUpd->files.gaps.added.bytesCount;
			//
			updRecordsCounts[1] = &dstUpd->files.indexes.added.recordsCount;
			updRecordsCounts[2] = &dstUpd->files.gaps.added.recordsCount;
            //
            updRecRdrsCounts[1] = &opq->index.rdrsCount;
            updRecRdrsCounts[2] = &opq->gaps.rdrsCount;
		}
		r = TRUE;
		//Flush files
		{
			TSStreamStorageFiles_writeBuffsOpqUnsafe_(opq, optUnitsReleaser);
		}
		//Close files
		{
			const SI32 count = (sizeof(files) / sizeof(files[0]));
			SI32 i; for(i = 0; i < count && r; i++){
				BOOL removed = FALSE;
				STNBFileRef* file = files[i];
				STNBArray* recs = fileRecs[i];
				NBASSERT(*fileStarts[i] <= *filePoss[i])
				//close
				if(file != NULL && NBFile_isSet(*file)){
					NBFile_close(*file);
					NBFile_release(file);
                    NBFile_null(file);
                    files[i] = NULL;
				}
				if(deleteAnyIfEmpty){
					if(*filePoss[i] <= 0 && paths[i]->length > 0){
						NBFilesystem_deleteFile(opq->cfg.fs, paths[i]->str);
						//reset
						NBString_empty(paths[i]);
						*fileStarts[i] = 0;
						*filePoss[i] = 0;
						removed = TRUE;
					}
				}
				//apply stats
				if(!removed && dstUpd != NULL){
                    if(updRecRdrsCounts[i] != NULL && updRecRdrsCounts[i] == 0 && recs != NULL){
                        NBArray_truncateBuffSize(recs, 0);
                    }
                    //stats
                    if(updCounts[i] != NULL){
                        *updCounts[i] = *updCounts[i] + 1;
                    }
                    if(updBytesCounts[i] != NULL){
                        *updBytesCounts[i] = *updBytesCounts[i] + *filePoss[i];
                    }
                    if(updRecordsCounts[i] != NULL && recs != NULL){
                        *updRecordsCounts[i] = *updRecordsCounts[i] + recs->use;
                    }
				}
			}
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  r;
}

UI64 TSStreamStorageFiles_deleteFiles(STTSStreamStorageFiles* obj, STTSStreamsStorageStatsUpd* dstUpd){
	UI64 rr = 0;
	STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque; 
	NBThreadMutex_lock(&opq->mutex);
	NBASSERT(!opq->dbg.inUnsafeAction)	//should not be called in parallel to feed thread
	if(opq->cfg.fs != NULL){
		STNBFileRef* files[3]		= { &opq->payload.file, &opq->index.file, &opq->gaps.file };
		STNBString* paths[3]		= { &opq->payload.path, &opq->index.path, &opq->gaps.path };
		STNBArray* fileRecs[3]		= { NULL, &opq->index.arr, &opq->gaps.arr };
		UI32*		fileStarts[3]	= { &opq->payload.fileStart, &opq->index.fileStart, &opq->gaps.fileStart };
		UI32*		filePoss[3]		= { &opq->payload.filePos, &opq->index.filePos, &opq->gaps.filePos };
		UI32*		updCounts[3]	= { NULL, NULL, NULL };
		UI64*		updBytesCounts[3]	= { NULL, NULL, NULL };
		UI32*		updRecordsCounts[3]	= { NULL, NULL, NULL };
		//stats
		if(dstUpd != NULL){
			//
			updCounts[0] = &dstUpd->files.payloads.removed.count;
			updCounts[1] = &dstUpd->files.indexes.removed.count;
			updCounts[2] = &dstUpd->files.gaps.removed.count;
			//
			updBytesCounts[0] = &dstUpd->files.payloads.removed.bytesCount;
			updBytesCounts[1] = &dstUpd->files.indexes.removed.bytesCount;
			updBytesCounts[2] = &dstUpd->files.gaps.removed.bytesCount;
			//
			updRecordsCounts[0] = NULL;
			updRecordsCounts[1] = &dstUpd->files.indexes.removed.recordsCount;
			updRecordsCounts[2] = &dstUpd->files.gaps.removed.recordsCount;
		}
		//Delete files
		{
			const SI32 count = (sizeof(files) / sizeof(files[0]));
			SI32 i; for(i = 0; i < count; i++){
				STNBFileRef* file = files[i];
				STNBArray* recs = fileRecs[i];
				NBASSERT(*fileStarts[i] <= *filePoss[i])
				//close
				if(file != NULL && NBFile_isSet(*file)){
                    NBFile_close(*file);
					NBFile_release(file);
                    NBFile_null(file);
                    files[i] = NULL;
				}
				//delete
				if(paths[i]->length > 0){
					if(NBFilesystem_deleteFile(opq->cfg.fs, paths[i]->str)){
						//PRINTF_INFO("TSStreamStorageFiles, file removed: '%s'.\n", paths[i]->str);
						rr += *filePoss[i];
						//Stats
						if(updCounts[i] != NULL){
							*updCounts[i] = *updCounts[i] + 1;
						}
						if(updBytesCounts[i] != NULL){
							*updBytesCounts[i] = *updBytesCounts[i] + (*filePoss[i]);
						}
						//empty recs
						if(recs != NULL){
							//Stats
							if(updRecordsCounts[i] != NULL){
								*updRecordsCounts[i] = *updRecordsCounts[i] + recs->use;
							}
                            NBArray_truncateBuffSize(recs, 0);
						}
						NBString_empty(paths[i]);
						*fileStarts[i] = 0;
						*filePoss[i] = 0;
					}
				}
			}
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return  rr;
}

static const BYTE _nalPrefix[4] = { 0x00, 0x00, 0x00, 0x01 };

BOOL TSStreamStorageFiles_writeBuffsOpqUnsafe_(STTSStreamStorageFilesOpq* opq, STTSStreamUnitsReleaser* optUnitsReleaser){
	BOOL errFnd = FALSE;
	STNBArray* payPend = &opq->payload.buff;
	STNBString* idxBuff = &opq->index.buff;
	//write payload data
	if(payPend->use != 0){
		if(NBFile_isSet(opq->payload.file)){
			NBFile_lock(opq->payload.file);
			{
				UI32 nalSz;
				STTSStreamUnit* u = NBArray_dataPtr(payPend, STTSStreamUnit);
				const STTSStreamUnit* uAfterEnd = u + payPend->use;
				while(u < uAfterEnd && !errFnd){
					nalSz = sizeof(_nalPrefix) + u->desc->hdr.prefixSz;
					//write NALU separator
					if(!errFnd && !opq->cfg.params.files.debug.write.simulateOnly && NBFile_write(opq->payload.file, _nalPrefix, sizeof(_nalPrefix)) != sizeof(_nalPrefix)){
						PRINTF_CONSOLE_ERROR("TSStreamStorageFiles, could not write payload header.\n");
						errFnd = TRUE;
					}
					//write unit prefix
					if(!errFnd && u->desc->hdr.prefixSz > 0 && !opq->cfg.params.files.debug.write.simulateOnly && NBFile_write(opq->payload.file, u->desc->hdr.prefix, u->desc->hdr.prefixSz) != u->desc->hdr.prefixSz){
						PRINTF_CONSOLE_ERROR("TSStreamStorageFiles, could not write payload prefix.\n");
						errFnd = TRUE;
					}
					//write payload
					if(!errFnd && u->desc->chunks != NULL && u->desc->chunksSz > 0){
						STNBDataPtr* chunk = u->desc->chunks; 
						const STNBDataPtr* chunkAfterEnd = chunk + u->desc->chunksSz;
						while(!errFnd && chunk < chunkAfterEnd){
							NBASSERT(chunk->ptr != NULL && chunk->use > 0)
							if(!errFnd && !opq->cfg.params.files.debug.write.simulateOnly && NBFile_write(opq->payload.file, chunk->ptr, chunk->use) != chunk->use){
								PRINTF_CONSOLE_ERROR("TSStreamStorageFiles, could not write payload.\n");
								errFnd = TRUE;
							}
							nalSz += chunk->use;
							//Next
							chunk++;
						}
					}
					NBASSERT(errFnd || nalSz == (sizeof(_nalPrefix) + u->desc->hdr.prefixSz + u->desc->chunksTotalSz))
					//next
					u++;
				}
				//flush
				if(!errFnd && opq->cfg.params.files.debug.write.atomicFlush){
					//flush payload
					NBFile_flush(opq->payload.file);
				}
				NBASSERT(opq->cfg.params.files.debug.write.simulateOnly || NBFile_curPos(opq->payload.file) == opq->payload.filePos)
			}
			NBFile_unlock(opq->payload.file);
		}
		//empty units
		{
			STTSStreamUnit* u = NBArray_dataPtr(payPend, STTSStreamUnit);
			TSStreamUnit_unitsReleaseGrouped(u, payPend->use, optUnitsReleaser);
			NBArray_empty(payPend);
		}
	}
	//write index records
	if(idxBuff->length != 0){
		if(NBFile_isSet(opq->index.file)){
			NBFile_lock(opq->index.file);
			{
				//write header if necesary
				if(opq->index.filePos == 0){
					STTSStreamFilesHeader head;
					if(opq->cfg.runId.length != sizeof(head.runId)){
						PRINTF_CONSOLE_ERROR("TSStreamStorageFiles, runId(%d bytes) should be %d bytes.\n", opq->cfg.runId.length, (SI32)sizeof(head.runId));
						errFnd = TRUE;
					} else {
						NBMemory_setZero(head);
						head.head			= TS_STREAM_FILE_IDX_HEADER;
						head.version		= TS_STREAM_FILE_IDX_HEAD_VERSION;
						NBMemory_copy(head.runId, opq->cfg.runId.str, sizeof(head.runId));
						head.fillStartTime	= opq->index.fillStartTime;
						head.fillTsRtp		= opq->index.fillTsRtp;
						head.foot			= TS_STREAM_FILE_IDX_FOOTER;
						if(!opq->cfg.params.files.debug.write.simulateOnly && NBFile_write(opq->index.file, &head, sizeof(head)) != sizeof(head)){
							PRINTF_CONSOLE_ERROR("TSStreamStorageFiles, could not write idx-header.\n");
							errFnd = TRUE;
						}
						//Init index
						if(!errFnd){
							opq->index.fileStart	+= sizeof(head);
							opq->index.filePos		+= sizeof(head);
							NBASSERT(opq->index.fileStart == sizeof(head))
							NBASSERT(opq->index.fileStart == opq->index.filePos)
							if(opq->cfg.params.files.debug.write.atomicFlush){
								NBFile_flush(opq->index.file);
							}
						}
					}
				}
				//write records
				if(!opq->cfg.params.files.debug.write.simulateOnly && NBFile_write(opq->index.file, idxBuff->str, idxBuff->length) != idxBuff->length){
					PRINTF_CONSOLE_ERROR("TSStreamStorageFiles, could not write idx-records.\n");
					errFnd = TRUE;
				}
				if(!errFnd){
					//flush
					if(opq->cfg.params.files.debug.write.atomicFlush){
						NBFile_flush(opq->index.file);
					}
					//move index pos
					NBASSERT(opq->index.fileStart <= opq->index.filePos)
					opq->index.filePos += idxBuff->length;
					NBASSERT(opq->cfg.params.files.debug.write.simulateOnly || NBFile_curPos(opq->index.file) == opq->index.filePos)
				}
			}
			NBFile_unlock(opq->index.file);
		}
		NBString_empty(idxBuff);
	}
	return (!errFnd);
}
	
UI32 TSStreamStorageFiles_appendBlocksUnsafe(STTSStreamStorageFiles* obj, STTSStreamUnit* units, const UI32 unitsSz, STTSStreamUnitsReleaser* optUnitsReleaser, STTSStreamFilesPos* dstCurPos, BOOL* dstStreamChanged){
	UI32 r = 0; BOOL streamChanged = FALSE;
	STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque;
	NBASSERT(!opq->dbg.inUnsafeAction)
	IF_NBASSERT(opq->dbg.inUnsafeAction = TRUE;)
	//Validate cur pos
	NBASSERT(
			 dstCurPos == NULL
			 || (
				dstCurPos->payload == opq->payload.filePos
				 && dstCurPos->index == opq->index.filePos
				 && dstCurPos->gaps == opq->gaps.filePos
				 && dstCurPos->fillStartTime == opq->index.fillStartTime
				 && dstCurPos->fillTsRtp == opq->index.fillTsRtp
				 && dstCurPos->fillTimeRtp == opq->index.fillTsRtpRelLast
				 )
			 )
	if(NBFile_isSet(opq->payload.file) && NBFile_isSet(opq->index.file) && NBFile_isSet(opq->gaps.file) && units != NULL && unitsSz > 0){
		STTSStreamFileIdx idxTmp;
		NBMemory_setZeroSt(idxTmp, STTSStreamFileIdx);
		//Process units
		{
			STTSStreamUnit* unit = units;
			const STTSStreamUnit* unitAfterEnd = unit + unitsSz;
			BOOL errFnd = FALSE, nalWrite = FALSE, flushBuffs = FALSE;
			UI32 tsRtp = 0, tsRtpRel = 0, nalPos = 0, nalSz = 0;
			BOOL startTsRtpSet = (opq->index.filePos != 0);
			STNBArray* payPend = &opq->payload.buff;
			STNBString* idxBuff = &opq->index.buff;
			//concat payload and index
			{
				while(unit < unitAfterEnd){
					nalWrite = flushBuffs = FALSE;
					//Validate nalSeq
					{
						if(unit->desc->isBlockStart){
							opq->nalSeq.isBlockStarted = nalWrite = flushBuffs = TRUE;
							opq->nalSeq.isKeyFrameFound = FALSE;
						} else if(!opq->nalSeq.isBlockStarted){
							PRINTF_INFO("TSStreamStorageFiles, stream ignoring nal (waiting for blockStart).\n");
						} else if(unit->desc->isFramePayload){
							NBASSERT(opq->nalSeq.isBlockStarted)
							if(unit->desc->isKeyframe){
								opq->nalSeq.isKeyFrameFound = nalWrite = TRUE;
							} else if(!opq->nalSeq.isKeyFrameFound){
								PRINTF_INFO("TSStreamStorageFiles, stream ignoring non-keyframe (waiting for keyFrame).\n");
							} else {
								nalWrite = TRUE;
							}
						} else {
							nalWrite = TRUE;
						}
					}
					//flush buffers
					if(flushBuffs){
						TSStreamStorageFiles_writeBuffsOpqUnsafe_(opq, optUnitsReleaser);
					}
					//concat NAL
					if(!nalWrite){
						PRINTF_INFO("TSStreamStorageFiles, stream ignoring nal (waiting for parameters and IDR-pic).\n");
					} else {
						//Validate timestamp
						const UI32 tsRtpLast = (startTsRtpSet ? opq->index.fillTsRtpLast : unit->desc->hdr.tsRtp);
						const UI32 tsRtpRelLast	= (startTsRtpSet ? opq->index.fillTsRtpRelLast : 0);
						if(unit->desc->hdr.tsRtp <= tsRtpLast){
							//timestamp is past in time, repeat current tsRtpRel
							tsRtpRel = tsRtpRelLast;
						} else {
							//timestamp is forward, advance tsRtpRel
							tsRtpRel = tsRtpRelLast + (unit->desc->hdr.tsRtp - tsRtpLast);
							if(tsRtpRel < tsRtpRelLast){
								//32-bits timestamp overflowed
								PRINTF_INFO("TSStreamStorageFiles, stream timestamp overflowed (~18h limit in 32 bits), opening new stream-file.\n");
								streamChanged = TRUE;
								errFnd = TRUE;
								break; //stop cicle
							}
						}
						//Header
						{
							NBASSERT(nalSz == 0)
							NBASSERT(unit->desc->hdr.type > 0)
							tsRtp	= unit->desc->hdr.tsRtp;
							nalPos	= opq->payload.filePos;
							//nals
							if(!startTsRtpSet){
								opq->index.fillStartTime	= NBDatetime_getCurUTCTimestamp();
								opq->index.fillTsRtp		= tsRtp;
								opq->index.fillTsRtpLast	= tsRtp;
								opq->index.fillTsRtpRelLast = 0;
								startTsRtpSet = TRUE;
							}
						}
						//Add unit
						{
							STTSStreamUnit u2;
							TSStreamUnit_init(&u2);
							TSStreamUnit_setAsOther(&u2, unit);
							NBArray_addValue(payPend, u2);
							nalSz = sizeof(_nalPrefix) + u2.desc->hdr.prefixSz + u2.desc->chunksTotalSz;
						}
						//Write index
						{
							NBASSERT(startTsRtpSet)
							NBASSERT(opq->index.fillTsRtpRelLast <= tsRtpRel) //this should be validated at the start of nalu
							opq->index.fillTsRtpLast	= tsRtp;
							opq->index.fillTsRtpRelLast	= tsRtpRel;
							//add idx to arr
							if(unit->desc->hdr.type != 1 || opq->cfg.params.files.index.allNALU){ //Coded slice of a non-IDR picture
								//write buff
								{
									//HVTP-PPPS-SSSF (H=header, V=version, T=nalType, P=filePos, S=nalSize, F=footer)
									NBString_concatByte(idxBuff, TS_STREAM_FILE_IDX_HEADER);
									NBString_concatByte(idxBuff, TS_STREAM_FILE_IDX_VERSION);
									NBString_concatByte(idxBuff, unit->desc->hdr.type);
									NBString_concatByte(idxBuff, ((tsRtpRel >> 24) & 0xFF));
									//
									NBString_concatByte(idxBuff, ((tsRtpRel >> 16) & 0xFF));
									NBString_concatByte(idxBuff, ((tsRtpRel >> 8) & 0xFF));
									NBString_concatByte(idxBuff, (tsRtpRel & 0xFF));
									NBString_concatByte(idxBuff, ((nalPos >> 24) & 0xFF));
									//
									NBString_concatByte(idxBuff, ((nalPos >> 16) & 0xFF));
									NBString_concatByte(idxBuff, ((nalPos >> 8) & 0xFF));
									NBString_concatByte(idxBuff, (nalPos & 0xFF));
									NBString_concatByte(idxBuff, ((nalSz >> 24) & 0xFF));
									//
									NBString_concatByte(idxBuff, ((nalSz >> 16) & 0xFF));
									NBString_concatByte(idxBuff, ((nalSz >> 8) & 0xFF));
									NBString_concatByte(idxBuff, (nalSz & 0xFF));
									NBString_concatByte(idxBuff, TS_STREAM_FILE_IDX_FOOTER);
									//
									NBASSERT((idxBuff->length % TS_STREAM_FILE_IDX_0_RECORDS_SZ) == 0)
								}
								//array
								{
									NBASSERT(nalSz <= 0xFFFFFF) //Max nalSz supported is ~16MB (24 bits)
									idxTmp.tsRtpRel	= tsRtpRel;
									idxTmp.pos		= nalPos;
									idxTmp.nalTypeAndsize = ((UI32)(unit->desc->hdr.type & 0xFF) << 24) | (nalSz & 0xFFFFFF);
                                    if(opq->index.rdrsCount > 0){
                                        //keep record
                                        NBArray_addValue(&opq->index.arr, idxTmp);
                                    }
                                    //hdr
                                    {
                                        if(opq->index.hdr.count == 0){
                                            opq->index.hdr.first = idxTmp;
                                        }
                                        opq->index.hdr.last = idxTmp;
                                        opq->index.hdr.count++;
                                    }
								}
							}
							NBASSERT(opq->payload.fileStart <= opq->payload.filePos)
							opq->payload.filePos += nalSz;
							//
							nalSz = 0;
						}
					}
					//next
					unit++;
				} //while
				//Sync partial write
				opq->payload.filePos += nalSz;
			}
			//write-atomically
			if(!opq->cfg.params.files.buffKeyframes){
				TSStreamStorageFiles_writeBuffsOpqUnsafe_(opq, optUnitsReleaser);
			}
			//result
			r = (UI32)(unit - units);
		}
	}
	if(dstCurPos != NULL){
		TSStreamStorageFiles_getFilesPosLockedOpq_(opq, dstCurPos);
	}
	if(dstStreamChanged != NULL){
		*dstStreamChanged = streamChanged;
	}
	NBASSERT(opq->dbg.inUnsafeAction)
	IF_NBASSERT(opq->dbg.inUnsafeAction = FALSE;)
	return r;
}

BOOL TSStreamStorageFiles_appendGapsUnsafe(STTSStreamStorageFiles* obj, STTSStreamUnit* units, const UI32 unitsSz, UI32* dstPayloadCurPos){
	BOOL r = FALSE;
	STTSStreamStorageFilesOpq* opq = (STTSStreamStorageFilesOpq*)obj->opaque;
	NBASSERT(!opq->dbg.inUnsafeAction)
	IF_NBASSERT(opq->dbg.inUnsafeAction = TRUE;)
	if(NBFile_isSet(opq->gaps.file)){
		//
		if(dstPayloadCurPos != NULL){
			*dstPayloadCurPos = opq->payload.filePos;
		}
	}
	NBASSERT(opq->dbg.inUnsafeAction)
	IF_NBASSERT(opq->dbg.inUnsafeAction = FALSE;)
	return  r;
}
