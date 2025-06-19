//
//  AUSceneCol.cpp
//  thinstream
//
//  Created by Marcos Ortega on 9/2/18.
//

#include "visual/TSVisualPch.h"
#include "visual/AUSceneCol.h"
#include "visual/TSColors.h"

#define AUSCENE_COL_DBL_TAP_MAX_TAP_DST_INCH	0.05f	//max inches of total movement allowed at touch to bec] considerated a tap
#define AUSCENE_COL_DBL_TAP_MAX_TAP_SECS		0.15f	//max time tap-down allowed
#define AUSCENE_COL_DBL_TAP_MAX_WAIT_SECS		0.50f	//max time between taps allowed

AUSceneCol::AUSceneCol(const SI32 iScene, AUEscenaObjeto* obj, AUSceneContentI* itfCol, AUEscenaListaItemI* itfItm, AUEscenaObjeto* touchInheritor, const float width, const float height, const NBMargenes margins, const ENSceneColMarginsMode marginsMode, const STNBColor8 bgColor)
: AUEscenaContenedorLimitado()
, _iScene(iScene)
{
	NB_DEFINE_NOMBRE_PUNTERO(this, "AUSceneCol")
	//Child resized callback
	{
		STNBCallbackMethod m;
		NBMemory_setZeroSt(m, STNBCallbackMethod);
		m.obj		= this;
		m.retain	= AUObjeto_retain_;
		m.release	= AUObjeto_release_;
		m.callback	= AUSceneCol::childResized;
		NBCallback_initWithMethods(&_chldResizedCallbck, &m);
		_chldResizedCount = 0;
	}
	//bg
	{
		_bg			= new(this) AUEscenaSprite();
		_bg->establecerMultiplicadorColor8(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
		_bg->establecerEscuchadorTouches(this, this);
		_bg->setTouchScrollEnabled(TRUE);
		_bg->setTouchMagnifyEnabled(TRUE);
		this->agregarObjetoEscena(_bg);
	}
	//obj
	{
		NBMemory_setZero(_objSizeBase);
		NBMemory_setZero(_refData);
		_obj		= obj;
		_itfCol		= itfCol;
		_itfItm		= itfItm;
		if(_itfCol != NULL){
			_itfCol->setResizedCallback(&_chldResizedCallbck);
			_itfCol->setTouchInheritor(_bg);
		}
		if(_obj != NULL){
			this->agregarObjetoEscena(_obj);
			_obj->retener(NB_RETENEDOR_THIS);
		}
	}
	//fg
	{
		_fg	= new(this) AUEscenaFigura(ENEscenaFiguraTipo_PoligonoCerrado);
		_fg->establecerModo(ENEscenaFiguraModo_Borde);
		_fg->agregarCoordenadaLocal(0, 0);
		_fg->agregarCoordenadaLocal(width, 0);
		_fg->agregarCoordenadaLocal(width, -height);
		_fg->agregarCoordenadaLocal(0, -height);
		_fg->setVisibleAndAwake(FALSE);
		_fg->establecerMultiplicadorColor8(255, 255, 255, 0);
		_fg->establecerEscuchadorTouches(this, this);
		this->agregarObjetoEscena(_fg);
	}
	_touchInheritor	= touchInheritor;
	_width			= 0.0f;
	_height			= 0.0f;
	_margins		= margins;
	_marginsMode	= marginsMode;
	_bgColor		= bgColor;
	NBMemory_setZero(_fixedToSideH);
	NBMemory_setZero(_fixedToSideV);
	ENDRScrollSideH prefH = ENDRScrollSideH_Count;
	ENDRScrollSideV prefV = ENDRScrollSideV_Count;
	if(_itfCol != NULL){
		prefH = _itfCol->getPreferedScrollH();
		prefV = _itfCol->getPreferedScrollV();
		if(prefH >= 0 && prefH < ENDRScrollSideH_Count){
			_fixedToSideH[prefH] = TRUE;
		}
		if(prefV >= 0 && prefV < ENDRScrollSideV_Count){
			_fixedToSideV[prefV] = TRUE;
		}
	}
	//Touch
	{
		NBMemory_setZero(_touch);
	}
	//Scroll
	{
		AUTextura* barTex	= NBGestorTexturas::texturaDesdeArchivoPNG("thinstream/scrolllbarMed.png");
		STNBColor8 barColor = TSColors::colorDef(ENTSColor_MainColor)->normal; barColor.a = TS_COLOR_SCROLL_BAR_ALPHA8_COLS;
		//Horizontal
		{
			NBMemory_setZero(_scroll.h);
			_scroll.h.objLimitMax	= 0.0f;
			_scroll.h.objLimitMin	= 0.0f;
			_scroll.h.marginLeft	= 0.0f;
			_scroll.h.marginRght	= 0.0f;
			_scroll.h.objSize		= 0.0f;
			_scroll.h.visibleSize	= 0.0f;
			_scroll.h.curSpeed		= 0.0f;	//cur scroll speed
			//
			_scroll.h.bar.maxAlpha8			= barColor.a;
			_scroll.h.bar.secsForAppear		= 0.20f;
			_scroll.h.bar.secsFullVisible	= 0.40f;
			_scroll.h.bar.secsForDisapr		= 0.20f;
			_scroll.h.bar.secsAccum			= _scroll.h.bar.secsForAppear + _scroll.h.bar.secsFullVisible + _scroll.h.bar.secsForDisapr; //Start hidden
			_scroll.h.bar.obj				= new AUEscenaSpriteElastico(barTex);
			_scroll.h.bar.obj->establecerMultiplicadorColor8(barColor.r, barColor.g, barColor.b, _scroll.h.bar.maxAlpha8);
			_scroll.h.bar.obj->establecerVisible(false);
			this->agregarObjetoEscena(_scroll.h.bar.obj);
		}
		//Vertical
		{
			NBMemory_setZero(_scroll.v);
			_scroll.v.objLimitMax	= 0.0f;
			_scroll.v.objLimitMin	= 0.0f;
			_scroll.v.marginTop		= 0.0f;
			_scroll.v.marginBtm		= 0.0f;
			_scroll.v.objSize		= 0.0f;
			_scroll.v.visibleSize	= 0.0f;
			_scroll.v.curSpeed		= 0.0f;	//cur scroll speed
			//
			_scroll.v.bar.maxAlpha8			= barColor.a;
			_scroll.v.bar.secsForAppear		= 0.20f;
			_scroll.v.bar.secsFullVisible	= 0.40f;
			_scroll.v.bar.secsForDisapr		= 0.20f;
			_scroll.v.bar.secsAccum			= _scroll.v.bar.secsForAppear + _scroll.v.bar.secsFullVisible + _scroll.v.bar.secsForDisapr; //Start hidden
			_scroll.v.bar.obj				= new AUEscenaSpriteElastico(barTex);
			_scroll.v.bar.obj->establecerMultiplicadorColor8(barColor.r, barColor.g, barColor.b, _scroll.v.bar.maxAlpha8);
			_scroll.v.bar.obj->establecerVisible(false);
			this->agregarObjetoEscena(_scroll.v.bar.obj);
		}
	}
	//
	this->privOrganize(width, height, margins);
	this->privScrollToSides(prefH, prefV);
	//
	NBGestorAnimadores::agregarAnimador(this, this);
}

AUSceneCol::~AUSceneCol(){
	AU_OBJETO_ASSERT_IS_ALIVE_THIS
	NB_GESTOR_AUOBJETOS_VALIDATE_ALL_OBJETS_TO_BE_ALIVE()
	NBGestorAnimadores::quitarAnimador(this);
	NBCallback_release(&_chldResizedCallbck);
	//Touch
	{
		if(_touch.anchor.ref != NULL){
			_itfCol->anchorDestroy(_touch.anchor.ref);
			_touch.anchor.ref = NULL;
		}
		NBMemory_setZero(_touch);
	}
	//Scroll
	{
		if(_scroll.h.bar.obj != NULL) _scroll.h.bar.obj->liberar(NB_RETENEDOR_THIS); _scroll.h.bar.obj = NULL;
		if(_scroll.v.bar.obj != NULL) _scroll.v.bar.obj->liberar(NB_RETENEDOR_THIS); _scroll.v.bar.obj = NULL;
	}
	//
	if(_bg != NULL) _bg->liberar(NB_RETENEDOR_THIS); _bg = NULL;
	if(_fg != NULL) _fg->liberar(NB_RETENEDOR_THIS); _fg = NULL;
	if(_obj != NULL) _obj->liberar(NB_RETENEDOR_THIS); _obj = NULL;
	_itfCol = NULL;
	_itfItm = NULL;
	_touchInheritor = NULL;
	NB_GESTOR_AUOBJETOS_VALIDATE_ALL_OBJETS_TO_BE_ALIVE()
}

//

AUEscenaObjeto* AUSceneCol::getObj() const {
	return _obj;
}

float AUSceneCol::getWidth() const {
	return _width;
}

float AUSceneCol::getHeight() const {
	return _height;
}

NBMargenes AUSceneCol::getMargins() const{
	return _margins;
}

void AUSceneCol::setTouchInheritor(AUEscenaObjeto* touchInheritor){
	_touchInheritor = touchInheritor;
}

//AUEscenaListaItemI

AUEscenaObjeto* AUSceneCol::itemObjetoEscena(){
	return this;
}

void AUSceneCol::organizarContenido(const float anchoParaContenido, const float altoVisible, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim){
	this->privOrganize(anchoParaContenido, altoVisible, _margins);
}

STListaItemDatos AUSceneCol::refDatos() {
	return _refData;
}

void AUSceneCol::establecerRefDatos(const SI32 tipo, const SI32 valor){
	_refData.tipo	= tipo;
	_refData.valor	= valor;
}

//

void AUSceneCol::childResized(void* pObj){
	AUSceneCol* obj = (AUSceneCol*)pObj;
	if(obj != NULL){
		obj->privScrollAutoAdjust();
		obj->_chldResizedCount++;
	}
}

void AUSceneCol::organize(){
	this->privOrganize(_width, _height, _margins);
}

void AUSceneCol::organize(const float width, const float height, const NBMargenes margins){
	this->privOrganize(width, height, margins);
}

void AUSceneCol::privOrganize(const float widthP, const float heightP, const NBMargenes margins){
	//Content
	{
		const STNBSize objSizeBaseBefore = _objSizeBase;
		const float widthBefore			= _width;
		const float heightBefore		= _height;
		_objSizeBase.width				= widthP - margins.left - margins.right;
		_objSizeBase.height				= heightP - margins.top - margins.bottom;
		{
			void* anchorCenter	= NULL;
			//Save current center
			if(anchorCenter == NULL && widthBefore > 0.0f && heightBefore > 0.0f && (widthBefore != widthP || heightBefore != heightP)){
				const NBPunto pos = _obj->traslacion();
				const STNBPoint center = NBST_P(STNBPoint, (widthBefore * 0.5f) - pos.x, (heightBefore * -0.5f) - pos.y);
				anchorCenter = _itfCol->anchorCreate(center);
				//PRINTF_INFO("old-center(%f, %f).\n", center.x, center.y);
			}
			//Resize and organize
			{
				BOOL done = FALSE;
				//Streched resize
				if(!done && _itfCol != NULL){
					const STNBSize scaledSize = _itfCol->getScaledSize();
					if(objSizeBaseBefore.width > 0.0f && scaledSize.width > 0.0f){
						const float relSzBefore	= (scaledSize.width / objSizeBaseBefore.width);
						const float widthNew	= (_objSizeBase.width * relSzBefore);
						//Apply resize
						if(widthNew > 1.0f){
							if(_itfCol->applyScaledSize(_objSizeBase, NBST_P(STNBSize, widthNew, _objSizeBase.height), FALSE, NULL, 0.0f)){
								done = TRUE;
							}
						}
					}
				}
				//Simple resize
				if(!done && _itfItm != NULL){
					_itfItm->organizarContenido(_objSizeBase.width, _objSizeBase.height, false, NULL, 0.0f);
					done = TRUE;
				}
			}
			//Set new center
			if(anchorCenter != NULL && _itfCol != NULL){
				const STNBAABox box = _itfCol->anchorGetBox(anchorCenter, NULL);
				const STNBPoint boxCenter = NBST_P(STNBPoint, box.xMin + ((box.xMax - box.xMin) * 0.5f), box.yMin + ((box.yMax - box.yMin) * 0.5f));
				const STNBPoint newPos = NBST_P(STNBPoint, (widthP * 0.5f) - boxCenter.x, (heightP * -0.5f) - boxCenter.y);
				_obj->establecerTraslacion(newPos.x, newPos.y);
				//PRINTF_INFO("new-center(%f, %f).\n", boxCenter.x, boxCenter.y);
				//Release anchor
				_itfCol->anchorDestroy(anchorCenter);
				anchorCenter = NULL;
			}
		}
		//ToDo: remove (do not center columns). Verify?
		{
			//Center horizontally
			/*if(_obj != NULL){
			 const NBCajaAABB box = _obj->cajaAABBLocal();
			 _obj->establecerTraslacion(0.0f - box.xMin + ((width - (box.xMax - box.xMin)) * 0.5f), _obj->traslacion().y);
			 //PRINTF_INFO("Moved col-obj to(%f, %f) from-box-x(%f, %f)-y(%f, %f).\n", _obj->traslacion().x, _obj->traslacion().y, box.xMin, box.xMax, box.yMin, box.yMax);
			 }*/
		}
		//Scroll
		{
			const float szObjBeforeH = _scroll.v.objSize;
			const float szVisBeforeH = _scroll.v.visibleSize;
			const float szObjBeforeW = _scroll.h.objSize;
			const float szVisBeforeW = _scroll.h.visibleSize;
			//Calculate
			{
				_scroll.v.visibleSize	= _objSizeBase.height;
				_scroll.v.marginTop		= 0.0f;
				_scroll.v.marginBtm		= 0.0f;
				//
				_scroll.h.visibleSize	= _objSizeBase.width;
				_scroll.h.marginLeft	= 0.0f;
				_scroll.h.marginRght	= 0.0f;
				//
				if(_obj == NULL){
					//
					_scroll.v.objSize		= 0.0f;
					_scroll.v.objLimitMax	= 0.0f;
					_scroll.v.objLimitMin	= 0.0f;
					//
					_scroll.h.objSize		= 0.0f;
					_scroll.h.objLimitMax	= 0.0f;
					_scroll.h.objLimitMin	= 0.0f;
					//
				} else {
					const NBCajaAABB box	= _obj->cajaAABBLocal();
					//
					_scroll.v.objSize		= (box.yMax - box.yMin);
					_scroll.v.objLimitMax	= box.yMax;
					_scroll.v.objLimitMin	= box.yMin;
					//
					_scroll.h.objSize		= (box.xMax - box.xMin);
					_scroll.h.objLimitMax	= box.xMax;
					_scroll.h.objLimitMin	= box.xMin;
					//
				}
			}
			//Reset scroll-bar 'show' animations
			{
				if(szObjBeforeH != _scroll.v.objSize || szVisBeforeH != _scroll.v.visibleSize){
					if(_scroll.v.objSize > (_scroll.v.visibleSize + 1.0f)){
						_scroll.v.bar.secsAccum = 0.0f;
					}
				}
				if(szObjBeforeW != _scroll.h.objSize || szVisBeforeW != _scroll.h.visibleSize){
					if(_scroll.h.objSize > (_scroll.h.visibleSize + 1.0f)){
						_scroll.h.bar.secsAccum = 0.0f;
					}
				}
			}
		}
		/*{
			if(_content.txt != NULL){
				const float wAval	= anchoMax - _margins.left - _margins.right;
				const float hAval	= altoMax - _margins.top - _margins.bottom;
				const STNBRect col = NBST_P(STNBRect,0, 0, wAval, 0);
				_content.txt->organizarTextoEnCol(col);
				{
					const NBCajaAABB cajaCont = _content.txt->cajaAABBLocalCalculada();
					const float txtHeight = (cajaCont.yMax - cajaCont.yMin);
					if(txtHeight <= hAval || anchoMax <= 0.0f || altoMax <= 0.0f){
						this->establecerLimites(0.0f, (anchoMax <= 0 ? (cajaCont.xMax - cajaCont.xMin) : anchoMax), 0.0f - _margins.top - txtHeight - _margins.bottom, 0.0f);
						_content.txt->emptyVisualFilters();
						_content.txt->establecerTraslacion(_margins.left/ * - cajaCont.xMin* /, 0.0f - _margins.top - cajaCont.yMax);
						_scroll.v.objLimitMax		= cajaCont.yMax;
						_scroll.v.objLimitMin		= cajaCont.yMin;
						_scroll.v.marginTop			= _margins.top;
						_scroll.v.marginBtm			= _margins.bottom;
						_scroll.v.objSize			= txtHeight;
						_scroll.v.visibleSize		= txtHeight;
					} else {
						this->establecerLimites(0.0f, (anchoMax <= 0 ? (cajaCont.xMax - cajaCont.xMin) : anchoMax), 0.0f - _margins.top - hAval - _margins.bottom, 0.0f);
						_content.txt->emptyVisualFilters();
						_content.txt->pushVisualFilter(_content.txtVisualFilter);
						_content.txt->establecerTraslacion(_margins.left / *- cajaCont.xMin* /, 0.0f - _margins.top - cajaCont.yMax);
						_content.txtVisualFilter->setOuterLimits(0.0f, (anchoMax <= 0 ? (cajaCont.xMax - cajaCont.xMin) : anchoMax), 0.0f - _margins.top - hAval - _margins.bottom, 0.0f);
						_content.txtVisualFilter->setInnerLimits(_margins.left, (anchoMax <= 0 ? (cajaCont.xMax - cajaCont.xMin) : anchoMax) - _margins.right,  0.0f - _margins.top - hAval, 0.0f - _margins.top);
						_scroll.v.objLimitMax		= cajaCont.yMax;
						_scroll.v.objLimitMin		= cajaCont.yMin;
						_scroll.v.marginTop			= _margins.top;
						_scroll.v.marginBtm			= _margins.bottom;
						_scroll.v.objSize			= txtHeight;
						_scroll.v.visibleSize		= hAval;
					}
				}
				_lastWidth = anchoMax;
			}
		}*/
	}
	//Bg
	{
		_bg->redimensionar(0.0f, 0.0f, widthP, -heightP);
	}
	//Fg
	{
		_fg->moverVerticeHacia(0, 0, 0);
		_fg->moverVerticeHacia(1, widthP, 0);
		_fg->moverVerticeHacia(2, widthP, -heightP);
		_fg->moverVerticeHacia(3, 0, -heightP);
	}
	//Set limits
	{
		if(_marginsMode == ENSceneColMarginsMode_ExcludeFromLimits){
			this->establecerLimites(0.0f + margins.left, widthP - margins.right, -heightP + margins.bottom, 0.0f - margins.top);
		} else {
			this->establecerLimites(0.0f, widthP, -heightP, 0.0f);
		}
	}
	//Set props
	_width			= widthP;
	_height			= heightP;
	_margins		= margins;
	//Scroll (after updating the variables)
	{
		this->privScrollAutoAdjust();
	}
	_chldResizedCount = 0;
}

void AUSceneCol::updateMargins(const NBMargenes margins){
	this->privOrganize(_width, _height, margins);
}

void AUSceneCol::setBorderVisible(const BOOL visible){
	//_fg->setVisibleAndAwake(visible);
}

BOOL AUSceneCol::getStatusBarDef(STTSSceneBarStatus* dst){
	BOOL r = FALSE;
	if(_itfCol != NULL){
		r = _itfCol->getStatusBarDef(dst);
	}
	return r;
}

BOOL AUSceneCol::execBarBackCmd(){
	BOOL r = FALSE;
	if(_itfCol != NULL){
		r = _itfCol->execBarBackCmd();
		this->privScrollAutoAdjust();
	}
	return r;
}

void AUSceneCol::execBarIconCmd(const SI32 iIcon){
	if(_itfCol != NULL){
		_itfCol->execBarIconCmd(iIcon);
		this->privScrollAutoAdjust();
	}
}

void AUSceneCol::applySearchFilter(const char** wordsLwr, const SI32 wordsLwrSz){
	if(_itfCol != NULL){
		_itfCol->applySearchFilter(wordsLwr, wordsLwrSz);
		this->privScrollAutoAdjust();
	}
}

//

void AUSceneCol::scrollToTop(const float secs){
	const NBCajaAABB box	= _obj->cajaAABBLocal();
	const NBPunto pos		= _obj->traslacion();
	STNBPoint posDst;
	posDst.x				= pos.x;
	posDst.y				= 0.0f - _margins.top - box.yMax;
	this->scrollToPoint(posDst, secs);
	//Scroll internals
	if(_itfCol != NULL){
		_itfCol->scrollToTop(secs);
	}
}

void AUSceneCol::scrollToPoint(const STNBPoint pos, const float secs){
	if(secs <= 0.0f){
		this->privScrollTo(pos.x, pos.y, FALSE);
	} else {
		const NBPunto cPos			= _obj->traslacion();
		_touch.panTo.anim.secsDur	= secs;
		_touch.panTo.anim.secsCur	= 0.0f;
		_touch.panTo.anim.posStart.x = cPos.x;
		_touch.panTo.anim.posStart.y = cPos.y;
		_touch.panTo.anim.posEnd	= pos;
	}
}

void AUSceneCol::tickAnimacion(float segsTranscurridos){
	if(this->idEscena >= 0){
		BOOL anchorUsed = FALSE, organize = FALSE;
		//
		if(_chldResizedCount > 0){
			organize = TRUE;
		}
		//Scroll
		{
			//Consume last scoll.deltaY
			{
				{
					_touch.scroll.deltaAccumSecs.x += segsTranscurridos;
					if((_touch.scroll.deltaAccum.width > 0.0f && _touch.scroll.deltaLast.width < 0.0f) || (_touch.scroll.deltaAccum.width < 0.0f && _touch.scroll.deltaLast.width > 0.0f)){
						//Direction changed
						_touch.scroll.speed.x			= 0.0f;
						_touch.scroll.deltaAccum.width	= 0.0f;
						_touch.scroll.deltaAccumSecs.x	= 0.0f;
					} else {
						//Acumulate this (is same direction)
						_touch.scroll.deltaAccum.width	+= _touch.scroll.deltaLast.width;
					}
					_touch.scroll.deltaLast.width		= 0.0f;
				}
				{
					_touch.scroll.deltaAccumSecs.y += segsTranscurridos;
					if((_touch.scroll.deltaAccum.height > 0.0f && _touch.scroll.deltaLast.height < 0.0f) || (_touch.scroll.deltaAccum.height < 0.0f && _touch.scroll.deltaLast.height > 0.0f)){
						//Direction changed
						_touch.scroll.speed.y			= 0.0f;
						_touch.scroll.deltaAccum.height	= 0.0f;
						_touch.scroll.deltaAccumSecs.y	= 0.0f;
					} else {
						//Acumulate this (is same direction)
						_touch.scroll.deltaAccum.height	+= _touch.scroll.deltaLast.height;
					}
					_touch.scroll.deltaLast.height		= 0.0f;
				}
			}
			//Scroll (only if not touching)
			if(_touch.first.ref == NULL && _touch.second.ref == NULL){
				//Apply accumulated speed
				if(_touch.scroll.deltaAccum.width != 0.0f){
					NBASSERT(_touch.scroll.deltaAccumSecs.x > 0.0f)
					if(_touch.scroll.deltaAccumSecs.x != 0.0f){
						_touch.scroll.speed.x		+= (_touch.scroll.deltaAccum.width / _touch.scroll.deltaAccumSecs.x);
						//PRINTF_INFO("_touch.scroll.deltaLast.height(%f); prevScrollSpeedY(%f) => _touch.scroll.speed.y(%f).\n", _touch.scroll.deltaLast.height, prevScrollSpeedY, _touch.scroll.speed.y);
					}
					_touch.scroll.deltaAccum.width	= 0.0f;
					_touch.scroll.deltaAccumSecs.x	= 0.0f;
				}
				if(_touch.scroll.deltaAccum.height != 0.0f){
					NBASSERT(_touch.scroll.deltaAccumSecs.y > 0.0f)
					if(_touch.scroll.deltaAccumSecs.y != 0.0f){
						_touch.scroll.speed.y		+= (_touch.scroll.deltaAccum.height / _touch.scroll.deltaAccumSecs.y);
						//PRINTF_INFO("_touch.scroll.deltaLast.height(%f); prevScrollSpeedY(%f) => _touch.scroll.speed.y(%f).\n", _touch.scroll.deltaLast.height, prevScrollSpeedY, _touch.scroll.speed.y);
					}
					_touch.scroll.deltaAccum.height	= 0.0f;
					_touch.scroll.deltaAccumSecs.y	= 0.0f;
				}
				//Apply scroll
				if(_touch.scroll.speed.x != 0.0f || _touch.scroll.speed.y != 0.0f){
					//PRINTF_INFO("_touch.scroll.speed.y(%f).\n", _touch.scroll.speed.y);
					this->privScroll(_touch.scroll.speed.x * segsTranscurridos, _touch.scroll.speed.y * segsTranscurridos, TRUE);
					_touch.scroll.speed.x -= (_touch.scroll.speed.x * pow(segsTranscurridos, 1.0f / 3.0f));
					if(_touch.scroll.speed.x >= -5.0f && _touch.scroll.speed.x <= 5.0f){
						_touch.scroll.speed.x = 0.0f;
					}
					_touch.scroll.speed.y -= (_touch.scroll.speed.y * pow(segsTranscurridos, 2.0f / 3.0f));
					if(_touch.scroll.speed.y >= -5.0f && _touch.scroll.speed.y <= 5.0f){
						_touch.scroll.speed.y = 0.0f;
					}
				}
			}
		}
		//Scrollbar visibility
		{
			//Horizontal
			{
				const float maxAlpha8 = (float)_scroll.h.bar.maxAlpha8;
				_scroll.h.bar.secsAccum += segsTranscurridos;
				if(_scroll.h.bar.secsAccum < _scroll.h.bar.secsForAppear){
					//Showing bar
					const float rel = (_scroll.h.bar.secsAccum / _scroll.h.bar.secsForAppear);
					_scroll.h.bar.obj->establecerMultiplicadorAlpha8(maxAlpha8 * (rel));
					_scroll.h.bar.obj->establecerVisible(true);
				} else if(_scroll.h.bar.secsAccum < (_scroll.h.bar.secsForAppear + _scroll.h.bar.secsFullVisible)){
					//Full visibility
					_scroll.h.bar.obj->establecerMultiplicadorAlpha8(maxAlpha8);
					_scroll.h.bar.obj->establecerVisible(true);
				} else {
					const float rel = ((_scroll.h.bar.secsAccum - _scroll.h.bar.secsForAppear - _scroll.h.bar.secsFullVisible) / _scroll.h.bar.secsForDisapr);
					if(rel < 1.0f){
						//Hidding
						_scroll.h.bar.obj->establecerMultiplicadorAlpha8(maxAlpha8 * (1.0f - (rel)));
						_scroll.h.bar.obj->establecerVisible(true);
					} else {
						//Hidde
						_scroll.h.bar.obj->establecerVisible(false);
						_scroll.h.bar.secsAccum = (_scroll.h.bar.secsForAppear + _scroll.h.bar.secsFullVisible + _scroll.h.bar.secsForDisapr + 1.0f); //to avoid big numbers
					}
				}
			}
			//Vertical
			{
				const float maxAlpha8 = (float)_scroll.v.bar.maxAlpha8;
				_scroll.v.bar.secsAccum += segsTranscurridos;
				if(_scroll.v.bar.secsAccum < _scroll.v.bar.secsForAppear){
					//Showing bar
					const float rel = (_scroll.v.bar.secsAccum / _scroll.v.bar.secsForAppear);
					_scroll.v.bar.obj->establecerMultiplicadorAlpha8(maxAlpha8 * (rel));
					_scroll.v.bar.obj->establecerVisible(true);
				} else if(_scroll.v.bar.secsAccum < (_scroll.v.bar.secsForAppear + _scroll.v.bar.secsFullVisible)){
					//Full visibility
					_scroll.v.bar.obj->establecerMultiplicadorAlpha8(maxAlpha8);
					_scroll.v.bar.obj->establecerVisible(true);
				} else {
					const float rel = ((_scroll.v.bar.secsAccum - _scroll.v.bar.secsForAppear - _scroll.v.bar.secsFullVisible) / _scroll.v.bar.secsForDisapr);
					if(rel < 1.0f){
						//Hidding
						_scroll.v.bar.obj->establecerMultiplicadorAlpha8(maxAlpha8 * (1.0f - (rel)));
						_scroll.v.bar.obj->establecerVisible(true);
					} else {
						//Hidde
						_scroll.v.bar.obj->establecerVisible(false);
						_scroll.v.bar.secsAccum = (_scroll.v.bar.secsForAppear + _scroll.v.bar.secsFullVisible + _scroll.v.bar.secsForDisapr + 1.0f); //to avoid big numbers
					}
				}
			}
		}
		//Zoom (only if not touching)
		{
			if(_touch.first.ref == NULL && _touch.second.ref == NULL){
				if(_touch.zoom.anim.secsDur > 0.0f && _objSizeBase.width > 0.0f){
					_touch.zoom.anim.secsCur += segsTranscurridos;
					{
						float rel = (_touch.zoom.anim.secsCur / _touch.zoom.anim.secsDur);
						if(rel >= 1.0f){
							rel = 1.0f;
							_touch.zoom.anim.secsCur	= 0.0f;
							_touch.zoom.anim.secsDur	= 0.0f;
						}
						if(_itfCol != NULL){
							if(FALSE){ //accelerated
								const float rel2	= (rel * rel);
								const float scale	= _touch.zoom.anim.scaleStart + ((_touch.zoom.anim.scaleEnd - _touch.zoom.anim.scaleStart) * rel2);
								_itfCol->applyScaledSize(_objSizeBase, NBST_P(STNBSize, _objSizeBase.width * scale, _objSizeBase.height * scale), FALSE, NULL, 0.0f);
							} else { //deaccelerated
								const float relInv	= (1.0f - rel);
								const float rel2	= (relInv * relInv * relInv);
								const float scale	= _touch.zoom.anim.scaleEnd + ((_touch.zoom.anim.scaleStart - _touch.zoom.anim.scaleEnd) * rel2);
								_itfCol->applyScaledSize(_objSizeBase, NBST_P(STNBSize, _objSizeBase.width * scale, _objSizeBase.height * scale), FALSE, NULL, 0.0f);
							}
							if(_touch.anchor.ref != NULL){
								//Center to anchor
								float scalePref				= 0.0f;
								const STNBAABox anchorBox	= _itfCol->anchorGetBox(_touch.anchor.ref, &scalePref);
								if(scalePref > 0.0f){
									_touch.zoom.anim.scaleEnd = scalePref;
								}
								if(_touch.panTo.anim.secsDur <= 0){
									this->privScrollTo(_touch.anchor.lclOrg.x - anchorBox.xMin - ((anchorBox.xMax - anchorBox.xMin) * 0.5f), _touch.anchor.lclOrg.y - anchorBox.yMin - ((anchorBox.yMax - anchorBox.yMin) * 0.5f), TRUE);
								}
								anchorUsed			= TRUE;
							} else {
								if(_touch.panTo.anim.secsDur <= 0){
									this->privScroll(0.0f, 0.0f, TRUE);
								}
							}
						}
					}
				}
			}
		}
		//PanTo (only if not touching)
		{
			if(_touch.first.ref == NULL && _touch.second.ref == NULL){
				if(_touch.panTo.anim.secsDur > 0.0f){
					_touch.panTo.anim.secsCur += segsTranscurridos;
					{
						float rel = (_touch.panTo.anim.secsCur / _touch.panTo.anim.secsDur);
						if(rel >= 1.0f){
							rel = 1.0f;
							_touch.panTo.anim.secsCur	= 0.0f;
							_touch.panTo.anim.secsDur	= 0.0f;
						}
						if(_itfCol != NULL){
							//Update end pos
							if(_touch.anchor.ref != NULL){
								float scalePref				= 0.0f;
								const STNBAABox anchorBox	= _itfCol->anchorGetBox(_touch.anchor.ref, &scalePref);
								const STNBPoint center		= NBST_P(STNBPoint, anchorBox.xMin + ((anchorBox.xMax - anchorBox.xMin) * 0.5f), anchorBox.yMin + ((anchorBox.yMax - anchorBox.yMin) * 0.5f));
								_touch.panTo.anim.posEnd.x	= 0.0f + _margins.left + ((_width - _margins.left - _margins.right) * 0.5f) - center.x;
								_touch.panTo.anim.posEnd.y	= 0.0f - _margins.top - ((_height - _margins.top - _margins.bottom) * 0.5f) - center.y;
								anchorUsed			= TRUE;
							}
							{
								//deaccelerated
								const float relInv	= (1.0f - rel);
								const float rel2	= (relInv * relInv * relInv);
								const STNBPoint pos = NBST_P(STNBPoint, 
									_touch.panTo.anim.posEnd.x + (_touch.panTo.anim.posStart.x - _touch.panTo.anim.posEnd.x) * rel2
									, _touch.panTo.anim.posEnd.y + (_touch.panTo.anim.posStart.y - _touch.panTo.anim.posEnd.y) * rel2
								);
								this->privScrollTo(pos.x, pos.y, TRUE);
							}
						}
					}
				}
			}
		}
		//Release anchor (if not used)
		if(!anchorUsed && _itfCol != NULL){
			if(_touch.anchor.ref != NULL){
				_itfCol->anchorDestroy(_touch.anchor.ref);
				_touch.anchor.ref = NULL;
			}
			//Load new anchor to focus
			if(_touch.panTo.anim.secsDur <= 0.0f){
				if(_touch.anchor.ref == NULL){
					float secsDur = 0.5f;
					_touch.anchor.ref = _itfCol->anchorToFocusClone(&secsDur);
					if(_touch.anchor.ref != NULL){
						{
							//
							//0.0f - _margins.top - box.yMax;
							//_height + _margins.bottom - box.yMin
							float scalePref				= 0.0f;
							const NBPunto cPos			= _obj->traslacion();
							const STNBAABox anchorBox	= _itfCol->anchorGetBox(_touch.anchor.ref, &scalePref);
							const STNBPoint center		= NBST_P(STNBPoint, anchorBox.xMin + ((anchorBox.xMax - anchorBox.xMin) * 0.5f), anchorBox.yMin + ((anchorBox.yMax - anchorBox.yMin) * 0.5f));
							_touch.panTo.anim.posStart.x = cPos.x;
							_touch.panTo.anim.posStart.y = cPos.y;
							_touch.panTo.anim.posEnd.x	= 0.0f + _margins.left + ((_width - _margins.left - _margins.right) * 0.5f) - center.x;
							_touch.panTo.anim.posEnd.y	= 0.0f - _margins.top - ((_height - _margins.top - _margins.bottom) * 0.5f) - center.y;
							_touch.panTo.anim.secsCur	= 0.0f;
							_touch.panTo.anim.secsDur	= secsDur;
							if(scalePref > 0.0f){
								const STNBSize scaledSz		= _itfCol->getScaledSize();
								const float scaleCur		= (scaledSz.width / _objSizeBase.width);
								_touch.zoom.anim.scaleStart = scaleCur;
								_touch.zoom.anim.scaleEnd	= scalePref;
								_touch.zoom.anim.secsCur	= 0.0f;
								_touch.zoom.anim.secsDur	= secsDur;
							}
							if(_touch.panTo.anim.secsDur <= 0.0f){
								//Apply inmediatly
								this->privScrollTo(_touch.panTo.anim.posEnd.x, _touch.panTo.anim.posEnd.y, TRUE);
								if(scalePref > 0.0f){
									_itfCol->applyScaledSize(_objSizeBase, NBST_P(STNBSize, _objSizeBase.width * scalePref, _objSizeBase.height * scalePref), FALSE, NULL, 0.0f);
								}
							}
							PRINTF_INFO("Starting panTo from(%f, %f) to(%f, %f).\n", _touch.panTo.anim.posStart.x, _touch.panTo.anim.posStart.y, _touch.panTo.anim.posEnd.x, _touch.panTo.anim.posEnd.y);
						}
						_itfCol->anchorToFocusClear();
					}
				}
			}
		}
		//Acum time for touches
		{
			if(_touch.first.ref != NULL){
				_touch.first.secsAcum += segsTranscurridos;
			}
			if(_touch.second.ref != NULL){
				_touch.second.secsAcum += segsTranscurridos;
			}
			{
				_touch.dblTap.secsDown -= segsTranscurridos;
				if(_touch.dblTap.secsDown < 0.0f){
					_touch.dblTap.secsDown = 0.0f;
				}
			}
		}
		//Organize
		if(organize){
			this->privOrganize(_width, _height, _margins);
		}
	}
}

void AUSceneCol::privScroll(const float scrollDeltaX, const float scrollDeltaY, const BOOL showScrollbar){
	if(_obj != NULL){
		const NBPunto pos = _obj->traslacion();
		this->privScrollTo(pos.x + scrollDeltaX, pos.y + scrollDeltaY, showScrollbar);
	}
}

void AUSceneCol::privScrollTo(const float xPos, const float yPos, const BOOL showScrollbar){
	if(_obj != NULL){
		const NBCajaAABB box = _obj->cajaAABBLocal();
		NBPunto pos;
		pos.x = xPos;
		pos.y = yPos;
		//Horizontal scroll
		{
			const BOOL isSmaller					= (box.xMax - box.xMin) <= (_width - _margins.left - _margins.right);
			_fixedToSideH[ENDRScrollSideH_Left]		= ((pos.x + box.xMin) > (0.0f + _margins.left) || isSmaller);
			_fixedToSideH[ENDRScrollSideH_Right]	= ((pos.x + box.xMax) < (0.0f + _width - _margins.right));
			if(_fixedToSideH[ENDRScrollSideH_Left]){
				pos.x = 0.0f + _margins.left - box.xMin;
			} else if(_fixedToSideH[ENDRScrollSideH_Right]){
				pos.x = 0.0f + _width - _margins.right - box.xMax;
			}
		}
		//Vertical scroll
		{
			const BOOL isSmaller					= (box.yMax - box.yMin) <= (_height - _margins.top - _margins.bottom);
			_fixedToSideV[ENDRScrollSideV_Top]		= ((pos.y + box.yMax) < (0.0f - _margins.top) || isSmaller);
			_fixedToSideV[ENDRScrollSideV_Bottom]	= ((pos.y + box.yMin) > (0.0f - _height + _margins.bottom));
			if(_fixedToSideV[ENDRScrollSideV_Top]){
				pos.y = 0.0f - _margins.top - box.yMax;
				//PRINTF_INFO("Fixed to top%s.\n", (isSmaller ? " (obj is smaller)" : ""));
			} else if(_fixedToSideV[ENDRScrollSideV_Bottom]){
				pos.y = 0.0f - _height + _margins.bottom - box.yMin;
				//PRINTF_INFO("Fixed to bottom.\n");
			}
		}
		_obj->establecerTraslacion(pos.x, pos.y);
		//Scroll
		{
			const BOOL isScrollableV	= (_scroll.v.objSize > (_scroll.v.visibleSize + 1));
			const BOOL isScrollableH	= (_scroll.h.objSize > (_scroll.h.visibleSize + 1));
			const NBTamano texSz		= _scroll.v.bar.obj->textura()->tamanoHD();
			//Vertical
			{
				const float yyMax	= (0.0f - _margins.top) - box.yMax;
				const float yyMin	= (0.0f - _height + _margins.bottom) - box.yMin;
				float relSz			= 1.0f, relPos = 0.0f;
				//Determine size and pos
				if(isScrollableV){
					if(_scroll.v.objSize > 0.0f){
						relSz			= (_scroll.v.visibleSize / _scroll.v.objSize);
					}
					if((relSz * _scroll.v.visibleSize) < (texSz.ancho * 3.0f)){
						if(texSz.ancho > 0.0f && _scroll.v.visibleSize > 0.0f){
							relSz		= (texSz.ancho * 3.0f) / _scroll.v.visibleSize;
						}
					}
					if(yyMax != yyMin){
						relPos		= (pos.y - yyMin) / (yyMax - yyMin);
					}
				}
				//Organize
				{
					const float height		= _scroll.v.visibleSize * relSz;
					_scroll.v.bar.obj->redimensionar(0.0f, 0.0f, height, -texSz.alto);
					_scroll.v.bar.obj->establecerRotacion(-90.0f);
					_scroll.v.bar.obj->establecerTraslacion((0.0f + _width - _margins.right), (0.0f - _height + _margins.bottom) + _scroll.v.marginTop + height + ((_scroll.v.visibleSize - height) * relPos));
					if(showScrollbar && (isScrollableV || !isScrollableH)){ //Allways show at least one scrollbar
						if(_scroll.v.bar.secsAccum >= (_scroll.v.bar.secsForAppear + _scroll.v.bar.secsFullVisible)){
							//Start visible
							_scroll.v.bar.secsAccum = 0.0f;
						} else if(_scroll.v.bar.secsAccum >= _scroll.v.bar.secsForAppear){
							//Keep visible
							_scroll.v.bar.secsAccum = _scroll.v.bar.secsForAppear;
						}
					}
				}
			}
			//Horizontal
			{
				const float xxMax	= (0.0f +_margins.left) - box.xMin;
				const float xxMin	= (0.0f + _width - _margins.right) - box.xMax;
				float relSz			= 1.0f, relPos = 0.0f;
				//Determine size and pos
				if(isScrollableH){
					if(_scroll.h.objSize > 0.0f){
						relSz			= (_scroll.h.visibleSize / _scroll.h.objSize);
					}
					if((relSz * _scroll.h.visibleSize) < (texSz.ancho * 3.0f)){
						if(texSz.ancho > 0.0f && _scroll.h.visibleSize > 0.0f){
							relSz		= (texSz.ancho * 3.0f) / _scroll.h.visibleSize;
						}
					}
					if(xxMax != xxMin){
						relPos		= (pos.x - xxMin) / (xxMax - xxMin);
					}
				}
				//Organize
				{
					const float width		= _scroll.h.visibleSize * relSz;
					_scroll.h.bar.obj->redimensionar(0.0f, 0.0f, width, texSz.alto);
					_scroll.h.bar.obj->establecerTraslacion((0.0f + _margins.left) + _scroll.h.marginLeft + ((_scroll.h.visibleSize - width) * (1.0f - relPos)), (0.0f - _height + _margins.bottom));
					if(showScrollbar && isScrollableH){
						if(_scroll.h.bar.secsAccum >= (_scroll.h.bar.secsForAppear + _scroll.h.bar.secsFullVisible)){
							//Start visible
							_scroll.h.bar.secsAccum = 0.0f;
						} else if(_scroll.h.bar.secsAccum >= _scroll.h.bar.secsForAppear){
							//Keep visible
							_scroll.h.bar.secsAccum = _scroll.h.bar.secsForAppear;
						}
					}
				}
			}
		}
		//Apply area-filter
		if(_itfCol != NULL){
			STNBAABox inner, outter;
			outter.xMin	= 0.0f - pos.x;
			outter.xMax	= _width - pos.x;
			outter.yMin	= -_height - pos.y;
			outter.yMax	= 0.0f - pos.y;
			//NBASSERT(outter.xMin <= outter.xMax && outter.yMin <= outter.yMax)
			inner.xMin	= outter.xMin + _margins.left;
			inner.xMax	= outter.xMax - _margins.right;
			inner.yMin	= outter.yMin + _margins.bottom;
			inner.yMax	= outter.yMax - _margins.top;
			//NBASSERT(inner.xMin <= inner.xMax && inner.yMin <= inner.yMax)
			//TMP (reduce visual range to test filter-area)
			/*{
				outter.xMin	+= (_width * 0.15f);
				outter.xMax	-= (_width * 0.15f);
				outter.yMin	+= (_height * 0.15f);
				outter.yMax	-= (_height * 0.15f);
				//
				inner.xMin	+= (_width * 0.15f);
				inner.xMax	-= (_width * 0.15f);
				inner.yMin	+= (_height * 0.25f);
				inner.yMax	-= (_height * 0.25f);
			}*/
			_itfCol->applyAreaFilter(outter, inner);
		}
	}
}

void AUSceneCol::privScrollToSides(const ENDRScrollSideH sideH, const ENDRScrollSideV sideV){
	const NBCajaAABB box = _obj->cajaAABBLocal();
	NBPunto pos = _obj->traslacion();
	//PRINTF_INFO("BEFORE left(%d) right(%d) top(%d) bottom(%d).\n", _fixedToSideH[ENDRScrollSideH_Left], _fixedToSideH[ENDRScrollSideH_Right], _fixedToSideV[ENDRScrollSideV_Top], _fixedToSideV[ENDRScrollSideV_Bottom]);
	//Horiz
	if(sideH == ENDRScrollSideH_Left){
		pos.x = 0.0f + _margins.left - box.xMin + 1.0f; //+1.0f to force side
	} else if(sideH == ENDRScrollSideH_Right){
		pos.x = 0.0f + _width - _margins.right - box.xMax - 1.0f; //-1.0f to force side
	}
	//Vertic
	if(sideV == ENDRScrollSideV_Top){
		pos.y = 0.0f - _margins.top - box.yMax - 1.0f; //-1.0f to force side
	} else if(sideV == ENDRScrollSideV_Bottom){
		pos.y = 0.0f - _height + _margins.bottom - box.yMin + 1.0f; //+1.0f to force side
	}
	//
	this->privScrollTo(pos.x, pos.y, FALSE);
	//PRINTF_INFO("AFTER left(%d) right(%d) top(%d) bottom(%d).\n", _fixedToSideH[ENDRScrollSideH_Left], _fixedToSideH[ENDRScrollSideH_Right], _fixedToSideV[ENDRScrollSideV_Top], _fixedToSideV[ENDRScrollSideV_Bottom]);
}

void AUSceneCol::privScrollAutoAdjust(){
	ENDRScrollSideH pprefH = ENDRScrollSideH_Count;
	ENDRScrollSideV pprefV = ENDRScrollSideV_Count;
	if(_itfCol != NULL){
		const ENDRScrollSideH prefH = _itfCol->getPreferedScrollH();
		const ENDRScrollSideV prefV = _itfCol->getPreferedScrollV();
		if(_fixedToSideH[prefH]){
			pprefH = prefH;
		}
		if(_fixedToSideV[prefV]){
			pprefV = prefV;
		}
	}
	this->privScrollToSides(pprefH, pprefV);
}

//TOUCHES

void AUSceneCol::touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto){
	//PRINTF_INFO("AUSceneCol::touchIniciado isFirst(%s).\n", (_touch.first.ref == touch ? "YES" : "NO"));
	if(objeto == _bg){
		if(_touch.first.ref == NULL){
			//Start of drag
			NBASSERT(_touch.second.ref == NULL)
			_touch.first.ref	= touch;
			{
				const NBPunto posLcl		= this->coordenadaEscenaALocal(posTouchEscena.x, posTouchEscena.y);
				const NBPunto trasObj		= _obj->traslacion();
				_touch.first.secsAcum		= 0.0f;
				_touch.first.dstAccum2		= 0.0f;
				_touch.first.lclPosStart.x	= posLcl.x;
				_touch.first.lclPosStart.y	= posLcl.y;
				_touch.first.lclPosTrack	= _touch.first.lclPosStart;
				_touch.first.lclPosCurr		= _touch.first.lclPosStart;
				_touch.first.objTras.x		= trasObj.x;
				_touch.first.objTras.y		= trasObj.y;
				//Stop actions
				NBMemory_setZero(_touch.scroll);
				NBMemory_setZero(_touch.zoom);
				NBMemory_setZero(_touch.panTo);
			}
		} else if(_touch.second.ref == NULL){
			//Start zoom-and-pan
			_touch.second.ref	= touch;
			{
				const NBPunto posLcl		= this->coordenadaEscenaALocal(posTouchEscena.x, posTouchEscena.y);
				const NBPunto trasObj		= _obj->traslacion();
				_touch.second.secsAcum		= 0.0f;
				_touch.second.dstAccum2		= 0.0f;
				_touch.second.lclPosStart.x	= posLcl.x;
				_touch.second.lclPosStart.y	= posLcl.y;
				_touch.second.lclPosTrack	= _touch.second.lclPosStart;
				_touch.second.lclPosCurr	= _touch.second.lclPosStart;
				_touch.second.objTras.x		= trasObj.x;
				_touch.second.objTras.y		= trasObj.y;
				//Stop actions
				NBMemory_setZero(_touch.dblTap);
				NBMemory_setZero(_touch.scroll);
				NBMemory_setZero(_touch.zoom);
				NBMemory_setZero(_touch.panTo);
				//Start zoom
				{
					const STNBSize scaledSz	= _itfCol->getScaledSize();
					const NBCajaAABB box	= _obj->cajaAABBLocal();
					const float objWidth	= (scaledSz.width > 0.0f ? scaledSz.width : (box.xMax - box.xMin));
					const float objHeight	= (box.yMax - box.yMin);
					if(objWidth > 0.0f && objHeight > 0.0f){
						const NBPunto posObj	= _obj->traslacion();
						//PRINTF_INFO("posObj(%f, %f) box-x(%f, %f)-y(%f, %f).\n", posObj.x, posObj.y, box.xMin, box.xMax, box.yMin, box.yMax);
						const STNBPoint pos0 = NBST_P(STNBPoint, 
							_touch.first.lclPosCurr.x - posObj.x
							, _touch.first.lclPosCurr.y - posObj.y
						);
						const STNBPoint pos1 = NBST_P(STNBPoint, 
							_touch.second.lclPosCurr.x - posObj.x
							, _touch.second.lclPosCurr.y - posObj.y
						);
						const STNBPoint posRel0 = NBST_P(STNBPoint, 
							(pos0.x - box.xMin) / objWidth
							, (pos0.y - box.yMin) / objHeight
						);
						const STNBPoint posRel1 = NBST_P(STNBPoint, 
							(pos1.x - box.xMin) / objWidth
							, (pos1.y - box.yMin) / objHeight
						);
						const STNBPoint posRelMid = NBST_P(STNBPoint, 
							posRel0.x + ((posRel1.x - posRel0.x) * 0.5f)
							, posRel0.y + ((posRel1.y - posRel0.y) * 0.5f)
						);
						_touch.zoom.objSizeStart.width	= objWidth;
						_touch.zoom.objSizeStart.height	= objHeight;
						_touch.zoom.objPosRelFocus.x	= posRelMid.x;
						_touch.zoom.objPosRelFocus.y	= posRelMid.y;
						//PRINTF_INFO("AUSceneCol, zoom mid(%f, %f) pos0(%f, %f)-rel(%f, %f); pos1(%f, %f)-rel(%f, %f).\n", posRelMid.x, posRelMid.y, pos0.x, pos0.y, posRel0.x, posRel0.y, pos1.x, pos1.y, posRel1.x, posRel1.y);
						//PRINTF_INFO("AUSceneCol, objSizeStart(%f, %f) objPosRelFocus(%f, %f).\n", _touch.zoom.objSizeStart.width, _touch.zoom.objSizeStart.height, _touch.zoom.objPosRelFocus.x, _touch.zoom.objPosRelFocus.y);
					}
				}
			}
		}
	}
}

