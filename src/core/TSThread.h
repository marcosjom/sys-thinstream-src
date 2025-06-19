//
//  TSThread.h
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#ifndef TSThread_h
#define TSThread_h

#include "nb/NBFrameworkDefs.h"
#include "core/config/TSCfgThreads.h"
//
#include "nb/core/NBIOPollster.h"
#include "core/TSContext.h"
#include "core/TSThreadStats.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSThread_ {
		void*		opaque;
	} STTSThread;
	
	void TSThread_init(STTSThread* obj);
	void TSThread_release(STTSThread* obj);

	//cfg
	BOOL TSThread_prepare(STTSThread* obj, STNBArray* tickLstnrs /*STTSContextLstnr*/, const UI32 threadIdx, const STTSCfgThreads* cfg);
	
	//pollster
	STNBIOPollsterRef TSThread_getPollster(STTSThread* obj);
	STNBIOPollsterSyncRef TSThread_getPollsterSync(STTSThread* obj);

	//thread
	BOOL TSThread_execute(STTSThread* obj);		//execute in calling thread
	BOOL TSThread_start(STTSThread* obj);		//start new thread
	BOOL TSThread_isRunning(STTSThread* obj);
	BOOL TSThread_isCurrentThread(STTSThread* obj);	//returns TRUE if we are calling this method inside the thread routine.
	void TSThread_stopFlag(STTSThread* obj);
	void TSThread_stop(STTSThread* obj);
	//Commands
	BOOL TSThread_statsFlushStart(STTSThread* obj, const UI32 iSeq);
	BOOL TSThread_statsFlushIsPend(STTSThread* obj, const UI32 iSeq);
	void TSThread_statsGet(STTSThread* obj, STTSThreadStatsData* dst, const BOOL resetAccum);
	
#ifdef __cplusplus
} //extern "C"
#endif
	
#endif /* TSThread_h */
