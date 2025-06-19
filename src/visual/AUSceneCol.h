//
//  AUSceneCol.hpp
//  thinstream
//
//  Created by Marcos Ortega on 9/2/18.
//

#ifndef AUSceneCol_hpp
#define AUSceneCol_hpp

#include "nb/core/NBCallback.h"
#include "AUEscenaContenedorLimitado.h"
#include "visual/AUSceneContentI.h"

typedef enum ENSceneColMarginsMode_ {
	ENSceneColMarginsMode_IncludeInLimits = 0,
	ENSceneColMarginsMode_ExcludeFromLimits,
	ENSceneColMarginsMode_Count
} ENSceneColMarginsMode;

typedef enum ENSceneColSide_ {
	ENSceneColSide_Left = 0,
	ENSceneColSide_Right,
	ENSceneColSide_Top,
	ENSceneColSide_Btm,
	ENSceneColSide_Count
} ENSceneColSide;

typedef struct STSceneColTouch_ {
	STGTouch*		ref;
	float			secsAcum;		//Time
	float			dstAccum2;		//Total movement accumulated in scene coords (squared)
	STNBPoint		lclPosStart;
	STNBPoint		lclPosTrack;	//Last track to determine orientation
	STNBPoint		lclPosCurr;
	STNBPoint		objTras;		//Traslation
} STSceneColTouch;

typedef struct STSceneColDblTap_ {
	float			secsDown;		//Secs countdown
} STSceneColDblTap;

typedef struct STSceneColScroll_ {
	STNBSize		deltaLast;
	STNBSize		deltaAccum;
	STNBPoint		deltaAccumSecs;
	STNBPoint		speed;				//cur scroll speed
} STSceneColScroll;

typedef struct STSceneColZoom_ {
	STNBPoint		objPosRelFocus;		//The center between two fingers
	STNBSize		objSizeStart;		//Size
	struct {
		float		scaleStart;			//Size
		float		scaleEnd;			//Size
		float		secsDur;
		float		secsCur;
	} anim;
} STSceneColZoom;

typedef struct STSceneColPanTo_ {
	//Allways to anchor
	struct {
		STNBPoint	posStart;
		STNBPoint	posEnd;
		float		secsDur;
		float		secsCur;
	} anim;
} STSceneColPanTo;

typedef struct STSceneColAnchor_ {
	void*			ref;
	STNBPoint		lclOrg;
} STSceneColAnchor;

