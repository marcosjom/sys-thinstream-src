//
//  TSStreamDefs.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSStreamDefs.h"
//
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"


//Read mode

STNBEnumMapRecord TSCfgStreamReadMode_sharedEnumMapRecs[] = {
	{ ENTSCfgStreamReadMode_Allways, "ENTSCfgStreamReadMode_Allways", "allways" }
	, { ENTSCfgStreamReadMode_WhenViewing, "ENTSCfgStreamReadMode_WhenViewing", "whenViewing" }
	, { ENTSCfgStreamReadMode_Never, "ENTSCfgStreamReadMode_Never", "never" }
};

STNBEnumMap TSCfgStreamReadMode_sharedEnumMap = {
	"ENSCfgStreamVersionReadMode"
	, TSCfgStreamReadMode_sharedEnumMapRecs
	, (sizeof(TSCfgStreamReadMode_sharedEnumMapRecs) / sizeof(TSCfgStreamReadMode_sharedEnumMapRecs[0]))
};

const STNBEnumMap* TSCfgStreamReadMode_getSharedEnumMap(void){
	return &TSCfgStreamReadMode_sharedEnumMap;
}

//Read source

STNBEnumMapRecord TSCfgStreamReadSource_sharedEnumMapRecs[] = {
	{ ENTSCfgStreamReadSource_MemoryBuffer, "ENTSCfgStreamReadSource_MemoryBuffer", "memoryBuffer" }
	, { ENTSCfgStreamReadSource_StorageBuffer, "ENTSCfgStreamReadSource_StorageBuffer", "storageBuffer" }
};

STNBEnumMap TSCfgStreamReadSource_sharedEnumMap = {
	"ENSCfgStreamVersionReadSource"
	, TSCfgStreamReadSource_sharedEnumMapRecs
	, (sizeof(TSCfgStreamReadSource_sharedEnumMapRecs) / sizeof(TSCfgStreamReadSource_sharedEnumMapRecs[0]))
};

const STNBEnumMap* TSCfgStreamReadSource_getSharedEnumMap(void){
	return &TSCfgStreamReadSource_sharedEnumMap;
}

//Write mode

STNBEnumMapRecord TSCfgStreamWriteMode_sharedEnumMapRecs[] = {
	{ ENTSCfgStreamWriteMode_Allways, "ENTSCfgStreamWriteMode_Allways", "allways" }
	, { ENTSCfgStreamWriteMode_MotionDetected, "ENTSCfgStreamWriteMode_MotionDetected", "motionDetected" }	
	, { ENTSCfgStreamWriteMode_Never, "ENTSCfgStreamWriteMode_Never", "never" }
};

STNBEnumMap TSCfgStreamWriteMode_sharedEnumMap = {
	"ENSCfgStreamVersionWriteMode"
	, TSCfgStreamWriteMode_sharedEnumMapRecs
	, (sizeof(TSCfgStreamWriteMode_sharedEnumMapRecs) / sizeof(TSCfgStreamWriteMode_sharedEnumMapRecs[0]))
};

const STNBEnumMap* TSCfgStreamWriteMode_getSharedEnumMap(void){
	return &TSCfgStreamWriteMode_sharedEnumMap;
}

//Stream params buffer units

STNBStructMapsRec TSStreamParamsBufferUnits_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParamsBufferUnits_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParamsBufferUnits_sharedStructMap);
	if(TSStreamParamsBufferUnits_sharedStructMap.map == NULL){
		STTSStreamParamsBufferUnits s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParamsBufferUnits);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addUIntM(map, s, minAlloc); //amount of threads to allocate at start
		NBStructMap_addUIntM(map, s, maxKeep); //amount of threads to keep max
		TSStreamParamsBufferUnits_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParamsBufferUnits_sharedStructMap);
	return TSStreamParamsBufferUnits_sharedStructMap.map;
}

//Stream params buffers

STNBStructMapsRec TSStreamParamsBuffers_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParamsBuffers_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParamsBuffers_sharedStructMap);
	if(TSStreamParamsBuffers_sharedStructMap.map == NULL){
		STTSStreamParamsBuffers s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParamsBuffers);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStructM(map, s, units, TSStreamParamsBufferUnits_getSharedStructMap()); //NAL-units in H.264
		TSStreamParamsBuffers_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParamsBuffers_sharedStructMap);
	return TSStreamParamsBuffers_sharedStructMap.map;
}

//Stream params RTP

STNBStructMapsRec TSStreamParamsRtp_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParamsRtp_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParamsRtp_sharedStructMap);
	if(TSStreamParamsRtp_sharedStructMap.map == NULL){
		STTSStreamParamsRtp s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParamsRtp);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, assumeConnDroppedAfter); //time to asumme the UTP connection was dropped
		NBStructMap_addUIntM(map, s, assumeConnDroppedAfterSecs); //time to asumme the UTP connection was dropped
		TSStreamParamsRtp_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParamsRtp_sharedStructMap);
	return TSStreamParamsRtp_sharedStructMap.map;
}

