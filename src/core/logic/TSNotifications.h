//
//  TSIdentityReq.h
//  thinstream
//
//  Created by Marcos Ortega on 2/3/19.
//

#ifndef TSNotifications_h
#define TSNotifications_h

#include "nb/core/NBDate.h"
#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"
#include "nb/core/NBString.h"
#include "core/server/TSServerOpq.h"
#include "core/server/TSNotifsList.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	void TSServer_sendNotificationList(STTSServerOpq* srvr, const STTSNotifsList* list);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSIdentityReq_h */

