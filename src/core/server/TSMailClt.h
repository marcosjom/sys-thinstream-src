//
//  DRStMailboxes.h
//  thinstream
//
//  Created by Marcos Ortega on 1/3/19.
//

#ifndef TSMailClt_h
#define TSMailClt_h

#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"
#include "nb/net/NBHttpRequest.h"
#include "core/config/TSCfgMail.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSMailClt_ {
		void* opaque;
	} STTSMailClt;
	
	void TSMailClt_init(STTSMailClt* obj);
	void TSMailClt_release(STTSMailClt* obj);
	
	BOOL TSMailClt_loadFromConfig(STTSMailClt* obj, const STTSCfgMail* cfg);
	BOOL TSMailClt_prepare(STTSMailClt* obj);
	void TSMailClt_stopFlag(STTSMailClt* obj);
	void TSMailClt_stop(STTSMailClt* obj);
	
	BOOL TSMailClt_addRequest(STTSMailClt* obj, const char* to, const char* subject, const char* body, const char* attachFilename, const char* attachMime, const void* attachData, const UI32 attachDataSz);
	
#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* DRStMailboxes_h */
