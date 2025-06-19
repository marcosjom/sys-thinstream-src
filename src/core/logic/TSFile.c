//
//  TSClinicReq.c
//  thinstream
//
//  Created by Marcos Ortega on 2/3/19.
//

#include "nb/NBFrameworkPch.h"
#include "core/logic/TSFile.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBMngrStructMaps.h"

//
//File fingerprint
//

STNBStructMapsRec TSFileFingerprint_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSFileFingerprint_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSFileFingerprint_sharedStructMap);
	if(TSFileFingerprint_sharedStructMap.map == NULL){
		STTSFileFingerprint s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSFileFingerprint);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addUIntM(map, s, lasTimeModified);	//file modification
		NBStructMap_addUIntM(map, s, bytesCount);		//binary size
		NBStructMap_addStrPtrM(map, s, hash);			//content's hash (sha1)
		//
		TSFileFingerprint_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSFileFingerprint_sharedStructMap);
	return TSFileFingerprint_sharedStructMap.map;
}

//
//File data
//

STNBStructMapsRec TSFileData_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSFileData_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSFileData_sharedStructMap);
	if(TSFileData_sharedStructMap.map == NULL){
		STTSFileData s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSFileData);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addPtrToArrayOfBytesM(map, s, content, contentSz, ENNBStructMapSign_Unsigned);
		NBStructMap_addStructPtrM(map, s, fingerprint, TSFileFingerprint_getSharedStructMap());
		//
		TSFileData_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSFileData_sharedStructMap);
	return TSFileData_sharedStructMap.map;
}

//
//File path
//

STNBStructMapsRec TSFilePath_sharedStructMap = STNBStructMapsRec_empty;

const STNBStructMap* TSFilePath_getSharedStructMap(void){
	NBMngrStructMaps_lock(&TSFilePath_sharedStructMap);
	if(TSFilePath_sharedStructMap.map == NULL){
		STTSFilePath s;
		STNBStructMap* map = NBMngrStructMaps_allocTypeM(STTSFilePath);
		NBStructMap_init(map, sizeof(s));
		NBStructMap_addStrPtrM(map, s, path);
		NBStructMap_addStructPtrM(map, s, fingerprint, TSFileFingerprint_getSharedStructMap());
		//
		TSFilePath_sharedStructMap.map = map;
	}
	NBMngrStructMaps_unlock(&TSFilePath_sharedStructMap);
	return TSFilePath_sharedStructMap.map;
}


//File

void TSFile_init(STTSFile* obj){
	NBMemory_setZeroSt(*obj, STTSFile);
	obj->props.count = 1;
	//NBThreadMutex_init(&obj->mutex);
}

void TSFile_release(STTSFile* obj){
	//NBThreadMutex_lock(&obj->mutex);
	if(obj->name != NULL){
		NBMemory_free(obj->name);
		obj->name = NULL;
	}
	if(obj->subfiles != NULL){
		SI32 i; for(i = 0; i < obj->subfiles->use; i++){
			STTSFile* sub = NBArray_itemAtIndex(obj->subfiles, i);
			TSFile_release(sub);
		}
		NBArray_empty(obj->subfiles);
		NBArray_release(obj->subfiles);
		NBMemory_free(obj->subfiles);
		obj->subfiles = NULL;
	}
	//NBThreadMutex_unlock(&obj->mutex);
	//NBThreadMutex_release(&obj->mutex);
}

void TSFile_setName(STTSFile* obj, const char* name, const ENTSFileType type, const SI64 fileSz){
	//NBThreadMutex_lock(&obj->mutex);
	{
		obj->props.type			= type;
		obj->props.bytesCount	= fileSz;
		NBString_strFreeAndNewBuffer(&obj->name, name);
	}
	//NBThreadMutex_unlock(&obj->mutex);
}

STTSFile* TSFile_addSubfile(STTSFile* obj, const char* name, const ENTSFileType type, const SI64 fileSz){
	STTSFile* r = NULL;
	//NBThreadMutex_lock(&obj->mutex);
	{
		if(obj->subfiles == NULL){
			obj->subfiles = NBMemory_allocType(STNBArray);
			NBArray_init(obj->subfiles, sizeof(STTSFile), NULL);
		}
		{
			STTSFile sub;
			TSFile_init(&sub);
			TSFile_setName(&sub, name, type, fileSz);
			r = (STTSFile*)NBArray_addValue(obj->subfiles, sub);
		}
	}
	//NBThreadMutex_unlock(&obj->mutex);
	return r;
}

STTSFile* TSFile_addSubfileObj(STTSFile* obj, const STTSFile* sub){
	STTSFile* r = NULL;
	//NBThreadMutex_lock(&obj->mutex);
	if(sub != NULL){
		if(obj->subfiles == NULL){
			obj->subfiles = NBMemory_allocType(STNBArray);
			NBArray_init(obj->subfiles, sizeof(STTSFile), NULL);
		}
		r = (STTSFile*)NBArray_addValue(obj->subfiles, *sub);
	}
	//NBThreadMutex_unlock(&obj->mutex);
	return r;
}

