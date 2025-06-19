//
//  TSThread.c
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/TSThread.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBThread.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBThreadStorage.h"
#include "nb/core/NBStruct.h"
#include "core/TSContext.h"

//TSThreadStorage

typedef struct STTSThreadStorage_ {
	BOOL	dummy;
} STTSThreadStorage;

void TSThreadStorage_init(STTSThreadStorage* obj);
void TSThreadStorage_release(STTSThreadStorage* obj);

//TSThread

typedef struct STTSThreadOpq_ {
	STNBThreadMutex		mutex;
	STNBThreadCond		cond;
	BOOL				stopFlag;
	BOOL				isPrepared;
	BOOL				isRunning;
	//cfg
	struct {
		UI32			idx;	//thread-index
		STTSCfgThreads	data;	//cfg
	} cfg;
	//time
	struct {
		STNBTimestampMicro micro;
	} time;
	//thread
	struct {
		//current
		struct {
			const STTSThreadStorage* vars;	//per-thread variable (allow to identify if calling method is running on a thread)
		} current;
	} thread;
	//pollster
	struct {
		STNBIOPollsterRef		ref;	//(unsafe) pollster; for static loaded sockets/files
		STNBIOPollsterSyncRef	sync;	//(safe) sync commands queue; for dynamic loaded sockets/files
	} pollster;
	//lsntrs
	struct {
		STNBArray* arr;	//STTSContextLstnr
	} lstnrs;
	//stats
	struct {
		STTSThreadStatsRef	provider; //stats
		STTSThreadStatsUpd	upd;	//pending update
		//flush
		struct {
			UI32		iSeqReq;		//flush sequence requested
			UI32		iSeqDone;		//flush sequence done
		} flush;
	} stats;
} STTSThreadOpq;

//

#define TS_THREAD_HAS_PEND_TASK_FLUSH_STATS(PTR)	((PTR)->stats.flush.iSeqDone != (PTR)->stats.flush.iSeqReq)
#define TS_THREAD_HAS_PEND_TASK(PTR)				TS_THREAD_HAS_PEND_TASK_FLUSH_STATS(PTR)

void TSThread_consumePendTaskStatsFlushLockedOpq_(STTSThreadOpq* opq);
void TSThread_consumePendTasksLockedOpq_(STTSThreadOpq* opq);

//poll lstnr

void TSThread_pollReturnedUnsafe_(STNBIOPollsterRef ref, const SI32 amm, void* usrData);

//TSThreadStorage (per-thread var)

static STNBThreadStorage __tsThreadStorage = NBThreadStorage_initialValue;
void __tsThreadStorageCreateMthd(void);
void __tsThreadStorageDestroyDataMthd(void* data);

void __tsThreadStorageCreateMthd(void){
	//PRINTF_INFO("__tsThreadStorageCreateMthd.\n");
	NBThreadStorage_create(&__tsThreadStorage);
	NBASSERT(NBThreadStorage_getData(&__tsThreadStorage) == NULL)
}

void __tsThreadStorageDestroyDataMthd(void* data){
	//PRINTF_INFO("__tsThreadStorageDestroyDataMthd.\n");
	if(data != NULL){
		STTSThreadStorage* st = (STTSThreadStorage*)data;
		TSThreadStorage_release(st);
		NBMemory_free(st);
		st = NULL;
	}
}


//

void TSThread_init(STTSThread* obj){
	STTSThreadOpq* opq = obj->opaque = NBMemory_allocType(STTSThreadOpq);
	NBMemory_setZeroSt(*opq, STTSThreadOpq);
	//
	NBThreadMutex_init(&opq->mutex);
	NBThreadCond_init(&opq->cond);
	//time
	{
		opq->time.micro		= NBTimestampMicro_getMonotonicFast();
	}
	//pollster
	{
		opq->pollster.ref	= NBIOPollster_alloc(NULL);
		opq->pollster.sync	= NBIOPollsterSync_alloc(NULL);
	}
	//stats
	{
		opq->stats.provider = TSThreadStats_alloc(NULL);
		TSThreadStatsUpd_init(&opq->stats.upd);
	}
}

