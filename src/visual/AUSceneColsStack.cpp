//
//  AUSceneColsStacksStack.cpp
//  thinstream
//
//  Created by Marcos Ortega on 9/2/18.
//

#include "visual/TSVisualPch.h"
#include "visual/AUSceneColsStack.h"

#define AU_SCENE_COL_STACK_ANIM_DUR		0.35f

AUSceneColsStack::AUSceneColsStack(const SI32 iScene, AUEscenaObjeto* touchInheritor, const float width, const float height, const NBMargenes margins, const STNBColor8 defBgColor)
: AUEscenaContenedorLimitado()
, _iScene(iScene)
{
	NB_DEFINE_NOMBRE_PUNTERO(this, "AUSceneColsStack")
	_touchInheritor	= touchInheritor;
	_width			= 0.0f;
	_height			= 0.0f;
	_margins		= margins;
	_defBgColor		= defBgColor;
	//
	NBArray_init(&_stack, sizeof(STSceneColsStackItm), NULL);
	NBArray_init(&_removing, sizeof(STSceneColsStackItm), NULL);
	//layers
	{
		_lyr			= new(this) AUEscenaContenedorLimitado();
		_lyr->establecerEscuchadorTouches(this, this);
		this->agregarObjetoEscena(_lyr);
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
	//touch
	{
		_touch.first	= NULL;
		NBMemory_setZero(_touch.startTouch);
	}
	//
	this->privOrganize(width, height, margins);
	//
	NBGestorAnimadores::agregarAnimador(this, this);
}

AUSceneColsStack::~AUSceneColsStack(){
	AU_OBJETO_ASSERT_IS_ALIVE_THIS
	NB_GESTOR_AUOBJETOS_VALIDATE_ALL_OBJETS_TO_BE_ALIVE()
	NBGestorAnimadores::quitarAnimador(this);
	_touchInheritor = NULL;
	{
		SI32 i; for(i = 0; i < _stack.use; i++){
			STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, i);
			if(itm->obj != NULL) itm->obj->liberar(NB_RETENEDOR_THIS); itm->obj = NULL;
		}
		NBArray_empty(&_stack);
		NBArray_release(&_stack);
	}
	{
		SI32 i; for(i = 0; i < _removing.use; i++){
			STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_removing, STSceneColsStackItm, i);
			if(itm->obj != NULL) itm->obj->liberar(NB_RETENEDOR_THIS); itm->obj = NULL;
		}
		NBArray_empty(&_removing);
		NBArray_release(&_removing);
	}
	{
		if(_lyr != NULL) _lyr->liberar(NB_RETENEDOR_THIS); _lyr = NULL;
		if(_fg != NULL) _fg->liberar(NB_RETENEDOR_THIS); _fg = NULL;
	}
	NB_GESTOR_AUOBJETOS_VALIDATE_ALL_OBJETS_TO_BE_ALIVE()
}

//

float AUSceneColsStack::getWidth() const {
	return _width;
}

float AUSceneColsStack::getHeight() const {
	return _height;
}

NBMargenes AUSceneColsStack::getMargins() const{
	return _margins;
}

//

STNBPoint AUSceneColsStack::privDeltaForSize(const ENSceneColSide side, const float rel){
	STNBPoint r;
	r.x = (side == ENSceneColSide_Left ? -_width : side == ENSceneColSide_Right ? _width : 0.0f) * rel;
	r.y = (side == ENSceneColSide_Btm ? -_height : side == ENSceneColSide_Top ? _height : 0.0f) * rel;
	return r;
}

SI32 AUSceneColsStack::getSize() const {
	return _stack.use;
}

