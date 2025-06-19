//
//  TSServerStreams.c
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSServerSessionReq.h"
//
#include <stdlib.h>	//for rand()
#include "nb/core/NBMemory.h"
#include "nb/core/NBNumParser.h"
//
#include "core/server/TSServerOpq.h"
#include "core/server/TSServerStreamReq.h"


void TSServerSessionReq_init(STTSServerSessionReq* obj){
	NBMemory_setZeroSt(*obj, STTSServerSessionReq);
    //
    NBString_initWithSz(&obj->buff, 4096, 4096, 0.10f);
    NBString_init(&obj->buff2);
    //state
    {
        obj->state.lastMsgTime = NBDatetime_getCurUTCTimestamp();
    }
	//
	NBThreadMutex_init(&obj->mutex);
	NBThreadCond_init(&obj->cond);
}

void TSServerSessionReq_release(STTSServerSessionReq* obj){
	NBThreadMutex_lock(&obj->mutex);
	{
        //rawLnk
        {
            if(NBHttpServiceRespRawLnk_isSet(&obj->rawLnk)){
                NBHttpServiceRespRawLnk_close(&obj->rawLnk);
            }
            NBMemory_setZeroSt(obj->rawLnk, STNBHttpServiceRespRawLnk);
        }
		//net
		{
			if(NBWebSocket_isSet(obj->net.websocket)){
				NBWebSocket_release(&obj->net.websocket);
				NBWebSocket_null(&obj->net.websocket);
			}
		}
		//sent
		{
			//streamsGrps
			{
				NBStruct_stRelease(TSStreamsServiceDesc_getSharedStructMap(), &obj->sync.cur, sizeof(obj->sync.cur));
				NBStruct_stRelease(TSStreamsServiceDesc_getSharedStructMap(), &obj->sync.sent, sizeof(obj->sync.sent));
			}
		}
        //
        {
            NBString_release(&obj->buff);
            NBString_release(&obj->buff2);
        }
		NBThreadCond_release(&obj->cond);
	}
	NBThreadMutex_unlock(&obj->mutex);
	NBThreadMutex_release(&obj->mutex);
}
