//
//  DRStMailboxes.h
//  thinstream
//
//  Created by Marcos Ortega on 1/3/19.
//

#ifndef TSFirebaseCMClt_h
#define TSFirebaseCMClt_h

#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"
#include "core/config/TSCfgFirebaseCM.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSFirebaseCMClt_ {
		void* opaque;
	} STTSFirebaseCMClt;
	
	void TSFirebaseCMClt_init(STTSFirebaseCMClt* obj);
	void TSFirebaseCMClt_release(STTSFirebaseCMClt* obj);
	
	BOOL TSFirebaseCMClt_loadFromConfig(STTSFirebaseCMClt* obj, const STTSCfgFirebaseCM* cfg);
	BOOL TSFirebaseCMClt_prepare(STTSFirebaseCMClt* obj);
	void TSFirebaseCMClt_stopFlag(STTSFirebaseCMClt* obj);
	void TSFirebaseCMClt_stop(STTSFirebaseCMClt* obj);
	
	BOOL TSFirebaseCMClt_addNotification(STTSFirebaseCMClt* obj, const char* token, const char* text, const char* extraDataName, const char* extraDataValue);
	
#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* DRStMailboxes_h */
