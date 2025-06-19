//
//  TSCertificate.h
//  thinstream
//
//  Created by Marcos Ortega on 5/3/19.
//

#ifndef TSCertificate_h
#define TSCertificate_h

#include "nb/core/NBStructMap.h"
#include "core/TSContext.h"

#ifdef __cplusplus
extern "C" {
#endif

	//Types of mailboxes
	
	typedef enum ENTSCertificateType_ {
		ENTSCertificateType_Device = 0,
		ENTSCertificateType_Identity,	//ToDo: remove
		ENTSCertificateType_Count
	} ENTSCertificateType;
	
	const STNBEnumMap* ENTSCertificateType_getSharedEnumMap(void);
	
	//
	//Certificate
	//
	
	typedef struct STTSCertificate_ {
		ENTSCertificateType type;
		BYTE*				p12;		//pkey and cert
		UI32				p12Sz;
		BYTE*				cert;		//cert only
		UI32				certSz;
	} STTSCertificate;
	
	const STNBStructMap* TSCertificate_getSharedStructMap(void);
	
	//
	// Request PKey and Cert
	//
	SI32 TSCertificate_getCert(STTSContext* ctx, const char* rootPath, const char* relPath, const ENTSCertificateType type, const char* serial, STNBX509* dstCert);
	SI32 TSCertificate_getPKeyAndCert(STTSContext* ctx, const char* rootPath, const char* relPath, const ENTSCertificateType type, const char* serial, STNBPKey* dstKey, STNBX509* dstCert, const char* pass);
	SI32 TSCertificate_hasPKey(STTSContext* ctx, const char* rootPath, const char* relPath, const ENTSCertificateType type, const char* serial);
	
#ifdef __cplusplus
} //extern "C" {
#endif


#endif /* TSCertificate_h */
