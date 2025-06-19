//
//  TSStreamDefs.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSStreamDefs_h
#define TSStreamDefs_h

#include "nb/core/NBDataPtr.h"
#include "nb/core/NBStructMap.h"
#include "nb/core/NBLog.h"
#include "nb/core/NBRange.h"
#include "core/logic/TSStreamUnitBuff.h"

#ifdef __cplusplus
extern "C" {
#endif
	
//Active mode

typedef enum ENTSCfgStreamReadMode_ {
	ENTSCfgStreamReadMode_Allways = 0, 	//stream is allways requested
	ENTSCfgStreamReadMode_WhenViewing,	//stream is requested only when at least one viewer is active
	ENTSCfgStreamReadMode_Never			//stream is disable (never requested)
} ENTSCfgStreamReadMode;

const STNBEnumMap* TSCfgStreamReadMode_getSharedEnumMap(void);

//Active mode

typedef enum ENTSCfgStreamReadSource_ {
	ENTSCfgStreamReadSource_MemoryBuffer = 0,	//packets in memory are sent (packets can be omited if write is to slow)
	ENTSCfgStreamReadSource_StorageBuffer,		//packets saved in storage are sent (this is used to sync remote backups of streams)
} ENTSCfgStreamReadSource;

const STNBEnumMap* TSCfgStreamReadSource_getSharedEnumMap(void);

//Write mode

typedef enum ENTSCfgStreamWriteMode_ {
	ENTSCfgStreamWriteMode_Allways = 0,		//write allways
	ENTSCfgStreamWriteMode_MotionDetected,	//write when motion was detected only
	ENTSCfgStreamWriteMode_Never			//write is disable (never requested)
} ENTSCfgStreamWriteMode;

const STNBEnumMap* TSCfgStreamWriteMode_getSharedEnumMap(void);

//Stream params buffer units

typedef struct STTSStreamParamsBufferUnits_ {
	UI32	minAlloc;	//amount of threads to allocate at start
	UI32	maxKeep;	//amount of threads to keep max
} STTSStreamParamsBufferUnits;

const STNBStructMap* TSStreamParamsBufferUnits_getSharedStructMap(void);

//Stream params buffers

typedef struct STTSStreamParamsBuffers_ {
	STTSStreamParamsBufferUnits units;	//NAL-units in H.264
} STTSStreamParamsBuffers;

const STNBStructMap* TSStreamParamsBuffers_getSharedStructMap(void);

//Stream params RTP

typedef struct STTSStreamParamsRtp_ {
	char*		assumeConnDroppedAfter;		//time to asumme the UTP connection was dropped (time in text form)
	UI32		assumeConnDroppedAfterSecs;	//time to asumme the UTP connection was dropped (time in secs)
} STTSStreamParamsRtp;

const STNBStructMap* TSStreamParamsRtp_getSharedStructMap(void);

//Stream params RTSP ports

typedef struct STTSStreamParamsRtspPorts_ {
	UI16		rtp;
	UI16		rtcp;
} STTSStreamParamsRtspPorts;

const STNBStructMap* TSStreamParamsRtspPorts_getSharedStructMap(void);

//Stream params RTSP

typedef struct STTSStreamParamsRtsp_ {
	STTSStreamParamsRtspPorts	ports;
} STTSStreamParamsRtsp;

const STNBStructMap* TSStreamParamsRtsp_getSharedStructMap(void);

//Stream params read

typedef struct STTSStreamParamsRead_ {
	ENTSCfgStreamReadMode	mode;	//defines when the packages will be ignored or processed
	ENTSCfgStreamReadSource	source;	//defines the source of the packets
} STTSStreamParamsRead;

const STNBStructMap* TSStreamParamsRead_getSharedStructMap(void);


//Stream params debug

typedef struct STTSStreamParamsFileDebugWrite_ {
	BOOL		simulateOnly;			//avoid writting to storage (avoid hdd/sdd wear while testing)
	BOOL		atomicFlush;			//flush is called after each chunk/index write
} STTSStreamParamsFileDebugWrite;

const STNBStructMap* TSStreamParamsFileDebugWrite_getSharedStructMap(void);

//Stream params debug

typedef struct STTSStreamParamsFileDebug_ {
	STTSStreamParamsFileDebugWrite	write;	//writting debug config
} STTSStreamParamsFileDebug;

const STNBStructMap* TSStreamParamsFileDebug_getSharedStructMap(void);

//Stream params files

typedef struct STTSStreamParamsFileIndex_ {
	BOOL		allNALU;	//includes all NALUnits
} STTSStreamParamsFileIndex;

const STNBStructMap* TSStreamParamsFileIndex_getSharedStructMap(void);

//Stream params files

typedef struct STTSStreamParamsFilePayload_ {
	char*		limit;				//per-file, string version, like "123", "123B" (bytes), "123b" (bits), "123K", "123KB", "123M", "123MB" (Mega-Bytes), "123Mb" (Mega-bits), "123T", "123TB", "123Tb"
	UI64		limitBytes;			//per-file, numeric version
	char*		divider;			//per-file, string version, like "60s", "60secs", "90m", "90min", "90min", "1d", "1day"
	UI32		dividerSecs;		//per-file, numeric version
} STTSStreamParamsFilePayload;

const STNBStructMap* TSStreamParamsFilePayload_getSharedStructMap(void);

//Stream params files

typedef struct STTSStreamParamsFiles_ {
	UI8								nameDigits;	//digits for filenames
	BOOL							buffKeyframes;	//accum in ram units grouped by keyframes; when FALSE units are written as they arrive.
	STTSStreamParamsFileIndex	 	index;		//index-file params
	STTSStreamParamsFilePayload		payload;	//payload-file params
	STTSStreamParamsFileDebug		debug;		//debug params
} STTSStreamParamsFiles;

const STNBStructMap* TSStreamParamsFiles_getSharedStructMap(void);

//Stream params write

typedef struct STTSStreamParamsWrite_ {
	ENTSCfgStreamWriteMode	mode;		//defines when the packages will be written or ignored
	STTSStreamParamsFiles	files;		//files config
} STTSStreamParamsWrite;

const STNBStructMap* TSStreamParamsWrite_getSharedStructMap(void);


//Stream params

typedef struct STTSStreamParams_ {
	STTSStreamParamsRead	read;		//read params
	STTSStreamParamsWrite	write;		//write params
	STTSStreamParamsBuffers	buffers;	//buffers
	STTSStreamParamsRtsp	rtsp;		//rtsp stream params
	STTSStreamParamsRtp		rtp;		//rtp stream params
} STTSStreamParams;

const STNBStructMap* TSStreamParams_getSharedStructMap(void);

//Stream merger params

typedef struct STTSStreamsMergerParams_ {
    UI32*         secsWaitAllFmtsMax;  //max seconds to wait for all stream formats to be available, if NULL use default value in code.
} STTSStreamsMergerParams;

const STNBStructMap* TSStreamsMergerParams_getSharedStructMap(void);

//Stream address

typedef struct STTSStreamAddress_ {
	char*		server;		//server
	SI32		port;		//port
	BOOL		useSSL;		//ssl
	char*		uri;		//uri
	char*		user;		//username
	char*		pass;		//password
} STTSStreamAddress;

const STNBStructMap* TSStreamAddress_getSharedStructMap(void);

BOOL TSStreamAddress_isEqual(const STTSStreamAddress* obj, const STTSStreamAddress* other);

//stream listener

typedef struct STTSStreamLstnr_ {
	void*	param;
	UI32	(*streamConsumeUnits)(void* param, STTSStreamUnit* units, const UI32 unitsSz, STTSStreamUnitsReleaser* optUnitsReleaser);
	void	(*streamEnded)(void* param);
} STTSStreamLstnr;

#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSStreamDefs_h */
