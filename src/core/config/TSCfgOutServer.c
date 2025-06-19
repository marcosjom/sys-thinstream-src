//
//  TSCfgOutServer.c
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgOutServer.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//Server

STNBStructMapsRec TSCfgOutServer_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgOutServer_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgOutServer_sharedStructMap);
	if(TSCfgOutServer_sharedStructMap.map == NULL){
		STTSCfgOutServer s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgOutServer);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, server);
		NBStructMap_addIntM(map, s, port);
		NBStructMap_addBoolM(map, s, useSSL);
		NBStructMap_addStrPtrM(map, s, reqId);
		TSCfgOutServer_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgOutServer_sharedStructMap);
	return TSCfgOutServer_sharedStructMap.map;
}
