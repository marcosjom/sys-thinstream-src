//
//  TSFilesystem.h
//  thinstream
//
//  Created by Marcos Ortega on 8/21/18.
//

#ifndef TSFilesystem_h
#define TSFilesystem_h

#include "nb/core/NBString.h"
#include "core/config/TSCfgFilesystem.h"
#include "nb/storage/NBStorages.h"
#include "nb/storage/NBStorageCache.h"
#if defined(__ANDROID__)
#	include "nb/files/NBFilesystemAndroid.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
	
	typedef struct STTSFilesystem_ {
		void*		opaque;
	} STTSFilesystem;
	
#	if defined(__ANDROID__)
	void					TSFilesystem_init(STTSFilesystem* obj, const STNBAndroidJniItf* jniItf, void* jniObj);
#	else
	void					TSFilesystem_init(STTSFilesystem* obj);
#	endif
	void					TSFilesystem_release(STTSFilesystem* obj);
	
	BOOL					TSFilesystem_loadFromConfig(STTSFilesystem* obj, const STTSCfgFilesystem* cfg);
	
	void					TSFilesystem_concatRootRelativeWithSlash(STTSFilesystem* obj, STNBString* dst);
	const char*				TSFilesystem_getRootRelative(STTSFilesystem* obj);
	STNBFilesystem*			TSFilesystem_getFilesystem(STTSFilesystem* obj);
	
	BOOL					TSFilesystem_setNotifMode(STTSFilesystem* obj, const ENNBStorageNotifMode mode);
	void					TSFilesystem_notifyAll(STTSFilesystem* obj);
	
	//Records listeners
	void					TSFilesystem_addRecordListener(STTSFilesystem* obj, const char* storagePath, const char* recordPath, void* refObj, const STNBStorageCacheItmLstnr* methods);
	void					TSFilesystem_removeRecordListener(STTSFilesystem* obj, const char* storagePath, const char* recordPath, void* refObj);
	
	//Read record
	STNBStorageRecordRead	TSFilesystem_getStorageRecordForRead(STTSFilesystem* obj, const char* storagePath, const char* recordPath, const STNBStructMap* pubMap, const STNBStructMap* privMap);
	void					TSFilesystem_returnStorageRecordFromRead(STTSFilesystem* obj, STNBStorageRecordRead* ref);
	
	//Write record
	STNBStorageRecordWrite	TSFilesystem_getStorageRecordForWrite(STTSFilesystem* obj, const char* storagePath, const char* recordPath, const BOOL createIfNew, const STNBStructMap* pubMap, const STNBStructMap* privMap);
	void					TSFilesystem_returnStorageRecordFromWrite(STTSFilesystem* obj, STNBStorageRecordWrite* ref);
	
	//Touch record (trigger a notification to listeners without saving to file)
	void					TSFilesystem_touchRecord(STTSFilesystem* obj, const char* storagePath, const char* recordPath);

	//List
	void					TSFilesystem_getStorageFiles(STTSFilesystem* obj, const char* storagePath, STNBString* dstStrs, STNBArray* dstFiles /*STNBFilesystemFile*/);
		
	//Reset all records data
	void					TSFilesystem_resetAllStorages(STTSFilesystem* obj, const BOOL deleteFiles);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSFilesystem_h */
