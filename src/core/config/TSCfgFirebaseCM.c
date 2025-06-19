//
//  TSCfgFirebaseCM.c
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgFirebaseCM.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

STNBStructMapsRec TSCfgFirebaseCM_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgFirebaseCM_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgFirebaseCM_sharedStructMap);
	if(TSCfgFirebaseCM_sharedStructMap.map == NULL){
		STTSCfgFirebaseCM s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgFirebaseCM);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, server);
		NBStructMap_addIntM(map, s, port);
		NBStructMap_addBoolM(map, s, useSSL);
		NBStructMap_addStrPtrM(map, s, key);
		NBStructMap_addStructM(map, s, cert, NBHttpCertSrcCfg_getSharedStructMap());
		TSCfgFirebaseCM_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgFirebaseCM_sharedStructMap);
	return TSCfgFirebaseCM_sharedStructMap.map;
}
