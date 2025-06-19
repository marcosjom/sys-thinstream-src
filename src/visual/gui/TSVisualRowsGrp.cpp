//
//  TSVisualRowsGrp.cpp
//  thinstream
//
//  Created by Marcos Ortega on 8/3/19.
//

#include "visual/TSVisualPch.h"
#include "visual/gui/TSVisualRowsGrp.h"
//
#include "visual/TSColors.h"
#include "visual/TSFonts.h"

TSVisualRowsGrp::TSVisualRowsGrp(const SI32 iScene, const float anchoUsar, const float altoUsar, AUAnimadorObjetoEscena* animObjetos, AUFuenteRender* fntNames, AUFuenteRender* fntExtras, const char* grpName, const UI32 bitsMask)
: AUEscenaContenedorLimitado()
{
	NB_DEFINE_NOMBRE_PUNTERO(this, "TSVisualRowsGrp")
	_iScene			= iScene;
	_anchoUsar		= 0.0f;
	_altoUsar		= 0.0f;
	NBMemory_setZero(_refDatos);
	//
	if(animObjetos != NULL){
		_animObjetos	= animObjetos;
		_animObjetos->retener(NB_RETENEDOR_THIS);
	} else {
		_animObjetos	= new(this) AUAnimadorObjetoEscena();
	}
	//
	if(fntNames != NULL){
		_fntNames	= fntNames;
		_fntNames->retener(NB_RETENEDOR_THIS);
	} else {
		_fntNames	= TSFonts::font(ENTSFont_ContentTitle);
		_fntNames->retener(NB_RETENEDOR_THIS);
	}
	//
	if(fntExtras != NULL){
		_fntExtras	= fntExtras;
		_fntExtras->retener(NB_RETENEDOR_THIS);
	} else {
		_fntExtras	= TSFonts::font(ENTSFont_ContentMid);
		_fntExtras->retener(NB_RETENEDOR_THIS);
	}
	//
	_isAnimating			= TRUE;
	_multipleOptionsRows	= FALSE;
	_bitsMask				= bitsMask;
	//
	{
		_title.isVisible	= TRUE;
		_title.isPosFixed	= FALSE;
		_title.pos			= 0.0f;
		_title.align		= ENNBTextLineAlignH_Left;
	}
	//Sync
	{
		NBMemory_setZero(_sync);
	}
	//
	{
		_empty.lyr		= new(this) AUEscenaContenedor();
		this->agregarObjetoEscena(_empty.lyr);
		//
		const STNBColor8 color = TSColors::colorDef(ENTSColor_Gray)->normal;
		_empty.bg		= new(this) AUEscenaSprite();
		_empty.bg->setVisibleAndAwake(FALSE);
		_empty.bg->establecerMultiplicadorColor8(255, 255, 255, 255);
		_empty.lyr->agregarObjetoEscena(_empty.bg);
		//
		_empty.txt		= new(this) AUEscenaTexto(_fntExtras);
		_empty.txt->establecerAlineaciones(ENNBTextLineAlignH_Center, ENNBTextAlignV_FromTop);
		_empty.txt->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
		_empty.txt->establecerTexto("(empty)");
		_empty.txt->setVisibleAndAwake(FALSE);
		_empty.lyr->agregarObjetoEscena(_empty.txt);
		//
		_empty.line		= new(this) AUEscenaFigura(ENEscenaFiguraTipo_Linea);
		_empty.line->agregarCoordenadaLocal(0.0f, 0.0f);
		_empty.line->agregarCoordenadaLocal(0.0f, 0.0f);
		_empty.line->establecerMultiplicadorColor8(color.r, color.g, color.b, 55);
		_empty.line->setVisibleAndAwake(FALSE);
		_empty.lyr->agregarObjetoEscena(_empty.line);
	}
	//grp
	{
		NBMemory_setZero(_grp);
		//
		_grp.layerBtns = new(this) AUEscenaContenedor();
		this->agregarObjetoEscena(_grp.layerBtns);
		//
		{
			const BOOL showLines = ((_bitsMask & ENTSVisualRowsGrpBitsMsk_HideLines) == 0);
			_grp.layerLines = new(this) AUEscenaContenedor();
			_grp.layerLines->setVisibleAndAwake(showLines);
			this->agregarObjetoEscena(_grp.layerLines);
		}
		//
		{
			const STNBColor8 color = TSColors::colorDef(ENTSColor_LightFront)->normal;
			_grp.bg		= new(this) AUEscenaSprite();
			_grp.bg->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
			this->agregarObjetoEscena(_grp.bg);
		}
		//
		{
			const STNBColor8 color = TSColors::colorDef(ENTSColor_Gray)->normal;
			_grp.title	= new(this) AUEscenaTexto(_fntNames);
			_grp.title->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
			_grp.title->establecerTexto(grpName);
			this->agregarObjetoEscena(_grp.title);
		}
		//
		NBString_initWithStr(&_grp.grpName, grpName);
	}
	//Itms
	{
		NBArray_init(&_itms, sizeof(STVisualRowsGrpItm), NULL);
		_itmsVisibleCount = 0;
	}
	//Organize
	this->privOrganizarContenido(anchoUsar, altoUsar, false, NULL, 0.0f);
	//
	NBGestorAnimadores::agregarAnimador(this, this);
}

