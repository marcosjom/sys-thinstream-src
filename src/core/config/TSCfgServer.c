//
//  TSCfgServers.c
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfgServer.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//Server defaults streams

STNBStructMapsRec TSCfgServerDefaultsStreams_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgServerDefaultsStreams_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgServerDefaultsStreams_sharedStructMap);
	if(TSCfgServerDefaultsStreams_sharedStructMap.map == NULL){
		STTSCfgServerDefaultsStreams s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgServerDefaultsStreams);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStructM(map, s, params, TSStreamParams_getSharedStructMap());
        NBStructMap_addStructM(map, s, merger, TSStreamsMergerParams_getSharedStructMap());
		TSCfgServerDefaultsStreams_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgServerDefaultsStreams_sharedStructMap);
	return TSCfgServerDefaultsStreams_sharedStructMap.map;
}

//Server defaults

STNBStructMapsRec TSCfgServerDefaults_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgServerDefaults_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgServerDefaults_sharedStructMap);
	if(TSCfgServerDefaults_sharedStructMap.map == NULL){
		STTSCfgServerDefaults s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgServerDefaults);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStructM(map, s, streams, TSCfgServerDefaultsStreams_getSharedStructMap());
		TSCfgServerDefaults_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgServerDefaults_sharedStructMap);
	return TSCfgServerDefaults_sharedStructMap.map;
}

//Server sync

STNBStructMapsRec STTSCfgServerSync_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgServerSync_getSharedStructMap(void){
	NBMngrStructMaps_lock(&STTSCfgServerSync_sharedStructMap);
	if(STTSCfgServerSync_sharedStructMap.map == NULL){
		STTSCfgServerSync s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgServerSync);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addUIntM(map, s, secsToFirstAction);
		NBStructMap_addUIntM(map, s, secsRetryAfterError);
		NBStructMap_addUIntM(map, s, secsBetweenAction);
		NBStructMap_addStructM(map, s, server, TSCfgOutServer_getSharedStructMap());
		NBStructMap_addStrPtrM(map, s, authToken);
		STTSCfgServerSync_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&STTSCfgServerSync_sharedStructMap);
	return STTSCfgServerSync_sharedStructMap.map;
}

//Server system reports

STNBStructMapsRec STTSCfgServerSystemReports_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgServerSystemReports_getSharedStructMap(void){
	NBMngrStructMaps_lock(&STTSCfgServerSystemReports_sharedStructMap);
	if(STTSCfgServerSystemReports_sharedStructMap.map == NULL){
		STTSCfgServerSystemReports s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgServerSystemReports);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addUIntM(map, s, secsToSave);
		NBStructMap_addUIntM(map, s, minsToNext);
		NBStructMap_addPtrToArrayOfStrPtrM(map, s, emails, emailsSz, ENNBStructMapSign_Unsigned);
		STTSCfgServerSystemReports_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&STTSCfgServerSystemReports_sharedStructMap);
	return STTSCfgServerSystemReports_sharedStructMap.map;
}

//Server

STNBStructMapsRec TSCfgServer_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfgServer_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfgServer_sharedStructMap);
	if(TSCfgServer_sharedStructMap.map == NULL){
		STTSCfgServer s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfgServer);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStructM(map, s, defaults, TSCfgServerDefaults_getSharedStructMap());
		NBStructMap_addStructM(map, s, telemetry, TSCfgTelemetry_getSharedStructMap());
		NBStructMap_addStructM(map, s, systemReports, TSCfgServerSystemReports_getSharedStructMap());
		NBStructMap_addStructM(map, s, syncClt, TSCfgServerSync_getSharedStructMap());
		NBStructMap_addStructM(map, s, buffers, NBDataPtrsPoolCfg_getSharedStructMap());	//pointers-pool (pre-allocated)
		NBStructMap_addStructM(map, s, rtsp, NBRtspClientCfg_getSharedStructMap());
		NBStructMap_addPtrToArrayOfStructM(map, s, streamsGrps, streamsGrpsSz, ENNBStructMapSign_Unsigned, TSCfgStreamsGrp_getSharedStructMap());
		TSCfgServer_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfgServer_sharedStructMap);
	return TSCfgServer_sharedStructMap.map;
}

