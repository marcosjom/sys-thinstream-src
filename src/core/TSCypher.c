//
//  TSCypher.c
//  thinstream
//
//  Created by Marcos Ortega on 8/21/18.
//

#include "nb/NBFrameworkPch.h"
#include "core/TSCypher.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBStruct.h"
#include "nb/core/NBString.h"
#include "nb/core/NBArray.h"
#include "nb/core/NBThread.h"
#include "nb/core/NBThreadCond.h"
#include "nb/crypto/NBAes256.h"
#include "nb/crypto/NBPKey.h"
#include "nb/crypto/NBX509.h"
#include "nb/crypto/NBPkcs12.h"
#include "nb/crypto/NBKeychain.h"
#if defined(__APPLE__)
#include "nb/crypto/NBKeychainApple.h"
#endif
//
#include <stdlib.h>	//for rand()

//

typedef struct STTSCypherSslContext_ {
	STNBSslContextRef	ctxt;
	STNBString			userId;
} STTSCypherSslContext;

typedef struct STTSCypherKeyCacheItm_ {
	STTSCypherDataKey	enc;	//encoded
	STTSCypherDataKey	dec;	//decoded
} STTSCypherKeyCacheItm;

typedef struct STTSCypherOpq_ {
	STTSCfgCypher		cfg;
	STTSContext*	context;
	STNBKeychain		keychain;
	STNBThreadMutex		mutex;
	STNBX509*			caCert;
	struct {
		STNBX509*			cert;	//for encryption
		STNBPKey*			key;	//for local decryption
		STTSCypherDataKey	sharedKeyPlain;	//for quick encryption
		STTSCypherDataKey	sharedKeyEnc;	//for quick encryption
	} my;
	STNBArray			sslContexts;	//STTSCypherSslContext*
	struct {
		STNBArray		jobs;			//STTSCypherJobRef*
		STNBArray		keysCache;		//STTSCypherKeyCacheItm (last used)
		STNBThreadMutex	mutex;
		STNBThreadCond	cond;
		BOOL			stopFlag;		//cypher thread
		SI32			threadsRunning;	//cypher thread
	} thread;
} STTSCypherOpq;

void TSCypher_init(STTSCypher* obj){
	STTSCypherOpq* opq = obj->opaque = NBMemory_allocType(STTSCypherOpq);
	NBMemory_setZeroSt(*opq, STTSCypherOpq);
	opq->context	= NULL;
	NBThreadMutex_init(&opq->mutex);
	NBKeychain_init(&opq->keychain);
#	if defined(__APPLE__)
	{
		STNBKeychainItf itf = NBKeychainApple_getItf();
		NBKeychain_setItf(&opq->keychain, &itf);
	}
#	endif
	opq->caCert		= NULL;
	{
		opq->my.cert	= NULL;
		opq->my.key		= NULL;
		NBMemory_setZero(opq->my.sharedKeyPlain);
		NBMemory_setZero(opq->my.sharedKeyEnc);
	}
	NBArray_init(&opq->sslContexts, sizeof(STTSCypherSslContext*), NULL);
	//Jobs
	{
		NBArray_init(&opq->thread.jobs, sizeof(STTSCypherJobRef*), NULL);
		NBArray_init(&opq->thread.keysCache, sizeof(STTSCypherKeyCacheItm), NULL);
		NBThreadMutex_init(&opq->thread.mutex);
		NBThreadCond_init(&opq->thread.cond);
		opq->thread.stopFlag		= FALSE;
		opq->thread.threadsRunning	= 0;
	}
}

void TSCypher_release(STTSCypher* obj){
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	{
		//Wait for jobs
		{
			//PRINTF_INFO("TSCypher_release, (lock-on).\n");
			NBThreadMutex_lock(&opq->thread.mutex);
			while(opq->thread.threadsRunning > 0){
				{
					opq->thread.stopFlag = TRUE;
					NBThreadCond_broadcast(&opq->thread.cond);
				}
				NBThreadCond_wait(&opq->thread.cond, &opq->thread.mutex);
			}
			//Release all pending jobs
			{
				SI32 i; for(i = 0; i < opq->thread.jobs.use; i++){
					STTSCypherJobRef* j = NBArray_itmValueAtIndex(&opq->thread.jobs, STTSCypherJobRef*, i);
					TSCypherJob_invalidate(j);
					TSCypherJob_release(j);
					NBMemory_free(j);
				}
				NBArray_empty(&opq->thread.jobs);
				NBArray_release(&opq->thread.jobs);
			}
			//Release keys cache
			{
				const STNBStructMap* keyMap = TSCypherDataKey_getSharedStructMap();
				SI32 i; for(i = 0; i < opq->thread.keysCache.use; i++){
					STTSCypherKeyCacheItm* c = NBArray_itmPtrAtIndex(&opq->thread.keysCache, STTSCypherKeyCacheItm, i);
					NBStruct_stRelease(keyMap, &c->enc, sizeof(c->enc));
					NBStruct_stRelease(keyMap, &c->dec, sizeof(c->dec));
				}
				NBArray_empty(&opq->thread.keysCache);
				NBArray_release(&opq->thread.keysCache);
			}
			//PRINTF_INFO("TSCypher_release, (lock-off).\n");
			NBThreadMutex_unlock(&opq->thread.mutex);
			NBThreadMutex_release(&opq->thread.mutex);
			opq->thread.stopFlag = FALSE;
		}
		NBStruct_stRelease(TSCfgCypher_getSharedStructMap(), &opq->cfg, sizeof(opq->cfg));
		opq->context	= NULL;
		NBKeychain_release(&opq->keychain);
		opq->caCert = NULL;
		{
			if(opq->my.key != NULL){
				NBPKey_release(opq->my.key);
				NBMemory_free(opq->my.key);
				opq->my.key = NULL;
			}
			if(opq->my.cert != NULL){
				NBX509_release(opq->my.cert);
				NBMemory_free(opq->my.cert);
				opq->my.cert = NULL;
			}
			NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &opq->my.sharedKeyPlain, sizeof(opq->my.sharedKeyPlain));
			NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &opq->my.sharedKeyEnc, sizeof(opq->my.sharedKeyEnc));
		}
		{
			UI32 i; for(i = 0; i < opq->sslContexts.use; i++){
				STTSCypherSslContext* ss = NBArray_itmValueAtIndex(&opq->sslContexts, STTSCypherSslContext*, i);
				NBSslContext_release(&ss->ctxt);
				NBString_release(&ss->userId);
				NBMemory_free(ss);
			}
			NBArray_empty(&opq->sslContexts);
			NBArray_release(&opq->sslContexts);
		}
	}
	NBThreadMutex_unlock(&opq->mutex);
	NBThreadMutex_release(&opq->mutex);
	NBMemory_free(obj->opaque);
	obj->opaque = NULL;
}

BOOL TSCypher_loadFromConfig(STTSCypher* obj, STTSContext* context, STNBX509* caCert, const STTSCfgCypher* cfg){
	BOOL r = TRUE;
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	if(!NBStruct_stClone(TSCfgCypher_getSharedStructMap(), cfg, sizeof(*cfg), &opq->cfg, sizeof(opq->cfg))){
		r = FALSE;
	}
	if(cfg->passLen < 6){
		PRINTF_CONSOLE_ERROR("Passwords cannot be shorter than 6 chars.\n");
		r = FALSE;
	}
	if(r){
		NBKeychain_setFilesystem(&opq->keychain, TSContext_getFilesystem(context));
		opq->context = context;
		opq->caCert = caCert;
	}
	return r;
}