TSVisualRowsGrp::~TSVisualRowsGrp(){
	NBGestorAnimadores::quitarAnimador(this);
	//
	if(_animObjetos != NULL) _animObjetos->liberar(NB_RETENEDOR_THIS); _animObjetos = NULL;
	//Fonts
	{
		if(_fntNames != NULL) _fntNames->liberar(NB_RETENEDOR_THIS); _fntNames = NULL;
		if(_fntExtras != NULL) _fntExtras->liberar(NB_RETENEDOR_THIS); _fntExtras = NULL;
	}
	//Itms
	{
		this->rowsEmpty();
		NBASSERT(_itms.use == 0)
		NBArray_release(&_itms);
	}
	//Grp
	{
		STVisualRowsGrp* itm = &_grp;
		if(itm->layerBtns != NULL) itm->layerBtns->liberar(NB_RETENEDOR_THIS); itm->layerBtns = NULL;
		if(itm->layerLines != NULL) itm->layerLines->liberar(NB_RETENEDOR_THIS); itm->layerLines = NULL;
		if(itm->title != NULL) itm->title->liberar(NB_RETENEDOR_THIS); itm->title = NULL;
		if(itm->bg != NULL) itm->bg->liberar(NB_RETENEDOR_THIS); itm->bg = NULL;
		NBString_release(&itm->grpName);
	}
	//Empty
	{
		if(_empty.lyr != NULL) _empty.lyr->liberar(NB_RETENEDOR_THIS); _empty.lyr = NULL;
		if(_empty.bg != NULL) _empty.bg->liberar(NB_RETENEDOR_THIS); _empty.bg = NULL;
		if(_empty.txt != NULL) _empty.txt->liberar(NB_RETENEDOR_THIS); _empty.txt = NULL;
		if(_empty.line != NULL) _empty.line->liberar(NB_RETENEDOR_THIS); _empty.line = NULL;
	}
}

//

UI32 TSVisualRowsGrp::getGrpID() const {
	return _grp.grpID;
}

const char* TSVisualRowsGrp::getGrpName() const {
	return _grp.grpName.str;
}

void TSVisualRowsGrp::setGrpID(const UI32 grpID){
	_grp.grpID = grpID;
}

void TSVisualRowsGrp::setGrpName(const char* grpName){
	NBString_set(&_grp.grpName, grpName);
	_grp.title->establecerTexto(grpName);
}

//

SI32 TSVisualRowsGrp::getRowsCount() const {
	return _itms.use;
}

//

void TSVisualRowsGrp::setTitlePos(const BOOL fixed, const float xPos, const ENNBTextLineAlignH align){
	_title.isPosFixed	= fixed;
	_title.pos			= xPos;
	_title.align		= align;
}

void TSVisualRowsGrp::setTitleVisible(const BOOL visible){
	_title.isVisible = visible;
	_grp.bg->setVisibleAndAwake(visible);
	_grp.title->setVisibleAndAwake(visible);
}

