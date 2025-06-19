//
//  TSCfg.h
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#ifndef TSCfgContext_h
#define TSCfgContext_h

#include "nb/core/NBStructMap.h"
#include "nb/files/NBFilesystem.h"
#include "nb/net/NBHttpServicePort.h"
#include "nb/crypto/NBX509Req.h"
#include "core/config/TSCfgThreads.h"
#include "core/config/TSCfgFilesystem.h"
#include "core/config/TSCfgCypher.h"
#include "core/config/TSCfgMail.h"
#include "core/config/TSCfgRestApiClt.h"
#include "core/config/TSCfgApplePushServices.h"
#include "core/config/TSCfgFirebaseCM.h"
#include "core/config/TSCfgOutServer.h"
#include "core/config/TSCfgLocale.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSCfgContext_ {
		STTSCfgThreads			threads;
		STTSCfgFilesystem		filesystem;
		STTSCfgCypher			cypher;
		STTSCfgMail				smtp;
		STTSCfgRestApiClt		sms;
		STTSCfgApplePushServices applePushServices;
		STTSCfgFirebaseCM		firebaseCM;
		STTSCfgLocale			locale;
		STNBHttpCertSrcCfg		deviceId;
		STNBHttpCertSrcCfg		identityId;
		STNBHttpCertSrcCfg		usrCert;
		STNBHttpCertSrcCfg		caCert;
	} STTSCfgContext;
	
	const STNBStructMap* TSCfgContext_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgContext_h */