//Decode

BOOL TSCypher_decDataKeyWithCurrentKeyOpq(STTSCypherOpq* opq, STTSCypherDataKey* dstKeyPlain, const STTSCypherDataKey* keyEnc){
	BOOL r = FALSE;
	if(opq != NULL){
		if(opq->my.key != NULL){
			//Local decryption
			r = TSCypher_decDataKey(dstKeyPlain, keyEnc, opq->my.key);
		}
	}
	return r;
}

BOOL TSCypher_decDataKeyWithCurrentKey(STTSCypher* obj, STTSCypherDataKey* dstKeyPlain, const STTSCypherDataKey* keyEnc){
	BOOL r = FALSE;
	if(obj != NULL){
		STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
		if(opq != NULL){
			r = TSCypher_decDataKeyWithCurrentKeyOpq(opq, dstKeyPlain, keyEnc);
		}
	}
	return r;
}

//Jobs

typedef enum ENTSCypherKeyCacheState_ {
	ENTSCypherKeyCacheState_NotFound = 0,	//Key is not at cache
	ENTSCypherKeyCacheState_FoundPending,	//Key is at cache, but not decrypted yet
	ENTSCypherKeyCacheState_FoundDecrypted,	//Key is at cache, already decrypted
	ENTSCypherKeyCacheState_Count
} ENTSCypherKeyCacheState;

ENTSCypherKeyCacheState TSCypher_loadKeyFromCacheLocked_(STTSCypherOpq* opq, STTSCypherTask* t){
	ENTSCypherKeyCacheState r = ENTSCypherKeyCacheState_NotFound;
	SI32 i; for(i = 0; i < opq->thread.keysCache.use; i++){
		STTSCypherKeyCacheItm* c = NBArray_itmPtrAtIndex(&opq->thread.keysCache, STTSCypherKeyCacheItm, i);
		if(c->enc.iterations == t->key.iterations && c->enc.passSz == t->key.passSz && c->enc.saltSz == t->key.saltSz){
			if(NBString_strIsEqualStrBytes((const char*)t->key.pass, (const char*)c->enc.pass, c->enc.passSz)){
				if(NBString_strIsEqualStrBytes((const char*)t->key.salt, (const char*)c->enc.salt, c->enc.saltSz)){
					if(TSCypherDataKey_isEmpty(&c->dec)){
						r = ENTSCypherKeyCacheState_FoundPending;
						//PRINTF_INFO("CypherThread, key found at cache (pending to decrypt).\n");
					} else {
						r = ENTSCypherKeyCacheState_FoundDecrypted;
						//Set
						const STNBStructMap* keyMap = TSCypherDataKey_getSharedStructMap();
						NBStruct_stRelease(keyMap, &t->keyPlain, sizeof(t->keyPlain));
						NBStruct_stClone(keyMap, &c->dec, sizeof(c->dec), &t->keyPlain, sizeof(t->keyPlain));
						//Move key to first position (idx zero)
						if(i > 0){
							const STTSCypherKeyCacheItm cc = *c;
							NBArray_removeItemAtIndex(&opq->thread.keysCache, i);
							NBArray_addItemsAtIndex(&opq->thread.keysCache, 0, &cc, sizeof(cc), 1);
						}
						//PRINTF_INFO("CypherThread, key found at cache (already decrypted).\n");
					}
					break;
				}
			}
		}
	}
	return r;
}

BOOL TSCypher_addKeyToCacheLocked_(STTSCypherOpq* opq, STTSCypherTask* t){
	const STNBStructMap* keyMap = TSCypherDataKey_getSharedStructMap();
	//Add key to first position (idx zero)
	{
		STTSCypherKeyCacheItm c;
		NBMemory_setZeroSt(c, STTSCypherKeyCacheItm);
		NBStruct_stClone(keyMap, &t->key, sizeof(t->key), &c.enc, sizeof(c.enc));
		NBStruct_stClone(keyMap, &t->keyPlain, sizeof(t->keyPlain), &c.dec, sizeof(c.dec));
		NBArray_addItemsAtIndex(&opq->thread.keysCache, 0, &c, sizeof(c), 1);
		//PRINTF_INFO("CypherThread, key pushed at cache (%d in total).\n", opq->thread.keysCache.use);
	}
	//Remove extra keys (at higher index)
	while(opq->thread.keysCache.use > 0 && opq->thread.keysCache.use > opq->cfg.keysCacheSz){
		STTSCypherKeyCacheItm* c = NBArray_itmPtrAtIndex(&opq->thread.keysCache, STTSCypherKeyCacheItm, opq->thread.keysCache.use - 1);
		NBStruct_stRelease(keyMap, &c->enc, sizeof(c->enc));
		NBStruct_stRelease(keyMap, &c->dec, sizeof(c->dec));
		NBArray_removeItemAtIndex(&opq->thread.keysCache, opq->thread.keysCache.use - 1);
		//PRINTF_INFO("CypherThread, key popped from cache (%d in total).\n", opq->thread.keysCache.use);
	}
	return TRUE;
}

BOOL TSCypher_updateOrRemoveKeyAtCacheLocked_(STTSCypherOpq* opq, STTSCypherTask* t){
	BOOL r = FALSE;
	//Search key in array
	SI32 i; for(i = 0; i < opq->thread.keysCache.use; i++){
		STTSCypherKeyCacheItm* c = NBArray_itmPtrAtIndex(&opq->thread.keysCache, STTSCypherKeyCacheItm, i);
		if(c->enc.iterations == t->key.iterations && c->enc.passSz == t->key.passSz && c->enc.saltSz == t->key.saltSz){
			if(NBString_strIsEqualStrBytes((const char*)t->key.pass, (const char*)c->enc.pass, c->enc.passSz)){
				if(NBString_strIsEqualStrBytes((const char*)t->key.salt, (const char*)c->enc.salt, c->enc.saltSz)){
					const STNBStructMap* keyMap = TSCypherDataKey_getSharedStructMap();
					if(TSCypherDataKey_isEmpty(&t->keyPlain)){
						//Remove key from cache
						NBStruct_stRelease(keyMap, &c->enc, sizeof(c->enc));
						NBStruct_stRelease(keyMap, &c->dec, sizeof(c->dec));
						NBArray_removeItemAtIndex(&opq->thread.keysCache, i);
					} else {
						//Update plain value
						NBStruct_stRelease(keyMap, &c->dec, sizeof(c->dec));
						NBStruct_stClone(keyMap, &t->keyPlain, sizeof(t->keyPlain), &c->dec, sizeof(c->dec));
					}
					r = TRUE;
					break;
				}
			}
		}
	}
	return r;
}

