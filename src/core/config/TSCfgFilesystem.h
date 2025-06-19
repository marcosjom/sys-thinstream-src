//
//  TSCfgFilesystem.h
//  thinstream-test
//
//  Created by Marcos Ortega on 8/20/18.
//

#ifndef TSCfgFilesystem_h
#define TSCfgFilesystem_h

#include "nb/core/NBString.h"
#include "nb/core/NBStructMap.h"
#include "nb/core/NBArray.h"
#include "nb/core/NBLog.h"
#include "nb/core/NBJson.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//streams storage params

	typedef struct STTSCfgFilesystemStreamsParams_ {
		char*			limit;				//string version, like "123", "123B" (bytes), "123b" (bits), "123K", "123KB", "123M", "123MB" (Mega-Bytes), "123Mb" (Mega-bits), "123T", "123TB", "123Tb"
		UI64			limitBytes;			//numeric version
		BOOL			isReadOnly;			//do not write nor delete files
		BOOL			keepUnknownFiles;	//do not remove files that are not part of the loaded tree
        BOOL            waitForLoad;        //if TRUE, then async load will block untill complete.
		UI32			initialLoadThreads;	//amount of threads to use at initial load
	} STTSCfgFilesystemStreamsParams;

	const STNBStructMap* TSCfgFilesystemStreamsParams_getSharedStructMap(void);
	BOOL TSCfgFilesystemStreamsParams_cloneNormalized(STTSCfgFilesystemStreamsParams* obj, const STTSCfgFilesystemStreamsParams* src);


	//streams storage

	typedef struct STTSCfgFilesystemStreams_ {
		char*							rootPath;
		char*							cfgFilename;	//if defined, cfg-file at rootPath with a 'STTSCfgFilesystemStreamsParams' struct will be used instead of "defaults" values.
		STTSCfgFilesystemStreamsParams	defaults; 		//defaults params
	} STTSCfgFilesystemStreams;
	
	const STNBStructMap* TSCfgFilesystemStreams_getSharedStructMap(void);

	//filesystem

	typedef struct STTSCfgFilesystem_ {
		char*						rootFolder;
		STTSCfgFilesystemStreams	streams;
		char**						pkgs;
		UI32						pkgsSz;
	} STTSCfgFilesystem;
	
	const STNBStructMap* TSCfgFilesystem_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgFilesystem_h */