BOOL AUSceneColsStack::push(AUEscenaObjeto* obj, AUSceneContentI* itfCol, AUEscenaListaItemI* itfItm, const ENSceneColSide animOutTo, const ENSceneColSide animInFrom, const ENSceneColStackPushMode pushMode, const ENSceneColMarginsMode marginsMode){
	BOOL r = FALSE;
	const float animDur = AU_SCENE_COL_STACK_ANIM_DUR;
	if(obj != NULL && itfCol != NULL && itfItm != NULL){
		//Start animating-out current col
		if(_stack.use > 0){
			STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, _stack.use - 1);
			itm->anim.secsDur		= animDur;
			itm->anim.secsCur		= 0.0f;
			itm->anim.from.side		= ENSceneColSide_Count;
			itm->anim.from.relPos	= 1.0f;
			itm->anim.from.relColor	= 1.0f;
			itm->anim.to.side		= animOutTo;
			itm->anim.to.relPos		= 0.25f;
			itm->anim.to.relColor	= 0.25f;
			itm->anim.acel			= TRUE;
			itm->deltaPos			= this->privDeltaForSize(itm->anim.from.side, itm->anim.from.relPos);
			itm->obj->setBorderVisible(itm->anim.secsDur > 0.0f);
			//
			if(pushMode == ENSceneColStackPushMode_Replace){
				//Move to removing-array
				NBArray_addValue(&_removing, *itm);
				NBArray_removeItemAtIndex(&_stack, _stack.use - 1);
			}
		}
		//Add new col and start animation
		{
			STSceneColsStackItm itm;
			NBMemory_setZeroSt(itm, STSceneColsStackItm);
			itm.obj = new(this) AUSceneCol(_iScene, obj, itfCol, itfItm, _lyr, _width, _height, _margins, marginsMode, _defBgColor);
			itm.anim.secsDur		= animDur;
			itm.anim.secsCur		= 0.0f;
			itm.anim.from.side		= animInFrom;
			itm.anim.from.relPos	= 1.0f;
			itm.anim.from.relColor	= 1.0f;
			itm.anim.to.side		= ENSceneColSide_Count;
			itm.anim.to.relPos		= 1.0f;
			itm.anim.to.relColor	= 1.0f;
			itm.anim.acel			= FALSE;
			itm.deltaPos			= this->privDeltaForSize(itm.anim.from.side, itm.anim.from.relPos);
			itm.obj->setBorderVisible(itm.anim.secsDur > 0.0f);
			//Set initial-pos
			{
				const UI8 relColor = (255.0f * itm.anim.from.relColor);
				const NBCajaAABB box = itm.obj->cajaAABBLocal();
				itm.obj->establecerTraslacion(0.0f + itm.deltaPos.x - box.xMin, 0.0f + itm.deltaPos.y - box.yMax);
				itm.obj->establecerMultiplicadorColor8(relColor, relColor, relColor, 255);
				//PRINTF_INFO("Moved col-stack-obj to(%f, %f) from-box-x(%f, %f)-y(%f, %f).\n", itm.obj->traslacion().x, itm.obj->traslacion().y, box.xMin, box.xMax, box.yMin, box.yMax);
			}
			//Add to the top
			_lyr->agregarObjetoEscena(itm.obj);
			//
			NBArray_addValue(&_stack, itm);
			r = TRUE;
		}
		//PRINTF_INFO("Push, stack-size(%d).\n", _stack.use);
	}
	return r;
}

BOOL AUSceneColsStack::pop(const ENSceneColSide animOutTo, const ENSceneColSide animInFrom){
	BOOL r = FALSE;
	const float animDur = AU_SCENE_COL_STACK_ANIM_DUR;
	if(_stack.use > 0){
		//PRINTF_INFO("Pop, stack-size(%d).\n", _stack.use);
		//Remove current
		{
			STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, _stack.use - 1);
			itm->anim.secsDur		= animDur;
			itm->anim.secsCur		= 0.0f;
			itm->anim.from.side		= ENSceneColSide_Count;
			itm->anim.from.relPos	= 1.0f;
			itm->anim.from.relColor	= 1.0f;
			itm->anim.to.side		= animOutTo;
			itm->anim.to.relPos		= 1.0f;
			itm->anim.to.relColor	= 1.0f;
			itm->anim.acel			= TRUE;
			itm->deltaPos			= this->privDeltaForSize(itm->anim.from.side, itm->anim.from.relPos);
			itm->obj->setBorderVisible(itm->anim.secsDur > 0.0f);
			//Move to removing-array
			NBArray_addValue(&_removing, *itm);
			NBArray_removeItemAtIndex(&_stack, _stack.use - 1);
		}
		//Activate new top
		if(_stack.use > 0){
			STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, _stack.use - 1);
			itm->anim.secsDur		= animDur;
			itm->anim.secsCur		= 0.0f;
			itm->anim.from.side		= animInFrom;
			itm->anim.from.relPos	= 0.25f;
			itm->anim.from.relColor	= 0.25f;
			itm->anim.to.side		= ENSceneColSide_Count;
			itm->anim.to.relPos		= 1.0f;
			itm->anim.to.relColor	= 1.0f;
			itm->anim.acel			= TRUE;
			itm->deltaPos			= this->privDeltaForSize(itm->anim.from.side, itm->anim.from.relPos);
			itm->obj->setBorderVisible(itm->anim.secsDur > 0.0f);
			//Add-or-move to the bottom
			{
				AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->obj->contenedor();
				if(parent != NULL){
					const SI32 idx = parent->hijos()->indiceDe(itm->obj); NBASSERT(idx >= 0)
					if(idx != 0) parent->hijoMover(idx, 0);
				} else {
					_lyr->agregarObjetoEscenaEnIndice(itm->obj, 0);
				}
			}
			//Set initial-pos
			{
				const UI8 relColor = (255.0f * itm->anim.from.relColor);
				const NBCajaAABB box = itm->obj->cajaAABBLocal();
				itm->obj->establecerTraslacion(0.0f + itm->deltaPos.x - box.xMin, 0.0f + itm->deltaPos.y - box.yMax);
				itm->obj->establecerMultiplicadorColor8(relColor, relColor, relColor, 255);
				//PRINTF_INFO("Moved col-stack-obj to(%f, %f) from-box-x(%f, %f)-y(%f, %f).\n", itm.obj->traslacion().x, itm.obj->traslacion().y, box.xMin, box.xMax, box.yMin, box.yMax);
			}
		}
		r = TRUE;
	}
	return r;
}

