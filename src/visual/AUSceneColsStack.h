//
//  AUSceneColsStacksStack.hpp
//  thinstream
//
//  Created by Marcos Ortega on 9/2/18.
//

#ifndef AUSceneColsStack_hpp
#define AUSceneColsStack_hpp

#include "AUEscenaContenedorLimitado.h"
#include "visual/AUSceneContentI.h"
#include "visual/AUSceneCol.h"

typedef enum ENSceneColStackPushMode_ {
	ENSceneColStackPushMode_Add = 0,
	ENSceneColStackPushMode_Replace,
	ENSceneColStackPushMode_Count
} ENSceneColStackPushMode;

typedef struct STSceneColsStackItm_ {
	AUSceneCol* 	obj;
	STNBPoint		deltaPos;
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
} STSceneColsStackItm;

class AUSceneColsStack
: public AUEscenaContenedorLimitado
, public NBAnimador
, public IEscuchadorTouchEscenaObjeto
{
public:
	AUSceneColsStack(const SI32 iScene, AUEscenaObjeto* touchInheritor, const float width, const float height, const NBMargenes margins, const STNBColor8 defBgColor);
	virtual ~AUSceneColsStack();
	//
	float					getWidth() const;
	float					getHeight() const;
	NBMargenes				getMargins() const;
	//
	SI32					getSize() const;
	BOOL					push(AUEscenaObjeto* obj, AUSceneContentI* itfCol, AUEscenaListaItemI* itfItm, const ENSceneColSide animOutTo, const ENSceneColSide animInFrom, const ENSceneColStackPushMode pushMode, const ENSceneColMarginsMode marginsMode);
	BOOL					pop(const ENSceneColSide animOutTo, const ENSceneColSide animInFrom);
	AUEscenaObjeto*			getTopObj() const;
	BOOL					getStatusBarDef(STTSSceneBarStatus* dst);
	BOOL					execBarBackCmd();
	void					execBarIconCmd(const SI32 iIcon);
	void					applySearchFilter(const char** wordsLwr, const SI32 wordsLwrSz);
	//
	void					organize();
	void					organize(const float width, const float height, const NBMargenes margins);
	void					updateMargins(const NBMargenes margins);
	void					setBorderVisible(const BOOL visible);
	void					scrollToTop(const float secs);
	//
	void					tickAnimacion(float segsTranscurridos);
	//TOUCHES
	void					touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto);
	void					touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
	void					touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
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
	STNBArray				_stack;		//STSceneColsStackItm
	STNBArray				_removing;	//STSceneColsStackItm
	//
	AUEscenaContenedorLimitado* _lyr;
	AUEscenaFigura*			_fg;	//border and layer for disabling touches
	struct {
		STGTouch*			first;
		STNBPoint			startTouch;
	} _touch;
	//
	STNBPoint				privDeltaForSize(const ENSceneColSide side, const float rel);
	void					privOrganize(const float width, const float height, const NBMargenes margins);
};

#endif /* AUSceneColsStacksStack_hpp */
