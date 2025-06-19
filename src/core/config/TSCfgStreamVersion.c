//
//  TSCfgStreamVersion.c
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgStreamVersion.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"



//Stream version

STNBStructMapsRec TSCfgStreamVersion_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgStreamVersion_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgStreamVersion_sharedStructMap);
	if(TSCfgStreamVersion_sharedStructMap.map == NULL){
		STTSCfgStreamVersion s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgStreamVersion);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, isDisabled);
		NBStructMap_addStrPtrM(map, s, sid); //subid (unique on the parent stream)
		NBStructMap_addStrPtrM(map, s, uri);
		NBStructMap_addStructPtrM(map, s, params, TSStreamParams_getSharedStructMap());
		TSCfgStreamVersion_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgStreamVersion_sharedStructMap);
	return TSCfgStreamVersion_sharedStructMap.map;
}
