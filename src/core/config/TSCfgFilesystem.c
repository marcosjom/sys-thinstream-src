//
//  TSCfgFilesystem.c
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgFilesystem.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//streams params

STNBStructMapsRec TSCfgFilesystemStreamsParams_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgFilesystemStreamsParams_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgFilesystemStreamsParams_sharedStructMap);
	if(TSCfgFilesystemStreamsParams_sharedStructMap.map == NULL){
		STTSCfgFilesystemStreamsParams s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgFilesystemStreamsParams);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, limit);				//string version, like "123", "123B" (bytes), "123b" (bits), "123K", "123KB", "123M", "123MB" (Mega-Bytes), "123Mb" (Mega-bits), "123T", "123TB", "123Tb"
		NBStructMap_addUIntM(map, s, limitBytes);			//numeric version
		NBStructMap_addBoolM(map, s, isReadOnly);			//do not write nor delete files
		NBStructMap_addBoolM(map, s, keepUnknownFiles);		//do not remove files that are not part of the loaded tree
        NBStructMap_addBoolM(map, s, waitForLoad);          //if TRUE, then async load will block untill complete.
		NBStructMap_addUIntM(map, s, initialLoadThreads);	//amount of threads to use at initial load
		TSCfgFilesystemStreamsParams_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgFilesystemStreamsParams_sharedStructMap);
	return TSCfgFilesystemStreamsParams_sharedStructMap.map;
}

BOOL TSCfgFilesystemStreamsParams_cloneNormalized(STTSCfgFilesystemStreamsParams* obj, const STTSCfgFilesystemStreamsParams* src){
	BOOL r = FALSE;
	if(obj != NULL && src != NULL){
		const STNBStructMap* stMap = TSCfgFilesystemStreamsParams_getSharedStructMap();
		//
		r = TRUE;
		//clone
		if(obj != src){
			NBStruct_stRelease(stMap, obj, sizeof(*obj));
			NBStruct_stClone(stMap, src, sizeof(*src), obj, sizeof(*obj));
		}
		//Validate params
		if(!NBString_strIsEmpty(obj->limit)){
			const SI64 bytes = NBString_strToBytes(obj->limit);
			//PRINTF_INFO("TSServer, '%s' parsed to %lld bytes\n", fsStreams2.limit, bytes);
			if(bytes <= 0){
				PRINTF_CONSOLE_ERROR("TSServer, '%s' parsed to %lld bytes (value not valid).\n", obj->limit, bytes);
				r = FALSE;
			} else {
				obj->limitBytes = (UI64)bytes;
			}
		} else if(obj->limitBytes <= 0){
			PRINTF_CONSOLE_ERROR("TSServer, filesystem/streams/limitBytes (%lld bytes) not valid.\n", obj->limitBytes);
			r = FALSE;
		}
	}
	return r;
}

//streams

STNBStructMapsRec TSCfgFilesystemStreams_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgFilesystemStreams_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgFilesystemStreams_sharedStructMap);
	if(TSCfgFilesystemStreams_sharedStructMap.map == NULL){
		STTSCfgFilesystemStreams s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgFilesystemStreams);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, rootPath);
		NBStructMap_addStrPtrM(map, s, cfgFilename); //if defined, cfg-file at rootPath with a 'STTSCfgFilesystemStreamsParams' struct will be used instead of "defaults" values.
		NBStructMap_addStructM(map, s, defaults, TSCfgFilesystemStreamsParams_getSharedStructMap()); //defaults params
		TSCfgFilesystemStreams_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgFilesystemStreams_sharedStructMap);
	return TSCfgFilesystemStreams_sharedStructMap.map;
}

//filesystem

STNBStructMapsRec TSCfgFilesystem_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgFilesystem_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgFilesystem_sharedStructMap);
	if(TSCfgFilesystem_sharedStructMap.map == NULL){
		STTSCfgFilesystem s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgFilesystem);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, rootFolder);
		NBStructMap_addStructM(map, s, streams, TSCfgFilesystemStreams_getSharedStructMap());
		NBStructMap_addPtrToArrayOfStrPtrM(map, s, pkgs, pkgsSz, ENNBStructMapSign_Unsigned);
		TSCfgFilesystem_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgFilesystem_sharedStructMap);
	return TSCfgFilesystem_sharedStructMap.map;
}