AUEscenaObjeto* AUSceneColsStack::getTopObj() const {
	AUEscenaObjeto* r = NULL;
	if(_stack.use > 0){
		STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, _stack.use - 1);
		r = itm->obj->getObj();
	}
	return r;
}

BOOL AUSceneColsStack::getStatusBarDef(STTSSceneBarStatus* dst){
	BOOL r = FALSE;
	if(_stack.use > 0){
		STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, _stack.use - 1);
		r = itm->obj->getStatusBarDef(dst);
	}
	return r;
}

BOOL AUSceneColsStack::execBarBackCmd(){
	BOOL r = FALSE;
	if(_stack.use > 0){
		STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, _stack.use - 1);
		r = itm->obj->execBarBackCmd();
	}
	return r;
}

void AUSceneColsStack::execBarIconCmd(const SI32 iIcon){
	if(_stack.use > 0){
		STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, _stack.use - 1);
		itm->obj->execBarIconCmd(iIcon);
	}
}

void AUSceneColsStack::applySearchFilter(const char** wordsLwr, const SI32 wordsLwrSz){
	if(_stack.use > 0){
		STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, _stack.use - 1);
		itm->obj->applySearchFilter(wordsLwr, wordsLwrSz);
	}
}

//

void AUSceneColsStack::organize(){
	this->privOrganize(_width, _height, _margins);
}

void AUSceneColsStack::organize(const float width, const float height, const NBMargenes margins){
	this->privOrganize(width, height, margins);
}

void AUSceneColsStack::privOrganize(const float width, const float height, const NBMargenes margins){
	//Organize childs
	{
		{
			SI32 i; for(i = 0; i < _stack.use; i++){
				STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, i);
				itm->obj->organize(width, height, margins);
				{
					const NBCajaAABB box = itm->obj->cajaAABBLocal();
					itm->obj->establecerTraslacion(0.0f + itm->deltaPos.x - box.xMin, 0.0f + itm->deltaPos.y - box.yMax);
					//PRINTF_INFO("Moved col-stack-obj to(%f, %f) from-box-x(%f, %f)-y(%f, %f).\n", itm->obj->traslacion().x, itm->obj->traslacion().y, box.xMin, box.xMax, box.yMin, box.yMax);
				}
			}
		}
		{
			SI32 i; for(i = 0; i < _removing.use; i++){
				STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_removing, STSceneColsStackItm, i);
				itm->obj->organize(width, height, margins);
				{
					const NBCajaAABB box = itm->obj->cajaAABBLocal();
					itm->obj->establecerTraslacion(0.0f + itm->deltaPos.x - box.xMin, 0.0f + itm->deltaPos.y - box.yMax);
					//PRINTF_INFO("Moved col-stack-obj-removing to(%f, %f) from-box-x(%f, %f)-y(%f, %f).\n", itm->obj->traslacion().x, itm->obj->traslacion().y, box.xMin, box.xMax, box.yMin, box.yMax);
				}
			}
		}
	}
	//Fg
	{
		_fg->moverVerticeHacia(0, 0, 0);
		_fg->moverVerticeHacia(1, width, 0);
		_fg->moverVerticeHacia(2, width, -height);
		_fg->moverVerticeHacia(3, 0, -height);
	}
	//Set limits
	{
		_lyr->establecerLimites(0.0f, width, -height, 0.0f);
		this->establecerLimites(0.0f, width, -height, 0.0f);
	}
	//Set props
	_width			= width;
	_height			= height;
	_margins		= margins;
}

