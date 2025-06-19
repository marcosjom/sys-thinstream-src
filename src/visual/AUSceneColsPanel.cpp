//
//  AUSceneColsPanelsStack.cpp
//  thinstream
//
//  Created by Marcos Ortega on 9/2/18.
//

#include "visual/TSVisualPch.h"
#include "visual/AUSceneColsPanel.h"

AUSceneColsPanel::AUSceneColsPanel(const SI32 iScene, AUEscenaObjeto* touchInheritor, const float width, const float height, const NBMargenes margins, const STNBColor8 defBgColor, const SI32 colsStackSz)
: AUEscenaContenedorLimitado()
, _iScene(iScene)
{
	NB_DEFINE_NOMBRE_PUNTERO(this, "AUSceneColsPanel")
	_touchInheritor	= touchInheritor;
	_width			= 0.0f;
	_height			= 0.0f;
	_margins		= margins;
	_defBgColor		= defBgColor;
	//
	_stacks.iCurrent = 0;
	NBArray_init(&_stacks.array, sizeof(STSceneColsPanelItm), NULL);
	//
	{
		_lyr		= new(this) AUEscenaContenedor();
		this->agregarObjetoEscena(_lyr);
	}
	//Add stacks
	{
		SI32 i; for(i = 0; i < colsStackSz; i++){
			STSceneColsPanelItm itm;
			NBMemory_setZeroSt(itm, STSceneColsPanelItm);
			itm.obj = new(this) AUSceneColsStack(_iScene, _touchInheritor, _width, _height, _margins, _defBgColor);
			//Set initial-pos
			{
				const NBCajaAABB box = itm.obj->cajaAABBLocal();
				itm.obj->establecerTraslacion(0.0f + itm.deltaPos.x - box.xMin, 0.0f + itm.deltaPos.y - box.yMax);
				//PRINTF_INFO("Moved col-panel-obj to(%f, %f) from-box-x(%f, %f)-y(%f, %f).\n", itm.obj->traslacion().x, itm.obj->traslacion().y, box.xMin, box.xMax, box.yMin, box.yMax);
			}
			//Add to the top of the top
			if(i == _stacks.iCurrent){
				_lyr->agregarObjetoEscena(itm.obj);
			}
			NBArray_addValue(&_stacks.array, itm);
		}
	}
	//
	this->privOrganize(width, height, margins);
	//
	NBGestorAnimadores::agregarAnimador(this, this);
}

AUSceneColsPanel::~AUSceneColsPanel(){
	NBGestorAnimadores::quitarAnimador(this);
	_touchInheritor = NULL;
	{
		SI32 i; for(i = 0; i < _stacks.array.use; i++){
			STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, i);
			if(itm->obj != NULL) itm->obj->liberar(NB_RETENEDOR_THIS); itm->obj = NULL;
		}
		NBArray_empty(&_stacks.array);
		NBArray_release(&_stacks.array);
	}
	{
		if(_lyr != NULL) _lyr->liberar(NB_RETENEDOR_THIS); _lyr = NULL;
	}
}

//

float AUSceneColsPanel::getWidth() const {
	return _width;
}

float AUSceneColsPanel::getHeight() const {
	return _height;
}

NBMargenes AUSceneColsPanel::getMargins() const{
	return _margins;
}

//

STNBPoint AUSceneColsPanel::privDeltaForSize(const ENSceneColSide side, const float rel){
	STNBPoint r;
	r.x = (side == ENSceneColSide_Left ? -_width : side == ENSceneColSide_Right ? _width : 0.0f) * rel;
	r.y = (side == ENSceneColSide_Btm ? -_height : side == ENSceneColSide_Top ? _height : 0.0f) * rel;
	return r;
}

SI32 AUSceneColsPanel::getCurrentStackIdx() const {
	return _stacks.iCurrent;
}

SI32 AUSceneColsPanel::getCurrentStackSz() const {
	SI32 r = 0;
	if(_stacks.iCurrent >= 0 && _stacks.iCurrent < _stacks.array.use){
		STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, _stacks.iCurrent);
		r = itm->obj->getSize();
	}
	return r;
}

