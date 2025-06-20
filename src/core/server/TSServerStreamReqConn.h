//
//  TSServer.h
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#ifndef TSServerStreamReqConn_h
#define TSServerStreamReqConn_h

#include "nb/core/NBMngrProcess.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
//
#include "core/logic/TSStreamUnitBuff.h"
#include "core/logic/TSClientRequesterConn.h"
#include "core/logic/TSStreamVersionStorage.h"

#ifdef __cplusplus
extern "C" {
#endif
	
//viewer
	
typedef struct STTSServerStreamReqConn_ {
	STNBThreadMutex				mutex;
	//state
	struct {
		//rcv
		struct {
			BOOL				started;			//first unit 7 found
			BOOL				ended;				//stream stopped (remove from array)
			UI32				iFrameSeq;			//video-image-related units received
			UI32				secsChangesNoUnits;	//secs changes without receiving units
		} rcv;
	} state;
	STTSClientRequesterConn*	live;		//live source
	STTSStreamVersionStorage*	storage;	//storage source
	STNBArray					buffs[2];	//STTSStreamUnit (last and current); each array contains from first nal7 to previous to next-nal7 (IDR-pic + its non-IDR children)
	//garbage (units to be released)
	struct {
		STNBArray				pend;		//STTSStreamUnit (to return or release)
		SI32					balance;	//optimization, early knowledge that units were given
		//origin
		struct {
			const void*			last;		//to identify units that came from this source
		} origin;
	} garbage;
} STTSServerStreamReqConn;

void TSServerStreamReqConn_init(STTSServerStreamReqConn* obj);
void TSServerStreamReqConn_release(STTSServerStreamReqConn* obj);
BOOL TSServerStreamReqConn_isConnectedToLive(STTSServerStreamReqConn* obj, STTSClientRequesterConn* conn);
BOOL TSServerStreamReqConn_isConnectedToVersionStorage(STTSServerStreamReqConn* obj, STTSStreamVersionStorage* conn);
BOOL TSServerStreamReqConn_conectToLive(STTSServerStreamReqConn* obj, STTSClientRequesterConn* conn);
BOOL TSServerStreamReqConn_conectToVersionStorage(STTSServerStreamReqConn* obj, STTSStreamVersionStorage* conn);
UI32 TSServerStreamReqConn_secsChangesWithoutUnits(STTSServerStreamReqConn* obj);
BOOL TSServerStreamReqConn_getStreamEndedFlag(STTSServerStreamReqConn* obj);
void TSServerStreamReqConn_tickSecChanged(STTSServerStreamReqConn* obj, UI32* dstFrameSeq);
//BOOL TSServerStreamReqConn_getStartUnitFmtFromNewestBuffer(STTSServerStreamReqConn* obj, STNBAvcPicDescBase* dst);
BOOL TSServerStreamReqConn_getStartUnitFromNewestBufferAndFlushOlders(STTSServerStreamReqConn* obj/*, const STNBAvcPicDescBase* optRequiredDescBase*/, STTSStreamUnit* dst, UI32* dstFrameSeq, BOOL* dstNewBlockAvailable, BOOL* dstNewBlockIsCompatible);
BOOL TSServerStreamReqConn_getNextBestLiveUnit(STTSServerStreamReqConn* obj/*, const STNBAvcPicDescBase* optRequiredDescBase*/, STTSStreamUnit* dstt, UI32* dstFrameSeq, BOOL* dstNewBlockAvailable, BOOL* dstNewBlockIsCompatible);
void TSServerStreamReqConn_consumeGarbageUnits(STTSServerStreamReqConn* obj, STNBArray* garbageUnits); //units to be released

#ifdef __cplusplus
} //extern "C"
#endif
	
#endif /* TSServer_h */
