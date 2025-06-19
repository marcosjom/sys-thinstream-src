//
//  TSDeviceLocal.c
//  thinstream
//
//  Created by Marcos Ortega on 5/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSDeviceLocal.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/2d/NBBitmap.h"
#include "nb/2d/NBPng.h"

//
//Device Meeting
//

STNBStructMapsRec TSDeviceLocalMeeting_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSDeviceLocalMeeting_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSDeviceLocalMeeting_sharedStructMap);
	if(TSDeviceLocalMeeting_sharedStructMap.map == NULL){
		STTSDeviceLocalMeeting s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSDeviceLocalMeeting);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, meetingId);
		NBStructMap_addUIntM(map, s, myVouchsAvailLast);
		NBStructMap_addUIntM(map, s, myVouchsAvailCur);
		TSDeviceLocalMeeting_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSDeviceLocalMeeting_sharedStructMap);
	return TSDeviceLocalMeeting_sharedStructMap.map;
}

//
//Device attest index
//

STNBStructMapsRec TSDeviceAttest_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSDeviceAttest_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSDeviceAttest_sharedStructMap);
	if(TSDeviceAttest_sharedStructMap.map == NULL){
		STTSDeviceAttest s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSDeviceAttest);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, meetingId);
		NBStructMap_addStrPtrM(map, s, filename);
		NBStructMap_addUIntM(map, s, verDatetime);
		TSDeviceAttest_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSDeviceAttest_sharedStructMap);
	return TSDeviceAttest_sharedStructMap.map;
}

//
//Device attest indexes
//

STNBStructMapsRec TSDeviceAttests_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSDeviceAttests_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSDeviceAttests_sharedStructMap);
	if(TSDeviceAttests_sharedStructMap.map == NULL){
		STTSDeviceAttests s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSDeviceAttests);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addPtrToArrayOfStructM(map, s, arr, arrSz, ENNBStructMapSign_Unsigned, TSDeviceAttest_getSharedStructMap());
		TSDeviceAttests_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSDeviceAttests_sharedStructMap);
	return TSDeviceAttests_sharedStructMap.map;
}

//
//Device
//

STNBStructMapsRec TSDeviceLocal_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSDeviceLocal_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSDeviceLocal_sharedStructMap);
	if(TSDeviceLocal_sharedStructMap.map == NULL){
		STTSDeviceLocal s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSDeviceLocal);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addBoolM(map, s, authContactRequested);
		NBStructMap_addBoolM(map, s, authNotifsRequested);
		NBStructMap_addPtrToArrayOfStructM(map, s, meets, meetsSz, ENNBStructMapSign_Unsigned, TSDeviceLocalMeeting_getSharedStructMap());
		//
		NBStructMap_addStructM(map, s, attests, TSDeviceAttests_getSharedStructMap()); //Local attestations indexes
		TSDeviceLocal_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSDeviceLocal_sharedStructMap);
	return TSDeviceLocal_sharedStructMap.map;
}


