//
//  TSCfgApplePushServices.h
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#ifndef TSCfgApplePushServices_h
#define TSCfgApplePushServices_h

#include "nb/core/NBStructMap.h"
#include "nb/net/NBHttpCfg.h" //for 'STNBHttpCertSrcCfg'

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSCfgApplePushServices_ {
		char*				server;	//api.sandbox.push.apple.com:443, api.push.apple.com:443
		SI32				port;
		BOOL				useSSL;
		char*				apnsTopic;
		STNBHttpCertSrcCfg	cert;
	} STTSCfgApplePushServices;
	
	const STNBStructMap* TSCfgApplePushServices_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgApplePushServices_h */
