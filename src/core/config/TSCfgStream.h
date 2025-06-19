//
//  TSCfgStream.h
//  thinstream
//
//  Created by Marcos Ortega on 8/31/18.
//

#ifndef TSCfgStream_h
#define TSCfgStream_h

#include "nb/core/NBStructMap.h"
#include "core/config/TSCfgStreamVersion.h"

#ifdef __cplusplus
extern "C" {
#endif

	//Header

	typedef struct STTSCfgStreamDevice_ {
		char*		uid;		//unique-id (manually set)
		char*		name;		//stream name
		char*		server;		//server
		SI32		port;		//port
		BOOL		useSSL;		//ssl
		char*		user;		//username
		char*		pass;		//password
	} STTSCfgStreamDevice;

	const STNBStructMap* TSCfgStreamDevice_getSharedStructMap(void);

	//Stream

	typedef struct STTSCfgStream_ {
		BOOL					isDisabled;	//
		STTSCfgStreamDevice		device;
		STTSCfgStreamVersion*	versions;	//substreams (versions)
		UI32					versionsSz;	//substreams (versions)
	} STTSCfgStream;
	
	const STNBStructMap* TSCfgStream_getSharedStructMap(void);

	//StreamsGrp

	typedef struct STTSCfgStreamsGrp_ {
		BOOL					isDisabled;	//
		char*					uid;		//unique-id (manually set)
		char*					name;		//streamsGrp name
		STTSCfgStream*			streams;
		UI32					streamsSz;
	} STTSCfgStreamsGrp;

	const STNBStructMap* TSCfgStreamsGrp_getSharedStructMap(void);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCfgStream_h */
