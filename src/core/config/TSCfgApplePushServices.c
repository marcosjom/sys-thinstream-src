//
//  TSCfgApplePushServices.c
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgApplePushServices.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

STNBStructMapsRec TSCfgApplePushServices_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgApplePushServices_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgApplePushServices_sharedStructMap);
	if(TSCfgApplePushServices_sharedStructMap.map == NULL){
		STTSCfgApplePushServices s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgApplePushServices);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, server);
		NBStructMap_addIntM(map, s, port);
		NBStructMap_addBoolM(map, s, useSSL);
		NBStructMap_addStrPtrM(map, s, apnsTopic);
		NBStructMap_addStructM(map, s, cert, NBHttpCertSrcCfg_getSharedStructMap());
		TSCfgApplePushServices_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgApplePushServices_sharedStructMap);
	return TSCfgApplePushServices_sharedStructMap.map;
}
