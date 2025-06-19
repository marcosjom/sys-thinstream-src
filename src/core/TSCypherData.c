//
//  TSCypher.c
//  thinstream
//
//  Created by Marcos Ortega on 8/21/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/TSCypherData.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"

//CypherData Key

STNBStructMapsRec TSCypherDataKey_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCypherDataKey_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCypherDataKey_sharedStructMap);
	if(TSCypherDataKey_sharedStructMap.map == NULL){
		STTSCypherDataKey s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCypherDataKey);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addPtrToArrayOfBytesM(map, s, pass, passSz, ENNBStructMapSign_Unsigned);
		NBStructMap_addPtrToArrayOfBytesM(map, s, salt, saltSz, ENNBStructMapSign_Unsigned);
		NBStructMap_addUIntM(map, s, iterations);
		TSCypherDataKey_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCypherDataKey_sharedStructMap);
	return TSCypherDataKey_sharedStructMap.map;
}

BOOL TSCypherDataKey_isEmpty(const STTSCypherDataKey* key){
	return (key->pass == NULL || key->passSz <= 0 || key->salt == NULL || key->saltSz <= 0 || key->iterations <= 0);
}

//CypherData Payload

STNBStructMapsRec TSCypherData_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSCypherData_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSCypherData_sharedStructMap);
	if(TSCypherData_sharedStructMap.map == NULL){
		STTSCypherData s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSCypherData);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addPtrToArrayOfBytesM(map, s, data, dataSz, ENNBStructMapSign_Unsigned);
		TSCypherData_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSCypherData_sharedStructMap);
	return TSCypherData_sharedStructMap.map;
}
