//
//  TSCfgMail.h
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#ifndef TSCfgMail_h
#define TSCfgMail_h

#include "nb/core/NBStructMap.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSCfgMail_ {
		BOOL		isMsExchange;
		char*		from;
		char*		server;
		SI32		port;
		BOOL		useSSL;
		char*		target;
		char*		user;
		char*		pass;
	} STTSCfgMail;
	
	const STNBStructMap* TSCfgMail_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgMail_h */