void TSVisualRowsGrp::setLinesVisible(const BOOL visible){
	_grp.layerLines->setVisibleAndAwake(visible);
}

//

void TSVisualRowsGrp_releaseItm_(STVisualRowsGrpItm* itm){
	if(itm->obj != NULL){
		AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->obj->contenedor();
		if(parent != NULL && parent == itm->lyr){
			parent->quitarObjetoEscena(itm->obj);
		}
		itm->obj->liberar(NB_RETENEDOR_NULL);
		itm->obj = NULL;
	}
	if(itm->line != NULL){
		AUEscenaContenedor* parent = (AUEscenaContenedor*)itm->line->contenedor();
		if(parent != NULL){
			parent->quitarObjetoEscena(itm->line);
		}
		itm->line->liberar(NB_RETENEDOR_NULL);
		itm->line = NULL;
	}
	if(itm->lyr != NULL){
		itm->lyr->liberar(NB_RETENEDOR_NULL);
		itm->lyr = NULL;
	}
	itm->itf = NULL;
	{
		SI32 i; for(i = 0; i < itm->options.arr.use; i++){
			STVisualRowsGrpItmOpt* opt = NBArray_itmPtrAtIndex(&itm->options.arr, STVisualRowsGrpItmOpt, i);
			NBString_release(&opt->optionId);
			if(opt->btn != NULL){
				AUEscenaContenedor* parent = (AUEscenaContenedor*)opt->btn->contenedor();
				if(parent != NULL){
					parent->quitarObjetoEscena(opt->btn);
				}
				opt->btn->liberar(NB_RETENEDOR_NULL);
				opt->btn = NULL;
			}
		}
		NBArray_empty(&itm->options.arr);
		NBArray_release(&itm->options.arr);
	}
}

//Sync

void TSVisualRowsGrp::syncStart(){
	//Flag all
	SI32 i; for(i = 0; i < _itms.use; i++){
		STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, i);
		itm->synced = FALSE;
	}
	//To determine the order
	_sync.addedCount = 0;
}

SI32 TSVisualRowsGrp::syncAdd(AUEscenaObjeto* obj, AUEscenaListaItemI* itf){
	return this->syncAddAtIdx(obj, itf, _sync.addedCount);
}

SI32 TSVisualRowsGrp::syncAddAtIdx(AUEscenaObjeto* obj, AUEscenaListaItemI* itf, const SI32 idx){
	SI32 r = -1;
	//Search
	SI32 i; for(i = 0; i < _itms.use; i++){
		STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, i);
		if(itm->obj == obj && itm->itf == itf){
			NBASSERT(!itm->synced) //Added twice or 'syncStart' not called
			itm->synced = TRUE;
			r = i;
			//Move (to the new order)
			if(idx >= 0 && idx < _itms.use){
				if(i != idx){
					//Swap objects
					STVisualRowsGrpItm itmTmp = *itm;
					STVisualRowsGrpItm* itm2 = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, idx);
					*itm	= *itm2;
					*itm2	= itmTmp;
					r = idx;
				}
			}
			break;
		}
	}
	//Add new
	if(i >= _itms.use){
		r = this->rowsAddAtIndex(obj, itf, idx);
	}
	//To determine the order
	_sync.addedCount++;
	return r;
}

void TSVisualRowsGrp::syncEndByRemovingUnsynced(){
	//Remove unflaged
	SI32 i; for(i = (_itms.use - 1); i >= 0; i--){
		STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, i);
		if(!itm->synced){
			TSVisualRowsGrp_releaseItm_(itm);
			NBArray_removeItemAtIndex(&_itms, i);
		}
	}
}

//Populate

void TSVisualRowsGrp::rowsEmpty(){
	SI32 i; for(i = 0; i < _itms.use; i++){
		STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, i);
		TSVisualRowsGrp_releaseItm_(itm);
	}
	NBArray_empty(&_itms);
	//To determine the order
	_sync.addedCount = 0;
}

