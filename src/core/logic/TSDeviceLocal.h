//
//  TSDeviceLocal.h
//  thinstream
//
//  Created by Marcos Ortega on 5/3/19.
//

#ifndef TSDeviceLocal_h
#define TSDeviceLocal_h

#include "nb/core/NBStructMap.h"
#include "nb/core/NBString.h"
#include "nb/files/NBFilesystem.h"
#include "nb/crypto/NBX509.h"
//
#include "core/TSCypher.h"
#include "core/logic/TSClientRoot.h"
#include "core/TSContext.h"

#ifdef __cplusplus
extern "C" {
#endif

	//
	//Device meeting record
	//

	typedef struct STTSDeviceLocalMeeting_ {
		char*			meetingId;			//Meetings
		UI32			myVouchsAvailCur;	//Current amount of vouches available
		UI32			myVouchsAvailLast;	//Last notified amount of vouchers available
	} STTSDeviceLocalMeeting;

	const STNBStructMap* TSDeviceLocalMeeting_getSharedStructMap(void);

	//
	//Device attest index
	//

	typedef struct STTSDeviceAttest_ {
		char*				meetingId;
		char*				filename;	//name
		UI64				verDatetime;
	} STTSDeviceAttest;

	const STNBStructMap* TSDeviceAttest_getSharedStructMap(void);

	//
	//Device attest indexes
	//

	typedef struct STTSDeviceAttests_ {
		STTSDeviceAttest*	arr;
		UI32				arrSz;
	} STTSDeviceAttests;

	const STNBStructMap* TSDeviceAttests_getSharedStructMap(void);

	//
	//Device
	//

	typedef struct STTSDeviceLocal_ {
		BOOL			authContactRequested;
		BOOL			authNotifsRequested;
		STTSDeviceLocalMeeting*	meets;		//Meetings
		UI32					meetsSz;	//Meetings
		STTSDeviceAttests attests;			//Local attestations indexes
	} STTSDeviceLocal;
	
	const STNBStructMap* TSDeviceLocal_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C" {
#endif

#endif /* TSDeviceLocal_h */
