//
//  TSClientOpq.h
//  thinstream
//
//  Created by Marcos Ortega on 5/3/19.
//

#ifndef TSClientOpq_h
#define TSClientOpq_h

#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBThread.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBString.h"
#include "nb/core/NBArray.h"
//
#include "nb/crypto/NBX509.h"
#include "nb/crypto/NBPKey.h"
#include "core/TSContext.h"
#include "core/TSRequestsMap.h"
#include "core/config/TSCfg.h"
#include "core/logic/TSStreamsStorage.h"
#include "core/logic/TSClientRequesterConn.h"
#include "core/logic/TSClientRequester.h"
#include "core/client/TSClientChannel.h"
#include "core/client/TSClientChannelRequest.h"
#include "core/TSCypher.h"
#include "core/TSCypherData.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//
	
	typedef struct STTSClientCtx_ {
		STNBPKey			key;
		STNBX509			cert;
		STNBSslContextRef	sslCtx;
	} STTSClientCtx;

	typedef struct STTSStreamViewerItmClt_ {
		STTSClientRequesterConn*	requester;
	} STTSStreamViewerItmClt;
	
	typedef struct STTSClientOpq_ {
		BOOL				isPreparing;
		BOOL				threadWorking2;	//time trigger thread (for programed requests)
		BOOL				shuttingDown;
		STNBThreadMutex		mutex;
		STNBThreadCond		cond;
		STTSCypher			cypher;
		UI32				cypherKeyLocalSeq;	//Key local
		UI32				cypherItfRemoteSeq;	//Itf remote
		STTSContext*		context;
		STNBX509*			caCert;
		//sync
		struct {
			STNBThreadMutex	mutex;
			BOOL			isReceiving;
			BOOL			isFirstDone;
			STNBString		streamId;
		} sync;
		//ssl
		struct {
			STNBString		curCertSerialHex;	//curDeviceId
			STNBSslContextRef defCtxt;
			STNBArray		ctxts;	//STTSClientCtx*
			UI32			ctxtsSeq;
		} ssl;
		//
		STTSRequestsMap		reqsMap;
		STNBArray			channels;	//STTSClientChannel*
		UI64				channelsReqsSeq; //To produce uniqueIds
		//App's orientation
		struct {
			SI32 portraitEnforcers;		//Amount of elements that required portrait mode
		} orientation;
		//streams
		struct {
			STTSClientRequesterRef	reqs;
		} streams;
	} STTSClientOpq;
	
	SI32	TSClient_loadCurrentDeviceKeyOpq(STTSClientOpq* opq);
	BOOL	TSClient_setCypherCertAndKeyOpq(STTSClientOpq* opq, STNBX509* cert, STNBPKey* key, const STTSCypherDataKey* sharedKeyEnc);
	BOOL	TSClient_setDeviceCertAndKeyOpq(STTSClientOpq* opq, STNBPKey* key, STNBX509* cert);
	
	BOOL	TSClient_concatDeviceIdOpq(STTSClientOpq* opq, STNBString* dst);

	//Log
	void	TSClient_logPendClearOpq(STTSClientOpq* opq);
	
	//Request
	UI64	TSClient_addRequestOpq(STTSClientOpq* opq, const ENTSRequestId reqId, const char** paramsPairsAndNull, const char* body, const UI32 bodySz, const UI64 secsWait, TSClientChannelReqCallbackFunc lstnr, void* lstnrParam);
	
	//Certiticates validation
	BOOL	TSClient_certIsSignedByCAOpq(STTSClientOpq* opq, STNBX509* cert);

	
#ifdef __cplusplus
} //extern "C" {
#endif

#endif /* TSClientOpq_h */
