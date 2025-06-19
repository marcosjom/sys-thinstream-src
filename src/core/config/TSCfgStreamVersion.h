//
//  TSCfgStreamVersion.h
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#ifndef TSCfgStreamVersion_h
#define TSCfgStreamVersion_h

#include "nb/core/NBStructMap.h"
#include "core/logic/TSStreamDefs.h"

#ifdef __cplusplus
extern "C" {
#endif

	//

	typedef struct STTSCfgStreamVersion_ {
		BOOL				isDisabled;	//
		char*				sid;		//subid (unnique on the parent stream)
		char*				uri;		//
		STTSStreamParams*	params;		//null means defaults.
	} STTSCfgStreamVersion;
	
	const STNBStructMap* TSCfgStreamVersion_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgStreamVersion_h */
