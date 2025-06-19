	//
//  TSRequestsMap.h
//  thinstream
//
//  Created by Marcos Ortega on 8/24/18.
//

#ifndef TSRequestsMap_h
#define TSRequestsMap_h

#include "nb/core/NBEnumMap.h"
#include "nb/core/NBStruct.h"
#include "nb/net/NBHttpMessage.h"
#include "nb/net/NBHttpServicePort.h"
#include "nb/net/NBHttpServiceConn.h"
#include "nb/crypto/NBCrc32.h"
#include "core/TSRequestsMap.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef enum ENTSRequestId_ {
		ENTSRequestId_Hello = 0,					//Hello echo msg to ping measurement or mantain active the connection.
		//
		ENTSRequestId_Count
	} ENTSRequestId;

	const STNBEnumMap* TSRequestId_getSharedEnumMap(void);
	
	typedef SI32 (*TSServerRequestMethod)(void* opq, const STNBHttpMessage* msg, void* stData, const STNBStructMap* stDataMap, const UI32 port, STNBHttpServiceConnRef conn, const STNBHttpServiceReqDesc reqDesc, STNBHttpServiceReqArrivalLnk reqLnk);
	typedef SI32 (*TSClientResponseMethod)(void* opq, const STNBHttpMessage* msg, void* stData, const STNBStructMap* stDataMap, void* reqLstnrParam);
	typedef SI32 (*TSClientStreamStartMethod)(void* opq, void** dstParam, SI32* dstDiscardData);
	typedef SI32 (*TSClientStreamPeekMethod)(void* opq, void* param, const void* data, const SI32 dataSz);
	typedef SI32 (*TSClientStreamEndMethod)(void* opq, void* param);
	
	typedef struct STTSRequestsMapItm_ {
		ENTSRequestId				reqId;			//numeric
		const char*					resource;		//resource
		//Server method (send complete request)
		TSServerRequestMethod		reqMethod;
		const STNBStructMap*		reqBodyMap;		//map of expected body at request
		//Client method (read complete response)
		TSClientResponseMethod		respMethod;
		const STNBStructMap*		respBodyMap;	//map of expected body at response
		//Client method (read stream stream)
		TSClientStreamStartMethod	respStartMethod;
		TSClientStreamPeekMethod	respPeekMethod;
		TSClientStreamEndMethod		respEndMethod;
	} STTSRequestsMapItm;
	
	typedef struct STTSRequestsMap_ {
		void* opaque;
	} STTSRequestsMap;
	
	void						TSRequestsMap_init(STTSRequestsMap* obj);
	void						TSRequestsMap_release(STTSRequestsMap* obj);
	
	ENTSRequestId				TSRequestsMap_getRequestIdByRes(const STTSRequestsMap* obj, const char* resource);
	STNBEnumMapRecord			TSRequestsMap_getRequestRecordById(const STTSRequestsMap* obj, const ENTSRequestId reqId);
	
	void						TSRequestsMap_setPropsById(STTSRequestsMap* obj, const ENTSRequestId reqId, TSServerRequestMethod reqMethod, const STNBStructMap* reqBodyMap, TSClientResponseMethod respMethod, const STNBStructMap* respBodyMap, TSClientStreamStartMethod respStartMethod, TSClientStreamPeekMethod respPeekMethod, TSClientStreamEndMethod respEndMethod);
	const STTSRequestsMapItm*	TSRequestsMap_getPropsById(const STTSRequestsMap* obj, const ENTSRequestId reqId);
	
	//UI32 TSRequestsMap_getCrc32Signature(const STTSRequestsMap* obj);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSRequestsMap_h */