void AUSceneColsStack::updateMargins(const NBMargenes margins){
	//Apply to cols
	{
		SI32 i; for(i = 0; i < _stack.use; i++){
			STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, i);
			itm->obj->updateMargins(margins);
		}
	}
	{
		SI32 i; for(i = 0; i < _removing.use; i++){
			STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_removing, STSceneColsStackItm, i);
			itm->obj->updateMargins(margins);
		}
	}
	//Organize
	this->privOrganize(_width, _height, margins);
}

void AUSceneColsStack::setBorderVisible(const BOOL visible){
	//_fg->setVisibleAndAwake(visible);
}

void AUSceneColsStack::scrollToTop(const float secs){
	if(_stack.use > 0){
		STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, _stack.use - 1);
		itm->obj->scrollToTop(secs);
	}
}

//

void AUSceneColsStack::tickAnimacion(float segsTranscurridos){
	if(this->idEscena >= 0){
		BOOL colRemoved = FALSE;
		//Animate childs
		{
			//
			{
				SI32 i; for(i = 0; i < _stack.use; i++){
					STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, i);
					if(itm->anim.secsDur > 0.0f){
						itm->anim.secsCur += segsTranscurridos;
						{
							float rel  = itm->anim.secsCur / itm->anim.secsDur;
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
							//Non-top-itm (hide)
							if((i + 1) != _stack.use){
								if(rel >= 1.0f){
									//Remove from layer
									{
										AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->obj->contenedor();
										if(parent != NULL) parent->quitarObjetoEscena(itm->obj);
										//PRINTF_INFO("ItmCol removed form scene: #%d d %d.\n", (i + 1), _stack.use);
									}
								}
							}
						}
					}
				}
			}
			//Removing-list
			{
				SI32 i; for(i = (_removing.use - 1); i >= 0; i--){
					STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_removing, STSceneColsStackItm, i);
					NBASSERT(itm->anim.secsDur > 0.0f) //must be animating
					itm->anim.secsCur += segsTranscurridos;
					{
						float rel  = itm->anim.secsCur / itm->anim.secsDur;
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
								const float rel2= (rel * rel);
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
						//Remove
						if(rel >= 1.0f){
							//Remove from layer
							{
								AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->obj->contenedor();
								if(parent != NULL) parent->quitarObjetoEscena(itm->obj);
							}
							//Release and remove form array
							{
								if(itm->obj != NULL) itm->obj->liberar(NB_RETENEDOR_THIS); itm->obj = NULL;
							}
							//PRINTF_INFO("AUSceneColsStack, itm released and removed from scene: #%d d %d.\n", (i + 1), _removing.use);
							NBArray_removeItemAtIndex(&_removing, i);
							colRemoved = TRUE;
						}
					}
				}
			}
		}
/*#		ifdef CONFIG_NB_GESTOR_MEMORIA_REGISTRAR_BLOQUES
		if(colRemoved){
			PRINTF_INFO("--------------------------\n");
			PRINTF_INFO("COLUMN REMOVED FROM SATCK.\n");
			PRINTF_INFO("REMAINING OBJECTS:\n");
			PRINTF_INFO("--------------------------\n");
			NBGestorMemoria::debug_imprimePunterosEnUso();
			PRINTF_INFO("-------------------------- (END)\n");
		}
#		endif*/
	}
}

//TOUCHES

void AUSceneColsStack::touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto){
	if(objeto == _lyr){
		//Start of drag
		if(_touch.first == NULL){
			_touch.first	= touch;
			{
				const NBPunto posLocal = _lyr->coordenadaEscenaALocal(posTouchEscena.x, posTouchEscena.y);
				_touch.startTouch.x		= posLocal.x;
				_touch.startTouch.y		= posLocal.y;
			}
		}
	}
}

