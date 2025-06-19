//
//  TSCfg.c
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgLocale.h"
//
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

STNBStructMapsRec TSCfgLocale_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgLocale_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgLocale_sharedStructMap);
	if(TSCfgLocale_sharedStructMap.map == NULL){
		STTSCfgLocale s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgLocale);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addPtrToArrayOfStrPtrM(map, s, paths, pathsSz, ENNBStructMapSign_Unsigned);
		TSCfgLocale_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgLocale_sharedStructMap);
	return TSCfgLocale_sharedStructMap.map;
}

