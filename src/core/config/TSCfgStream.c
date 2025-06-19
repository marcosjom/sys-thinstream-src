//
//  TSCfgStream.c
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgStream.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//Head

STNBStructMapsRec TSCfgStreamDevice_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgStreamDevice_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgStreamDevice_sharedStructMap);
	if(TSCfgStreamDevice_sharedStructMap.map == NULL){
		STTSCfgStreamDevice s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgStreamDevice);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, uid);
		NBStructMap_addStrPtrM(map, s, name);
		NBStructMap_addStrPtrM(map, s, server);
		NBStructMap_addIntM(map, s, port);
		NBStructMap_addBoolM(map, s, useSSL);
		NBStructMap_addStrPtrM(map, s, user);
		NBStructMap_addStrPtrM(map, s, pass);
		TSCfgStreamDevice_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgStreamDevice_sharedStructMap);
	return TSCfgStreamDevice_sharedStructMap.map;
}

//Stream

STNBStructMapsRec TSCfgStream_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgStream_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgStream_sharedStructMap);
	if(TSCfgStream_sharedStructMap.map == NULL){
		STTSCfgStream s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgStream);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, isDisabled);
		NBStructMap_addStructM(map, s, device, TSCfgStreamDevice_getSharedStructMap());
		NBStructMap_addPtrToArrayOfStructM(map, s, versions, versionsSz, ENNBStructMapSign_Unsigned, TSCfgStreamVersion_getSharedStructMap());
		TSCfgStream_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgStream_sharedStructMap);
	return TSCfgStream_sharedStructMap.map;
}

//StreamsGrp

STNBStructMapsRec TSCfgStreamsGrp_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgStreamsGrp_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgStreamsGrp_sharedStructMap);
	if(TSCfgStreamsGrp_sharedStructMap.map == NULL){
		STTSCfgStreamsGrp s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgStreamsGrp);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, isDisabled);
		NBStructMap_addStrPtrM(map, s, uid);		//unique-id
		NBStructMap_addStrPtrM(map, s, name);		//streamsGrp name
		NBStructMap_addPtrToArrayOfStructM(map, s, streams, streamsSz, ENNBStructMapSign_Unsigned, TSCfgStream_getSharedStructMap());
		TSCfgStreamsGrp_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgStreamsGrp_sharedStructMap);
	return TSCfgStreamsGrp_sharedStructMap.map;
}
