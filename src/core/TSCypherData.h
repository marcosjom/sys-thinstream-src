//
//  TSCypher.h
//  thinstream
//
//  Created by Marcos Ortega on 8/21/18.
//

#ifndef TSCypherData_h
#define TSCypherData_h

#include "nb/core/NBString.h"
#include "nb/crypto/NBPKey.h"
#include "nb/crypto/NBX509.h"
#include "core/config/TSCfgCypher.h"
#include "core/TSContext.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//CypherData Key
	
	typedef struct STTSCypherDataKey_ {
		BYTE*		pass;		//password encrypted with public-cert
		UI32		passSz;		//password encrypted with public-cert
		BYTE*		salt;		//salt encrypted with public-cert
		UI32		saltSz;		//salt encrypted with public-cert
		UI32		iterations;	//iterations
	} STTSCypherDataKey;
	
	const STNBStructMap* TSCypherDataKey_getSharedStructMap(void);
	
	BOOL TSCypherDataKey_isEmpty(const STTSCypherDataKey* key);
	
	//CypherData Payload
	
	typedef struct STTSCypherData_ {
		BYTE*		data;	//encrypted data
		UI32		dataSz;	//encrypted data
	} STTSCypherData;
	
	const STNBStructMap* TSCypherData_getSharedStructMap(void);
	
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCypher_h */
