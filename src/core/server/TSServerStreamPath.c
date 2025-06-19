//
//  TSServerStreamReq.c
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/server/TSServerStreamPath.h"

//stream path

void TSServerStreamPath_init(STTSServerStreamPath* obj){
	NBMemory_setZeroSt(*obj, STTSServerStreamPath);
}

void TSServerStreamPath_release(STTSServerStreamPath* obj){
	if(obj->src != NULL) { NBMemory_free(obj->src); obj->src = NULL; }
	if(obj->parts.runId != NULL) { NBMemory_free(obj->parts.runId); obj->parts.runId = NULL; }
	if(obj->parts.groupId != NULL) { NBMemory_free(obj->parts.groupId); obj->parts.groupId = NULL; }
	if(obj->parts.streamId != NULL) { NBMemory_free(obj->parts.streamId); obj->parts.streamId = NULL; }
	if(obj->parts.versionId != NULL) { NBMemory_free(obj->parts.versionId); obj->parts.versionId = NULL; }
	if(obj->parts.sourceId != NULL) { NBMemory_free(obj->parts.sourceId); obj->parts.sourceId = NULL; }
}

BOOL TSServerStreamPath_parserSrc(STTSServerStreamPath* obj, const char* src){
	//reset
	{
		if(obj->src != NULL) { NBMemory_free(obj->src); obj->src = NULL; }
		if(obj->parts.runId != NULL) { NBMemory_free(obj->parts.runId); obj->parts.runId = NULL; }
		if(obj->parts.groupId != NULL) { NBMemory_free(obj->parts.groupId); obj->parts.groupId = NULL; }
		if(obj->parts.streamId != NULL) { NBMemory_free(obj->parts.streamId); obj->parts.streamId = NULL; }
		if(obj->parts.versionId != NULL) { NBMemory_free(obj->parts.versionId); obj->parts.versionId = NULL; }
		if(obj->parts.sourceId != NULL) { NBMemory_free(obj->parts.sourceId); obj->parts.sourceId = NULL; }
		NBMemory_setZeroSt(obj->parts.runIdRng, STNBRangeI);
		NBMemory_setZeroSt(obj->parts.groupIdRng, STNBRangeI);
		NBMemory_setZeroSt(obj->parts.streamIdRng, STNBRangeI);
		NBMemory_setZeroSt(obj->parts.versionIdRng, STNBRangeI);
	}
	//parse
	obj->parts.runIdRng.start = 0;
	const SI32 pRunEnd = NBString_strIndexOf(src, "/", obj->parts.runIdRng.start);
	if(pRunEnd < 0){
		obj->parts.runIdRng.size = NBString_strLenBytes(src) - obj->parts.runIdRng.start;  
	} else {
		const SI32 pGroupEnd			= NBString_strIndexOf(src, "/", pRunEnd + 1);
		obj->parts.runIdRng.size		= pRunEnd - obj->parts.runIdRng.start;
		obj->parts.groupIdRng.start	= pRunEnd + 1;
		if(pGroupEnd < 0){
			obj->parts.groupIdRng.size	= NBString_strLenBytes(src) - pRunEnd - 1; 
		} else {
			const SI32 pStreamEnd			= NBString_strIndexOf(src, "/", pGroupEnd + 1);
			obj->parts.groupIdRng.size		= pGroupEnd - pRunEnd - 1;
			obj->parts.streamIdRng.start	= pGroupEnd + 1;
			if(pStreamEnd < 0){
				obj->parts.streamIdRng.size = NBString_strLenBytes(src) - pGroupEnd - 1; 
			} else {
				const SI32 pVersionEnd			= NBString_strIndexOf(src, "/", pStreamEnd + 1);
				obj->parts.streamIdRng.size	= pStreamEnd - pGroupEnd - 1;
				obj->parts.versionIdRng.start	= pStreamEnd + 1;
				if(pVersionEnd < 0){
					obj->parts.versionIdRng.size	= NBString_strLenBytes(src) - pStreamEnd - 1; 
				} else {
					const SI32 pSourceEnd			= NBString_strIndexOf(src, "/", pVersionEnd + 1);
					obj->parts.versionIdRng.size	= pVersionEnd - pStreamEnd - 1;
					obj->parts.sourceIdRng.start	= pVersionEnd + 1;
					if(pSourceEnd < 0){
						obj->parts.sourceIdRng.size	= NBString_strLenBytes(src) - pVersionEnd - 1;
					} else {
						obj->parts.sourceIdRng.size	= pSourceEnd - pVersionEnd - 1;
					}
				}
			}
		}
	}
	if(obj->parts.runIdRng.size > 0) obj->parts.runId = NBString_strNewBufferBytes(&src[obj->parts.runIdRng.start], obj->parts.runIdRng.size);
	if(obj->parts.groupIdRng.size > 0) obj->parts.groupId = NBString_strNewBufferBytes(&src[obj->parts.groupIdRng.start], obj->parts.groupIdRng.size);
	if(obj->parts.streamIdRng.size > 0) obj->parts.streamId = NBString_strNewBufferBytes(&src[obj->parts.streamIdRng.start], obj->parts.streamIdRng.size);
	if(obj->parts.versionIdRng.size > 0) obj->parts.versionId = NBString_strNewBufferBytes(&src[obj->parts.versionIdRng.start], obj->parts.versionIdRng.size);
	if(obj->parts.sourceIdRng.size > 0) obj->parts.sourceId = NBString_strNewBufferBytes(&src[obj->parts.sourceIdRng.start], obj->parts.sourceIdRng.size);
	//
	return (obj->parts.runIdRng.size > 0 && obj->parts.groupIdRng.size > 0 && obj->parts.streamIdRng.size > 0 && obj->parts.versionIdRng.size > 0);
}