void AUSceneCol::touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	//PRINTF_INFO("AUSceneCol::touchMovido isFirst(%s).\n", (_touch.first.ref == touch ? "YES" : "NO"));
	if(objeto == _bg){
		const STNBPoint posFrstBefore = _touch.first.lclPosCurr;
		//Update touches curPos
		{
			const NBPunto posLcl = this->coordenadaEscenaALocal(posActualEscena.x, posActualEscena.y);
			const STNBSize scnDeltas = {
				posActualEscena.x - posAnteriorEscena.x
				, posActualEscena.y - posAnteriorEscena.y
			};
			if(_touch.first.ref == touch){
				_touch.first.lclPosCurr.x	= posLcl.x;
				_touch.first.lclPosCurr.y	= posLcl.y;
				_touch.first.dstAccum2		+= (scnDeltas.width * scnDeltas.width) + (scnDeltas.height * scnDeltas.height);
			} else if(_touch.second.ref == touch){
				_touch.second.lclPosCurr.x	= posLcl.x;
				_touch.second.lclPosCurr.y	= posLcl.y;
				_touch.second.dstAccum2		+= (scnDeltas.width * scnDeltas.width) + (scnDeltas.height * scnDeltas.height);
			}
		}
		//Consume
		if(_touch.first.ref != NULL && _touch.second.ref != NULL && (_touch.first.ref == touch || _touch.second.ref == touch)){
			//Apply zoom
			const STNBSize orgDeltas = {
				_touch.second.lclPosStart.x - _touch.first.lclPosStart.x
				, _touch.second.lclPosStart.y - _touch.first.lclPosStart.y
			};
			const STNBSize curDeltas = {
				_touch.second.lclPosCurr.x - _touch.first.lclPosCurr.x
				, _touch.second.lclPosCurr.y - _touch.first.lclPosCurr.y
			};
			const float orgDst2 = (orgDeltas.width * orgDeltas.width) + (orgDeltas.height * orgDeltas.height);
			const float curDst2 = (curDeltas.width * curDeltas.width) + (curDeltas.height * curDeltas.height);
			if(orgDst2 > 0.0f && curDst2 > 0.0f && _touch.zoom.objSizeStart.width > 0.0f){
				const float orgDst = sqrtf(orgDst2);
				const float curDst = sqrtf(curDst2);
				if(_itfCol != NULL && _width > 0.0f && _height > 0.0f){
					float scale = (_touch.zoom.objSizeStart.width + (curDst - orgDst)) / _width;
					if(scale < 0.25f) scale = 0.25f;
					if(scale > 5.00f) scale = 5.00f;
					//PRINTF_INFO("AUSceneCol, applingScale(%f).\n", scale);
					_itfCol->applyScaledSize(NBST_P(STNBSize, _width, _height ), NBST_P(STNBSize, _width * scale, _height * scale ), false, NULL, 0.0f);
					//Scroll
					{
						const NBCajaAABB box	= _obj->cajaAABBLocal();
						const STNBPoint curMid	= NBST_P(STNBPoint, 
							_touch.first.lclPosCurr.x + ((_touch.second.lclPosCurr.x - _touch.first.lclPosCurr.x) * 0.5f)
							, _touch.first.lclPosCurr.y + ((_touch.second.lclPosCurr.y - _touch.first.lclPosCurr.y) * 0.5f)
						);
						const STNBPoint curTras	= NBST_P(STNBPoint, 
							curMid.x - box.xMin - ((box.xMax - box.xMin) * _touch.zoom.objPosRelFocus.x)
							, curMid.y - box.yMin - ((box.yMax - box.yMin) * _touch.zoom.objPosRelFocus.y)
						);
						//PRINTF_INFO("curMid(%f, %f) objPosRelFocus(%f, %f).\n", curMid.x, curMid.y, _touch.zoom.objPosRelFocus.x, _touch.zoom.objPosRelFocus.y);
						//PRINTF_INFO("AUSceneCol, zoom pos0(%f, %f); pos1(%f, %f).\n", pos0.x, pos0.y, pos1.x, pos1.y);
						this->privScrollTo(curTras.x, curTras.y, TRUE);
					}
				}
			}
			//Stop actions
			NBMemory_setZero(_touch.dblTap);
			NBMemory_setZero(_touch.scroll);
		} else if(_touch.first.ref == touch){
			//Apply scroll
			NBASSERT(_touch.second.ref == NULL)
			{
				const STNBSize delta = NBST_P(STNBSize, 
					_touch.first.lclPosCurr.x - _touch.first.lclPosStart.x
					, _touch.first.lclPosCurr.y - _touch.first.lclPosStart.y
				);
				const STNBPoint exptdPos = NBST_P(STNBPoint, 
					_touch.first.objTras.x + delta.width
					, _touch.first.objTras.y + delta.height
				);
				this->privScrollTo(exptdPos.x, exptdPos.y, TRUE);
				_touch.scroll.deltaLast.width	= (_touch.first.lclPosCurr.x - posFrstBefore.x);
				_touch.scroll.deltaLast.height	= (_touch.first.lclPosCurr.y - posFrstBefore.y);
				//Track direction
				if(_obj != NULL){
					//Determine orientation
					const float minDrag		= NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.125f);
					const STNBSize deltaPrev = NBST_P(STNBSize, 
						_touch.first.lclPosTrack.x - _touch.first.lclPosStart.x
						, _touch.first.lclPosTrack.y - _touch.first.lclPosStart.y
					);
					const float prevDst2	= (deltaPrev.width * deltaPrev.width) + (deltaPrev.height * deltaPrev.height);
					const float curDst2		= (delta.width * delta.width) + (delta.height * delta.height);
					const float minDrag2	= minDrag * minDrag;
					//PRINTF_INFO("prevDst2(%f) curDst2(%f) minDrag2(%f).\n", prevDst2, curDst2, minDrag2);
					if(prevDst2 < minDrag2 && curDst2 >= minDrag2){
						const NBCajaAABB box	= _obj->cajaAABBLocal();
						const BOOL fitsInWidth	= (box.xMax - box.xMin) < (_width - _margins.left - _margins.right + 1.0f);
						const BOOL fitsInHeight	= (box.yMax - box.yMin) < (_height - _margins.top - _margins.bottom + 1.0f);
						_touch.first.lclPosTrack = _touch.first.lclPosCurr;
						//Determine if horizontal-pan is not being consumed
						if(fitsInWidth || fitsInHeight){
							const NBPunto realPos	= _obj->traslacion();
							const STNBSize deltaPos = NBST_P(STNBSize, 
								realPos.x - exptdPos.x
								, realPos.y - exptdPos.y
							);
							//PRINTF_INFO("deltaPos.width(%f) deltaPos.height(%f)%s.\n", deltaPos.width, deltaPos.height, (_touchInheritor != NULL ? " (touchInheritor exists)" : ""));
							{
								const BOOL isWidthDrag	= (fitsInWidth && (delta.width * delta.width) > (delta.height * delta.height) && deltaPos.width != 0.0f);
								const BOOL isHeightDrag	= (fitsInHeight && (delta.height * delta.height) > (delta.width * delta.width) && deltaPos.height != 0.0f);
								if(isWidthDrag || isHeightDrag){
									//Horizontal drag (inherit touch to parent)
									if(_touchInheritor != NULL){
										objeto->liberarTouch(touch, posInicialEscena, posAnteriorEscena, posActualEscena, TRUE, _touchInheritor);
									}
									//Stop actions
									NBMemory_setZero(_touch.dblTap);
									NBMemory_setZero(_touch.scroll);
								}
							}
						}
					}
				}
			}
		}
	}
}

