//
//  AUSceneBarTop.cpp
//  app-refranero
//
//  Created by Marcos Ortega on 14/03/14.
//  Copyright (c) 2014 NIBSA. All rights reserved.
//

#include "visual/TSVisualPch.h"
#include "visual/AUSceneBarStatus.h"
//
#include "nb/core/NBMngrStructMaps.h"

STNBStructMapsRec DRSceneBarStatus_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* DRSceneBarStatus_getSharedStructMap(void){
	NBMngrStructMaps_lock(&DRSceneBarStatus_sharedStructMap);
	if(DRSceneBarStatus_sharedStructMap.map == NULL){
		STTSSceneBarStatus s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSSceneBarStatus);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, title);
		NBStructMap_addStrPtrM(map, s, subtitle);
		NBStructMap_addIntM(map, s, leftIconId);
		NBStructMap_addIntM(map, s, rightIconMask);
		//Orientation
		NBStructMap_addUIntM(map, s, orientMask); //ENAppOrientationBit
		NBStructMap_addUIntM(map, s, orientPref); //ENAppOrientationBit
		//Help
		NBStructMap_addUIntM(map, s, helpMask); //ENVisualHelpBit
		//
		DRSceneBarStatus_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&DRSceneBarStatus_sharedStructMap);
	return DRSceneBarStatus_sharedStructMap.map;
}
