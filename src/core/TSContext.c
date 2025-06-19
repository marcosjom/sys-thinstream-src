//
//  TSContext.c
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/TSContext.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBString.h"
#include "nb/core/NBArray.h"
#include "nb/files/NBFilesystem.h"
#include "nb/files/NBFilesystemPkgs.h"
#include "core/TSThread.h"
//
#include <stdlib.h>	//for rand()

//Types of devices

STNBEnumMapRecord TSDeviceType_sharedEnumMapRecs[] = {
	{ ENTSDeviceType_Unknown, "ENTSDeviceType_Unknown", "unk" }
	, { ENTSDeviceType_Computer, "ENTSDeviceType_Computer", "dsk" }
	, { ENTSDeviceType_Handled, "ENTSDeviceType_Handled", "hnd" }
	, { ENTSDeviceType_Tablet, "ENTSDeviceType_Tablet", "tblt" }
};

STNBEnumMap TSDeviceType_sharedEnumMap = {
	"ENTSDeviceType"
	, TSDeviceType_sharedEnumMapRecs
	, (sizeof(TSDeviceType_sharedEnumMapRecs) / sizeof(TSDeviceType_sharedEnumMapRecs[0]))
};

const STNBEnumMap* TSDeviceType_getSharedEnumMap(void){
	return &TSDeviceType_sharedEnumMap;
}

//Family of devices

STNBEnumMapRecord TSDeviceFamily_sharedEnumMapRecs[] = {
	{ ENTSDeviceFamily_Unknown, "ENTSDeviceFamily_Unknown", "unk" }
	, { ENTSDeviceFamily_Apple, "ENTSDeviceFamily_Apple", "apl" }
	, { ENTSDeviceFamily_Windows, "ENTSDeviceFamily_Windows", "win" }
	, { ENTSDeviceFamily_Android, "ENTSDeviceFamily_Android", "and" }
};

STNBEnumMap TSDeviceFamily_sharedEnumMap = {
	"ENTSDeviceFamily"
	, TSDeviceFamily_sharedEnumMapRecs
	, (sizeof(TSDeviceFamily_sharedEnumMapRecs) / sizeof(TSDeviceFamily_sharedEnumMapRecs[0]))
};

const STNBEnumMap* TSDeviceFamily_getSharedEnumMap(void){
	return &TSDeviceFamily_sharedEnumMap;
}

//-----------
//- TSContext
//-----------

typedef struct STTSContextOpq_ {
	STTSFilesystem	fs;
	STNBX509*		caCert;
	STNBLocale		locale;
	//log
	struct {
		STNBString	pend;			//lines to report
		STNBString	filepath;		//file path analized
		SI64		bytesAnalzd;	//position of bytes analyzed
	} log;
	STNBString		runId;			//runId
	BOOL			stopFlag;
    BOOL*           stopInterrupt;  //parent interruption flag
	//cfg
	struct {
		STTSCfg		data;			//cfg
	} cfg;
	//buffers
	struct {
		STNBDataPtrsPoolRef		pool;
		STNBDataPtrsStatsRef	stats;
	} buffers;
	//threads
	struct {
		STNBArray	arr; 	//STTSThread
		BOOL		areRunning;
		UI32		iNextPollster;	//next thread to be used (by 'pollsters.provider')
	} threads;
	//pollsters
	struct {
		STNBIOPollstersProviderRef provider;
	} pollsters;
	//lstnrs
	struct {
		STNBArray	arr;	//STTSContextLstnr
	} lstnrs;
} STTSContextOpq;

//

void TSContext_stopFlagOpq_(STTSContextOpq* opq);
BOOL TSContext_stopFlagThreadIfIsCurrentOpq_(STTSContextOpq* opq);
void TSContext_stopOpq_(STTSContextOpq* opq);
BOOL TSContext_loadLocaleFilepathOpq_(STTSContextOpq* opq, const char* localeFilePath);
BOOL TSContext_loadLocaleFilepathAtRootOpq_(STTSContextOpq* opq, STNBFilesystem* fs, const ENNBFilesystemRoot root, const char* localeFilePath);

STNBIOPollsterRef TSContext_getPollsterOpq_(void* usrData); //returns a pollster to use
STNBIOPollsterSyncRef TSContext_getPollsterSyncOpq_(void* usrData); //returns a pollster to use

//

#if defined(__ANDROID__)
void TSContext_init(STTSContext* obj, const STNBAndroidJniItf* jniItf, void* jniObj)
#else
void TSContext_init(STTSContext* obj)
#endif
{
	STTSContextOpq* opq = obj->opaque = NBMemory_allocType(STTSContextOpq);
	NBMemory_setZeroSt(*opq, STTSContextOpq);
#	if defined(__ANDROID__)
	TSFilesystem_init(&opq->fs, jniItf, jniObj);
#	else
	TSFilesystem_init(&opq->fs);
#	endif
	opq->caCert	= NULL;
	NBLocale_init(&opq->locale);
	//Log
	{
		NBString_initWithSz(&opq->log.pend, 4096, 4096, 0.10f);
		NBString_init(&opq->log.filepath);
		opq->log.bytesAnalzd = 0;
	}
	//runId
	{
		
		STNBSha1 hash;
		NBSha1_init(&hash);
		NBSha1_feedStart(&hash);
		//Timestamp
		{
			const UI64 now = NBDatetime_getCurUTCTimestamp();
			NBSha1_feed(&hash, &now, sizeof(now));
		}
		//Random
		{
			SI32 i; for(i = 0; i < 32; i++){
				const SI32 v = rand(); 
				NBSha1_feed(&hash, &v, sizeof(v));
			}
		}
		NBSha1_feedEnd(&hash);
		//Hex value
		{
			const STNBSha1HashHex hex = NBSha1_getHashHex(&hash);
			NBString_initWithStr(&opq->runId, hex.v);
		}
		NBSha1_release(&hash);
	}
	//cfg
	//already zeroed
	//buffers
	{
		opq->buffers.pool = NBDataPtrsPool_alloc(NULL);
		opq->buffers.stats = NBDataPtrsStats_alloc(NULL);
	}
	//threads
	{
		NBArray_init(&opq->threads.arr, sizeof(STTSThread), NULL);
	}
	//pollsters
	{
		opq->pollsters.provider =  NBIOPollstersProvider_alloc(NULL);
		//cfg
		{
			STNBIOPollstersProviderItf itf;
			NBMemory_setZeroSt(itf, STNBIOPollstersProviderItf);
			itf.getPollster = TSContext_getPollsterOpq_;
			itf.getPollsterSync = TSContext_getPollsterSyncOpq_;
			NBIOPollstersProvider_setItf(opq->pollsters.provider, &itf, opq);
		}
	}
	//lstnrs
	{
		NBArray_init(&opq->lstnrs.arr, sizeof(STTSContextLstnr), NULL);
	}
}