void TSCypher_runJob_(STTSCypherOpq* opq, STTSCypherJobRef* job, UI32* dstTotalSuccess, UI32* dstTotalErr){
	UI32 totalSuccess = 0, totalErr = 0;
	{
		const SI32 count = TSCypherJob_getTasksCount(job);
		//PRINTF_INFO("CypherThread, processing %d tasks of job.\n", count);
		SI32 i; for(i = 0; i < count; i++){
			STTSCypherTask* t = TSCypherJob_getTaskAtIdx(job, i);
			if(TSCypherJob_isInvalidated(job)){
				totalErr++;
			} else {
				//PRINTF_INFO("CypherThread, processing %d datas.\n", t->blocks.use);
				if(t->type != ENTSCypherTaskType_Decrypt){
					PRINTF_CONSOLE_ERROR("CypherThread, only decrypt taks implemented yet.\n");
					totalErr++;
				} else {
					BOOL isKeyLoaded = FALSE;
					//Determine if key was already decrypted
					if(!isKeyLoaded){
						if(!TSCypherDataKey_isEmpty(&t->keyPlain)){
							isKeyLoaded		= TRUE;
						}
					}
					//Search at cache
					if(!isKeyLoaded){
						//PRINTF_INFO("TSCypher_runJob_, loading key (lock-on).\n");
						NBThreadMutex_lock(&opq->thread.mutex);
						if(TSCypher_loadKeyFromCacheLocked_(opq, t) == ENTSCypherKeyCacheState_FoundDecrypted){
							isKeyLoaded		= TRUE;
							break;
						}
						//PRINTF_INFO("TSCypher_runJob_, loading key (lock-off).\n");
						NBThreadMutex_unlock(&opq->thread.mutex);
					}
					//
					if(!isKeyLoaded){
						if(!TSCypher_decDataKeyWithCurrentKeyOpq(opq, &t->keyPlain, &t->key)){
							//PRINTF_CONSOLE_ERROR("CypherThread, TSCypher_decDataKeyWithCurrentKeyOpq failed for task '%s'.\n", t->name.str);
						} else {
							isKeyLoaded = TRUE;
							//Add to cache
							{
								//PRINTF_INFO("TSCypher_runJob_, updating cache (lock-on).\n");
								NBThreadMutex_lock(&opq->thread.mutex);
								TSCypher_updateOrRemoveKeyAtCacheLocked_(opq, t);
								//PRINTF_INFO("TSCypher_runJob_, updating cache (lock-off).\n");
								NBThreadMutex_unlock(&opq->thread.mutex);
							}
						}
					}
					if(!isKeyLoaded){
						totalErr++;
					} else {
						//Execute tasks
						{
							SI32 i; for(i = 0; i < t->blocks.use; i++){
								STTSCypherTaskData* data = NBArray_itmPtrAtIndex(&t->blocks, STTSCypherTaskData, i);
								if(!data->processed){
									switch (data->type) {
										case ENTSCypherTaskDataType_Embedded:
											if(!TSCypher_genDataDec(&data->embedded.output, &t->keyPlain, data->embedded.input.data, data->embedded.input.dataSz)){
												PRINTF_CONSOLE_ERROR("CypherThread, TSCypher_genDataDec failed.\n");
												totalErr++;
											} else {
												//PRINTF_INFO("CypherThread, TSCypher_genDataDec success.\n");
												totalSuccess++;
												data->processed = TRUE;
											}
											break;
										case ENTSCypherTaskDataType_FileRef:
											//Create destination folder path
											if(data->fileRef.output.length > 0){
												const SI32 posSlash0 = NBString_lastIndexOf(&data->fileRef.output, "/", data->fileRef.output.length - 1);
												const SI32 posSlash1 = NBString_lastIndexOf(&data->fileRef.output, "\\", data->fileRef.output.length - 1);
												const SI32 posSlash = (posSlash0 > posSlash1 ? posSlash0 : posSlash1);
												if(posSlash > 0){
													STNBString fldrs;
													NBString_init(&fldrs);
													NBString_concatBytes(&fldrs, data->fileRef.output.str, posSlash);
													NBFilesystem_createFolderPath(TSContext_getFilesystem(opq->context), fldrs.str);
													NBString_release(&fldrs);
												}
											}
											//Decrypt
											if(!TSCypher_genFileDec(&t->keyPlain, data->fileRef.input.str, data->fileRef.inputPos, data->fileRef.inputSz, data->fileRef.output.str)){
												PRINTF_CONSOLE_ERROR("CypherThread, TSCypher_genFileDec failed.\n");
												totalErr++;
											} else {
												//PRINTF_INFO("CypherThread, TSCypher_genFileDec success.\n");
												totalSuccess++;
												data->processed = TRUE;
											}
											break;
										default:
											NBASSERT(FALSE)
											break;
									}
								}
							}
						}
					}
				}
			}
		}
	}
	//
	if(dstTotalSuccess != NULL) *dstTotalSuccess = totalSuccess;
	if(dstTotalErr != NULL) *dstTotalErr = totalErr;
}

//Thread

typedef struct STTSCypherThread_ {
	UI32			iThread;
	STTSCypherOpq*	opq;
} STTSCypherThread;

