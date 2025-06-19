//
//  TSCfg.h
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#ifndef TSCfg_h
#define TSCfg_h

#include "nb/core/NBString.h"
#include "nb/core/NBStructMap.h"
#include "nb/core/NBArray.h"
#include "nb/core/NBJson.h"
#include "nb/files/NBFilesystem.h"
#include "nb/net/NBHttpServicePort.h"
#include "nb/crypto/NBX509Req.h"
#include "core/config/TSCfgContext.h"
#include "core/config/TSCfgServer.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSCfg_ {
		STTSCfgContext			context;
		STTSCfgServer			server;
		STNBHttpServiceCfg		http;
	} STTSCfg;
	
	const STNBStructMap* TSCfg_getSharedStructMap(void);
	
	void TSCfg_init(STTSCfg* obj);
	void TSCfg_release(STTSCfg* obj);
	
	BOOL TSCfg_loadFromFilePath(STTSCfg* obj, const char* filePath);
	BOOL TSCfg_loadFromFilePathAtRoot(STTSCfg* obj, STNBFilesystem* fs, const ENNBFilesystemRoot root, const char* filePath);
	BOOL TSCfg_loadFromJsonStr(STTSCfg* obj, const char* jsonStr);
	BOOL TSCfg_loadFromJsonStrBytes(STTSCfg* obj, const char* jsonStr, const UI32 jsonStrSz);
	BOOL TSCfg_loadFromJsonNode(STTSCfg* obj, STNBJson* json, const STNBJsonNode* parentNode);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfg_h */