void TSThread_release(STTSThread* obj){
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	{
		//wait for thread
		while(opq->isRunning){
			opq->stopFlag = TRUE;
			NBThreadCond_broadcast(&opq->cond);
			NBThreadCond_wait(&opq->cond, &opq->mutex);
		}
		//pollster
		{
			//ref
			if(NBIOPollster_isSet(opq->pollster.ref)){
				NBIOPollster_release(&opq->pollster.ref);
				NBIOPollster_null(&opq->pollster.ref);
			}
			//sync
			if(NBIOPollsterSync_isSet(opq->pollster.sync)){
				NBIOPollsterSync_release(&opq->pollster.sync);
				NBIOPollsterSync_null(&opq->pollster.sync);
			}
		}
		if(opq->lstnrs.arr != NULL){
			opq->lstnrs.arr = NULL;
		}
		//stats
		{
			if(TSThreadStats_isSet(opq->stats.provider)){
				TSThreadStats_release(&opq->stats.provider);
				TSThreadStats_null(&opq->stats.provider);
			}
			TSThreadStatsUpd_release(&opq->stats.upd);
		}
		//cfg
		{
			NBStruct_stRelease(TSCfgThreads_getSharedStructMap(), &opq->cfg.data, sizeof(opq->cfg.data));
		}
		NBThreadCond_release(&opq->cond);
	}
	NBThreadMutex_unlock(&opq->mutex);
	NBThreadMutex_release(&opq->mutex);
	NBMemory_free(opq);
	obj->opaque = NULL;
}

//cfg