SI64 TSCypher_runJobsMethd_(STNBThread* t, void* param){
	SI64 r = 0;
	STTSCypherThread* tt = (STTSCypherThread*)param;
	if(tt != NULL){
		STTSCypherOpq* opq = (STTSCypherOpq*)tt->opq;
		NBASSERT(opq->thread.threadsRunning > 0)
		{
			STNBArray tKeyAtOtherThread, tKeyNotFound, tKeyAtOtherThreadLL, tKeyNotFoundLL;
			NBArray_init(&tKeyAtOtherThread, sizeof(STTSCypherTask*), NULL);
			NBArray_init(&tKeyNotFound, sizeof(STTSCypherTask*), NULL);
			NBArray_init(&tKeyAtOtherThreadLL, sizeof(STTSCypherTask*), NULL);
			NBArray_init(&tKeyNotFoundLL, sizeof(STTSCypherTask*), NULL);
			{
				//PRINTF_INFO("TSCypher_runJobsMethd_, starting (lock-on).\n");
				NBThreadMutex_lock(&opq->thread.mutex);
				while(!opq->thread.stopFlag){
					STTSCypherJobRef* job = NULL;
					UI32 jobKeysTotal = 0, jobKeysFound = 0;
					NBArray_empty(&tKeyAtOtherThread);
					NBArray_empty(&tKeyNotFound);
					//Find job (locked)
					while(!opq->thread.stopFlag && job == NULL){
						//Search a job
						if(opq->thread.jobs.use > 0){
							//Analyze
							{
								//PRINTF_INFO("TSCypher_runJobsMethd_, looking for a job.\n");
								SI32 i; for(i = 0; i < opq->thread.jobs.use; i++){
									STTSCypherJobRef* j = NBArray_itmValueAtIndex(&opq->thread.jobs, STTSCypherJobRef*, i);
									if(TSCypherJob_getState(j) == ENTSCypherJobState_Waiting){
										//Detect the quickest job (invalidated or all keys already loaded)
										UI32 keysTotal = 0, keysLoaded = 0;
										NBArray_empty(&tKeyAtOtherThreadLL);
										NBArray_empty(&tKeyNotFoundLL);
										if(!TSCypherJob_isInvalidated(j)){
											const SI32 count = TSCypherJob_getTasksCount(j);
											SI32 i; for(i = 0; i < count; i++){
												STTSCypherTask* t = TSCypherJob_getTaskAtIdx(j, i);
												keysTotal++;
												if(t->type != ENTSCypherTaskType_Decrypt){
													PRINTF_CONSOLE_ERROR("CypherThread, only decrypt taks implemented yet");
												} else {
													BOOL isKeyLoaded = FALSE;
													//Determine if key was already decrypted
													if(!isKeyLoaded){
														if(!TSCypherDataKey_isEmpty(&t->keyPlain)){
															isKeyLoaded	= TRUE;
														}
													}
													//Search at cache
													if(!isKeyLoaded){
														const ENTSCypherKeyCacheState keyState = TSCypher_loadKeyFromCacheLocked_(opq, t);
														if(keyState == ENTSCypherKeyCacheState_FoundDecrypted){
															//Key loaded from cache (already decrypted)
															isKeyLoaded	= TRUE;
														} else if(keyState == ENTSCypherKeyCacheState_FoundPending){
															//Key present in (in process to be decrypted bu other task)
															NBArray_addValue(&tKeyAtOtherThreadLL, t);
														} else {
															NBASSERT(keyState == ENTSCypherKeyCacheState_NotFound)
															//Key not found at cache
															NBArray_addValue(&tKeyNotFoundLL, t);
														}
													}
													//Add key as found
													if(isKeyLoaded){
														keysLoaded++;
													}
												}
											}
										}
										//Set as job (if no key-decrypting by other job was found)
										if(tKeyAtOtherThreadLL.use > 0){
											//Ignore this job to avoid double decryption of same key
											//(other thread is in process of decrytion of same keys).
											//PRINTF_INFO("CypherThread, ignoring job due to %d keys being decrypted by other thread.\n", tKeyAtOtherThreadLL.use);
										} else {
											job				= j;
											jobKeysTotal	= keysTotal;
											jobKeysFound	= keysLoaded;
											{
												NBArray_empty(&tKeyAtOtherThread);
												NBArray_addItems(&tKeyAtOtherThread, NBArray_data(&tKeyAtOtherThreadLL), sizeof(STTSCypherTask*), tKeyAtOtherThreadLL.use);
											}
											{
												NBArray_empty(&tKeyNotFound);
												NBArray_addItems(&tKeyNotFound, NBArray_data(&tKeyNotFoundLL), sizeof(STTSCypherTask*), tKeyNotFoundLL.use);
											}
											//Process this (if is quick-job)
											if(keysTotal == keysLoaded){
												//PRINTF_INFO("TSCypher_runJobsMethd_, piking quick-job.\n");
												break;
											}
										}
									}
								}
							}
							//Thread-0 must be used only for quick jobs (other threats can be locked by remote decryption tasks).
							if(opq->thread.threadsRunning > 1 && tt->iThread == 0 && jobKeysTotal != jobKeysFound){
								//PRINTF_INFO("TSCypher_runJobsMethd_, resigning to job in thread 0 in multithread TSCypher.\n");
								job = NULL;
							}
							//Update job state
							if(job != NULL){
								TSCypherJob_setState(job, ENTSCypherJobState_Working);
								//Add keys not found to cache (this will tell other thread that this key is in process of decryption)
								{
									SI32 i; for(i = 0; i < tKeyNotFound.use; i++){
										STTSCypherTask* t = NBArray_itmValueAtIndex(&tKeyNotFound, STTSCypherTask*, i);
										if(!TSCypher_addKeyToCacheLocked_(opq, t)){
											PRINTF_CONSOLE_ERROR("CypherThread, TSCypher_addKeyToCacheLocked_ failed.\n");
										}
									}
								}
								//if(jobKeysTotal == jobKeysFound){
								//	PRINTF_INFO("CypherThread, executing quick-job (thread #%d).\n", (tt->iThread + 1));
								//} else {
								//	PRINTF_INFO("CypherThread, executing slow-job (thread #%d).\n", (tt->iThread + 1));
								//}
							}
						}
						//Wait for a job
						if(job == NULL){
							//PRINTF_INFO("TSCypher_runJobsMethd_, waiting for a job.\n");
							NBThreadCond_wait(&opq->thread.cond, &opq->thread.mutex);
							//PRINTF_INFO("TSCypher_runJobsMethd_, ended wait for a job.\n");
						}
					}
					//Process and release
					if(job != NULL){
						UI32 totalSuccess = 0, totalErr = 0;
						//Process
						{
							//PRINTF_INFO("TSCypher_runJobsMethd_, procesing job (lock-off).\n");
							NBThreadMutex_unlock(&opq->thread.mutex);
							//Execute job (unlocked)
							{
								TSCypher_runJob_(opq, job, &totalSuccess, &totalErr);
							}
							//PRINTF_INFO("TSCypher_runJobsMethd_, processed job (lock-on).\n");
							NBThreadMutex_lock(&opq->thread.mutex);
							//Update keys results (locked)
							{
								SI32 i; for(i = 0; i < tKeyNotFound.use; i++){
									STTSCypherTask* t = NBArray_itmValueAtIndex(&tKeyNotFound, STTSCypherTask*, i);
									TSCypher_updateOrRemoveKeyAtCacheLocked_(opq, t);
								}
							}
						}
						//Update state (locked)
						{
							//PRINTF_INFO("TSCypher_runJobsMethd_, updating state of job.\n");
							const ENTSCypherJobState result = (totalErr == 0 ? ENTSCypherJobState_Done : totalSuccess == 0 ? ENTSCypherJobState_Error : ENTSCypherJobState_Done);
							TSCypherJob_setState(job, result);
							if(result == ENTSCypherJobState_Error){
								PRINTF_INFO("CypherThread, job ended with success(%d) errors(%d).\n", totalSuccess, totalErr);
							}
							//if(totalSuccess == 0 || totalErr != 0){
							//	PRINTF_INFO("CypherThread, job ended with success(%d) errors(%d).\n", totalSuccess, totalErr);
							//}
						}
						//Remove job (locked)
						{
							BOOL fnd = FALSE;
							SI32 i; for(i = 0 ; i < opq->thread.jobs.use; i++){
								STTSCypherJobRef* itm = NBArray_itmValueAtIndex(&opq->thread.jobs, STTSCypherJobRef*, i);
								if(itm == job){
									//PRINTF_INFO("TSCypher_runJobsMethd_, removing job.\n");
									NBArray_removeItemAtIndex(&opq->thread.jobs, i);
									TSCypherJob_release(job);
									NBMemory_free(job);
									job = NULL;
									fnd = TRUE;
									break;
								}
							} NBASSERT(fnd) //Should be there
						}
					}
				}
				NBASSERT(opq->thread.threadsRunning > 0)
				opq->thread.threadsRunning--;
				//PRINTF_INFO("TSCypher_runJobsMethd_, ended (lock-off).\n");
				NBThreadMutex_unlock(&opq->thread.mutex);
			}
			NBArray_release(&tKeyNotFoundLL);
			NBArray_release(&tKeyAtOtherThreadLL);
			NBArray_release(&tKeyNotFound);
			NBArray_release(&tKeyAtOtherThread);
		}
	}
	//Release
	if(tt != NULL){
		NBMemory_free(tt);
		tt = NULL;
	}
	//Release
	if(t != NULL){
		NBThread_release(t);
		NBMemory_free(t);
		t = NULL;
	}
	return r;
}

