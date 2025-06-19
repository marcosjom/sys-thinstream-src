//
//  TSThreadStats.h
//  thinstream
//
//  Created by Marcos Ortega on 8/2/22.
//

#ifndef TSThreadStats_h
#define TSThreadStats_h

#include "nb/NBFrameworkDefs.h"
//
#include "nb/NBObject.h"
#include "nb/core/NBString.h"
#include "nb/core/NBLog.h"

#ifdef __cplusplus
extern "C" {
#endif

//TSThreadStatsTime

typedef struct STTSThreadStatsTime_ {
	UI64					usMin; //micro-secs
	UI64					usMax; //micro-secs
	UI64					usTotal; //micro-secs
} STTSThreadStatsTime;

//TSThreadStatsTicks

typedef struct STTSThreadStatsTicks_ {
	UI64					count;
	STTSThreadStatsTime		pollTime;
	STTSThreadStatsTime		notifTime;
} STTSThreadStatsTicks;

//TSThreadStatsState

typedef struct STTSThreadStatsState_ {
	STTSThreadStatsTicks	ticks;
	UI64					updCalls;
} STTSThreadStatsState;

typedef struct STTSThreadStatsData_ {
	STTSThreadStatsState	loaded;		//loaded
	STTSThreadStatsState	accum;
	STTSThreadStatsState	total;
} STTSThreadStatsData;

//Update

typedef struct STTSThreadStatsUpd_ {
	STTSThreadStatsTicks	ticks;	//ticks
} STTSThreadStatsUpd;

void TSThreadStatsUpd_init(STTSThreadStatsUpd* obj);
void TSThreadStatsUpd_release(STTSThreadStatsUpd* obj);
void TSThreadStatsUpd_reset(STTSThreadStatsUpd* obj);
void TSThreadStatsUpd_addPollTime(STTSThreadStatsUpd* obj, const UI64 usecs);
void TSThreadStatsUpd_addNotifTime(STTSThreadStatsUpd* obj, const UI64 usecs);

//------------------
//- TSThreadStats
//------------------

NB_OBJREF_HEADER(TSThreadStats)

//

STTSThreadStatsData TSThreadStats_getData(STTSThreadStatsRef ref, const BOOL resetAccum);
void TSThreadStats_concat(STTSThreadStatsRef ref, const ENNBLogLevel logLvl, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst, const BOOL resetAccum);
void TSThreadStats_concatData(const STTSThreadStatsData* obj, const ENNBLogLevel logLvl, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst);
void TSThreadStats_concatState(const STTSThreadStatsState* obj, const ENNBLogLevel logLvl, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst);
void TSThreadStats_concatTicks(const STTSThreadStatsTicks* obj, const ENNBLogLevel logLvl, const BOOL ignoreEmpties, STNBString* dst);

//Buffers
void TSThreadStats_applyUpdate(STTSThreadStatsRef ref, const STTSThreadStatsUpd* upd);

#endif /* TSThreadStats_h */
