//
//  TSServerLstnr.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSServerLstnr.h"
#include "nb/core/NBMngrStructMaps.h"

//Keys pairs

STNBStructMapsRec TSDeviceDecKey_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSDeviceDecKey_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSDeviceDecKey_sharedStructMap);
	if(TSDeviceDecKey_sharedStructMap.map == NULL){
		STTSDeviceDecKey s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSDeviceDecKey);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, processed);
		NBStructMap_addBoolM(map, s, success);
		NBStructMap_addStructM(map, s, keyForIdentity, TSCypherDataKey_getSharedStructMap());
		NBStructMap_addStructM(map, s, keyForDevice, TSCypherDataKey_getSharedStructMap());
		TSDeviceDecKey_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSDeviceDecKey_sharedStructMap);
	return TSDeviceDecKey_sharedStructMap.map;
}
