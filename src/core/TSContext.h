//
//  TSContext.h
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#ifndef TSContext_h
#define TSContext_h

#include "nb/core/NBString.h"
#include "nb/core/NBLocale.h"
#include "nb/core/NBIOPollstersProvider.h"
#include "core/config/TSCfg.h"
#include "core/TSFilesystem.h"
#include "nb/crypto/NBX509.h"
#if defined(__ANDROID__)
#	include "nb/files/NBFilesystemAndroid.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef enum ENTSDeviceType_ {
		ENTSDeviceType_Unknown = 0,
		ENTSDeviceType_Computer,	//Desktop or laptop
		ENTSDeviceType_Handled,		//Smarthphone or similar size
		ENTSDeviceType_Tablet,		//Tablet
		ENTSDeviceType_Count
	} ENTSDeviceType;
	
	const STNBEnumMap* TSDeviceType_getSharedEnumMap(void);
	
	typedef enum ENTSDeviceFamily_ {
		ENTSDeviceFamily_Unknown = 0,
		ENTSDeviceFamily_Apple,
		ENTSDeviceFamily_Windows,
		ENTSDeviceFamily_Android,
		ENTSDeviceFamily_Count
	} ENTSDeviceFamily;
	
	const STNBEnumMap* TSDeviceFamily_getSharedEnumMap(void);
	
	//

	typedef enum ENTSDurationPart_ {
		ENTSDurationPart_Second = 0,
		ENTSDurationPart_Minute,
		ENTSDurationPart_Hour,
		ENTSDurationPart_Day,
		ENTSDurationPart_Month,
		ENTSDurationPart_Year
	} ENTSDurationPart;

	//-------------------
	//- TSContextLstnrItf
	//-------------------

	typedef struct STTSContextLstnrItf_ {
		void	(*contextTick)(const STNBTimestampMicro time, void* usrData);	//every time the first TSThread poll ends.
	} STTSContextLstnrItf;

	//----------------
	//- TSContextLstnr
	//----------------

	typedef struct STTSContextLstnr_ {
		STTSContextLstnrItf	itf;
		void*				usrData;
	} STTSContextLstnr;

	//
	
	typedef struct STTSContext_ {
		void*		opaque;
	} STTSContext;
	
#	if defined(__ANDROID__)
	void					TSContext_init(STTSContext* obj, const STNBAndroidJniItf* jniItf, void* jniObj);
#	else
	void					TSContext_init(STTSContext* obj);
