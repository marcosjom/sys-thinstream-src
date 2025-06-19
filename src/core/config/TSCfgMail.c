//
//  TSCfgMail.c
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgMail.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

STNBStructMapsRec TSCfgMail_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgMail_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgMail_sharedStructMap);
	if(TSCfgMail_sharedStructMap.map == NULL){
		STTSCfgMail s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgMail);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, isMsExchange);
		NBStructMap_addStrPtrM(map, s, from);
		NBStructMap_addStrPtrM(map, s, server);
		NBStructMap_addIntM(map, s, port);
		NBStructMap_addBoolM(map, s, useSSL);
		NBStructMap_addStrPtrM(map, s, target);
		NBStructMap_addStrPtrM(map, s, user);
		NBStructMap_addStrPtrM(map, s, pass);
		TSCfgMail_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgMail_sharedStructMap);
	return TSCfgMail_sharedStructMap.map;
}