SI32 TSVisualRowsGrp::rowsAdd(AUEscenaObjeto* obj, AUEscenaListaItemI* itf){
	const SI32 r = this->rowsAddAtIndex(obj, itf, _itms.use);
	//To determine the order
	_sync.addedCount++;
	return r;
}

SI32 TSVisualRowsGrp::rowsAddAtIndex(AUEscenaObjeto* obj, AUEscenaListaItemI* itf, const SI32 idx){
	SI32 r = -1;
	//Add to array
	const STNBColor8 color = TSColors::colorDef(ENTSColor_Gray)->normal;
	STVisualRowsGrpItm itm;
	NBMemory_setZeroSt(itm, STVisualRowsGrpItm);
	itm.lyr		= new(this) AUEscenaContenedorLimitado();
	itm.obj 	= obj; NBASSERT(obj != NULL)
	itm.itf		= itf; NBASSERT(itf != NULL)
	itm.line	= new(this) AUEscenaFigura(ENEscenaFiguraTipo_Linea);
	itm.line->agregarCoordenadaLocal(0.0f, 0.0f);
	itm.line->agregarCoordenadaLocal(0.0f, 0.0f);
	itm.line->establecerMultiplicadorColor8(color.r, color.g, color.b, 55);
	itm.synced	= TRUE;
	if(itm.obj != NULL){
		itm.obj->retener(NB_RETENEDOR_THIS);
		itm.lyr->agregarObjetoEscena(itm.obj);
	}
	{
		NBArray_init(&itm.options.arr, sizeof(STVisualRowsGrpItmOpt), NULL);
	}
	_grp.layerBtns->agregarObjetoEscena(itm.lyr);
	_grp.layerLines->agregarObjetoEscena(itm.line);
	if(idx >= 0 && idx <= _itms.use){
		NBArray_addItemsAtIndex(&_itms, idx, &itm, sizeof(itm), 1);
		r = idx;
	} else {
		NBArray_addValue(&_itms, itm);
		r = (_itms.use - 1);
	}
	return r;
}

SI32 TSVisualRowsGrp::rowsIndexByObj(AUEscenaObjeto* rowObj){
	SI32 r = -1;
	if(rowObj != NULL){
		SI32 i; for(i = 0; i < _itms.use; i++){
			STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, i);
			if(itm->obj == rowObj){
				r = i;
				break;
			}
		}
	}
	return r;
}

BOOL TSVisualRowsGrp::rowsRemoveByObj(AUEscenaObjeto* rowObj){
	BOOL r = FALSE;
	if(rowObj != NULL){
		SI32 i; for(i = 0; i < _itms.use; i++){
			STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, i);
			if(itm->obj == rowObj){
				TSVisualRowsGrp_releaseItm_(itm);
				NBArray_removeItemAtIndex(&_itms, i);
				r = TRUE;
				break;
			}
		}
	}
	return r;
}

void TSVisualRowsGrp::rowAddOption(const SI32 rowIdx, const char* optionId, AUIButton* optBtn){
	if(rowIdx >= 0 && rowIdx < _itms.use && optBtn != NULL){
		STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, rowIdx);
		BOOL fnd = FALSE;
		//Search current
		{
			SI32 i; for(i = 0; i < itm->options.arr.use; i++){
				STVisualRowsGrpItmOpt* opt = NBArray_itmPtrAtIndex(&itm->options.arr, STVisualRowsGrpItmOpt, i);
				if(opt->btn == optBtn){
					if(opt->btn->contenedor() != itm->lyr){
						itm->lyr->agregarObjetoEscenaEnIndice(opt->btn, 0);
					}
					fnd = TRUE;
					break;
				}
			}
		}
		//Add
		if(!fnd){
			STVisualRowsGrpItmOpt opt;
			NBMemory_setZeroSt(opt, STVisualRowsGrpItmOpt);
			NBString_initWithStr(&opt.optionId, optionId);
			opt.btn = optBtn;
			if(opt.btn != NULL){
				opt.btn->retener(NB_RETENEDOR_THIS);
				itm->lyr->agregarObjetoEscenaEnIndice(opt.btn, 0);
			}
			NBArray_addValue(&itm->options.arr, opt);
		}
	}
}

