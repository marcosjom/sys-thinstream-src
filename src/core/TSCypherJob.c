//
//  TSCypher.c
//  thinstream
//
//  Created by Marcos Ortega on 8/21/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/TSCypherJob.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBString.h"
#include "nb/core/NBArray.h"
//#include "nb/crypto/NBAes256.h"
//#include "nb/crypto/NBPKey.h"
//#include "nb/crypto/NBX509.h"

typedef struct STTSCypherJobOpq_ {
	BOOL				isInvalidated;
	SI32				iCurTask; //current taks being filled
	SI32				seqId;	//sequenceof jobs added
	ENTSCypherJobState	state;
	STNBArray			tasks;	//STTSCypherTask
	STNBThreadMutex		mutex;
	SI32				retainCount;
} STTSCypherJobOpq;

void TSCypherJob_init(STTSCypherJobRef* obj){
	STTSCypherJobOpq* opq = obj->opaque = NBMemory_allocType(STTSCypherJobOpq);
	NBMemory_setZeroSt(*opq, STTSCypherJobOpq);
	//
	opq->state			= ENTSCypherJobState_Iddle;
	NBArray_init(&opq->tasks, sizeof(STTSCypherTask), NULL);
	NBThreadMutex_init(&opq->mutex);
	opq->retainCount	 = 1;
}

void TSCypherJob_initWithOther(STTSCypherJobRef* obj, STTSCypherJobRef* other){
	TSCypherJob_retain(other);
	obj->opaque = other->opaque;
}

void TSCypherJob_retain(STTSCypherJobRef* obj){
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		NBThreadMutex_lock(&opq->mutex);
		NBASSERT(opq->retainCount > 0)
		opq->retainCount++;
		NBThreadMutex_unlock(&opq->mutex);
	}
}

void TSCypherJob_release(STTSCypherJobRef* obj){
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		NBThreadMutex_lock(&opq->mutex);
		NBASSERT(opq->retainCount > 0)
		opq->retainCount--;
		if(opq->retainCount > 0){
			NBThreadMutex_unlock(&opq->mutex);
		} else {
			//Tasks
			{
				const STNBStructMap* keyMap = TSCypherDataKey_getSharedStructMap();
				const STNBStructMap* dataMap = TSCypherData_getSharedStructMap();
				SI32 i; for(i = 0; i < opq->tasks.use; i++){
					STTSCypherTask* t = NBArray_itmPtrAtIndex(&opq->tasks, STTSCypherTask, i);
					NBString_release(&t->name);
					NBStruct_stRelease(keyMap, &t->key, sizeof(t->key));
					NBStruct_stRelease(keyMap, &t->keyPlain, sizeof(t->keyPlain));
					{
						SI32 i; for(i = 0; i < t->blocks.use; i++){
							STTSCypherTaskData* data = NBArray_itmPtrAtIndex(&t->blocks, STTSCypherTaskData, i);
							NBString_release(&data->name);
							switch (data->type) {
								case ENTSCypherTaskDataType_Embedded:
									NBStruct_stRelease(dataMap, &data->embedded.input, sizeof(data->embedded.input));
									NBStruct_stRelease(dataMap, &data->embedded.output, sizeof(data->embedded.output));
									break;
								case ENTSCypherTaskDataType_FileRef:
									NBString_release(&data->fileRef.input);
									NBString_release(&data->fileRef.output);
									break;
								default:
									NBASSERT(FALSE)
									break;
							}
						}
						NBArray_empty(&t->blocks);
						NBArray_release(&t->blocks);
					}
				}
				NBArray_empty(&opq->tasks);
				NBArray_release(&opq->tasks);
			}
			NBThreadMutex_unlock(&opq->mutex);
			NBThreadMutex_release(&opq->mutex);
			//
			NBMemory_free(obj->opaque);
			obj->opaque = NULL;
		}
	}
}

BOOL TSCypherJob_isInvalidated(STTSCypherJobRef* obj){
	BOOL r = TRUE;
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		r = opq->isInvalidated;
	}
	return r;
}

void TSCypherJob_invalidate(STTSCypherJobRef* obj){
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		opq->isInvalidated = TRUE;
	}
}

