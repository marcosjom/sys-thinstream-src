//
//  TSCertificate.c
//  thinstream
//
//  Created by Marcos Ortega on 5/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSCertificate.h"
//
#include "nb/core/NBMemory.h"
#include "nb/crypto/NBPkcs12.h"
#include "nb/core/NBMngrStructMaps.h"

//Types of certificates

STNBEnumMapRecord TSCertificateType_sharedEnumMapRecs[] = {
	{ ENTSCertificateType_Device, "ENTSCertificateType_Device", "device" }
	, { ENTSCertificateType_Identity, "ENTSCertificateType_Identity", "identity" }
};

STNBEnumMap TSCertificateType_sharedEnumMap = {
	"ENTSCertificateType"
	, TSCertificateType_sharedEnumMapRecs
	, (sizeof(TSCertificateType_sharedEnumMapRecs) / sizeof(TSCertificateType_sharedEnumMapRecs[0]))
};

const STNBEnumMap* ENTSCertificateType_getSharedEnumMap(void){
	return &TSCertificateType_sharedEnumMap;
}

//
//Certificate
//

STNBStructMapsRec TSCertificate_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCertificate_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCertificate_sharedStructMap);
	if(TSCertificate_sharedStructMap.map == NULL){
		STTSCertificate s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCertificate);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addEnumM(map, s, type, ENTSCertificateType_getSharedEnumMap());
		NBStructMap_addPtrToArrayOfBytesM(map, s, p12, p12Sz, ENNBStructMapSign_Unsigned);
		NBStructMap_addPtrToArrayOfBytesM(map, s, cert, certSz, ENNBStructMapSign_Unsigned);
		TSCertificate_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCertificate_sharedStructMap);
	return TSCertificate_sharedStructMap.map;
}

//
// Request PKey and Cert
//

SI32 TSCertificate_getCert(STTSContext* ctx, const char* rootPath, const char* relPath, const ENTSCertificateType type, const char* serial, STNBX509* dstCert){
	BOOL r = FALSE;
	//Load certificate
	{
		STNBStorageRecordRead ref = TSContext_getStorageRecordForRead(ctx, rootPath, relPath, NULL, TSCertificate_getSharedStructMap());
		if(ref.data != NULL){
			STTSCertificate* priv = (STTSCertificate*)ref.data->priv.stData;
			if(priv->type != type){
				PRINTF_CONSOLE_ERROR("Certificate type does not match; found(%d) srching(%d).\n", priv->type, type);
			} else if(priv->cert == NULL || priv->certSz <= 0){
				PRINTF_CONSOLE_ERROR("Certificate-record does not have cert-data\n");
			} else {
				if(!NBX509_createFromDERBytes(dstCert, priv->cert, priv->certSz)){
					PRINTF_CONSOLE_ERROR("Certificate could not be parsed.\n");
				} else {
					//PRINTF_INFO("Certificate loaded.\n");
					r = TRUE;
				}
			}
		}
		TSContext_returnStorageRecordFromRead(ctx, &ref);
	}
	return r;
}

SI32 TSCertificate_getPKeyAndCert(STTSContext* ctx, const char* rootPath, const char* relPath, const ENTSCertificateType type, const char* serial, STNBPKey* dstKey, STNBX509* dstCert, const char* pass){
	BOOL r = FALSE;
	//Load certificate
	{
		STNBStorageRecordRead ref = TSContext_getStorageRecordForRead(ctx, rootPath, relPath, NULL, TSCertificate_getSharedStructMap());
		if(ref.data != NULL){
			STTSCertificate* priv = (STTSCertificate*)ref.data->priv.stData;
			if(priv->type != type){
				PRINTF_CONSOLE_ERROR("Certificate type does not match; found(%d) srching(%d).\n", priv->type, type);
			} else if(priv->p12 == NULL || priv->p12Sz <= 0){
				PRINTF_CONSOLE_ERROR("Certificate-record does not have p12-data\n");
			} else {
				STNBPkcs12 p12;
				NBPkcs12_init(&p12);
				if(!NBPkcs12_createFromDERBytes(&p12, priv->p12, priv->p12Sz)){
					PRINTF_CONSOLE_ERROR("Certificate p12 could not be parsed.\n");
				} else if(!NBPkcs12_getCertAndKey(&p12, dstKey, dstCert, pass)){
					PRINTF_CONSOLE_ERROR("Certificate p12 could not extract pkey/cert with the specified password.\n");
				} else {
					//PRINTF_INFO("Certificate p12, pkey and cert extracted.\n");
					r = TRUE;
				}
				NBPkcs12_release(&p12);
			}
		}
		TSContext_returnStorageRecordFromRead(ctx, &ref);
	}
	return r;
}

SI32 TSCertificate_hasPKey(STTSContext* ctx, const char* rootPath, const char* relPath, const ENTSCertificateType type, const char* serial){
	BOOL r = FALSE;
	//Load certificate
	{
		STNBStorageRecordRead ref = TSContext_getStorageRecordForRead(ctx, rootPath, relPath, NULL, TSCertificate_getSharedStructMap());
		if(ref.data != NULL){
			STTSCertificate* priv = (STTSCertificate*)ref.data->priv.stData;
			if(priv->type != type){
				PRINTF_CONSOLE_ERROR("Certificate type does not match; found(%d) srching(%d).\n", priv->type, type);
			} else if(priv->p12 != NULL && priv->p12Sz > 0){
				r = TRUE;
			}
		}
		TSContext_returnStorageRecordFromRead(ctx, &ref);
	}
	return r;
}