BOOL TSVisualRowsGrp::rowDragOptionStartedByObj(AUEscenaObjeto* rowObj){
	BOOL r = FALSE;
	if(rowObj != NULL){
		SI32 i; for(i = 0; i < _itms.use; i++){
			STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, i);
			if(itm->obj == rowObj){
				NBASSERT(itm->options.dragDepth >= 0)
				itm->options.dragDepth++;
				break;
			}
		}
	}
	return r;
}

BOOL TSVisualRowsGrp::rowDragOptionApplyByObj(AUEscenaObjeto* rowObj, const float deltaX, const BOOL onlySimulate){
	BOOL r = FALSE;
	if(rowObj != NULL){
		SI32 i; for(i = 0; i < _itms.use; i++){
			STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, i);
			if(itm->obj == rowObj){
				NBASSERT(onlySimulate || itm->options.dragDepth > 0)
				if(itm->options.width > 0.0f){
					if(deltaX < 0.0f){
						//Show options
						if(itm->options.relVisible < 1.0f){
							if(!onlySimulate){
								_isAnimating = TRUE;
								itm->options.relVisible += (-deltaX / itm->options.width);
								if(itm->options.relVisible > 1.0f) itm->options.relVisible = 1.0f;
								this->privOrganizeOptions(itm);
							}
							r = TRUE;
						}
					} else {
						//Hide options
						if(itm->options.relVisible > 0.0f){
							if(!onlySimulate){
								_isAnimating = TRUE;
								itm->options.relVisible -= (deltaX / itm->options.width);
								if(itm->options.relVisible < 0.0f) itm->options.relVisible = 0.0f;
								this->privOrganizeOptions(itm);
							}
							r = TRUE;
						}
					}
				}
			} else if(!onlySimulate && !_multipleOptionsRows){
				if(itm->options.relVisible >= 1.0f){
					_isAnimating = TRUE;
					itm->options.relVisible = 0.99f;
					this->privOrganizeOptions(itm);
				}
			}
		}
	}
	return r;
}

BOOL TSVisualRowsGrp::rowDragOptionEndedByObj(AUEscenaObjeto* rowObj){
	BOOL r = FALSE;
	if(rowObj != NULL){
		SI32 i; for(i = 0; i < _itms.use; i++){
			STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, i);
			if(itm->obj == rowObj){
				NBASSERT(itm->options.dragDepth > 0)
				itm->options.dragDepth--;
				break;
			}
		}
	}
	return r;
}

void TSVisualRowsGrp::privOrganizeOptions(STVisualRowsGrpItm* itm){
	const NBCajaAABB boxBase = itm->lyr->cajaAABBLocal();
	itm->options.width = 0.0f;
	if(itm->options.arr.use > 0){
		float xRight = boxBase.xMax;
		SI32 i; for(i = (itm->options.arr.use - 1); i >= 0; i--){
			STVisualRowsGrpItmOpt* opt = NBArray_itmPtrAtIndex(&itm->options.arr, STVisualRowsGrpItmOpt, i);
			if(opt->btn != NULL){
				const NBCajaAABB boxOpt = opt->btn->cajaAABBLocal();
				opt->btn->establecerTraslacion(xRight - boxOpt.xMax + ((boxBase.xMax - xRight) * (1.0f - itm->options.relVisible)), boxBase.yMax - boxOpt.yMax);
				opt->btn->setVisibleAndAwake(itm->options.relVisible > 0.0f);
				xRight -= (boxOpt.xMax - boxOpt.xMin);
			}
		}
		itm->options.width += (boxBase.xMax - xRight);
	}
	//Translate row
	if(itm->obj != NULL){
		itm->obj->establecerTraslacion(0.0f - (itm->options.width * itm->options.relVisible), itm->obj->traslacion().y);
	}
}

SI32 TSVisualRowsGrp::rowsVisibleCount() const {
	return _itmsVisibleCount;
}

