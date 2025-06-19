//
//  TSDevice.c
//  thinstream
//
//  Created by Marcos Ortega on 5/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSDevice.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/2d/NBBitmap.h"
#include "nb/2d/NBPng.h"
#include "nb/crypto/NBBase64.h"
//
#include "core/server/TSServerOpq.h"
#include "core/client/TSClientOpq.h"

//
//Notification token type
//

STNBEnumMapRecord TSNotifTokenType_sharedEnumMapRecs[] = {
	{ ENTSNotifTokenType_iOS, "ENTSNotifTokenType_iOS", "ios" }
	, { ENTSNotifTokenType_Android, "ENTSNotifTokenType_Android", "android" }
};

STNBEnumMap TSNotifTokenType_sharedEnumMap = {
	"ENTSNotifTokenType"
	, TSNotifTokenType_sharedEnumMapRecs
	, (sizeof(TSNotifTokenType_sharedEnumMapRecs) / sizeof(TSNotifTokenType_sharedEnumMapRecs[0]))
};

const STNBEnumMap* TSNotifTokenType_getSharedEnumMap(void){
	return &TSNotifTokenType_sharedEnumMap;
}

//
//Notif token
//

STNBStructMapsRec TSNotifToken_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSNotifToken_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSNotifToken_sharedStructMap);
	if(TSNotifToken_sharedStructMap.map == NULL){
		STTSNotifToken s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSNotifToken);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addEnumM(map, s, type, TSNotifTokenType_getSharedEnumMap());
		NBStructMap_addStrPtrM(map, s, token);
		NBStructMap_addStrPtrM(map, s, lang);
		TSNotifToken_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSNotifToken_sharedStructMap);
	return TSNotifToken_sharedStructMap.map;
}

//
//Device token (secondary device to build first key/certificate)
//

STNBStructMapsRec TSDeviceToken_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSDeviceToken_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSDeviceToken_sharedStructMap);
	if(TSDeviceToken_sharedStructMap.map == NULL){
		STTSDeviceToken s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSDeviceToken);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addUIntM(map, s, created);
		NBStructMap_addStrPtrM(map, s, token);
		NBStructMap_addStrPtrM(map, s, identityId);
		NBStructMap_addEnumM(map, s, deviceType, TSDeviceType_getSharedEnumMap());
		NBStructMap_addEnumM(map, s, deviceFamily, TSDeviceFamily_getSharedEnumMap());
		NBStructMap_addStructM(map, s, encKey, TSCypherDataKey_getSharedStructMap());
		NBStructMap_addStructM(map, s, linkName, TSCypherData_getSharedStructMap());
		NBStructMap_addBoolM(map, s, isAttestBackup);
		NBStructMap_addUIntM(map, s, validSince);
		NBStructMap_addUIntM(map, s, validUntil);
		TSDeviceToken_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSDeviceToken_sharedStructMap);
	return TSDeviceToken_sharedStructMap.map;
}

//
//Device
//

STNBStructMapsRec TSDevice_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSDevice_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSDevice_sharedStructMap);
	if(TSDevice_sharedStructMap.map == NULL){
		STTSDevice s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSDevice);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, identityId); //Identity linked to main device (null is secondary device)
		NBStructMap_addStructM(map, s, notifToken, TSNotifToken_getSharedStructMap());
		TSDevice_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSDevice_sharedStructMap);
	return TSDevice_sharedStructMap.map;
}

//
//Device props
//

STNBStructMapsRec TSDeviceProps_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSDeviceProps_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSDeviceProps_sharedStructMap);
	if(TSDeviceProps_sharedStructMap.map == NULL){
		STTSDeviceProps s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSDeviceProps);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, nm);
		NBStructMap_addEnumM(map, s, tp, TSDeviceType_getSharedEnumMap());
		NBStructMap_addEnumM(map, s, fm, TSDeviceFamily_getSharedEnumMap());
		TSDeviceProps_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSDeviceProps_sharedStructMap);
	return TSDeviceProps_sharedStructMap.map;
}

//-----------------------------

//Set notification token request

