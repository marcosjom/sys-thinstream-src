//
//  TSFile.h
//  thinstream
//
//  Created by Marcos Ortega on 2/3/19.
//

#ifndef TSFile_h
#define TSFile_h

#include "nb/core/NBString.h"
#include "nb/core/NBStructMap.h"
#include "nb/core/NBArray.h"
#include "nb/core/NBThreadMutex.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//
	//File fingerprint
	//

	typedef struct STTSFileFingerprint_ {
		UI32		bytesCount;			//binary size (zero if not set)
		UI64		lasTimeModified;	//file modification  (zero if not set)
		char*		hash;				//content's hash (sha1) (NULL if not set)
	} STTSFileFingerprint;

	const STNBStructMap* TSFileFingerprint_getSharedStructMap(void);

	//
	//File data
	//

	typedef struct STTSFileData_ {
		BYTE*					content;
		UI32					contentSz;
		STTSFileFingerprint*	fingerprint;	//content's fingerprint (NULL if not set)
	} STTSFileData;

	const STNBStructMap* TSFileData_getSharedStructMap(void);

	//
	//File path (reference)
	//

	typedef struct STTSFilePath_ {
		char*					path;
		STTSFileFingerprint*	fingerprint;	//content's fingerprint (NULL if not set)
	} STTSFilePath;

	const STNBStructMap* TSFilePath_getSharedStructMap(void);


	//File

	typedef enum ENTSFileType_ {
		ENTSFileType_File = 0,
		ENTSFileType_Folder,
		ENTSFileType_Count
	} ENTSFileType;

	typedef struct STTSFileProps_ {
		ENTSFileType	type;
		UI32			count;
		SI64			bytesCount;	//fileSz
	} STTSFileProps;

	typedef struct STTSFile_ {
		char*			name;		//filename
		STTSFileProps	props;		//props
		STNBArray*		subfiles;	//STTSFile, subfiles
		//STNBThreadMutex	mutex;
	} STTSFile;

	void			TSFile_init(STTSFile* obj);
	void			TSFile_release(STTSFile* obj);
	//
	void			TSFile_setName(STTSFile* obj, const char* name, const ENTSFileType type, const SI64 fileSz);
	STTSFile*		TSFile_addSubfile(STTSFile* obj, const char* name, const ENTSFileType type, const SI64 fileSz);
	STTSFile*		TSFile_addSubfileObj(STTSFile* obj, const STTSFile* sub);
	STTSFile*		TSFile_getSubfile(STTSFile* obj, const char* name);
	STTSFile*		TSFile_getSubfileAtIdx(STTSFile* obj, const SI32 idx);
	STTSFileProps	TSFile_removeSubfile(STTSFile* obj, const char* name);
	STTSFileProps	TSFile_getRecursiveSubfilesCount(STTSFile* obj, const ENTSFileType typeFilter);
    //
    void            TSFile_concatTreeRecursive(STTSFile* obj, const BOOL ignoreEmptyFolders, const BOOL ignoreEmptyFiles, STNBString* dst);

#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSClinicReq_h */