void AUSceneColsStack::touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	if(objeto == _lyr){
		//Drag continued
		if(_touch.first == touch){
			const NBPunto posLocal = _lyr->coordenadaEscenaALocal(posActualEscena.x, posActualEscena.y);
			const STNBPoint delta = NBST_P(STNBPoint, 
				posLocal.x - _touch.startTouch.x
				, posLocal.y - _touch.startTouch.y
			);
			//Apply scroll
			if(_stack.use <= 1){
				if(_touchInheritor != NULL){
					objeto->liberarTouch(touch, posInicialEscena, posAnteriorEscena, posActualEscena, TRUE, _touchInheritor);
				}
			} else {
				STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, _stack.use - 1);
				if(itm->anim.secsDur <= 0){
					const NBCajaAABB box	= itm->obj->cajaAABBLocal();
					UI8 relColor			= 255;
					itm->deltaPos.x			= (delta.x < 0.0f ? 0.0f : delta.x);
					itm->deltaPos.y			= 0.0f;
					itm->obj->establecerTraslacion(0.0f + itm->deltaPos.x - box.xMin, 0.0f + itm->deltaPos.y - box.yMax);
					itm->obj->establecerMultiplicadorColor8(relColor, relColor, relColor, 255);
					itm->obj->setBorderVisible(itm->deltaPos.x != 0.0f);
					{
						const STNBPoint deltaStart = this->privDeltaForSize(ENSceneColSide_Count, 1.0f);
						const STNBPoint deltaFinal = this->privDeltaForSize(ENSceneColSide_Right, 1.0f);
						float rel = itm->deltaPos.x / (deltaFinal.x - deltaStart.x);
						if(rel <= 0.0f){
							//Hide any previous col
							SI32 i; for(i = (_stack.use - 2); i >= 0; i--){
								STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, i);
								if(itm->anim.secsDur <= 0.0f){
									AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->obj->contenedor();
									if(parent != NULL){
										parent->quitarObjetoEscena(itm->obj);
										PRINTF_INFO("Col #%d/%d removed from scene by dragging.\n", (i + 1), _stack.use);
									}
								}
							}
						} else {
							//Ensure prev col is visible
							if(_stack.use > 1){
								STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, _stack.use - 2);
								if(itm->anim.secsDur <= 0.0f){
									//Add or move to the bottom
									{
										AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->obj->contenedor();
										if(parent != NULL){
											const SI32 idx = parent->hijos()->indiceDe(itm->obj); NBASSERT(idx >= 0)
											if(idx != 0) parent->hijoMover(idx, 0);
										} else {
											_lyr->agregarObjetoEscenaEnIndice(itm->obj, 0);
										}
									}
									//Apply state
									{
										const UI8 relColor = 255.0f * (0.25f + (0.75f * rel));
										const STNBPoint deltaStart = this->privDeltaForSize(ENSceneColSide_Left, 0.25f);
										const STNBPoint deltaFinal = this->privDeltaForSize(ENSceneColSide_Count, 1.0f);
										itm->deltaPos.x = (deltaStart.x + ((deltaFinal.x - deltaStart.x) * rel));
										itm->deltaPos.y = (deltaStart.y + ((deltaFinal.y - deltaStart.y) * rel));
										itm->obj->establecerTraslacion(0.0f + itm->deltaPos.x - box.xMin, 0.0f + itm->deltaPos.y - box.yMax);
										itm->obj->establecerMultiplicadorColor8(relColor, relColor, relColor, 255);
										itm->obj->setBorderVisible(itm->deltaPos.x != 0.0f);
									}
								}
							}
						}
					}
				}
			}
		}
	}
}

