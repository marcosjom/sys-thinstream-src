//
//  TSCfgCypher.h
//  thinstream
//
//  Created by Marcos Ortega on 8/21/18.
//

#ifndef TSCfgCypher_h
#define TSCfgCypher_h

#include "nb/core/NBString.h"
#include "nb/core/NBStructMap.h"
#include "nb/core/NBJson.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSCfgCypherAes_ {
		char*		salt;
		UI32		iterations;
	} STTSCfgCypherAes;
	
	const STNBStructMap* TSCfgCypherAes_getSharedStructMap(void);
	
	typedef struct STTSCfgCypher_ {
		BOOL				enabled;
		UI8					passLen;
		SI32				keysCacheSz;
		SI32				jobThreads;
		STTSCfgCypherAes	aes;
	} STTSCfgCypher;
	
	const STNBStructMap* TSCfgCypher_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgCypher_h */
