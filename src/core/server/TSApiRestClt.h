//
//  DRStMailboxes.h
//  thinstream
//
//  Created by Marcos Ortega on 1/3/19.
//

#ifndef TSApiRestClt_h
#define TSApiRestClt_h

#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"
#include "nb/net/NBHttpRequest.h"
#include "core/config/TSCfgRestApiClt.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSApiRestClt_ {
		void* opaque;
	} STTSApiRestClt;
	
	void TSApiRestClt_init(STTSApiRestClt* obj);
	void TSApiRestClt_release(STTSApiRestClt* obj);
	
	BOOL TSApiRestClt_loadFromConfig(STTSApiRestClt* obj, const STTSCfgRestApiClt* cfg);
	BOOL TSApiRestClt_prepare(STTSApiRestClt* obj);
	void TSApiRestClt_stopFlag(STTSApiRestClt* obj);
	void TSApiRestClt_stop(STTSApiRestClt* obj);
	
	BOOL TSApiRestClt_addRequest(STTSApiRestClt* obj, const STNBHttpRequest* req);
	
#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* DRStMailboxes_h */
