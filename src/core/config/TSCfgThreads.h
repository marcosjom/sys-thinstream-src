//
//  TSCfg.h
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#ifndef TSCfgThreads_h
#define TSCfgThreads_h

#include "nb/core/NBStructMap.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//ThreadPollster

	typedef struct STTSCfgThreadPolling_ {
		UI32		timeout;	//poll ms timeout
	} STTSCfgThreadPolling;

	const STNBStructMap* TSCfgThreadPolling_getSharedStructMap(void);

	//Threads

	typedef struct STTSCfgThreads_ {
		UI32					amount;		//amount of threads
		STTSCfgThreadPolling	polling;	//polling
	} STTSCfgThreads;
	
	const STNBStructMap* TSCfgThreads_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgThread_h */
