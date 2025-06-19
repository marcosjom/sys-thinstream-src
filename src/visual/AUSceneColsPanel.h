//
//  AUSceneColsPanelsStack.hpp
//  thinstream
//
//  Created by Marcos Ortega on 9/2/18.
//

#ifndef AUSceneColsPanel_hpp
#define AUSceneColsPanel_hpp

#include "AUEscenaContenedorLimitado.h"
#include "visual/AUSceneContentI.h"
#include "visual/AUSceneCol.h"
#include "visual/AUSceneColsStack.h"

typedef struct STSceneColsPanelItm_ {
	AUSceneColsStack*	obj;
	STNBPoint			deltaPos;
	struct {
		float			secsDur;	//animation duration
		float			secsCur;	//animation progress
		struct {
			ENSceneColSide	side;
			float			relPos;
			float			relColor;
		} from;
		struct {
			ENSceneColSide	side;
			float			relPos;
			float			relColor;
		} to;
		BOOL			acel;		//accelerated or desacelerated
	} anim;
} STSceneColsPanelItm;

class AUSceneColsPanel
: public AUEscenaContenedorLimitado
, public NBAnimador
{
public:
	AUSceneColsPanel(const SI32 iScene, AUEscenaObjeto* touchInheritor, const float width, const float height, const NBMargenes margins, const STNBColor8 defBgColor, const SI32 colsStackSz);
	virtual ~AUSceneColsPanel();
	//
	float					getWidth() const;
	float					getHeight() const;
	NBMargenes				getMargins() const;
	//
	SI32					getCurrentStackIdx() const;
	SI32					getCurrentStackSz() const;
	void					setCurrentStack(const SI32 idx, const ENSceneColSide animOutTo, const ENSceneColSide animInFrom);
	BOOL					push(AUEscenaObjeto* obj, AUSceneContentI* itfCol, AUEscenaListaItemI* itfItm, const ENSceneColSide animOutTo, const ENSceneColSide animInFrom, const ENSceneColStackPushMode pushMode, const ENSceneColMarginsMode marginsMode);
	BOOL					pop(const ENSceneColSide animOutTo, const ENSceneColSide animInFrom);
	BOOL					getStatusBarDef(STTSSceneBarStatus* dst);
	BOOL					execBarBackCmd();
	void					execBarIconCmd(const SI32 iIcon);
	void					applySearchFilter(const char** wordsLwr, const SI32 wordsLwrSz);
	//
	void					organize();
	void					organize(const float width, const float height, const NBMargenes margins);
	void					updateMargins(const NBMargenes margins);
	void					scrollToTop(const float secs);
	//
	void					tickAnimacion(float segsTranscurridos);
	//
	AUOBJMETODOS_CLASESID_DEFINICION
	AUOBJMETODOS_CLASESNOMBRES_DEFINICION
	AUOBJMETODOS_CLONAR_DEFINICION
protected:
	SI32					_iScene;
	AUEscenaObjeto*			_touchInheritor;
	float					_width;
	float					_height;
	NBMargenes				_margins;
	STNBColor8				_defBgColor;
	struct {
		SI32				iCurrent;	//current stack
		STNBArray			array;		//STSceneColsPanelItm
	} _stacks;
	//
	AUEscenaContenedor*		_lyr;
	//
	STNBPoint				privDeltaForSize(const ENSceneColSide side, const float rel);
	void					privOrganize(const float width, const float height, const NBMargenes margins);
};

#endif /* AUSceneColsPanelsStack_hpp */
