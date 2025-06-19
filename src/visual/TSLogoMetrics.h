//
//  AUSceneLogoMetrics.hpp
//  thinstream-test
//
//  Created by Marcos Ortega on 8/5/18.
//

#ifndef AUSceneLogoMetrics_hpp
#define AUSceneLogoMetrics_hpp

#include "AUEscenaContenedor.h"
#include "nb/2d/NBSize.h"
#include "nb/2d/NBRect.h"
#include "core/TSDefs.h"

//Full-logo

typedef enum ENLogoType_ {
	ENLogoType_Logotype = 0,
	ENLogoType_Isotype,
	ENLogoType_Count
} ENLogoType;

//Logo

typedef struct STTSLogoDesc2D_ {
	ENLogoType type;
	STNBSize size;
} STTSLogoDesc2D;

typedef struct STTSLogoPartPos_ {
	STNBSize	scale;
	STNBPoint	posAtLogo; //inside parent
} STTSLogoPartPos;

//Launchscreen

typedef struct STTSLaunchLogoSzVersion_ {
	const char*			path;
	float				pathScale;
} STTSLaunchLogoSzVersion;

typedef struct STNBLaunchCfgOrient_ {
	ENTSScreenOrient				orient;
	STNBPoint					relPos;
	STNBSize					relMaxSz;
	STTSLaunchLogoSzVersion*	imgSzVers;
	const UI32					imgSzVersLen;
} STNBLaunchCfgOrient;

typedef struct STNBLaunchCfgType_ {
	ENLogoType			type;
	STNBLaunchCfgOrient	orients[ENTSScreenOrient_Count];
} STNBLaunchCfgType;

typedef struct STTSLaunchLogo_ {
	ENTSScreenOrient		orient;
	STTSLaunchLogoSzVersion	szVersion;
	STNBPoint				scenePos;
	STNBSize				sceneSize;
} STTSLaunchLogo;

//

class DRLogoMetrics {
	public:
		//Logo descriptions
		static STTSLogoDesc2D	getLogoDesc2D(const ENLogoType type);
		static STTSLaunchLogo	getLaunchscreenLogoSzVersion(const NBCajaAABB screenBox, const ENLogoType type);
		static STTSLaunchLogo	getLogoSzVersionForSize(const STNBSize sizeMax, const ENLogoType type);
};

#endif /* AUSceneLogoMetrics_hpp */