BOOL TSCypher_startThreads(STTSCypher* obj){
	BOOL r = FALSE;
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	//Start thread
	{
		PRINTF_INFO("TSCypher_startThreads, starting threads (lock-on).\n");
		NBThreadMutex_lock(&opq->thread.mutex);
		if(opq->thread.threadsRunning > 0){
			PRINTF_CONSOLE_ERROR("Could not start CypherThread, already running.\n");
		} else {
			const SI32 totalThreads = opq->cfg.jobThreads;
			opq->thread.stopFlag = FALSE;
			SI32 i; for(i = 0; i < totalThreads; i++){
				STTSCypherThread* tt = NBMemory_allocType(STTSCypherThread);
				NBMemory_setZeroSt(*tt, STTSCypherThread);
				tt->iThread	= i;
				tt->opq		= opq;
				//
				STNBThread* t = NBMemory_allocType(STNBThread);
				NBThread_init(t);
				NBThread_setIsJoinable(t, FALSE);
				//
				opq->thread.threadsRunning++;
				if(!NBThread_start(t, TSCypher_runJobsMethd_, tt, NULL)){
					PRINTF_CONSOLE_ERROR("Could not start CypherThread.\n");
					//Release
					NBMemory_free(tt);
					tt = NULL;
					//Release
					opq->thread.threadsRunning--;
					NBThread_release(t);
					NBMemory_free(t);
					t = NULL;
					//
					break;
				}
			}
			//Result
			if(totalThreads <= 0){
				PRINTF_CONSOLE_ERROR("CypherThread configured for %d threads.\n", totalThreads);
				opq->thread.stopFlag = TRUE;
				r = FALSE;
			} else if(i < totalThreads){
				PRINTF_CONSOLE_ERROR("CypherThread started only %d (of %d) threads.\n", i, totalThreads);
				opq->thread.stopFlag = TRUE;
				r = FALSE;
			} else {
				PRINTF_INFO("CypherThread started %d (of %d) threads.\n", i, totalThreads);
				r = TRUE;
			}
		}
		PRINTF_INFO("TSCypher_startThreads, threads started (lock-off).\n");
		NBThreadMutex_unlock(&opq->thread.mutex);
	}
	return r;
}

BOOL TSCypher_addAsyncJob(STTSCypher* obj, STTSCypherJobRef* job){
	BOOL r = FALSE;
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	if(opq != NULL && job != NULL){
		{
			//PRINTF_INFO("TSCypher_addAsyncJob, (lock-on).\n");
			NBThreadMutex_lock(&opq->thread.mutex);
			if(opq->thread.threadsRunning <= 0){
				TSCypherJob_setState(job, ENTSCypherJobState_Error);
				PRINTF_CONSOLE_ERROR("CypherThread is not running, ignoring job.\n");
			} else {
				//Add to array and broadcast-cond
				TSCypherJob_setState(job, ENTSCypherJobState_Waiting);
				//Copy reference and add
				{
					STTSCypherJobRef* cpy = NBMemory_allocType(STTSCypherJobRef);
					TSCypherJob_initWithOther(cpy, job);
					NBArray_addValue(&opq->thread.jobs, cpy);
				}
				//Broadcast (activate thread)
				NBThreadCond_broadcast(&opq->thread.cond);
				//PRINTF_INFO("CypherThread added job (%d in total).\n", opq->thread.jobs.use);
				r = TRUE;
			}
			//PRINTF_INFO("TSCypher_addAsyncJob, (lock-off).\n");
			NBThreadMutex_unlock(&opq->thread.mutex);
		}
	}
	return r;
}

//

BOOL TSCypher_isKeyAnySet(STTSCypher* obj){
	BOOL r = FALSE;
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	r = (opq->my.cert != NULL && opq->my.key != NULL) ? TRUE : FALSE;
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSCypher_isKeyLocalSet(STTSCypher* obj){
	BOOL r = FALSE;
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	r = (opq->my.cert != NULL && opq->my.key != NULL ? TRUE : FALSE);
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSCypher_setCertAndKey_(STTSCypher* obj, STNBX509* cert, STNBPKey* key){
	BOOL r = FALSE;
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	if(key == NULL || cert == NULL){
		NBThreadMutex_lock(&opq->mutex);
		{
			//Release current
			if(opq->my.key != NULL){
				NBPKey_release(opq->my.key);
				NBMemory_free(opq->my.key);
				opq->my.key = NULL;
			}
			if(opq->my.cert != NULL){
				NBX509_release(opq->my.cert);
				NBMemory_free(opq->my.cert);
				opq->my.cert = NULL;
			}
			NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &opq->my.sharedKeyPlain, sizeof(opq->my.sharedKeyPlain));
			NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &opq->my.sharedKeyEnc, sizeof(opq->my.sharedKeyEnc));
		}
		NBThreadMutex_unlock(&opq->mutex);
		r = TRUE;
	} else {
		STNBPKey* nKey = NBMemory_allocType(STNBPKey);
		STNBX509* nCert = NBMemory_allocType(STNBX509);
		NBPKey_init(nKey);
		NBX509_init(nCert);
		if(!NBPKey_createFromOther(nKey, key)){
			PRINTF_CONSOLE_ERROR("TSCypher_setCertAndKey, NBPKey_createFromOther failed.\n");
		} else if(!NBX509_createFromOther(nCert, cert)){
			PRINTF_CONSOLE_ERROR("TSCypher_setCertAndKey, NBX509_createFromOther failed.\n");
		} else {
			NBThreadMutex_lock(&opq->mutex);
			{
				//Release current
				if(opq->my.key != NULL){
					NBPKey_release(opq->my.key);
					NBMemory_free(opq->my.key);
					opq->my.key = NULL;
				}
				if(opq->my.cert != NULL){
					NBX509_release(opq->my.cert);
					NBMemory_free(opq->my.cert);
					opq->my.cert = NULL;
				}
				NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &opq->my.sharedKeyPlain, sizeof(opq->my.sharedKeyPlain));
				NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &opq->my.sharedKeyEnc, sizeof(opq->my.sharedKeyEnc));
				//Set new
				opq->my.key		= nKey; nKey = NULL;
				opq->my.cert	= nCert; nCert = NULL;
				r = TRUE;
#				ifdef NB_CONFIG_INCLUDE_ASSERTS
				{
					STNBString ss;
					NBString_init(&ss);
					if(NBX509_concatSerialHex(cert, &ss)){
					//	PRINTF_INFO("TSCypher_setCertAndKey success for '%s'.\n", ss.str);
					}
					{
						STNBString test, enc, dec;
						NBString_initWithStr(&test, "test");
						NBString_init(&enc);
						NBString_init(&dec);
						NBASSERT(NBPKey_concatEncryptedBytes(opq->my.key, test.str, test.length, &enc))
						NBASSERT(NBX509_concatDecryptedBytes(opq->my.cert, enc.str, enc.length, &dec))
						NBASSERT(NBString_isEqual(&dec, test.str))
						NBString_release(&enc);
						NBString_release(&dec);
						NBString_release(&test);
					}
					NBString_release(&ss);
				}
#				else
				//PRINTF_INFO("TSCypher_setCertAndKey success.\n");
#				endif
			}
			NBThreadMutex_unlock(&opq->mutex);
		}
		//Release (if not consumed)
		if(nKey != NULL){
			NBPKey_release(nKey);
			NBMemory_free(nKey);
			nKey = NULL;
		}
		if(nCert != NULL){
			NBX509_release(nCert);
			NBMemory_free(nCert);
			nCert = NULL;
		}
	}
	return r;
}

