//
//  TSServerStats.h
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#ifndef TSServerStats_h
#define TSServerStats_h

#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"
#include "nb/net/NBHttpStats.h"
#include "core/TSContext.h"
#include "logic/TSClientRequesterConnStats.h"   //for STTSClientRequesterConnStatsState
#include "logic/TSStreamsStorageStats.h"        //for STTSStreamsStorageStatsState
#ifdef __cplusplus
extern "C" {
#endif
	
	//
	//Root data
	//
	
	typedef struct STTSServerStats_ {
		UI64			timeStart;
		UI64			timeEnd;
		SI32			starts;
		SI32			shutdowns;
		STNBHttpStatsData stats;
	} STTSServerStats;
	
	const STNBStructMap* TSServerStats_getSharedStructMap(void);
	
	//Tools
	BOOL TSServerStats_loadCurrentAsServiceStart(STTSContext* context, STTSServerStats* dst);
	BOOL TSServerStats_saveCurrentAsServiceShutdown(STTSContext* context, STTSServerStats* srcAndDst);
	BOOL TSServerStats_saveCurrentStats(STTSContext* context, STTSServerStats* srcAndDst);
	BOOL TSServerStats_saveAndOpenNewStats(STTSContext* context, STTSServerStats* srcAndDst);

    //TSServerStatsThreadState

#   ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
    typedef struct STTSServerStatsThreadState_ {
        const char*                 name;
        const char*                 firstKnownMethod;
        ENNBLogLevel                statsLevel;
        STNBMngrProcessStatsState   stats;
    } STTSServerStatsThreadState;
#   endif

    const STNBStructMap* TSServerStatsThreadState_getSharedStructMap(void);

#   ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
    void TSServerStatsThreadState_init(STTSServerStatsThreadState* obj);
    void TSServerStatsThreadState_release(STTSServerStatsThreadState* obj);
#   endif

    //TSServerStatsProcessState

#   ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
    typedef struct STTSServerStatsProcessState_ {
        STNBMngrProcessStatsState       stats;
        STTSServerStatsThreadState*     threads;
        UI32                            threadsSz;
    } STTSServerStatsProcessState;
#   endif

    const STNBStructMap* TSServerStatsProcessState_getSharedStructMap(void);

#   ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
    void TSServerStatsProcessState_init(STTSServerStatsProcessState* obj);
    void TSServerStatsProcessState_release(STTSServerStatsProcessState* obj);
#   endif

    // TSServerStatsState

    typedef struct STTSServerStatsState_ {
        STNBDataPtrsStatsState              buffs;
        STNBRtspClientStatsState            rtsp;
        STTSClientRequesterConnStatsState   reqs;
        STTSStreamsStorageStatsState        storage;
#       ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
        STTSServerStatsProcessState         process;
#       endif
    } STTSServerStatsState;

    const STNBStructMap* TSServerStatsState_getSharedStructMap(void);

    //ENTSServerStartsGrp

    typedef enum ENTSServerStartsGrp_ {
        ENTSServerStartsGrp_Loaded = 0,    //currently loaded content
        ENTSServerStartsGrp_Accum,        //activitity since last period (ex: each second)
        ENTSServerStartsGrp_Total,        //activity since start
        //
        ENTSServerStartsGrp_Count
    } ENTSServerStartsGrp;

    //TSServerStatsData

    typedef struct STTSServerStatsData_ {
        STTSServerStatsState    loaded;        //loaded
        STTSServerStatsState    accum;        //aacum
        STTSServerStatsState    total;        //total
    } STTSServerStatsData;

    const STNBStructMap* TSServerStatsData_getSharedStructMap(void);

    void TSServerStatsData_init(STTSServerStatsData* obj);
    void TSServerStatsData_release(STTSServerStatsData* obj);

#ifdef __cplusplus
} //extern "C" {
#endif

#endif /* TSServerStats_h */