//Stream params RTSP ports

STNBStructMapsRec TSStreamParamsRtspPorts_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParamsRtspPorts_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParamsRtspPorts_sharedStructMap);
	if(TSStreamParamsRtspPorts_sharedStructMap.map == NULL){
		STTSStreamParamsRtspPorts s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParamsRtspPorts);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addUIntM(map, s, rtp);
		NBStructMap_addUIntM(map, s, rtcp);
		TSStreamParamsRtspPorts_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParamsRtspPorts_sharedStructMap);
	return TSStreamParamsRtspPorts_sharedStructMap.map;
}

//Stream params RTSP

STNBStructMapsRec TSStreamParamsRtsp_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParamsRtsp_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParamsRtsp_sharedStructMap);
	if(TSStreamParamsRtsp_sharedStructMap.map == NULL){
		STTSStreamParamsRtsp s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParamsRtsp);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStructM(map, s, ports, TSStreamParamsRtspPorts_getSharedStructMap());
		TSStreamParamsRtsp_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParamsRtsp_sharedStructMap);
	return TSStreamParamsRtsp_sharedStructMap.map;
}

//Stream params read

STNBStructMapsRec TSStreamParamsRead_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParamsRead_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParamsRead_sharedStructMap);
	if(TSStreamParamsRead_sharedStructMap.map == NULL){
		STTSStreamParamsRead s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParamsRead);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addEnumM(map, s, mode, TSCfgStreamReadMode_getSharedEnumMap());		//defines when packages will be ignored or processed
		NBStructMap_addEnumM(map, s, source, TSCfgStreamReadSource_getSharedEnumMap());	//defines the source of the packets
		TSStreamParamsRead_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParamsRead_sharedStructMap);
	return TSStreamParamsRead_sharedStructMap.map;
}

//Stream params file debug write

STNBStructMapsRec TSStreamParamsFileDebugWrite_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParamsFileDebugWrite_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParamsFileDebugWrite_sharedStructMap);
	if(TSStreamParamsFileDebugWrite_sharedStructMap.map == NULL){
		STTSStreamParamsFileDebugWrite s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParamsFileDebugWrite);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, simulateOnly);
		NBStructMap_addBoolM(map, s, atomicFlush);
		TSStreamParamsFileDebugWrite_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParamsFileDebugWrite_sharedStructMap);
	return TSStreamParamsFileDebugWrite_sharedStructMap.map;
}

//Stream params file debug

STNBStructMapsRec TSStreamParamsFileDebug_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParamsFileDebug_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParamsFileDebug_sharedStructMap);
	if(TSStreamParamsFileDebug_sharedStructMap.map == NULL){
		STTSStreamParamsFileDebug s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParamsFileDebug);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStructM(map, s, write, TSStreamParamsFileDebugWrite_getSharedStructMap());
		TSStreamParamsFileDebug_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParamsFileDebug_sharedStructMap);
	return TSStreamParamsFileDebug_sharedStructMap.map;
}

//Stream params file index

STNBStructMapsRec TSStreamParamsFileIndex_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParamsFileIndex_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParamsFileIndex_sharedStructMap);
	if(TSStreamParamsFileIndex_sharedStructMap.map == NULL){
		STTSStreamParamsFileIndex s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParamsFileIndex);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, allNALU);
		TSStreamParamsFileIndex_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParamsFileIndex_sharedStructMap);
	return TSStreamParamsFileIndex_sharedStructMap.map;
}

//Stream params file payload

STNBStructMapsRec TSStreamParamsFilePayload_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParamsFilePayload_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParamsFilePayload_sharedStructMap);
	if(TSStreamParamsFilePayload_sharedStructMap.map == NULL){
		STTSStreamParamsFilePayload s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParamsFilePayload);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, limit);			//per-file, string version, like "123", "123B" (bytes), "123b" (bits), "123K", "123KB", "123M", "123MB" (Mega-Bytes), "123Mb" (Mega-bits), "123T", "123TB", "123Tb"
		NBStructMap_addUIntM(map, s, limitBytes);		//per-file, numeric version
		NBStructMap_addStrPtrM(map, s, divider);		//per-file, string version, like "60s", "60secs", "90m", "90min", "90min", "1d", "1day"
		NBStructMap_addUIntM(map, s, dividerSecs);		//per-file, numeric version
		TSStreamParamsFilePayload_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParamsFilePayload_sharedStructMap);
	return TSStreamParamsFilePayload_sharedStructMap.map;
}

