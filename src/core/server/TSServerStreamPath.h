//
//  TSServer.h
//  thinstream
//
//  Created by Marcos Ortega on 8/19/18.
//

#ifndef TSServerStreamPath_h
#define TSServerStreamPath_h

#include "nb/core/NBProcess.h"
#include "nb/core/NBRange.h"

#ifdef __cplusplus
extern "C" {
#endif

//stream source

typedef struct STTSServerStreamPath_ {
	char*			src;			//full src path
	//parts
	struct {
		STNBRangeI	runIdRng;		//"*" (all), '_' (current) or specific runId
		char*		runId;			//"*" (all), '_' (current) or specific runId
		STNBRangeI	groupIdRng;		//"*" (all), '_' (any) or specific groupId
		char*		groupId;		//"*" (all), '_' (any) or specific groupId
		STNBRangeI	streamIdRng;	//"*" (all) or specific groupId
		char*		streamId;		//"*" (all) or specific groupId
		STNBRangeI	versionIdRng;	//"*" (all) or specific groupId
		char*		versionId;		//"*" (all) or specific groupId
		STNBRangeI	sourceIdRng;	//"" (default, ~"live"), "live" or "storage"
		char*		sourceId;		//"" (default, ~"live"), "live" or "storage"
	} parts;
} STTSServerStreamPath;

void TSServerStreamPath_init(STTSServerStreamPath* obj);
void TSServerStreamPath_release(STTSServerStreamPath* obj);
//
BOOL TSServerStreamPath_parserSrc(STTSServerStreamPath* obj, const char* src);

#ifdef __cplusplus
} //extern "C"
#endif
	
#endif /* TSServer_h */
