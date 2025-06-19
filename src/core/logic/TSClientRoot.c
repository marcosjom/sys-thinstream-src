//
//  TSClientRoot.c
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSClientRoot.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBMngrStructMaps.h"

//Error log send-mode

STNBEnumMapRecord DRErrorLogMode_sharedEnumMapRecs[] = {
	{ ENDRErrorLogSendMode_Ask, "ENDRErrorLogSendMode_Ask", "ask" }
	, { ENDRErrorLogSendMode_Allways, "ENDRErrorLogSendMode_Allways", "sendAll" }
	, { ENDRErrorLogSendMode_Never, "ENDRErrorLogSendMode_Never", "never" }
};

STNBEnumMap DRErrorLogMode_sharedEnumMap = {
	"ENDRErrorLogSendMode"
	, DRErrorLogMode_sharedEnumMapRecs
	, (sizeof(DRErrorLogMode_sharedEnumMapRecs) / sizeof(DRErrorLogMode_sharedEnumMapRecs[0]))
};

const STNBEnumMap* ENDRErrorLogSendMode_getSharedEnumMap(void){
	return &DRErrorLogMode_sharedEnumMap;
}

//
//Root data (registration)
//

STNBStructMapsRec TSClientRootReq_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSClientRootReq_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSClientRootReq_sharedStructMap);
	if(TSClientRootReq_sharedStructMap.map == NULL){
		STTSClientRootReq s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSClientRootReq);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, clinicName);
		NBStructMap_addStrPtrM(map, s, firstNames);
		NBStructMap_addStrPtrM(map, s, lastNames);
		NBStructMap_addUIntM(map, s, verifSeqIdTime);
		NBStructMap_addStrPtrM(map, s, verifSeqId);
		NBStructMap_addBoolM(map, s, willOverrideOther);
		NBStructMap_addBoolM(map, s, canHaveMoreCodes);
		NBStructMap_addBoolM(map, s, linkedToClinic);
		NBStructMap_addBoolM(map, s, isAdmin);
		NBStructMap_addPtrToArrayOfBytesM(map, s, pkeyDER, pkeyDERSz, ENNBStructMapSign_Unsigned);
		//
		TSClientRootReq_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSClientRootReq_sharedStructMap);
	return TSClientRootReq_sharedStructMap.map;
}

//
//Root data
//

STNBStructMapsRec TSClientRoot_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSClientRoot_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSClientRoot_sharedStructMap);
	if(TSClientRoot_sharedStructMap.map == NULL){
		STTSClientRoot s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSClientRoot);
		NBStructMap_init(map, sizeof(s));
		//Old
		NBStructMap_addEnumM(map, s, errLogMode, ENDRErrorLogSendMode_getSharedEnumMap());
		//New
		NBStructMap_addStrPtrM(map, s, deviceId);
		NBStructMap_addStructM(map, s, sharedKeyEnc, TSCypherDataKey_getSharedStructMap());
		//Registration
		NBStructMap_addStructM(map, s, req, TSClientRootReq_getSharedStructMap());
		//
		TSClientRoot_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSClientRoot_sharedStructMap);
	return TSClientRoot_sharedStructMap.map;
}

//Tools

BOOL TSClientRoot_getCurrentDeviceId(STTSContext* context, STNBString* dstDeviceId){
	BOOL r = FALSE;
	STNBStorageRecordRead ref = TSContext_getStorageRecordForRead(context, "client/_root/", "_current", NULL, TSClientRoot_getSharedStructMap());
	if(ref.data != NULL){
		STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
		if(!NBString_strIsEmpty(priv->deviceId)){
			if(dstDeviceId != NULL){
				NBString_set(dstDeviceId, priv->deviceId);
			}
			r = TRUE;
		}
	}
	TSContext_returnStorageRecordFromRead(context, &ref);
	return r;
}

