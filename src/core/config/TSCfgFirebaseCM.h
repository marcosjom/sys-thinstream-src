//
//  TSCfgFirebaseCM.h
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#ifndef TSCfgFirebaseCM_h
#define TSCfgFirebaseCM_h

#include "nb/core/NBStructMap.h"
#include "nb/net/NBHttpCfg.h"	//for 'STNBHttpCertSrcCfg'

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSCfgFirebaseCM_ {
		char*				server;	//api.sandbox.push.apple.com:443, api.push.apple.com:443
		SI32				port;
		BOOL				useSSL;
		char*				key;
		STNBHttpCertSrcCfg	cert;
	} STTSCfgFirebaseCM;
	
	const STNBStructMap* TSCfgFirebaseCM_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgFirebaseCM_h */
