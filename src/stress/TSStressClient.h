

#ifndef TSStressClient_h
/*
#define TSStressClient_h

#include "nb/core/NBMemory.h"
#include "nb/core/NBThread.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
//Core
#include "core/TSContext.h"
#include "config/TSCfg.h"
//Client
#include "client/TSClient.h"
#ifdef __cplusplus
extern "C" {
#endif

	//Stats
	
	typedef struct STTSStressClientStatsData_ {
		UI32			sigupsCount;	//signups (processor heavy due to digital signature)
		NBTHREAD_CLOCK	sigupsClocks;	//signups (processor heavy due to digital signature)
		UI32			seensCount;		//fileSeen (light, just set/update one record)
		NBTHREAD_CLOCK	seensClocks;	//fileSeen (light, just set/update one record)
		UI32			sleepingCount;	//current clients just creating an artificial wait time
		UI32			syncsCount;		//current clients with sync request live
		UI32			reconnCount;	//force reconnection (to test server stability)
	} STTSStressClientStatsData;
	
	typedef struct STTSStressClientStats_ {
		struct {
			UI32					expected;		//Amount of threads expected
			UI32					running;		//Amount of threads working
			UI32					cltIniting;		//Amount of threads initing the client library (init only one thread at the time)
			UI32					cltInited;		//Amount of threads inited the client library
			UI32					withCSR;		//Amount of threads created their CSR
			UI32					withIdentity;	//Amount of threads created their identity
			UI32					joined;			//Amount of threads joined the meeting
		} threads;
		STTSStressClientStatsData	alive;		//Currently alive
		STTSStressClientStatsData	accum;		//Accumulated per second
		STTSStressClientStatsData	total;		//Global stats
	} STTSStressClientStats;
	
	//Client
	
	typedef struct STTSStressClientClient_ {
		STTSCfg*				cfg;
		STTSContext*		context;
		STTSClient*				client;
		STTSSyncClient*			sync;
		//
		STNBThreadMutex			mutex;
		STNBThreadCond			cond;
		//Session data
		struct {
			void*				thread;		//STTSStressClientThread
			STNBString			emailHash;	//tmp random-hash created for this client
			STNBString			phoneHash;	//tmp random-hash created for this client
			BOOL				joined;		//joined the meeting
		} data;
	} STTSStressClientClient;
	
	//Run
	
	typedef struct STTSStressClientRun_ {
		BOOL					stopFlag;
		BOOL					isMainThreadRunning;	//The thread per second
		STTSStressClientStats	stats;
		STNBArray				clients;	//STTSStressClientClient*
		//Meeting between all threads
		struct {
			BOOL				creating;
			STNBString			meetingId;
		} meeting;
		//
		STNBThreadMutex			mutex;
		STNBThreadCond			cond;
	} STTSStressClientRun;
	
	void TSStressClientRun_init(STTSStressClientRun* obj);
	void TSStressClientRun_release(STTSStressClientRun* obj);
	
	void TSStressClientRun_setStopFlag(STTSStressClientRun* obj);
	void TSStressClientRun_waitForAll(STTSStressClientRun* obj);
	BOOL TSStressClientRun_start(STTSStressClientRun* obj, const UI32 ammThreads);
	
#ifdef __cplusplus
} //extern "C"
#endif
*/

#endif /* TSStressClient_h */
