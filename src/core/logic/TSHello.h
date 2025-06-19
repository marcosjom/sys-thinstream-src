//
//  DRStMailboxes.h
//  thinstream
//
//  Created by Marcos Ortega on 1/3/19.
//

#ifndef TSHello_h
#define TSHello_h

#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStructMap.h"
#include "nb/net/NBHttpMessage.h"
#include "nb/net/NBHttpServicePort.h"
#include "nb/net/NBHttpServiceConn.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	SI32 TSHello_srvProcess(void* opq, const STNBHttpMessage* msg, void* stData, const STNBStructMap* stDataMap, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk);
	
#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* DRStMailboxes_h */
