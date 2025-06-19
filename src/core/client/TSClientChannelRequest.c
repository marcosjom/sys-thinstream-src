//
//  TSClientChannelRequest.c
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/client/TSClientChannelRequest.h"

void TSClientChannelRequest_release(STTSClientChannelRequest* req){
	NBArray_release(&req->paramsIdx);
	NBString_release(&req->paramsStrs);
	NBString_release(&req->body);
	if(req->response != NULL){
		NBHttpMessage_release(req->response);
		NBMemory_free(req->response);
		req->response = NULL;
	}
}