void AUSceneColsPanel::setCurrentStack(const SI32 idx, const ENSceneColSide animOutTo, const ENSceneColSide animInFrom){
	if(_stacks.iCurrent != idx){
		const float animDur		= 0.50f;
		//Hide current
		if(_stacks.iCurrent >= 0 && _stacks.iCurrent < _stacks.array.use){
			STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, _stacks.iCurrent);
			itm->anim.secsDur		= animDur;
			itm->anim.secsCur		= 0.0f;
			itm->anim.from.side		= ENSceneColSide_Count;
			itm->anim.from.relPos	= 1.0f;
			itm->anim.from.relColor	= 1.0f;
			itm->anim.to.side		= animOutTo;
			itm->anim.to.relPos		= 1.0f;
			itm->anim.to.relColor	= 0.25f;
			itm->anim.acel			= TRUE;
			itm->deltaPos			= this->privDeltaForSize(itm->anim.from.side, itm->anim.from.relPos);
			itm->obj->setBorderVisible(itm->anim.secsDur > 0.0f);
			_stacks.iCurrent = -1;
			//Move to the bottom
			{
				AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->obj->contenedor();
				if(parent != NULL){
					const SI32 idx = parent->hijos()->indiceDe(itm->obj);
					if(idx != 0) _lyr->hijoMover(idx, 0);
				}
			}
		}
		//Show new
		if(idx >= 0 && idx < _stacks.array.use){
			STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, idx);
			itm->anim.secsDur		= animDur;
			itm->anim.secsCur		= 0.0f;
			itm->anim.from.side		= animInFrom;
			itm->anim.from.relPos	= 1.0f;
			itm->anim.from.relColor	= 1.0f;
			itm->anim.to.side		= ENSceneColSide_Count;
			itm->anim.to.relPos		= 1.0f;
			itm->anim.to.relColor	= 1.0f;
			itm->anim.acel			= FALSE;
			itm->deltaPos			= this->privDeltaForSize(itm->anim.from.side, itm->anim.from.relPos);
			itm->obj->setBorderVisible(itm->anim.secsDur > 0.0f);
			//Organize
			{
				itm->obj->organize(_width, _height, _margins);
				{
					const UI8 relColor = (255.0f * itm->anim.from.relColor);
					const NBCajaAABB box = itm->obj->cajaAABBLocal();
					itm->obj->establecerTraslacion(0.0f + itm->deltaPos.x - box.xMin, 0.0f + itm->deltaPos.y - box.yMax);
					itm->obj->establecerMultiplicadorColor8(relColor, relColor, relColor, 255);
					//PRINTF_INFO("Moved col-panel-obj to(%f, %f) from-box-x(%f, %f)-y(%f, %f).\n", itm->obj->traslacion().x, itm->obj->traslacion().y, box.xMin, box.xMax, box.yMin, box.yMax);
				}
			}
			//Move to the top
			{
				AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->obj->contenedor();
				if(parent == NULL) _lyr->agregarObjetoEscena(itm->obj);
			}
			_stacks.iCurrent = idx;
		}
	}
}

BOOL AUSceneColsPanel::push(AUEscenaObjeto* obj, AUSceneContentI* itfCol, AUEscenaListaItemI* itfItm, const ENSceneColSide animOutTo, const ENSceneColSide animInFrom, const ENSceneColStackPushMode pushMode, const ENSceneColMarginsMode marginsMode){
	BOOL r = FALSE;
	if(_stacks.iCurrent >= 0 && _stacks.iCurrent < _stacks.array.use){
		STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, _stacks.iCurrent);
		r = itm->obj->push(obj, itfCol, itfItm, animOutTo, animInFrom, pushMode, marginsMode);
	}
	return r;
}

BOOL AUSceneColsPanel::pop(const ENSceneColSide animOutTo, const ENSceneColSide animInFrom){
	BOOL r = FALSE;
	if(_stacks.iCurrent >= 0 && _stacks.iCurrent < _stacks.array.use){
		STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, _stacks.iCurrent);
		r = itm->obj->pop(animOutTo, animInFrom);
	}
	return r;
}

BOOL AUSceneColsPanel::getStatusBarDef(STTSSceneBarStatus* dst){
	BOOL r = FALSE;
	if(_stacks.iCurrent >= 0 && _stacks.iCurrent < _stacks.array.use){
		STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, _stacks.iCurrent);
		r = itm->obj->getStatusBarDef(dst);
	}
	return r;
}

BOOL AUSceneColsPanel::execBarBackCmd(){
	BOOL r = FALSE;
	if(_stacks.iCurrent >= 0 && _stacks.iCurrent < _stacks.array.use){
		STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, _stacks.iCurrent);
		r = itm->obj->execBarBackCmd();
	}
	return r;
}

void AUSceneColsPanel::execBarIconCmd(const SI32 iIcon){
	if(_stacks.iCurrent >= 0 && _stacks.iCurrent < _stacks.array.use){
		STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, _stacks.iCurrent);
		itm->obj->execBarIconCmd(iIcon);
	}
}

void AUSceneColsPanel::applySearchFilter(const char** wordsLwr, const SI32 wordsLwrSz){
	if(_stacks.iCurrent >= 0 && _stacks.iCurrent < _stacks.array.use){
		STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, _stacks.iCurrent);
		itm->obj->applySearchFilter(wordsLwr, wordsLwrSz);
	}
}