void AUSceneColsStack::touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	if(objeto == _lyr){
		//Drag ended
		if(_touch.first == touch){
			_touch.first = NULL;
			_touch.startTouch.x = _touch.startTouch.y = 0;
			//End scroll
			if(_stack.use > 0){
				STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, _stack.use - 1);
				if(itm->anim.secsDur <= 0){
					const STNBPoint deltaStart = this->privDeltaForSize(ENSceneColSide_Count, 1.0f);
					const STNBPoint deltaFinal = this->privDeltaForSize(ENSceneColSide_Right, 1.0f);
					float rel = itm->deltaPos.x / (deltaFinal.x - deltaStart.x);
					if(_stack.use <= 1 || rel <= 0.35f){
						//Restore as top-col
						itm->anim.secsCur		= 0.0f;
						itm->anim.secsDur		= AU_SCENE_COL_STACK_ANIM_DUR;
						itm->anim.acel			= FALSE;
						itm->anim.from.side		= ENSceneColSide_Right;
						itm->anim.from.relPos	= rel;
						itm->anim.from.relColor	= 1.0f;
						itm->anim.to.side		= ENSceneColSide_Count;
						itm->anim.to.relPos		= 1.0f;
						itm->anim.to.relColor	= 1.0f;
						PRINTF_INFO("Restoring from rel(%f).\n", rel);
						//Hide any previous col
						{
							SI32 i; for(i = (_stack.use - 2); i >= 0; i--){
								STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, i);
								if(itm->anim.secsDur <= 0.0f){
									AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->obj->contenedor();
									if(parent != NULL){
										itm->anim.secsCur		= 0.0f;
										itm->anim.secsDur		= AU_SCENE_COL_STACK_ANIM_DUR;
										itm->anim.acel			= TRUE;
										itm->anim.from.side		= ENSceneColSide_Left;
										itm->anim.from.relPos	= (0.25f * (1.0f - rel));
										itm->anim.from.relColor	= (0.25f + (0.75f * rel));
										itm->anim.to.side		= ENSceneColSide_Left;
										itm->anim.to.relPos		= 0.25f;
										itm->anim.to.relColor	= 1.0f;
									}
								}
							}
						}
					} else {
						//Remove as top-col
						itm->anim.secsCur		= 0.0f;
						itm->anim.secsDur		= AU_SCENE_COL_STACK_ANIM_DUR;
						itm->anim.acel			= TRUE;
						itm->anim.from.side		= ENSceneColSide_Right;
						itm->anim.from.relPos	= rel;
						itm->anim.from.relColor	= 1.0f;
						itm->anim.to.side		= ENSceneColSide_Right;
						itm->anim.to.relPos		= 1.0f;
						itm->anim.to.relColor	= 1.0f;
						NBArray_addValue(&_removing, *itm);
						NBArray_removeItemAtIndex(&_stack, _stack.use - 1);
						PRINTF_INFO("Removing from rel(%f).\n", rel);
						//Hide any previous col
						{
							SI32 i; for(i = (_stack.use - 1); i >= 0; i--){
								STSceneColsStackItm* itm = NBArray_itmPtrAtIndex(&_stack, STSceneColsStackItm, i);
								if(itm->anim.secsDur <= 0.0f){
									if((i + 1) == _stack.use){
										//Set as current
										itm->anim.secsCur		= 0.0f;
										itm->anim.secsDur		= AU_SCENE_COL_STACK_ANIM_DUR;
										itm->anim.acel			= FALSE;
										itm->anim.from.side		= ENSceneColSide_Left;
										itm->anim.from.relPos	= (0.25f * (1.0f - rel));
										itm->anim.from.relColor	= (0.25f + (0.75f * rel));
										itm->anim.to.side		= ENSceneColSide_Count;
										itm->anim.to.relPos		= 1.0f;
										itm->anim.to.relColor	= 1.0f;
									} else {
										//Remove
										AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->obj->contenedor();
										if(parent != NULL){
											itm->anim.secsCur		= 0.0f;
											itm->anim.secsDur		= AU_SCENE_COL_STACK_ANIM_DUR;
											itm->anim.acel			= TRUE;
											itm->anim.from.side		= ENSceneColSide_Left;
											itm->anim.from.relPos	= (0.25f * (1.0f - rel));
											itm->anim.from.relColor	= (0.25f + (0.75f * rel));
											itm->anim.to.side		= ENSceneColSide_Left;
											itm->anim.to.relPos		= 0.25f;
											itm->anim.to.relColor	= 1.0f;
										}
									}
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

AUOBJMETODOS_CLASESID_MULTICLASE(AUSceneColsStack, AUEscenaContenedorLimitado)
AUOBJMETODOS_CLASESNOMBRES_MULTICLASE(AUSceneColsStack, "AUSceneColsStack", AUEscenaContenedorLimitado)
AUOBJMETODOS_CLONAR_NULL(AUSceneColsStack)
