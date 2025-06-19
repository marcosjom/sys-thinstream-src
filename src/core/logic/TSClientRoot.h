//
//  TSClientRoot.h
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#ifndef TSClientRoot_h
#define TSClientRoot_h

#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"
#include "core/logic/TSDevice.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//Error log send-mode
	
	typedef enum ENDRErrorLogSendMode_ {
		ENDRErrorLogSendMode_Ask = 0,		//Ask user
		ENDRErrorLogSendMode_Allways,	//Send without asking
		ENDRErrorLogSendMode_Never,			//Never send
		ENDRErrorLogSendMode_Count
	} ENDRErrorLogSendMode;
	
	const STNBEnumMap* ENDRErrorLogSendMode_getSharedEnumMap(void);
	
	//
	//Root data registration
	//
	
	typedef struct STTSClientRootReq_ {
		char*		clinicName;
		char*		firstNames;
		char*		lastNames;
		UI64		verifSeqIdTime; //UTC
		char*		verifSeqId;	//"I'm this person and accept the terms"
		BOOL		willOverrideOther;
		BOOL		canHaveMoreCodes;
		BOOL		linkedToClinic;
		BOOL		isAdmin;
		BYTE*		pkeyDER;		//
		UI32		pkeyDERSz;		//
	} STTSClientRootReq;

	const STNBStructMap* TSClientRootReq_getSharedStructMap(void);

	//
	//Root data
	//

	typedef struct STTSClientRoot_ {
		//Old
		ENDRErrorLogSendMode			errLogMode;		//Send error log mode
		//New
		char*							deviceId;
		STTSCypherDataKey				sharedKeyEnc;	//for quick encryption
		//Registration process
		STTSClientRootReq				req;
	} STTSClientRoot;
	
	const STNBStructMap* TSClientRoot_getSharedStructMap(void);
	
	//Tools
	BOOL TSClientRoot_getCurrentDeviceId(STTSContext* context, STNBString* dstDeviceId);
	
#ifdef __cplusplus
} //extern "C" {
#endif

#endif /* TSClientRoot_h */