void TSVisualRowsGrp::applyAreaFilter(const STNBAABox outter, const STNBAABox inner){
	STVisualRowsGrp* grp	= &_grp;
	//Grp-header
	{
		if(_title.isVisible){
			grp->title->setVisibleAwakeAndAlphaByParentArea(outter, inner);
			grp->bg->setVisibleAwakeAndAlphaByParentArea(outter, inner);
		} else {
			_grp.bg->setVisibleAndAwake(_title.isVisible);
			_grp.title->setVisibleAndAwake(_title.isVisible);
		}
	}
	//Rows
	if(_itms.use > 0){
		SI32 i; for(i = 0; i < _itms.use; i++){
			STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, i);
			itm->lyr->setVisibleAwakeAndAlphaByParentArea(outter, inner);
			if((i + 1) == _itms.use){
				itm->line->setVisibleAndAwake(FALSE);
			} else {
				itm->line->setVisibleAwakeAndAlphaByParentArea(outter, inner);
			}
		}
	}
	//Empty
	{
		_empty.lyr->setVisibleAwakeAndAlphaByParentArea(outter, inner);
	}
}

BOOL TSVisualRowsGrp::applyScaledSize(const STNBSize sizeBase, const STNBSize sizeScaled, const bool notifyChange, AUAnimadorObjetoEscena* animator, const float secsAnim){
	return FALSE;
}

STNBSize TSVisualRowsGrp::getScaledSize(){
	return NBST_P(STNBSize, _anchoUsar, _altoUsar );
}

ENDRScrollSideH TSVisualRowsGrp::getPreferedScrollH(){
	return ENDRScrollSideH_Left;
}

ENDRScrollSideV TSVisualRowsGrp::getPreferedScrollV(){
	return ENDRScrollSideV_Top;
}

void* TSVisualRowsGrp::anchorCreate(const STNBPoint lclPos){
	return NULL;
}

void* TSVisualRowsGrp::anchorToFocusClone(float* dstSecsDur){
	return NULL;
}

void* TSVisualRowsGrp::anchorClone(const void* anchorRef){
	return NULL;
}

void TSVisualRowsGrp::anchorToFocusClear(){
	//
}

STNBAABox TSVisualRowsGrp::anchorGetBox(void* anchorRef, float* dstScalePref){
	STNBAABox r; NBMemory_setZeroSt(r, STNBAABox);
	NBASSERT(anchorRef == NULL)
	return r;
}

void TSVisualRowsGrp::anchorDestroy(void* anchorRef){
	NBASSERT(anchorRef == NULL)
}

void TSVisualRowsGrp::scrollToTop(const float secs){
	//
}

//

AUEscenaObjeto* TSVisualRowsGrp::itemObjetoEscena(){
	return this;
}

STListaItemDatos TSVisualRowsGrp::refDatos(){
	return _refDatos;
}

void TSVisualRowsGrp::establecerRefDatos(const SI32 tipo, const SI32 valor){
	_refDatos.tipo	= tipo;
	_refDatos.valor	= valor;
}

void TSVisualRowsGrp::organizarContenido(const float ancho, const float altoVisible, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim){
	this->privOrganizarContenido(ancho, altoVisible, notificarCambioAltura, animator, secsAnim);
}

