//
//  DRStMailboxes.c
//  thinstream
//
//  Created by Marcos Ortega on 1/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSHello.h"
//
#include "nb/core/NBMemory.h"
#include "core/server/TSServerOpq.h"

SI32 TSHello_srvProcess(void* pOpq, const STNBHttpMessage* msg, void* stData, const STNBStructMap* stDataMap, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk){
	BOOL r = FALSE; //NBASSERT(NBHttpServiceResp_getResponseCode(req) == 0)
	//STTSServerOpq* opq = (STTSServerOpq*)pOpq;
	{
		//Just return "200 Hello"
        //
        //r = NBHttpServiceReqArrivalLnk_setDefaultResponseCode(&reqLnk, 200, "Hello")
        //    && NBHttpServiceReqArrivalLnk_setDefaultResponseBodyStr(&reqLnk, "Hello");
	}
	return r;
}
