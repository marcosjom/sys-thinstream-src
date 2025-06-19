//
//  TSCfgTelemetry.h
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#ifndef TSCfgTelemetry_h
#define TSCfgTelemetry_h

#include "nb/core/NBLog.h"
#include "nb/core/NBStructMap.h"
#include "nb/net/NBRtspClientStats.h"

#ifdef __cplusplus
extern "C" {
#endif

	//Filesystem

	typedef struct STTSCfgTelemetryFilesystem_ {
		ENNBLogLevel	statsLevel;
	} STTSCfgTelemetryFilesystem;

	const STNBStructMap* TSCfgTelemetryFilesystem_getSharedStructMap(void);

	//Streams

	typedef struct STTSCfgTelemetryStreams_ {
		ENNBLogLevel		statsLevel;
		BOOL				perPortDetails;
		BOOL				perStreamDetails;
		STNBRtspStatsCfg	rtsp;
	} STTSCfgTelemetryStreams;

	const STNBStructMap* TSCfgTelemetryStreams_getSharedStructMap(void);

	//Thread

	typedef struct STTSCfgTelemetryThread_ {
		char*			name;
		char*			firstKnownFunc;
		ENNBLogLevel	statsLevel;
		BOOL			locksByMethod;
	} STTSCfgTelemetryThread;

	const STNBStructMap* TSCfgTelemetryThread_getSharedStructMap(void);

	//Process

	typedef struct STTSCfgTelemetryProcess_ {
		ENNBLogLevel			statsLevel;
		BOOL					locksByMethod;
		STTSCfgTelemetryThread*	threads;
		UI32					threadsSz;
	} STTSCfgTelemetryProcess;

	const STNBStructMap* TSCfgTelemetryProcess_getSharedStructMap(void);

	//Stats

	typedef struct STTSCfgTelemetry_ {
		STTSCfgTelemetryFilesystem	filesystem;
		STNBDataPtrsStatsCfg		buffers;
		STTSCfgTelemetryStreams		streams;
		STTSCfgTelemetryProcess		process;
	} STTSCfgTelemetry;
	
	const STNBStructMap* TSCfgTelemetry_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgTelemetry_h */