void TSVisualRowsGrp::privOrganizarContenido(const float ancho, const float alto, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim){
	if(ancho > 0.0f){
		const float grpMarginI = NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.05);
		float xLeft = 0.0f, xRight = ancho, yTop = 0.0f;
		//Grp
		{
			STVisualRowsGrp* grp	= &_grp;
			float xLeftTitle		= grpMarginI;
			_itmsVisibleCount		= 0;
			//Organize grp-header
			if(!_title.isVisible){
				_grp.bg->setVisibleAndAwake(_title.isVisible);
				_grp.title->setVisibleAndAwake(_title.isVisible);
			} else {
				if(!_title.isPosFixed){
					grp->title->establecerAlineacionH(ENNBTextLineAlignH_Left);
					grp->title->organizarTexto(xRight - xLeftTitle - grpMarginI);
					const NBCajaAABB box = grp->title->cajaAABBLocal();
					grp->title->establecerTraslacion(xLeftTitle - box.xMin, yTop - grpMarginI - box.yMax);
				} else {
					xLeftTitle = _title.pos;
					grp->title->establecerAlineacionH(_title.align);
					grp->title->organizarTexto(ancho);
					const NBCajaAABB box = grp->title->cajaAABBLocal();
					grp->title->establecerTraslacion(xLeftTitle, yTop - grpMarginI - box.yMax);
				}
				//Bg
				{
					const NBCajaAABB box = grp->title->cajaAABBLocal();
					const float heightBg = grpMarginI + grpMarginI + (box.yMax - box.yMin);
					grp->bg->redimensionar(xLeft, yTop, (xRight - xLeft), -heightBg);
					yTop -= heightBg;
				}
			}
			//Rows
			{
				SI32 i; for(i = 0; i < _itms.use; i++){
					STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, i);
					//Organize obj
					{
						itm->itf->organizarContenido(xRight - xLeft, 0.0f, false, animator, secsAnim);
						{
							const NBCajaAABB box = itm->obj->cajaAABBLocal();
							itm->obj->establecerTraslacion(0.0f, 0.0f);
							itm->lyr->establecerLimites(box);
							//Organize options
							itm->options.width = 0.0f;
							if(itm->options.arr.use > 0){
								float xRight = box.xMax;
								SI32 i; for(i = (itm->options.arr.use - 1); i >= 0; i--){
									STVisualRowsGrpItmOpt* opt = NBArray_itmPtrAtIndex(&itm->options.arr, STVisualRowsGrpItmOpt, i);
									if(opt->btn != NULL){
										const NBMargenes margins = opt->btn->margenes();
										const NBCajaAABB boxMin = opt->btn->limitesParaContenido();
										const float heightMarg	= (box.yMax - box.yMin) - (boxMin.yMax - boxMin.yMin) + margins.top + margins.bottom;
										opt->btn->establecerMargenes(margins.left, margins.right, (heightMarg / 2.0f), (heightMarg / 2.0f), opt->btn->margenI());
										opt->btn->organizarContenido((boxMin.xMax - boxMin.xMin), (box.yMax - box.yMin), FALSE, NULL, 0.0f);
										opt->btn->setVisibleAndAwake(itm->obj->visible());
									}
								}
								this->privOrganizeOptions(itm);
							}
							//Move
							if(itm->obj->visible()){
								const STNBPoint pos = NBST_P(STNBPoint, 
									xLeft - box.xMin
									, yTop - box.yMax
								);
								const NBPunto curPos = itm->lyr->traslacion();
								if(curPos.x != pos.x || curPos.y != pos.y){
									if(animator != NULL && secsAnim > 0.0f){
										animator->animarPosicion(itm->lyr, pos.x, pos.y, secsAnim, ENAnimPropVelocidad_Desacelerada);
									} else {
										itm->lyr->establecerTraslacion(pos.x, pos.y);
									}
								}
								yTop -= (box.yMax - box.yMin);
								_itmsVisibleCount++;
							}
						}
					}
					//Organize line
					if(itm->line != NULL){
						itm->line->moverVerticeHacia(0, xLeftTitle, 0.0f);
						itm->line->moverVerticeHacia(1, ancho, 0.0f);
						itm->line->setVisibleAndAwake(itm->obj->visible() && (i + 1) < _itms.use); //hide last line
						{
							const STNBPoint pos = NBST_P(STNBPoint, 0.0f, yTop);
							const NBPunto curPos = itm->line->traslacion();
							if(curPos.x != pos.x || curPos.y != pos.y){
								if(animator != NULL && secsAnim > 0.0f){
									animator->animarPosicion(itm->line, pos.x, pos.y, secsAnim, ENAnimPropVelocidad_Desacelerada);
								} else {
									itm->line->establecerTraslacion(pos.x, pos.y);
								}
							}
						}
					}
				}
			}
			//Empty
			{
				const float yTopStart = yTop;
				//Position
				{
					const NBCajaAABB box = _empty.txt->cajaAABBLocal();
					const float height = (box.yMax - box.yMin);
					//Bg
					{
						_empty.bg->redimensionar(0.0f, 0.0f, xRight - xLeft, -height - grpMarginI - grpMarginI);
						{
							const STNBPoint pos = NBST_P(STNBPoint, xLeft, yTop );
							const NBPunto curPos = _empty.bg->traslacion();
							if(curPos.x != pos.x || curPos.y != pos.y){
								if(animator != NULL && secsAnim > 0.0f){
									animator->animarPosicion(_empty.bg, pos.x, pos.y, secsAnim, ENAnimPropVelocidad_Desacelerada);
								} else {
									_empty.bg->establecerTraslacion(pos.x, pos.y);
								}
							}
						}
					}
					//Txt
					{
						{
							const STNBPoint pos = NBST_P(STNBPoint, xLeft - box.xMin + (((xRight - xLeft) - (box.xMax - box.xMin)) * 0.5f), yTop - grpMarginI );
							const NBPunto curPos = _empty.txt->traslacion();
							if(curPos.x != pos.x || curPos.y != pos.y){
								if(animator != NULL && secsAnim > 0.0f){
									animator->animarPosicion(_empty.txt, pos.x, pos.y, secsAnim, ENAnimPropVelocidad_Desacelerada);
								} else {
									_empty.txt->establecerTraslacion(pos.x, pos.y);
								}
							}
						}
						yTop -= (height + grpMarginI + grpMarginI);
					}
					//Line
					{
						_empty.line->moverVerticeHacia(0, xLeft, 0.0f);
						_empty.line->moverVerticeHacia(1, xRight, 0.0f);
						{
							const STNBPoint pos = NBST_P(STNBPoint, 0.0f, yTop );
							const NBPunto curPos = _empty.line->traslacion();
							if(curPos.x != pos.x || curPos.y != pos.y){
								if(animator != NULL && secsAnim > 0.0f){
									animator->animarPosicion(_empty.line, pos.x, pos.y, secsAnim, ENAnimPropVelocidad_Desacelerada);
								} else {
									_empty.line->establecerTraslacion(pos.x, pos.y);
								}
							}
						}
					}
				}
				//Set visibility
				if(_itmsVisibleCount > 0){
					_empty.bg->setVisibleAndAwake(FALSE);
					_empty.txt->setVisibleAndAwake(FALSE);
					_empty.line->setVisibleAndAwake(FALSE);
					yTop = yTopStart;
				} else {
					_empty.bg->setVisibleAndAwake(TRUE);
					_empty.txt->setVisibleAndAwake(TRUE);
					_empty.line->setVisibleAndAwake(FALSE /*TRUE*/);
				}
			}
		}
		//Set limits
		{
			this->establecerLimites(0.0f, ancho, yTop, 0.0f);
		}
		_isAnimating	= TRUE;
		_anchoUsar		= ancho;
		_altoUsar		= alto;
	}
}