BOOL TSCypher_setSharedKey_(STTSCypher* obj, const STTSCypherDataKey* sharedKeyEnc){
	BOOL r = FALSE;
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	if(sharedKeyEnc == NULL){
		NBThreadMutex_lock(&opq->mutex);
		{
			//Release current
			NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &opq->my.sharedKeyPlain, sizeof(opq->my.sharedKeyPlain));
			NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &opq->my.sharedKeyEnc, sizeof(opq->my.sharedKeyEnc));
		}
		NBThreadMutex_unlock(&opq->mutex);
		r = TRUE;
	} else {
		NBThreadMutex_lock(&opq->mutex);
		if(opq->my.key != NULL){
			STTSCypherDataKey sharedKeyPlain;
			NBMemory_setZeroSt(sharedKeyPlain, STTSCypherDataKey);
			if(TSCypher_decDataKeyWithCurrentKeyOpq(opq, &sharedKeyPlain, sharedKeyEnc)){
				//Release current
				NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &opq->my.sharedKeyPlain, sizeof(opq->my.sharedKeyPlain));
				NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &opq->my.sharedKeyEnc, sizeof(opq->my.sharedKeyEnc));
				//Set new
				NBStruct_stClone(TSCypherDataKey_getSharedStructMap(), &sharedKeyPlain, sizeof(sharedKeyPlain), &opq->my.sharedKeyPlain, sizeof(opq->my.sharedKeyPlain));
				NBStruct_stClone(TSCypherDataKey_getSharedStructMap(), sharedKeyEnc, sizeof(*sharedKeyEnc), &opq->my.sharedKeyEnc, sizeof(opq->my.sharedKeyEnc));
				r = TRUE;
			}
			NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &sharedKeyPlain, sizeof(sharedKeyPlain));
		}
		NBThreadMutex_unlock(&opq->mutex);
	}
	return r;
}