#	endif
	void					TSContext_release(STTSContext* obj);
	//Desc
	ENTSDeviceType			TSContext_getDeviceType(void);
	ENTSDeviceFamily		TSContext_getDeviceFamily(void);
	//runId
	const char*				TSContext_getRunId(STTSContext* obj);
	//buffers
	STNBDataPtrsPoolRef		TSContext_getBuffersPool(STTSContext* obj);
	STNBDataPtrsStatsRef	TSContext_getBuffersStats(STTSContext* obj);
	//pollsters
	STNBIOPollstersProviderRef TSContext_getPollstersProvider(STTSContext* obj);
	//Lstnrs
	BOOL					TSContext_addLstnr(STTSContext* obj, STTSContextLstnrItf* itf, void* usrData);
	BOOL					TSContext_removeLstnr(STTSContext* obj, void* usrData);
	//Config
	const STTSCfg*			TSContext_getCfg(const STTSContext* obj);
	BOOL					TSContext_prepare(STTSContext* obj, const STTSCfg* cfg);
	//execution
	BOOL					TSContext_start(STTSContext* obj);
	BOOL					TSContext_startAndUseThisThread(STTSContext* obj);
	void					TSContext_stopFlag(STTSContext* obj);
	void					TSContext_stop(STTSContext* obj);
	BOOL					TSContext_stopFlagThreadIfIsCurrent(STTSContext* obj);	//(complement to 'TSContext_startAndUseThisThread') stops the thread execution cycle if this method is called from inside of it.
	//
	BOOL					TSContext_loadLocaleStrBytes(STTSContext* obj, const char* data, const UI32 dataSz);
	BOOL					TSContext_loadLocaleFilepath(STTSContext* obj, const char* localeFilePath);
	BOOL					TSContext_loadLocaleFilepathAtRoot(STTSContext* obj, STNBFilesystem* fs, const ENNBFilesystemRoot root, const char* localeFilePath);
	BOOL					TSContext_pushPreferedLang(STTSContext* obj, const char* lang);
	const char*				TSContext_getPreferedLangAtIdx(STTSContext* obj, const UI32 idx, const char* lngDef);
	//Log
	void					TSContext_logAnalyzeFile(STTSContext* obj, const char* filepath, const BOOL printContent);
	UI32					TSContext_logPendLength(STTSContext* obj);
	void					TSContext_logPendGet(STTSContext* obj, STNBString* dst);
	void					TSContext_logPendClear(STTSContext* obj);
	//Filesystem
	BOOL					TSContext_createRootFolders(STTSContext* obj);
	void					TSContext_concatRootRelativeWithSlash(STTSContext* obj, STNBString* dst);
	STNBFilesystem*			TSContext_getFilesystem(STTSContext* obj);
	const char*				TSContext_getRootRelative(STTSContext* obj);
	//Locale
	STNBLocale*				TSContext_getLocale(STTSContext* obj);
	const char*				TSContext_getStr(STTSContext* obj, const char* uid, const char* strDefault);
	const char*				TSContext_getStrLang(STTSContext* obj, const char* lang, const char* uid, const char* strDefault);
	//Records
	BOOL					TSContext_setNotifMode(STTSContext* obj, const ENNBStorageNotifMode mode);
	void					TSContext_notifyAll(STTSContext* obj);
	void					TSContext_addRecordListener(STTSContext* obj, const char* storagePath, const char* recordPath, void* refObj, const STNBStorageCacheItmLstnr* methods);
	void					TSContext_addRecordListenerAtClinic(STTSContext* obj, const char* clinicId, const char* storagePath, const char* recordPath, void* refObj, const STNBStorageCacheItmLstnr* methods);
	void					TSContext_removeRecordListener(STTSContext* obj, const char* storagePath, const char* recordPath, void* refObj);
	void					TSContext_removeRecordListenerAtClinic(STTSContext* obj, const char* clinicId, const char* storagePath, const char* recordPath, void* refObj);
	STNBStorageRecordRead	TSContext_getStorageRecordForRead(STTSContext* obj, const char* storagePath, const char* recordPath, const STNBStructMap* pubMap, const STNBStructMap* privMap);
	STNBStorageRecordRead	TSContext_getStorageRecordForReadAtClinic(STTSContext* obj, const char* clinicId, const char* storagePath, const char* recordPath, const STNBStructMap* pubMap, const STNBStructMap* privMap);
	void					TSContext_returnStorageRecordFromRead(STTSContext* obj, STNBStorageRecordRead* ref);
	STNBStorageRecordWrite	TSContext_getStorageRecordForWrite(STTSContext* obj, const char* storagePath, const char* recordPath, const BOOL createIfNew, const STNBStructMap* pubMap, const STNBStructMap* privMap);
	STNBStorageRecordWrite	TSContext_getStorageRecordForWriteAtClinic(STTSContext* obj, const char* clinicId, const char* storagePath, const char* recordPath, const BOOL createIfNew, const STNBStructMap* pubMap, const STNBStructMap* privMap);
	void					TSContext_returnStorageRecordFromWrite(STTSContext* obj, STNBStorageRecordWrite* ref);
	void					TSContext_touchRecord(STTSContext* obj, const char* storagePath, const char* recordPath);
	void					TSContext_touchRecordAtClinic(STTSContext* obj, const char* clinicId, const char* storagePath, const char* recordPath);
	void					TSContext_getStorageFiles(STTSContext* obj, const char* storagePath, STNBString* dstStrs, STNBArray* dstFiles /*STNBFilesystemFile*/);
	void					TSContext_resetAllStorages(STTSContext* obj, const BOOL deleteFiles);
	//Hashes
	void					TSContext_concatRandomHashBin(STNBString* dst, const SI32 rndBytes);
	void					TSContext_concatRandomHashHex(STNBString* dst, const SI32 rndBytes);
	//Format
	void					TSContext_concatDurationFromSecs(STTSContext* obj, STNBString* dst, const UI64 secs, const ENTSDurationPart minPart, const char* lang);
	void					TSContext_concatHumanTimeFromSeconds(STTSContext* obj, const UI32 seconds, const BOOL useShortTags, const UI32 maxPartsCount, STNBString* dst);
	void 					TSContext_concatHumanDate(STTSContext* obj, const STNBDatetime localDatetime, const BOOL useShortTags, const char* separatorDateFromTime, STNBString* dst); 
	
#ifdef __cplusplus
} //extern "C"
#endif
	
#endif /* TSContext_h */