SI32 TSNotifToken_srvProcess(void* srvrOpq, const STNBHttpMessage* msg, void* stData, const STNBStructMap* stDataMap, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk){
	BOOL r = FALSE;
	/*NBASSERT(NBHttpServiceResp_getResponseCode(req) == 0)
	STTSServerOpq* opq = (STTSServerOpq*)srvrOpq;
	if(stDataMap != TSNotifToken_getSharedStructMap()){
        r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 501, "Server missconfig structMap unmatch")
            && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Server missconfig structMap unmatch");
	} else if(stData == NULL){
         r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Empty request")
             && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Empty request");
	} else {
		//const char* langPref = NBHttpHeader_getFieldOrDefault(&msg->header, "lang", "en", FALSE);
		/ *{
			const STTSNotifToken* data = (STTSNotifToken*)stData;
			BOOL isMainDevice = FALSE;
			STNBString curDeviceId;
			NBString_init(&curDeviceId);
			if(!TSDevice_getIdentitiesFromCert(opq->context, cert, &isMainDevice, NULL, &curDeviceId, NULL, NULL, NULL)){
                 r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Id records not found")
                     && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Id records not found");
			} else if(curDeviceId.length <= 0){
                r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Undetermined current deviceId")
                    && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Undetermined current deviceId");
			/ *} else if(myIdentityId.length <= 0){
                r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Undetermined identityId")
                    && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Undetermined identityId");
			} else if(myEmailHash.length <= 0){
                r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Undetermined identity mailboxId1")
                    && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Undetermined identity mailboxId1");
			} else if(myPhoneHash.length <= 0){
                r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Undetermined identity mailboxId2")
                    && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Undetermined identity mailboxId2");
                * /
			} else {
				STNBStorageRecordWrite ref = TSContext_getStorageRecordForWrite(opq->context, "devices/", curDeviceId.str, FALSE, NULL, TSDevice_getSharedStructMap());
				if(ref.data == NULL){
                     r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Device not found")
                         && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Device not found");
				} else {
					STTSDevice* priv = (STTSDevice*)ref.data->priv.stData;
					if(priv == NULL){
                     r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Device data not found")
                         && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Device data not found");
					} else {
						if(NBString_strIsEmpty(data->token) && !NBString_strIsEmpty(priv->notifToken.token)){
							PRINTF_WARNING("Deleting current device notification token.\n");
						}
						NBStruct_stRelease(TSNotifToken_getSharedStructMap(), &priv->notifToken, sizeof(priv->notifToken));
						NBStruct_stClone(TSNotifToken_getSharedStructMap(), data, sizeof(*data), &priv->notifToken, sizeof(priv->notifToken));
						ref.privModified = TRUE;
					}
				}
				TSContext_returnStorageRecordFromWrite(opq->context, &ref);
			}
			NBString_release(&curDeviceId);
		}* /
		//Send response
		if(NBHttpServiceResp_getResponseCode(req) == 0){
             r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 200, "Token set")
                 && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Token set");
		}
	}*/
	return r;
}

//Set notification token response

SI32 TSNotifToken_cltProcess(void* cltOpq, const STNBHttpMessage* msg, void* stData, const STNBStructMap* stDataMap, void* reqLstnrParam){
	BOOL r = TRUE;
	/*if(stDataMap != TSDeviceNotifTokenSetResp_getSharedStructMap()){
		PRINTF_INFO("Client missconfig structMap unmatch.\n"); NBASSERT(FALSE)
	} else if(msg->header.statusLine == NULL){
		PRINTF_INFO("Client, received repsonse with no header.\n");
	} else if(msg->header.statusLine->statusCode < 200 || msg->header.statusLine->statusCode >= 300){
		PRINTF_INFO("Client, received repsonse-code(%d): '%s'.\n", msg->header.statusLine->statusCode, &msg->header.strs.str[msg->header.statusLine->reasonPhrase]);
	} else {
		STTSClientOpq* opq = (STTSClientOpq*)cltOpq;
		//Process response
		NBASSERT(stData != NULL)
		if(stData != NULL){
			STTSDeviceNotifTokenSetResp* respData = (STTSDeviceNotifTokenSetResp*)stData;
			PRINTF_INFO("Meeting, attest invalidated for meeting '%s'.\n", respData->meetingId);
		}
	}*/
	PRINTF_INFO("TSNotifToken_cltProcess returned.\n");
	return r;
}


//---------------------------------------

//
//Request for device-error-found
//

STNBStructMapsRec TSDeviceLogNewReq_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSDeviceLogNewReq_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSDeviceLogNewReq_sharedStructMap);
	if(TSDeviceLogNewReq_sharedStructMap.map == NULL){
		STTSDeviceLogNewReq s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSDeviceLogNewReq);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addUIntM(map, s, time);
		NBStructMap_addStrPtrM(map, s, log);
		TSDeviceLogNewReq_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSDeviceLogNewReq_sharedStructMap);
	return TSDeviceLogNewReq_sharedStructMap.map;
}

