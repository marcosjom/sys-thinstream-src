//
//  TSCfg.c
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgThreads.h"
//
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//ThreadsPollster

STNBStructMapsRec TSCfgThreadPolling_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgThreadPolling_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgThreadPolling_sharedStructMap);
	if(TSCfgThreadPolling_sharedStructMap.map == NULL){
		STTSCfgThreadPolling s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgThreadPolling);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addUIntM(map, s, timeout); //poll ms timeout
		TSCfgThreadPolling_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgThreadPolling_sharedStructMap);
	return TSCfgThreadPolling_sharedStructMap.map;
}

//Threads

STNBStructMapsRec TSCfgThreads_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgThreads_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgThreads_sharedStructMap);
	if(TSCfgThreads_sharedStructMap.map == NULL){
		STTSCfgThreads s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgThreads);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addUIntM(map, s, amount); //amount of threads
		NBStructMap_addStructM(map, s, polling, TSCfgThreadPolling_getSharedStructMap()); //polling
		TSCfgThreads_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgThreads_sharedStructMap);
	return TSCfgThreads_sharedStructMap.map;
}