void TSContext_release(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	//stop threads
	{
		TSContext_stopOpq_(opq);
	}
	//runId
	{
		NBString_release(&opq->runId);
	}
	//pollsters
	{
		if(NBIOPollstersProvider_isSet(opq->pollsters.provider)){
			NBIOPollstersProvider_release(&opq->pollsters.provider);
			NBIOPollstersProvider_null(&opq->pollsters.provider);
		}
	}
	//threads
	{
		//arr
		{
			SI32 i; for(i = 0; i < opq->threads.arr.use; i++){
				STTSThread* t = NBArray_itmPtrAtIndex(&opq->threads.arr, STTSThread, i);
				TSThread_release(t);
			}
			NBArray_empty(&opq->threads.arr);
			NBArray_release(&opq->threads.arr);
		}
	}
	//buffers
	{
		 NBDataPtrsPool_release(&opq->buffers.pool);
		 NBDataPtrsStats_release(&opq->buffers.stats);
	}
	//lstnrs
	{
		NBArray_empty(&opq->lstnrs.arr);
		NBArray_release(&opq->lstnrs.arr);
	}
	//cfg
	NBStruct_stRelease(TSCfg_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
	//
	TSFilesystem_release(&opq->fs);
	if(opq->caCert != NULL){
		NBMemory_free(opq->caCert);
		opq->caCert = NULL;
	}
	NBLocale_release(&opq->locale);
	//Log
	{
		NBString_release(&opq->log.pend);
		NBString_release(&opq->log.filepath);
		opq->log.bytesAnalzd = 0;
	}
	NBMemory_free(opq);
	obj->opaque = NULL;
}

//pollster

STNBIOPollsterRef TSContext_getPollsterOpq_(void* usrData){ //returns a pollster to use
	STTSContextOpq* opq = (STTSContextOpq*)usrData;
	STNBIOPollsterRef r = NB_OBJREF_NULL;
	if(opq->threads.arr.use > 0){
		const SI32 iNextStart = (opq->threads.iNextPollster % opq->threads.arr.use);
		do {
			STTSThread* t = NBArray_itmPtrAtIndex(&opq->threads.arr, STTSThread, (opq->threads.iNextPollster % opq->threads.arr.use));
			r = TSThread_getPollster(t);
			opq->threads.iNextPollster = (opq->threads.iNextPollster + 1) % opq->threads.arr.use;
		} while(!NBIOPollster_isSet(r) && opq->threads.iNextPollster != iNextStart);
	}
	return r;
}

STNBIOPollsterSyncRef TSContext_getPollsterSyncOpq_(void* usrData){ //returns a pollster to use
	STTSContextOpq* opq = (STTSContextOpq*)usrData;
	STNBIOPollsterSyncRef r = NB_OBJREF_NULL;
	if(opq->threads.arr.use > 0){
		const SI32 iNextStart = (opq->threads.iNextPollster % opq->threads.arr.use);
		do {
			STTSThread* t = NBArray_itmPtrAtIndex(&opq->threads.arr, STTSThread, (opq->threads.iNextPollster % opq->threads.arr.use));
			r = TSThread_getPollsterSync(t);
			opq->threads.iNextPollster = (opq->threads.iNextPollster + 1) % opq->threads.arr.use;
		} while(!NBIOPollsterSync_isSet(r) && opq->threads.iNextPollster != iNextStart);
	}
	return r;
}

//Desc

ENTSDeviceType TSContext_getDeviceType(void){
	ENTSDeviceType r = ENTSDeviceType_Computer;
#	if defined(_WIN32)
	r = ENTSDeviceType_Computer;
#	elif defined(__ANDROID__)
	r = ENTSDeviceType_Tablet;
#	elif defined(__QNX__)
	r = ENTSDeviceType_Tablet;
#	elif defined(__APPLE__)
#		if (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE) || (defined(TARGET_OS_IOS) && TARGET_OS_IOS)
		r = ENTSDeviceType_Tablet;
#		else
		r = ENTSDeviceType_Computer;
#		endif
#	else
	r = ENTSDeviceType_Computer;
#	endif
	return r;
}

ENTSDeviceFamily TSContext_getDeviceFamily(void){
	ENTSDeviceFamily r = ENTSDeviceFamily_Windows;
#	if defined(_WIN32)
	r = ENTSDeviceFamily_Windows;
#	elif defined(__ANDROID__)
	r = ENTSDeviceFamily_Android;
#	elif defined(__QNX__)
	r = ENTSDeviceFamily_Windows;
#	elif defined(__APPLE__)
	r = ENTSDeviceFamily_Apple;
#	else
	r = ENTSDeviceFamily_Windows;
#	endif
	return r;
}

//runId

const char* TSContext_getRunId(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return opq->runId.str;
}

//buffers

STNBDataPtrsPoolRef TSContext_getBuffersPool(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return opq->buffers.pool;
}

STNBDataPtrsStatsRef TSContext_getBuffersStats(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return opq->buffers.stats;
}

//pollsters

STNBIOPollstersProviderRef TSContext_getPollstersProvider(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return opq->pollsters.provider;
}

//Lstnrs

BOOL TSContext_addLstnr(STTSContext* obj, STTSContextLstnrItf* itf, void* usrData){
	BOOL r = FALSE;
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	if(!opq->threads.areRunning){
		STTSContextLstnr lstnr;
		NBMemory_setZeroSt(lstnr, STTSContextLstnr);
		lstnr.itf		= *itf;
		lstnr.usrData	= usrData;
		NBArray_addValue(&opq->lstnrs.arr, lstnr);
		r = TRUE;
	}
	return r;
}

BOOL TSContext_removeLstnr(STTSContext* obj, void* usrData){
	BOOL r = FALSE;
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	if(!opq->threads.areRunning){
		SI32 i; for(i = 0;  i < opq->lstnrs.arr.use; i++){
			STTSContextLstnr* lstnr = NBArray_itmPtrAtIndex(&opq->lstnrs.arr, STTSContextLstnr, i);
			if(lstnr->usrData == usrData){
				NBArray_removeItemAtIndex(&opq->lstnrs.arr, i);
				r = TRUE;
				break;
			}
		}
	}
	return r;
}

//Config

const STTSCfg* TSContext_getCfg(const STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return &opq->cfg.data;
}

BOOL TSContext_prepare(STTSContext* obj, const STTSCfg* pCfg){
	BOOL r = TRUE;
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	const STNBStructMap* fsDefMap = TSCfgFilesystemStreamsParams_getSharedStructMap();
	STTSCfgFilesystemStreamsParams fsDef;
	NBMemory_setZeroSt(fsDef, STTSCfgFilesystemStreamsParams);
	//stream params
	if(r){
		if(!TSCfgFilesystemStreamsParams_cloneNormalized(&fsDef, &pCfg->context.filesystem.streams.defaults)){
			PRINTF_CONSOLE_ERROR("TSServer, TSCfgFilesystemStreamsParams_cloneNormalized failed.\n");
			r = FALSE;
		}
	}
	//Filesystem
	if(r){
		if(!TSFilesystem_loadFromConfig(&opq->fs, &pCfg->context.filesystem)){
			PRINTF_CONSOLE_ERROR("TSContext, TSFilesystem_loadFromConfig failed.\n");
			r = FALSE;
		}
	}
	//Load locale (if defined)
	if(r){
		if(pCfg->context.locale.paths != NULL && pCfg->context.locale.pathsSz > 0){
			UI32 i; for(i = 0; i < pCfg->context.locale.pathsSz; i++){
				const char* path = pCfg->context.locale.paths[i];
				if(!NBString_strIsEmpty(path)){
					if(!TSContext_loadLocaleFilepathOpq_(opq, path)){
						PRINTF_CONSOLE_ERROR("TSServer, Server could not load locale file: '%s'.\n", path);
						r = FALSE;
					} else {
						PRINTF_INFO("TSServer, Server loaded locale: '%s'.\n", path);
					}
				}
			}
		}
	}
	//Prepare buffers
	if(r){
		STNBDataPtrsStatsRef buffsStats = NB_OBJREF_NULL;
		if(pCfg->server.telemetry.buffers.statsLevel > ENNBLogLevel_None){
			buffsStats = opq->buffers.stats;
		}
		if(!NBDataPtrsPool_setStatsProvider(opq->buffers.pool, buffsStats)){
			PRINTF_CONSOLE_ERROR("TSServer, NBDataPtrsPool_setStatsProvider failed.\n");
			r = FALSE;
		} else if(!NBDataPtrsPool_setCfg(opq->buffers.pool, &pCfg->server.buffers)){
			PRINTF_CONSOLE_ERROR("TSServer, NBDataPtrsPool_setCfg failed.\n");
			r = FALSE;
		}
	}
	//Prepare threads arr
	if(r){
		if(pCfg->context.threads.amount <= 0){
			PRINTF_CONSOLE_ERROR("TSServer, 'threads/amount' muts be greather than zero.\n");
			r = FALSE;
		}
		if(pCfg->context.threads.polling.timeout <= 0){
			PRINTF_CONSOLE_ERROR("TSServer, 'threads/polling/timeout' muts be greather than zero.\n");
			r = FALSE;
		}
		if(r){
			//prepare threads
			UI32 i; for(i = 0; i < pCfg->context.threads.amount; i++){
				STTSThread t;
				TSThread_init(&t);
				if(!TSThread_prepare(&t, (i == 0 ? &opq->lstnrs.arr : NULL), i, &pCfg->context.threads)){
					PRINTF_CONSOLE_ERROR("TSServer, TSThread_prepare failed for thread #%d.\n", (i + 1));
					r = FALSE;
					break;
				} else {
					NBArray_addValue(&opq->threads.arr, t);
				}
			}
			//stop threads (if failed)
			if(!r){
				TSContext_stopOpq_(opq);
				//remove all
				NBArray_empty(&opq->threads.arr);
			}
		}
	}
	//clone cfg
	if(r){
		const STNBStructMap* cfgMap = TSCfg_getSharedStructMap();
		NBStruct_stRelease(cfgMap, &opq->cfg.data, sizeof(opq->cfg.data));
		NBStruct_stClone(cfgMap, pCfg, sizeof(*pCfg), &opq->cfg.data, sizeof(opq->cfg.data));
		//Apply interpreted values
		{
			NBStruct_stRelease(fsDefMap, &opq->cfg.data.context.filesystem.streams.defaults, sizeof(opq->cfg.data.context.filesystem.streams.defaults));
			NBStruct_stClone(fsDefMap, &fsDef, sizeof(fsDef), &opq->cfg.data.context.filesystem.streams.defaults, sizeof(opq->cfg.data.context.filesystem.streams.defaults));
		}
	}
	//
	{
		NBStruct_stRelease(fsDefMap, &fsDef, sizeof(fsDef));
	}
	//
	return r;
}

BOOL TSContext_start(STTSContext* obj){
	BOOL r = FALSE;
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	//threads
	if(!opq->threads.areRunning && opq->threads.arr.use > 0){
		r = opq->threads.areRunning = TRUE;
		//start threads
		{
			SI32 i;
			//stop signal
			for(i = 0; i < opq->threads.arr.use; i++){
				STTSThread* t = NBArray_itmPtrAtIndex(&opq->threads.arr, STTSThread, i);
				if(!TSThread_start(t)){
					PRINTF_CONSOLE_ERROR("TSContext, TSThread_start failed for thread #%d.\n", (i + 1));
					r = FALSE;
					break;
				}
			}
		}
		//stop threads (if failed)
		if(!r){
			TSContext_stopOpq_(opq);
			opq->threads.areRunning = FALSE;
		}
	}
	return r;
}

BOOL TSContext_startAndUseThisThread(STTSContext* obj){
	BOOL r = FALSE;
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	//threads
	if(!opq->threads.areRunning && opq->threads.arr.use > 0){
		r = opq->threads.areRunning = TRUE;
		//start threads
		{
			SI32 i;
			//stop signal
			for(i = (SI32)opq->threads.arr.use - 1; i >= 0; i--){
				STTSThread* t = NBArray_itmPtrAtIndex(&opq->threads.arr, STTSThread, i);
				if(i != 0){
					//start own thread
					if(!TSThread_start(t)){
						PRINTF_CONSOLE_ERROR("TSContext, TSThread_start failed for thread #%d.\n", (i + 1));
						r = FALSE;
						break;
					}
				} else {
					//use this thread
					if(!TSThread_execute(t)){
						PRINTF_CONSOLE_ERROR("TSContext, TSThread_execute failed for thread #%d.\n", (i + 1));
						r = FALSE;
						break;
					}
				}
			}
		}
		//stop threads (if failed)
		if(!r){
			TSContext_stopOpq_(opq);
			opq->threads.areRunning = FALSE;
		}
	}
	return r;
}

void TSContext_stopFlagOpq_(STTSContextOpq* opq){
	//context
	opq->stopFlag = TRUE;
	//threads
	{
		SI32 i; for(i = 0; i < opq->threads.arr.use; i++){
			STTSThread* t = NBArray_itmPtrAtIndex(&opq->threads.arr, STTSThread, i);
			TSThread_stopFlag(t);
		}
	}
}

void TSContext_stopFlag(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	TSContext_stopFlagOpq_(opq);
}

void TSContext_stopOpq_(STTSContextOpq* opq){
	//stop-signal
	{
		//context
		opq->stopFlag = TRUE;
		//threads
		{
			SI32 i; for(i = 0; i < opq->threads.arr.use; i++){
				STTSThread* t = NBArray_itmPtrAtIndex(&opq->threads.arr, STTSThread, i);
				TSThread_stopFlag(t);
			}
		}
	}
	//stop
	{
		SI32 i; for(i = 0; i < opq->threads.arr.use; i++){
			STTSThread* t = NBArray_itmPtrAtIndex(&opq->threads.arr, STTSThread, i);
			if(TSThread_isCurrentThread(t)){
				PRINTF_INFO("TSContext, 'stop' called from thread #%d/%d, ignoring stop-wait.\n", (i + 1), opq->threads.arr.use);
			} else {
				TSThread_stop(t);
			}
		}
	}
}
	
void TSContext_stop(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	TSContext_stopOpq_(opq);
}


BOOL TSContext_stopFlagThreadIfIsCurrentOpq_(STTSContextOpq* opq){ //(complement to 'TSContext_startAndUseThisThread') stops the thread execution cycle if this method is called from inside of it.
	BOOL r = FALSE;
	//threads
	{
		SI32 i; for(i = 0; i < opq->threads.arr.use; i++){
			STTSThread* t = NBArray_itmPtrAtIndex(&opq->threads.arr, STTSThread, i);
			if(TSThread_isCurrentThread(t)){
				TSThread_stopFlag(t);
				r = TRUE;
				break;
			}
		}
	}
	return r;
}

BOOL TSContext_stopFlagThreadIfIsCurrent(STTSContext* obj){	//(complement to 'TSContext_startAndUseThisThread') stops the thread execution cycle if this method is called from inside of it.
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return TSContext_stopFlagThreadIfIsCurrentOpq_(opq);
}

BOOL TSContext_loadLocaleStrBytes(STTSContext* obj, const char* data, const UI32 dataSz){
	BOOL r = FALSE;
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	//Load locale file
	{
		if(data != NULL && dataSz > 0){
			if(!NBLocale_loadFromBytes(&opq->locale, data, dataSz)){
				PRINTF_CONSOLE_ERROR("TSContext, TSContext_loadLocaleStrBytes failed for %d bytes.\n", dataSz);
			} else {
				r = TRUE;
			}
		}
	}
	return r;
}

BOOL TSContext_loadLocaleFilepathOpq_(STTSContextOpq* opq, const char* localeFilePath){
	BOOL r = FALSE;
	//Load locale file
	if(localeFilePath != NULL){
		if(localeFilePath[0] != '\0'){
			if(!NBLocale_loadFromFilePath(&opq->locale, localeFilePath)){
				PRINTF_CONSOLE_ERROR("TSContext, NBLocale_loadFromFilePath failed for: '%s'.\n", localeFilePath);
			} else {
				r = TRUE;
			}
		}
	}
	return r;
}
	
BOOL TSContext_loadLocaleFilepath(STTSContext* obj, const char* localeFilePath){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return TSContext_loadLocaleFilepathOpq_(opq, localeFilePath);
}

BOOL TSContext_loadLocaleFilepathAtRootOpq_(STTSContextOpq* opq, STNBFilesystem* fs, const ENNBFilesystemRoot root, const char* localeFilePath){
	BOOL r = FALSE;
	//Load locale file
	if(localeFilePath != NULL){
		if(localeFilePath[0] != '\0'){
            STNBFileRef file = NBFile_alloc(NULL);
			if(!NBFilesystem_openAtRoot(fs, root, localeFilePath, ENNBFileMode_Read, file)){
				PRINTF_CONSOLE_ERROR("TSContext, NBFilesystem_openAtRoot failed for: '%s'.\n", localeFilePath);
			} else {
				NBFile_lock(file);
				if(!NBLocale_loadFromFile(&opq->locale, file)){
					PRINTF_CONSOLE_ERROR("TSContext, NBLocale_loadFromFile failed for: '%s'.\n", localeFilePath);
				} else {
					r = TRUE;
				}
				NBFile_unlock(file);
			}
			NBFile_release(&file);
		}
	}
	return r;
}
	
BOOL TSContext_loadLocaleFilepathAtRoot(STTSContext* obj, STNBFilesystem* fs, const ENNBFilesystemRoot root, const char* localeFilePath){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return TSContext_loadLocaleFilepathAtRootOpq_(opq, fs, root, localeFilePath);
}

BOOL TSContext_pushPreferedLang(STTSContext* obj, const char* lang){
	BOOL r = FALSE;
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	r = NBLocale_pushPreferedLang(&opq->locale, lang);
	return r;
}

const char* TSContext_getPreferedLangAtIdx(STTSContext* obj, const UI32 idx, const char* lngDef){
	const char* r = NULL;
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	r = NBLocale_getPreferedLangAtIdx(&opq->locale, idx, lngDef);
	return r;
}

//Log

void TSContext_logAnalyzeFile(STTSContext* obj, const char* filepath, const BOOL printContent){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	//
	NBString_empty(&opq->log.filepath);
	NBString_empty(&opq->log.pend);
	//
	{
        STNBFileRef file = NBFile_alloc(NULL);
		if(!NBFile_open(file, filepath, ENNBFileMode_Read)){
			//
		} else {
			UI64 bytesTotal = 0;
			IF_PRINTF(UI64 linesTotal = 0;)
			{
				STNBArray linesAccum;
				STNBString lineCur;
				NBArray_init(&linesAccum, sizeof(STNBString), NULL);
				NBString_initWithSz(&lineCur, 4096, 4096, 0.10f);
				{
					if(printContent){
						PRINTF_INFO("TSContext_logAnalyzeFile, output log content: start ---------->.\n\n\n\n\n");
					}
					//Read content
					{
						char buff[4097];
						UI32 linesAfterPend = 0, linesIgnoredAccum = 0;
						NBFile_lock(file);
						while(TRUE){
							const SI32 read = NBFile_read(file, buff, sizeof(buff) - 1);
							if(read <= 0 && bytesTotal == 0){
								break; //empty file
							} else {
								buff[read] = '\0';
								if(printContent){
									printf("%s", buff);
								}
								{
									size_t i;
									for(i = 0; i < read || (read <= 0 && i == 0); i++){
										//Concat char to curLine
										if(buff[i] != '\0'){
											NBString_concatByte(&lineCur, buff[i]);
										}
										//End line
										if(buff[i] == '\n' || read <= 0){
											//Analyze if line is important
											{
												BOOL isFocusLn = FALSE;
												if(!isFocusLn) if(NBString_strIndexOf(lineCur.str, "Assert", 0) >= 0) isFocusLn = TRUE;
												if(!isFocusLn) if(NBString_strIndexOf(lineCur.str, "assert", 0) >= 0) isFocusLn = TRUE;
												if(isFocusLn){
													linesAfterPend = 5;
												}
											}
											//Push line to stack
											{
												STNBString ln;
												NBString_initWithStrBytes(&ln, lineCur.str, lineCur.length);
												NBArray_addValue(&linesAccum, ln);
												NBString_empty(&lineCur);
											}
											//Consume line stack
											if(linesAfterPend > 0){
												if(linesIgnoredAccum > 0){
													NBString_concat(&opq->log.pend, "... +");
													NBString_concatUI32(&opq->log.pend, linesIgnoredAccum);
													NBString_concat(&opq->log.pend, " lines ...\n");
												}
												while(linesAccum.use > 0){
													STNBString* str = NBArray_itmPtrAtIndex(&linesAccum, STNBString, 0);
													NBString_concatBytes(&opq->log.pend, str->str, str->length);
													NBString_release(str);
													NBArray_removeItemAtIndex(&linesAccum, 0);
												}
												linesAfterPend--;
												linesIgnoredAccum = 0;
											} else {
												//Pop lines
												while(linesAccum.use > 5){
													STNBString* str = NBArray_itmPtrAtIndex(&linesAccum, STNBString, 0);
													NBString_release(str);
													NBArray_removeItemAtIndex(&linesAccum, 0);
													linesIgnoredAccum++;
												}
											}
										}
									}
									bytesTotal += read;
								}
								if(read <= 0){
									break;
								}
							}
						}
						//Add last lines
						{
							linesIgnoredAccum += linesAccum.use;
							if(opq->log.pend.length > 0 && linesIgnoredAccum > 0){
								NBString_concat(&opq->log.pend, "... +");
								NBString_concatUI32(&opq->log.pend, linesIgnoredAccum);
								NBString_concat(&opq->log.pend, " lines ...\n");
							}
						}
						NBFile_unlock(file);
						NBFile_close(file);
					}
					if(printContent){
						PRINTF_INFO("TSContext_logAnalyzeFile, output log content: <---------- end.\n\n\n\n\n");
					}
					if(opq->log.pend.length > 0){
						PRINTF_INFO("TSContext_logAnalyzeFile, output log with %llu lines (%llu chars). Report:\n\n%s\n\n", linesTotal, bytesTotal, opq->log.pend.str);
					} else {
						PRINTF_INFO("TSContext_logAnalyzeFile, output log with %llu lines (%llu chars).\n", linesTotal, bytesTotal);
					}
				}
				//Release
				{
					SI32 i; for(i = 0; i < linesAccum.use; i++){
						STNBString* str = NBArray_itmPtrAtIndex(&linesAccum, STNBString, i);
						NBString_release(str);
					}
					NBArray_empty(&linesAccum);
					NBArray_release(&linesAccum);
					NBString_release(&lineCur);
				}
			}
			//
			NBString_set(&opq->log.filepath, filepath);
			opq->log.bytesAnalzd = bytesTotal;
		}
		NBFile_release(&file);
	}
}

UI32 TSContext_logPendLength(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return opq->log.pend.length;
}

void TSContext_logPendGet(STTSContext* obj, STNBString* dst){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	if(dst != NULL){
		NBString_setBytes(dst, opq->log.pend.str, opq->log.pend.length);
	}
}

void TSContext_logPendClear(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	NBString_empty(&opq->log.pend);
	/*if(clearFile){
		if(opq->log.filepath.length > 0 && opq->log.bytesAnalzd > 0){
			STNBFileRef file = NBFile_alloc(NULL);
			if(!NBFile_open(&file, opq->log.filepath.str, ENNBFileMode_Both)){
				//
			} else {
				NBFile_lock(file);
				if(NBFile_seek(file, 0, ENNBFileRelative_End)){
					const SI64 pos = NBFile_curPos(&file);
					if(pos < opq->log.bytesAnalzd){
						PRINTF_CONSOLE_ERROR("TSContext_logPendClear, log is shorted than when was previously read, avoiding the clear action.\n");
					} else if(NBFile_seek(file, 0, ENNBFileRelative_Start)){
						SI64 iPos = 0;
						char buff[4096];
						NBMemory_setZero(buff);
						while(iPos < opq->log.bytesAnalzd){
							const SI64 remain	= (opq->log.bytesAnalzd - iPos);
							const SI64 write	= (remain >= sizeof(buff) ? sizeof(buff) : remain); NBASSERT(write > 0)
							if(NBFile_write(file, buff, (UI32)write) != write){
								PRINTF_CONSOLE_ERROR("TSContext_logPendClear, could not write %lld bytes.\n", write);
								break;
							} else {
								PRINTF_INFO("TSContext_logPendClear, %lld bytes written.\n", write);
								iPos += write;
							}
						}
					}
				}
				NBFile_flush(file);
				NBFile_unlock(file);
				NBFile_close(file);
			}
			NBFile_release(&file);
		}
	}*/
}

//Filesystem

BOOL TSContext_createRootFolders(STTSContext* obj){
	BOOL r = FALSE;
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	STNBFilesystem* fs	= TSFilesystem_getFilesystem(&opq->fs);
	const char* rootRel = TSFilesystem_getRootRelative(&opq->fs);
	if(fs != NULL && rootRel != NULL){
        const ENNBFilesystemRoot rootDir = ENNBFilesystemRoot_WorkDir;
		//[root]
		if(NBFilesystem_folderExistsAtRoot(fs, rootDir, rootRel)){
			//PRINTF_INFO("TSContext, [root] folder found.\n");
			r = TRUE;
		} else if(!NBFilesystem_createFolderAtRoot(fs, rootDir, rootRel)){
			PRINTF_CONSOLE_ERROR("TSContext, could not create [root] folder: '%s'.\n", rootRel);
		} else {
			PRINTF_INFO("TSContext, [root] folder created.\n");
			r = TRUE;
		}
		//Children
		if(r){
			//[root]/clinics
			{
				STNBString path;
				NBString_init(&path);
				NBString_concat(&path, rootRel);
				if(path.length > 0){
					if(path.str[path.length - 1] != '/'){
						NBString_concatByte(&path, '/');
					}
				}
				NBString_concat(&path, "clinics");
				if(NBFilesystem_folderExistsAtRoot(fs, rootDir, path.str)){
					//PRINTF_INFO("TSContext, [root]/clinics folder found.\n");
				} else if(!NBFilesystem_createFolderAtRoot(fs, rootDir, path.str)){
					PRINTF_CONSOLE_ERROR("TSContext, could not create [root]/clinics folder: '%s'.\n", path.str);
					r = FALSE;
				} else {
					PRINTF_INFO("TSContext, [root]/clinics folder created.\n");
				}
				NBString_release(&path);
			}
		}
	}
	return r;
}

void TSContext_concatRootRelativeWithSlash(STTSContext* obj, STNBString* dst){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	TSFilesystem_concatRootRelativeWithSlash(&opq->fs, dst);
}

STNBFilesystem* TSContext_getFilesystem(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return TSFilesystem_getFilesystem(&opq->fs);
}

const char* TSContext_getRootRelative(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return TSFilesystem_getRootRelative(&opq->fs);
}

//Locale

STNBLocale* TSContext_getLocale(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return &opq->locale;
}

const char* TSContext_getStr(STTSContext* obj, const char* uid, const char* strDefault){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return NBLocale_getStr(&opq->locale, uid, strDefault);
}

const char* TSContext_getStrLang(STTSContext* obj, const char* lang, const char* uid, const char* strDefault){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return NBLocale_getStrLang(&opq->locale, lang, uid, strDefault);
}

//Records

BOOL TSContext_setNotifMode(STTSContext* obj, const ENNBStorageNotifMode mode){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return TSFilesystem_setNotifMode(&opq->fs, mode);
}

void TSContext_notifyAll(STTSContext* obj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return TSFilesystem_notifyAll(&opq->fs);
}

void TSContext_addRecordListener(STTSContext* obj, const char* storagePath, const char* recordPath, void* refObj, const STNBStorageCacheItmLstnr* methods){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	TSFilesystem_addRecordListener(&opq->fs, storagePath, recordPath, refObj, methods);
}

void TSContext_addRecordListenerAtClinic(STTSContext* obj, const char* clinicId, const char* storagePath, const char* recordPath, void* refObj, const STNBStorageCacheItmLstnr* methods){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	STNBString path;
	NBString_init(&path);
	NBString_concat(&path, "clinics/");
	NBString_concat(&path, clinicId);
	if(path.length > 0){
		if(path.str[path.length - 1] != '/'){
			NBString_concatByte(&path, '/');
		}
	}
	if(storagePath != NULL){
		UI32 iFirst = 0;
		while(storagePath[iFirst] == '/'){
			iFirst++;
		}
		NBString_concat(&path, &storagePath[iFirst]);
	}
	//
	TSFilesystem_addRecordListener(&opq->fs, path.str, recordPath, refObj, methods);
	//
	NBString_release(&path);
}

void TSContext_removeRecordListener(STTSContext* obj, const char* storagePath, const char* recordPath, void* refObj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	TSFilesystem_removeRecordListener(&opq->fs, storagePath, recordPath, refObj);
}

void TSContext_removeRecordListenerAtClinic(STTSContext* obj, const char* clinicId, const char* storagePath, const char* recordPath, void* refObj){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	STNBString path;
	NBString_init(&path);
	NBString_concat(&path, "clinics/");
	NBString_concat(&path, clinicId);
	if(path.length > 0){
		if(path.str[path.length - 1] != '/'){
			NBString_concatByte(&path, '/');
		}
	}
	if(storagePath != NULL){
		UI32 iFirst = 0;
		while(storagePath[iFirst] == '/'){
			iFirst++;
		}
		NBString_concat(&path, &storagePath[iFirst]);
	}
	//
	TSFilesystem_removeRecordListener(&opq->fs, path.str, recordPath, refObj);
	//
	NBString_release(&path);
}

STNBStorageRecordRead TSContext_getStorageRecordForRead(STTSContext* obj, const char* storagePath, const char* recordPath, const STNBStructMap* pubMap, const STNBStructMap* privMap){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return TSFilesystem_getStorageRecordForRead(&opq->fs, storagePath, recordPath, pubMap, privMap);
}

STNBStorageRecordRead TSContext_getStorageRecordForReadAtClinic(STTSContext* obj, const char* clinicId, const char* storagePath, const char* recordPath, const STNBStructMap* pubMap, const STNBStructMap* privMap){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	STNBStorageRecordRead r;
	STNBString path;
	NBString_init(&path);
	NBString_concat(&path, "clinics/");
	NBString_concat(&path, clinicId);
	if(path.length > 0){
		if(path.str[path.length - 1] != '/'){
			NBString_concatByte(&path, '/');
		}
	}
	if(storagePath != NULL){
		UI32 iFirst = 0;
		while(storagePath[iFirst] == '/'){
			iFirst++;
		}
		NBString_concat(&path, &storagePath[iFirst]);
	}
	//
	r = TSFilesystem_getStorageRecordForRead(&opq->fs, path.str, recordPath, pubMap, privMap);
	//
	NBString_release(&path);
	return r;
}

void TSContext_returnStorageRecordFromRead(STTSContext* obj, STNBStorageRecordRead* ref){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return TSFilesystem_returnStorageRecordFromRead(&opq->fs, ref);
}

STNBStorageRecordWrite TSContext_getStorageRecordForWrite(STTSContext* obj, const char* storagePath, const char* recordPath, const BOOL createIfNew, const STNBStructMap* pubMap, const STNBStructMap* privMap){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return TSFilesystem_getStorageRecordForWrite(&opq->fs, storagePath, recordPath, createIfNew, pubMap, privMap);
}

STNBStorageRecordWrite TSContext_getStorageRecordForWriteAtClinic(STTSContext* obj, const char* clinicId, const char* storagePath, const char* recordPath, const BOOL createIfNew, const STNBStructMap* pubMap, const STNBStructMap* privMap){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	STNBStorageRecordWrite r;
	STNBString path;
	NBString_init(&path);
	NBString_concat(&path, "clinics/");
	NBString_concat(&path, clinicId);
	if(path.length > 0){
		if(path.str[path.length - 1] != '/'){
			NBString_concatByte(&path, '/');
		}
	}
	if(storagePath != NULL){
		UI32 iFirst = 0;
		while(storagePath[iFirst] == '/'){
			iFirst++;
		}
		NBString_concat(&path, &storagePath[iFirst]);
	}
	//
	r = TSFilesystem_getStorageRecordForWrite(&opq->fs, path.str, recordPath, createIfNew, pubMap, privMap);
	//
	NBString_release(&path);
	return r;
}

void TSContext_returnStorageRecordFromWrite(STTSContext* obj, STNBStorageRecordWrite* ref){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	return TSFilesystem_returnStorageRecordFromWrite(&opq->fs, ref);
}

void TSContext_touchRecord(STTSContext* obj, const char* storagePath, const char* recordPath){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	TSFilesystem_touchRecord(&opq->fs, storagePath, recordPath);
}

void TSContext_touchRecordAtClinic(STTSContext* obj, const char* clinicId, const char* storagePath, const char* recordPath){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	STNBString path;
	NBString_init(&path);
	NBString_concat(&path, "clinics/");
	NBString_concat(&path, clinicId);
	if(path.length > 0){
		if(path.str[path.length - 1] != '/'){
			NBString_concatByte(&path, '/');
		}
	}
	if(storagePath != NULL){
		UI32 iFirst = 0;
		while(storagePath[iFirst] == '/'){
			iFirst++;
		}
		NBString_concat(&path, &storagePath[iFirst]);
	}
	//
	TSFilesystem_touchRecord(&opq->fs, path.str, recordPath);
	//
	NBString_release(&path);
}

void TSContext_getStorageFiles(STTSContext* obj, const char* storagePath, STNBString* dstStrs, STNBArray* dstFiles /*STNBFilesystemFile*/){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	TSFilesystem_getStorageFiles(&opq->fs, storagePath, dstStrs, dstFiles);
}

void TSContext_resetAllStorages(STTSContext* obj, const BOOL deleteFiles){
	STTSContextOpq* opq = (STTSContextOpq*)obj->opaque;
	TSFilesystem_resetAllStorages(&opq->fs, deleteFiles);
}

//Hashes

void TSContext_concatRandomHashBin(STNBString* dst, const SI32 rndBytes){
	const UI64 curTime = NBDatetime_getCurUTCTimestamp();
	STNBSha1 hash;
	NBSha1_init(&hash);
	NBSha1_feed(&hash, &curTime, sizeof(curTime));
	UI32 i; for(i = 0; i < rndBytes; i++){
		const UI32 rnd = (rand() % 0xFFFFFFFF);
		NBSha1_feed(&hash, &rnd, sizeof(rnd));
	}
	NBSha1_feedEnd(&hash);
	NBASSERT(sizeof(hash.hash.v) == 20)
	if(dst != NULL){
		NBString_concatBytes(dst, (const char*)&hash.hash.v[0], sizeof(hash.hash.v));
	}
	NBSha1_release(&hash);
}

void TSContext_concatRandomHashHex(STNBString* dst, const SI32 rndBytes){
	const UI64 curTime = NBDatetime_getCurUTCTimestamp();
	STNBSha1 hash;
	NBSha1_init(&hash);
	NBSha1_feed(&hash, &curTime, sizeof(curTime));
	UI32 i; for(i = 0; i < rndBytes; i++){
		const UI32 rnd = (rand() % 0xFFFFFFFF);
		NBSha1_feed(&hash, &rnd, sizeof(rnd));
	}
	NBSha1_feedEnd(&hash);
	NBASSERT(sizeof(hash.hash.v) == 20)
	if(dst != NULL){
		const STNBSha1HashHex hex = NBSha1_getHashHex(&hash);
		NBString_concat(dst, hex.v);
	}
	NBSha1_release(&hash);
}

//Format

void TSContext_concatDurationFromSecs(STTSContext* obj, STNBString* dst, const UI64 pSecs, const ENTSDurationPart minPart, const char* lang){
	if(dst != NULL){
		UI64 secs = pSecs;
		const UI32 lenStart = dst->length;
		if((secs >= (60.0f * 60.0f * 24.0f * 30.4 * 12.0f) && minPart <= ENTSDurationPart_Year) || minPart == ENTSDurationPart_Year){
			const UI32 val = secs / (60.0f * 60.0f * 24.0f * 30.4 * 12.0f);
			if(lenStart != dst->length) NBString_concat(dst, ", ");
			if(val > 1){
				NBString_concatUI32(dst, val);
				NBString_concatByte(dst, ' ');
				NBString_concat(dst, TSContext_getStrLang(obj, lang, "_timeYear", "year"));
			} else if(val == 1 || minPart == ENTSDurationPart_Year){
				NBString_concatUI32(dst, val);
				NBString_concatByte(dst, ' ');
				NBString_concat(dst, TSContext_getStrLang(obj, lang, "_timeYears", "years"));
			}
			secs -= (val * (60.0f * 60.0f * 24.0f * 30.4 * 12.0f));
		}
		if((secs >= (60.0f * 60.0f * 24.0f * 30.4) && minPart <= ENTSDurationPart_Month) || minPart == ENTSDurationPart_Month){
			const UI32 val = secs / (60.0f * 60.0f * 24.0f * 30.4f);
			if(lenStart != dst->length) NBString_concat(dst, ", ");
			if(val > 1){
				NBString_concatUI32(dst, val);
				NBString_concatByte(dst, ' ');
				NBString_concat(dst, TSContext_getStrLang(obj, lang, "_timeMonths", "months"));
			} else if(val == 1 || minPart == ENTSDurationPart_Month){
				NBString_concatUI32(dst, val);
				NBString_concatByte(dst, ' ');
				NBString_concat(dst, TSContext_getStrLang(obj, lang, "_timeMonth", "month"));
			}
			secs -= (val * (60.0f * 60.0f * 24.0f * 30.4f));
		}
		if((secs >= (60.0f * 60.0f * 24.0f) && minPart <= ENTSDurationPart_Day) || minPart == ENTSDurationPart_Day){
			const UI32 val = secs / (60.0f * 60.0f * 24.0f);
			if(lenStart != dst->length) NBString_concat(dst, ", ");
			if(val > 1){
				NBString_concatUI32(dst, val);
				NBString_concatByte(dst, ' ');
				NBString_concat(dst, TSContext_getStrLang(obj, lang, "_timeDays", "days"));
			} else if(val == 1 || minPart == ENTSDurationPart_Day){
				NBString_concatUI32(dst, val);
				NBString_concatByte(dst, ' ');
				NBString_concat(dst, TSContext_getStrLang(obj, lang, "_timeDay", "day"));
			}
			secs -= (val * (60.0f * 60.0f * 24.0f));
		}
		if((secs >= (60.0f * 60.0f) && minPart <= ENTSDurationPart_Hour) || minPart == ENTSDurationPart_Hour){
			const UI32 val = secs / (60.0f * 60.0f);
			if(lenStart != dst->length) NBString_concat(dst, ", ");
			if(val > 1){
				NBString_concatUI32(dst, val);
				NBString_concatByte(dst, ' ');
				NBString_concat(dst, TSContext_getStrLang(obj, lang, "_timeHours", "hours"));
			} else if(val == 1 || minPart == ENTSDurationPart_Hour){
				NBString_concatUI32(dst, val);
				NBString_concatByte(dst, ' ');
				NBString_concat(dst, TSContext_getStrLang(obj, lang, "_timeHour", "hour"));
			}
			secs -= (val * (60.0f * 60.0f));
		}
		if((secs >= (60.0f) && minPart <= ENTSDurationPart_Minute) || minPart == ENTSDurationPart_Minute){
			const UI32 val = secs / (60.0f);
			if(lenStart != dst->length) NBString_concat(dst, ", ");
			if(val > 1){
				NBString_concatUI32(dst, val);
				NBString_concatByte(dst, ' ');
				NBString_concat(dst, TSContext_getStrLang(obj, lang, "_timeMinutes", "minutes"));
			} else if(val == 1 || minPart == ENTSDurationPart_Minute){
				NBString_concatUI32(dst, val);
				NBString_concatByte(dst, ' ');
				NBString_concat(dst, TSContext_getStrLang(obj, lang, "_timeMinute", "minute"));
			}
			secs -= (val * (60.0f));
		}
		if((secs >= 0 && minPart <= ENTSDurationPart_Second) || minPart == ENTSDurationPart_Second){
			const UI64 val = secs;
			if(lenStart != dst->length) NBString_concat(dst, ", ");
			if(val > 1){
				NBString_concatUI64(dst, val);
				NBString_concatByte(dst, ' ');
				NBString_concat(dst, TSContext_getStrLang(obj, lang, "_timeSeconds", "seconds"));
			} else if(val == 1 || minPart == ENTSDurationPart_Second){
				NBString_concatUI64(dst, val);
				NBString_concatByte(dst, ' ');
				NBString_concat(dst, TSContext_getStrLang(obj, lang, "_timeSecond", "second"));
			}
			secs -= val;
		}
	}
}

void TSContext_concatHumanTimeFromSeconds(STTSContext* obj, const UI32 seconds, const BOOL useShortTags, const UI32 maxPartsCount, STNBString* dst){
	if(dst != NULL){
		UI32 partsAdded = 0;
		UI32 secsRemain = seconds;
		//days
		if(secsRemain >= (60 * 60 * 24) && (maxPartsCount <= 0 || partsAdded < maxPartsCount)){
			const UI32 val	= (secsRemain / (60 * 60 * 24));
			secsRemain		-= val * (60 * 60 * 24);
			if(partsAdded != 0) NBString_concat(dst, (useShortTags ? ":" : ", "));
			NBString_concatUI32(dst, val);
			NBString_concat(dst, useShortTags ? "d" : val == 1 ? " day" : " days");
			partsAdded++;
		}
		//hours
		if(secsRemain >= (60 * 60) && (maxPartsCount <= 0 || partsAdded < maxPartsCount)){
			const UI32 val	= (secsRemain / (60 * 60));
			secsRemain		-= val * (60 * 60);
			if(partsAdded != 0) NBString_concat(dst, (useShortTags ? ":" : ", "));
			NBString_concatUI32(dst, val);
			NBString_concat(dst, useShortTags ? "h" : val == 1 ? " hour" : " hours");
			partsAdded++;
		}
		//minutes
		if(secsRemain >= 60 && (maxPartsCount <= 0 || partsAdded < maxPartsCount)){
			const UI32 val	= (secsRemain / 60);
			secsRemain		-= val * 60;
			if(partsAdded != 0) NBString_concat(dst, (useShortTags ? ":" : ", "));
			NBString_concatUI32(dst, val);
			NBString_concat(dst, useShortTags ? "m" : val == 1 ? " minute" : " minutes");
			partsAdded++;
		}
		//seconds
		if((secsRemain > 0|| partsAdded == 0) && (maxPartsCount <= 0 || partsAdded < maxPartsCount)){
			const UI32 val	= secsRemain;
			secsRemain		-= val;
			if(partsAdded != 0) NBString_concat(dst, (useShortTags ? ":" : ", "));
			NBString_concatUI32(dst, val);
			NBString_concat(dst, useShortTags ? "s" : val == 1 ? " second" : " seconds");
			partsAdded++;
		}
	}
}

void TSContext_concatHumanDate(STTSContext* obj, const STNBDatetime localDatetime, const BOOL useShortTags, const char* separatorDateFromTime, STNBString* dst){
	if(dst != NULL){
		UI32 partsAdded = 0;
		//Month
		{
			switch (localDatetime.month) {
			case 1:
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concat(dst, (useShortTags ? "Jan" : "January"));
				partsAdded++;
				break;
			case 2:
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concat(dst, (useShortTags ? "Feb" : "February"));
				partsAdded++;
				break;	
			case 3:
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concat(dst, (useShortTags ? "Mar":  "March"));
				partsAdded++;
				break;
			case 4:
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concat(dst, (useShortTags ? "Apr":  "April"));
				partsAdded++;
				break;
			case 5:
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concat(dst, (useShortTags ? "May":  "May"));
				partsAdded++;
				break;
			case 6:
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concat(dst, (useShortTags ? "Jun":  "June"));
				partsAdded++;
				break;
			case 7:
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concat(dst, (useShortTags ? "Jul":  "July"));
				partsAdded++;
				break;
			case 8:
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concat(dst, (useShortTags ? "Aug":  "August"));
				partsAdded++;
				break;
			case 9:
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concat(dst, (useShortTags ? "Sep":  "September"));
				partsAdded++;
				break;
			case 10:
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concat(dst, (useShortTags ? "Oct":  "October"));
				partsAdded++;
				break;
			case 11:
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concat(dst, (useShortTags ? "Nov":  "November"));
				partsAdded++;
				break;
			case 12:
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concat(dst, (useShortTags ? "Dec":  "December"));
				partsAdded++;
				break;
			default:
				break;
			}
		}
		//Day
		{
			if(partsAdded > 0) NBString_concat(dst, " ");
			NBString_concatUI32(dst, localDatetime.day);
			partsAdded++;
		}
		//Year
		{
			BOOL ignore = FALSE;
			if(useShortTags){
				STNBDatetime curTime = NBDatetime_getCurLocal();
				if(curTime.year == localDatetime.year){
					ignore = TRUE;
				}
			}
			if(!ignore){
				if(partsAdded > 0) NBString_concat(dst, ", ");
				NBString_concatUI32(dst, localDatetime.year);
				partsAdded++;
			}
		}
		//Hour and minute
		if(localDatetime.hour > 0 || localDatetime.min > 0){
			const char* midday = (localDatetime.hour >= 12 ? "pm" : "am");
			if(partsAdded > 0) NBString_concat(dst, (separatorDateFromTime == NULL ? ", " : separatorDateFromTime));
			if(localDatetime.hour > 12){
				NBString_concatUI32(dst, localDatetime.hour - 12);
			} else {
				NBString_concatUI32(dst, localDatetime.hour);
			}
			partsAdded++;
			//minute
			if(localDatetime.min > 0){
				NBString_concat(dst, ":");
				if(localDatetime.min < 10) NBString_concat(dst, "0");
				NBString_concatUI32(dst, localDatetime.min);
				partsAdded++;
			}
			//midday (am/pm)
			{
				NBString_concat(dst, " ");
				NBString_concat(dst, midday);
				partsAdded++;
			}
		}
	}
}
