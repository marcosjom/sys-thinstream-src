//
//  TSIdentityReq.c
//  thinstream
//
//  Created by Marcos Ortega on 2/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSNotifications.h"
//
#include "nb/core/NBMemory.h"
#include "core/logic/TSDevice.h"
#include "core/server/TSServer.h"


//Notifications

void TSServer_sendNotificationList(STTSServerOpq* opq, const STTSNotifsList* list){
	if(list != NULL){
		/*SI32 i; for(i = 0; i < list->arr.use; i++){
			const STTSNotifsListItm* itm = NBArray_itmPtrAtIndex(&list->arr, STTSNotifsListItm, i);
		}*/
	}
}
