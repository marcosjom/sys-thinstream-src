//
//  TSCfgCypher.c
//  thinstream
//
//  Created by Marcos Ortega on 8/21/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgCypher.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//StructMap

STNBStructMapsRec TSCfgCypherAes_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgCypherAes_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgCypherAes_sharedStructMap);
	if(TSCfgCypherAes_sharedStructMap.map == NULL){
		STTSCfgCypherAes s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgCypherAes);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, salt);
		NBStructMap_addUIntM(map, s, iterations);
		TSCfgCypherAes_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgCypherAes_sharedStructMap);
	return TSCfgCypherAes_sharedStructMap.map;
}

STNBStructMapsRec TSCfgCypher_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgCypher_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgCypher_sharedStructMap);
	if(TSCfgCypher_sharedStructMap.map == NULL){
		STTSCfgCypher s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgCypher);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, enabled);
		NBStructMap_addUIntM(map, s, passLen);
		NBStructMap_addIntM(map, s, keysCacheSz);
		NBStructMap_addIntM(map, s, jobThreads);
		NBStructMap_addStructM(map, s, aes, TSCfgCypherAes_getSharedStructMap());
		TSCfgCypher_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgCypher_sharedStructMap);
	return TSCfgCypher_sharedStructMap.map;
}
