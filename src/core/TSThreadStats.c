//
//  TSThreadStats.c
//  thinstream
//
//  Created by Marcos Ortega on 8/2/22.
//

#include "nb/NBFrameworkPch.h"
#include "core/TSThreadStats.h"
//
#include "nb/core/NBMemory.h"
#include "nb/core/NBThreadMutex.h"

//Opaque state

typedef struct STTSThreadStatsOpq_ {
	STNBObject			prnt;	//parent
	STTSThreadStatsData	data;
} STTSThreadStatsOpq;

NB_OBJREF_BODY(TSThreadStats, STTSThreadStatsOpq, NBObject)

//init

void TSThreadStats_initZeroed(STNBObject* obj){
	//STTSThreadStatsOpq* opq = (STTSThreadStatsOpq*)obj;
}

void TSThreadStats_uninitLocked(STNBObject* obj){
	//STTSThreadStatsOpq* opq = (STTSThreadStatsOpq*)obj;
}

//
	
STTSThreadStatsData TSThreadStats_getData(STTSThreadStatsRef ref, const BOOL resetAccum){
	STTSThreadStatsData r;
	STTSThreadStatsOpq* opq = (STTSThreadStatsOpq*)ref.opaque;
	NBASSERT(TSThreadStats_isClass(ref))
	NBObject_lock(opq);
	{
		r = opq->data;
		//reset
		if(resetAccum){
			NBMemory_setZero(opq->data.accum);
		}
	}
	NBObject_unlock(opq);
	return r;
}

//

void TSThreadStats_concatFormatedBytes_(const UI64 bytesCount, STNBString* dst){
	if(dst != NULL){
		if(bytesCount >= (1024 * 1024 * 1024)){
			const UI64 div = (1024 * 1024 * 1024);
			const double p0 = (double)(bytesCount / div); 
			const double p1 = (double)(bytesCount % div) / (double)div;
			NBString_concatDouble(dst, p0 + p1, 1);
			NBString_concat(dst, "GB");
		} else if(bytesCount >= (1024 * 1024)){
			const UI64 div = (1024 * 1024);
			const double p0 = (double)(bytesCount / div); 
			const double p1 = (double)(bytesCount % div) / (double)div;
			NBString_concatDouble(dst, p0 + p1, 1);
			NBString_concat(dst, "MB");
		} else if(bytesCount >= (1024)){
			const UI64 div = (1024);
			const double p0 = (double)(bytesCount / div); 
			const double p1 = (double)(bytesCount % div) / (double)div;
			NBString_concatDouble(dst, p0 + p1, 1);
			NBString_concat(dst, "KB");
		} else {
			NBString_concatUI64(dst, bytesCount);
			NBString_concat(dst, "B");
		}
	}
}

void TSThreadStats_concat(STTSThreadStatsRef ref, const ENNBLogLevel logLvl, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst, const BOOL resetAccum){
	STTSThreadStatsOpq* opq = (STTSThreadStatsOpq*)ref.opaque;
	NBASSERT(TSThreadStats_isClass(ref))
	NBObject_lock(opq);
	{
		TSThreadStats_concatData(&opq->data, logLvl, loaded, accum, total, prefixFirst, prefixOthers, ignoreEmpties, dst);
		//reset
		if(resetAccum){
			NBMemory_setZero(opq->data.accum);
		}
	}
	NBObject_unlock(opq);
}