void AUSceneCol::touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	//PRINTF_INFO("AUSceneCol::touchFinalizad isFirst(%s).\n", (_touch.first.ref == touch ? "YES" : "NO"));
	if(objeto == _bg){
		//Update touches curPos
		{
			const NBPunto posLcl = this->coordenadaEscenaALocal(posActualEscena.x, posActualEscena.y);
			const STNBSize scnDeltas = {
				posActualEscena.x - posAnteriorEscena.x
				, posActualEscena.y - posAnteriorEscena.y
			};
			if(_touch.first.ref == touch){
				_touch.first.lclPosCurr.x	= posLcl.x;
				_touch.first.lclPosCurr.y	= posLcl.y;
				_touch.first.dstAccum2		+= (scnDeltas.width * scnDeltas.width) + (scnDeltas.height * scnDeltas.height);
			} else if(_touch.second.ref == touch){
				_touch.second.lclPosCurr.x	= posLcl.x;
				_touch.second.lclPosCurr.y	= posLcl.y;
				_touch.second.dstAccum2		+= (scnDeltas.width * scnDeltas.width) + (scnDeltas.height * scnDeltas.height);
			}
		}
		//Consume
		if(_touch.first.ref != NULL && _touch.second.ref != NULL && (_touch.first.ref == touch || _touch.second.ref == touch)){
			//Zoom ended
			if(_touch.first.ref == touch){
				_touch.first = _touch.second;
			}
			NBMemory_setZero(_touch.second);
			NBMemory_setZero(_touch.zoom);
			NBMemory_setZero(_touch.panTo);
			//Start scroll
			{
				const NBPunto trasObj		= _obj->traslacion();
				_touch.first.lclPosStart	= _touch.first.lclPosCurr;
				_touch.first.lclPosTrack	= _touch.first.lclPosStart;
				_touch.first.objTras.x		= trasObj.x;
				_touch.first.objTras.y		= trasObj.y;
				//Stop actions
				NBMemory_setZero(_touch.dblTap);
				NBMemory_setZero(_touch.scroll);
			}
		} else if(_touch.first.ref == touch){
			//Drag ended
			NBASSERT(_touch.second.ref == NULL)
			//Analyze quick-tap
			{
				const float maxDstScn = NBGestorEscena::anchoPulgadasAEscena(_iScene, AUSCENE_COL_DBL_TAP_MAX_TAP_DST_INCH);
				if(_touch.first.dstAccum2 <= (maxDstScn * maxDstScn)){
					if(_touch.first.secsAcum < AUSCENE_COL_DBL_TAP_MAX_TAP_SECS){
						if(_touch.dblTap.secsDown <= 0.0f){
							//PRINTF_INFO("Touch was a first still-and-quick-tap.\n");
							_touch.dblTap.secsDown = AUSCENE_COL_DBL_TAP_MAX_WAIT_SECS;
						} else {
							//PRINTF_INFO("Touch was a double-tap.\n");
							_touch.dblTap.secsDown = 0.0f; //avoid nested double-taps
							//Start zoom animation
							{
								const STNBSize scaledSz		= _itfCol->getScaledSize();
								this->privStartAnimToScale(scaledSz.width == _objSizeBase.width ? 3.0f : 1.0f);
							}
						}
					} else {
						//PRINTF_INFO("Touch was a still-but-wait-tap.\n");
					}
				} else {
					//PRINTF_INFO("Touch was a movement-tap.\n");
				}
			}
			NBMemory_setZero(_touch.first);
		}
	}
}