SI32 TSDeviceLogNewReq_srvProcess(void* srvrOpq, const STNBHttpMessage* msg, void* stData, const STNBStructMap* stDataMap, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk){
	BOOL r = FALSE;
	/*
	NBASSERT(NBHttpServiceResp_getResponseCode(req) == 0)
	//STTSServerOpq* opq = (STTSServerOpq*)srvrOpq;
	if(stDataMap != TSDeviceLogNewReq_getSharedStructMap()){
         r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 501, "Server missconfig structMap unmatch")
             && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Server missconfig structMap unmatch");
	} else if(stData == NULL){
         r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 400, "Empty request")
             && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Empty request");
	} else {
		//const char* langPref = NBHttpHeader_getFieldOrDefault(&msg->header, "lang", "en", FALSE);
		const STNBStructMap* respMap = TSDeviceLogNewResp_getSharedStructMap();
		STTSDeviceLogNewResp resp;
		NBMemory_setZeroSt(resp, STTSDeviceLogNewResp);
		resp.time			= NBDatetime_getCurUTCTimestamp();
		{
			const STTSDeviceLogNewReq* data = (STTSDeviceLogNewReq*)stData;
			if(data != NULL){
				if(!NBString_strIsEmpty(data->log)){
					STNBString logId;
					NBString_init(&logId);
					TSContext_concatRandomHashHex(&logId, 128);
					{
						PRINTF_CONSOLE_ERROR("[start-of-logId:%s] from device:\n%s\n[end-of-logId:%s]\n", logId.str, data->log, logId.str);
						NBString_strFreeAndNewBuffer(&resp.logId, logId.str);
					}
					NBString_release(&logId);
				}
			}
		}
		//Send response
		if(NBHttpServiceResp_getResponseCode(req) == 0){
             r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 200, "OK")
                 && NBHttpServiceResp_concatResponseBodyStructAsJson(req, respMap, &resp, sizeof(resp));;
		}
		NBStruct_stRelease(respMap, &resp, sizeof(resp));
	}*/
	return r;
}

//
//Response from device-error-found
//

STNBStructMapsRec TSDeviceLogNewResp_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSDeviceLogNewResp_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSDeviceLogNewResp_sharedStructMap);
	if(TSDeviceLogNewResp_sharedStructMap.map == NULL){
		STTSDeviceLogNewResp s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSDeviceLogNewResp);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addUIntM(map, s, time);
		NBStructMap_addStrPtrM(map, s, logId);
		TSDeviceLogNewResp_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSDeviceLogNewResp_sharedStructMap);
	return TSDeviceLogNewResp_sharedStructMap.map;
}

SI32 TSDeviceLogNewReq_cltProcess(void* cltOpq, const STNBHttpMessage* msg, void* stData, const STNBStructMap* stDataMap, void* reqLstnrParam){
	BOOL r = TRUE;
	if(stDataMap != TSDeviceLogNewResp_getSharedStructMap()){
		PRINTF_INFO("Client missconfig structMap unmatch.\n"); NBASSERT(FALSE)
	} else if(msg->header.statusLine == NULL){
		PRINTF_INFO("Client, received repsonse with no header.\n");
	} else if(msg->header.statusLine->statusCode < 200 || msg->header.statusLine->statusCode >= 300){
		PRINTF_INFO("Client, received repsonse-code(%d): '%s'.\n", msg->header.statusLine->statusCode, &msg->header.strs.str[msg->header.statusLine->reasonPhrase]);
	} else {
		STTSClientOpq* opq = (STTSClientOpq*)cltOpq;
		//Process response
		NBASSERT(stData != NULL)
		if(stData != NULL){
			IF_PRINTF(STTSDeviceLogNewResp* respData = (STTSDeviceLogNewResp*)stData;)
			//Clear log data
			TSClient_logPendClearOpq(opq);
			PRINTF_INFO("Client, device log report saves as 'logId:%s'.\n", respData->logId);
		}
	}
	return r;
}

//---------------------------------------

//
// Tools
//

void TSDevice_getPassword(STTSCypher* obj, STNBString* dst, const char* deviceId, const UI8 passLen, const BOOL creatIfNecesary){
	STNBString passName;
	NBString_init(&passName);
	NBString_concat(&passName, "dev_");
	NBString_concat(&passName, deviceId);
	TSCypher_getPassword(obj, dst, passName.str, passLen, creatIfNecesary);
	NBString_release(&passName);
}

void TSDevice_concatFormatedId(const char* deviceId, const UI8 blcksSz, STNBString* dst){
	if(dst != NULL){
		if(deviceId != NULL && blcksSz > 0){
			UI32 added = 0;
			const char* ptr = deviceId;
			while(*ptr != '\0'){
				if(((added + 1) % (blcksSz + 1)) == 0){
					NBString_concatByte(dst, ' ');
					added++;
				}
				NBString_concatByte(dst, *ptr);
				added++;
				ptr++;
			}
		}
	}
}
