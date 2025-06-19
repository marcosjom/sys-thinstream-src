//
//  TSFilesystem.c
//  thinstream
//
//  Created by Marcos Ortega on 8/21/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/TSFilesystem.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBString.h"
#include "nb/core/NBArray.h"
#include "nb/core/NBThread.h"
#include "nb/core/NBThreadMutex.h"
#include "nb/core/NBThreadCond.h"
#include "nb/files/NBFilesystem.h"
#include "nb/files/NBFilesystemPkgs.h"
#ifdef _WIN32
#	include "nb/files/NBFilesystemWin.h"
#elif defined(__ANDROID__)
#	include "nb/files/NBFilesystemAndroid.h"
#elif defined(__APPLE__)
#	include "nb/files/NBFilesystemApple.h"
#endif

typedef struct STTSFilesystemOpq_ {
	STNBFilesystem			fs;
#	ifdef _WIN32
	STNBFilesystemWin		fsWin;
#	elif defined(__ANDROID__)
	STNBFilesystemAndroid	fsAndroid;
#	elif defined(__APPLE__)
	STNBFilesystemApple		fsApple;
#	endif
	STNBFilesystemPkgs		fsPkgs;
	STNBStorages			storages;
} STTSFilesystemOpq;

#if defined(__ANDROID__)
void TSFilesystem_init(STTSFilesystem* obj, const STNBAndroidJniItf* jniItf, void* jniObj)
#else
void TSFilesystem_init(STTSFilesystem* obj)
#endif
{
	STTSFilesystemOpq* opq = obj->opaque = NBMemory_allocType(STTSFilesystemOpq);
	NBMemory_setZeroSt(*opq, STTSFilesystemOpq);
	//Filesystems
	{
		NBFilesystem_init(&opq->fs);
#		ifdef _WIN32
		NBFilesystemWin_init(&opq->fsWin);
		NBFilesystem_pushItf(&opq->fs, NBFilesystemWin_createItf, &opq->fsWin); //Enable specials folders
#		elif defined(__ANDROID__)
		NBFilesystemAndroid_init(&opq->fsAndroid, jniItf, jniObj);
		NBFilesystem_pushItf(&opq->fs, NBFilesystemAndroid_createItf, &opq->fsAndroid); //Enable specials folders
#		elif defined(__APPLE__)
		NBFilesystemApple_init(&opq->fsApple);
		NBFilesystem_pushItf(&opq->fs, NBFilesystemApple_createItf, &opq->fsApple); //Enable specials folders
#		endif
		NBFilesystemPkgs_init(&opq->fsPkgs);
		NBFilesystem_pushItf(&opq->fs, NBFilesystemPkgs_createItf, &opq->fsPkgs);	//Enable packages's virtual structure
	}
	//Storages
	NBStorages_init(&opq->storages);
	NBStorages_setFilesystem(&opq->storages, &opq->fs);
}

void TSFilesystem_release(STTSFilesystem* obj){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	//Storages
	NBStorages_release(&opq->storages);
	//Filesystems
	{
		NBFilesystem_release(&opq->fs);
		NBFilesystemPkgs_release(&opq->fsPkgs);
#		ifdef _WIN32
		NBFilesystemWin_release(&opq->fsWin);
#		elif defined(__ANDROID__)
		NBFilesystemAndroid_release(&opq->fsAndroid);
#		elif defined(__APPLE__)
		NBFilesystemApple_release(&opq->fsApple);
#		endif
	}
	NBMemory_free(obj->opaque);
	obj->opaque = NULL;
}

BOOL TSFilesystem_loadFromConfig(STTSFilesystem* obj, const STTSCfgFilesystem* cfg){
	BOOL r = TRUE;
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	//Storages
	{
		NBStorages_setRootRelative(&opq->storages, cfg->rootFolder);
	}
	//Packages
	{
		UI32 i; for(i = 0; i < cfg->pkgsSz; i++){
			const char* w = cfg->pkgs[i];
			if(w[0] != '\0'){
				PRINTF_INFO("pkg: '%s'.\n", w);
				NBFilesystemPkgs_addPkgFromPath(&opq->fsPkgs, ENNBFilesystemRoot_AppBundle, w);
			}
		}
	}
	return r;
}

