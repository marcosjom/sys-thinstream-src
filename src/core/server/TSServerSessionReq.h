//
//  TSServer.h
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#ifndef TSServerSessionReq_h
#define TSServerSessionReq_h

#include "nb/core/NBMngrProcess.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/net/NBHttpServicePort.h"
#include "nb/net/NBHttpServiceConn.h"
#include "nb/net/NBWebSocket.h"
#include "core/logic/TSStreamsSessionMsg.h"	//for 'STTSStreamsServiceDesc'
#include "nb/net/NBHttpServiceRespRawLnk.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//Session
	
	typedef struct STTSServerSessionReq_ {
        void*                   obj;                //TSServerStreams*
		char*					uid;				//session unique id
        STNBHttpServiceRespRawLnk rawLnk;           //link
        STNBString              buff;
        STNBString              buff2;
		//net
		struct {
			STNBWebSocketRef	websocket;			//if "Upgrade: websocket"
		} net;
		//state
		struct {
			BOOL				stopFlag;			//stopFlag
			BOOL				isDisconnected;		//is currently at its won thread-connection
			UI32				secsDisconnected;	//secs the session was disconnected
            UI64                msgSeq;
            UI64                lastMsgTime;
		} state;
		//sent
		struct {
			UI64				timeLast;			//last message sent
			STTSStreamsServiceDesc	cur;			//stream state sent
			STTSStreamsServiceDesc	sent;			//stream state sent
		} sync;
		STNBThreadMutex			mutex;
		STNBThreadCond			cond;
	} STTSServerSessionReq;

	void TSServerSessionReq_init(STTSServerSessionReq* obj);
	void TSServerSessionReq_release(STTSServerSessionReq* obj);

#ifdef __cplusplus
} //extern "C"
#endif
	
#endif /* TSServerSessionReq_h */
