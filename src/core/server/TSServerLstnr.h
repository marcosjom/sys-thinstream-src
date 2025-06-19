//
//  TSServerLstnr.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSServerLstnr_h
#define TSServerLstnr_h

#include "nb/crypto/NBX509.h"
#include "nb/net/NBHttpMessage.h"
#include "core/TSCypherData.h"
#include "core/TSRequestsMap.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//Keys pairs
	
	typedef struct STTSDeviceDecKey_ {
		BOOL				processed;
		BOOL				success;
		STTSCypherDataKey	keyForIdentity;	//encrypted for identity
		STTSCypherDataKey	keyForDevice;	//encrypted for device
	} STTSDeviceDecKey;
	
	const STNBStructMap* TSDeviceDecKey_getSharedStructMap(void);
	
	//
	
	typedef struct STTSServerLstnr_ {
		void*		obj;
		BOOL		(*cltConnected)(void* param, const STNBX509* cert);
		void		(*cltDisconnected)(void* param, const STNBX509* cert);
		BOOL		(*cltRequested)(void* param, const STNBX509* cert, const ENTSRequestId reqId, const UI32 respCode, const char* respReason, const UI32 respBodyLen);
	} STTSServerLstnr;
	
	typedef struct STTSServerSyncItf_ {
		void*		obj;
		BOOL		(*isDeviceOnline)(void* param, const char* deviceId, BOOL* dstIsUnlocked, BOOL* dstIsSyncMeetings, BOOL* dstCertsSendFirst, BOOL* dstMeetsSendFirst);
	} STTSServerSyncItf;
	
#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSServerLstnr_h */