//

void TSFilesystem_concatRootRelativeWithSlash(STTSFilesystem* obj, STNBString* dst){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	if(dst != NULL){
		const char* root = NBStorages_getRootRelative(&opq->storages);
		if(root != NULL){
			if(*root != '\0'){
				NBString_concat(dst, root);
				if(dst->str[dst->length - 1] != '/' && dst->str[dst->length - 1] != '\\'){
					NBString_concatByte(dst, '/');
				}
			}
		}
	}
}

const char* TSFilesystem_getRootRelative(STTSFilesystem* obj){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	return NBStorages_getRootRelative(&opq->storages);
}

STNBFilesystem* TSFilesystem_getFilesystem(STTSFilesystem* obj){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	return &opq->fs;
}

//

BOOL TSFilesystem_setNotifMode(STTSFilesystem* obj, const ENNBStorageNotifMode mode){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	return NBStorages_setNotifMode(&opq->storages, mode);
}

void TSFilesystem_notifyAll(STTSFilesystem* obj){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	return NBStorages_notifyAll(&opq->storages);
}

//Records listeners
void TSFilesystem_addRecordListener(STTSFilesystem* obj, const char* storagePath, const char* recordPath, void* refObj, const STNBStorageCacheItmLstnr* methods){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	NBStorages_addRecordListener(&opq->storages, storagePath, recordPath, refObj, methods);
}

void TSFilesystem_removeRecordListener(STTSFilesystem* obj, const char* storagePath, const char* recordPath, void* refObj){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	NBStorages_removeRecordListener(&opq->storages, storagePath, recordPath, refObj);
}

//Read record

STNBStorageRecordRead TSFilesystem_getStorageRecordForRead(STTSFilesystem* obj, const char* storagePath, const char* recordPath, const STNBStructMap* pubMap, const STNBStructMap* privMap){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	return NBStorages_getStorageRecordForRead(&opq->storages, storagePath, recordPath, pubMap, privMap);
}

void TSFilesystem_returnStorageRecordFromRead(STTSFilesystem* obj, STNBStorageRecordRead* ref){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	return NBStorages_returnStorageRecordFromRead(&opq->storages, ref);
}

//Write record

STNBStorageRecordWrite TSFilesystem_getStorageRecordForWrite(STTSFilesystem* obj, const char* storagePath, const char* recordPath, const BOOL createIfNew, const STNBStructMap* pubMap, const STNBStructMap* privMap){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	return NBStorages_getStorageRecordForWrite(&opq->storages, storagePath, recordPath, createIfNew, pubMap, privMap);
}

void TSFilesystem_returnStorageRecordFromWrite(STTSFilesystem* obj, STNBStorageRecordWrite* ref){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	return NBStorages_returnStorageRecordFromWrite(&opq->storages, ref);
}

//Touch record (trigger a notification to listeners without saving to file)

void TSFilesystem_touchRecord(STTSFilesystem* obj, const char* storagePath, const char* recordPath){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	return NBStorages_touchRecord(&opq->storages, storagePath, recordPath);
}

//List

void TSFilesystem_getStorageFiles(STTSFilesystem* obj, const char* storagePath, STNBString* dstStrs, STNBArray* dstFiles /*STNBFilesystemFile*/){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	NBStorages_getFiles(&opq->storages, storagePath, FALSE, dstStrs, dstFiles);
}

//Reset all records data

void TSFilesystem_resetAllStorages(STTSFilesystem* obj, const BOOL deleteFiles){
	STTSFilesystemOpq* opq = (STTSFilesystemOpq*)obj->opaque;
	NBStorages_resetAllStorages(&opq->storages, deleteFiles);
}
