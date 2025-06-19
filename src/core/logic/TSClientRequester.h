//
//  TSFile.h
//  thinstream
//
//  Created by Marcos Ortega on 2/3/19.
//

#ifndef TSClientRequester_h
#define TSClientRequester_h

#include "nb/NBFrameworkDefs.h"
#include "nb/NBObject.h"
#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"
#include "nb/2d/NBSize.h"
//
#include "core/logic/TSStreamsSessionMsg.h"
#include "core/logic/TSClientRequesterConn.h"
#include "core/logic/TSClientRequesterDevice.h"

#ifdef __cplusplus
extern "C" {
#endif

//

NB_OBJREF_HEADER(TSClientRequester)

//Requester lstnr

typedef struct STTSClientRequesterLstnrItf_ {
	void (*retain)(void* param);
	void (*release)(void* param);
	void (*requesterSourcesUpdated)(void* param, STTSClientRequesterRef reqRef);
} STTSClientRequesterLstnrItf;

typedef struct STTSClientRequesterLstnr_ {
	void* param;
	STTSClientRequesterLstnrItf itf;
} STTSClientRequesterLstnr;

//----------------
//- TSClientRequesterCmdState
//----------------

typedef struct STTSClientRequesterCmdState_ {
	UI32				seq;
	BOOL				isPend;		//zero means command was completed
	STNBRtspCmdState	rtsp;		//
} STTSClientRequesterCmdState;

void TSClientRequesterCmdState_init(STTSClientRequesterCmdState* obj);
void TSClientRequesterCmdState_release(STTSClientRequesterCmdState* obj);

//cfg
BOOL TSClientRequester_setStatsProvider(STTSClientRequesterRef ref, STNBRtspClientStats* rtspStats, STTSClientRequesterConnStats* reqsStats);
BOOL TSClientRequester_setExternalBuffers(STTSClientRequesterRef ref, STNBDataPtrsPoolRef pool);
BOOL TSClientRequester_setCfg(STTSClientRequesterRef ref, const STNBRtspClientCfg* cfg, const char* runId, const ENNBRtspRessState stoppedState);
BOOL TSClientRequester_setPollster(STTSClientRequesterRef ref, STNBIOPollsterRef pollster, STNBIOPollsterSyncRef pollSync);
BOOL TSClientRequester_setPollstersProvider(STTSClientRequesterRef ref, STNBIOPollstersProviderRef provider);

//lstnrs (requester)
BOOL TSClientRequester_addLstnr(STTSClientRequesterRef ref, STTSClientRequesterLstnr* lstnr);
BOOL TSClientRequester_removeLstnr(STTSClientRequesterRef ref, void* param);

BOOL TSClientRequester_prepare(STTSClientRequesterRef ref);
BOOL TSClientRequester_start(STTSClientRequesterRef ref);
void TSClientRequester_stopFlag(STTSClientRequesterRef ref);
BOOL TSClientRequester_isBusy(STTSClientRequesterRef ref);
void TSClientRequester_tickSecChanged(STTSClientRequesterRef ref, const UI64 timestamp);
	
//Commands
void TSClientRequester_statsFlushStart(STTSClientRequesterRef ref, STTSClientRequesterCmdState* dst);
BOOL TSClientRequester_statsFlushIsPend(STTSClientRequesterRef ref, STTSClientRequesterCmdState* dst);
void TSClientRequester_statsGet(STTSClientRequesterRef ref, STNBArray* dstPortsStats /*STNBRtpClientPortStatsData*/, STNBArray* dstSsrcsStats /*STNBRtpClientSsrcStatsData*/, const BOOL resetAccum);

//grps
STTSClientRequesterDeviceRef* TSClientRequester_getGrpsAndLock(STTSClientRequesterRef ref, SI32* dstSz); 
void TSClientRequester_returnGrpsAndUnlock(STTSClientRequesterRef ref, STTSClientRequesterDeviceRef* grsp);

//register devices
BOOL TSClientRequester_addDeviceThinstream(STTSClientRequesterRef ref, const char* server, const UI16 port);
BOOL TSClientRequester_addDeviceRtspStream(STTSClientRequesterRef ref, const char* groupId, const char* groupName, const char* streamId, const char* streamName, const char* versionId, const char* versionName, const STTSStreamAddress* address, const STTSStreamParams* params);

//rtsp
BOOL TSClientRequester_rtspAddStream(STTSClientRequesterRef ref, const char* groupId, const char* groupName, const char* streamId, const char* streamName, const char* versionId, const char* versionName, const STTSStreamAddress* address, const STTSStreamParams* params);
BOOL TSClientRequester_rtspAddViewer(STTSClientRequesterRef ref, STTSStreamLstnr* viewer, const char* groupId, const char* groupName, const char* streamId, const char* streamName, const char* versionId, const char* versionName, const STTSStreamAddress* address, const STTSStreamParams* params);
BOOL TSClientRequester_rtspRemoveViewer(STTSClientRequesterRef ref, STTSStreamLstnr* viewer);
BOOL TSClientRequester_rtspConcatTargetBySsrc(STTSClientRequesterRef ref, const UI16 ssrc, STNBString* dst);
BOOL TSClientRequester_rtspConcatStreamVersionIdBySsrc(STTSClientRequesterRef ref, const UI16 ssrc, STNBString* dst);
STTSClientRequesterConn* TSClientRequester_getRtspReqRetained(STTSClientRequesterRef ref, const char* streamId, const char* versionId);
void TSClientRequester_returnRtspReq(STTSClientRequesterRef ref, STTSClientRequesterConn* req);
BOOL TSClientRequester_isRtspReqStreamIdAtGroupId(STTSClientRequesterRef ref, const char* streamId, const char* groupId);

//viewers
//BOOL TSClientRequester_addStreamViewer(STTSClientRequesterRef ref, const char* optRootId, const char* runId, const char* streamId, const char* versionId, const char* sourceId, STTSStreamLstnr* viewer);
//BOOL TSClientRequester_removeStreamViewer(STTSClientRequesterRef ref, const char* optRootId, const char* runId, const char* streamId, const char* versionId, const char* sourceId, void* param);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSClientRequester_h */

