//
//  TSCypher.h
//  thinstream
//
//  Created by Marcos Ortega on 8/21/18.
//

#ifndef TSCypher_h
#define TSCypher_h

#include "nb/core/NBString.h"
#include "nb/crypto/NBPKey.h"
#include "nb/crypto/NBX509.h"
#include "core/config/TSCfgCypher.h"
#include "core/TSContext.h"
#include "core/TSCypherData.h"
#include "core/TSCypherJob.h"

#ifdef __cplusplus
extern "C" {
#endif

	//Itf for remote decryption
	
	typedef struct STTSCypherDecItf_ {
		void*		obj;
		BOOL		(*decDataKey)(void* obj, STTSCypherDataKey* dstKeyPlain, const STTSCypherDataKey* keyEnc);
	} STTSCypherDecItf;
	
	//
	
	typedef struct STTSCypher_ {
		void*		opaque;
	} STTSCypher;
	
	void TSCypher_init(STTSCypher* obj);
	void TSCypher_release(STTSCypher* obj);
	
	BOOL TSCypher_loadFromConfig(STTSCypher* obj, STTSContext* context, STNBX509* caCert, const STTSCfgCypher* cfg);
	
	//Thread
	BOOL TSCypher_startThreads(STTSCypher* obj);
	BOOL TSCypher_addAsyncJob(STTSCypher* obj, STTSCypherJobRef* job);
	
	BOOL TSCypher_isKeyAnySet(STTSCypher* obj);		//When decryption is made local or remote
	BOOL TSCypher_isKeyLocalSet(STTSCypher* obj);	//When decryption is made local
	BOOL TSCypher_setCertAndKey_(STTSCypher* obj, STNBX509* cert, STNBPKey* key); //When decryption is made local
	BOOL TSCypher_setSharedKey_(STTSCypher* obj, const STTSCypherDataKey* sharedKeyEnc);
	BOOL TSCypher_getSharedKey(STTSCypher* obj, STTSCypherDataKey* dstSharedKeyPlain, STTSCypherDataKey* dstSharedKeyEnc);
	BOOL TSCypher_encForCurrentKey(STTSCypher* obj, const void* plain, const UI32 plainSz, STNBString* dst);
	BOOL TSCypher_decWithCurrentKey(STTSCypher* obj, const void* enc, const UI32 encSz, STNBString* dst);
	BOOL TSCypher_concatCertSerialHex(STTSCypher* obj, STNBString* dst);
	BOOL TSCypher_concatBytesSignatureSha1(STTSCypher* obj, const void* data, const SI32 dataSz, STNBString* dstBin);
	//Clone data
	void TSCypher_cloneData(STTSCypherData* dst, const STTSCypherData* src);
	//Encode
	BOOL TSCypher_genDataKeyWithCurrentKey(STTSCypher* obj, STTSCypherDataKey* dstEnc, STTSCypherDataKey* dstPlain);
	BOOL TSCypher_genDataKey(STTSCypherDataKey* dstEnc, STTSCypherDataKey* dstPlain, STNBX509* cert);
	BOOL TSCypher_genDataEnc(STTSCypherData* dstEnc, const STTSCypherDataKey* keyPlain, const void* plainData, const UI32 plainDataSz);
	BOOL TSCypher_genDataReEnc(STTSCypherData* dstEnc, const STTSCypherDataKey* keyPlainEnc, const STTSCypherData* srcEnc, const STTSCypherDataKey* srcKeyPlainDec);
	//Decode
	BOOL TSCypher_decDataKeyWithCurrentKey(STTSCypher* obj, STTSCypherDataKey* dstKeyPlain, const STTSCypherDataKey* keyEnc);
	BOOL TSCypher_encDataKey(STTSCypherDataKey* dstKeyEnc, const STTSCypherDataKey* keyPlain, STNBX509* cert);
	BOOL TSCypher_decDataKey(STTSCypherDataKey* dstKeyPlain, const STTSCypherDataKey* keyEnc, STNBPKey* key);
	BOOL TSCypher_genDataDec(STTSCypherData* dstDec, const STTSCypherDataKey* keyPlain, const void* encData, const UI32 encDataSz);
	BOOL TSCypher_genDataDecStr(STNBString* dstDec, const STTSCypherDataKey* keyPlain, const void* encData, const UI32 encDataSz);
	BOOL TSCypher_genFileDec(const STTSCypherDataKey* keyPlain, const char* inputFilepath, const SI64 pos, const SI64 sz, const char* outputFilepath);
	//
	void TSCypher_getPassword(STTSCypher* obj, STNBString* dst, const char* passName, const UI8 passLen, const BOOL creatIfNecesary);
	void TSCypher_genPassword(STNBString* dst, const UI8 passLen);
	void TSCypher_destroyString(STNBString* str);
	void TSCypher_destroyStr(char* str, const UI32 strSz);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCypher_h */