STTSFile* TSFile_getSubfile(STTSFile* obj, const char* name){
	STTSFile* r = NULL;
	//NBThreadMutex_lock(&obj->mutex);
	if(obj->subfiles != NULL){
		SI32 i; for(i = 0; i < obj->subfiles->use; i++){
			STTSFile* sub = NBArray_itemAtIndex(obj->subfiles, i);
			if(NBString_strIsEqual(sub->name, name)){
				r = sub;
				break;
			}
		}
	}
	//NBThreadMutex_unlock(&obj->mutex);
	return r;
}

STTSFileProps TSFile_removeSubfile(STTSFile* obj, const char* name){
	STTSFileProps r;
	NBMemory_setZeroSt(r, STTSFileProps);
	//NBThreadMutex_lock(&obj->mutex);
	if(obj->subfiles != NULL){
		SI32 i; for(i = 0; i < obj->subfiles->use; i++){
			STTSFile* sub = NBArray_itemAtIndex(obj->subfiles, i);
			if(NBString_strIsEqual(sub->name, name)){
				//PRINTF_INFO("TSFile, removing '%s'.\n", name);
				r = sub->props; NBASSERT(r.count > 0)
				TSFile_release(sub);
				NBArray_removeItemAtIndex(obj->subfiles, i);
				break;
			}
		}
	}
	//NBThreadMutex_unlock(&obj->mutex);
	return r;
}

STTSFileProps TSFile_getRecursiveSubfilesCount(STTSFile* obj, const ENTSFileType typeFilter){
	STTSFileProps r;
	NBMemory_setZeroSt(r, STTSFileProps);
	//NBThreadMutex_lock(&obj->mutex);
	if(obj->subfiles != NULL){
		SI32 i; for(i = 0; i < obj->subfiles->use; i++){
			STTSFile* sub = NBArray_itemAtIndex(obj->subfiles, i);
			const STTSFileProps subProps = TSFile_getRecursiveSubfilesCount(sub, typeFilter);
			r.count			+= subProps.count;
			r.bytesCount	+= subProps.bytesCount;
			if(typeFilter == ENTSFileType_Count || sub->props.type == typeFilter){
				r.count			+= sub->props.count;
				r.bytesCount	+= sub->props.bytesCount;
			}
		}
	}
	//NBThreadMutex_unlock(&obj->mutex);
	return r;
}

UI32 TSFile_concatTreeRecursiveChildren_(STTSFile* obj, const char* prefixChild, const char* prefixGrandchild, const BOOL ignoreEmptyFolders, const BOOL ignoreEmptyFiles, STNBString* dst){
    UI32 r = 0;
    if(obj->subfiles != NULL){
        SI32 i; for(i = 0; i < obj->subfiles->use; i++){
            STTSFile* sub = NBArray_itemAtIndex(obj->subfiles, i);
            if(sub->props.type == ENTSFileType_Folder){
                //folder
                if((sub->subfiles != NULL && sub->subfiles->use > 0) || !ignoreEmptyFolders){
                    UI32 rr = 0;
                    const SI32 lenBefore = dst->length;
                    NBString_concat(dst, prefixChild);
                    NBString_concat(dst, sub->name);
                    NBString_concatByte(dst, '\n');
                    if(sub->subfiles != NULL){
                        STNBString prefix2, prefix3;
                        NBString_init(&prefix2);
                        NBString_init(&prefix3);
                        NBString_concat(&prefix2, prefixGrandchild);
                        NBString_concat(&prefix2, "|- ");
                        NBString_concat(&prefix3, prefixGrandchild);
                        NBString_concat(&prefix3, "|  ");
                        {
                            const SI32 lenBefore = dst->length;
                            const UI32 addedCount = TSFile_concatTreeRecursiveChildren_(sub, prefix2.str, prefix3.str, ignoreEmptyFolders, ignoreEmptyFiles, dst);
                            if(addedCount == 0){
                                NBString_truncate(dst, lenBefore);
                            } else {
                                rr++;
                            }
                        }
                        NBString_release(&prefix2);
                    }
                    if(rr == 0){
                        NBString_truncate(dst, lenBefore);
                    } else {
                        r++;
                    }
                }
            } else {
                //file
                if(sub->props.bytesCount > 0 || !ignoreEmptyFiles){
                    NBString_concat(dst, prefixChild);
                    NBString_concat(dst, sub->name);
                    NBString_concatByte(dst, '\n');
                    r++;
                }
            }
        }
    }
    return r;
}
    
void TSFile_concatTreeRecursive(STTSFile* obj, const BOOL ignoreEmptyFolders, const BOOL ignoreEmptyFiles, STNBString* dst){
    if(dst != NULL){
        NBString_concat(dst, obj->name);
        NBString_concatByte(dst, '\n');
        if(obj->subfiles != NULL){
            STNBString prefix, prefix2;
            NBString_init(&prefix);
            NBString_init(&prefix2);
            NBString_concat(&prefix, "|- ");
            NBString_concat(&prefix2, "|  ");
            {
                const SI32 lenBefore = dst->length;
                const UI32 addedCount = TSFile_concatTreeRecursiveChildren_(obj, prefix.str, prefix2.str, ignoreEmptyFolders, ignoreEmptyFiles, dst);
                if(addedCount == 0){
                    NBString_truncate(dst, lenBefore);
                }
            }
            NBString_release(&prefix);
            NBString_release(&prefix2);
        }
    }
}
