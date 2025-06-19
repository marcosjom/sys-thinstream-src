//
//  TSClientChannel.h
//  thinstream
//
//  Created by Marcos Ortega on 8/24/18.
//

#ifndef TSClientChannel_h
#define TSClientChannel_h

#include "core/TSContext.h"
//
#include "nb/net/NBHttpMessage.h"
#include "core/TSRequestsMap.h"
#include "core/TSCypher.h"
#include "core/client/TSClientChannelRequest.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSClientChannelItf_ {
		UI64 (*addRequest)(void* obj, const ENTSRequestId reqId, const char** paramsPairsAndNull, const char* body, const UI32 bodySz, const UI64 secsWait, TSClientChannelReqCallbackFunc lstnr, void* lstnrParam);
		void (*removeRequestsByListenerParam)(void* obj, void* lstnrParam);
	} STTSClientChannelItf;
	
	typedef struct STTSClientChannel_ {
		void* opaque;
	} STTSClientChannel;
	
	void TSClientChannel_init(STTSClientChannel* obj);
	void TSClientChannel_release(STTSClientChannel* obj);
	
	const STNBHttpPortCfg* TSClientChannel_getCfg(STTSClientChannel* obj);
	
	BOOL TSClientChannel_prepare(STTSClientChannel* obj, void* clientOpq, STTSContext* context, STTSCypher* cypher, STNBSslContextRef defSslCtxt, const BOOL defSslCtxtIsAuthenticated, const STNBX509* caCert, const STTSRequestsMap* reqsMap, const STNBHttpPortCfg* cfg);
	BOOL TSClientChannel_sslContextSet(STTSClientChannel* obj, STNBSslContextRef sslCtxt, const BOOL isAuthenticated); //Connection will be reset
	BOOL TSClientChannel_sslContextTouch(STTSClientChannel* obj); //Force to reset the current connection
	UI64 TSClientChannel_addRequest(STTSClientChannel* obj, const UI64* optUniqueId, const ENTSRequestId reqId, const char* lang, const char** paramsPairsAndNull, const char* body, const UI32 bodySz, const UI64 secsWait, TSClientChannelReqCallbackFunc lstnr, void* lstnrParam);
	void TSClientChannel_removeRequestsByListenerParam(STTSClientChannel* obj, void* lstnrParam);
	void TSClientChannel_getRequestsResultsToNotifyAndRelease(STTSClientChannel* obj, STNBArray* dst);
	//
	void TSClientChannel_triggerTimedRequests(STTSClientChannel* obj);
	void TSClientChannel_disconnect(STTSClientChannel* obj, const UI64* filterReqUid);
	void TSClientChannel_startShutdown(STTSClientChannel* obj);
	void TSClientChannel_waitForAll(STTSClientChannel* obj);
	//Stats
	UI32 TSClientChannel_getStatsTryConnAccum(STTSClientChannel* obj, const BOOL resetAccum);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSClientChannel_h */
