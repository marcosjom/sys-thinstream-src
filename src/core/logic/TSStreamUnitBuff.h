//
//  TSStreamUnitBuff.h
//  thinstream
//
//  Created by Marcos Ortega on 9/3/19.
//

#ifndef TSStreamUnitBuff_h
#define TSStreamUnitBuff_h

#include "nb/core/NBDataPtr.h"
#include "nb/core/NBArray.h"
//
#include "core/logic/TSStreamUnit.h"

#ifdef __cplusplus
extern "C" {
#endif

//stream unit

NB_OBJREF_HEADER(TSStreamUnitBuff)

//listener
STTSStreamUnit TSStreamUnitBuff_setDataAsOwningUnit(STTSStreamUnitBuffRef ref, const void* origin, const UI32 unitId, const STTSStreamUnitBuffHdr* hdr, STNBArray* chunks, const BOOL swapBuffers/*, const STNBAvcPicDescBase* knownProps*/, STNBDataPtrReleaser* optPtrsReleaser);

//data
BOOL TSStreamUnitBuff_setData(STTSStreamUnitBuffRef ref, const void* origin, const UI32 unitId, const STTSStreamUnitBuffHdr* hdr, STNBArray* chunks, const BOOL swapBuffers/*, const STNBAvcPicDescBase* knownProps*/, STNBDataPtrReleaser* optPtrsReleaser);
void TSStreamUnitBuff_empty(STTSStreamUnitBuffRef ref, STNBDataPtrReleaser* optPtrsReleaser);

#ifdef __cplusplus
} //extern "C"
#endif
		
#endif /* TSStreamDefs_h */
