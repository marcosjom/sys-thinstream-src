//
//  TSStreamBuilder.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSStreamBuilder_h
#define TSStreamBuilder_h

#include "nb/core/NBString.h"
#include "nb/core/NBDataPtr.h"
#include "nb/core/NBDataCursor.h"
#include "nb/net/NBWebSocket.h"
#include "nb/net/NBHttpServiceRespRawLnk.h"
#include "nb/files/NBMp4.h"
#include "core/logic/TSStreamUnitBuff.h"

#ifdef __cplusplus
extern "C" {
#endif

//stream data sequence (the base of state machine model)

typedef enum ENTSStreamBuilderFmt_ {
	ENTSStreamBuilderFmt_H264Raw = 0,			//raw h264
	//ENTSStreamBuilderFmt_Mp4Fragmented,		//mp4 fragmented by I-frames
	//ENTSStreamBuilderFmt_Mp4FullyFragmented,	//mp4 fragmented per frame
	//
	ENTSStreamBuilderFmt_Count
} ENTSStreamBuilderFmt;

typedef enum ENTSStreamBuilderUnitsPrefix_ {
	ENTSStreamBuilderUnitsPrefix_0x00000001 = 0,//prefix + raw-h264-unit
	ENTSStreamBuilderUnitsPrefix_UnitSize32,	//UnitSize-32bits (network order) + raw-h264-unit
	//
	ENTSStreamBuilderUnitsPrefix_Count
} ENTSStreamBuilderUnitsPrefix;

typedef enum ENTSStreamBuilderPayloadWrapper_ {
	ENTSStreamBuilderMessageWrapper_None = 0,	//no wrapper
	ENTSStreamBuilderMessageWrapper_WebSocket,	//websocket headers before each nal (frame)
	//
	ENTSStreamBuilderMessageWrapper_Count
} ENTSStreamBuilderMessageWrapper;

NB_OBJREF_HEADER(TSStreamBuilder)

//cfg
BOOL TSStreamBuilder_setFormatAndUnitsPrefix(STTSStreamBuilderRef ref, const ENTSStreamBuilderFmt fmt, const ENTSStreamBuilderUnitsPrefix unitsPrefix);
BOOL TSStreamBuilder_setMsgsWrapper(STTSStreamBuilderRef ref, const ENTSStreamBuilderMessageWrapper msgsWrapper);
BOOL TSStreamBuilder_setRealTimeOffsets(STTSStreamBuilderRef ref, const BOOL realTimeOffsets);

//void TSStreamBuilder_concatUnitOwning(STTSStreamBuilderRef ref, const UI32 msUnitDuration, STTSStreamUnit* u, STNBString* dst, STTSStreamUnitsReleaser* optUnitsReleaser);
void TSStreamBuilder_httpReqSendUnitOwning(STTSStreamBuilderRef ref, const UI32 msUnitDuration, STTSStreamUnit* u, STNBHttpServiceRespRawLnk dst, STTSStreamUnitsReleaser* optUnitsReleaser);

#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSStreamDefs_h */