//Build

ENTSCypherJobState TSCypherJob_getState(STTSCypherJobRef* obj){
	//No mutex
	ENTSCypherJobState r = ENTSCypherJobState_Error;
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		NBASSERT(opq->state >= 0 && opq->state < ENTSCypherJobState_Count)
		r = opq->state;
	}
	return r;
}

BOOL TSCypherJob_openTaskWithKey(STTSCypherJobRef* obj, const char* name, const ENTSCypherTaskType type, const STTSCypherDataKey* key){
	BOOL r = FALSE;
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		if(type >=0 && type < ENTSCypherTaskType_Count && key != NULL){
			NBThreadMutex_lock(&opq->mutex);
			NBASSERT(opq->state != ENTSCypherJobState_Waiting && opq->state != ENTSCypherJobState_Working)
			if(opq->state != ENTSCypherJobState_Waiting && opq->state != ENTSCypherJobState_Working){
				STTSCypherTask task;
				NBMemory_setZeroSt(task, STTSCypherTask);
				NBString_initWithStr(&task.name, name);
				task.type = type;
				NBStruct_stClone(TSCypherDataKey_getSharedStructMap(), key, sizeof(*key), &task.key, sizeof(task.key));
				NBArray_init(&task.blocks, sizeof(STTSCypherTaskData), NULL);
				NBArray_addValue(&opq->tasks, task);
				opq->iCurTask = (opq->tasks.use - 1);
				opq->seqId++;
				r = TRUE;
			}
			NBThreadMutex_unlock(&opq->mutex);
		}
	}
	return r;
}

BOOL TSCypherJob_openTaskExisting(STTSCypherJobRef* obj, const char* name){
	BOOL r = FALSE;
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		NBThreadMutex_lock(&opq->mutex);
		NBASSERT(opq->state != ENTSCypherJobState_Waiting && opq->state != ENTSCypherJobState_Working)
		if(opq->state != ENTSCypherJobState_Waiting && opq->state != ENTSCypherJobState_Working){
			SI32 i; for(i = 0 ; i < opq->tasks.use; i++){
				const STTSCypherTask* t = NBArray_itmPtrAtIndex(&opq->tasks, STTSCypherTask, i);
				if(NBString_strIsEqual(t->name.str, name)){
					opq->iCurTask = i;
					r = TRUE;
					break;
				}
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	return r;
}

BOOL TSCypherJob_addToOpenTaskEmbeddedData(STTSCypherJobRef* obj, const char* name, const STTSCypherData* data){
	BOOL r = FALSE;
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		if(data != NULL){
			NBThreadMutex_lock(&opq->mutex);
			NBASSERT(opq->state != ENTSCypherJobState_Waiting && opq->state != ENTSCypherJobState_Working)
			if(opq->state != ENTSCypherJobState_Waiting && opq->state != ENTSCypherJobState_Working){
				if(opq->tasks.use > 0){
					NBASSERT(opq->iCurTask >= 0 && opq->iCurTask < opq->tasks.use)
					STTSCypherTask* task = NBArray_itmPtrAtIndex(&opq->tasks, STTSCypherTask, opq->iCurTask);
					{
						STTSCypherTaskData dd;
						NBMemory_setZeroSt(dd, STTSCypherTaskData);
						dd.type = ENTSCypherTaskDataType_Embedded;
						NBString_initWithStr(&dd.name, name);
						//Embedded
						{
							NBStruct_stClone(TSCypherData_getSharedStructMap(), data, sizeof(*data), &dd.embedded.input, sizeof(dd.embedded.input));
						}
						opq->seqId++;
						NBArray_addValue(&task->blocks, dd);
					}
					r = TRUE;
				}
			}
			NBThreadMutex_unlock(&opq->mutex);
		}
	}
	return r;
}

BOOL TSCypherJob_addToOpenTaskFileRef(STTSCypherJobRef* obj, const char* name, const char* filename, const SI64 pos, const SI64 sz, const char* filePathOutput){
	BOOL r = FALSE;
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		if(!NBString_strIsEmpty(filename) && pos >= 0){
			NBThreadMutex_lock(&opq->mutex);
			NBASSERT(opq->state != ENTSCypherJobState_Waiting && opq->state != ENTSCypherJobState_Working)
			if(opq->state != ENTSCypherJobState_Waiting && opq->state != ENTSCypherJobState_Working){
				if(opq->tasks.use > 0){
					NBASSERT(opq->iCurTask >= 0 && opq->iCurTask < opq->tasks.use)
					STTSCypherTask* task = NBArray_itmPtrAtIndex(&opq->tasks, STTSCypherTask, opq->iCurTask);
					{
						STTSCypherTaskData dd;
						NBMemory_setZeroSt(dd, STTSCypherTaskData);
						dd.type = ENTSCypherTaskDataType_FileRef;
						NBString_initWithStr(&dd.name, name);
						//FileRef
						{
							NBString_initWithStr(&dd.fileRef.input, filename);
							NBString_initWithStr(&dd.fileRef.output, filePathOutput);
							dd.fileRef.inputPos = pos;
							dd.fileRef.inputSz	= sz;
						}
						opq->seqId++;
						NBArray_addValue(&task->blocks, dd);
					}
					r = TRUE;
				}
			}
			NBThreadMutex_unlock(&opq->mutex);
		}
	}
	return r;
}