//

void AUSceneColsPanel::organize(){
	this->privOrganize(_width, _height, _margins);
}

void AUSceneColsPanel::organize(const float width, const float height, const NBMargenes margins){
	this->privOrganize(width, height, margins);
}

void AUSceneColsPanel::privOrganize(const float width, const float height, const NBMargenes margins){
	//Organize childs
	{
		{
			SI32 i; for(i = 0; i < _stacks.array.use; i++){
				STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, i);
				itm->obj->organize(width, height, margins);
				{
					const NBCajaAABB box = itm->obj->cajaAABBLocal();
					itm->obj->establecerTraslacion(0.0f + itm->deltaPos.x - box.xMin, 0.0f + itm->deltaPos.y - box.yMax);
					//PRINTF_INFO("Moved col-panel-obj to(%f, %f) from-box-x(%f, %f)-y(%f, %f).\n", itm->obj->traslacion().x, itm->obj->traslacion().y, box.xMin, box.xMax, box.yMin, box.yMax);
				}
			}
		}
	}
	//Set limits
	{
		this->establecerLimites(0.0f, width, -height, 0.0f);
	}
	//Set props
	_width			= width;
	_height			= height;
	_margins		= margins;
}

//

void AUSceneColsPanel::updateMargins(const NBMargenes margins){
	//Apply to stacks
	{
		SI32 i; for(i = 0; i < _stacks.array.use; i++){
			STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, i);
			itm->obj->updateMargins(margins);
		}
		
	}
	//Organize
	this->privOrganize(_width, _height, margins);
}

void AUSceneColsPanel::scrollToTop(const float secs){
	if(_stacks.iCurrent >= 0 && _stacks.iCurrent < _stacks.array.use){
		STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, _stacks.iCurrent);
		itm->obj->scrollToTop(secs);
	}
}

//

void AUSceneColsPanel::tickAnimacion(float segsTranscurridos){
	if(this->idEscena >= 0){
		//Animate childs
		{
			SI32 i; for(i = 0; i < _stacks.array.use; i++){
				STSceneColsPanelItm* itm = NBArray_itmPtrAtIndex(&_stacks.array, STSceneColsPanelItm, i);
				if(itm->anim.secsDur > 0.0f){
					itm->anim.secsCur += segsTranscurridos;
					float rel  = itm->anim.secsCur / itm->anim.secsDur;
					{
						//Limit rel
						if(rel >= 1.0f){
							rel = 1.0f;
							itm->anim.secsCur = itm->anim.secsDur = 0.0f;
						}
						//Apply
						{
							const NBCajaAABB box	= itm->obj->cajaAABBLocal();
							const STNBPoint from	= this->privDeltaForSize(itm->anim.from.side, itm->anim.from.relPos);
							const STNBPoint to		= this->privDeltaForSize(itm->anim.to.side, itm->anim.to.relPos);
							UI8 relColor			= 255;
							if(itm->anim.acel){
								const float rel2 = (rel * rel);
								itm->deltaPos.x = (from.x + ((to.x - from.x) * rel2));
								itm->deltaPos.y = (from.y + ((to.y - from.y) * rel2));
								relColor		= 255.0f * (itm->anim.from.relColor + ((itm->anim.to.relColor - itm->anim.from.relColor) * rel2));
							} else {
								const float relInv = (1.0f - rel);
								const float rel2 = (relInv * relInv);
								itm->deltaPos.x = (to.x + ((from.x - to.x) * rel2));
								itm->deltaPos.y = (to.y + ((from.y - to.y) * rel2));
								relColor		= 255.0f * (itm->anim.to.relColor + ((itm->anim.from.relColor - itm->anim.to.relColor) * rel2));
							}
							itm->obj->establecerTraslacion(0.0f + itm->deltaPos.x - box.xMin, 0.0f + itm->deltaPos.y - box.yMax);
							itm->obj->establecerMultiplicadorColor8(relColor, relColor, relColor, 255);
							itm->obj->setBorderVisible(itm->anim.secsDur > 0.0f);
						}
						//Remove from layer
						if(i != _stacks.iCurrent){
							if(rel >= 1.0f){
								//Remove from layer
								{
									AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->obj->contenedor();
									if(parent != NULL) parent->quitarObjetoEscena(itm->obj);
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

AUOBJMETODOS_CLASESID_MULTICLASE(AUSceneColsPanel, AUEscenaContenedorLimitado)
AUOBJMETODOS_CLASESNOMBRES_MULTICLASE(AUSceneColsPanel, "AUSceneColsPanel", AUEscenaContenedorLimitado)
AUOBJMETODOS_CLONAR_NULL(AUSceneColsPanel)
