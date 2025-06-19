//
//  TSIdentityReq.h
//  thinstream
//
//  Created by Marcos Ortega on 2/3/19.
//

#ifndef TSIdentityLocal_h
#define TSIdentityLocal_h

#include "nb/core/NBDate.h"
#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"
#include "nb/core/NBString.h"
#include "core/TSCypher.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//
	//Identity local (current user)
	//
	
	typedef struct STTSIdentityLocal_ {
		char*				identityId;
		STTSCypherDataKey	encKey;	//Local shared-enc-key
		UI32				infosShownMask;	//ENVisualHelpBit
		//Registration
		BYTE*				pass;			//Optional, if user enabled biometrics
		UI32				passSz;			//Optional, if user enabled biometrics
		BOOL				passIsUsrDef;	//Pass was defined by the system or the user
		char*				name;
		char*				countryIso2;
		char*				email;
		char*				phone;
		char*				imei;
		STNBDate			photoIdExpDate;
		SI32				photosIdCount;
	} STTSIdentityLocal;
	
	const STNBStructMap* TSIdentityLocal_getSharedStructMap(void);
		
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSIdentityLocal_h */

