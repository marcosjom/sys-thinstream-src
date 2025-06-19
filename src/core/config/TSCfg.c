//
//  TSCfg.c
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/config/TSCfg.h"
//
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

STNBStructMapsRec TSCfg_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCfg_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCfg_sharedStructMap);
	if(TSCfg_sharedStructMap.map == NULL){
		STTSCfg s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCfg);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStructM(map, s, context, TSCfgContext_getSharedStructMap());
		NBStructMap_addStructM(map, s, server, TSCfgServer_getSharedStructMap());
		NBStructMap_addStructM(map, s, http, NBHttpServiceCfg_getSharedStructMap());		
		TSCfg_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCfg_sharedStructMap);
	return TSCfg_sharedStructMap.map;
}

//

void TSCfg_init(STTSCfg* obj){
	NBMemory_set(obj, 0, sizeof(*obj));
}

void TSCfg_release(STTSCfg* obj){
	NBStruct_stRelease(TSCfg_getSharedStructMap(), obj, sizeof(*obj)); //ToDo: implement
	NBMemory_set(obj, 0, sizeof(*obj));
}

BOOL TSCfg_loadFromFilePath(STTSCfg* obj, const char* filePath){
	BOOL r = FALSE;
	STNBJson json;
	NBJson_init(&json);
	if(!NBJson_loadFromFilePath(&json, filePath)){
		PRINTF_CONSOLE_ERROR("Could not load JSON file: '%s'.\n", filePath);
	} else {
		//Load server
		if(!TSCfg_loadFromJsonNode(obj, &json, NBJson_rootMember(&json))){
			PRINTF_CONSOLE_ERROR("Could not load server config: '%s'.\n", filePath);
		} else {
			r = TRUE;
		}
	}
	NBJson_release(&json);
	return r;
}

BOOL TSCfg_loadFromFilePathAtRoot(STTSCfg* obj, STNBFilesystem* fs, const ENNBFilesystemRoot root, const char* filePath){
	BOOL r = FALSE;
	STNBString str;
	NBString_initWithSz(&str, 4096, 4096, 0.10f);
	if(!NBFilesystem_readFromFilepathAtRoot(fs, root, filePath, &str)){
		PRINTF_CONSOLE_ERROR("Could not load JSON file from root(%d): '%s'.\n", root, filePath);
	} else if(!TSCfg_loadFromJsonStr(obj, str.str)){
		PRINTF_CONSOLE_ERROR("Could not load config from loaded content: '%s'.\n", filePath);
	} else {
		r = TRUE;
	}
	NBString_release(&str);
	return r;
}

BOOL TSCfg_loadFromJsonStr(STTSCfg* obj, const char* jsonStr){
	BOOL r = FALSE;
	STNBJson json;
	NBJson_init(&json);
	if(!NBJson_loadFromStr(&json, jsonStr)){
		PRINTF_CONSOLE_ERROR("Could not load JSON from str-len(%d).\n", NBString_strLenBytes(jsonStr));
	} else {
		r = TRUE;
		//Load server
		if(!TSCfg_loadFromJsonNode(obj, &json, NBJson_rootMember(&json))){
			PRINTF_CONSOLE_ERROR("Could not load server config from from str-len(%d).\n", NBString_strLenBytes(jsonStr));
		}
	}
	NBJson_release(&json);
	return r;
}

BOOL TSCfg_loadFromJsonStrBytes(STTSCfg* obj, const char* jsonStr, const UI32 jsonStrSz){
	BOOL r = FALSE;
	STNBJson json;
	NBJson_init(&json);
	if(!NBJson_loadFromStrBytes(&json, jsonStr, jsonStrSz)){
		PRINTF_CONSOLE_ERROR("Could not load JSON from str-len(%d).\n", NBString_strLenBytes(jsonStr));
	} else {
		r = TRUE;
		//Load server
		if(!TSCfg_loadFromJsonNode(obj, &json, NBJson_rootMember(&json))){
			PRINTF_CONSOLE_ERROR("Could not load server config from from str-len(%d).\n", NBString_strLenBytes(jsonStr));
		}
	}
	NBJson_release(&json);
	return r;
}

BOOL TSCfg_loadFromJsonNode(STTSCfg* obj, STNBJson* json, const STNBJsonNode* parentNode){
    BOOL r = FALSE;
    //release
    NBStruct_stRelease(TSCfg_getSharedStructMap(), obj, sizeof(*obj));
    //load
	r = NBStruct_stReadFromJsonNode(json, parentNode, TSCfg_getSharedStructMap(), obj, sizeof(*obj));
	/*if(r){
		STNBString str;
		NBString_init(&str);
		NBStruct_stConcatAsJson(&str, TSCfg_getSharedStructMap(), obj, sizeof(*obj));
		NBString_release(&str);
	}*/
	return r;
}
