//
//  TSServer.c
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSServer.h"
#include "core/server/TSServerOpq.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBString.h"
#include "nb/core/NBArray.h"
#include "nb/core/NBDatetime.h"
#include "nb/core/NBNumParser.h"
#include "nb/crypto/NBSha1.h"
#include "nb/crypto/NBPkcs12.h"
#include "nb/net/NBHttpBuilder.h"
#include "nb/2d/NBPng.h"
#include "core/config/TSCfg.h"
#include "core/TSRequestsMap.h"
#include "core/logic/TSDevice.h"
//
#include <stdlib.h>	//for rand()

//Stats
void TSServerStats_statsChangedOpq_(STTSServerOpq* opq);
void TSServerStats_getDataLockedOpq_(STTSServerOpq* opq, STTSServerStatsData* dst, STNBArray* dstPortsStats /*STNBRtpClientPortStatsData*/, STNBArray* dstSsrcsStats /*STNBRtpClientSsrcStatsData*/, const BOOL resetAccum);

//Files cleanup
void TSServerStats_cleanOldFilesOpq_(STTSServerOpq* opq);

void TSServer_init(STTSServer* obj){
	STTSServerOpq* opq = obj->opaque = NBMemory_allocType(STTSServerOpq);
	NBMemory_setZeroSt(*opq, STTSServerOpq);
	opq->state.cur	= ENTSServerState_stopped;
	NBThreadMutex_init(&opq->mutex);
	NBThreadCond_init(&opq->cond);
	//	
	TSRequestsMap_init(&opq->reqsMap);
	//stats
	{
		TSServerStatsData_init(&opq->stats.print.data);
		NBArray_init(&opq->stats.print.ports, sizeof(STNBRtpClientPortStatsData), NULL);
		NBArray_init(&opq->stats.print.ssrcs, sizeof(STNBRtpClientSsrcStatsData), NULL);
		NBString_initWithSz(&opq->stats.print.strPrint, 4096, 4096, 0.10f);
	}
	//http
	{ 
		opq->http.service	= NBHttpService_alloc(NULL);
		//stats
		{
			NBHttpServiceCmdState_init(&opq->http.stats.flushCmd);
		}
	}
	/*
	TSApiRestClt_init(&opq->smsClt); //Sms sender client
	TSMailClt_init(&opq->mailClt); //Mail sender client
	TSApplePushServicesClt_init(&opq->applePushServices);
	TSFirebaseCMClt_init(&opq->firebaseCMClt);
	TSServerLogicClt_init(&opq->logicClt);
	*/
	//lstnr
	{
		//
	}
	//RTSP
	{
		opq->rtsp.enabled = FALSE;
		NBRtspClientStats_init(&opq->rtsp.stats);
	}
	//streams
	{
		TSServerStreams_init(&opq->streams.mngr);
		//
		TSStreamsStorage_init(&opq->streams.storage.mngr);
		TSStreamsStorageStats_init(&opq->streams.storage.stats);
		//requesters
		{
			opq->streams.reqs.mngr = TSClientRequester_alloc(NULL); 
			TSClientRequesterConnStats_init(&opq->streams.reqs.stats.conn);
			TSClientRequesterCmdState_init(&opq->streams.reqs.stats.flushCmd);
		}
	}
}

void TSServer_stopFlagOpq(STTSServerOpq* opq);
void TSServer_stopFlagLockedOpq(STTSServerOpq* opq);
void TSServer_stopOpq(STTSServerOpq* obj);
void TSServer_stopLockedOpq(STTSServerOpq* obj);
BOOL TSServer_isBusyOpq(STTSServerOpq* obj);
BOOL TSServer_isBusyLockedOpq(STTSServerOpq* obj);

