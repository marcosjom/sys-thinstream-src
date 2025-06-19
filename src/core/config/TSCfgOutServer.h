//
//  TSCfgOutServer.h
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#ifndef TSCfgOutServer_h
#define TSCfgOutServer_h

#include "nb/core/NBStructMap.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//Server
	
	typedef struct STTSCfgOutServer_ {
		char*		server;
		SI32		port;
		BOOL		useSSL;
		const char* reqId;
	} STTSCfgOutServer;
	
	const STNBStructMap* TSCfgOutServer_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgOutServer_h */
