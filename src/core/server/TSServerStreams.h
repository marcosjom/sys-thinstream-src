//
//  TSServer.h
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#ifndef TSServerStreams_h
#define TSServerStreams_h

#include "nb/core/NBProcess.h"
#include "nb/core/NBArray.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/net/NBHttpServicePort.h"
#include "nb/net/NBHttpServiceConn.h"
#include "core/logic/TSClientRequester.h"
#include "core/logic/TSStreamVersionStorage.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	struct STTSServerStreams_;
	struct STTSServerStreamReq_;

	//Streams

	typedef struct STTSServerStreams_ {
		void*				opq;
		//state
		struct {
			BOOL			stopFlag;
			UI64			lastSec;
		} state;
		//sessions
		struct {
			UI64			iSeq;
			STNBArray		arr;	//STTSServerSessionReq*
		} sessions;
		//streams
		struct {
			UI64			iSeq;
			STNBArray		arr;	//STTSServerStreamReq*
		} viewers;
		STNBThreadMutex		mutex;
		STNBThreadCond		cond;
	} STTSServerStreams;
	
	//

	void TSServerStreams_init(STTSServerStreams* obj);
	void TSServerStreams_release(STTSServerStreams* obj);

	//cfg
	void TSServerStreams_setServerOpq(STTSServerStreams* obj, void* opq);

	//	
	void TSServerStreams_stopFlag(STTSServerStreams* obj);
	void TSServerStreams_stop(STTSServerStreams* obj);
	void TSServerStreams_tickSecChanged(STTSServerStreams* obj, const UI64 timestamp);

	//
	BOOL TSServerStreams_cltRequested(STTSServerStreams* obj, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk);

#ifdef __cplusplus
} //extern "C"
#endif
	
#endif /* TSServer_h */