void TSServer_release(STTSServer* obj){
	STTSServerOpq* opq = (STTSServerOpq*)obj->opaque;
	if(opq->syncItf != NULL){
		NBMemory_setZeroSt(*opq->syncItf, STTSServerSyncItf);
		NBMemory_free(opq->syncItf);
		opq->syncItf = NULL;
	}
	//context
	{
		if(opq->context.isListening){
			if(opq->context.cur != NULL){
				TSContext_removeLstnr(opq->context.cur, opq);
			}
			opq->context.isListening = FALSE;
		}
	}
	//Stop
	{
		NBThreadMutex_lock(&opq->mutex);
		{
			//Signal (trigger parallel stop)
			{
				TSServer_stopFlagLockedOpq(opq);
			}
			//Stop (and wait)
			{
				TSServer_stopLockedOpq(opq);
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	//http
	{
		if(NBHttpService_isSet(opq->http.service)){
			NBHttpService_release(&opq->http.service);
			NBHttpService_null(&opq->http.service);
		}
		//stats
		{
			NBHttpServiceCmdState_release(&opq->http.stats.flushCmd);
		}
	}
	//Streams
	{
		//cfg
		{
			const STNBStructMap* stMap = TSCfgFilesystemStreamsParams_getSharedStructMap();
			NBStruct_stRelease(stMap, &opq->streams.cfg, sizeof(opq->streams.cfg));
		}
		//mngr
		{
			TSServerStreams_release(&opq->streams.mngr);
		}
		//Requesters (should be empty at this point)
		{
			//stats
			if(TSClientRequester_isSet(opq->streams.reqs.mngr)){
				TSClientRequester_release(&opq->streams.reqs.mngr);
				TSClientRequester_null(&opq->streams.reqs.mngr);
			}
			TSClientRequesterConnStats_release(&opq->streams.reqs.stats.conn);
			TSClientRequesterCmdState_release(&opq->streams.reqs.stats.flushCmd);
		}
		//Storage
		TSStreamsStorage_release(&opq->streams.storage.mngr);
		TSStreamsStorageStats_release(&opq->streams.storage.stats);
	}
	//RTSP
	{
		NBRtspClientStats_release(&opq->rtsp.stats);
	}
	//stats
	{
		//stats
		{
			//ports
			{
				NBArray_empty(&opq->stats.print.ports);
				NBArray_release(&opq->stats.print.ports);
			}
			//ssrcs
			{
				NBArray_empty(&opq->stats.print.ssrcs);
				NBArray_release(&opq->stats.print.ssrcs);
			}
			NBString_release(&opq->stats.print.strPrint);
			TSServerStatsData_release(&opq->stats.print.data);
		}
	}
	TSRequestsMap_release(&opq->reqsMap);
    //loading
    {
        if(NBThreadsPool_isSet(opq->loading.threadsPool)){
            NBThreadsPool_release(&opq->loading.threadsPool);
            NBThreadsPool_null(&opq->loading.threadsPool);
        }
    }
	//context
	{
		opq->context.cur = NULL;
	}
	if(opq->caKey != NULL){
		NBPKey_release(opq->caKey);
		NBMemory_free(opq->caKey);
		opq->caKey = NULL;
	}
	if(opq->caCert != NULL){
		NBX509_release(opq->caCert);
		NBMemory_free(opq->caCert);
		opq->caCert = NULL;
	}
	NBThreadCond_release(&opq->cond);
	NBThreadMutex_release(&opq->mutex);
	NBMemory_setZero(opq->lstnr);
	//
	NBMemory_free(obj->opaque);
	obj->opaque = NULL;
}

//Sync itf

void TSServer_setSyncItf(STTSServer* obj, const STTSServerSyncItf* itf){
	STTSServerOpq* opq = (STTSServerOpq*)obj->opaque;
	//Release previous
	if(opq->syncItf != NULL){
		NBMemory_setZeroSt(*opq->syncItf, STTSServerSyncItf);
		NBMemory_free(opq->syncItf);
		opq->syncItf = NULL;
	}
	//Set new
	if(itf != NULL){
		opq->syncItf = NBMemory_allocType(STTSServerSyncItf);
		*opq->syncItf = *itf;
	}
}


BOOL TSServer_isDeviceOnlineOpq(STTSServerOpq* opq, const char* deviceId, BOOL* dstIsUnlocked, BOOL* dstIsSyncMeetings, BOOL* dstCertsSendFirst, BOOL* dstMeetsSendFirst){
	BOOL r = FALSE;
	if(opq->syncItf != NULL){
		if(opq->syncItf->isDeviceOnline != NULL){
			r = (*opq->syncItf->isDeviceOnline)(opq->syncItf->obj, deviceId, dstIsUnlocked, dstIsSyncMeetings, dstCertsSendFirst, dstMeetsSendFirst);
		}
	}
	return r;
}
	
BOOL TSServer_isDeviceOnline(STTSServer* obj, const char* deviceId, BOOL* dstIsUnlocked, BOOL* dstIsSyncMeetings, BOOL* dstCertsSendFirst, BOOL* dstMeetsSendFirst){
	STTSServerOpq* opq = (STTSServerOpq*)obj->opaque;
	return TSServer_isDeviceOnlineOpq(opq, deviceId, dstIsUnlocked, dstIsSyncMeetings, dstCertsSendFirst, dstMeetsSendFirst);
}

//

void TSServer_contextTickOpq_(const STNBTimestampMicro time, void* usrData){
	STTSServerOpq* opq = (STTSServerOpq*)usrData;
	//secChanged
	if(opq->context.tickLast.timestamp != time.timestamp){
		//flush-stats
		if(!opq->stats.isFlushing){
			BOOL isFlushing = FALSE;
			//TSClientRequester
			{
				//start
				if(!opq->streams.reqs.stats.flushCmd.isPend){
					opq->streams.reqs.stats.startTime = time;
					if(TSClientRequester_isSet(opq->streams.reqs.mngr)){
						TSClientRequester_statsFlushStart(opq->streams.reqs.mngr, &opq->streams.reqs.stats.flushCmd);
					}
				}
				//flag
				if(opq->streams.reqs.stats.flushCmd.isPend){
					isFlushing = TRUE;
				}
			}
			//NBHttpService
			{
				//start
				if(!opq->http.stats.flushCmd.isPend){
					opq->http.stats.startTime = time;
					if(NBHttpService_isSet(opq->http.service)){
						NBHttpService_statsFlushStart(opq->http.service, &opq->http.stats.flushCmd);
					}
				}
				if(opq->http.stats.flushCmd.isPend){
					isFlushing = TRUE;
				}
			}
			//print now
			if(isFlushing){
				opq->stats.isFlushing	= TRUE;
				opq->stats.startTime	= time;
			} else {
				PRINTF_INFO("TSServer, TSServer stats were flushed inmediatly.\n");
                //apply stats (set as current or print)
                TSServerStats_statsChangedOpq_(opq);
                TSServerStats_cleanOldFilesOpq_(opq);
			}
		}
		//increase runtime
		{
			opq->secsPrepared++;
			//Quit //ToDo: set flag in config-file to explicitly enable auto-quit (for security reasons)
			if(opq->state.cur == ENTSServerState_running){
				NBASSERT(opq->msRunAndQuit >= 0)
				if(opq->msRunAndQuit != 0 && opq->secsPrepared == (opq->msRunAndQuit / 1000) && !opq->stopFlag.applied){
					PRINTF_INFO("TSServer, auto-stop-signal after %d secs from prepared-status (%d secs running).\n", opq->secsPrepared, opq->secsPrepared);
					TSServer_stopFlagOpq(opq);
					opq->stopFlag.applied = TRUE;
				}
			}
		}
		//streams
		{
			TSServerStreams_tickSecChanged(&opq->streams.mngr, time.timestamp);
		}
		//streams requesters
		{
			TSClientRequester_tickSecChanged(opq->streams.reqs.mngr, time.timestamp);
		}
		//consume external stopFlag
		if(opq->stopFlag.external != NULL && *opq->stopFlag.external && !opq->stopFlag.applied){
			PRINTF_INFO("TSServer, consuming stop-signal after %d secs from prepared-status (%d secs running).\n", opq->secsPrepared, opq->secsPrepared);
			TSServer_stopFlagOpq(opq);
			opq->stopFlag.applied = TRUE;
		}
		//stop current thread (complementary to 'TSContext_startAndUseThisThread')
		if(opq->stopFlag.applied && !opq->stopFlag.appliedToThread && opq->context.cur != NULL && !TSServer_isBusyOpq(opq)){
			PRINTF_INFO("TSServer, calling 'TSContext_stopFlagThreadIfIsCurrent'.\n");
			if(TSContext_stopFlagThreadIfIsCurrent(opq->context.cur)){
				PRINTF_INFO("TSServer, main thread stopFlag after %d secs from prepared-status (%d secs running).\n", opq->secsPrepared, opq->secsPrepared);
				opq->stopFlag.appliedToThread = TRUE;
			}
		}
	}
	//Flush
	if(opq->stats.isFlushing){
		BOOL isFlushing = FALSE;
		//TSClientRequester
		if(opq->streams.reqs.stats.flushCmd.isPend){
			if(TSClientRequester_isSet(opq->streams.reqs.mngr) && TSClientRequester_statsFlushIsPend(opq->streams.reqs.mngr, &opq->streams.reqs.stats.flushCmd)){
				isFlushing = TRUE;
			} else {
				//Flush was completed
				NBASSERT(!opq->streams.reqs.stats.flushCmd.isPend)
#				ifdef NB_CONFIG_INCLUDE_ASSERTS
				/*{
					const SI64 usecs = NBTimestampMicro_getDiffInUs(&opq->http.stats.startTime, &time);
					if(usecs < 0){
						PRINTF_INFO("TSServer, TSClientRequester stats took %lld.%lld%lldms to flush.\n", 0, 0, 0);
					}  else {
						PRINTF_INFO("TSServer, TSClientRequester stats took %lld.%lld%lldms to flush.\n", (usecs / 1000), ((usecs % 1000) / 100), ((usecs % 100) / 10));
					}
				}*/
#				endif
			}
		}
		//NBHttpService
		if(opq->http.stats.flushCmd.isPend){
			if(NBHttpService_isSet(opq->http.service) && NBHttpService_statsFlushIsPend(opq->http.service, &opq->http.stats.flushCmd)){
				isFlushing = TRUE;
			} else {
				//Flush was completed
				NBASSERT(!opq->http.stats.flushCmd.isPend)
#				ifdef NB_CONFIG_INCLUDE_ASSERTS
				/*{
					const SI64 usecs = NBTimestampMicro_getDiffInUs(&opq->http.stats.startTime, &time);
					if(usecs < 0){
						PRINTF_INFO("TSServer, NBHttpService stats took %lld.%lld%lldms to flush.\n", 0, 0, 0);
					} else {
						PRINTF_INFO("TSServer, NBHttpService stats took %lld.%lld%lldms to flush.\n", (usecs / 1000), ((usecs % 1000) / 100), ((usecs % 100) / 10));
					}
				}*/
#				endif
			} 
		}
		//result
		if(!isFlushing){
			opq->stats.isFlushing = FALSE;
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			/*{
				const SI64 usecs = NBTimestampMicro_getDiffInUs(&opq->stats.startTime, &time);
				if(usecs < 0){
					PRINTF_INFO("TSServer, TSServer stats took %lld.%lld%lldms to flush.\n", 0, 0, 0);
				}  else {
					PRINTF_INFO("TSServer, TSServer stats took %lld.%lld%lldms to flush.\n", (usecs / 1000), ((usecs % 1000) / 100), ((usecs % 100) / 10));
				}
			}*/
#			endif
			//apply stats (set as current or print)
            TSServerStats_statsChangedOpq_(opq);
            TSServerStats_cleanOldFilesOpq_(opq);
		}
	}
	//keep
	opq->context.tickLast = time;
}

//

//Stats
void TSServerStats_concatDataStreamsLocked_(STTSServerOpq* opq, const STTSServerStatsData* obj, const STTSCfgTelemetry* cfg, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, const STNBRtpClientPortStatsData* portsStats, const UI32 portsStatsSz, const STNBRtpClientSsrcStatsData* ssrcsStats, const UI32 ssrcsStatsSz, STNBString* dst);
void TSServerStats_concatStateStreamsLocked_(STTSServerOpq* opq, const STTSServerStatsState* obj, const STTSCfgTelemetry* cfg, const ENTSServerStartsGrp grp, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, const STNBRtpClientPortStatsData* portsStats, const UI32 portsStatsSz, const STNBRtpClientSsrcStatsData* ssrcStats, const UI32 ssrcStatsSz, STNBString* dst);
#ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
void TSServerStats_concatProcessState(const STTSServerStatsProcessState* obj, const ENNBLogLevel logLvl, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst);
#endif

void TSServerStats_statsChangedOpq_(STTSServerOpq* opq){
	//disable memory stats
#	ifdef CONFIG_NB_INCLUDE_THREADS_ENABLED
	NBProcess_memStatsEnabledThreadPush(FALSE, __FILE__, __LINE__, __func__);
#	endif
	NBThreadMutex_lock(&opq->mutex);
	{
		const STTSCfg* cCfg = TSContext_getCfg(opq->context.cur);
		//reset
		{
			//ports
			{
				NBArray_empty(&opq->stats.print.ports);
			}
			//ssrcs
			{
				NBArray_empty(&opq->stats.print.ssrcs);
			}
			NBString_empty(&opq->stats.print.strPrint);
			TSServerStatsData_release(&opq->stats.print.data);
			TSServerStatsData_init(&opq->stats.print.data);
		}
		//
		TSServerStats_getDataLockedOpq_(opq, &opq->stats.print.data, (cCfg->server.telemetry.streams.perPortDetails ? &opq->stats.print.ports : NULL), (cCfg->server.telemetry.streams.perStreamDetails ? &opq->stats.print.ssrcs : NULL), TRUE);
		{
			//Print
			{ 
				const BOOL ignoreEmpties	= TRUE;
				const BOOL loaded			= TRUE;
				const BOOL accum			= TRUE;
				const BOOL total			= FALSE;
				const char* prefixFirst		= "";
				const char* prefixOthers	= "                       ";
				{
					const STNBRtpClientPortStatsData* portsStatsArr = NBArray_dataPtr(&opq->stats.print.ports, STNBRtpClientPortStatsData);
					const STNBRtpClientSsrcStatsData* ssrcsStatsArr = NBArray_dataPtr(&opq->stats.print.ssrcs, STNBRtpClientSsrcStatsData);
					TSServerStats_concatDataStreamsLocked_(opq, &opq->stats.print.data, &cCfg->server.telemetry, loaded, accum, total, prefixFirst, prefixOthers, ignoreEmpties, portsStatsArr, opq->stats.print.ports.use, ssrcsStatsArr, opq->stats.print.ssrcs.use, &opq->stats.print.strPrint);
				}
				if(opq->stats.print.strPrint.length > 0){
					PRINTF_CONSOLE("%s\n", opq->stats.print.strPrint.str);
				}
			}
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
#	ifdef CONFIG_NB_INCLUDE_THREADS_ENABLED
	NBProcess_memStatsEnabledThreadPop(__FILE__, __LINE__, __func__);
#	endif
}

//Files cleanup

void TSServerStats_cleanOldFilesOpq_(STTSServerOpq* opq){
    NBThreadMutex_lock(&opq->mutex);
    {
        //Delete oldest files
        if(!opq->streams.cfg.isReadOnly){
            const UI64 limitBytes = opq->streams.cfg.limitBytes; NBASSERT(limitBytes > 0)
            const UI64 totalBytes = TSStreamsStorageStats_totalBytesState(&opq->stats.print.data.loaded.storage);
            if(totalBytes > limitBytes){
                const UI64 maxBytesToDelete = (totalBytes - limitBytes);
                const UI64 deletedBytes = TSStreamsStorage_deleteOldestFiles(&opq->streams.storage.mngr, maxBytesToDelete, opq->streams.cfg.keepUnknownFiles);
                if(deletedBytes > 0){
                    PRINTF_INFO("TSServer, oldest %u MBs (%d MBs requested of %d MBs) deleted from StreamsStorage.\n", (UI32)(deletedBytes / (1024 * 1024)), (UI32)(maxBytesToDelete / (1024 * 1024)), (UI32)(totalBytes / (1024 * 1024)));
                }
            }
        }
    }
    NBThreadMutex_unlock(&opq->mutex);
}


//Stop

void TSServer_preparedSignalLockedOpq(STTSServerOpq* opq){
	//Global state
	NBASSERT(opq->state.cur == ENTSServerState_preparing)
	if(opq->state.cur == ENTSServerState_preparing){
		/*
		//Send prepared signal (trigger parallel stop)
		{
			TSApiRestClt_stopFlag(&opq->smsClt);
			//PRINTF_INFO("TSServer, TSApiRestClt_stopFlag.\n");
		}
		{
			TSMailClt_stopFlag(&opq->mailClt);
			//PRINTF_INFO("TSServer, TSMailClt_stopFlag.\n");
		}
		{
			TSApplePushServicesClt_stopFlag(&opq->applePushServices);
			//PRINTF_INFO("TSServer, TSApplePushServicesClt_stopFlag.\n");
		}
		{
			TSFirebaseCMClt_stopFlag(&opq->firebaseCMClt);
			//PRINTF_INFO("TSServer, TSFirebaseCMClt_stopFlag.\n");
		}
		{
			TSServerLogicClt_stopFlag(&opq->logicClt);
			//PRINTF_INFO("TSServer, TSServerLogicClt_stopFlag.\n");
		}
		//RTSP
		{
			NBRtspClient_startSignal(&opq->rtsp.client);
		}*/
		//vClock
		{
			//Do not stop the virtual-clock
			//Since it is crucial to cleanup operation.
		}
		//Notify state
		opq->state.cur = ENTSServerState_prepared;
		NBThreadCond_broadcast(&opq->cond);
	}
}

//Stop

void TSServer_stopFlagLockedOpq(STTSServerOpq* opq){
	//Global state
	if(opq->state.cur != ENTSServerState_stopped){
		//Notify state
		opq->state.cur = ENTSServerState_stopping;
		NBThreadCond_broadcast(&opq->cond);
	}
	//http
	if(NBHttpService_isSet(opq->http.service)){
		NBHttpService_stopFlag(opq->http.service);
	}
	/*
	//Send stop signal (trigger parallel stop)
	{
		TSApiRestClt_stopFlag(&opq->smsClt);
		//PRINTF_INFO("TSServer, TSApiRestClt_stopFlag.\n");
	}
	{
		TSMailClt_stopFlag(&opq->mailClt);
		//PRINTF_INFO("TSServer, TSMailClt_stopFlag.\n");
	}
	{
		TSApplePushServicesClt_stopFlag(&opq->applePushServices);
		//PRINTF_INFO("TSServer, TSApplePushServicesClt_stopFlag.\n");
	}
	{
		TSFirebaseCMClt_stopFlag(&opq->firebaseCMClt);
		//PRINTF_INFO("TSServer, TSFirebaseCMClt_stopFlag.\n");
	}
	{
		TSServerLogicClt_stopFlag(&opq->logicClt);
		//PRINTF_INFO("TSServer, TSServerLogicClt_stopFlag.\n");
	}
	*/
	//requester
	{
		TSClientRequester_stopFlag(opq->streams.reqs.mngr);
	}
	//streams
	{
		TSServerStreams_stopFlag(&opq->streams.mngr);
	}
	//vClock
	{
		//Do not stop the virtual-clock
		//Since it is crucial to cleanup operation.
	}
	//Signal
	NBThreadCond_broadcast(&opq->cond);
}

void TSServer_stopFlagOpq(STTSServerOpq* opq){
	NBThreadMutex_lock(&opq->mutex);
	TSServer_stopFlagLockedOpq(opq);
	NBThreadMutex_unlock(&opq->mutex);
}

void TSServer_stopFlag(STTSServer* obj){
	STTSServerOpq* opq = (STTSServerOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	TSServer_stopFlagLockedOpq(opq);
	NBThreadMutex_unlock(&opq->mutex);
}

void TSServer_stopLockedOpq(STTSServerOpq* opq){
	//Global state
	if(opq->state.cur != ENTSServerState_stopped){
		//Notify state
		opq->state.cur = ENTSServerState_stopping;
		NBThreadCond_broadcast(&opq->cond);
	}
	//Send stop signal and wait for threads to exit
	/*
	{
		TSApiRestClt_stop(&opq->smsClt);
		//PRINTF_INFO("TSServer, TSApiRestClt_stop.\n");
	}
	{
		TSMailClt_stop(&opq->mailClt);
		//PRINTF_INFO("TSServer, TSMailClt_stop.\n");
	}
	{
		TSApplePushServicesClt_stop(&opq->applePushServices);
		//PRINTF_INFO("TSServer, TSApplePushServicesClt_stop.\n");
	}
	{
		TSFirebaseCMClt_stop(&opq->firebaseCMClt);
		//PRINTF_INFO("TSServer, TSFirebaseCMClt_stop.\n");
	}
	{
		TSServerLogicClt_stop(&opq->logicClt);
		//PRINTF_INFO("TSServer, TSServerLogicClt_stop.\n");
	}
	*/
	//Streams
	{
		//mngr
		TSServerStreams_stop(&opq->streams.mngr);
	}
	//vClock
	{
		//Do not stop the virtual-clock
		//Since it is crucial to cleanup operation.
	}
	//Notify state
	{
		opq->state.cur = ENTSServerState_stopped;
		NBThreadCond_broadcast(&opq->cond);
	}
	//PRINTF_INFO("TSServer, stopped.\n");
}

void TSServer_stopOpq(STTSServerOpq* opq){
	NBThreadMutex_lock(&opq->mutex);
	TSServer_stopLockedOpq(opq);
	NBThreadMutex_unlock(&opq->mutex);
}

void TSServer_stop(STTSServer* obj){
	STTSServerOpq* opq = (STTSServerOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	TSServer_stopLockedOpq(opq);
	NBThreadMutex_unlock(&opq->mutex);
}

BOOL TSServer_isBusyLockedOpq(STTSServerOpq* opq){
	BOOL r = FALSE;
    if(!r && (opq->loading.prepareDepth > 0 || opq->loading.storageDepth > 0)){
        r = TRUE;
    }
	if(!r && TSClientRequester_isBusy(opq->streams.reqs.mngr)){
		r = TRUE;
	}
	if(!r && NBHttpService_isBusy(opq->http.service)){
		r = TRUE;
	}
	return r;
}

BOOL TSServer_isBusyOpq(STTSServerOpq* opq){
	BOOL r = FALSE;
	NBThreadMutex_lock(&opq->mutex);
	{
		r = TSServer_isBusyLockedOpq(opq);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSServer_isBusy(STTSServer* obj){
	BOOL r = FALSE;
	STTSServerOpq* opq = (STTSServerOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
    {
        r = TSServer_isBusyLockedOpq(opq);
    }
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

//Prepare

BOOL TSServer_addMailToLocalQueue_(void* opq, const char* to, const char* subject, const char* body, const char* attachFilename, const char* attachMime, const void* attachData, const UI32 attachDataSz){
	return TSServer_addMailToLocalQueue((STTSServerOpq*)opq, to, subject, body, attachFilename, attachMime, attachData, attachDataSz);
}

void TSServer_TSStreamsStorageLoadEndedFunc_(const BOOL isSuccess, void* usrData);
void TSServer_loadEndedDoPostActionsLockedOpq_(STTSServerOpq* opq);

BOOL TSServer_prepare(STTSServer* obj, STTSContext* context, const BOOL* externalStopFlag, const SI32 msRunAndQuit, STTSServerLstnr* lstnr){
	BOOL r = TRUE;
	STTSServerOpq* opq			= (STTSServerOpq*)obj->opaque;
	const STTSCfg* pCfg			= TSContext_getCfg(context);
	const char* runId			= TSContext_getRunId(context);
	STNBFilesystem* fs			= TSContext_getFilesystem(context);
	STNBDataPtrsPoolRef bPool	= TSContext_getBuffersPool(context);
	STNBIOPollstersProviderRef pProv = TSContext_getPollstersProvider(context);
	NBThreadMutex_lock(&opq->mutex);
	//
	{
		BOOL isListeningContext = FALSE;
		//Change state
		opq->state.cur = ENTSServerState_preparing;
        //
        NBASSERT(opq->loading.prepareDepth == 0)
        opq->loading.prepareDepth++;
		//Load CA's private key and certificate
		if(r){
			const STNBHttpCertSrcCfg* s = &pCfg->context.caCert;
			if(s->key.pass == NULL || s->key.path == NULL){
				PRINTF_CONSOLE_ERROR("TSServer, Server requires 'key.pass' and 'key.path'.\n");
				r = FALSE;
			} else {
				const char* pKey = s->key.path;
				const char* pPass = s->key.pass;
				STNBPkcs12 p12;
				NBPkcs12_init(&p12);
				if(!NBPkcs12_createFromDERFile(&p12, pKey)){
					PRINTF_CONSOLE_ERROR("TSServer, Could not load key: '%s'.\n", pKey);
					r = FALSE;
				} else {
					STNBPKey* key = NBMemory_allocType(STNBPKey);
					STNBX509* cert = NBMemory_allocType(STNBX509);
					NBPKey_init(key);
					NBX509_init(cert);
					if(!NBPkcs12_getCertAndKey(&p12, key, cert, pPass)){
						PRINTF_CONSOLE_ERROR("TSServer, Could not extract key and cer: '%s'.\n", pKey);
						r = FALSE;
					} else {
						opq->caKey = key;
						opq->caCert = cert;
						key = NULL;
						cert = NULL;
					}
					if(key != NULL){
						NBPKey_release(key);
						NBMemory_free(key);
						key = NULL;
					}
					if(cert != NULL){
						NBX509_release(cert);
						NBMemory_free(cert);
						cert = NULL;
					}
				}
				NBPkcs12_release(&p12);
			}
		}
		//Prepare http service
		if(r){
			STNBHttpServiceLstnrItf itf;
			NBMemory_setZeroSt(itf, STNBHttpServiceLstnrItf);
			itf.httpCltConnected		= TSServer_httpCltConnected_;
			itf.httpCltDisconnected		= TSServer_httpCltDisconnected_;
            itf.httpCltReqArrived       = TSServer_httpCltReqArrived_;
			//
			if(!NBHttpService_setCaCertAndKey(opq->http.service, opq->caCert, opq->caKey)){
				PRINTF_CONSOLE_ERROR("TSServer, NBHttpService_setCaCertAndKey failed.\n");
				r = FALSE;
			} else if(!NBHttpService_setPollstersProvider(opq->http.service, pProv)){
				PRINTF_CONSOLE_ERROR("TSServer, NBHttpService_setPollsterProvider failed.\n");
				r = FALSE;
			} else if(!NBHttpService_prepare(opq->http.service, &pCfg->http, &itf, opq)){
				PRINTF_CONSOLE_ERROR("TSServer, NBHttpService_prepare failed.\n");
				r = FALSE;
			} else if(!NBHttpService_startListening(opq->http.service)){
				PRINTF_CONSOLE_ERROR("TSServer, NBHttpService_prepare failed.\n");
				r = FALSE;
			} else {
				PRINTF_INFO("TSServer, NBHttpService started.\n");
			}
		}
		/*//Prepare SMS distribution client
		if(r){
			if(!TSApiRestClt_loadFromConfig(&opq->smsClt, &pCfg->context.sms)){
				PRINTF_CONSOLE_ERROR("TSServer, Could not load TSApiRestClt from loaded cfg.\n");
				r = FALSE;
			} else if(!TSApiRestClt_prepare(&opq->smsClt)){
				PRINTF_CONSOLE_ERROR("TSServer, Could not start-thread TSApiRestClt from loaded cfg.\n");
				r = FALSE;
			} else {
				//Test
			}
		}
		//Prepare Mail distribution client
		if(r){
			if(!TSMailClt_loadFromConfig(&opq->mailClt, &pCfg->smtp)){
				PRINTF_CONSOLE_ERROR("TSServer, Could not load TSMailClt from loaded cfg.\n");
				r = FALSE;
			} else if(!TSMailClt_prepare(&opq->mailClt)){
				PRINTF_CONSOLE_ERROR("TSServer, Could not start-thread TSMailClt from loaded cfg.\n");
				r = FALSE;
			} else {
				//test
			}
		}
		//Prepare applePushServices
		if(r){
			if(!TSApplePushServicesClt_loadFromConfig(&opq->applePushServices, &pCfg->applePushServices)){
				PRINTF_CONSOLE_ERROR("TSServer, Could not load TSApplePushServicesClt from loaded cfg.\n");
				r = FALSE;
			} else if(!TSApplePushServicesClt_prepare(&opq->applePushServices)){
				PRINTF_CONSOLE_ERROR("TSServer, Could not start-thread TSApplePushServicesClt from loaded cfg.\n");
				r = FALSE;
			} else {
				//
			}
		}
		//Prepare firebase messaging services
		if(r){
			if(!TSFirebaseCMClt_loadFromConfig(&opq->firebaseCMClt, &pCfg->firebaseCM)){
				PRINTF_CONSOLE_ERROR("TSServer, Could not load TSFirebaseCMClt from loaded cfg.\n");
				r = FALSE;
			} else if(!TSFirebaseCMClt_prepare(&opq->firebaseCMClt)){
				PRINTF_CONSOLE_ERROR("TSServer, Could not start-thread TSFirebaseCMClt from loaded cfg.\n");
				r = FALSE;
			} else {
				//
			}
		}*/
		//Prepare rtsp client
		if(r && !pCfg->server.rtsp.isDisabled){
			if(!TSClientRequester_setStatsProvider(opq->streams.reqs.mngr, &opq->rtsp.stats, &opq->streams.reqs.stats.conn)){
				PRINTF_CONSOLE_ERROR("TSServer, TSClientRequester_setStatsProvider failed.\n");
				r = FALSE;
			} else if(!TSClientRequester_setExternalBuffers(opq->streams.reqs.mngr, bPool)){
				PRINTF_CONSOLE_ERROR("TSServer, TSClientRequester_setExternalBuffers failed.\n");
				r = FALSE;
			} else if(!TSClientRequester_setPollstersProvider(opq->streams.reqs.mngr, pProv)){
				PRINTF_CONSOLE_ERROR("TSServer, TSClientRequester_setPollstersProvider failed.\n");
				r = FALSE;
			} else if(!TSClientRequester_setCfg(opq->streams.reqs.mngr, &pCfg->server.rtsp, runId, ENNBRtspRessState_Stopped)){
				PRINTF_CONSOLE_ERROR("TSServer, TSClientRequester_setCfg failed.\n");
				r = FALSE;
			} else if(!TSClientRequester_prepare(opq->streams.reqs.mngr)){
				PRINTF_CONSOLE_ERROR("TSServer, TSClientRequester_prepare failed.\n");
				r = FALSE;
			} else if(!TSClientRequester_start(opq->streams.reqs.mngr)){
				PRINTF_CONSOLE_ERROR("TSServer, TSClientRequester_start failed.\n");
				r = FALSE;
			}
		}
		//Prepare streams storage
		if(r){
			if(!TSStreamsStorage_setStatsProvider(&opq->streams.storage.mngr, &opq->streams.storage.stats)){
				PRINTF_CONSOLE_ERROR("TSServer, TSStreamsStorage_setStatsProvider failed.\n");
				r = FALSE;
			} else if(!TSStreamsStorage_setFilesystem(&opq->streams.storage.mngr, fs)){
				PRINTF_CONSOLE_ERROR("TSServer, TSStreamsStorage_setFilesystem failed.\n");
				r = FALSE;
			} else if(!TSStreamsStorage_setRootPath(&opq->streams.storage.mngr, pCfg->context.filesystem.streams.rootPath)){
				PRINTF_CONSOLE_ERROR("TSServer, TSStreamsStorage_setRootPath failed: '%s'.\n", pCfg->context.filesystem.streams.rootPath);
				r = FALSE;
			} else if(!TSStreamsStorage_setRunId(&opq->streams.storage.mngr, context, runId)){
				PRINTF_CONSOLE_ERROR("TSServer, TSStreamsStorage_setRunId failed: '%s'.\n", pCfg->context.filesystem.streams.rootPath);
				r = FALSE; //
			} else {
				const STNBStructMap* stParamsMap = TSCfgFilesystemStreamsParams_getSharedStructMap();
				//set defaults as current params
				{
					NBStruct_stRelease(stParamsMap, &opq->streams.cfg, sizeof(opq->streams.cfg));
					NBStruct_stClone(stParamsMap, &pCfg->context.filesystem.streams.defaults, sizeof(pCfg->context.filesystem.streams.defaults), &opq->streams.cfg, sizeof(opq->streams.cfg));
				}
				//Load folder specific params file
				if(!NBString_strIsEmpty(pCfg->context.filesystem.streams.cfgFilename)){
					STNBString filepath;
					NBString_init(&filepath);
					//build path
					{
						NBString_concat(&filepath, pCfg->context.filesystem.streams.rootPath);
						if(filepath.length > 0 && filepath.str[filepath.length - 1] != '/' && filepath.str[filepath.length - 1] != '\\'){
							NBString_concatByte(&filepath, '/');
						}
						NBString_concat(&filepath, pCfg->context.filesystem.streams.cfgFilename);
					}
					//load file
					{
                        STNBFileRef file = NBFile_alloc(NULL);
						if(!NBFile_open(file, filepath.str, ENNBFileMode_Read)){
							PRINTF_INFO("TSServer, streams-params file does not exists: '%s'.\n", filepath.str);
						} else {
							//load struct
							NBFile_lock(file);
							{
								STTSCfgFilesystemStreamsParams params;
								NBMemory_setZeroSt(params, STTSCfgFilesystemStreamsParams);
								if(!NBStruct_stReadFromJsonFile(file, stParamsMap, &params, sizeof(params))){
									PRINTF_CONSOLE_ERROR("TSServer, streams-params file could not be parsed: '%s'.\n", filepath.str);
								} else {
									STTSCfgFilesystemStreamsParams params2;
									NBMemory_setZeroSt(params2, STTSCfgFilesystemStreamsParams);
									if(!TSCfgFilesystemStreamsParams_cloneNormalized(&params2, &params)){
										PRINTF_CONSOLE_ERROR("TSServer, TSCfgFilesystemStreamsParams_cloneNormalized failed for: '%s'.\n", filepath.str);
									} else {
										//set as current params
										NBStruct_stRelease(stParamsMap, &opq->streams.cfg, sizeof(opq->streams.cfg));
										NBStruct_stClone(stParamsMap, &params2, sizeof(params2), &opq->streams.cfg, sizeof(opq->streams.cfg));
										PRINTF_INFO("TSServer, streams-params loaded from: '%s'.\n", filepath.str);
									}
									NBStruct_stRelease(stParamsMap, &params2, sizeof(params2));
								}
								NBStruct_stRelease(stParamsMap, &params, sizeof(params));
							}
							NBFile_unlock(file);
						}
						NBFile_release(&file);
					}
					NBString_release(&filepath);
				}
				//streams-params
#				ifdef NB_CONFIG_INCLUDE_ASSERTS
				{
					STNBString str;
					NBString_init(&str);
					NBStruct_stConcatAsJson(&str, stParamsMap, &opq->streams.cfg, sizeof(opq->streams.cfg));
					PRINTF_INFO("TSServer, streams-params to be used: '%s'.\n", str.str);
					NBString_release(&str);
				}
#				endif
				//Load (ignore error)
				{
					const BOOL forceLoadAll = FALSE;
					IF_PRINTF(const UI64 startTime = NBDatetime_getCurUTCTimestamp();)
					UI32 ammThreadsToUSe = 1;
					PRINTF_CONSOLE("TSServer, streams storage loading (%d threads max): '%s'.\n", opq->streams.cfg.initialLoadThreads, pCfg->context.filesystem.streams.rootPath);
					{
						//prepare threads-pool (optional)
						if(!NBThreadsPool_isSet(opq->loading.threadsPool) && opq->streams.cfg.initialLoadThreads > 1){
							STNBThreadsPoolCfg cfg;
							NBMemory_setZeroSt(cfg, STNBThreadsPoolCfg);
							//cfg.minAlloc = cfg.maxKeep = opq->streams.cfg.initialLoadThreads; //ToDo: remove.
							{
                                opq->loading.threadsPool = NBThreadsPool_alloc(NULL);
								if(!NBThreadsPool_setCfg(opq->loading.threadsPool, &cfg)){
									PRINTF_ERROR("TSServer, NBThreadsPool_setCfg failed for storage-load.\n");
									NBThreadsPool_release(&opq->loading.threadsPool);
									NBThreadsPool_null(&opq->loading.threadsPool);
								} else if(!NBThreadsPool_prepare(opq->loading.threadsPool)){
									PRINTF_ERROR("TSServer, NBThreadsPool_prepare failed for storage-load.\n");
									NBThreadsPool_release(&opq->loading.threadsPool);
									NBThreadsPool_null(&opq->loading.threadsPool);
								} else {
									ammThreadsToUSe = opq->streams.cfg.initialLoadThreads;
								}
							}
						}
						//load
						{
                            BOOL rr = FALSE;
							const char* ignoreTrashFiles = pCfg->context.filesystem.streams.cfgFilename;
							const UI32 ignoreTrashFilesSz = (NBString_strIsEmpty(ignoreTrashFiles) ? 0 : 1);
                            NBASSERT(opq->loading.prepareDepth == 1)
                            NBASSERT(opq->loading.storageDepth == 0)
                            opq->loading.storageDepth++;
                            NBThreadMutex_unlock(&opq->mutex);
                            {
                                //unlocked (it could notiofy inmediatly if no more than 1 thread for loading)
                                if(!TSStreamsStorage_loadStart(&opq->streams.storage.mngr, forceLoadAll, &ignoreTrashFiles, ignoreTrashFilesSz, externalStopFlag, TSServer_TSStreamsStorageLoadEndedFunc_, opq, opq->loading.threadsPool, ammThreadsToUSe)){
                                    PRINTF_ERROR("TSServer, TSStreamsStorage_loadStart failed for storage-load.\n");
                                    r = FALSE;
                                } else {
                                    rr = TRUE;
                                    //wait for load to finish (async but blocking)
                                    if(opq->streams.cfg.waitForLoad){
                                        NBThreadMutex_lock(&opq->mutex);
                                        while(opq->loading.storageDepth > 0){
                                            NBThreadCond_wait(&opq->cond, &opq->mutex);
                                        }
                                        NBThreadMutex_unlock(&opq->mutex);
                                    }
                                }
                            }
                            NBThreadMutex_lock(&opq->mutex);
                            NBASSERT(opq->loading.prepareDepth == 1)
                            if(!rr){
                                NBASSERT(opq->loading.storageDepth == 1)
                                if(opq->loading.storageDepth > 0){
                                    opq->loading.storageDepth--;
                                }
                            }
						}
					}
					PRINTF_INFO("TSServer, streams storage loaded (%llu secs, %d threads max): '%s'.\n", (NBDatetime_getCurUTCTimestamp() - startTime), ammThreadsToUSe, pCfg->context.filesystem.streams.rootPath);
				}
			}
		}
		//Prepare logic server
		/*if(r){
			if(!TSServerLogicClt_loadFromConfig(&opq->logicClt, &pCfg->server, context, opq->caKey, TSServer_addMailToLocalQueue_, opq, &opq->stats, &opq->channelsStats)){
				PRINTF_CONSOLE_ERROR("TSServer, Could not load TSServerLogicClt from loaded cfg.\n");
				r = FALSE;
			} else if(!TSServerLogicClt_prepare(&opq->logicClt)){
				PRINTF_CONSOLE_ERROR("TSServer, Could not start-thread TSServerLogicClt from loaded cfg.\n");
				r = FALSE;
			} else {
				//
			}
		}*/
		//Force error (testing stop signals)
		//r = FALSE;
		//register at context
		if(r){
			STTSContextLstnrItf itf;
			NBMemory_setZeroSt(itf, STTSContextLstnrItf);
			itf.contextTick = TSServer_contextTickOpq_;
			opq->context.tickLast = NBTimestampMicro_getMonotonicFast();
			if(!TSContext_addLstnr(context, &itf, opq)){
				PRINTF_CONSOLE_ERROR("TSServer, TSContext_addLstnr.\n");
				r = FALSE;
			} else {
				isListeningContext = TRUE;
			}
		}
		//Change state
		if(!r){
			PRINTF_CONSOLE_ERROR("TSServer, Could not prepare all services, canceling...\n");
			//Cancel
			{
				//Signal
				TSServer_stopFlagLockedOpq(opq);
				//Wait for all (unlocked)
				TSServer_stopLockedOpq(opq);
				//context
				if(isListeningContext){
					if(context != NULL){
						TSContext_removeLstnr(context, opq);
					}
					isListeningContext = FALSE;
				}
			}
		} else {
			//context
			{
				if(opq->context.isListening){
					if(opq->context.cur != NULL){
						TSContext_removeLstnr(opq->context.cur, opq);
						opq->context.cur = NULL;
					}
					opq->context.isListening = FALSE;
				}
				opq->context.cur = context;
				opq->context.isListening = isListeningContext;
			}
			//lstnr
			{
				if(lstnr == NULL){
					NBMemory_setZero(opq->lstnr);
				} else {
					opq->lstnr	= *lstnr;
				}
			}
			//streams
			{
				TSServerStreams_setServerOpq(&opq->streams.mngr, opq);
			}
			/*{
				TSServerStats_loadCurrentAsServiceStart(context, &opq->stats);
				NBHttpStats_setData(&opq->channelsStats, &opq->stats.stats);
			}*/
			{
				if(msRunAndQuit > 0){
					opq->msRunAndQuit = msRunAndQuit; 
				}
			}
			//Make all workers to continue
			TSServer_preparedSignalLockedOpq(opq);
		}
        //Add static streams (must be done after start)
        if(r){
            if(pCfg->server.streamsGrps != NULL && pCfg->server.streamsGrpsSz > 0){
                UI32 i; for(i = 0; i < pCfg->server.streamsGrpsSz && r; i++){
                    const STTSCfgStreamsGrp* grp = &pCfg->server.streamsGrps[i];
                    if(!grp->isDisabled && grp->streams != NULL && grp->streamsSz > 0){
                        UI32 i; for(i = 0; i < grp->streamsSz && r; i++){
                            const STTSCfgStream* s = &grp->streams[i];
                            if(!s->isDisabled && s->versions != NULL && s->versionsSz > 0){
                                UI32 i; for(i = 0; i < s->versionsSz  && r; i++){
                                    STTSCfgStreamVersion* v = &s->versions[i];
                                    if(!v->isDisabled){
                                        //register stream in the RTSP client
                                        STTSStreamAddress address;
                                        NBMemory_setZeroSt(address, STTSStreamAddress);
                                        address.server      = s->device.server;
                                        address.port        = s->device.port;
                                        address.useSSL      = s->device.useSSL;
                                        address.uri         = v->uri;
                                        address.user        = s->device.user;
                                        address.pass        = s->device.pass;
                                        if(!TSClientRequester_rtspAddStream(opq->streams.reqs.mngr, grp->uid, grp->name, s->device.uid, s->device.name, v->sid, v->sid, &address, (v->params != NULL ? v->params : &pCfg->server.defaults.streams.params))){
                                            PRINTF_CONSOLE_ERROR("TSServer, TSClientRequester_rtspAddStream failed for static viewer: '%s'.\n", v->uri);
                                            r = FALSE;
                                        } else {
                                            PRINTF_INFO("TSServer, stream added: '%s'.\n", v->uri);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        //end-prepare-call
        NBASSERT(opq->loading.prepareDepth == 1)
        if(opq->loading.prepareDepth > 0){
            opq->loading.prepareDepth--;
        }
		//Add storage as viewer
		if(r){
            //both, this call and the async sotrage-load completed
            if(opq->loading.prepareDepth == 0 && opq->loading.storageDepth == 0){
                TSServer_loadEndedDoPostActionsLockedOpq_(opq);
            }
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

void TSServer_TSStreamsStorageLoadEndedFunc_(const BOOL isSuccess, void* usrData){
    STTSServerOpq* opq = (STTSServerOpq*)usrData;
    NBThreadMutex_lock(&opq->mutex);
    {
        //end-storage-load
        NBASSERT(opq->loading.storageDepth == 1)
        if(opq->loading.storageDepth > 0){
            opq->loading.storageDepth--;
        }
        //both, this call and the async sotrage-load completed
        if(opq->loading.prepareDepth == 0 && opq->loading.storageDepth == 0){
            TSServer_loadEndedDoPostActionsLockedOpq_(opq);
        }
        NBThreadCond_broadcast(&opq->cond);
    }
    NBThreadMutex_unlock(&opq->mutex);
}

void TSServer_loadEndedDoPostActionsLockedOpq_(STTSServerOpq* opq){
    NBASSERT(opq->context.cur != NULL)
    if(opq->context.cur != NULL){
        const STTSCfg* pCfg = TSContext_getCfg(opq->context.cur);
        //Add storages as viewers
        if(opq->streams.cfg.isReadOnly) {
            PRINTF_CONSOLE("------------------------------------------------------------\n");
            PRINTF_CONSOLE("- TSServer, storage in READ-ONLY mode; no added as viewers -.\n");
            PRINTF_CONSOLE("------------------------------------------------------------\n");
        } else if(pCfg->server.streamsGrps != NULL && pCfg->server.streamsGrpsSz > 0){
            UI32 i; for(i = 0; i < pCfg->server.streamsGrpsSz; i++){
                const STTSCfgStreamsGrp* grp = &pCfg->server.streamsGrps[i];
                if(!grp->isDisabled && grp->streams != NULL && grp->streamsSz > 0){
                    UI32 i; for(i = 0; i < grp->streamsSz; i++){
                        const STTSCfgStream* s = &grp->streams[i];
                        if(!s->isDisabled && s->versions != NULL && s->versionsSz > 0){
                            UI32 i; for(i = 0; i < s->versionsSz; i++){
                                STTSCfgStreamVersion* v = &s->versions[i];
                                if(!v->isDisabled){
                                    if(!NBString_strIsEmpty(s->device.uid) && !NBString_strIsEmpty(v->sid)){
                                        STTSStreamStorage* strgStream = TSStreamsStorage_getStreamRetained(&opq->streams.storage.mngr, s->device.uid, TRUE);
                                        if(strgStream == NULL){
                                            PRINTF_CONSOLE_ERROR("TSServer, TSStreamsStorage_getStreamRetained failed for '%s'.\n", s->device.uid);
                                        } else {
                                            STTSStreamVersionStorage* strgVersion = TSStreamStorage_getVersionRetained(strgStream, v->sid, TRUE);
                                            NBASSERT(strgVersion != NULL)
                                            if(strgVersion == NULL){
                                                PRINTF_CONSOLE_ERROR("TSServer, TSStreamStorage_getVersionRetained failed for '%s' / '%s'.\n", s->device.uid, v->sid);
                                            } else {
                                                const BOOL ignoreIfCurrentExists = TRUE;
                                                if(!TSStreamVersionStorage_setParams(strgVersion, (v->params != NULL ? &v->params->write : &pCfg->server.defaults.streams.params.write))){
                                                    PRINTF_CONSOLE_ERROR("TSServer, TSStreamVersionStorage_setParams failed for '%s'.\n", v->sid);
                                                    NBASSERT(FALSE);
                                                } else if(!TSStreamVersionStorage_createNextFilesGrp(strgVersion, ignoreIfCurrentExists)){
                                                    PRINTF_CONSOLE_ERROR("TSServer, TSStreamStorage, TSStreamVersionStorage_createNextFilesGrp failed for '%s'.\n", v->sid);
                                                } else {
                                                    //Add as viewer
                                                    STTSStreamLstnr viewer = TSStreamVersionStorage_getItfAsStreamLstnr(strgVersion);
                                                    STTSStreamAddress address;
                                                    NBMemory_setZeroSt(address, STTSStreamAddress);
                                                    address.server      = s->device.server;
                                                    address.port        = s->device.port;
                                                    address.useSSL      = s->device.useSSL;
                                                    address.uri         = v->uri;
                                                    address.user        = s->device.user;
                                                    address.pass        = s->device.pass;
                                                    if(!TSClientRequester_rtspAddViewer(opq->streams.reqs.mngr, &viewer, grp->uid, grp->name, s->device.uid, s->device.name, v->sid, v->sid, &address, (v->params != NULL ? v->params : &pCfg->server.defaults.streams.params))){
                                                        PRINTF_CONSOLE_ERROR("TSServer, TSClientRequester_rtspAddViewer failed for static viewer: '%s'.\n", v->uri);
                                                    } else {
                                                        PRINTF_CONSOLE("TSServer, storage added as viewers: '%s:%d/%s'\n", s->device.server, s->device.port, v->uri);
                                                    }
                                                }
                                                TSStreamStorage_returnVersion(strgStream, strgVersion);
                                            }
                                            TSStreamsStorage_returnStream(&opq->streams.storage.mngr, strgStream);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

BOOL TSServer_execute(STTSServer* obj, const BOOL* externalStopFlag){
	BOOL r = FALSE;
	STTSServerOpq* opq = (STTSServerOpq*)obj->opaque;
	if(opq->state.cur == ENTSServerState_prepared){
		r = TRUE;
		//stop-flag
		{
			opq->stopFlag.external	= externalStopFlag;
			opq->stopFlag.applied	= FALSE;
			opq->stopFlag.appliedToThread = FALSE;
		}
		//
		if(r){
			//set as running
			{
				NBThreadMutex_lock(&opq->mutex);
				opq->state.cur = ENTSServerState_running;
				NBThreadCond_broadcast(&opq->cond);
				NBThreadMutex_unlock(&opq->mutex);
			}
			//run (unlocked)
			{
				if(!TSContext_startAndUseThisThread(opq->context.cur)){
					PRINTF_CONSOLE_ERROR("TSServer, TSContext_startAndUseThisThread failed.\n");
					NBThreadMutex_lock(&opq->mutex);
					{
						opq->state.cur = ENTSServerState_prepared;
						NBThreadCond_broadcast(&opq->cond);
					}
					NBThreadMutex_unlock(&opq->mutex);
				} else {
					PRINTF_INFO("TSServer, stopped after %d secs from prepared-status (%d secs running).\n", opq->secsPrepared, opq->secsPrepared);
				}
				//stop
				{
					TSContext_stop(opq->context.cur);
				}
			}
			//wait while running
			{
				NBThreadMutex_lock(&opq->mutex);
				{
					while(opq->state.cur == ENTSServerState_running){
						NBThreadCond_wait(&opq->cond, &opq->mutex);
					}
				}
				NBThreadMutex_unlock(&opq->mutex);
			}
		}
		//stop-flag
		{
			opq->stopFlag.external	= NULL;
		}
		//
	}
	return r;
}

//Context

STTSContext* TSServer_getContext(STTSServer* obj){
	STTSServerOpq* opq = (STTSServerOpq*)obj->opaque;
	return opq->context.cur;
}

//Locale

const char* TSServer_getLocaleStrOpq(STTSServerOpq* opq, const char* lang, const char* strId, const char* defaultValue){
	return TSContext_getStrLang(opq->context.cur, lang, strId, defaultValue);
}

const char* TSServer_getLocaleStr(STTSServer* obj, const char* lang, const char* strId, const char* defaultValue){
	STTSServerOpq* opq = (STTSServerOpq*)obj->opaque;
	return TSServer_getLocaleStrOpq(opq, lang, strId, defaultValue);
}

//Re-encode

BOOL TSServer_reEncDataKeyOpq(STTSServerOpq* opq, const STTSCypherDataKey* keySrc, STTSCypherDataKey* keyDst, STNBX509* encCert){
	BOOL r = FALSE;
	if(opq == NULL || encCert == NULL){
		PRINTF_CONSOLE_ERROR("TSServer, TSServer_reEncDataKey, opq-or-encCert are NULL.\n");
	} else {
#		ifdef NB_CONFIG_INCLUDE_ASSERTS
		//PRINTF_INFO("TSServer, TSServer_reEncDataKey with opq(%llu) caKey(%llu) caKey-opq(%llu).\n", (UI64)opq, (UI64)opq->caKey, (UI64)opq->caKey->opaque);
#		endif
		const STNBStructMap* map = TSCypherDataKey_getSharedStructMap();
		STTSCypherDataKey plainKey;
		NBMemory_setZeroSt(plainKey, STTSCypherDataKey);
		if(!TSCypher_decDataKey(&plainKey, keySrc, opq->caKey)){
			PRINTF_CONSOLE_ERROR("TSServer, TSCypher_decDataKey failed.\n");
		} else {
			STNBString pass2, salt2;
			NBString_initWithSz(&pass2, keySrc->passSz + 8, 256, 0.10f);
			NBString_initWithSz(&salt2, keySrc->saltSz + 8, 256, 0.10f);
			{
				if(!NBX509_concatEncryptedBytes(encCert, plainKey.pass, plainKey.passSz, &pass2)){
					PRINTF_CONSOLE_ERROR("TSServer, NBX509_concatEncryptedBytes failed for pass.\n");
				} else {
					if(!NBX509_concatEncryptedBytes(encCert, plainKey.salt, plainKey.saltSz, &salt2)){
						PRINTF_CONSOLE_ERROR("TSServer, NBX509_concatEncryptedBytes failed for salt.\n");
					} else {
						//
						NBString_strFreeAndNewBufferBytes((char**)&keyDst->pass, pass2.str, pass2.length);
						keyDst->passSz = pass2.length;
						//
						NBString_strFreeAndNewBufferBytes((char**)&keyDst->salt, salt2.str, salt2.length);
						keyDst->saltSz	= salt2.length;
						//
						keyDst->iterations	= plainKey.iterations;
						//
						r = TRUE;
					}
				}
			}
			NBString_release(&pass2);
			NBString_release(&salt2);
		}
		NBStruct_stRelease(map, &plainKey, sizeof(plainKey));
	}
	return r;
}

//ToDo: remove
BOOL TSServer_reEncDataKey(STTSServer* obj, const STTSCypherDataKey* keySrc, STTSCypherDataKey* keyDst, STNBX509* encCert){
	BOOL r = FALSE;
	if(obj == NULL){
		PRINTF_CONSOLE_ERROR("TSServer, TSServer_reEncDataKey, obj is NULL.\n");
	} else {
		STTSServerOpq* opq = (STTSServerOpq*)obj->opaque;
		r = TSServer_reEncDataKeyOpq(opq, keySrc, keyDst, encCert);
	}
	return r;
}

//ToDo: remove
/*BOOL TSServer_decDataKeyWithCurrentKey(STTSServer* obj, STTSCypherDataKey* dstKeyPlain, const STTSCypherDataKey* keyEnc){
	BOOL r = FALSE;
	if(obj == NULL){
		PRINTF_CONSOLE_ERROR("TSServer, TSServer_reEncDataKey, obj is NULL.\n");
	} else {
		STTSServerOpq* opq = (STTSServerOpq*)obj->opaque;
		if(opq == NULL || opq->caKey == NULL){
			PRINTF_CONSOLE_ERROR("TSServer, TSServer_reEncDataKey, opq-or-encCert are NULL.\n");
		} else {
			if(!TSCypher_decDataKey(dstKeyPlain, keyEnc, opq->caKey)){
				PRINTF_CONSOLE_ERROR("TSServer, TSCypher_decDataKey failed.\n");
			} else {
				r = TRUE;
			}
		}
	}
	return r;
}*/

//SMS

BOOL TSServer_addSmsToLocalQueue(STTSServerOpq* opq, const char* to, const char* body){
	BOOL r = FALSE;
	if(opq == NULL){
		PRINTF_CONSOLE_ERROR("TSServer, TSServer_addSmsToLocalQueue, opq is NULL.\n");
	} else {
		BOOL reqBuilt = FALSE;
		STNBHttpRequest req;
		NBHttpRequest_init(&req);
		//Build request
		{
			const STTSCfg* cfg = TSContext_getCfg(opq->context.cur);
			STNBString str, strTmp, strTag, strVal;
			NBString_init(&str);
			NBString_init(&strTmp);
			NBString_init(&strTag);
			NBString_init(&strVal);
			//Add headers
			if(cfg->context.sms.headers != NULL && cfg->context.sms.headersSz > 0){
				SI32 i; for(i = 0; i < cfg->context.sms.headersSz; i++){
					const STTSCfgRestApiCltTemplate* tmpl = &cfg->context.sms.headers[i];
					NBString_set(&str, tmpl->value);
					//Replace tags
					if(tmpl->tags != NULL && tmpl->tagsSz > 0){
						SI32 i; for(i = 0; i < tmpl->tagsSz; i++){
							const STTSCfgRestApiCltTag* tag = &tmpl->tags[i];
							if(!NBString_strIsEmpty(tag->tag)){
								//Tag
								{
									NBString_empty(&strTag);
									NBString_concatByte(&strTag, '[');
									NBString_concat(&strTag, tag->tag);
									NBString_concatByte(&strTag, ']');
								}
								//Value
								{
									const char* valPlain = "";
									//ToDo: define
									{
										
									}
									if(tag->isJson){
										NBString_empty(&strVal);
										NBJson_concatScaped(&strVal, valPlain);
									} else {
										NBString_set(&strVal, valPlain);
									}
								}
								NBString_replace(&str, strTag.str, strVal.str);
							}
						}
					}
					NBHttpRequest_addHeader(&req, tmpl->name, str.str);
					//PRINTF_INFO("TSServer, TSServer_addSmsToLocalQueue, header added: '%s': '%s'.\n", tmpl->name, str.str);
				}
			}
			//Add body
			if(!NBString_strIsEmpty(cfg->context.sms.body.value)){
				NBString_set(&str, cfg->context.sms.body.value);
				//Replace tags
				if(cfg->context.sms.body.tags != NULL && cfg->context.sms.body.tagsSz > 0){
					SI32 i; for(i = 0; i < cfg->context.sms.body.tagsSz; i++){
						const STTSCfgRestApiCltTag* tag = &cfg->context.sms.body.tags[i];
						if(!NBString_strIsEmpty(tag->tag)){
							//Tag
							{
								NBString_empty(&strTag);
								NBString_concatByte(&strTag, '[');
								NBString_concat(&strTag, tag->tag);
								NBString_concatByte(&strTag, ']');
							}
							//Value
							{
								const char* valPlain = "";
								if(NBString_strIsEqual(tag->tag, "to")){
									NBString_empty(&strTmp);
									//Remove '+', '(', ')', and ' ' form number
									if(!NBString_strIsEmpty(to)){
										const char* c = to;
										while(*c != '\0'){
											if(*c != '+' && *c != '(' && *c != ')' && *c != ' '){
												NBString_concatByte(&strTmp, *c);
											}
											c++;
										}
									}
									valPlain = strTmp.str;
								} else if(NBString_strIsEqual(tag->tag, "body")){
									valPlain = body;
								}
								if(tag->isJson){
									NBString_empty(&strVal);
									NBJson_concatScaped(&strVal, valPlain);
								} else {
									NBString_set(&strVal, valPlain);
								}
							}
							NBString_replace(&str, strTag.str, strVal.str);
						}
					}
				}
				NBHttpRequest_setContent(&req, str.str);
				//PRINTF_INFO("TSServer, TSServer_addSmsToLocalQueue, content: '%s'.\n", str.str);
				reqBuilt = TRUE;
			}
			NBString_release(&strVal);
			NBString_release(&strTag);
			NBString_release(&strTmp);
			NBString_release(&str);
		}
		if(reqBuilt){
			/*
			TSApiRestClt_addRequest(&opq->smsClt, &req);
			*/
			r = TRUE;
		}
		NBHttpRequest_release(&req);
	}
	return r;
}

//Mail

BOOL TSServer_addMailToLocalQueue(STTSServerOpq* opq, const char* to, const char* subject, const char* body, const char* attachFilename, const char* attachMime, const void* attachData, const UI32 attachDataSz){
	return FALSE; //TSMailClt_addRequest(&opq->mailClt, to, subject, body, attachFilename, attachMime, attachData, attachDataSz);
}

//Notifications

BOOL TSServer_addAppleNotification(STTSServerOpq* opq, const char* token, const char* text, const char* extraDataName, const char* extraDataValue){
	BOOL r = FALSE;
	if(opq == NULL){
		PRINTF_CONSOLE_ERROR("TSServer, TSServer_addAppleNotification, opq is NULL.\n");
	} else {
		r = FALSE; //TSApplePushServicesClt_addNotification(&opq->applePushServices, token, text, extraDataName, extraDataValue);
	}
	return r;
}

BOOL TSServer_addAndroidNotification(STTSServerOpq* opq, const char* token, const char* text, const char* extraDataName, const char* extraDataValue){
	BOOL r = FALSE;
	if(opq == NULL){
		PRINTF_CONSOLE_ERROR("TSServer, TSServer_addAndroidNotification, opq is NULL.\n");
	} else {
		r = FALSE; //TSFirebaseCMClt_addNotification(&opq->firebaseCMClt, token, text, extraDataName, extraDataValue);
	}
	return r;
}

//Stats

void TSServerStats_getDataLockedOpq_(STTSServerOpq* opq, STTSServerStatsData* dst, STNBArray* dstPortsStats /*STNBRtpClientPortStatsData*/, STNBArray* dstSsrcsStats /*STNBRtpClientSsrcStatsData*/, const BOOL resetAccum){
#	ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
	const STTSCfg* cfg			= TSContext_getCfg(opq->context.cur);
#	endif
	STNBDataPtrsPoolRef bPool	= TSContext_getBuffersPool(opq->context.cur);
	STNBDataPtrsStatsRef bStats	= TSContext_getBuffersStats(opq->context.cur);
	//flush accumulated stats (atomicStats is disabled by default)
	NBDataPtrsPool_flushStats(bPool);
	//build stats
	const STNBDataPtrsStatsData buffsStats			= NBDataPtrsStats_getData(bStats, resetAccum);
	const STNBRtspClientStatsData rtspStats			= NBRtspClientStats_getData(&opq->rtsp.stats, resetAccum);
	const STTSClientRequesterConnStatsData reqsStats = TSClientRequesterConnStats_getData(&opq->streams.reqs.stats.conn, resetAccum);
	const STTSStreamsStorageStatsData storageStats	= TSStreamsStorageStats_getData(&opq->streams.storage.stats, resetAccum);
#	ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
	const STTSCfgTelemetryProcess* cfgProcess		= &cfg->server.telemetry.process;
	STNBProcessStatsData processStats;
	STNBProcessStatsData* threadStats			= NULL;
	UI32 threadStatsSz								= 0;
	NBProcessStatsData_init(&processStats);
	//load process stats (unlocked to avoid lock into stats)
	{
		NBThreadMutex_unlock(&opq->mutex);
		{
			NBMemory_setZero(processStats);
			if(cfgProcess->statsLevel > ENNBLogLevel_None){
				NBProcess_getGlobalStatsData(&processStats, cfgProcess->locksByMethod, resetAccum);
			}
			if(cfgProcess->threads != NULL && cfgProcess->threadsSz > 0){
				NBASSERT(threadStatsSz == 0)
				//Count threads with visible data
				{
					UI32 i; for(i = 0; i < cfgProcess->threadsSz; i++){
						const STTSCfgTelemetryThread* cfgT = &cfgProcess->threads[i];
						if(cfgT->statsLevel > ENNBLogLevel_None && !NBString_strIsEmpty(cfgT->firstKnownFunc)){
							threadStatsSz++;
						}
					}
				}
				//Allocate stats
				if(threadStatsSz > 0){
					UI32 threadStatsUse = 0;								
					threadStats = NBMemory_allocTypes(STNBProcessStatsData, threadStatsSz);
					{
						UI32 i; for(i = 0; i < cfgProcess->threadsSz; i++){
							const STTSCfgTelemetryThread* cfgT = &cfgProcess->threads[i];
							if(cfgT->statsLevel > ENNBLogLevel_None && !NBString_strIsEmpty(cfgT->firstKnownFunc)){
								STNBProcessStatsData* d = &threadStats[threadStatsUse++]; NBASSERT(threadStatsUse <= threadStatsSz)
								NBProcessStatsData_init(d);
								NBProcess_getThreadStatsDataByFirstKwnonFuncName(cfgT->firstKnownFunc, d, cfgT->locksByMethod, resetAccum);
							}
						}
					}
					NBASSERT(threadStatsUse == threadStatsSz)
				}
			}
		}
		NBThreadMutex_lock(&opq->mutex);
	}
#	endif
	//Process stats data
	{
		//loaded
		dst->loaded.buffs		= buffsStats.loaded;
		dst->loaded.rtsp		= rtspStats.loaded;
		dst->loaded.reqs		= reqsStats.loaded;
		dst->loaded.storage		= storageStats.loaded;
#		ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
		if(cfg->server.telemetry.process.statsLevel > ENNBLogLevel_None){
			dst->loaded.process.stats = processStats.alive;
			NBMemory_setZeroSt(processStats.alive, STNBProcessStatsState); //internal data will be released by 'TSServerStatsData'
		}
		if(threadStats != NULL && threadStatsSz > 0){
			dst->loaded.process.threadsSz	= threadStatsSz;
			dst->loaded.process.threads		= NBMemory_allocTypes(STTSServerStatsThreadState, threadStatsSz);
			{
				UI32 i; for(i = 0; i < threadStatsSz; i++){
					const STTSCfgTelemetryThread* cfgT	= &cfgProcess->threads[i];
					STNBProcessStatsData* src		= &threadStats[i];
					STTSServerStatsThreadState* dst2	= &dst->loaded.process.threads[i];
					NBMemory_setZeroSt(*dst2, STTSServerStatsThreadState);
					dst2->name				= cfgT->name;
					dst2->firstKnownMethod	= cfgT->firstKnownFunc;
					dst2->statsLevel		= cfgT->statsLevel;
					dst2->stats				= src->alive; 
					NBMemory_setZeroSt(src->alive, STNBProcessStatsState); //internal data will be released by 'TSServerStatsData'
				}
			}
		}
#		endif
		//accum
		dst->accum.buffs	= buffsStats.accum;
		dst->accum.rtsp	= rtspStats.accum;
		dst->accum.reqs	= reqsStats.accum;
		dst->accum.storage	= storageStats.accum;
#		ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
		if(cfg->server.telemetry.process.statsLevel > ENNBLogLevel_None){
			dst->accum.process.stats = processStats.accum;
			NBMemory_setZeroSt(processStats.accum, STNBProcessStatsState); //internal data will be released by 'TSServerStatsData'
		}
		if(threadStats != NULL && threadStatsSz > 0){
			dst->accum.process.threadsSz	= threadStatsSz;
			dst->accum.process.threads		= NBMemory_allocTypes(STTSServerStatsThreadState, threadStatsSz);
			{
				UI32 i; for(i = 0; i < threadStatsSz; i++){
					const STTSCfgTelemetryThread* cfgT	= &cfgProcess->threads[i];
					STNBProcessStatsData* src		= &threadStats[i];
					STTSServerStatsThreadState* dst2	= &dst->accum.process.threads[i];
					NBMemory_setZeroSt(*dst2, STTSServerStatsThreadState);
					dst2->name				= cfgT->name;
					dst2->firstKnownMethod	= cfgT->firstKnownFunc;
					dst2->statsLevel		= cfgT->statsLevel;
					dst2->stats				= src->accum;
					NBMemory_setZeroSt(src->accum, STNBProcessStatsState); //internal data will be released by 'TSServerStatsData'
				}
			}
		}
#		endif
		//total
		dst->total.buffs	= buffsStats.total;
		dst->total.rtsp	= rtspStats.total;
		dst->total.reqs	= reqsStats.total;
		dst->total.storage	= storageStats.total;
#		ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
		if(cfg->server.telemetry.process.statsLevel > ENNBLogLevel_None){
			dst->total.process.stats = processStats.total;
			NBMemory_setZeroSt(processStats.total, STNBProcessStatsState); //internal data will be released by 'TSServerStatsData'
		}
		if(threadStats != NULL && threadStatsSz > 0){
			dst->total.process.threadsSz	= threadStatsSz;
			dst->total.process.threads		= NBMemory_allocTypes(STTSServerStatsThreadState, threadStatsSz);
			{
				UI32 i; for(i = 0; i < threadStatsSz; i++){
					const STTSCfgTelemetryThread* cfgT	= &cfgProcess->threads[i];
					STNBProcessStatsData* src		= &threadStats[i];
					STTSServerStatsThreadState* dst2	= &dst->total.process.threads[i];
					NBMemory_setZeroSt(*dst2, STTSServerStatsThreadState);
					dst2->name				= cfgT->name;
					dst2->firstKnownMethod	= cfgT->firstKnownFunc;
					dst2->statsLevel		= cfgT->statsLevel;
					dst2->stats				= src->total;
					NBMemory_setZeroSt(src->total, STNBProcessStatsState); //internal data will be released by 'TSServerStatsData'
				}
			}
		}
#		endif
	}
	//
#	ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
	{
		//threads
		{
			if(threadStats != NULL){
				if(threadStatsSz > 0){
					UI32 i; for(i = 0; i < threadStatsSz; i++){
						STNBProcessStatsData* d = &threadStats[i];
						NBProcessStatsData_release(d);
					}
				}
				NBMemory_free(threadStats);
				threadStats = NULL;
			}
			threadStatsSz = 0;
		}
		//process
		NBProcessStatsData_release(&processStats);
	}
#	endif
}

void TSServerStats_concatDataStreamsLocked_(STTSServerOpq* opq, const STTSServerStatsData* obj, const STTSCfgTelemetry* cfg, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, const STNBRtpClientPortStatsData* portsStats, const UI32 portsStatsSz, const STNBRtpClientSsrcStatsData* ssrcsStats, const UI32 ssrcsStatsSz, STNBString* dst){
	if(dst != NULL){
		BOOL opened = FALSE;
		const char* preExtra = "        |- ";
		STNBString str, pre;
		NBString_initWithSz(&str, 0, 256, 0.10f);
		NBString_initWithSz(&pre, NBString_strLenBytes(prefixOthers) + NBString_strLenBytes(preExtra) + 1, 256, 0.10f);
		NBString_concat(&pre, prefixOthers);
		NBString_concat(&pre, preExtra);
		if(loaded){
			NBString_empty(&str);
			TSServerStats_concatStateStreamsLocked_(opq, &obj->loaded, cfg, ENTSServerStartsGrp_Loaded, "", pre.str, ignoreEmpties, portsStats, portsStatsSz, ssrcsStats, ssrcsStatsSz, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "loaded:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		if(accum){
			NBString_empty(&str);
			TSServerStats_concatStateStreamsLocked_(opq, &obj->accum, cfg, ENTSServerStartsGrp_Accum, "", pre.str, ignoreEmpties, portsStats, portsStatsSz, ssrcsStats, ssrcsStatsSz, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  " accum:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		if(total){
			NBString_empty(&str);
			TSServerStats_concatStateStreamsLocked_(opq, &obj->total, cfg, ENTSServerStartsGrp_Total, "", pre.str, ignoreEmpties, portsStats, portsStatsSz, ssrcsStats, ssrcsStatsSz, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  " total:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		NBString_release(&pre);
		NBString_release(&str);
	}
}

void TSServerStats_concatStateStreamsLocked_(STTSServerOpq* opq, const STTSServerStatsState* obj, const STTSCfgTelemetry* cfg, const ENTSServerStartsGrp grp, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, const STNBRtpClientPortStatsData* portsStats, const UI32 portsStatsSz, const STNBRtpClientSsrcStatsData* ssrcStats, const UI32 ssrcStatsSz, STNBString* dst){
	if(dst != NULL){
		BOOL opened = FALSE;
		const char* preExtra = "       |- ";
		STNBString str, pre;
		NBString_initWithSz(&str, 0, 256, 0.10f);
		NBString_initWithSz(&pre, NBString_strLenBytes(prefixOthers) + NBString_strLenBytes(preExtra) + 1, 256, 0.10f);
		NBString_concat(&pre, prefixOthers);
		NBString_concat(&pre, preExtra);
        //files
#       ifdef CONFIG_NB_INCLUDE_THREADS_ENABLED
        if(grp == ENTSServerStartsGrp_Loaded){
            BOOL opened2 = FALSE;
            SI32 i; SI64 countUnnamed = 0, counts[ENNBHndlNativeType_Count];
            NBProcess_getHndlsByTypesCounts(counts, ENNBHndlNativeType_Count);
            for(i = 0; i <= ENNBHndlNativeType_Count; i++){
                SI64 count = 0; const char* name = "";
                if(i == ENNBHndlNativeType_Count){
                    count = countUnnamed;
                    name = "others";
                } else {
                    count = counts[i];
                    switch (i) {
                        case ENNBHndlNativeType_Undef:
                            name = "undef";
                            break;
                        case ENNBHndlNativeType_Socket:
                            name = "sckt";
                            break;
                        case ENNBHndlNativeType_FileWin:
                            name = "file-win";
                            break;
                        case ENNBHndlNativeType_FileStd:
                            name = "file-std";
                            break;
                        case ENNBHndlNativeType_FilePosix:
                            name = "file-psx";
                            break;
                        default:
                            //Implement
                            NBASSERT(FALSE)
                            count = 0;
                            countUnnamed += count;
                            break;
                    }
                }
                if(count > 0 && name != NULL){
                    if(!opened2){
                        if(opened){
                            NBString_concat(dst, "\n");
                            NBString_concat(dst, prefixOthers);
                        } else {
                            NBString_concat(dst, prefixFirst);
                        }
                        NBString_concat(dst,  "  hndls:  ");
                        opened2 = TRUE;
                    } else {
                        NBString_concat(dst,  ", ");
                    }
                    NBString_concatSI64(dst, count);
                    NBString_concat(dst, " ");
                    NBString_concat(dst, name);
                    opened = TRUE;
                }
            }
        }
#       endif
		//buffs
		{
			NBString_empty(&str);
			NBDataPtrsStats_concatState(&obj->buffs, &cfg->buffers, (grp == ENTSServerStartsGrp_Loaded), "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "  buffs:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		//rtsp
		{
			NBString_empty(&str);
			NBRtspClientStats_concatState(&obj->rtsp, &cfg->streams.rtsp, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "   rtsp:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		//reqs
		{
			NBString_empty(&str);
			TSClientRequesterConnStats_concatState(&obj->reqs, &cfg->streams, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "   reqs:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		//storage
		{
			NBString_empty(&str);
			TSStreamsStorageStats_concatState(&obj->storage, &cfg->filesystem, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "storage:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		//threads
		{
#			ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
			NBString_empty(&str);
			TSServerStats_concatProcessState(&obj->process, cfg->process.statsLevel, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "threads:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
#			endif
		}
		//ports
		if(portsStats != NULL && portsStatsSz > 0){
			UI32 i; for(i = 0; i < portsStatsSz; i++){
				const STNBRtpClientPortStatsData* port = &portsStats[i];
				NBString_empty(&str);
				NBRtpClientStats_concatState((grp == ENTSServerStartsGrp_Total ? &port->data.total : grp == ENTSServerStartsGrp_Accum ? &port->data.accum : &port->data.loaded), &cfg->streams.rtsp.rtp, "", pre.str, ignoreEmpties, &str);
				if(str.length > 0){
					if(opened){
						NBString_concat(dst, "\n");
						NBString_concat(dst, prefixOthers);
					} else {
						NBString_concat(dst, prefixFirst);
					}
					//name
					{
						NBString_concat(dst,  "port-");
						NBString_concatUI32(dst, (UI32)port->port);
					}
					NBString_concat(dst,  ": ");
					NBString_concat(dst, str.str);
					opened = TRUE;
				}
			}
		}
		//ssrcs
		if(ssrcStats != NULL && ssrcStatsSz > 0){
			UI32 i; for(i = 0; i < ssrcStatsSz; i++){
				const STNBRtpClientSsrcStatsData* ssrc = &ssrcStats[i];
				NBString_empty(&str);
				NBRtpClientStats_concatState((grp == ENTSServerStartsGrp_Total ? &ssrc->data.total : grp == ENTSServerStartsGrp_Accum ? &ssrc->data.accum : &ssrc->data.loaded), &cfg->streams.rtsp.rtp, "", pre.str, ignoreEmpties, &str);
				if(str.length > 0){
					if(opened){
						NBString_concat(dst, "\n");
						NBString_concat(dst, prefixOthers);
					} else {
						NBString_concat(dst, prefixFirst);
					}
					//name
					{
						if(!TSClientRequester_rtspConcatStreamVersionIdBySsrc(opq->streams.reqs.mngr, ssrc->ssrc, dst)){
							NBString_concat(dst,  "ssrc-");
							NBString_concatUI32(dst, (UI32)ssrc->ssrc);
						}
					}
					NBString_concat(dst,  ": ");
					NBString_concat(dst, str.str);
					opened = TRUE;
				}
			}
		}
		NBString_release(&pre);
		NBString_release(&str);
	}
}

#ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
void TSServerStats_concatProcessState(const STTSServerStatsProcessState* obj, const ENNBLogLevel logLvl, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL){
		BOOL opened = FALSE;
		const char* preExtra = "       |- ";
		STNBString str, pre;
		NBString_initWithSz(&str, 0, 256, 0.10f);
		NBString_initWithSz(&pre, NBString_strLenBytes(prefixOthers) + NBString_strLenBytes(preExtra) + 1, 256, 0.10f);
		NBString_concat(&pre, prefixOthers);
		NBString_concat(&pre, preExtra);
		//global
		{
			NBString_empty(&str);
			NBProcess_concatStatsState(&obj->stats, logLvl, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "global:  ");
				if(str.length <= 0){
					NBString_concat(dst, "(empty)");
				} else {
					NBString_concat(dst, str.str);
				}
				opened = TRUE;
			}
		}
		//threads
		if(obj->threads != NULL && obj->threadsSz > 0){
			UI32 i; for(i = 0; i < obj->threadsSz; i++){
				const STTSServerStatsThreadState* t = &obj->threads[i];
				NBString_empty(&str);
				NBProcess_concatStatsState(&t->stats, t->statsLevel, "", pre.str, ignoreEmpties, &str);
				if(str.length > 0){
					if(opened){
						NBString_concat(dst, "\n");
						NBString_concat(dst, prefixOthers);
					} else {
						NBString_concat(dst, prefixFirst);
					}
					NBString_concat(dst, t->name);
					NBString_concat(dst,  ": ");
					if(str.length <= 0){
						NBString_concat(dst, "(empty)");
					} else {
						NBString_concat(dst, str.str);
					}
					opened = TRUE;
				}
			}
		}
		NBString_release(&pre);
		NBString_release(&str);
	}
}
#endif


//HttpServiceLstnr

BOOL TSServer_httpCltConnected_(STNBHttpServiceRef srv, const UI32 port, STNBHttpServiceConnRef conn, void* param){
	BOOL r = TRUE;
	//STTSServerOpq* opq = (STTSServerOpq*)param;
	return r;
}

void TSServer_httpCltDisconnected_(STNBHttpServiceRef srv, const UI32 port, STNBHttpServiceConnRef conn, void* param){
	//STTSServerOpq* opq = (STTSServerOpq*)param;
}

BOOL TSServer_httpCltReqArrived_(STNBHttpServiceRef srv, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk, void* usrData){ //called when header-frist-line arrived, when header completion arrived or when body completing arrived; first to populate required methods into 'dstLtnr' take ownership and stops further calls to this method.
    BOOL r = FALSE, execute = TRUE;
    STTSServerOpq* opq = (STTSServerOpq*)usrData;
    NBThreadMutex_lock(&opq->mutex);
    {
        //Wait if server is preparing
        while(opq->state.cur >= ENTSServerState_preparing && opq->state.cur <= ENTSServerState_prepared){
            NBThreadCond_wait(&opq->cond, &opq->mutex);
        }
        //Process
        if(opq->state.cur != ENTSServerState_running){
            r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 500, "Server is not runnig")
                && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Server is not runnig");
            execute = FALSE;
        }
    }
    NBThreadMutex_unlock(&opq->mutex);
    //execute (unlocked)
    if(execute){
        if(
           NBString_strIndexOfLike(reqDesc.firstLine.target, "/stream.h.264", 0) >= 0 || NBString_strIndexOfLike(reqDesc.firstLine.target, "/stream.mp4", 0) >= 0
           //ToDo: remove
           //|| NBString_strIndexOfLike(reqDesc.firstLine.target, "/stream", 0) >= 0 || NBString_strIndexOfLike(reqDesc.firstLine.target, "/stream/", 0) >= 0
           //
           || NBString_strIndexOfLike(reqDesc.firstLine.target, "/session", 0) >= 0 || NBString_strIndexOfLike(reqDesc.firstLine.target, "/session/", 0) >= 0
           )
        {
            r = TSServerStreams_cltRequested(&opq->streams.mngr, port, conn, reqDesc, reqLnk);
        } else {
            r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 200, "OK")
                && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Hola pinolero! TSServer ejecutando!");
        }
    }
    //return
    return r; //(r == TRUE) =  keep the TCP connection
}




