//
//  TSClientRequesterConn.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSClientRequesterConn_h
#define TSClientRequesterConn_h

#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/core/NBDataPtr.h"
#include "nb/net/NBRtspClient.h"
//
#include "core/config/TSCfgStream.h"
#include "core/config/TSCfgStreamVersion.h"
#include "core/config/TSCfgServer.h"
#include "core/logic/TSStreamDefs.h"
#include "core/logic/TSStreamUnitBuff.h"
#include "core/logic/TSClientRequesterConnStats.h"
//

#ifdef __cplusplus
extern "C" {
#endif

struct STTSClientRequesterConn_;

typedef struct STTSClientRequesterConnLstnrItf_ {
	void	(*rtspStreamStateChanged)(void* param, struct STTSClientRequesterConn_* obj, const char* streamId, const char* versionId, const ENNBRtspRessState state);
	//void	(*rtspStreamVideoPropsChanged)(void* param, struct STTSClientRequesterConn_* obj, const char* streamId, const char* versionId, const STNBAvcPicDescBase* props); //samples = pixels
} STTSClientRequesterConnLstnrItf;

typedef struct STTSClientRequesterConnLstnr_ {
	void*	param;
	STTSClientRequesterConnLstnrItf itf;
} STTSClientRequesterConnLstnr;

typedef struct STTSClientRequesterConnViewer_ {
    BOOL            isFirstNotifyDone;
    STTSStreamLstnr lstnr;
} STTSClientRequesterConnViewer;

typedef struct STTSClientRequesterConn_ {
	STNBThreadMutex			mutex;
	STNBThreadCond			cond;
	//rtsp
	struct {
		STNBRtspClient*		clt;
		ENNBRtspRessState	stoppedState;
	} rtsp;
	//Config
	struct {
		char*				runId;			//device
		char*				groupId;		//cams group
		char*				groupName;		//cams group
		char*				streamId;		//camera
		char*				streamName;		//camera
		char*				versionId;		//stream
		char*				versionName;	//stream
		STTSStreamAddress*	address;
		STTSStreamParams*	params;
		BOOL				isRtsp;
		STTSClientRequesterConnLstnr lstnr;
	} cfg;
	//Stats
	struct {
		STTSClientRequesterConnStats*	provider;
		STTSClientRequesterConnStatsUpd	upd;
	} stats;
	//State
	struct { 
		//rtsp
		struct {
			ENNBRtspRessState state;
			//picDesc
			/*struct {
				UI32			iSeq;
				STNBAvcPicDescBase cur;	 //to apply
                STNBAvcPicDescBase tmp;  //before applying to 'cur'
				BOOL			changed;
			} picDesc;*/
		} rtsp;
		//rtp
		struct {
			UI16			ssrc;
			//packets (received)
			struct {
				//time (to determine if connection was lost)
				struct {
					UI64	start;	//first packet time
					UI64	last;	//last packet
				} utcTime;
				//seq (to determine integrity)
				struct {
					BOOL	isFirstKnown;	//first packet arrived (to determine the seqNum)
					UI32	lastSeqNum;		//last seqNum arrived (to determine sequence)
				} seqNum;
			} packets;
		} rtp;
		//units (stream NALU)
		struct {
			UI32			iSeq;		//to apply as unitId
			//STNBAvcParser	parser;
            //time (to determine if connection was lost)
            struct {
                UI64    start;    //first packet time
                UI64    last;    //last packet
            } utcTime;
			//pool
			struct {
				STNBArray	buffs;		//STTSStreamUnitBuffRef, pool of stream-units buffs
				STNBArray	data;		//STTSStreamUnit, pool of stream-units data (data points to buffs)
			} pool;
            //frames
            struct {
                //cur
                struct {
                    //state
                    struct {
                        int  isInvalid;              //payload must be discarded
                        int  lastCompletedNalType;   //
                        int  nalsCountPerType[32];   //counts of NALs contained by this frame, by type (32 max)
                        //delimeter
                        struct {
                            int         isPresent;
                            int         primary_pic_type;   //u(3)
                            int         slicesAllowedPrimaryPicturePerType[32];   //slices allowed in a primary picture
                        } delimeter;
                    } state;
                    STNBArray units; //STTSStreamUnit, units belogns to cur frame
                } cur;
                STNBArray    fromLastIDR; //STTSStreamUnit, pool of stream-units data (data points to buffs)
            } frames;
			//fragments
			struct {
				STTSStreamUnitBuffHdr hdr; //header
				STNBArray	arr;		//STNBDataPtr, accumulated chunks for current unit
				UI32		bytesCount;	//accum
			} frags;
			//state
			/* ToDo: remove (this is a non-compliant validation).
            struct {
				BOOL		nal7Found;	//params
				BOOL		nal5Found;	//IDR
			} state;
            */
		} units;
	} state;
	//Viewers
	struct {
		BOOL				registered; //registered at NBRtspClient
		UI32				iSeq;
		STNBArray			cur;		//STTSClientRequesterConnViewer, to notify
		//notify
		struct {
			UI32			iSeq;       //previous to notification (checked vs )
			STNBArray		cur;		//STTSClientRequesterConnViewer, to notify
		} notify;
	} viewers;
	//dbg
#	ifdef NB_CONFIG_INCLUDE_ASSERTS
	struct {
		BOOL		inUnsafeAction;		//currently executing unsafe code
		BOOL		inConsumeAction;	//currently executing rtpConsume code
	} dbg;
#	endif
} STTSClientRequesterConn;
	
void TSClientRequesterConn_init(STTSClientRequesterConn* obj);
void TSClientRequesterConn_release(STTSClientRequesterConn* obj);

//Config
BOOL TSClientRequesterConn_setStatsProvider(STTSClientRequesterConn* obj, STTSClientRequesterConnStats* statsProvider);
BOOL TSClientRequesterConn_setRtspClient(STTSClientRequesterConn* obj, STNBRtspClient* clt, const ENNBRtspRessState stoppedState);
BOOL TSClientRequesterConn_setConfig(STTSClientRequesterConn* obj, const char* runId, const char* groupId, const char* groupName, const char* streamId, const char* streamName, const char* versionId, const char* versionName, const STTSStreamAddress* address, const STTSStreamParams* params);
BOOL TSClientRequesterConn_setLstnr(STTSClientRequesterConn* obj, STTSClientRequesterConnLstnr* lstnr);
BOOL TSClientRequesterConn_start(STTSClientRequesterConn* obj);
BOOL TSClientRequesterConn_isAddress(STTSClientRequesterConn* obj, const STTSStreamAddress* address);



//Viewers
UI32 TSClientRequesterConn_viewersCount(STTSClientRequesterConn* obj);
BOOL TSClientRequesterConn_addViewer(STTSClientRequesterConn* obj, const STTSStreamLstnr* viewer, UI32* dstCount);
BOOL TSClientRequesterConn_removeViewer(STTSClientRequesterConn* obj, void* param, UI32* dstCount);

#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSClientRequesterConn_h */
