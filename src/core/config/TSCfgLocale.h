//
//  TSCfg.h
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#ifndef TSCfgLocale_h
#define TSCfgLocale_h

#include "nb/core/NBStructMap.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSCfgLocale_ {
		char**			paths;
		UI32			pathsSz;
	} STTSCfgLocale;
	
	const STNBStructMap* TSCfgLocale_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgLocale_h */
