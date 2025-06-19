//
//  TSCfgRestApiClt.c
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgRestApiClt.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

// Tag

STNBStructMapsRec TSCfgRestApiCltTag_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgRestApiCltTag_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgRestApiCltTag_sharedStructMap);
	if(TSCfgRestApiCltTag_sharedStructMap.map == NULL){
		STTSCfgRestApiCltTag s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgRestApiCltTag);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, tag);
		NBStructMap_addBoolM(map, s, isJson);
		TSCfgRestApiCltTag_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgRestApiCltTag_sharedStructMap);
	return TSCfgRestApiCltTag_sharedStructMap.map;
}

// Template

STNBStructMapsRec TSCfgRestApiCltTemplate_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgRestApiCltTemplate_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgRestApiCltTemplate_sharedStructMap);
	if(TSCfgRestApiCltTemplate_sharedStructMap.map == NULL){
		STTSCfgRestApiCltTemplate s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgRestApiCltTemplate);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, name);
		NBStructMap_addStrPtrM(map, s, value);
		NBStructMap_addPtrToArrayOfStructM(map, s, tags, tagsSz, ENNBStructMapSign_Unsigned, TSCfgRestApiCltTag_getSharedStructMap());
		TSCfgRestApiCltTemplate_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgRestApiCltTemplate_sharedStructMap);
	return TSCfgRestApiCltTemplate_sharedStructMap.map;
}

//Rest Api config

STNBStructMapsRec TSCfgRestApiClt_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgRestApiClt_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgRestApiClt_sharedStructMap);
	if(TSCfgRestApiClt_sharedStructMap.map == NULL){
		STTSCfgRestApiClt s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgRestApiClt);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStructM(map, s, server, TSCfgOutServer_getSharedStructMap());
		NBStructMap_addPtrToArrayOfStructM(map, s, headers, headersSz, ENNBStructMapSign_Unsigned, TSCfgRestApiCltTemplate_getSharedStructMap());
		NBStructMap_addStructM(map, s, body, TSCfgRestApiCltTemplate_getSharedStructMap());
		TSCfgRestApiClt_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgRestApiClt_sharedStructMap);
	return TSCfgRestApiClt_sharedStructMap.map;
}
