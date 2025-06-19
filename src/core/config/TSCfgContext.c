//
//  TSCfg.c
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgContext.h"
//
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

STNBStructMapsRec TSCfgContext_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgContext_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgContext_sharedStructMap);
	if(TSCfgContext_sharedStructMap.map == NULL){
		STTSCfgContext s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgContext);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStructM(map, s, threads, TSCfgThreads_getSharedStructMap());
		NBStructMap_addStructM(map, s, filesystem, TSCfgFilesystem_getSharedStructMap());
		NBStructMap_addStructM(map, s, cypher, TSCfgCypher_getSharedStructMap());
		NBStructMap_addStructM(map, s, smtp, TSCfgMail_getSharedStructMap());
		NBStructMap_addStructM(map, s, sms, TSCfgRestApiClt_getSharedStructMap());
		NBStructMap_addStructM(map, s, applePushServices, TSCfgApplePushServices_getSharedStructMap());
		NBStructMap_addStructM(map, s, firebaseCM, TSCfgFirebaseCM_getSharedStructMap());
		NBStructMap_addStructM(map, s, locale, TSCfgLocale_getSharedStructMap());
		NBStructMap_addStructM(map, s, deviceId, NBHttpCertSrcCfg_getSharedStructMap());
		NBStructMap_addStructM(map, s, identityId, NBHttpCertSrcCfg_getSharedStructMap());
		NBStructMap_addStructM(map, s, usrCert, NBHttpCertSrcCfg_getSharedStructMap());
		NBStructMap_addStructM(map, s, caCert, NBHttpCertSrcCfg_getSharedStructMap());
		TSCfgContext_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgContext_sharedStructMap);
	return TSCfgContext_sharedStructMap.map;
}
