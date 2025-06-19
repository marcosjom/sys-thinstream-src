//
//  AUGpMenuFinPartida.h
//  game-Refranero
//
//  Created by Marcos Ortega on 14/03/14.
//  Copyright (c) 2014 NIBSA. All rights reserved.
//

#ifndef AUSceneBarStatus_h
#define AUSceneBarStatus_h

typedef struct STTSSceneBarStatus_ {
	char*	title;
	char*	subtitle;
	SI32	leftIconId;			//ENSceneBarTopIcon
	SI32	rightIconMask;		//ENSceneBarIconBit
	//Orientation
	UI32	orientMask;			//ENAppOrientationBit
	UI32	orientPref;			//ENAppOrientationBit
	UI32	orientApplyOnce;	//ENAppOrientationBit (orientation to automatically apply when autorate is enabled)
	//Help
	UI32	helpMask;			//ENVisualHelpBit
} STTSSceneBarStatus;

const STNBStructMap* DRSceneBarStatus_getSharedStructMap(void);

#endif