void TSThreadStats_concatData(const STTSThreadStatsData* obj, const ENNBLogLevel logLvl, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL){
		BOOL opened = FALSE;
		const char* preExtra = "        |- ";
		STNBString str, pre;
		NBString_initWithSz(&str, 0, 256, 0.10f);
		NBString_initWithSz(&pre, NBString_strLenBytes(prefixOthers) + NBString_strLenBytes(preExtra) + 1, 256, 0.10f);
		NBString_concat(&pre, prefixOthers);
		NBString_concat(&pre, preExtra);
		if(loaded){
			NBString_empty(&str);
			TSThreadStats_concatState(&obj->loaded, logLvl, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "loaded:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		if(accum){
			NBString_empty(&str);
			TSThreadStats_concatState(&obj->accum, logLvl, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  " accum:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		if(total){
			NBString_empty(&str);
			TSThreadStats_concatState(&obj->total, logLvl, "", pre.str, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  " total:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		NBString_release(&pre);
		NBString_release(&str);
	}
}

void TSThreadStats_concatState(const STTSThreadStatsState* obj, const ENNBLogLevel logLvl, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL){
		BOOL opened = FALSE;
		const char* preExtra = "       |- ";
		STNBString str, pre;
		NBString_initWithSz(&str, 0, 256, 0.10f);
		NBString_initWithSz(&pre, NBString_strLenBytes(prefixOthers) + NBString_strLenBytes(preExtra) + 1, 256, 0.10f);
		NBString_concat(&pre, prefixOthers);
		NBString_concat(&pre, preExtra);
		//buffers
		{
			NBString_empty(&str);
			TSThreadStats_concatTicks(&obj->ticks, logLvl, ignoreEmpties, &str);
			if(str.length > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "  ticks:  ");
				NBString_concat(dst, str.str);
				opened = TRUE;
			}
		}
		//UpdCalls
		/*{
			if(!ignoreEmpties || obj->updCalls > 0){
				if(opened){
					NBString_concat(dst, "\n");
					NBString_concat(dst, prefixOthers);
				} else {
					NBString_concat(dst, prefixFirst);
				}
				NBString_concat(dst,  "updCall: ");
				NBString_concatUI64(dst, obj->updCalls);
				opened = TRUE;
			}
		}*/
		NBString_release(&pre);
		NBString_release(&str);
	}
}

void TSThreadStats_concatTicks(const STTSThreadStatsTicks* obj, const ENNBLogLevel logLvl, const BOOL ignoreEmpties, STNBString* dst){
	if(dst != NULL){
		BOOL opened = FALSE;
		//count
		if(!ignoreEmpties || obj->count > 0){
			//pollTime
			if(!ignoreEmpties || obj->pollTime.usMin > 0 || obj->pollTime.usMax > 0 || obj->pollTime.usTotal > 0){
				if(opened) NBString_concat(dst, ", ");
				NBString_concat(dst, "poll(");
				//
				NBString_concatUI64(dst, obj->pollTime.usMin / 1000);
				NBString_concat(dst, ".");
				NBString_concatUI64(dst, (obj->pollTime.usMin % 1000) / 100);
				NBString_concatUI64(dst, (obj->pollTime.usMin % 100) / 10);
				//
				{
					const UI64 avg = (obj->pollTime.usTotal / obj->count);
					NBString_concat(dst, "-");
					NBString_concatUI64(dst, avg / 1000);
					NBString_concat(dst, ".");
					NBString_concatUI64(dst, (avg % 1000) / 100);
					NBString_concatUI64(dst, (avg % 100) / 10);
				}
				//
				NBString_concat(dst, "-");
				NBString_concatUI64(dst, obj->pollTime.usMax / 1000);
				NBString_concat(dst, ".");
				NBString_concatUI64(dst, (obj->pollTime.usMax % 1000) / 100);
				NBString_concatUI64(dst, (obj->pollTime.usMax % 100) / 10);
				//
				NBString_concat(dst, "ms)");
				opened = TRUE;
			}
			//notifTime
			if(!ignoreEmpties || obj->notifTime.usMin > 0 || obj->notifTime.usMax > 0 || obj->notifTime.usTotal > 0){
				if(opened) NBString_concat(dst, ", ");
				NBString_concat(dst, "notif(");
				//
				NBString_concatUI64(dst, obj->notifTime.usMin / 1000);
				NBString_concat(dst, ".");
				NBString_concatUI64(dst, (obj->notifTime.usMin % 1000) / 100);
				NBString_concatUI64(dst, (obj->notifTime.usMin % 100) / 10);
				//
				{
					const UI64 avg = (obj->notifTime.usTotal / obj->count);
					NBString_concat(dst, "-");
					NBString_concatUI64(dst, avg / 1000);
					NBString_concat(dst, ".");
					NBString_concatUI64(dst, (avg % 1000) / 100);
					NBString_concatUI64(dst, (avg % 100) / 10);
				}
				//
				NBString_concat(dst, "-");
				NBString_concatUI64(dst, obj->notifTime.usMax / 1000);
				NBString_concat(dst, ".");
				NBString_concatUI64(dst, (obj->notifTime.usMax % 1000) / 100);
				NBString_concatUI64(dst, (obj->notifTime.usMax % 100) / 10);
				//
				NBString_concat(dst, "ms)");
				opened = TRUE;
			}
			//ticks
			{
				if(opened) NBString_concat(dst, ", ");
				NBString_concatUI64(dst, obj->count);
				NBString_concat(dst, " ticks");
				opened = TRUE;
			}
		}
	}
}

//Buffers

void TSThreadStats_applyUpdate(STTSThreadStatsRef ref, const STTSThreadStatsUpd* upd){
	STTSThreadStatsOpq* opq = (STTSThreadStatsOpq*)ref.opaque;
	NBASSERT(TSThreadStats_isClass(ref))
	if(upd != NULL){
		if(upd->ticks.count != 0){
			NBObject_lock(opq);
			//
			{
				//loaded
				//
				//accum
				{
					if(opq->data.accum.ticks.count == 0){
						opq->data.accum.ticks.pollTime.usMin = upd->ticks.pollTime.usMin;
						opq->data.accum.ticks.pollTime.usMax = upd->ticks.pollTime.usMax;
						opq->data.accum.ticks.pollTime.usTotal = upd->ticks.pollTime.usTotal;
						//
						opq->data.accum.ticks.notifTime.usMin = upd->ticks.notifTime.usMin;
						opq->data.accum.ticks.notifTime.usMax = upd->ticks.notifTime.usMax;
						opq->data.accum.ticks.notifTime.usTotal = upd->ticks.notifTime.usTotal;
					} else {
						if(opq->data.accum.ticks.pollTime.usMin > upd->ticks.pollTime.usMin) opq->data.accum.ticks.pollTime.usMin = upd->ticks.pollTime.usMin;
						if(opq->data.accum.ticks.pollTime.usMax < upd->ticks.pollTime.usMax) opq->data.accum.ticks.pollTime.usMax = upd->ticks.pollTime.usMax;
						opq->data.accum.ticks.pollTime.usTotal += upd->ticks.pollTime.usTotal;
						//
						if(opq->data.accum.ticks.notifTime.usMin > upd->ticks.notifTime.usMin) opq->data.accum.ticks.notifTime.usMin = upd->ticks.notifTime.usMin;
						if(opq->data.accum.ticks.notifTime.usMax < upd->ticks.notifTime.usMax) opq->data.accum.ticks.notifTime.usMax = upd->ticks.notifTime.usMax;
						opq->data.accum.ticks.notifTime.usTotal += upd->ticks.notifTime.usTotal;
					}
					opq->data.accum.ticks.count += upd->ticks.count;
				}
				//total
				{
					if(opq->data.total.ticks.count == 0){
						opq->data.total.ticks.pollTime.usMin = upd->ticks.pollTime.usMin;
						opq->data.total.ticks.pollTime.usMax = upd->ticks.pollTime.usMax;
						opq->data.total.ticks.pollTime.usTotal = upd->ticks.pollTime.usTotal;
						//
						opq->data.total.ticks.notifTime.usMin = upd->ticks.notifTime.usMin;
						opq->data.total.ticks.notifTime.usMax = upd->ticks.notifTime.usMax;
						opq->data.total.ticks.notifTime.usTotal = upd->ticks.notifTime.usTotal;
					} else {
						if(opq->data.total.ticks.pollTime.usMin > upd->ticks.pollTime.usMin) opq->data.total.ticks.pollTime.usMin = upd->ticks.pollTime.usMin;
						if(opq->data.total.ticks.pollTime.usMax < upd->ticks.pollTime.usMax) opq->data.total.ticks.pollTime.usMax = upd->ticks.pollTime.usMax;
						opq->data.total.ticks.pollTime.usTotal += upd->ticks.pollTime.usTotal;
						//
						if(opq->data.total.ticks.notifTime.usMin > upd->ticks.notifTime.usMin) opq->data.total.ticks.notifTime.usMin = upd->ticks.notifTime.usMin;
						if(opq->data.total.ticks.notifTime.usMax < upd->ticks.notifTime.usMax) opq->data.total.ticks.notifTime.usMax = upd->ticks.notifTime.usMax;
						opq->data.total.ticks.notifTime.usTotal += upd->ticks.notifTime.usTotal;
					}
					opq->data.total.ticks.count += upd->ticks.count;
				}
			}
			//updCalls
			{
				//accum
				opq->data.accum.updCalls++;
				//total
				opq->data.total.updCalls++;
			}
			NBObject_unlock(opq);
		}
	}
}

//

void TSThreadStatsUpd_init(STTSThreadStatsUpd* obj){
	NBMemory_setZeroSt(*obj, STTSThreadStatsUpd);
}

void TSThreadStatsUpd_release(STTSThreadStatsUpd* obj){
	//
}

void TSThreadStatsUpd_reset(STTSThreadStatsUpd* obj){
	NBMemory_setZeroSt(*obj, STTSThreadStatsUpd);
}

void TSThreadStatsUpd_addPollTime(STTSThreadStatsUpd* obj, const UI64 usecs){
	if(obj->ticks.count == 0){
		obj->ticks.pollTime.usMin = obj->ticks.pollTime.usMax = usecs;
		obj->ticks.pollTime.usTotal = usecs;
	} else {
		if(obj->ticks.pollTime.usMin > usecs) obj->ticks.pollTime.usMin = usecs;
		if(obj->ticks.pollTime.usMax < usecs) obj->ticks.pollTime.usMax = usecs;
		obj->ticks.pollTime.usTotal += usecs;
	}
}

void TSThreadStatsUpd_addNotifTime(STTSThreadStatsUpd* obj, const UI64 usecs){
	if(obj->ticks.count == 0){
		obj->ticks.notifTime.usMin = obj->ticks.notifTime.usMax = usecs;
		obj->ticks.notifTime.usTotal = usecs;
	} else {
		if(obj->ticks.notifTime.usMin > usecs) obj->ticks.notifTime.usMin = usecs;
		if(obj->ticks.notifTime.usMax < usecs) obj->ticks.notifTime.usMax = usecs;
		obj->ticks.notifTime.usTotal += usecs;
	}
}

