//
//  DRStMailboxes.h
//  thinstream
//
//  Created by Marcos Ortega on 1/3/19.
//

#ifndef TSServerLogicClt_h
#define TSServerLogicClt_h

#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"
#include "nb/net/NBHttpRequest.h"
#include "core/config/TSCfgServer.h"
#include "core/TSContext.h"
#include "nb/files/NBFilesystem.h"
#include "core/server/TSServerStats.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef BOOL (*PtrAddMailToLocalQueueFunc)(void* param, const char* to, const char* subject, const char* body, const char* attachFilename, const char* attachMime, const void* attachData, const UI32 attachDataSz);

	typedef struct STTSServerLogicClt_ {
		void* opaque;
	} STTSServerLogicClt;
	
	void TSServerLogicClt_init(STTSServerLogicClt* obj);
	void TSServerLogicClt_release(STTSServerLogicClt* obj);
	
	BOOL TSServerLogicClt_loadFromConfig(STTSServerLogicClt* obj, const STTSCfgServer* cfg, STTSContext* context, STNBPKey* caKey, PtrAddMailToLocalQueueFunc funcSendMail, void* funcSendMailParam, STTSServerStats* stats, STNBHttpStatsRef channelsStats);
	BOOL TSServerLogicClt_prepare(STTSServerLogicClt* obj);
	void TSServerLogicClt_stopFlag(STTSServerLogicClt* obj);
	void TSServerLogicClt_stop(STTSServerLogicClt* obj);

#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* DRStMailboxes_h */
