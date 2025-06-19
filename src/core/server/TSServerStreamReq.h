//
//  TSServer.h
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#ifndef TSServerStreamReq_h
#define TSServerStreamReq_h

#include "nb/core/NBProcess.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBRange.h"
#include "nb/core/NBArray.h"
#include "nb/core/NBDataCursor.h"
#include "nb/net/NBUrl.h"
#include "nb/net/NBHttpService.h"
#include "nb/net/NBWebSocket.h"
#include "nb/files/NBMp4.h"
//
#include "core/server/TSServerStreamPath.h"
#include "core/server/TSServerStreamReqConn.h"
#include "core/logic/TSStreamBuilder.h"

#ifdef __cplusplus
extern "C" {
#endif

//stream req

typedef struct STTSServerStreamReq_ {
	STNBThreadMutex			mutex;
	STNBThreadCond			cond;
	struct STTSServerStreams_* obj;         //parent
	//cfg
	struct {
		STNBUrl				uri;			//requested uri
		STNBArray			srcs;			//STTSServerStreamPath
		//params
		struct {
			UI32			secsPerStream;	//'&secsPerStream=#', if src refers to multiple streams, each stream will be presented this amount of seconds.
			UI32			skipFramesMax;	//'&skipFramesMax=#', amount of streams to skip;
											//this helps on systems that render slower than network traffic;
											//also helps to force real-time presentation.
            UI32            sameProfileOnly; //'&sameProfileOnly=1', in raw-streams, forces the stream to contain only frames that share the same coded profile.
			BOOL			mp4FullyFrag;	//'&mp4FullyFrag=1', fully fragmented (each 'moof' + 'mdat' will contain one frame)
			BOOL			mp4MultiFiles;	//'&mp4MultiFiles=1', if enabled a new file ('ftyp' + 'moov') will be started when switchin to a non-compatible stream.
											//					  if disabled (default), only compatible streams will be added to the mp4.
			BOOL			mp4RealTime;	//'&mp4RealTime=1', each frame duration will be calculated based on the previous frame (one-frame delay is produced)
		} params;
	} cfg;
	//net
	struct {
        STNBHttpServiceRespRawLnk rawLnk;
		STNBWebSocketRef	  	websocket;	//if connection was upgraded
		//send
		struct {
			STTSStreamBuilderRef queue;		//queue
		} send;
	} net;
	//mp4
	/*struct {
		BOOL				isCurFmt;	//if format is mp4.
	} mp4;*/
	//reqs
	struct {
		STNBArray			arr;	    //STTSServerStreamReqConn*
        //fmts (defining stream combined-format)
        /*struct {
            UI64            msWaitAllFmtsRemain;    //milliseconds remaining before defining stream best-fmt when some of the streams does not have fmt yet.
            BOOL            analyzedAtCurSec;       //merged-fmt analyzed at current second
            STNBAvcPicDescBase* mergedFmt;          //merged stream fmt, NULL if not defined yet.
        } fmt;*/
        //cur
		struct {
			STTSServerStreamReqConn* req;	    //currenty sending
			UI32			secsChanges;
			BOOL			changeFlagsApplied;			
		} cur;
	} reqs;
	//garbage
	struct {
		STNBArray			arr; //STTSStreamUnit
	} garbage;
	//state
	struct {
		BOOL 				stopFlag;
		BOOL				netErr;
		//time spent
		struct {
			STNBTimestampMicro	last;
		} tick;
		//curSec
		struct {
			UI32		units;
			UI32		frames;	//video-picture-related stream unit
		} curSec;
		//stream
		struct {
			//units (all units)
			struct {
				UI32 count;
				STNBTimestampMicro lastTime;
			} units;
			//frames (video-picture-related stream unit)
			struct {
				UI32 count;
				UI32 msWaited;			//milliseconds since last pic sent
				STNBTimestampMicro lastTime;
			} frames;
			//playTime
			struct {
				UI64		msReal;		//ms really passed since first framePayload-sent
				UI64		msVirtual;	//ms timestamp in video sent (usually lower or equal to 'msReal')
			} playTime;
		} stream;
	} state;
} STTSServerStreamReq;

void TSServerStreamReq_init(STTSServerStreamReq* obj);
void TSServerStreamReq_release(STTSServerStreamReq* obj);
BOOL TSServerStreamReq_setCfg(STTSServerStreamReq* obj, struct STTSServerStreams_* streamServer, const char* absPath, const char* query, const char* fragment, const STNBHttpServiceReqDesc reqDesc);
BOOL TSServerStreamReq_prepare(STTSServerStreamReq* obj, const STNBHttpServiceRespRawLnk* rawLnk);
void TSServerStreamReq_stopFlag(STTSServerStreamReq* obj);
void TSServerStreamReq_httpReqTick(STTSServerStreamReq* obj, const STNBTimestampMicro timeMicro, const UI64 msCurTick, const UI32 msNextTick, UI32* dstMsNextTick);


#ifdef __cplusplus
} //extern "C"
#endif
	
#endif /* TSServer_h */
