//
//  TSFile.h
//  thinstream
//
//  Created by Marcos Ortega on 2/3/19.
//

#ifndef TSClientRequesterDevice_h
#define TSClientRequesterDevice_h

#include "nb/NBFrameworkDefs.h"
#include "nb/NBObject.h"
#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"
#include "nb/2d/NBSize.h"
//
#include "core/logic/TSStreamsSessionMsg.h"
#include "core/logic/TSClientRequesterConn.h"

#ifdef __cplusplus
extern "C" {
#endif

//DeviceType

typedef enum ENTSClientRequesterDeviceType_ {
	ENTSClientRequesterDeviceType_Native = 0,	//another thinstream server
	ENTSClientRequesterDeviceType_Local,		//rtsp-server (camera) and/or storage
	//
	ENTSClientRequesterDeviceType_Count
} ENTSClientRequesterDeviceType;

//DeviceConnState

typedef enum ENTSClientRequesterDeviceConnState_ {
	ENTSClientRequesterDeviceConnState_Disconnected = 0,	//disconnected
	ENTSClientRequesterDeviceConnState_Connecting,			//connecting
	ENTSClientRequesterDeviceConnState_Connected,			//connected (active)
	//
	ENTSClientRequesterDeviceConnState_Count
} ENTSClientRequesterDeviceConnState;

//Device

NB_OBJREF_HEADER(TSClientRequesterDevice)

typedef struct STTSClientRequesterDeviceLstnrItf_ {
	void (*retain)(void* param);
	void (*release)(void* param);
	void (*requesterSourceConnStateChanged)(void* param, const STTSClientRequesterDeviceRef source, const ENTSClientRequesterDeviceConnState state);
	void (*requesterSourceDescChanged)(void* param, const STTSClientRequesterDeviceRef source);
} STTSClientRequesterDeviceLstnrItf;

typedef struct STTSClientRequesterDeviceLstnr_ {
	void* param;
	STTSClientRequesterDeviceLstnrItf itf;
} STTSClientRequesterDeviceLstnr;

//lstnr
BOOL TSClientRequesterDevice_addLstnr(STTSClientRequesterDeviceRef ref, STTSClientRequesterDeviceLstnr* lstnr);
BOOL TSClientRequesterDevice_removeLstnr(STTSClientRequesterDeviceRef ref, void* param);

//state
BOOL TSClientRequesterDevice_startAsThinstream(STTSClientRequesterDeviceRef ref, const char* server, const UI16 port);
BOOL TSClientRequesterDevice_startAsRtsp(STTSClientRequesterDeviceRef ref, const char* runId, const char* groupId, const char* groupName, const char* streamId, const char* streamName, const char* server, const UI16 port);
void TSClientRequesterDevice_stopFlag(STTSClientRequesterDeviceRef ref);
void TSClientRequesterDevice_stop(STTSClientRequesterDeviceRef ref);
BOOL TSClientRequesterDevice_isRunning(STTSClientRequesterDeviceRef ref);
//rtsp events
BOOL TSClientRequesterDevice_rtspStreamStateChanged(STTSClientRequesterDeviceRef ref, const char* streamId, const char* versionId, const ENNBRtspRessState state);
//BOOL TSClientRequesterDevice_rtspStreamVideoPropsChanged(STTSClientRequesterDeviceRef ref, const char* streamId, const char* versionId, const STNBAvcPicDescBase* props);

//
//BOOL TSClientRequesterDevice_isBetterCandidateForStream(STTSClientRequesterDeviceRef ref, const char* optRootId, const char* runId, const char* streamId, const char* versionId, const char* sourceId, STTSClientRequesterDeviceRef curBestGrp, const UI32 curBestGrpDepth, UI32 *dstBestDepth);

//StreamsGrp (ToDo: move to .c file)

typedef struct STTSClientRequesterDeviceOpq_ {
	STNBObject			prnt;
	STNBThreadCond		cond;
	ENTSClientRequesterDeviceConnState connState;
	STTSStreamsServiceDesc remoteDesc;
	//cfg
	struct {
		ENTSClientRequesterDeviceType type;
		//server
		struct {
			char*		server;
			UI16		port;
		} server;
	} cfg;
	//lstnrs
	struct {
		STNBArray		arr;	//STTSClientRequesterDeviceLstnr
	} lstnrs;
	//reqs (rtsp)
	struct {
		STNBArray		reqs;		//STTSClientRequesterConn*
	} rtsp;
	//incoming and outcoming connections
	struct {
		BOOL			stopFlag;
		BOOL			isRunning;
		STNBSocketRef	socket;
	} syncing;
} STTSClientRequesterDeviceOpq;

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSClientRequesterDevice_h */

