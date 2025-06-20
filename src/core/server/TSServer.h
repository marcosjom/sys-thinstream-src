//
//  TSServer.h
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#ifndef TSServer_h
#define TSServer_h

#include "nb/core/NBMngrProcess.h"
#include "nb/core/NBString.h"
#include "nb/core/NBLog.h"
#include "nb/core/NBDataPtrsStats.h"
#include "core/config/TSCfg.h"
#include "core/TSContext.h"
#include "core/TSCypher.h"
#include "core/server/TSServerLstnr.h"
#include "core/logic/TSStreamsStorageStats.h"
#include "core/logic/TSClientRequesterConnStats.h"

#ifdef __cplusplus
extern "C" {
#endif
		
	typedef struct STTSServer_ {
		void*		opaque;
	} STTSServer;
	
	void TSServer_init(STTSServer* obj);
	void TSServer_release(STTSServer* obj);
	
	BOOL TSServer_prepare(STTSServer* obj, STTSContext* context, const BOOL* externalStopFlag, const SI32 msRunAndQuit, STTSServerLstnr* lstnr);
	BOOL TSServer_execute(STTSServer* obj, const BOOL* externalStopFlag);
	void TSServer_stopFlag(STTSServer* obj);
	void TSServer_stop(STTSServer* obj);
	BOOL TSServer_isBusy(STTSServer* obj);

	//Context
	STTSContext*	TSServer_getContext(STTSServer* obj);
	
	//Locale
	const char*		TSServer_getLocaleStr(STTSServer* obj, const char* lang, const char* strId, const char* defaultValue);
	
	//Sync itf
	void TSServer_setSyncItf(STTSServer* obj, const STTSServerSyncItf* itf);
	BOOL TSServer_isDeviceOnline(STTSServer* obj, const char* deviceId, BOOL* dstIsUnlocked, BOOL* dstIsSyncMeetings, BOOL* dstCertsSendFirst, BOOL* dstMeetsSendFirst);

	//Re-encode
	BOOL TSServer_reEncDataKey(STTSServer* obj, const STTSCypherDataKey* keySrc, STTSCypherDataKey* keyDst, STNBX509* encCert);
	//ToDo: remove
	//BOOL TSServer_decDataKeyWithCurrentKey(STTSServer* obj, STTSCypherDataKey* dstKeyPlain, const STTSCypherDataKey* keyEnc);
	
#ifdef __cplusplus
} //extern "C"
#endif
	
#endif /* TSServer_h */
