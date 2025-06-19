//
//  TSIdentityLocal.c
//  thinstream
//
//  Created by Marcos Ortega on 2/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSIdentityLocal.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBMngrStructMaps.h"

//
//Identity local (current user)
//

STNBStructMapsRec TSIdentityLocal_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSIdentityLocal_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSIdentityLocal_sharedStructMap);
	if(TSIdentityLocal_sharedStructMap.map == NULL){
		STTSIdentityLocal s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSIdentityLocal);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, identityId);
		NBStructMap_addStructM(map, s, encKey, TSCypherDataKey_getSharedStructMap());
		NBStructMap_addUIntM(map, s, infosShownMask);	//ENVisualHelpBit
		//Registration
		NBStructMap_addPtrToArrayOfBytesM(map, s, pass, passSz, ENNBStructMapSign_Unsigned);
		NBStructMap_addBoolM(map, s, passIsUsrDef);
		NBStructMap_addStrPtrM(map, s, name);
		NBStructMap_addStrPtrM(map, s, countryIso2);
		NBStructMap_addStrPtrM(map, s, email);
		NBStructMap_addStrPtrM(map, s, phone);
		NBStructMap_addStrPtrM(map, s, imei);
		NBStructMap_addStructM(map, s, photoIdExpDate, NBDate_getSharedStructMap());
		NBStructMap_addIntM(map, s, photosIdCount);
		//
		TSIdentityLocal_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSIdentityLocal_sharedStructMap);
	return TSIdentityLocal_sharedStructMap.map;
}