class AUSceneCol
: public AUEscenaContenedorLimitado
, public NBAnimador
, public IEscuchadorTouchEscenaObjeto
, public AUEscenaListaItemI
{
	public:
		AUSceneCol(const SI32 iScene, AUEscenaObjeto* obj, AUSceneContentI* itfCol, AUEscenaListaItemI* itfItm, AUEscenaObjeto* touchInheritor, const float width, const float height, const NBMargenes margins, const ENSceneColMarginsMode marginsMode, const STNBColor8 bgColor);
		virtual ~AUSceneCol();
		//
		AUEscenaObjeto*			getObj() const;
		float					getWidth() const;
		float					getHeight() const;
		NBMargenes				getMargins() const;
		void					setTouchInheritor(AUEscenaObjeto* touchInheritor);
		//AUEscenaListaItemI
		AUEscenaObjeto*			itemObjetoEscena();
		void					organizarContenido(const float anchoParaContenido, const float altoVisible, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim);
		STListaItemDatos		refDatos();
		void					establecerRefDatos(const SI32 tipo, const SI32 valor);
		//
		static void				childResized(void* obj);
		void					organize();
		void					organize(const float width, const float height, const NBMargenes margins);
		void					updateMargins(const NBMargenes margins);
		void					setBorderVisible(const BOOL visible);
		BOOL					getStatusBarDef(STTSSceneBarStatus* dst);
		BOOL					execBarBackCmd();
		void					execBarIconCmd(const SI32 iIcon);
		void					applySearchFilter(const char** wordsLwr, const SI32 wordsLwrSz);
		//
		void					scrollToTop(const float secs);
		void					scrollToPoint(const STNBPoint pos, const float secs);
		//
		void					tickAnimacion(float segsTranscurridos);
		//TOUCHES
		void					touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto);
		void					touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
		void					touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
		//Scroll
		void					touchScrollApply(AUEscenaObjeto* objeto, const STNBPoint posScene, const STNBSize size, const BOOL animate);
		//Magnify
		void					touchMagnifyApply(AUEscenaObjeto* objeto, const STNBPoint posScene, const float magnification, const BOOL isSmarthMag);
		//
		AUOBJMETODOS_CLASESID_DEFINICION
		AUOBJMETODOS_CLASESNOMBRES_DEFINICION
		AUOBJMETODOS_CLONAR_DEFINICION
	protected:
		SI32					_iScene;
		STNBCallback			_chldResizedCallbck;
		SI32					_chldResizedCount;
		AUEscenaSprite*			_bg;
		AUEscenaFigura*			_fg;			//border and layer for disabling touches
		STNBSize				_objSizeBase;	//Size defined for the obj
		AUEscenaObjeto*			_obj;
		STListaItemDatos		_refData;
		AUSceneContentI*		_itfCol;
		AUEscenaListaItemI*		_itfItm;
		AUEscenaObjeto*			_touchInheritor;	//Objet to pass touches
		float					_width;
		float					_height;
		NBMargenes				_margins;
		ENSceneColMarginsMode	_marginsMode;
		STNBColor8				_bgColor;
		//
		BOOL					_fixedToSideH[ENDRScrollSideH_Count];
		BOOL					_fixedToSideV[ENDRScrollSideV_Count];
		//Touch
		struct {
			STSceneColTouch		first;	//First touch
			STSceneColTouch		second;	//Second touch
			STSceneColDblTap	dblTap;	//Double tap track
			STSceneColScroll	scroll;	//Scroll anim
			STSceneColZoom		zoom;	//Zoom anim
			STSceneColPanTo		panTo;	//Pan to anchor
			STSceneColAnchor	anchor;	//Anchor for animations
		} _touch;
		//Scroll
		struct {
			//Horizontal
			struct {
				float					marginLeft;			//Scroll bar empty space
				float					marginRght;			//Scroll bar empty space
				float					objLimitMax;		//Reference for alignmet right
				float					objLimitMin;		//Reference for alignmet left
				float					objSize;			//Object size
				float					visibleSize;		//Length for content
				float					curSpeed;			//cur scroll speed
				struct {
					UI8					maxAlpha8;			//Alpha
					float				secsForAppear;		//secs required for start showing
					float				secsFullVisible;	//visible
					float				secsForDisapr;		//secs required for start showing
					float				secsAccum;			//visible
					AUEscenaSpriteElastico*	obj;			//Object
				} bar;
			} h;
			//Vertical
			struct {
				float					marginTop;			//Scroll bar empty space
				float					marginBtm;			//Scroll bar empty space
				float					objLimitMax;		//Reference for alignmet top
				float					objLimitMin;		//Reference for alignmet btm
				float					objSize;			//Object size
				float					visibleSize;		//Length for content
				float					curSpeed;			//cur scroll speed
				struct {
					UI8					maxAlpha8;			//Alpha
					float				secsForAppear;		//secs required for start showing
					float				secsFullVisible;	//visible
					float				secsForDisapr;		//secs required for start showing
					float				secsAccum;			//visible
					AUEscenaSpriteElastico*	obj;			//Object
				} bar;
			} v;
		} _scroll;
		//
		void					privOrganize(const float width, const float height, const NBMargenes margins);
		void					privScroll(const float deltaX, const float deltaY, const BOOL showScrollbar);
		void					privScrollTo(const float xPos, const float yPos, const BOOL showScrollbar);
		void					privScrollToSides(const ENDRScrollSideH sideH, const ENDRScrollSideV sideV);
		void					privScrollAutoAdjust();
		void					privStartAnimToScale(const float scale);
};

#endif /* AUSceneCol_hpp */
