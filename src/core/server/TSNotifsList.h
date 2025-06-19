//
//  TSDevice.h
//  thinstream
//
//  Created by Marcos Ortega on 5/3/19.
//

#ifndef TSNotifsList_h
#define TSNotifsList_h

#include "nb/core/NBStructMap.h"
#include "nb/core/NBString.h"
#include "nb/core/NBArray.h"
//

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSNotifsListItm_ {
		STNBString	textLocaleId;
		STNBString	text;
		STNBString	extraPayloadName;
		STNBString	extraPayloadData;
	} STTSNotifsListItm;
	
	typedef struct STTSNotifsList_ {
		STNBArray	arr; //STTSNotifsListItm
	} STTSNotifsList;
	
	void TSNotifsList_init(STTSNotifsList* obj);
	void TSNotifsList_release(STTSNotifsList* obj);
	
	void TSNotifsList_add(STTSNotifsList* obj, const char* textLocaleId, const char* text, const char* extraPayloadName, void* extraPayloadData, const UI32 extraPayloadDataSz, const STNBStructMap* extraPayloadDataMap);
	
#ifdef __cplusplus
} //extern "C" {
#endif

#endif /* TSNotifsList_h */
