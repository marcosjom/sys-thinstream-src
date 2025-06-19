//
//  TSClientRequesterConnStats.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSClientRequesterConnStats_h
#define TSClientRequesterConnStats_h

#include "nb/core/NBMemory.h"
#include "nb/core/NBString.h"
#include "nb/core/NBLog.h"
#include "core/config/TSCfgTelemetry.h"

#ifdef __cplusplus
extern "C" {
#endif

//TSClientRequesterConnStatsTime

typedef struct STTSClientRequesterConnStatsTime_ {
    UI32    count;
    UI32    msMin;
    UI32    msMax;
    UI32    msTotal;
} STTSClientRequesterConnStatsTime;

const STNBStructMap* TSClientRequesterConnStatsTime_getSharedStructMap(void);

//TSClientRequesterConnStatsNaluTimes

typedef struct STTSClientRequesterConnStatsNaluTimes_ {
    STTSClientRequesterConnStatsTime wait;  //wait (gap-time between two sequential units)
    STTSClientRequesterConnStatsTime arrival; //arrival (time between first and last packet of unit)
} STTSClientRequesterConnStatsNaluTimes;

const STNBStructMap* TSClientRequesterConnStatsNaluTimes_getSharedStructMap(void);

//TSClientRequesterConnStatsNalu

typedef struct STTSClientRequesterConnStatsNalu_ {
	UI32	count;
	UI64	bytesCount;
    STTSClientRequesterConnStatsNaluTimes times;
} STTSClientRequesterConnStatsNalu;

const STNBStructMap* TSClientRequesterConnStatsNalu_getSharedStructMap(void);

//Update

typedef struct STTSClientRequesterConnStatsUpd_ {
	//allocated
	struct {
		UI32		newAllocs;
		BOOL		populated; //min and max are populated
		UI32		min;
		UI32		max;
	} buffs;
	//processed
	struct {
		UI32								total;
		STTSClientRequesterConnStatsNalu	byNalu[32];
	} processed;
	//ignored
	struct {
		UI32								total;
		STTSClientRequesterConnStatsNalu	byNalu[32];
	} ignored;
	//malformed
	struct {
		STTSClientRequesterConnStatsNalu	total;
		STTSClientRequesterConnStatsNalu	byNalu[32];
		STTSClientRequesterConnStatsNalu	unknown;
	} malformed;
} STTSClientRequesterConnStatsUpd;

void TSClientRequesterConnStatsUpd_init(STTSClientRequesterConnStatsUpd* obj);
void TSClientRequesterConnStatsUpd_release(STTSClientRequesterConnStatsUpd* obj);
void TSClientRequesterConnStatsUpd_naluProcessed(STTSClientRequesterConnStatsUpd* obj, const UI8 iNalType, const UI64 bytesCount, const UI32 msWait, const UI32 msArrival);
void TSClientRequesterConnStatsUpd_naluIgnored(STTSClientRequesterConnStatsUpd* obj, const UI8 iNalType, const UI64 bytesCount);
void TSClientRequesterConnStatsUpd_naluMalformed(STTSClientRequesterConnStatsUpd* obj, const UI8 iNalType, const UI64 bytesCount);
void TSClientRequesterConnStatsUpd_unknownMalformed(STTSClientRequesterConnStatsUpd* obj, const UI64 bytesCount);

//TSClientRequesterConnStatsNalus

typedef struct STTSClientRequesterConnStatsNalusBuffs_ {
    UI32        newAllocs;
    BOOL        populated; //min and max are populated
    UI32        min;
    UI32        max;
} STTSClientRequesterConnStatsNalusBuffs;

const STNBStructMap* TSClientRequesterConnStatsNalusBuffs_getSharedStructMap(void);

//TSClientRequesterConnStatsNalus

typedef struct STTSClientRequesterConnStatsNalus_ {
    STTSClientRequesterConnStatsNalusBuffs buffs; //stream-units allocated
	STTSClientRequesterConnStatsNalu	byNalu[32];
	STTSClientRequesterConnStatsNalu	ignored[32];
	STTSClientRequesterConnStatsNalu	malform[32];	//packets lost
	STTSClientRequesterConnStatsNalu	malformUnkw;	//packets lost (unkown nalType)
} STTSClientRequesterConnStatsNalus;

const STNBStructMap* TSClientRequesterConnStatsNalus_getSharedStructMap(void);

//TSClientRequesterConnStatsState

typedef struct STTSClientRequesterConnStatsState_ {
	STTSClientRequesterConnStatsNalus	nalus;
	UI64							    updCalls;
} STTSClientRequesterConnStatsState;

const STNBStructMap* TSClientRequesterConnStatsState_getSharedStructMap(void);

//TSClientRequesterConnStatsData

typedef struct STTSClientRequesterConnStatsData_ {
	STTSClientRequesterConnStatsState	loaded;		//loaded
	STTSClientRequesterConnStatsState	accum;		//accum
	STTSClientRequesterConnStatsState	total;		//total
} STTSClientRequesterConnStatsData;

const STNBStructMap* TSClientRequesterConnStatsData_getSharedStructMap(void);

//TSClientRequesterConnStats

typedef struct STTSClientRequesterConnStats_ {
	void* opaque;
} STTSClientRequesterConnStats;

void TSClientRequesterConnStats_init(STTSClientRequesterConnStats* obj);
void TSClientRequesterConnStats_release(STTSClientRequesterConnStats* obj);

//Data
STTSClientRequesterConnStatsData TSClientRequesterConnStats_getData(STTSClientRequesterConnStats* obj, const BOOL resetAccum);
void TSClientRequesterConnStats_concat(STTSClientRequesterConnStats* obj, const STTSCfgTelemetryStreams* cfg, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst, const BOOL resetAccum);
void TSClientRequesterConnStats_concatData(const STTSClientRequesterConnStatsData* obj, const STTSCfgTelemetryStreams* cfg, const BOOL loaded, const BOOL accum, const BOOL total, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst);
void TSClientRequesterConnStats_concatState(const STTSClientRequesterConnStatsState* obj, const STTSCfgTelemetryStreams* cfg, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst);
void TSClientRequesterConnStats_concatNalus(const STTSClientRequesterConnStatsNalus* obj, const STTSCfgTelemetryStreams* cfg, const char* prefixFirst, const char* prefixOthers, const BOOL ignoreEmpties, STNBString* dst);

//Upd
void TSClientRequesterConnStats_addUpd(STTSClientRequesterConnStats* obj, const STTSClientRequesterConnStatsUpd* upd);

#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSClientRequesterConnStats_h */
