//
//  TSCfgTelemetry.c
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgTelemetry.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//Filesystem

STNBStructMapsRec TSCfgTelemetryFilesystem_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgTelemetryFilesystem_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgTelemetryFilesystem_sharedStructMap);
	if(TSCfgTelemetryFilesystem_sharedStructMap.map == NULL){
		STTSCfgTelemetryFilesystem s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgTelemetryFilesystem);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addEnumM(map, s, statsLevel, NBLogLevel_getSharedEnumMap());
		TSCfgTelemetryFilesystem_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgTelemetryFilesystem_sharedStructMap);
	return TSCfgTelemetryFilesystem_sharedStructMap.map;
}

//Streams

STNBStructMapsRec TSCfgTelemetryStreams_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgTelemetryStreams_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgTelemetryStreams_sharedStructMap);
	if(TSCfgTelemetryStreams_sharedStructMap.map == NULL){
		STTSCfgTelemetryStreams s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgTelemetryStreams);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addEnumM(map, s, statsLevel, NBLogLevel_getSharedEnumMap());
		NBStructMap_addBoolM(map, s, perPortDetails);
		NBStructMap_addBoolM(map, s, perStreamDetails);
		NBStructMap_addStructM(map, s, rtsp, NBRtspStatsCfg_getSharedStructMap());
		TSCfgTelemetryStreams_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgTelemetryStreams_sharedStructMap);
	return TSCfgTelemetryStreams_sharedStructMap.map;
}

//Thread

STNBStructMapsRec TSCfgTelemetryThread_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgTelemetryThread_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgTelemetryThread_sharedStructMap);
	if(TSCfgTelemetryThread_sharedStructMap.map == NULL){
		STTSCfgTelemetryThread s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgTelemetryThread);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, name);
		NBStructMap_addStrPtrM(map, s, firstKnownFunc);
		NBStructMap_addEnumM(map, s, statsLevel, NBLogLevel_getSharedEnumMap());
		NBStructMap_addBoolM(map, s, locksByMethod);
		TSCfgTelemetryThread_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgTelemetryThread_sharedStructMap);
	return TSCfgTelemetryThread_sharedStructMap.map;
}

//Process

STNBStructMapsRec TSCfgTelemetryProcess_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgTelemetryProcess_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgTelemetryProcess_sharedStructMap);
	if(TSCfgTelemetryProcess_sharedStructMap.map == NULL){
		STTSCfgTelemetryProcess s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgTelemetryProcess);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addEnumM(map, s, statsLevel, NBLogLevel_getSharedEnumMap());
		NBStructMap_addBoolM(map, s, locksByMethod);
		NBStructMap_addPtrToArrayOfStructM(map, s, threads, threadsSz, ENNBStructMapSign_Unsigned, TSCfgTelemetryThread_getSharedStructMap());
		TSCfgTelemetryProcess_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgTelemetryProcess_sharedStructMap);
	return TSCfgTelemetryProcess_sharedStructMap.map;
}

//Stats

STNBStructMapsRec TSCfgTelemetry_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgTelemetry_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgTelemetry_sharedStructMap);
	if(TSCfgTelemetry_sharedStructMap.map == NULL){
		STTSCfgTelemetry s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgTelemetry);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStructM(map, s, filesystem, TSCfgTelemetryFilesystem_getSharedStructMap());
		NBStructMap_addStructM(map, s, buffers, NBDataPtrsStatsCfg_getSharedStructMap());
		NBStructMap_addStructM(map, s, streams, TSCfgTelemetryStreams_getSharedStructMap());
		NBStructMap_addStructM(map, s, process, TSCfgTelemetryProcess_getSharedStructMap());
		TSCfgTelemetry_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgTelemetry_sharedStructMap);
	return TSCfgTelemetry_sharedStructMap.map;
}
