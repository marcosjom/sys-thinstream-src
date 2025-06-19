//
//  TSServer.h
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#ifndef TSServerOpq_h
#define TSServerOpq_h

#include "nb/core/NBString.h"
#include "core/config/TSCfg.h"
#include "core/TSContext.h"
//
#include "nb/core/NBStruct.h"
#include "nb/core/NBThread.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBThreadsPool.h"
#include "nb/core/NBDataPtrsPool.h"
#include "nb/core/NBDataPtrsPool.h"
#include "nb/core/NBDataPtrsStats.h"
//
#include "nb/net/NBHttpService.h"
#include "nb/net/NBRtspClient.h"
#include "nb/net/NBRtspClientStats.h"
//
#include "core/config/TSCfg.h"
#include "core/server/TSNotifsList.h"
#include "core/server/TSApiRestClt.h"
#include "core/server/TSMailClt.h"
#include "core/server/TSApplePushServicesClt.h"
#include "core/server/TSFirebaseCMClt.h"
#include "core/server/TSServerLogicClt.h"
#include "core/server/TSServerStreams.h"
#include "core/logic/TSClientRequesterConnStats.h"
#include "core/logic/TSClientRequesterConn.h"
#include "core/logic/TSClientRequester.h"
#include "core/TSRequestsMap.h"
//
#include "core/server/TSServerLstnr.h"
#include "core/server/TSServerStats.h"
#include "core/logic/TSDevice.h"
#include "core/logic/TSStreamsStorage.h"

//

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef enum ENTSServerState_ {
		ENTSServerState_stopped = 0,	//nothing is loaded nor running
		ENTSServerState_stopping,		//things could be running
		ENTSServerState_preparing,		//loading data
		ENTSServerState_prepared,		//data loaded
		ENTSServerState_running			//data loaded and running
	} ENTSServerState;

    //TSServer

	typedef struct STTSServerOpq_ {
		STNBThreadMutex				mutex;
		STNBThreadCond				cond;
		//context
		struct {
			STTSContext*			cur;
			BOOL					isListening;
			STNBTimestampMicro		tickLast;
		} context;
        //loading
        struct {
            STNBThreadsPoolRef      threadsPool;
            SI32                    prepareDepth; //prepare calls
            SI32                    storageDepth; //storage-load calls
        } loading;
		UI32						secsPrepared;
		SI32						msRunAndQuit;
		//stop-flag
		struct {
			const BOOL*				external;	//read-only
			BOOL					applied;
			BOOL					appliedToThread;	//(complementary to 'TSContext_startAndUseThisThread') 
		} stopFlag;
		//state
		struct {
			ENTSServerState			cur;
		} state;
		//Defaults
		STNBPKey*					caKey;
		STNBX509*					caCert;
		STTSRequestsMap				reqsMap;
		//stats
		struct {
			STTSServerStats			provider;
			BOOL					isFlushing;
			STNBTimestampMicro		startTime;
			//print
			struct {
				STTSServerStatsData data;
				STNBString			strPrint;
				STNBArray			ports;
				STNBArray			ssrcs;
			} print;
		} stats;
		//http
		struct {
			STNBHttpServiceRef		service;
			//stats
			struct {
				STNBHttpServiceCmdState flushCmd;
				STNBTimestampMicro		startTime;
			} stats;
		} http;
		/*
		STTSApiRestClt				smsClt;		//Sms sender client
		STTSMailClt					mailClt;	//Mail sender client
		STTSApplePushServicesClt	applePushServices;	//AppleNotifs sender client
		STTSFirebaseCMClt			firebaseCMClt;		//Firebaseotifs sender client
		STTSServerLogicClt			logicClt;
		*/
		STTSServerLstnr				lstnr;
		STTSServerSyncItf*			syncItf;
		//RTSP
		struct {
			BOOL					enabled;
			STNBRtspClientStats		stats;
		} rtsp;
		//streams
		struct {
			STTSCfgFilesystemStreamsParams cfg;
			STTSServerStreams		mngr;
			//storage
			struct {
				STTSStreamsStorage		mngr;
				STTSStreamsStorageStats	stats;
			} storage;
			//Requesters
			struct {
				STTSClientRequesterRef	mngr;
				//stats
				struct {
					STTSClientRequesterConnStats	conn;
					STTSClientRequesterCmdState		flushCmd;
					STNBTimestampMicro				startTime;
				} stats;
			} reqs;
		} streams;
	} STTSServerOpq;

	//httpSrv callbacks
	BOOL TSServer_httpCltConnected_(STNBHttpServiceRef srv, const UI32 port, STNBHttpServiceConnRef conn, void* usrData);
	void TSServer_httpCltDisconnected_(STNBHttpServiceRef srv, const UI32 port, STNBHttpServiceConnRef conn, void* usrData);
    BOOL TSServer_httpCltReqArrived_(STNBHttpServiceRef srv, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk, void* usrData);    //called when header-frist-line arrived, when header completion arrived or when body completing arrived; first to populate required methods into 'dstLtnr' take ownership and stops further calls to this method.
	
	//Itf to sync server
	BOOL TSServer_isDeviceOnlineOpq(STTSServerOpq* opq, const char* deviceId, BOOL* dstIsUnlocked, BOOL* dstIsSyncMeetings, BOOL* dstCertsSendFirst, BOOL* dstMeetsSendFirst);
	
	//Re enc
	BOOL TSServer_reEncDataKeyOpq(STTSServerOpq* opq, const STTSCypherDataKey* keySrc, STTSCypherDataKey* keyDst, STNBX509* encCert);
	
	//Locale
	const char* TSServer_getLocaleStrOpq(STTSServerOpq* opq, const char* lang, const char* strId, const char* defaultValue);
	
	//SMS
	BOOL TSServer_addSmsToLocalQueue(STTSServerOpq* opq, const char* to, const char* body);

	//Mail
	BOOL TSServer_addMailToLocalQueue(STTSServerOpq* opq, const char* to, const char* subject, const char* body, const char* attachFilename, const char* attachMime, const void* attachData, const UI32 attachDataSz);
	
	//Notifications
	BOOL TSServer_addAppleNotification(STTSServerOpq* opq, const char* token, const char* text, const char* extraDataName, const char* extraDataValue);
	BOOL TSServer_addAndroidNotification(STTSServerOpq* opq, const char* token, const char* text, const char* extraDataName, const char* extraDataValue);
	
#ifdef __cplusplus
} //extern "C"
#endif
	
#endif /* TSServer_h */
