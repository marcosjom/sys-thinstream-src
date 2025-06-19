//
//  TSStreamsSessionMsg.c
//  thinstream
//
//  Created by Marcos Ortega on 2/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSStreamsSessionMsg.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBMngrStructMaps.h"

// stream state

/*STNBEnumMapRecord TSStreamState_sharedEnumMapRecs[] = {
	{ ENTSStreamState_Stopped, "ENTSStreamState_Stopped", "stopped" }
	, { ENTSStreamState_Paused, "ENTSStreamState_Paused", "paused" }
	, { ENTSStreamState_Playing, "ENTSStreamState_Playing", "playing" }
};

STNBEnumMap TSStreamState_sharedEnumMap = {
	"ENTSStreamState"
	, TSStreamState_sharedEnumMapRecs
	, (sizeof(TSStreamState_sharedEnumMapRecs) / sizeof(TSStreamState_sharedEnumMapRecs[0]))
};

const STNBEnumMap* TSStreamState_getSharedEnumMap(void){
	return &TSStreamState_sharedEnumMap;
}*/

// stream version src desc

STNBStructMapsRec TSStreamVerSrcDesc_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamVerSrcDesc_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamVerSrcDesc_sharedStructMap);
	if(TSStreamVerSrcDesc_sharedStructMap.map == NULL){
		STTSStreamVerSrcDesc s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamVerSrcDesc);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, isOnline);
		//NBStructMap_addStructM(map, s, props, NBAvcPicDescBase_getSharedStructMap());
		//
		TSStreamVerSrcDesc_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamVerSrcDesc_sharedStructMap);
	return TSStreamVerSrcDesc_sharedStructMap.map;
}

// stream version desc

STNBStructMapsRec TSStreamVerDesc_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamVerDesc_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamVerDesc_sharedStructMap);
	if(TSStreamVerDesc_sharedStructMap.map == NULL){
		STTSStreamVerDesc s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamVerDesc);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, syncFnd);
		NBStructMap_addUIntM(map, s, iSeq);
		NBStructMap_addStrPtrM(map, s, uid);
		NBStructMap_addStrPtrM(map, s, name);
		NBStructMap_addStructM(map, s, live, TSStreamVerSrcDesc_getSharedStructMap());
		NBStructMap_addStructM(map, s, storage, TSStreamVerSrcDesc_getSharedStructMap());
		//
		TSStreamVerDesc_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamVerDesc_sharedStructMap);
	return TSStreamVerDesc_sharedStructMap.map;
}

// stream desc

STNBStructMapsRec TSStreamDesc_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamDesc_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamDesc_sharedStructMap);
	if(TSStreamDesc_sharedStructMap.map == NULL){
		STTSStreamDesc s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamDesc);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, syncFnd);
		NBStructMap_addUIntM(map, s, iSeq);
		NBStructMap_addStrPtrM(map, s, uid);
		NBStructMap_addStrPtrM(map, s, name);
		NBStructMap_addPtrToArrayOfStructM(map, s, versions, versionsSz, ENNBStructMapSign_Unsigned, TSStreamVerDesc_getSharedStructMap());
		//NBStructMap_addEnumM(map, s, state, TSStreamState_getSharedEnumMap());
		//
		TSStreamDesc_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamDesc_sharedStructMap);
	return TSStreamDesc_sharedStructMap.map;
}

// streams grp

STNBStructMapsRec TSStreamsGrpDesc_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsGrpDesc_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamsGrpDesc_sharedStructMap);
	if(TSStreamsGrpDesc_sharedStructMap.map == NULL){
		STTSStreamsGrpDesc s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsGrpDesc);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, syncFnd);
		NBStructMap_addUIntM(map, s, iSeq);
		NBStructMap_addStrPtrM(map, s, uid);
		NBStructMap_addStrPtrM(map, s, name);
		NBStructMap_addPtrToArrayOfStructM(map, s, streams, streamsSz, ENNBStructMapSign_Unsigned, TSStreamDesc_getSharedStructMap());
		//
		TSStreamsGrpDesc_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamsGrpDesc_sharedStructMap);
	return TSStreamsGrpDesc_sharedStructMap.map;
}

// streams grps

