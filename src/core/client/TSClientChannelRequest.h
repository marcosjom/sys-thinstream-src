//
//  TSClientChannelRequest.h
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#ifndef TSClientChannelRequest_h
#define TSClientChannelRequest_h

#include "nb/core/NBString.h"
#include "nb/core/NBArray.h"
#include "nb/net/NBHttpMessage.h"
#include "core/TSRequestsMap.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	struct STTSClientChannelRequest_;
	
	typedef void (*TSClientChannelReqCallbackFunc)(const struct STTSClientChannelRequest_* req, void* lParam);
	
	typedef struct STTSClientChannelRequest_ {
		UI64							uid;
		UI64							notBefore;	//timestamp
		BOOL							active;
		ENTSRequestId					reqId;
		STNBArray						paramsIdx;	//UI32
		STNBString						paramsStrs;
		STNBString						body;
		TSClientChannelReqCallbackFunc	lstnr;
		void*							lstnrParam;
		STNBHttpMessage*				response;
	} STTSClientChannelRequest;
	
	void TSClientChannelRequest_release(STTSClientChannelRequest* req);
	
#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSClientChannelRequest_h */
