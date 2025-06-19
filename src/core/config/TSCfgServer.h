//
//  TSCfgServers.h
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#ifndef TSCfgServer_h
#define TSCfgServer_h

#include "nb/core/NBStructMap.h"
#include "nb/core/NBDataPtrsPool.h"
#include "nb/core/NBThreadsPool.h"
#include "nb/net/NBRtspClient.h"
#include "core/config/TSCfgOutServer.h"
#include "core/config/TSCfgStream.h"
#include "core/config/TSCfgTelemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

	//Server defaults

	typedef struct STTSCfgServerDefaultsStreams_ {
		STTSStreamParams	params;		//params
        STTSStreamsMergerParams merger;
	} STTSCfgServerDefaultsStreams;

	const STNBStructMap* TSCfgServerDefaultsStreams_getSharedStructMap(void);

	//Server defaults

	typedef struct STTSCfgServerDefaults_ {
		STTSCfgServerDefaultsStreams streams; //streams defaults
	} STTSCfgServerDefaults;

	const STNBStructMap* TSCfgServerDefaults_getSharedStructMap(void);

	//Server synchronizations

	typedef struct STTSCfgServerSync_ {
		UI32				secsToFirstAction;		//secs to first sync after starting the service
		UI32				secsRetryAfterError;	//secs to run sync after error sync
		UI32				secsBetweenAction;		//secs to run sync after success sync
		STTSCfgOutServer	server;					//API server
		char*				authToken;				//token used for authentication
	} STTSCfgServerSync;

	const STNBStructMap* TSCfgServerSync_getSharedStructMap(void);

	//Server system reports

	typedef struct STTSCfgServerSystemReports_ {
		UI32		secsToSave;		//secs to analyze all meetings after last analysis
		UI32		minsToNext;		//minutes to analyze all meetings after last analysis
		char**		emails;
		UI32		emailsSz;
	} STTSCfgServerSystemReports;

	const STNBStructMap* TSCfgServerSystemReports_getSharedStructMap(void);

	//Server
	
	typedef struct STTSCfgServer_ {
		STTSCfgServerDefaults			defaults;
		STTSCfgTelemetry				telemetry;
		STTSCfgServerSystemReports		systemReports;
		STTSCfgServerSync				syncClt;
		STNBDataPtrsPoolCfg				buffers;			//pointers-pool (pre-allocated)
		STNBRtspClientCfg				rtsp;				//RTSP client (including buffers, RTP and RTCP servers)
		STTSCfgStreamsGrp*				streamsGrps;		//static streams
		UI32							streamsGrpsSz;		//static streams
	} STTSCfgServer;
	
	const STNBStructMap* TSCfgServer_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgServers_h */