void AUSceneCol::privStartAnimToScale(const float scale){
	//Start zoom animation
	if(_objSizeBase.width > 0.0f){
		const NBPunto trasObj		= _obj->traslacion();
		const STNBSize scaledSz		= _itfCol->getScaledSize();
		const float scaleCur		= (scaledSz.width / _objSizeBase.width);
		_touch.zoom.anim.scaleStart	= scaleCur;
		_touch.zoom.anim.scaleEnd	= scale;
		_touch.zoom.anim.secsCur	= 0.0f;
		_touch.zoom.anim.secsDur	= 0.35f;
		//Set anchor for animation
		{
			if(_touch.anchor.ref != NULL){
				_itfCol->anchorDestroy(_touch.anchor.ref);
				_touch.anchor.ref = NULL;
			}
			_touch.anchor.ref			= _itfCol->anchorCreate(NBST_P(STNBPoint, _touch.first.lclPosCurr.x - trasObj.x, _touch.first.lclPosCurr.y - trasObj.y ));
			_touch.anchor.lclOrg		= _touch.first.lclPosCurr;
		}
		//PRINTF_INFO("Animating zoom from(%f) to(%f).\n", _touch.zoom.anim.scaleStart, _touch.zoom.anim.scaleEnd);
	}
}

//Scroll