//Read

SI32 TSCypherJob_getSeqId(STTSCypherJobRef* obj){
	SI32 r = 0;
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		r = opq->seqId;
	}
	return r;
}
	
const STTSCypherTask* TSCypherJob_getTask(STTSCypherJobRef* obj, const char* taskName){
	const STTSCypherTask* r = NULL;
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		NBThreadMutex_lock(&opq->mutex);
		NBASSERT(opq->state != ENTSCypherJobState_Waiting && opq->state != ENTSCypherJobState_Working)
		{
			SI32 i; for(i = 0; i < opq->tasks.use; i++){
				STTSCypherTask* t = NBArray_itmPtrAtIndex(&opq->tasks, STTSCypherTask, i);
				if(NBString_isEqual(&t->name, taskName)){
					r = t;
					break;
				}
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	return r;
}

const STTSCypherTaskData* TSCypherJob_getTaskData(STTSCypherJobRef* obj, const char* taskName, const char* dataName){
	const STTSCypherTaskData* r = NULL;
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		NBThreadMutex_lock(&opq->mutex);
		NBASSERT(opq->state != ENTSCypherJobState_Waiting && opq->state != ENTSCypherJobState_Working)
		{
			SI32 i; for(i = 0; i < opq->tasks.use; i++){
				STTSCypherTask* t = NBArray_itmPtrAtIndex(&opq->tasks, STTSCypherTask, i);
				if(NBString_isEqual(&t->name, taskName)){
					SI32 i; for(i = 0; i < t->blocks.use; i++){
						STTSCypherTaskData* data = NBArray_itmPtrAtIndex(&t->blocks, STTSCypherTaskData, i);
						if(NBString_isEqual(&data->name, dataName)){
							r = data;
							break;
						}
					}
					break;
				}
			}
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	return r;
}

//

void TSCypherJob_setState(STTSCypherJobRef* obj, const ENTSCypherJobState state){
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		NBASSERT(state >= 0 && state < ENTSCypherJobState_Count)
		NBThreadMutex_lock(&opq->mutex);
		opq->state = state;
		NBThreadMutex_unlock(&opq->mutex);
	}
}

SI32 TSCypherJob_getTasksCount(STTSCypherJobRef* obj){
	SI32 r = 0;
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		r = opq->tasks.use;
	}
	return r;
}

STTSCypherTask* TSCypherJob_getTaskAtIdx(STTSCypherJobRef* obj, const SI32 idx){
	STTSCypherTask* r = NULL;
	STTSCypherJobOpq* opq = (STTSCypherJobOpq*)obj->opaque;
	if(opq != NULL){
		if(idx >= 0 && idx < opq->tasks.use){
			r = NBArray_itmPtrAtIndex(&opq->tasks, STTSCypherTask, idx);
		}
	}
	return r;
}