STNBStructMapsRec TSStreamsServiceDesc_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsServiceDesc_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamsServiceDesc_sharedStructMap);
	if(TSStreamsServiceDesc_sharedStructMap.map == NULL){
		STTSStreamsServiceDesc s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsServiceDesc);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, syncFnd);
		NBStructMap_addUIntM(map, s, iSeq);
		NBStructMap_addStrPtrM(map, s, runId);
		NBStructMap_addPtrToArrayOfStructM(map, s, streamsGrps, streamsGrpsSz, ENNBStructMapSign_Unsigned, TSStreamsGrpDesc_getSharedStructMap());
		NBStructMap_addPtrToArrayOfStructM(map, s, subServices, subServicesSz, ENNBStructMapSign_Unsigned, map);
		//
		TSStreamsServiceDesc_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamsServiceDesc_sharedStructMap);
	return TSStreamsServiceDesc_sharedStructMap.map;
}

// stream update action

STNBEnumMapRecord TSStreamUpdAction_sharedEnumMapRecs[] = {
	{ ENTSStreamUpdAction_New, "ENTSStreamUpdAction_New", "new" }
	, { ENTSStreamUpdAction_Changed, "ENTSStreamUpdAction_Changed", "upd" }
	, { ENTSStreamUpdAction_Removed, "ENTSStreamUpdAction_Removed", "rmv" }
};

STNBEnumMap TSStreamUpdAction_sharedEnumMap = {
	"ENTSStreamUpdAction"
	, TSStreamUpdAction_sharedEnumMapRecs
	, (sizeof(TSStreamUpdAction_sharedEnumMapRecs) / sizeof(TSStreamUpdAction_sharedEnumMapRecs[0]))
};

const STNBEnumMap* TSStreamUpdAction_getSharedEnumMap(void){
	return &TSStreamUpdAction_sharedEnumMap;
}

// stream update

STNBStructMapsRec TSStreamUpd_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamUpd_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamUpd_sharedStructMap);
	if(TSStreamUpd_sharedStructMap.map == NULL){
		STTSStreamUpd s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamUpd);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addEnumM(map, s, action, TSStreamUpdAction_getSharedEnumMap());
		NBStructMap_addStructM(map, s, desc, TSStreamDesc_getSharedStructMap());
		//
		TSStreamUpd_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamUpd_sharedStructMap);
	return TSStreamUpd_sharedStructMap.map;
}

// streamsGrp update

STNBStructMapsRec TSStreamsGrpUpd_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsGrpUpd_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamsGrpUpd_sharedStructMap);
	if(TSStreamsGrpUpd_sharedStructMap.map == NULL){
		STTSStreamsGrpUpd s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsGrpUpd);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, uid);
		NBStructMap_addStrPtrM(map, s, name);
		NBStructMap_addPtrToArrayOfStructM(map, s, streams, streamsSz, ENNBStructMapSign_Unsigned, TSStreamUpd_getSharedStructMap());
		//
		TSStreamsGrpUpd_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamsGrpUpd_sharedStructMap);
	return TSStreamsGrpUpd_sharedStructMap.map;
}

// streams grps update

STNBStructMapsRec TSStreamsServiceUpd_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsServiceUpd_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamsServiceUpd_sharedStructMap);
	if(TSStreamsServiceUpd_sharedStructMap.map == NULL){
		STTSStreamsServiceUpd s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsServiceUpd);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, runId);
		NBStructMap_addPtrToArrayOfStructM(map, s, grps, grpsSz, ENNBStructMapSign_Unsigned, TSStreamsGrpUpd_getSharedStructMap());
		NBStructMap_addPtrToArrayOfStructM(map, s, subServices, subServicesSz, ENNBStructMapSign_Unsigned, map);
		//
		TSStreamsServiceUpd_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamsServiceUpd_sharedStructMap);
	return TSStreamsServiceUpd_sharedStructMap.map;
}

//session stream msg

STNBStructMapsRec TSStreamsSessionMsg_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSStreamsSessionMsg_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSStreamsSessionMsg_sharedStructMap);
	if(TSStreamsSessionMsg_sharedStructMap.map == NULL){
		STTSStreamsSessionMsg s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSStreamsSessionMsg);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, sessionId);	//only sent in first message
		NBStructMap_addStrPtrM(map, s, runId);	//server execution unique identifier
		NBStructMap_addUIntM(map, s, time);		//sender-side timestamp
		NBStructMap_addUIntM(map, s, iSeq);		//sequence of this message
		NBStructMap_addStructPtrM(map, s, streamsUpds, TSStreamsServiceUpd_getSharedStructMap());
		//
		TSStreamsSessionMsg_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSStreamsSessionMsg_sharedStructMap);
	return TSStreamsSessionMsg_sharedStructMap.map;
}