void TSVisualRowsGrp::tickAnimacion(float segsTranscurridos){
	if(_isAnimating){
		_isAnimating = FALSE;
		//Rows
		{
			SI32 i; for(i = 0; i < _itms.use; i++){
				STVisualRowsGrpItm* itm = NBArray_itmPtrAtIndex(&_itms, STVisualRowsGrpItm, i);
				if(itm->options.dragDepth > 0 || itm->options.relVisible >= 1.0f){
					_isAnimating = TRUE; //keep active
				} else if(itm->options.relVisible > 0.0f){
					float deltaRel = (itm->options.relVisible * 0.10f);
					if(deltaRel < 0.03f) deltaRel = 0.03f;
					itm->options.relVisible -= deltaRel;
					if(itm->options.relVisible < 0.0f) itm->options.relVisible = 0.0f;
					this->privOrganizeOptions(itm);
					if(itm->options.relVisible > 0.0f){
						_isAnimating = TRUE; //keep active
					}
				}
			}
		}
	}
}

//

AUOBJMETODOS_CLASESID_MULTICLASE(TSVisualRowsGrp, AUEscenaContenedorLimitado)
AUOBJMETODOS_CLASESNOMBRES_MULTICLASE(TSVisualRowsGrp, "TSVisualRowsGrp", AUEscenaContenedorLimitado)
AUOBJMETODOS_CLONAR_NULL(TSVisualRowsGrp)