//Stream params files

STNBStructMapsRec TSStreamParamsFiles_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParamsFiles_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParamsFiles_sharedStructMap);
	if(TSStreamParamsFiles_sharedStructMap.map == NULL){
		STTSStreamParamsFiles s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParamsFiles);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addUIntM(map, s, nameDigits);													//per-file
		NBStructMap_addBoolM(map, s, buffKeyframes);												//accum in ram units grouped by keyframes; when FALSE units are written as they arrive.
		NBStructMap_addStructM(map, s, index, TSStreamParamsFileIndex_getSharedStructMap());		//index-file params
		NBStructMap_addStructM(map, s, payload, TSStreamParamsFilePayload_getSharedStructMap());	//payload-file params
		NBStructMap_addStructM(map, s, debug, TSStreamParamsFileDebug_getSharedStructMap());		//debug params
		TSStreamParamsFiles_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParamsFiles_sharedStructMap);
	return TSStreamParamsFiles_sharedStructMap.map;
}




//Stream params write

STNBStructMapsRec TSStreamParamsWrite_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParamsWrite_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParamsWrite_sharedStructMap);
	if(TSStreamParamsWrite_sharedStructMap.map == NULL){
		STTSStreamParamsWrite s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParamsWrite);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addEnumM(map, s, mode, TSCfgStreamWriteMode_getSharedEnumMap());	//defines when the packages will be written or ignored
		NBStructMap_addStructM(map, s, files, TSStreamParamsFiles_getSharedStructMap()); //files
		TSStreamParamsWrite_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParamsWrite_sharedStructMap);
	return TSStreamParamsWrite_sharedStructMap.map;
}

//Stream params

STNBStructMapsRec TSStreamParams_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamParams_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamParams_sharedStructMap);
	if(TSStreamParams_sharedStructMap.map == NULL){
		STTSStreamParams s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamParams);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStructM(map, s, read, TSStreamParamsRead_getSharedStructMap());		//read params
		NBStructMap_addStructM(map, s, write, TSStreamParamsWrite_getSharedStructMap());	//write params
		NBStructMap_addStructM(map, s, buffers, TSStreamParamsBuffers_getSharedStructMap());	//buffers
		NBStructMap_addStructM(map, s, rtp, TSStreamParamsRtp_getSharedStructMap());		//udp stream params
		NBStructMap_addStructM(map, s, rtsp, TSStreamParamsRtsp_getSharedStructMap());		//rtsp stream params
		TSStreamParams_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamParams_sharedStructMap);
	return TSStreamParams_sharedStructMap.map;
}

//Stream merger params

STNBStructMapsRec TSStreamsMergerParams_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsMergerParams_getSharedStructMap(void){
    NBMngrStructMaps_lock(&TSStreamsMergerParams_sharedStructMap);
    if(TSStreamsMergerParams_sharedStructMap.map == NULL){
        STTSStreamsMergerParams s;
        STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsMergerParams);
        NBStructMap_init(map, sizeof(s));
        NBStructMap_addUIntPtrM(map, s, secsWaitAllFmtsMax); //max seconds to wait for all stream formats to be available, if NULL use default value in code should be used.
        TSStreamsMergerParams_sharedStructMap.map = map;
    }
    NBMngrStructMaps_unlock(&TSStreamsMergerParams_sharedStructMap);
    return TSStreamsMergerParams_sharedStructMap.map;
}

//Stream address

STNBStructMapsRec TSStreamAddress_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamAddress_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamAddress_sharedStructMap);
	if(TSStreamAddress_sharedStructMap.map == NULL){
		STTSStreamAddress s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamAddress);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, server);
		NBStructMap_addIntM(map, s, port);
		NBStructMap_addBoolM(map, s, useSSL);
		NBStructMap_addStrPtrM(map, s, uri);
		NBStructMap_addStrPtrM(map, s, user);
		NBStructMap_addStrPtrM(map, s, pass);
		TSStreamAddress_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamAddress_sharedStructMap);
	return TSStreamAddress_sharedStructMap.map;
}

BOOL TSStreamAddress_isEqual(const STTSStreamAddress* obj, const STTSStreamAddress* other){
	BOOL r = FALSE;
	if(obj == other){
		r = TRUE; //this includes both NULL
	} else if(obj != NULL && other != NULL){
		if(obj->port == other->port && obj->useSSL == other->useSSL){
			if(NBString_strIsEqual(obj->uri, other->uri)){
				if(NBString_strIsEqual(obj->server, other->server)){
					if(NBString_strIsEqual(obj->user, other->user)){
						if(NBString_strIsEqual(obj->pass, other->pass)){
							r = TRUE;
						}
					}
				}
			}
		}
	}
	return r;
}
