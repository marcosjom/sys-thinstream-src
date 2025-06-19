//
//  DRStMailboxes.h
//  thinstream
//
//  Created by Marcos Ortega on 1/3/19.
//

#ifndef TSApplePushServicesClt_h
#define TSApplePushServicesClt_h

#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"
#include "core/config/TSCfgApplePushServices.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSApplePushServicesClt_ {
		void* opaque;
	} STTSApplePushServicesClt;
	
	void TSApplePushServicesClt_init(STTSApplePushServicesClt* obj);
	void TSApplePushServicesClt_release(STTSApplePushServicesClt* obj);
	
	BOOL TSApplePushServicesClt_loadFromConfig(STTSApplePushServicesClt* obj, const STTSCfgApplePushServices* cfg);
	BOOL TSApplePushServicesClt_prepare(STTSApplePushServicesClt* obj);
	void TSApplePushServicesClt_stopFlag(STTSApplePushServicesClt* obj);
	void TSApplePushServicesClt_stop(STTSApplePushServicesClt* obj);
	
	BOOL TSApplePushServicesClt_addNotification(STTSApplePushServicesClt* obj, const char* token, const char* text, const char* extraDataName, const char* extraDataValue);
	
#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* DRStMailboxes_h */
