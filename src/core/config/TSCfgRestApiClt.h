//
//  TSCfgRestApiClt.h
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#ifndef TSCfgRestApiClt_h
#define TSCfgRestApiClt_h

#include "nb/core/NBStructMap.h"
#include "core/config/TSCfgOutServer.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//Tag

	typedef struct STTSCfgRestApiCltTag_ {
		char*		tag;	//tag name excluding '[', ']'.
		BOOL		isJson;	//tag content must be scaped as JSON.
	} STTSCfgRestApiCltTag;

	const STNBStructMap* TSCfgRestApiCltTag_getSharedStructMap(void);

	//Template

	typedef struct STTSCfgRestApiCltTemplate_ {
		char*					name;
		char*					value;
		STTSCfgRestApiCltTag*	tags;	// tags to replace
		UI32					tagsSz;	// tags to replace
	} STTSCfgRestApiCltTemplate;

	const STNBStructMap* TSCfgRestApiCltTemplate_getSharedStructMap(void);

	//Rest Api config

	typedef struct STTSCfgRestApiClt_ {
		STTSCfgOutServer			server;			//server
		STTSCfgRestApiCltTemplate*	headers;		// headers
		UI32						headersSz;		// headers
		STTSCfgRestApiCltTemplate	body;			// body
	} STTSCfgRestApiClt;
	
	const STNBStructMap* TSCfgRestApiClt_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgRestApiClt_h */
