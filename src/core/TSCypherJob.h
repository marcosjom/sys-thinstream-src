//
//  TSCypher.h
//  thinstream
//
//  Created by Marcos Ortega on 8/21/18.
//

#ifndef TSCypherJob_h
#define TSCypherJob_h

#include "nb/core/NBString.h"
#include "nb/core/NBArray.h"
#include "nb/core/NBThreadMutex.h"
#include "core/TSCypherData.h"

#ifdef __cplusplus
extern "C" {
#endif
	
	//CypherJob
	
	typedef enum ENTSCypherTaskType_ {
		ENTSCypherTaskType_Decrypt = 0,
		ENTSCypherTaskType_Encrpyt,
		ENTSCypherTaskType_Count
	} ENTSCypherTaskType;
	
	typedef enum ENTSCypherTaskDataType_ {
		ENTSCypherTaskDataType_Embedded = 0,
		ENTSCypherTaskDataType_FileRef,
		ENTSCypherTaskDataType_Count
	} ENTSCypherTaskDataType;
	
	typedef enum ENTSCypherJobState_ {
		ENTSCypherJobState_Iddle = 0,
		ENTSCypherJobState_Waiting,
		ENTSCypherJobState_Working,
		ENTSCypherJobState_Error,
		ENTSCypherJobState_Done,
		ENTSCypherJobState_Count
	} ENTSCypherJobState;
	
	typedef struct STTSCypherTaskData_ {
		STNBString				name;
		ENTSCypherTaskDataType	type;
		BOOL					processed;
		union {
			//Embedded data
			struct {
				STTSCypherData	input;
				STTSCypherData	output;
			} embedded;
			//File reference
			struct {
				STNBString		input;		//filepath
				SI64			inputPos;	//filepos
				SI64			inputSz;	//bytes
				STNBString		output;		//filepath
			} fileRef;
		};
	} STTSCypherTaskData;
	
	typedef struct STTSCypherTask_ {
		STNBString			name;
		ENTSCypherTaskType	type;
		STTSCypherDataKey	key;		//key
		STTSCypherDataKey	keyPlain;	//key
		STNBArray			blocks;	//STTSCypherTaskData data encoded with this key
	} STTSCypherTask;
	
	typedef struct STTSCypherJobRef_ {
		void*				opaque;
	} STTSCypherJobRef;
	
	//
	
	void TSCypherJob_init(STTSCypherJobRef* obj);
	void TSCypherJob_initWithOther(STTSCypherJobRef* obj, STTSCypherJobRef* other);
	void TSCypherJob_retain(STTSCypherJobRef* obj);
	void TSCypherJob_release(STTSCypherJobRef* obj);
	
	//
	BOOL TSCypherJob_isInvalidated(STTSCypherJobRef* obj);
	void TSCypherJob_invalidate(STTSCypherJobRef* obj);
	
	//Build
	ENTSCypherJobState TSCypherJob_getState(STTSCypherJobRef* obj);
	BOOL TSCypherJob_openTaskWithKey(STTSCypherJobRef* obj, const char* name, const ENTSCypherTaskType type, const STTSCypherDataKey* key);
	BOOL TSCypherJob_openTaskExisting(STTSCypherJobRef* obj, const char* name);
	BOOL TSCypherJob_addToOpenTaskEmbeddedData(STTSCypherJobRef* obj, const char* name, const STTSCypherData* data);
	BOOL TSCypherJob_addToOpenTaskFileRef(STTSCypherJobRef* obj, const char* name, const char* filename, const SI64 pos, const SI64 sz, const char* filePathOutput);
	//Read
	SI32 TSCypherJob_getSeqId(STTSCypherJobRef* obj);
	const STTSCypherTask* TSCypherJob_getTask(STTSCypherJobRef* obj, const char* taskName);
	const STTSCypherTaskData* TSCypherJob_getTaskData(STTSCypherJobRef* obj, const char* taskName, const char* dataName);
	//Tasks read
	void TSCypherJob_setState(STTSCypherJobRef* obj, const ENTSCypherJobState state);
	SI32 TSCypherJob_getTasksCount(STTSCypherJobRef* obj);
	STTSCypherTask* TSCypherJob_getTaskAtIdx(STTSCypherJobRef* obj, const SI32 idx);
	
#ifdef __cplusplus
} //extern "C"
#endif

#endif /* TSCypher_h */
