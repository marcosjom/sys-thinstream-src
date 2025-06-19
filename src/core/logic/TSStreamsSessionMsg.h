//
//  TSFile.h
//  thinstream
//
//  Created by Marcos Ortega on 2/3/19.
//

#ifndef TSStreamsSessionMsg_h
#define TSStreamsSessionMsg_h

#include "nb/NBFrameworkDefs.h"
#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"

#ifdef __cplusplus
extern "C" {
#endif

// stream state

/*typedef enum ENTSStreamState_ {
	ENTSStreamState_Stopped = 0,//disconnected
	ENTSStreamState_Paused,		//connected but not streaming
	ENTSStreamState_Playing,	//connected and streaming 
	//
	ENTSStreamState_Count
} ENTSStreamState;

const STNBEnumMap* TSStreamState_getSharedEnumMap(void);*/

// stream version source desc

typedef struct STTSStreamVerSrcDesc_ {
	BOOL		isOnline;
	//STNBAvcPicDescBase props; // last kwnos props
} STTSStreamVerSrcDesc;

const STNBStructMap* TSStreamVerSrcDesc_getSharedStructMap(void);

// stream version desc

typedef struct STTSStreamVerDesc_ {
	BOOL	syncFnd;
	UI32	iSeq;
	char*	uid;					//versionId
	char*	name;					//show-name
	STTSStreamVerSrcDesc live;		//live source
	STTSStreamVerSrcDesc storage;	//storage source
} STTSStreamVerDesc;

const STNBStructMap* TSStreamVerDesc_getSharedStructMap(void);

// stream desc

typedef struct STTSStreamDesc_ {
	BOOL				syncFnd;
	UI32				iSeq;
	char*				uid;	//streamId
	char*				name;	//show-name
	STTSStreamVerDesc*	versions;
	UI32				versionsSz;
	//ENTSStreamState	state; 
} STTSStreamDesc;

const STNBStructMap* TSStreamDesc_getSharedStructMap(void);

// streams grp

typedef struct STTSStreamsGrpDesc_ {
	BOOL			syncFnd;
	UI32			iSeq;
	char*			uid;		//groupId
	char*			name;		//show-name
	STTSStreamDesc*	streams;
	UI32			streamsSz;
} STTSStreamsGrpDesc;

const STNBStructMap* TSStreamsGrpDesc_getSharedStructMap(void);

// streams grps

typedef struct STTSStreamsServiceDesc_ {
	BOOL							syncFnd;
	UI32							iSeq;
	char*							runId;	//runId
	STTSStreamsGrpDesc*				streamsGrps;
	UI32							streamsGrpsSz;
	struct STTSStreamsServiceDesc_*	subServices;
	UI32							subServicesSz;
} STTSStreamsServiceDesc;

const STNBStructMap* TSStreamsServiceDesc_getSharedStructMap(void);

// stream upd action

typedef enum ENTSStreamUpdAction_ {
	ENTSStreamUpdAction_New = 0,	//new record
	ENTSStreamUpdAction_Changed,	//record changed
	ENTSStreamUpdAction_Removed,	//record removed 
	//
	ENTSStreamUpdAction_Count
} ENTSStreamUpdAction;

const STNBEnumMap* TSStreamUpdAction_getSharedEnumMap(void);

// stream upd

typedef struct STTSStreamUpd_ {
	ENTSStreamUpdAction	action;
	STTSStreamDesc		desc; 
} STTSStreamUpd;

const STNBStructMap* TSStreamUpd_getSharedStructMap(void);

// streamsGrp upd

typedef struct STTSStreamsGrpUpd_ {
	char*				uid;		//groupId
	char*				name;		//groupName
	STTSStreamUpd*		streams;
	UI32				streamsSz; 
} STTSStreamsGrpUpd;

const STNBStructMap* TSStreamsGrpUpd_getSharedStructMap(void);

// streams upd

typedef struct STTSStreamsServiceUpd_ {
	char*							runId;
	STTSStreamsGrpUpd*				grps;
	UI32							grpsSz;
	struct STTSStreamsServiceUpd_*	subServices;
	UI32							subServicesSz;
} STTSStreamsServiceUpd;

const STNBStructMap* TSStreamsServiceUpd_getSharedStructMap(void);

//session stream msg

typedef struct STTSStreamsSessionMsg_ {
	char*					sessionId;		//only sent at first msg
	char*					runId;			//server execution unique identifier
	UI64					time;			//sender-side timestamp
	UI64					iSeq;			//sequence of this message
	STTSStreamsServiceUpd*	streamsUpds;
} STTSStreamsSessionMsg;

const STNBStructMap* TSStreamsSessionMsg_getSharedStructMap(void);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSStreamsSessionMsg_h */