void AUSceneCol::touchScrollApply(AUEscenaObjeto* objeto, const STNBPoint posScene, const STNBSize size, const BOOL animate){
	if(_bg == objeto){
		this->privScroll(size.width, size.height, TRUE);
	}
}

void AUSceneCol::touchMagnifyApply(AUEscenaObjeto* objeto, const STNBPoint posScene, const float magnification, const BOOL isSmarthMag){
	if(_bg == objeto){
		if(_itfCol != NULL && _width > 0.0f && _height > 0.0f){
			{
				void* anchorCenter	= NULL;
				//Save current center
				if(anchorCenter == NULL && _width > 0.0f && _height > 0.0f){
					const NBPunto pos = _obj->traslacion();
					const STNBPoint center = NBST_P(STNBPoint, (_width * 0.5f) - pos.x, (_height * -0.5f) - pos.y);
					anchorCenter = _itfCol->anchorCreate(center);
					PRINTF_INFO("old-center(%f, %f).\n", center.x, center.y);
				}
				//Resize and organize
				{
					//Streched resize
					if(_itfCol != NULL){
						const STNBSize scaledSz		= _itfCol->getScaledSize();
						if(scaledSz.width > 0.0f && scaledSz.height > 0.0f){
							const float scaleCur		= (scaledSz.width / _objSizeBase.width);
							float scale = scaleCur + magnification;
							if(scale < 0.25f) scale = 0.25f;
							if(scale > 5.00f) scale = 5.00f;
							//PRINTF_INFO("AUSceneCol, applingScale(%f).\n", scale);
							_itfCol->applyScaledSize(NBST_P(STNBSize, _width, _height ), NBST_P(STNBSize, _width * scale, _height * scale ), false, NULL, 0.0f);
						}
					}
				}
				//Set new center
				if(anchorCenter != NULL && _itfCol != NULL){
					const STNBAABox box = _itfCol->anchorGetBox(anchorCenter, NULL);
					const STNBPoint boxCenter = NBST_P(STNBPoint, box.xMin + ((box.xMax - box.xMin) * 0.5f), box.yMin + ((box.yMax - box.yMin) * 0.5f));
					const STNBPoint newPos = NBST_P(STNBPoint, (_width * 0.5f) - boxCenter.x, (_height * -0.5f) - boxCenter.y);
					_obj->establecerTraslacion(newPos.x, newPos.y);
					PRINTF_INFO("new-center(%f, %f).\n", boxCenter.x, boxCenter.y);
					//Release anchor
					_itfCol->anchorDestroy(anchorCenter);
					anchorCenter = NULL;
				}
				//
				this->privScroll(0.0f, 0.0f, TRUE);
			}
		}
	}
}

//

AUOBJMETODOS_CLASESID_MULTICLASE(AUSceneCol, AUEscenaContenedorLimitado)
AUOBJMETODOS_CLASESNOMBRES_MULTICLASE(AUSceneCol, "AUSceneCol", AUEscenaContenedorLimitado)
AUOBJMETODOS_CLONAR_NULL(AUSceneCol)
