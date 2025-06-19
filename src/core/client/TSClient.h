//
//  TSClient.h
//  thinstream
//
//  Created by Marcos Ortega on 8/5/18.
//

#ifndef TSClient_h
#define TSClient_h

#include "nb/crypto/NBX509.h"
#include "nb/crypto/NBPKey.h"
#include "core/TSContext.h"
#include "core/TSRequestsMap.h"
#include "core/config/TSCfg.h"
#include "core/client/TSClientChannel.h"
#include "core/client/TSClientChannelRequest.h"
#include "core/TSCypher.h"
#include "core/logic/TSDevice.h"
#include "core/logic/TSClientRequesterConnStats.h"
#include "core/logic/TSStreamsStorageStats.h"
#include "core/logic/TSClientRequester.h"

#ifdef __cplusplus
extern "C" {
#endif


	//stats

#	ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
	typedef struct STTSClientStatsThreadState_ {
		const char*					name;						
		const char*					firstKnownMethod;
		ENNBLogLevel				statsLevel;
		STNBProcessStatsState	stats;
	} STTSClientStatsThreadState;
#	endif

#	ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
	typedef struct STTSClientStatsProcessState_ {
		STNBProcessStatsState		stats;
		STTSClientStatsThreadState*		threads;
		UI32							threadsSz;
	} STTSClientStatsProcessState;
#	endif

	typedef struct STTSClientStatsState_ {
		STNBDataPtrsStatsState			buffs;
#		ifdef CONFIG_NB_INCLUDE_THREADS_METRICS
		STTSClientStatsProcessState		process;
#		endif
	} STTSClientStatsState;

	typedef enum ENTSClientStartsGrp_ {
		ENTSClientStartsGrp_Loaded = 0,	//currently loaded content
		ENTSClientStartsGrp_Accum,		//activitity since last period (ex: each second)
		ENTSClientStartsGrp_Total,		//activity since start
		//
		ENTSClientStartsGrp_Count
	} ENTSClientStartsGrp;

	typedef struct STTSClientStatsData_ {
		STTSClientStatsState	loaded;		//loaded
		STTSClientStatsState	accum;		//aacum
		STTSClientStatsState	total;		//total
	} STTSClientStatsData;

	//
	typedef struct STTSClient_ {
		void* opaque;
	} STTSClient;
	
	void TSClient_init(STTSClient* obj);
	void TSClient_release(STTSClient* obj);
	
	BOOL				TSClient_prepare(STTSClient* obj, const char* rootPath, STTSContext* context, const STTSCfg* cfg, const void* caCertData, const UI32 caCertDataSz);
	//App's orientation (amount of elements that required portrait mode)
	SI32				TSClient_getAppPortraitEnforcersCount(STTSClient* obj);
	void				TSClient_appPortraitEnforcerPush(STTSClient* obj);
	void				TSClient_appPortraitEnforcerPop(STTSClient* obj);
	//Sync flag
	BOOL				TSClient_syncIsReceiving(STTSClient* obj);
	BOOL				TSClient_syncIsFirstDone(STTSClient* obj);
	void				TSClient_syncConcatStreamId(STTSClient* obj, STNBString* dst);
	void				TSClient_syncSetReceiving(STTSClient* obj, const BOOL isReceiving);
	void				TSClient_syncSetFirstDone(STTSClient* obj, const BOOL isSyncFirstDone);
	void				TSClient_syncSetStreamId(STTSClient* obj, const char* streamId);
	//Log
	void				TSClient_logAnalyzeFile(STTSClient* obj, const char* filepath, const BOOL printContent);
	UI32				TSClient_logPendLength(STTSClient* obj);
	void				TSClient_logPendGet(STTSClient* obj, STNBString* dst);
	void				TSClient_logPendClear(STTSClient* obj);
	//Local
	const char*			TSClient_getLocalePreferedLangAtIdx(STTSClient* obj, const UI32 idx, const char* lngDef);
	STNBLocale*			TSClient_getLocale(STTSClient* obj);
	const char*			TSClient_getStr(STTSClient* obj, const char* uid, const char* strDefault);
	//
	void				TSClient_concatRootFolderPath(const STTSClient* obj, STNBString* dst);
	const char*			TSClient_getRootFolderName(const STTSClient* obj);
	STTSContext*		TSClient_getContext(STTSClient* obj);
	STTSCypher*			TSClient_getCypher(STTSClient* obj);
	STNBFilesystem*		TSClient_getFilesystem(STTSClient* obj);
	STTSClientRequesterRef TSClient_getStreamsRequester(STTSClient* obj); 
	//
	STNBX509*			TSClient_getCACert(STTSClient* obj);
	UI32				TSClient_getDeviceKeySeq(STTSClient* obj);
	BOOL				TSClient_isDeviceKeySet(STTSClient* obj);
	//BOOL				TSClient_setDeviceCertAndKey(STTSClient* obj, STNBPKey* key, STNBX509* cert); //Force new connections
	//BOOL				TSClient_touchCurrentKeyAndCert(STTSClient* obj); //Force new connections
	BOOL				TSClient_encForCurrentKey(STTSClient* obj, const void* plain, const UI32 plainSz, STNBString* dst);
	BOOL				TSClient_decWithCurrentKey(STTSClient* obj, const void* enc, const UI32 encSz, STNBString* dst);
	BOOL				TSClient_decKeyLocal(STTSClient* obj, const STTSCypherDataKey* keyEncForRemote, STTSCypherDataKey* dstKeyEncForLocal);
	BOOL				TSClient_decDataKeyOpq(void* opq, STTSCypherDataKey* dstKeyPlain, const STTSCypherDataKey* keyEnc);
	//Stats
	UI32				TSClient_getStatsTryConnAccum(STTSClient* obj, const BOOL resetAccum);
	//Requests
	UI64				TSClient_addRequest(STTSClient* obj, const ENTSRequestId reqId, const char** paramsPairsAndNull, const char* body, const UI32 bodySz, const UI64 secsWait, TSClientChannelReqCallbackFunc lstnr, void* lstnrParam);
	void				TSClient_removeRequestsByListenerParam(STTSClient* obj, void* lstnrParam);
	void				TSClient_notifyAll(STTSClient* obj);
	//
	BOOL				TSClient_isCypherKeyAnySet(STTSClient* obj);		//When decryption is made local or remote
	BOOL				TSClient_isCypherKeyLocalSet(STTSClient* obj);		//When decryption is made local
	//
	UI32				TSClient_getCypherKeyLocalSeq(STTSClient* obj);		//Key local
	UI32				TSClient_getCypherItfRemoteSeq(STTSClient* obj);	//Itf remote
	//
	//BOOL				TSClient_setCypherCertAndKey(STTSClient* obj, STNBX509* cert, STNBPKey* key, const STTSCypherDataKey* sharedKeyEnc);
	void				TSClient_disconnect(STTSClient* obj, const UI64* filterReqUid);
	void				TSClient_startShutdown(STTSClient* obj);
	void				TSClient_waitForAll(STTSClient* obj);

	//
	BOOL				TSClient_concatDeviceId(STTSClient* obj, STNBString* dst);

	//Certiticates validation
	BOOL				TSClient_certIsSignedByCA(STTSClient* obj, STNBX509* cert);
	
	//Records listeners
	void					TSClient_addRecordListener(STTSClient* obj, const char* storagePath, const char* recordPath, void* refObj, const STNBStorageCacheItmLstnr* methods);
	void					TSClient_addRecordListenerAtClinic(STTSClient* obj, const char* clinicId, const char* storagePath, const char* recordPath, void* refObj, const STNBStorageCacheItmLstnr* methods);
	//
	void					TSClient_removeRecordListener(STTSClient* obj, const char* storagePath, const char* recordPath, void* refObj);
	void					TSClient_removeRecordListenerAtClinic(STTSClient* obj,  const char* clinicId, const char* storagePath, const char* recordPath, void* refObj);
	
	//Read record
	STNBStorageRecordRead	TSClient_getStorageRecordForRead(STTSClient* obj, const char* storagePath, const char* recordPath, const STNBStructMap* pubMap, const STNBStructMap* privMap);
	STNBStorageRecordRead	TSClient_getStorageRecordForReadAtClinic(STTSClient* obj, const char* clinicId, const char* storagePath, const char* recordPath, const STNBStructMap* pubMap, const STNBStructMap* privMap);
	void					TSClient_returnStorageRecordFromRead(STTSClient* obj, STNBStorageRecordRead* ref);
	
	//Write record
	STNBStorageRecordWrite	TSClient_getStorageRecordForWrite(STTSClient* obj, const char* storagePath, const char* recordPath, const BOOL createIfNew, const STNBStructMap* pubMap, const STNBStructMap* privMap);
	STNBStorageRecordWrite	TSClient_getStorageRecordForWriteAtClinic(STTSClient* obj, const char* clinicId, const char* storagePath, const char* recordPath, const BOOL createIfNew, const STNBStructMap* pubMap, const STNBStructMap* privMap);
	void					TSClient_returnStorageRecordFromWrite(STTSClient* obj, STNBStorageRecordWrite* ref);
	
	//Touch record (trigger a notification to listeners without saving to file)
	void					TSClient_touchRecord(STTSClient* obj, const char* storagePath, const char* recordPath);
	void					TSClient_touchRecordAtClinic(STTSClient* obj, const char* clinicId, const char* storagePath, const char* recordPath);
	
	
	//Time format
	void					TSClient_concatHumanTimeFromSeconds(STTSClient* obj, const UI32 seconds, const BOOL useShortTags, const UI32 maxPartsCount, STNBString* dst);
	void					TSClient_concatHumanDate(STTSClient* obj, const STNBDatetime localDatetime, const BOOL useShortTags, const char* separatorDateFromTime, STNBString* dst);
	//
	// Build request "DeviceNew"
	//
	
	SI32 TSClient_asyncBuildPkeyAndCsr(STTSClient* clt, STNBPKey** dstKey, STNBX509Req** dstCsr, BOOL* dstIsBuilding);
	
	//---------------------------------------
	
	//
	// Load current device certificate
	//
	
	SI32 TSClient_loadCurrentDeviceKey(STTSClient* clt);
	
	//---------------------------------------
	
	//List data records
	void TSClient_getStorageFiles(STTSClient* clt, const char* storagePath, STNBString* dstStrs, STNBArray* dstFiles /*STNBFilesystemFile*/);
							
	//Reset records and local data
	void TSClient_resetLocalData(STTSClient* clt);
	
	//
	// Delete currrent identity (local profile, private keys, certiticates and indentity-block)
	//
	
	SI32 TSClient_deleteCurrentIdentityAll(STTSClient* clt);
	
	//Extract files from package to app's library folder

	typedef struct STPkgLoadDef_ {
		const char* path;
		const void*	data;	//optional, persistent data
		const UI32	dataSz;	//optional, persistent data size
	} STPkgLoadDef;

	void TSClient_extractFilesFromPackageToPath(STNBFilesystem* fs, const char* dstPath, const STPkgLoadDef* loadDefs, const SI32 loadDefsSz);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSClient_h */