BOOL TSThread_prepare(STTSThread* obj, STNBArray* tickLstnrs, const UI32 threadIdx, const STTSCfgThreads* cfg){
	BOOL r = FALSE;
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(!opq->isPrepared && !opq->isRunning && cfg != NULL){
		r = TRUE;
		//polling
		if(r){
			if(cfg->polling.timeout <= 0){
				PRINTF_CONSOLE_ERROR("TSThread #%d, 'polling/timeout' must be greater to zero.\n", threadIdx);
				r = FALSE;
			}
		}
		//lstnrs
		if(r){
			opq->lstnrs.arr = tickLstnrs;
		}
		//clone
		if(r){
			//clone cfg
			const STNBStructMap* cfgMap = TSCfgThreads_getSharedStructMap();
			NBStruct_stRelease(cfgMap, &opq->cfg.data, sizeof(opq->cfg.data));
			NBStruct_stClone(cfgMap, cfg, sizeof(*cfg), &opq->cfg.data, sizeof(opq->cfg.data));
			//
			opq->cfg.idx = threadIdx;
			opq->isPrepared = TRUE;
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

//pollster

STNBIOPollsterRef TSThread_getPollster(STTSThread* obj){
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	return opq->pollster.ref;
}

STNBIOPollsterSyncRef TSThread_getPollsterSync(STTSThread* obj){
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	return opq->pollster.sync;
}

//thread 

SI64 TSThread_executeAndReleaseThread_(STNBThread* thread, void* param);
void TSThread_executeLockedOpq_(STTSThreadOpq* opq);

BOOL TSThread_execute(STTSThread* obj){
	BOOL r = FALSE;
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(opq->isPrepared && !opq->isRunning){
		//execute
		opq->isRunning = TRUE;
		TSThread_executeLockedOpq_(opq);
		PRINTF_INFO("TSThread #%d returned.\n", opq->cfg.idx);
		NBASSERT(!opq->isRunning)
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSThread_start(STTSThread* obj){
	BOOL r = FALSE;
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(opq->isPrepared && !opq->isRunning){
		STNBThread* t = NBMemory_allocType(STNBThread);
		NBThread_init(t);
		NBThread_setIsJoinable(t, FALSE);
		opq->isRunning = TRUE;
		if(!NBThread_start(t, TSThread_executeAndReleaseThread_, opq, NULL)){
			opq->isRunning = FALSE;
		} else {
			//consume
			t = NULL;
			r = TRUE;
		}
		//release (if not consumed)
		if(t != NULL){
			NBThread_release(t);
			NBMemory_free(t);
			t = NULL;
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSThread_isRunning(STTSThread* obj){
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	return opq->isRunning;
}

BOOL TSThread_isCurrentThread(STTSThread* obj){	//returns TRUE if we are calling this method inside the thread routine.
	BOOL r = FALSE;
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	//No need for lock since we are using thread-specific variables
	{
		NBThreadStorage_initOnce(&__tsThreadStorage, __tsThreadStorageCreateMthd);
		{
			const STTSThreadStorage* vars = (STTSThreadStorage*)NBThreadStorage_getData(&__tsThreadStorage);
			r = (vars != NULL && opq->thread.current.vars == vars);
		}
	}
	return r;
}

void TSThread_stopFlag(STTSThread* obj){
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	{
		opq->stopFlag = TRUE;
		NBThreadCond_broadcast(&opq->cond);
	}
	NBThreadMutex_unlock(&opq->mutex);
}

void TSThread_stop(STTSThread* obj){
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	{
		//wait for thread
		while(opq->isRunning){
			opq->stopFlag = TRUE;
			NBThreadCond_broadcast(&opq->cond);
			NBThreadCond_wait(&opq->cond, &opq->mutex);
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
}

//Commands

void TSThread_consumePendTaskStatsFlushLockedOpq_(STTSThreadOpq* opq){
	if(TSThreadStats_isSet(opq->stats.provider)){
		TSThreadStats_applyUpdate(opq->stats.provider, &opq->stats.upd);
	}
	TSThreadStatsUpd_reset(&opq->stats.upd);
	//set as current
	opq->stats.flush.iSeqDone = opq->stats.flush.iSeqReq;
}

void TSThread_consumePendTasksLockedOpq_(STTSThreadOpq* opq){
	if(TS_THREAD_HAS_PEND_TASK_FLUSH_STATS(opq)){
		TSThread_consumePendTaskStatsFlushLockedOpq_(opq);
	}
}

//

SI64 TSThread_executeAndReleaseThread_(STNBThread* thread, void* param){
	SI64 r = 0;
	STTSThreadOpq* opq = (STTSThreadOpq*)param;
	//Execute
	{
		NBThreadMutex_lock(&opq->mutex);
		{
			TSThread_executeLockedOpq_(opq);
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	//release
	if(thread != NULL){
		NBThread_release(thread);
		NBMemory_free(thread);
		thread = NULL;
	}
	return r;
}

void TSThread_pollReturnedUnsafe_(STNBIOPollsterRef ref, const SI32 amm, void* usrData){
	STTSThreadOpq* opq = (STTSThreadOpq*)usrData;
	const STNBTimestampMicro nowMicro = NBTimestampMicro_getMonotonicFast();
	//Analyze pollTime
	{
		const SI64 usecs = NBTimestampMicro_getDiffInUs(&opq->time.micro, &nowMicro); //NBASSERT(usecs >= 0)
		if(usecs >= 0){
			TSThreadStatsUpd_addPollTime(&opq->stats.upd, usecs);
		}
	}
	//set
	opq->time.micro = nowMicro;
}

void TSThread_executeLockedOpq_(STTSThreadOpq* opq){
	//enable per-thread storage
	{
		NBThreadStorage_initOnce(&__tsThreadStorage, __tsThreadStorageCreateMthd);
		{
			STTSThreadStorage* vars = (STTSThreadStorage*)NBThreadStorage_getData(&__tsThreadStorage);
			if(vars == NULL){
				vars = NBMemory_allocType(STTSThreadStorage);
				TSThreadStorage_init(vars);
				NBThreadStorage_setData(&__tsThreadStorage, vars, __tsThreadStorageDestroyDataMthd);
			}
			opq->thread.current.vars = vars;
		}
	}
	//flags as running
	{
		NBASSERT(opq->isRunning)
		opq->isRunning = TRUE;
		NBThreadCond_broadcast(&opq->cond);
		PRINTF_INFO("TSThread #%d started.\n", opq->cfg.idx);
	}
	//time
	{
		opq->time.micro = NBTimestampMicro_getMonotonicFast();
	}
	//config pollster
	{
		//set poll listener
		if(opq->cfg.idx == 0){
			STNBIOPollsterLstnrItf itf;
			NBMemory_setZeroSt(itf, STNBIOPollsterLstnrItf);
			itf.pollReturned = TSThread_pollReturnedUnsafe_;
			NBIOPollster_setLstnr(opq->pollster.ref, &itf, opq);
		}
		//set unsafe mode (reduces locks)
		NBIOPollster_setUnsafeMode(opq->pollster.ref, TRUE);
	}
	if(!NBIOPollster_engineStart(opq->pollster.ref)){
		PRINTF_ERROR("TSThread #%d, NBIOPollster_engineStart failed.\n", opq->cfg.idx);
	} else {
		//cycle
		NBThreadMutex_unlock(&opq->mutex);
		{
			while(!opq->stopFlag){
				//poll (vClock will be updated by pollster-listener-callback)
				NBIOPollster_enginePoll(opq->pollster.ref, opq->cfg.data.polling.timeout, opq->pollster.sync);
				//lstnrs
				if(opq->lstnrs.arr != NULL && opq->lstnrs.arr->use != 0){
					SI32 i; for(i = (SI32)opq->lstnrs.arr->use - 1; i >= 0 && !opq->stopFlag; i--){
						STTSContextLstnr* lstnr = NBArray_itmPtrAtIndex(opq->lstnrs.arr, STTSContextLstnr, i);
						if(lstnr->itf.contextTick != NULL){
							(*lstnr->itf.contextTick)(opq->time.micro, lstnr->usrData);
						}
					}
				}
				//stats
				{
					//Analyze notifTime
					{
						const STNBTimestampMicro nowMicro = NBTimestampMicro_getMonotonicFast();
						const SI64 usecs = NBTimestampMicro_getDiffInUs(&opq->time.micro, &nowMicro); //NBASSERT(usecs >= 0)
						if(usecs >= 0){
							TSThreadStatsUpd_addNotifTime(&opq->stats.upd, usecs);
						}
						//set
						opq->time.micro = nowMicro;
					}
					//tick
					opq->stats.upd.ticks.count++;
					//flush stats
					if(TS_THREAD_HAS_PEND_TASK(opq)){
						TSThread_consumePendTasksLockedOpq_(opq);
						NBASSERT(!TS_THREAD_HAS_PEND_TASK(opq))
					}
					/*if(secChanged){
						STNBString str;
						NBString_init(&str);
						{
							TSThreadStats_applyUpdate(opq->stats.provider, &opq->stats.upd);
							TSThreadStatsUpd_reset(&opq->stats.upd);
							{
								const BOOL loaded = FALSE;
								const BOOL accum = TRUE;
								const BOOL total = TRUE;
								const STTSThreadStatsData data = TSThreadStats_getData(opq->stats.provider, TRUE);
								TSThreadStats_concatData(&data, ENNBLogLevel_Info, loaded, accum, total, "", "", TRUE, &str);
								PRINTF_INFO("TSThread #%d, stats: %s\n", opq->cfg.idx, str.str);
							}
						}
						NBString_release(&str);
						//ToDo: implement (flush-stats)
					}*/
				}
			}
			//stop
			if(!NBIOPollster_engineStop(opq->pollster.ref)){
				PRINTF_ERROR("TSThread #%d, NBIOPollster_engineStop failed.\n", opq->cfg.idx);
			}
		}
		NBThreadMutex_lock(&opq->mutex);
	}
	//flags as not-running
	{
		NBASSERT(opq->isRunning)
		opq->isRunning = FALSE;
		NBThreadCond_broadcast(&opq->cond);
		PRINTF_INFO("TSThread #%d ended.\n", opq->cfg.idx);
	}
	//disable per-thread storage
	{
		STTSThreadStorage* vars = (STTSThreadStorage*)NBThreadStorage_getData(&__tsThreadStorage);
		if(vars != NULL){
			NBThreadStorage_setData(&__tsThreadStorage, NULL, __tsThreadStorageDestroyDataMthd);
			TSThreadStorage_release(vars);
			NBMemory_free(vars);
			vars = NULL;
		}
		opq->thread.current.vars = NULL;
	}
}

//Commands

BOOL TSThread_statsFlushStart(STTSThread* obj, const UI32 iSeq){
	BOOL r = FALSE;
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	NBASSERT(opq != NULL)
	NBObject_lock(opq);
	if(opq->isRunning){
		if(opq->stats.flush.iSeqReq < iSeq){
			opq->stats.flush.iSeqReq = iSeq;
			r = TRUE;
		}
		//flush here
		if(!r){
			if(TS_THREAD_HAS_PEND_TASK_FLUSH_STATS(opq)){
				TSThread_consumePendTaskStatsFlushLockedOpq_(opq);
				NBASSERT(!TS_THREAD_HAS_PEND_TASK_FLUSH_STATS(opq))
			}
			opq->stats.flush.iSeqDone = opq->stats.flush.iSeqReq;
		}
	}
	NBObject_unlock(opq);
	return r;
}

BOOL TSThread_statsFlushIsPend(STTSThread* obj, const UI32 iSeq){
	BOOL r = FALSE;
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	NBASSERT(opq != NULL)
	NBObject_lock(opq);
	{
		r = TS_THREAD_HAS_PEND_TASK_FLUSH_STATS(opq);
		//flush here
		if(r && !opq->isRunning){
			TSThread_consumePendTaskStatsFlushLockedOpq_(opq);
			NBASSERT(!TS_THREAD_HAS_PEND_TASK_FLUSH_STATS(opq))
			opq->stats.flush.iSeqDone = opq->stats.flush.iSeqReq;
			r = FALSE;
		}
	}
	NBObject_unlock(opq);
	return r;
}

void TSThread_statsGet(STTSThread* obj, STTSThreadStatsData* dst, const BOOL resetAccum){
	STTSThreadOpq* opq = (STTSThreadOpq*)obj->opaque;
	NBASSERT(opq != NULL)
	NBObject_lock(opq);
	if(TSThreadStats_isSet(opq->stats.provider)){
		*dst = TSThreadStats_getData(opq->stats.provider, resetAccum);
	}
	NBObject_unlock(opq);
}

//TSThreadStorage

void TSThreadStorage_init(STTSThreadStorage* obj){
	NBMemory_setZeroSt(*obj, STTSThreadStorage);
}

void TSThreadStorage_release(STTSThreadStorage* obj){
	//nothing
}