BOOL TSCypher_getSharedKey(STTSCypher* obj, STTSCypherDataKey* dstSharedKeyPlain, STTSCypherDataKey* dstSharedKeyEnc){
	BOOL r = FALSE;
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	{
		if(dstSharedKeyPlain != NULL){
			NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), dstSharedKeyPlain, sizeof(*dstSharedKeyPlain));
			NBStruct_stClone(TSCypherDataKey_getSharedStructMap(), &opq->my.sharedKeyPlain, sizeof(opq->my.sharedKeyPlain), dstSharedKeyPlain, sizeof(*dstSharedKeyPlain));
		}
		if(dstSharedKeyEnc != NULL){
			NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), dstSharedKeyEnc, sizeof(*dstSharedKeyEnc));
			NBStruct_stClone(TSCypherDataKey_getSharedStructMap(), &opq->my.sharedKeyEnc, sizeof(opq->my.sharedKeyEnc), dstSharedKeyEnc, sizeof(*dstSharedKeyEnc));
		}
		r = TRUE;
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSCypher_encForCurrentKey(STTSCypher* obj, const void* plain, const UI32 plainSz, STNBString* dst){
	BOOL r = FALSE;
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(opq->my.cert != NULL){
		r = NBX509_concatEncryptedBytes(opq->my.cert, plain, plainSz, dst);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSCypher_decWithCurrentKey(STTSCypher* obj, const void* enc, const UI32 encSz, STNBString* dst){
	BOOL r = FALSE;
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(opq->my.key != NULL){
		r = NBPKey_concatDecryptedBytes(opq->my.key, enc, encSz, dst);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSCypher_concatCertSerialHex(STTSCypher* obj, STNBString* dst){
	BOOL r = FALSE;
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	if(opq->my.cert != NULL){
		r = NBX509_concatSerialHex(opq->my.cert, dst);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

BOOL TSCypher_concatBytesSignatureSha1(STTSCypher* obj, const void* data, const SI32 dataSz, STNBString* dstBin){
	BOOL r = FALSE;
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	NBThreadMutex_lock(&opq->mutex);
	//ToDo: implement remote version of this using itf
	if(opq->my.key != NULL){
		r = NBPKey_concatBytesSignatureSha1(opq->my.key, data, dataSz, dstBin);
	}
	NBThreadMutex_unlock(&opq->mutex);
	return r;
}

//Encode

void TSCypher_cloneData(STTSCypherData* dst, const STTSCypherData* src){
	NBString_strFreeAndNewBufferBytes((char**)&dst->data, (const char*)src->data, src->dataSz);
	dst->dataSz = src->dataSz;
}

BOOL TSCypher_genDataKeyWithCurrentKey(STTSCypher* obj, STTSCypherDataKey* dstEnc, STTSCypherDataKey* dstPlain){
	BOOL r = FALSE;
	if(obj != NULL){
		STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
		if(opq != NULL){
			r = TSCypher_genDataKey(dstEnc, dstPlain, opq->my.cert);
		}
	}
	return r;
}

BOOL TSCypher_genDataKey(STTSCypherDataKey* dstEnc, STTSCypherDataKey* dstPlain, STNBX509* cert){
	BOOL r = FALSE;
	if(cert != NULL){
		const UI32 iterations = 2 + (rand() % 30);
		STNBString pass, salt, pass2, salt2;
		NBString_init(&pass);
		NBString_init(&pass2);
		NBString_init(&salt);
		NBString_init(&salt2);
		TSCypher_genPassword(&pass, 32);
		TSCypher_genPassword(&salt, 32);
		if(!NBX509_concatEncryptedBytes(cert, pass.str, pass.length, &pass2)){
			PRINTF_CONSOLE_ERROR("NBX509_concatEncryptedBytes failed.\n");
		} else if(!NBX509_concatEncryptedBytes(cert, salt.str, salt.length, &salt2)){
			PRINTF_CONSOLE_ERROR("NBX509_concatEncryptedBytes failed.\n");
		} else {
			if(dstPlain != NULL){
				NBString_strFreeAndNewBufferBytes((char**)&dstPlain->pass, pass.str, pass.length);
				NBString_strFreeAndNewBufferBytes((char**)&dstPlain->salt, salt.str, salt.length);
				dstPlain->passSz = pass.length;
				dstPlain->saltSz = salt.length;
				dstPlain->iterations = iterations;
			}
			if(dstEnc != NULL){
				NBString_strFreeAndNewBufferBytes((char**)&dstEnc->pass, pass2.str, pass2.length);
				NBString_strFreeAndNewBufferBytes((char**)&dstEnc->salt, salt2.str, salt2.length);
				dstEnc->passSz = pass2.length;
				dstEnc->saltSz = salt2.length;
				dstEnc->iterations = iterations;
			}
			r = TRUE;
		}
		NBString_release(&pass2);
		NBString_release(&pass);
		NBString_release(&salt2);
		NBString_release(&salt);
	}
	return r;
}

BOOL TSCypher_genDataEnc(STTSCypherData* dstEnc, const STTSCypherDataKey* keyPlain, const void* plainData, const UI32 plainDataSz){
	BOOL r = FALSE;
	if(dstEnc != NULL && keyPlain != NULL){
		if(plainDataSz <= 0){
			if(dstEnc->data != NULL) NBMemory_free(dstEnc->data);
			dstEnc->data	= NULL;
			dstEnc->dataSz	= 0;
			r = TRUE;
		} else if(plainData != NULL && plainDataSz > 0){
			STNBString enc;
			NBString_init(&enc);
			if(!NBAes256_aesEncrypt(plainData, plainDataSz, (const char*)keyPlain->pass, keyPlain->passSz, keyPlain->salt, keyPlain->saltSz, keyPlain->iterations, &enc)){
				PRINTF_CONSOLE_ERROR("NBAes256_aesEncrypt failed.\n");
			} else {
				//Set
				{
					if(dstEnc->data != NULL) NBMemory_free(dstEnc->data);
					dstEnc->data	= (BYTE*)enc.str;
					dstEnc->dataSz	= enc.length;
					NBString_resignToContent(&enc);
				}
				r = TRUE;
			}
			NBString_release(&enc);
		}
	}
	return r;
}

BOOL TSCypher_genDataReEnc(STTSCypherData* dstEnc, const STTSCypherDataKey* keyPlainEnc, const STTSCypherData* srcEnc, const STTSCypherDataKey* srcKeyPlainDec){
	BOOL r = FALSE;
	if(dstEnc != NULL && keyPlainEnc != NULL && srcEnc != NULL && srcKeyPlainDec != NULL){
		STTSCypherData dec;
		NBMemory_setZeroSt(dec, STTSCypherData);
		if(!TSCypher_genDataDec(&dec, srcKeyPlainDec, srcEnc->data, srcEnc->dataSz)){
			PRINTF_CONSOLE_ERROR("TSCypher_genDataDec failed.\n");
		} else if(!TSCypher_genDataEnc(dstEnc, keyPlainEnc, dec.data, dec.dataSz)){
			PRINTF_CONSOLE_ERROR("TSCypher_genDataEnc failed.\n");
		} else {
			r = TRUE;
		}
		NBStruct_stRelease(TSCypherData_getSharedStructMap(), &dec, sizeof(dec));
	}
	return r;
}

//Decode

BOOL TSCypher_encDataKey(STTSCypherDataKey* dstKeyEnc, const STTSCypherDataKey* keyPlain, STNBX509* cert){
	BOOL r = FALSE;
	if(keyPlain != NULL && cert != NULL){
		if(!TSCypherDataKey_isEmpty(keyPlain)){
			STNBString pass, salt;
			NBString_init(&pass);
			NBString_init(&salt);
			if(!NBX509_concatEncryptedBytes(cert, keyPlain->pass, keyPlain->passSz, &pass)){
				PRINTF_CONSOLE_ERROR("NBX509_concatEncryptedBytes failed.\n");
			} else if(!NBX509_concatEncryptedBytes(cert, keyPlain->salt, keyPlain->saltSz, &salt)){
				PRINTF_CONSOLE_ERROR("NBX509_concatEncryptedBytes failed.\n");
			} else {
				if(dstKeyEnc != NULL){
					dstKeyEnc->iterations = keyPlain->iterations;
					NBString_strFreeAndNewBufferBytes((char**)&dstKeyEnc->pass, pass.str, pass.length);
					NBString_strFreeAndNewBufferBytes((char**)&dstKeyEnc->salt, salt.str, salt.length);
					dstKeyEnc->passSz = pass.length;
					dstKeyEnc->saltSz = salt.length;
				}
				r = TRUE;
			}
			NBString_release(&pass);
			NBString_release(&salt);
		}
	}
	return r;
}

BOOL TSCypher_decDataKey(STTSCypherDataKey* dstKeyPlain, const STTSCypherDataKey* keyEnc, STNBPKey* key){
	BOOL r = FALSE;
	if(keyEnc != NULL && key != NULL){
		if(!TSCypherDataKey_isEmpty(keyEnc)){
			STNBString pass, salt;
			NBString_init(&pass);
			NBString_init(&salt);
			if(!NBPKey_concatDecryptedBytes(key, keyEnc->pass, keyEnc->passSz, &pass)){
				PRINTF_CONSOLE_ERROR("NBPKey_concatDecryptedBytes failed for enc-pass-sz(%d).\n", keyEnc->passSz);
			} else if(!NBPKey_concatDecryptedBytes(key, keyEnc->salt, keyEnc->saltSz, &salt)){
				PRINTF_CONSOLE_ERROR("NBPKey_concatDecryptedBytes failed for end-salt-sz(%d).\n", keyEnc->saltSz);
			} else {
				if(dstKeyPlain != NULL){
					dstKeyPlain->iterations = keyEnc->iterations;
					NBString_strFreeAndNewBufferBytes((char**)&dstKeyPlain->pass, pass.str, pass.length);
					NBString_strFreeAndNewBufferBytes((char**)&dstKeyPlain->salt, salt.str, salt.length);
					dstKeyPlain->passSz = pass.length;
					dstKeyPlain->saltSz = salt.length;
				}
				r = TRUE;
			}
			NBString_release(&pass);
			NBString_release(&salt);
		}
	}
#	ifdef NB_CONFIG_INCLUDE_ASSERTS
	if(!r){
		STNBString str;
		NBString_init(&str);
		NBStruct_stConcatAsJson(&str, TSCypherDataKey_getSharedStructMap(), keyEnc, sizeof(*keyEnc));
		PRINTF_CONSOLE_ERROR("TSCypher_decDataKey failed for encKey:\n-->%s<--\n", str.str);
		NBString_release(&str);
	}
#	endif
	return r;
}

BOOL TSCypher_genDataDec(STTSCypherData* dstDec, const STTSCypherDataKey* keyPlain, const void* encData, const UI32 encDataSz){
	BOOL r = FALSE;
	if(dstDec != NULL && keyPlain != NULL){
		if(encDataSz <= 0){
			if(dstDec->data != NULL) NBMemory_free(dstDec->data);
			dstDec->data	= NULL;
			dstDec->dataSz	= 0;
			r = TRUE;
		} else if(encData != NULL && encDataSz > 0){
			STNBString dec;
			NBString_init(&dec);
			if(!NBAes256_aesDecrypt(encData, encDataSz, (const char*)keyPlain->pass, keyPlain->passSz, keyPlain->salt, keyPlain->saltSz, keyPlain->iterations, &dec)){
				PRINTF_CONSOLE_ERROR("NBAes256_aesDecrypt failed.\n");
			} else {
				//Set
				{
					if(dstDec->data != NULL) NBMemory_free(dstDec->data);
					dstDec->data	= (BYTE*)dec.str;
					dstDec->dataSz	= dec.length;
					NBString_resignToContent(&dec);
				}
				r = TRUE;
			}
			NBString_release(&dec);
		}
	}
	return r;
}

BOOL TSCypher_genDataDecStr(STNBString* dstDec, const STTSCypherDataKey* keyPlain, const void* encData, const UI32 encDataSz){
	BOOL r = FALSE;
	if(dstDec != NULL && keyPlain != NULL){
		if(encDataSz <= 0){
			if(dstDec != NULL){
				NBString_empty(dstDec);
			}
			r = TRUE;
		} else if(encData != NULL && encDataSz > 0){
			STNBString dec;
			NBString_init(&dec);
			if(!NBAes256_aesDecrypt(encData, encDataSz, (const char*)keyPlain->pass, keyPlain->passSz, keyPlain->salt, keyPlain->saltSz, keyPlain->iterations, &dec)){
				PRINTF_CONSOLE_ERROR("NBAes256_aesDecrypt failed.\n");
			} else {
				//Set
				if(dstDec != NULL){
					NBString_swapContent(&dec, dstDec);
				}
				r = TRUE;
			}
			NBString_release(&dec);
		}
	}
	return r;
}

BOOL TSCypher_genFileDec(const STTSCypherDataKey* keyPlain, const char* inputFilepath, const SI64 pos, const SI64 sz, const char* outputFilepath){
	BOOL r = FALSE;
	if(keyPlain != NULL && !NBString_strIsEmpty(inputFilepath) && !NBString_strIsEmpty(outputFilepath) && pos >= 0){
        STNBFileRef in = NBFile_alloc(NULL);
		if(!NBFile_open(in, inputFilepath, ENNBFileMode_Read)){
			PRINTF_CONSOLE_ERROR("TSCypher_genFileDec, Could not open file for read: '%s'.\n", inputFilepath);
		} else {
			NBFile_lock(in);
			if(!NBFile_seek(in, pos, ENNBFileRelative_Start)){
				PRINTF_CONSOLE_ERROR("TSCypher_genFileDec, Could not seek file for read: '%s'.\n", inputFilepath);
			} else {
                STNBFileRef out = NBFile_alloc(NULL);
				if(!NBFile_open(out, outputFilepath, ENNBFileMode_Write)){
					PRINTF_CONSOLE_ERROR("TSCypher_genFileDec, Could not open file for write: '%s'.\n", outputFilepath);
				} else {
					NBFile_lock(out);
					{
						STNBAes256 dec;
						NBAes256_init(&dec);
						if(!NBAes256_feedStart(&dec, ENAes256Op_Decrypt, (const char*)keyPlain->pass, keyPlain->passSz, (const BYTE*)keyPlain->salt, keyPlain->saltSz, keyPlain->iterations)){
							PRINTF_CONSOLE_ERROR("TSCypher_genFileDec, Could not start  for: '%s' -> '%s'.\n", inputFilepath, outputFilepath);
						} else {
							r = TRUE;
							{
								const UI32 encBlockSz	= NBAes256_blockSize();
								const UI32 bytesPerRead	= (256 * encBlockSz);
								const UI32 outBuffSz	= bytesPerRead + encBlockSz;
								BYTE* inBuff			= (BYTE*)NBMemory_alloc(bytesPerRead);
								BYTE* outBuff			= (BYTE*)NBMemory_alloc(outBuffSz);
								const UI32 cryptdDataSz = (UI32)sz;
								UI32 iByte = 0, wTotal = 0;
								while(iByte < cryptdDataSz){
									const UI32 bytesRemain	= (cryptdDataSz - iByte); //NBASSERT((bytesRemain % encBlockSz) == 0)
									const UI32 bytesToRead	= (bytesRemain > bytesPerRead ? bytesPerRead : bytesRemain); //NBASSERT((bytesToRead % encBlockSz) == 0)
									const SI32 read			= NBFile_read(in, inBuff, bytesToRead);
									if(read <= 0){
										break;
									} else {
										const SI32 written	= NBAes256_feed(&dec, outBuff, (const SI32)outBuffSz, inBuff, read); //NBASSERT((written % encBlockSz) == 0)
										if(written < 0){
											PRINTF_CONSOLE_ERROR("TSCypher_genFileDec, NBAes256_feed returned error.\n");
											r = FALSE; //NBASSERT(FALSE)
											break;
										} else {
											if(written > 0){
												NBFile_write(out, outBuff, written);
											}
											iByte += read;
											wTotal += written;
										}
									}
								}
								//Final
								if(r){
									const SI32 written = NBAes256_feedEnd(&dec, outBuff, (const SI32)outBuffSz);
									if(written < 0){
										PRINTF_CONSOLE_ERROR("TSCypher_genFileDec, NBAes256_feedEnd returned error.\n");
										r = FALSE; //NBASSERT(FALSE)
									} else {
										if(written > 0){
											NBFile_write(out, outBuff, written);
										}
										wTotal += written;
									}
								}
								//PRINTF_INFO("TSCypher_genFileDec, %d of %d bytes processed (%d bytes produced, %d-encBlockSz).\n", iByte, cryptdDataSz, wTotal, encBlockSz);
								NBASSERT(!r || (iByte == cryptdDataSz))
								NBASSERT(!r || (cryptdDataSz >= wTotal && cryptdDataSz <= (wTotal + encBlockSz)))
								NBMemory_free(outBuff);
								NBMemory_free(inBuff);
							}
						}
						NBAes256_release(&dec);
					}
					NBFile_unlock(out);
				}
				NBFile_release(&out);
			}
			NBFile_unlock(in);
		}
		NBFile_release(&in);
	}
	return r;
}

//

void TSCypher_getPassword(STTSCypher* obj, STNBString* dst, const char* passName, const UI8 passLen, const BOOL creatIfNecesary){
	STTSCypherOpq* opq = (STTSCypherOpq*)obj->opaque;
	NBString_empty(dst);
	if(!NBKeychain_getPassword(&opq->keychain, passName, dst)){
		NBString_empty(dst);
		if(creatIfNecesary){
			TSCypher_genPassword(dst, passLen);
			if(!NBKeychain_addPassword(&opq->keychain, passName, dst->str, FALSE)){
				PRINTF_CONSOLE_ERROR("Could not add password to keychain.\n");
				NBString_empty(dst);
			} else {
				//Test saved value
				NBString_empty(dst);
				if(!NBKeychain_getPassword(&opq->keychain, passName, dst)){
					PRINTF_CONSOLE_ERROR("Could not retrieve password created.\n");
					NBString_empty(dst);
				}
			}
		}
	}
}

void TSCypher_genPassword(STNBString* dst, const UI8 passLen){
	const UI32 firstChar = 21 /*'!'*/, lastChar = 125 /*}*/, posibChars = lastChar - firstChar;
	NBString_empty(dst);
	UI32 i; for(i = 0; i < passLen; i++){
		char c = firstChar + (rand() % posibChars);
		NBString_concatByte(dst, c);
	}
}

void TSCypher_destroyString(STNBString* str){
	TSCypher_destroyStr(str->str, str->length);
}

void TSCypher_destroyStr(char* str, const UI32 strSz){
	const UI32 pass = 10;
	UI32 p, i;
	for(p = 0; p < pass; p++){
		for(i = 0 ; i < strSz; i++){
			str[i] = '\0';
		}
	}
}

