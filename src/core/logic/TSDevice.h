//
//  TSDevice.h
//  thinstream
//
//  Created by Marcos Ortega on 5/3/19.
//

#ifndef TSDevice_h
#define TSDevice_h

#include "nb/core/NBStructMap.h"
#include "nb/core/NBString.h"
#include "nb/files/NBFilesystem.h"
#include "nb/crypto/NBX509.h"
//
#include "core/TSContext.h"
#include "core/TSCypher.h"
#include "core/server/TSServerLstnr.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//
	//Notification token type
	//
	
	typedef enum ENTSNotifTokenType_ {
		ENTSNotifTokenType_iOS = 0,		//iOS
		ENTSNotifTokenType_Android,		//Android
		ENTSNotifTokenType_Count
	} ENTSNotifTokenType;
	
	const STNBEnumMap* TSNotifTokenType_getSharedEnumMap(void);
	
	//
	//Notif token
	//
	
	typedef struct STTSNotifToken_ {
		ENTSNotifTokenType type;
		char*		token;	//Hex
		char*		lang;	//Prefered language
	} STTSNotifToken;
	
	const STNBStructMap* TSNotifToken_getSharedStructMap(void);
	
	//
	//Device token (secondary device to build first key/certificate)
	//
	
	typedef struct STTSDeviceToken_ {
		UI64				created;	//timestamp
		char*				token;		//Hex
		//Consumer
		char*				identityId;		//IdentityId that consumed this token
		ENTSDeviceType		deviceType;
		ENTSDeviceFamily	deviceFamily;
		STTSCypherDataKey	encKey;
		STTSCypherData		linkName;
		BOOL				isAttestBackup;
		UI64				validSince;		//Time of linkage
		UI64				validUntil;		//Time of expiration
	} STTSDeviceToken;
	
	const STNBStructMap* TSDeviceToken_getSharedStructMap(void);
	
	//
	//Device props (made compact to fit in a QRCode)
	//

	typedef struct STTSDeviceProps_ {
		char*				nm;	//name
		ENTSDeviceType		tp;	//type
		ENTSDeviceFamily	fm;	//family
	} STTSDeviceProps;

	const STNBStructMap* TSDeviceProps_getSharedStructMap(void);

	//
	//Device
	//
	
	typedef struct STTSDevice_ {
		char*				identityId;		//Identity linked as main device (null is secondary device)
		STTSNotifToken		notifToken;
	} STTSDevice;
	
	const STNBStructMap* TSDevice_getSharedStructMap(void);
	
	//-----------------------------
	
	//Set notification token request
	
	SI32 TSNotifToken_srvProcess(void* srvrOpq, const STNBHttpMessage* msg, void* stData, const STNBStructMap* stDataMap, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk);
	
	//Set notification token response
	
	SI32 TSNotifToken_cltProcess(void* cltOpq, const STNBHttpMessage* msg, void* stData, const STNBStructMap* stDataMap, void* reqLstnrParam);
	
	
	
	//---------------------------------------
	
	//
	//Request for device-error-found
	//
	
	typedef struct STTSDeviceLogNewReq_ {
		UI64		time;
		char*		log;
	} STTSDeviceLogNewReq;
	
	const STNBStructMap* TSDeviceLogNewReq_getSharedStructMap(void);
	
	SI32 TSDeviceLogNewReq_srvProcess(void* srvrOpq, const STNBHttpMessage* msg, void* stData, const STNBStructMap* stDataMap, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk);
	
	//
	//Response from device-error-found
	//
	
	typedef struct STTSDeviceLogNewResp_ {
		UI64		time;
		char*		logId;
	} STTSDeviceLogNewResp;
	
	const STNBStructMap* TSDeviceLogNewResp_getSharedStructMap(void);
	
	SI32 TSDeviceLogNewReq_cltProcess(void* cltOpq, const STNBHttpMessage* msg, void* stData, const STNBStructMap* stDataMap, void* reqLstnrParam);
	
	//---------------------------------------
	
	//
	// Tools
	//
	void TSDevice_getPassword(STTSCypher* obj, STNBString* dst, const char* deviceId, const UI8 passLen, const BOOL creatIfNecesary);
	void TSDevice_concatFormatedId(const char* deviceId, const UI8 blcksSz, STNBString* dst);
	
#ifdef __cplusplus
} //extern "C" {
#endif

#endif /* TSDevice_h */
