//
//  AUSceneContentI.h
//  thinstream
//
//  Created by Marcos Ortega on 9/2/18.
//

#ifndef AUSceneContentI_h
#define AUSceneContentI_h

#include "AUAnimadorObjetoEscena.h"
#include "nb/core/NBCallback.h"
#include "visual/AUSceneBarStatus.h"

typedef enum ENDRScrollSideH_ {
	ENDRScrollSideH_Left = 0,
	ENDRScrollSideH_Right,
	ENDRScrollSideH_Count
} ENDRScrollSideH;

typedef enum ENDRScrollSideV_ {
	ENDRScrollSideV_Top = 0,
	ENDRScrollSideV_Bottom,
	ENDRScrollSideV_Count
} ENDRScrollSideV;

class AUSceneContentI {
	public:
		//
		virtual void		setResizedCallback(STNBCallback* parentCallback) = 0;
		virtual void		setTouchInheritor(AUEscenaObjeto* touchInheritor) = 0;
		virtual BOOL		getStatusBarDef(STTSSceneBarStatus* dst) = 0;
		virtual BOOL		execBarBackCmd() = 0;
		virtual void		execBarIconCmd(const SI32 iIcon) = 0;
		virtual void		applySearchFilter(const char** wordsLwr, const SI32 wordsLwrSz) = 0;
		virtual void		applyAreaFilter(const STNBAABox outter, const STNBAABox inner) = 0;
		virtual BOOL		applyScaledSize(const STNBSize sizeBase, const STNBSize sizeScaled, const bool notifyChange, AUAnimadorObjetoEscena* animator, const float secsAnim) = 0;
		virtual STNBSize	getScaledSize() = 0;
		virtual ENDRScrollSideH getPreferedScrollH() = 0;
		virtual ENDRScrollSideV getPreferedScrollV() = 0;
		virtual void*		anchorCreate(const STNBPoint lclPos) = 0;
		virtual void*		anchorClone(const void* anchorRef) = 0;
		virtual void*		anchorToFocusClone(float* dstSecsDur) = 0;
		virtual void		anchorToFocusClear() = 0;
		virtual STNBAABox	anchorGetBox(void* anchorRef, float* dstScalePref) = 0;
		virtual void		anchorDestroy(void* anchorRef) = 0;
		virtual void		scrollToTop(const float secs) = 0;
		
};

#endif /* AUSceneContentI_h */
