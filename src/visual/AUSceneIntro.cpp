//
//  AUSceneIntro.cpp
//  thinstream-test
//
//  Created by Marcos Ortega on 8/5/18.
//

#include "visual/TSVisualPch.h"
#include "visual/AUSceneIntro.h"
//
#include "nb/core/NBStruct.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/2d/NBJpeg.h"
#include "nb/crypto/NBBase64.h"
#include "NBMngrOSTools.h"
#include "visual/TSFonts.h"
#include "core/logic/TSClientRoot.h"
#include "core/logic/TSDevice.h"
#include "core/logic/TSDeviceLocal.h"
#include "visual/gui/TSVisualTag.h"
#include "visual/gui/TSVisualIcons.h"
#include "visual/AUSceneCol.h"

#define TS_POPUPS_REQ_WAIT_SECS_BEFORE_REQ_LOCAL	1
#define TS_POPUPS_REQ_WAIT_SECS_BEFORE_SHOW_LOCAL	0

//Content

typedef struct STSceneInfoContent_NotifsAuth_ {
	AUEscenaContenedorLimitado*	formLayer;
	AUEscenaSprite*				icon;
	AUEscenaTexto*				explain;
	float						explainWidth;
	//Status
	BOOL						requestAuth; //btn Pressed
	ENAppNotifAuthState			lastStatus;
} STSceneInfoContent_NotifsAuth;

typedef struct STSceneInfoContent_TermsAccept_ {
	AUEscenaContenedor*		formLayer;
	//AUEscenaTexto*		txtTitle;
	AUEscenaTexto*			txtExplainTop;
	//Terms link
	struct {
		AUEscenaTexto*		txt;
		AUEscenaFigura*		line;
	} linkTerms;
	//Privacy link
	struct {
		AUEscenaTexto*		txt;
		AUEscenaFigura*		line;
	} linkPrivacy;
} STSceneInfoContent_TermsAccept;

//Methods

void SceneInfoContentBaseDef_init(STSceneInfoContentBaseDef* obj);
void SceneInfoContentBaseDef_release(STSceneInfoContentBaseDef* obj);

//Stack
void SceneInfoStack_init(STSceneInfoStack* obj);
void SceneInfoStack_release(STSceneInfoStack* obj, AUSceneIntro* intro);
//StackItm
void SceneInfoStackItm_init(STSceneInfoStack* obj);
void SceneInfoStackItm_release(STSceneInfoStack* obj, AUSceneIntro* intro);
//Content
void SceneInfoContent_init(STSceneInfoContent* obj);
void SceneInfoContent_release(STSceneInfoContent* obj, AUSceneIntro* intro);

//Stack Itm

/*
typedef struct STSceneInfoStackItm_ {
	STSceneInfoContent			content;
	struct STSceneInfoStack_*	subStack;
} STSceneInfoStackItm;
*/

void SceneInfoStackItm_init(STSceneInfoStackItm* obj){
	SceneInfoContent_init(&obj->content);
	obj->subStack = NULL;
}

void SceneInfoStackItm_release(STSceneInfoStackItm* obj, AUSceneIntro* intro){
	SceneInfoContent_release(&obj->content, intro);
	if(obj->subStack != NULL){
		SceneInfoStack_release(obj->subStack, intro);
		NBMemory_free(obj->subStack);
		obj->subStack = NULL;
	}
}

//Stack

/*
typedef struct STSceneInfoStack_ {
	UI32		iCurStep;		//Current step
	STNBArray	steps;			//ENSceneInfoContent, steps list
	STNBArray	contentStack;	//STSceneInfoStackItm*
} STSceneInfoStack;
*/

void SceneInfoStack_init(STSceneInfoStack* obj){
	obj->iCurStep	= -1;
	NBArray_init(&obj->stepsSequence, sizeof(ENSceneInfoContent), NULL);
	{
		STSceneInfoStepsInds* steps = &obj->stepsIndicators;
		{
			steps->layerVisible = TRUE;
			steps->layer = new AUEscenaContenedorLimitado();
		}
		{
			steps->layerLines = new AUEscenaContenedor();
			steps->layer->agregarObjetoEscena(steps->layerLines);
		}
		{
			NBArray_init(&steps->inds, sizeof(STSceneInfoStepInd), NULL);
		}
	}
	NBArray_init(&obj->contentStack, sizeof(STSceneInfoStackItm*), NULL);
}

void AUSceneIntro::privAddStepToSequence(STSceneInfoStack* obj, const ENSceneInfoContent stepId, STSceneInfoStack* stepsTemplates, AUFuenteRender* fntStepName){
	NBArray_addValue(&obj->stepsSequence, stepId);
	//Add indicator
	if(stepsTemplates != NULL && fntStepName != NULL){
		const STSceneInfoStackItm* itm = NULL;
		{
			//Search current
			{
				SI32 i; for(i = 0; i < stepsTemplates->contentStack.use; i++){
					const STSceneInfoStackItm* itm2 = NBArray_itmValueAtIndex(&stepsTemplates->contentStack, STSceneInfoStackItm*, i);
					if(itm2->content.uid == stepId){
						itm = itm2;
						break;
					}
				}
			}
			//Create template
			if(itm == NULL){
				this->privStackPushContent(stepsTemplates, stepId);
				itm = NBArray_itmValueAtIndex(&stepsTemplates->contentStack, STSceneInfoStackItm*, stepsTemplates->contentStack.use - 1);
				NBASSERT(itm->content.uid == stepId)
			}
		}
		const STSceneInfoContent* content	= &itm->content;
		const STSceneInfoContentBase* base	= &content->base;
		//Create step indicator
		if(base->showAsStep){
			STSceneInfoStepInd ind;
			NBMemory_setZeroSt(ind, STSceneInfoStepInd);
			ind.iStep = obj->stepsSequence.use - 1;
			{
				ind.lineToNext = new AUEscenaFigura(ENEscenaFiguraTipo_Linea);
				ind.lineToNext->establecerModo(ENEscenaFiguraModo_Borde);
				ind.lineToNext->agregarCoordenadaLocal(0, 0);
				ind.lineToNext->agregarCoordenadaLocal(1, 0);
				ind.lineToNext->establecerMultiplicadorColor8(200, 200, 200, 255);
				obj->stepsIndicators.layerLines->agregarObjetoEscena(ind.lineToNext);
			}
			{
				//Red: 216, 61, 47
				//Gray: 217, 217, 217
				ind.circle = new AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#step"));
				ind.circle->establecerMultiplicadorColor8(200, 200, 200, 255);
				obj->stepsIndicators.layer->agregarObjetoEscena(ind.circle);
			}
			if(base->shortName.length == 0){
				ind.text = NULL;
			} else {
				ind.text = new AUEscenaTexto(fntStepName);
				ind.text->establecerTexto(base->shortName.str);
				ind.text->establecerMultiplicadorColor8(200, 200, 200, 255);
				obj->stepsIndicators.layer->agregarObjetoEscena(ind.text);
			}
			NBArray_addValue(&obj->stepsIndicators.inds, ind);
		}
	}
}

void SceneInfoStack_release(STSceneInfoStack* obj, AUSceneIntro* intro){
	obj->iCurStep	= -1;
	NBArray_release(&obj->stepsSequence);
	{
		STSceneInfoStepsInds* steps = &obj->stepsIndicators;
		if(steps->layer != NULL) steps->layer->liberar(NB_RETENEDOR_NULL); steps->layer = NULL;
		if(steps->layerLines != NULL) steps->layerLines->liberar(NB_RETENEDOR_NULL); steps->layerLines = NULL;
		{
			SI32 i; for(i = 0 ; i < steps->inds.use; i++){
				STSceneInfoStepInd* ind = NBArray_itmPtrAtIndex(&steps->inds, STSceneInfoStepInd, i);
				if(ind->circle != NULL) ind->circle->liberar(NB_RETENEDOR_NULL); ind->circle = NULL;
				if(ind->lineToNext != NULL) ind->lineToNext->liberar(NB_RETENEDOR_NULL); ind->lineToNext = NULL;
				if(ind->text != NULL) ind->text->liberar(NB_RETENEDOR_NULL); ind->text = NULL;
			}
			NBArray_empty(&steps->inds);
			NBArray_release(&steps->inds);
		}
	}
	{
		SI32 i; for(i = 0; i < obj->contentStack.use; i++){
			STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&obj->contentStack, STSceneInfoStackItm*, i);
			SceneInfoStackItm_release(itm, intro);
			NBMemory_free(itm);
		}
		NBArray_release(&obj->contentStack);
	}
}

//Content

void SceneInfoContent_init(STSceneInfoContent* obj){
	obj->uid		= ENSceneInfoContent_Conteo;
	{
		NBMemory_setZero(obj->base);
		NBString_init(&obj->base.shortName);
		NBString_init(&obj->base.privacyTitle);
		NBString_init(&obj->base.privacyText);
		obj->base.buttonsPos = ENNBTextAlignV_Count;
		NBArray_init(&obj->base.buttons, sizeof(STSceneInfoBtn), NULL);
		NBArray_init(&obj->base.txtBoxes, sizeof(AUITextBox*), NULL);
		NBArray_init(&obj->base.touchables, sizeof(AUEscenaObjeto*), NULL);
	}
	obj->layer		= new AUEscenaContenedor();
	//Buttons
	{
		obj->btnsLayer	= new AUEscenaContenedor();
	}
	//formLayer
	{
		obj->formLayerLimit	= new AUEscenaContenedorLimitado();	//Add this after every 'extra'; the form must be at the top
		//Glass
		{
			obj->formLayerGlassLayer	= new AUEscenaContenedor();
			obj->formLayerGlassLayer->establecerVisible(false);
			obj->formLayerLimit->agregarObjetoEscena(obj->formLayerGlassLayer);
			//
			obj->formLayerGlass = new AUEscenaSprite();
			obj->formLayerGlass->establecerMultiplicadorColor8(0, 0, 0, 150);
			obj->formLayerGlassLayer->agregarObjetoEscena(obj->formLayerGlass);
		}
		//Form
		obj->formLayer	= new AUEscenaContenedor();
		obj->formLayerLimit->agregarObjetoEscena(obj->formLayer);
	}
	obj->data = NULL;
}

void SceneInfoContent_release(STSceneInfoContent* content, AUSceneIntro* intro){
	//Release data
	if(content->data != NULL){
		intro->privContentDestroy(content->uid, content->data);
		content->data = NULL;
	}
	//Release layer
	if(content->formLayerLimit != NULL) content->formLayerLimit->liberar(NB_RETENEDOR_NULL); content->formLayerLimit = NULL;
	if(content->formLayerGlassLayer != NULL) content->formLayerGlassLayer->liberar(NB_RETENEDOR_NULL); content->formLayerGlassLayer = NULL;
	if(content->formLayerGlass != NULL) content->formLayerGlass->liberar(NB_RETENEDOR_NULL); content->formLayerGlass = NULL;
	if(content->formLayer != NULL) content->formLayer->liberar(NB_RETENEDOR_NULL); content->formLayer = NULL;
	if(content->btnsLayer != NULL) content->btnsLayer->liberar(NB_RETENEDOR_NULL); content->btnsLayer = NULL;
	if(content->layer != NULL){
		AUEscenaContenedor* parent = (AUEscenaContenedor*)content->layer->contenedor();
		if(parent != NULL) parent->quitarObjetoEscena(content->layer);
		content->layer->liberar(NB_RETENEDOR_NULL);
		content->layer = NULL;
	}
	//Release base
	{
		STSceneInfoContentBase* base = &content->base;
		if(base->title != NULL) base->title->liberar(NB_RETENEDOR_NULL); base->title = NULL;
		if(base->explainTop != NULL) base->explainTop->liberar(NB_RETENEDOR_NULL); base->explainTop = NULL;
		if(base->privacyLnk != NULL) base->privacyLnk->liberar(NB_RETENEDOR_NULL); base->privacyLnk = NULL;
		if(base->privacyLnkIco != NULL) base->privacyLnkIco->liberar(NB_RETENEDOR_NULL); base->privacyLnkIco = NULL;
		if(base->privacyLnkLine != NULL) base->privacyLnkLine->liberar(NB_RETENEDOR_NULL); base->privacyLnkLine = NULL;
		{
			SI32 i; for(i = 0 ; i < base->buttons.use; i++){
				STSceneInfoBtn* btn = NBArray_itmPtrAtIndex(&base->buttons, STSceneInfoBtn, i);
				if(btn->btn != NULL) btn->btn->liberar(NB_RETENEDOR_NULL); btn->btn = NULL;
				NBString_release(&btn->optionId);
			}
			NBArray_empty(&base->buttons);
			NBArray_release(&base->buttons);
		}
		{
			SI32 i; for(i = 0 ; i < base->txtBoxes.use; i++){
				AUITextBox* obj = NBArray_itmValueAtIndex(&base->txtBoxes, AUITextBox*, i);
				if(obj != NULL) obj->liberar(NB_RETENEDOR_NULL); obj = NULL;
			}
			NBArray_empty(&base->txtBoxes);
			NBArray_release(&base->txtBoxes);
		}
		{
			SI32 i; for(i = 0 ; i < base->touchables.use; i++){
				AUEscenaObjeto* obj = NBArray_itmValueAtIndex(&base->touchables, AUEscenaObjeto*, i);
				if(obj != NULL) obj->liberar(NB_RETENEDOR_NULL); obj = NULL;
			}
			NBArray_empty(&base->touchables);
			NBArray_release(&base->touchables);
		}
		NBString_release(&base->privacyTitle);
		NBString_release(&base->privacyText);
		NBString_release(&base->shortName);
	}
}

//

AUSceneIntro::AUSceneIntro(STTSClient* coreClt, SI32 iScene, IScenesListener* escuchador, const ENSceneInfoMode mode) : AUAppEscena() {
	NB_DEFINE_NOMBRE_PUNTERO(this, "AUSceneIntro")
	//
	ENStatusBarStyle barStyle = ENStatusBarStyle_Dark;
	//
	_iScene			= iScene;
	_listener		= escuchador;
	_coreClt		= coreClt;
	_animator		= new(this) AUAnimadorObjetoEscena();
	_layerContent	= new(this) AUEscenaContenedor();
	//
	_metrics		= this->privFormMetrics();
	//
	NBString_init(&_tmpFormatString);
	//
	this->privCalculateLimits();
	{
		const float kbHeightPx	= NBGestorTeclas::tecladoEnPantallaAlto();
		_keybHeight				= NBGestorEscena::altoPuertoAGrupo(_iScene, ENGestorEscenaGrupo_GUI, kbHeightPx);
	}
	//Shared-data
	{
#		ifdef NB_CONFIG_INCLUDE_ASSERTS
		//NBString_set(&_data.name, "Marcos JOSUE ortega mORALES"); //Initial name; just for debug testing
		//NBString_set(&_data.password, "12345678"); //Initial password; just for debug testing
#		endif
	}
	_widthToUse		= (_lastBoxUsable.xMax - _lastBoxUsable.xMin);
	_heightToUse	= (_lastBoxUsable.yMax - _lastBoxUsable.yMin);
	//TexIcons
	{
		_texIcoErr = NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#btnCancel"); _texIcoErr->retener(NB_RETENEDOR_THIS);
	}
	//Bg
	{
		_bgColor	= TSColors::colorDef(ENTSColor_Light)->normal;
		_bg			= new(this) AUEscenaSprite();
		_bg->establecerMultiplicadorColor8(_bgColor.r, _bgColor.g, _bgColor.b, _bgColor.a);
		_layerContent->agregarObjetoEscena(_bg);
	}
	//Steps layer (under the "Back" button)
	{
		_contents.lyr	= new(this) AUEscenaContenedor();
		_layerContent->agregarObjetoEscena(_contents.lyr);
	}
	//Reset data btn
	{
		NBMemory_setZero(_resetData);
		{
			const char* strLoc = TSClient_getStr(_coreClt, "resetDataBtn", "Reset local data");
			AUFuenteRender* btnFont		= TSFonts::font(ENTSFont_Btn);
			const float btnMarginIntH	= NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.07f); //Internal margin (top & bottom)
			AUTextura* tex				= NBGestorTexturas::texturaDesdeArchivo("thinstream/mesh.png#btn");
			const STSceneInfoColorDef* bgColor = TSColors::colorDef(ENTSColor_White);
			const STSceneInfoColorDef* fgColor = TSColors::colorDef(ENTSColor_Gray);
			NBColor8 bgs[3], fgs[3];
			NBCOLOR_ESTABLECER(bgs[0], bgColor->normal.r, bgColor->normal.g, bgColor->normal.b, 0 /*bgColor->normal.a*/)
			NBCOLOR_ESTABLECER(bgs[1], bgColor->hover.r, bgColor->hover.g, bgColor->hover.b, 0 /*bgColor->hover.a*/)
			NBCOLOR_ESTABLECER(bgs[2], bgColor->touched.r, bgColor->touched.g, bgColor->touched.b, 0 /*bgColor->touched.a*/)
			NBCOLOR_ESTABLECER(fgs[0], fgColor->normal.r, fgColor->normal.g, fgColor->normal.b, fgColor->normal.a)
			NBCOLOR_ESTABLECER(fgs[1], fgColor->hover.r, fgColor->hover.g, fgColor->hover.b, fgColor->hover.a)
			NBCOLOR_ESTABLECER(fgs[2], fgColor->touched.r, fgColor->touched.g, fgColor->touched.b, fgColor->touched.a)
			AUIButton* obj = new(this) AUIButton();
			obj->agregarTextoMultilinea(btnFont, strLoc, ENIBotonItemAlign_Center, 0.5f, 0, 0, fgs[0], fgs[1], fgs[2]);
			obj->establecerFondo(true, tex);
			obj->establecerFondoColores(bgs[0], bgs[1], bgs[2]);
			obj->establecerMargenes(0.0f, 0.0f, btnMarginIntH, btnMarginIntH, 0.0f);
			obj->establecerSegsRetrasarOnTouch(0.0f);
			obj->establecerEscuchadorBoton(this);
			obj->organizarContenido(false);
			_layerContent->agregarObjetoEscena(obj);
			_resetData.btn = obj;
			_resetData.btn->setVisibleAndAwake(FALSE);
		}
	}
	//Back
	{
		{
			_back.layerColor = NBST_P(STNBColor8,  0, 0, 0, 255 );
			_back.layer = new(this) AUEscenaContenedor();
			_back.layer->establecerMultiplicadorColor8(_back.layerColor.r, _back.layerColor.g, _back.layerColor.b, _back.layerColor.a);
			_back.layer->establecerEscuchadorTouches(this, this);
			_layerContent->agregarObjetoEscena(_back.layer);
		}
		{
			_back.sprite = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#back"));
			_back.layer->agregarObjetoEscena(_back.sprite);
		}
		{
			const NBPunto spritePos		= _back.sprite->traslacion();
			const NBCajaAABB spriteBox	= _back.sprite->cajaAABBLocal();
			AUFuenteRender* font		= TSFonts::font(ENTSFont_Btn);
			{
				const char* strLoc = TSClient_getStr(_coreClt, "back", "Back");
				_back.text = new(this) AUEscenaTexto(font);
				_back.text->establecerTexto(strLoc);
				_back.text->establecerMultiplicadorAlpha8(0);
			}
			{
				const float spriteW		= (spriteBox.xMax - spriteBox.xMin);
				const NBCajaAABB txtBox	= _back.text->cajaAABBLocal();
				const STNBPoint txtPos	= { spritePos.x + spriteBox.xMax - txtBox.xMin + (spriteW * 0.75f), spritePos.y + spriteBox.yMax - (((spriteBox.yMax - spriteBox.yMin) - (txtBox.yMax - txtBox.yMin)) * 0.5f) };
				_back.text->establecerTraslacion(txtPos.x, txtPos.y);
				{
					//Dummy sprite to expand touchable area
					_back.dummy = new(this) AUEscenaSprite();
					_back.dummy->redimensionar(1.0f, 1.0f);
					_back.dummy->establecerMultiplicadorAlpha8(0);
					_back.dummy->establecerTraslacion(txtPos.x + txtBox.xMax + (txtBox.xMax - txtBox.xMin), txtPos.y + txtBox.yMin);
					_back.layer->agregarObjetoEscena(_back.dummy);
				}
			}
			_back.layer->establecerVisible(false);
			_back.layer->agregarObjetoEscena(_back.text);
		}
	}
	//TopRightLogo
	{
		_topLogo.pathScale = 0.0f;
		_topLogo.sprite = new AUEscenaSprite();
		_topLogo.sprite->redimensionar(1.0f, 1.0f);
		_topLogo.sprite->establecerVisible(false);
		_layerContent->agregarObjetoEscena(_topLogo.sprite);
		//Will be configured at "orgnize" call.
	}
	//Contents
	{
		_barStyle = ENStatusBarStyle_Dark;
		//Templates stack
		{
			SceneInfoStack_init(&_contents.templatesStack);
			if(mode == ENSceneInfoMode_Register){
				//Preload all steps
				SI32 i; for(i = 0; i < ENSceneInfoContent_Conteo; i++){
					this->privStackPushContent(&_contents.templatesStack, (ENSceneInfoContent)i);
				}
			}
		}
		//Main stack
		{
			AUFuenteRender* fntStepName	= TSFonts::font(ENTSFont_Btn);
			_contents.mainStack = NBMemory_allocType(STSceneInfoStack);
			SceneInfoStack_init(_contents.mainStack);
			NBArray_init(&_contents.anims, sizeof(STSceneInfoContentAnim), NULL);
			NBArray_init(&_contents.stacks, sizeof(STSceneInfoStack*), NULL);
			NBArray_addValue(&_contents.stacks, _contents.mainStack);
			//Define steps
			{
				switch (mode) {
					default:
						//ENSceneInfoMode_Register
						{
							barStyle = ENStatusBarStyle_Dark;
							//
							this->privAddStepToSequence(_contents.mainStack, ENSceneInfoContent_TermsAccept, &_contents.templatesStack, fntStepName);							
						}
						break;
				}
			}
			//Add indicators to scene
			_layerContent->agregarObjetoEscena(_contents.mainStack->stepsIndicators.layer);
		}
	}
	if(barStyle >= 0 && barStyle < ENStatusBarStyle_Count){
		_barStyle = barStyle;
	}
	//Building request msgbox
	{
		NBThreadMutex_init(&_reqInvitation.mutex);
		_reqInvitation.verif.requestId			= 0;
		//confirm
		_reqInvitation.confirm.isBuilding		= FALSE;
		_reqInvitation.confirm.isBuildingLast	= _reqInvitation.confirm.isBuilding;
		_reqInvitation.confirm.popBuildingRef	= NULL;
		_reqInvitation.confirm.popConfirmRef	= NULL;
		_reqInvitation.confirm.pkey				= NULL; //Private key
		_reqInvitation.confirm.csr				= NULL; //Csr
		_reqInvitation.confirm.requestId		= 0;
	}
	//Popup
	{
		_popupRef	= NULL;
	}
	//
	NBGestorEscena::establecerColorFondo(_iScene, 0.0f, 0.0f, 0.0f, 1.0f);
	NBGestorEscena::normalizaCajaProyeccionEscena(_iScene);
	NBGestorEscena::normalizaMatricesCapasEscena(_iScene);
	_atFocus	= false;
	this->escenaColocarEnPrimerPlano();
	//
	this->privOrganizeContent();
	NBGestorAnimadores::agregarAnimador(this, this);
	//Visual keyboard
	NBGestorTeclas::agregarEscuchadorVisual(this);
	//Show first
	this->privContentShowNext(FALSE);
	//Add to file change listener
	{
		STNBStorageCacheItmLstnr methds;
		NBMemory_setZeroSt(methds, STNBStorageCacheItmLstnr);
		methds.retain			= AUObjeto_retain_;
		methds.release			= AUObjeto_release_;
		methds.recordChanged	= AUSceneIntro::recordChanged;
		TSClient_addRecordListener(_coreClt, "client/_root/", "_current", this, &methds);
	}
}

AUSceneIntro::~AUSceneIntro(){
	NBGestorTeclas::quitarEscuchadorVisual(this);
	TSClient_removeRequestsByListenerParam(_coreClt, this);
	NBGestorAnimadores::quitarAnimador(this);
	this->escenaQuitarDePrimerPlano();
	//
	TSClient_removeRecordListener(_coreClt, "client/_root/", "_current", this);
	//
	NBString_release(&_tmpFormatString);
	//
	{
		//Wait until build finish
		NBThreadMutex_lock(&_reqInvitation.mutex);
		{
			while(_reqInvitation.confirm.isBuilding){
				NBThreadMutex_unlock(&_reqInvitation.mutex);
				NBThread_mSleep(25);
				NBThreadMutex_lock(&_reqInvitation.mutex);
			}
			if(_reqInvitation.confirm.popBuildingRef != NULL){
				_listener->popRelease(_reqInvitation.confirm.popBuildingRef);
				_reqInvitation.confirm.popBuildingRef = NULL;
			}
			if(_reqInvitation.confirm.popConfirmRef != NULL){
				_listener->popRelease(_reqInvitation.confirm.popConfirmRef);
				_reqInvitation.confirm.popConfirmRef = NULL;
			}
			if(_reqInvitation.confirm.pkey != NULL){
				NBPKey_release(_reqInvitation.confirm.pkey);
				NBMemory_free(_reqInvitation.confirm.pkey);
				_reqInvitation.confirm.pkey = NULL;
			}
			if(_reqInvitation.confirm.csr != NULL){
				NBX509Req_release(_reqInvitation.confirm.csr);
				NBMemory_free(_reqInvitation.confirm.csr);
				_reqInvitation.confirm.csr = NULL;
			}
		}
		NBThreadMutex_unlock(&_reqInvitation.mutex);
		NBThreadMutex_release(&_reqInvitation.mutex);
	}
	//Pop up
	{
		if(_popupRef != NULL){
			_listener->popHide(_popupRef);
			_listener->popRelease(_popupRef);
			_popupRef = NULL;
		}
	}
	//Shared-data
	{
		//
	}
	//TexIcons
	{
		if(_texIcoErr != NULL) _texIcoErr->liberar(NB_RETENEDOR_THIS); _texIcoErr = NULL;
	}
	//Bg
	{
		if(_bg != NULL) _bg->liberar(NB_RETENEDOR_THIS); _bg = NULL;
	}
	//Back
	{
		if(_back.layer != NULL) _back.layer->liberar(NB_RETENEDOR_THIS); _back.layer = NULL;
		if(_back.sprite != NULL) _back.sprite->liberar(NB_RETENEDOR_THIS); _back.sprite = NULL;
		if(_back.text != NULL) _back.text->liberar(NB_RETENEDOR_THIS); _back.text = NULL;
		if(_back.dummy != NULL) _back.dummy->liberar(NB_RETENEDOR_THIS); _back.dummy = NULL;
	}
	//ToRightLogo
	{
		if(_topLogo.sprite != NULL) _topLogo.sprite->liberar(NB_RETENEDOR_THIS); _topLogo.sprite = NULL;
	}
	//Contents
	{
		if(_contents.lyr != NULL) _contents.lyr->liberar(NB_RETENEDOR_THIS); _contents.lyr = NULL;
		//Anims
		{
			SI32 i; for(i = 0; i < _contents.anims.use; i++){
				STSceneInfoContentAnim* anim = NBArray_itmPtrAtIndex(&_contents.anims, STSceneInfoContentAnim, i);
				if(anim->animator != NULL) anim->animator->liberar(NB_RETENEDOR_THIS); anim->animator = NULL;
			}
			NBArray_empty(&_contents.anims);
			NBArray_release(&_contents.anims);
		}
		//MainStack
		{
			SceneInfoStack_release(_contents.mainStack, this);
			NBMemory_free(_contents.mainStack);
			_contents.mainStack = NULL;
			//
			NBArray_release(&_contents.stacks);
		}
		//Templates stack
		SceneInfoStack_release(&_contents.templatesStack, this);
	}
	//Reset data
	{
		if(_resetData.btn != NULL){ _resetData.btn->liberar(NB_RETENEDOR_THIS); _resetData.btn = NULL; }
		if(_resetData.popRef != NULL){
			_listener->popHide(_resetData.popRef);
			_listener->popRelease(_resetData.popRef);
			_resetData.popRef = NULL;
		}
	}
	//
	if(_animator != NULL) _animator->liberar(NB_RETENEDOR_THIS); _animator = NULL;
	if(_layerContent != NULL) _layerContent->liberar(NB_RETENEDOR_THIS); _layerContent = NULL;
	//
	_coreClt = NULL;
}

//

const char* AUSceneIntro::privGetTmpStringByReplacing(const char* str, const char* find, const char* replace){
	NBString_set(&_tmpFormatString, str);
	NBString_replace(&_tmpFormatString, find, replace);
	return _tmpFormatString.str;
}

const char* AUSceneIntro::privGetTmpStringByReplacing2(const char* str, const char* find, const char* replace, const char* find2, const char* replace2){
	NBString_set(&_tmpFormatString, str);
	NBString_replace(&_tmpFormatString, find, replace);
	NBString_replace(&_tmpFormatString, find2, replace2);
	return _tmpFormatString.str;
}

//

bool AUSceneIntro::escenaEnPrimerPlano(){
	return _atFocus;
}

void AUSceneIntro::escenaColocarEnPrimerPlano(){
	if(!_atFocus){
		NBColor8 coloBlanco; NBCOLOR_ESTABLECER(coloBlanco, 255, 255, 255, 255);
		NBGestorEscena::agregarObjetoCapa(_iScene, ENGestorEscenaGrupo_GUI, _layerContent, coloBlanco);
		NBGestorEscena::establecerColorFondo(_iScene, 0.0f, 0.0f, 0.0f, 1.0f);
		NBGestorEscena::agregarEscuchadorCambioPuertoVision(_iScene, this);
		NBGestorAnimadores::agregarAnimador(this, this);
		if(_animator != NULL) _animator->reanudarAnimacion();
		//Contents
		{
			//Anims (continue)
			{
				SI32 i; for(i = 0; i < _contents.anims.use; i++){
					STSceneInfoContentAnim* anim = NBArray_itmPtrAtIndex(&_contents.anims, STSceneInfoContentAnim, i);
					if(anim->animator != NULL){
						anim->animator->reanudarAnimacion();
					}
				}
			}
		}
		//Set status bar style
		{
			NBMngrOSTools::setBarStyle(_barStyle);
		}
		//Enforce portrait mode
		{
			TSClient_appPortraitEnforcerPush(_coreClt);
		}
		_atFocus = true;
	}
}

void AUSceneIntro::escenaQuitarDePrimerPlano(){
	if(_atFocus){
		NBGestorAnimadores::quitarAnimador(this);
		NBGestorEscena::quitarObjetoCapa(_iScene, _layerContent);
		NBGestorEscena::quitarEscuchadorCambioPuertoVision(_iScene, this);
		if(_animator != NULL) _animator->detenerAnimacion();
		//Contents
		{
			//Anims (pause)
			{
				SI32 i; for(i = 0; i < _contents.anims.use; i++){
					STSceneInfoContentAnim* anim = NBArray_itmPtrAtIndex(&_contents.anims, STSceneInfoContentAnim, i);
					if(anim->animator != NULL){
						anim->animator->detenerAnimacion();
					}
				}
			}
		}
		//Release enforced portrait mode
		{
			TSClient_appPortraitEnforcerPop(_coreClt);
		}
		_atFocus = false;
	}
}

void AUSceneIntro::escenaGetOrientations(UI32* dstMask, ENAppOrientationBit* dstPrefered, ENAppOrientationBit* dstToApplyOnce, BOOL* dstAllowAutoRotate){
	if(dstMask != NULL) *dstMask = ENAppOrientationBits_All;
	if(dstPrefered != NULL) *dstPrefered = ENAppOrientationBit_Portrait;
	if(dstAllowAutoRotate != NULL) *dstAllowAutoRotate = TRUE;
}

bool AUSceneIntro::escenaPermitidoGirarPantalla(){
	return true;
}

bool AUSceneIntro::escenaEstaAnimandoSalida(){
	if(_animator != NULL) if(_animator->conteoAnimacionesEjecutando() != 0) return true;
	if(_contents.anims.use > 0) return true;
	return false;
}

void AUSceneIntro::escenaAnimarEntrada(){
	//
}

void AUSceneIntro::escenaAnimarSalida(){
	//
}

//

/*void AUSceneIntro::appProcesarNotificacionLocal(const char* grp, const SI32 localId, const char* objTip, const SI32 objId){
 
 }*/

//

void AUSceneIntro::privCalculateLimits(){
	const NBCajaAABB box = NBGestorEscena::cajaProyeccionGrupo(_iScene, ENGestorEscenaGrupo_GUI);
	//full box
	{
		_lastBox.xMin = box.xMin;
		_lastBox.xMax = box.xMax;
		_lastBox.yMin = box.yMin;
		_lastBox.yMax = box.yMax;
	}
	//internal content box
	{
		_lastBoxUsable = _lastBox;
		//Remove top bar
		{
			const float padPxs	= NBMngrOSTools::getWindowTopPaddingPxs();
			const float padScn	= NBGestorEscena::altoPuertoAGrupo(_iScene, ENGestorEscenaGrupo_GUI, padPxs);
			_lastBoxUsable.yMax	-= padScn;
		}
		//Remove btm bar
		{
			const float padPxs	= NBMngrOSTools::getWindowBtmPaddingPxs();
			const float padScn	= NBGestorEscena::altoPuertoAGrupo(_iScene, ENGestorEscenaGrupo_GUI, padPxs);
			_lastBoxUsable.yMin	+= padScn;
		}
		//Remove bottom space if big screen
		/*{
			NBTamano scnSize;		NBCAJAAABB_TAMANO(scnSize, _lastBox)
			const NBTamano inchSz	= NBGestorEscena::tamanoEscenaAPulgadas(_iScene, scnSize);
			const float inchDiag	= sqrtf((inchSz.ancho * inchSz.ancho) + (inchSz.alto * inchSz.alto));
			if(inchDiag >= 4.5f){
				//iPhone XR: 6.1 inch
				//iPhone 7: 4.7 inch
				//iPhone SE: 4.0 inch
				_lastBoxUsable.yMin += NBGestorEscena::altoPulgadasAEscena(_iScene, 0.15f);
			}
		}*/
	}
}

void AUSceneIntro::puertoDeVisionModificado(const SI32 iEscena, const STNBViewPortSize before, const STNBViewPortSize after){
	//Update metrics
	_metrics = this->privFormMetrics();
	//Update limits
	this->privCalculateLimits();
	_animator->purgarAnimaciones();
	//Contents
	{
		//Anims (purge)
		{
			SI32 i; for(i = 0; i < _contents.anims.use; i++){
				STSceneInfoContentAnim* anim = NBArray_itmPtrAtIndex(&_contents.anims, STSceneInfoContentAnim, i);
				//PRINTF_INFO("PURGING animation #%d / %d (%s).\n", (i + 1), _contents.anims.use, (anim->isOutAnim ? "OUT" : "IN"));
				STSceneInfoContent* content = &anim->stackItm->content;
				//Animator
				if(anim->animator != NULL){
					anim->animator->purgarAnimaciones();
					anim->animator->liberar(NB_RETENEDOR_THIS);
					anim->animator = NULL;
				}
				//Layers
				{
					AUEscenaContenedor* lyrParent = (AUEscenaContenedor*)content->layer->contenedor();
					AUEscenaContenedor* btnsParent = (AUEscenaContenedor*)content->btnsLayer->contenedor();
					if(anim->isOutAnim){
						//Remove layers
						if(lyrParent != NULL) lyrParent->quitarObjetoEscena(content->layer);
						if(btnsParent != NULL) btnsParent->quitarObjetoEscena(content->btnsLayer);
					} else {
						//Add layers (in case were removed by previous out-animation)
						if(lyrParent != _contents.lyr) _contents.lyr->agregarObjetoEscena(content->layer);
						if(btnsParent != _contents.lyr) _contents.lyr->agregarObjetoEscena(content->btnsLayer);
					}
				}
				//Free memory //ToDo: implement
				/*SceneInfoStackItm_release(anim->stackItm);
				NBMemory_free(anim->stackItm);
				anim->stackItm = NULL;*/
			}
			NBArray_empty(&_contents.anims);
		}
		//Objs
		{
			SI32 s; for(s = (_contents.stacks.use - 1); s >= 0; s--){ //Todo: implement all
				STSceneInfoStack* stack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, s);
				SI32 i; for(i = (stack->contentStack.use - 1); i >= 0; i--){
					STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&stack->contentStack, STSceneInfoStackItm*, i);
					STSceneInfoContent* content = &itm->content;
					if(content->data != NULL){
						this->privContentUpdateAfterResize(content->uid, content->data);
					}
				}
			}
		}
	}
	//
	this->privOrganizeContent();
}

void AUSceneIntro::privOrganizeContent(){
	const NBCajaAABB cajaEscena = _lastBox;
	//
	const STSceneInfoContent* curContent = this->privCurStackCurContent();
	//Bg
	{
		NBTamano tamEscena; NBCAJAAABB_TAMANO(tamEscena, cajaEscena);
		NBPunto centroEscena; NBCAJAAABB_CENTRO(centroEscena, cajaEscena);
		//
		BOOL visible = TRUE;
		_bgColor = TSColors::colorDef(ENTSColor_Light)->normal;
		if(curContent != NULL){
			visible = (curContent->base.bgColor < ENTSColor_Count);
			_bgColor = TSColors::colorDef(curContent->base.bgColor)->normal;
		}
		_bg->establecerVisible(visible);
		_bg->establecerMultiplicadorColor8(_bgColor.r, _bgColor.g, _bgColor.b, _bgColor.a);
		_bg->redimensionar(cajaEscena.xMin, cajaEscena.yMax, tamEscena.ancho, -tamEscena.alto);
		//Update OS-bar style
		{
			const SI32 colorAvg = ((SI32)_bgColor.r + (SI32)_bgColor.g + (SI32)_bgColor.b) / 3;
			if(visible){
				if(colorAvg < 100){
					_barStyle = ENStatusBarStyle_Light;
				} else {
					_barStyle = ENStatusBarStyle_Dark;
				}
				if(_atFocus){
					NBMngrOSTools::setBarStyle(_barStyle);
				}
			}
		}
	}
	//
	const STNBSize sceneSz		= { (_lastBoxUsable.xMax - _lastBoxUsable.xMin), (_lastBoxUsable.yMax - _lastBoxUsable.yMin) };
	const STNBSize sceneSzInch	= { NBGestorEscena::anchoEscenaAPulgadas(_iScene, sceneSz.width), NBGestorEscena::altoEscenaAPulgadas(_iScene, sceneSz.height) };
	//const float sceneDiagInch	= sqrtf((sceneSzInch.width * sceneSzInch.width) + (sceneSzInch.height * sceneSzInch.height));
	float yTop					= _lastBoxUsable.yMax - _metrics.sepV;
	float yBtm					= _lastBoxUsable.yMin + _metrics.sepV;
	//
	/*{
		const float scaleHD = NBGestorEscena::escalaEscenaParaHD(_iScene);
		const NBTamano dpi	= NBGestorEscena::puntosPorPulgada(_iScene);
		PRINTF_INFO("Organizing content scene(%f, %f) inch(%f, %f = %f) scaleHD(%f) dpi(%f, %f).\n", sceneSz.width, sceneSz.height, sceneSzInch.width, sceneSzInch.height, sqrtf((sceneSzInch.width * sceneSzInch.width) + (sceneSzInch.height * sceneSzInch.height)), scaleHD, dpi.ancho, dpi.alto);
	}*/
	//
	_widthToUse = (_lastBoxUsable.xMax - _lastBoxUsable.xMin);
	_heightToUse = (_lastBoxUsable.yMax - _lastBoxUsable.yMin);
	const float anchoDosPulgadas = NBGestorEscena::anchoPulgadasAEscena(_iScene, 2.0f);
	if(_widthToUse > (anchoDosPulgadas * 2.0f)){
		_widthToUse = anchoDosPulgadas;
	}
	const float altoTresPulgadas = NBGestorEscena::altoPulgadasAEscena(_iScene, 3.0f);
	if(_heightToUse > altoTresPulgadas){
		_heightToUse = altoTresPulgadas;
	}
	//TopRightLogo and Back
	{
		//Resize TopRightLogo
		{
			const NBCajaAABB backSpriteBox	= _back.sprite->cajaAABBLocal();
			const float backSpriteH	= (backSpriteBox.yMax - backSpriteBox.yMin);
			const float relWidthForLogo = 0.75f;
			const STTSLaunchLogo logoVer = DRLogoMetrics::getLogoSzVersionForSize(NBST_P(STNBSize,  (sceneSz.width * relWidthForLogo), backSpriteH * 3.0f ), ENLogoType_Logotype);
			if(_topLogo.pathScale != logoVer.szVersion.pathScale){
				AUTextura* tex = NBGestorTexturas::texturaDesdeArchivo(logoVer.szVersion.path); NBASSERT(tex != NULL)
				PRINTF_INFO("Updating logo from scale(%f) to (%f).\n", _topLogo.pathScale, logoVer.szVersion.pathScale);
				_topLogo.sprite->establecerTextura(tex);
				_topLogo.sprite->redimensionar(tex);
				_topLogo.pathScale = logoVer.szVersion.pathScale;
			}
		}
		//Align to the bigest size visible
		const NBCajaAABB backBox	= _back.layer->cajaAABBLocal();
		const NBCajaAABB logoBox	= _topLogo.sprite->cajaAABBLocal();
		const float backH			= (backBox.yMax - backBox.yMin);
		const float logoH			= (logoBox.yMax - logoBox.yMin);
		const float maxHeight		= (backH > logoH ? backH : logoH);
		{
			BOOL visible = FALSE;
			_back.layerColor = NBST_P(STNBColor8,  0, 0, 0, 255 );
			if(curContent != NULL){
				visible = (curContent->base.backColor < ENTSColor_Count /*&& _contents.curObjIdx > 0*/);
				_back.layerColor = TSColors::colorDef(curContent->base.backColor)->normal;
			}
			_back.layerPos				= NBST_P(STNBPoint,  _lastBoxUsable.xMin - backBox.xMin + _metrics.sepH, yTop - backBox.yMax - ((maxHeight - (backBox.yMax - backBox.yMin)) * 0.5f) );
			_back.layer->establecerMultiplicadorColor8(_back.layerColor.r, _back.layerColor.g, _back.layerColor.b, _back.layerColor.a);
			_back.layer->establecerTraslacion(_back.layerPos.x, _back.layerPos.y);
			_back.layer->establecerVisible(visible);
		}
		{
			BOOL visible = FALSE;
			if(curContent != NULL){
				visible = curContent->base.showTopLogo;
			}
			_topLogo.spritePos	= NBST_P(STNBPoint,  _lastBoxUsable.xMax - logoBox.xMax - (((_lastBoxUsable.xMax - _lastBoxUsable.xMin) - (logoBox.xMax - logoBox.xMin)) * 0.5f), yTop - logoBox.yMax - ((maxHeight - (logoBox.yMax - logoBox.yMin)) * 0.5f) );
			_topLogo.sprite->establecerTraslacion(_topLogo.spritePos.x, _topLogo.spritePos.y);
			_topLogo.sprite->establecerVisible(visible);
		}
		yTop -= (maxHeight + _metrics.sepV);
	}
	//Steps indicators
	//yBtm += _metrics.sepV * 2; //Tmp: add extra margin until steps are reactivated
	const float yBtmBeforeSteps = yBtm;
	if(_contents.mainStack->stepsIndicators.inds.use > 0){
		STSceneInfoStepsInds* steps = &_contents.mainStack->stepsIndicators;
		//Calculate width
		const NBTamano circleSz	= NBArray_itmPtrAtIndex(&steps->inds, STSceneInfoStepInd, 0)->circle->textura()->tamanoHD();
		float maxNameWidth		= 0.0f, maxNameHeight = 0.0f;
		SI32 visibleNamesCount	= 0;
		{
			SI32 i; for(i = 0 ; i < steps->inds.use; i++){
				STSceneInfoStepInd* ind = NBArray_itmPtrAtIndex(&steps->inds, STSceneInfoStepInd, i);
				if(ind->text != NULL){
					const NBCajaAABB box = ind->text->cajaAABBLocal();
					const float width	= (box.xMax - box.xMin);
					const float height	= (box.yMax - box.yMin);
					if(maxNameWidth < width) maxNameWidth = width;
					if(maxNameHeight < height) maxNameHeight = height;
					if(width != 0 || height != 0){
						visibleNamesCount++;
					}
				}
			}
		}
		//Organize internal (steps)
		{
			float xPos = 0.0f, bottomY	= -circleSz.alto;
			AUEscenaFigura* lastLine	= NULL;
			const float naturalWidth	= (circleSz.ancho * 1.5f);
			const float withNameWidth	= (naturalWidth > (maxNameWidth + (circleSz.ancho * 0.5f)) ? naturalWidth : (maxNameWidth + (circleSz.ancho * 0.5f)));
			const BOOL showNames		= (visibleNamesCount > 0 && (withNameWidth * steps->inds.use) < (sceneSz.width * 0.90f));
			const float widthPerStep	= (showNames ? withNameWidth : naturalWidth);
			SI32 i; for(i = 0 ; i < steps->inds.use; i++){
				STSceneInfoStepInd* ind = NBArray_itmPtrAtIndex(&steps->inds, STSceneInfoStepInd, i);
				//Circle and line
				const NBCajaAABB circleBox	= ind->circle->cajaAABBLocal();
				const float circleW	= (circleBox.xMax - circleBox.xMin);
				const float circleH	= (circleBox.yMax - circleBox.yMin);
				ind->circle->establecerTraslacion(xPos - circleBox.xMin + ((widthPerStep - circleW) * 0.5f), 0 - circleBox.yMax);
				ind->lineToNext->establecerTraslacion(xPos + (widthPerStep * 0.5f), circleH * -0.5f);
				ind->lineToNext->moverVerticeHacia(0, 0.0f, 0.0f);
				ind->lineToNext->moverVerticeHacia(0, widthPerStep, 0.0f);
				//Text
				if(ind->text != NULL){
					const NBCajaAABB nameBox = ind->text->cajaAABBLocal();
					const float nameW	= (nameBox.xMax - nameBox.xMin);
					const float nameH	= (nameBox.yMax - nameBox.yMin);
					const float nameTop	= (circleH * -1.15);
					const float nameBtm	= nameTop - nameH;
					ind->text->establecerVisible(showNames);
					ind->text->establecerTraslacion(xPos - nameBox.xMin + ((widthPerStep - nameW) * 0.5f), nameTop - nameBox.yMax);
					if(bottomY > nameBtm){
						if(ind->text->visible()){
							bottomY = nameBtm;
						}
					}
				}
				//
				lastLine = ind->lineToNext;
				xPos += widthPerStep;
			}
			steps->layer->establecerLimites(0, (widthPerStep * steps->inds.use), bottomY, 0);
			if(lastLine != NULL){
				lastLine->establecerVisible(FALSE);
			}
		}
		//Organize in scene
		{
			BOOL visible = FALSE;
			if(curContent != NULL){
				visible = curContent->base.showSteps;
			}
			const NBCajaAABB box = steps->layer->cajaAABBLocal();
			steps->layerPos = NBST_P(STNBPoint,  _lastBoxUsable.xMin - box.xMin + (((_lastBoxUsable.xMax - _lastBoxUsable.xMin) - (box.xMax - box.xMin)) * 0.5f), yBtm - box.yMin );
			steps->layer->establecerTraslacion(steps->layerPos.x, steps->layerPos.y);
			//iPhoneSE: 4" (3.48 x 1.96)
			//iPhone6S: 4.7" (4.1 x 2.3)
			//iPhone6SPlus: 5.5" (4.79 x 2.7)
			//iPadMini: 7.9" (6.28 x 4.7)
			if(sceneSzInch.height < 4.0f){
				steps->layerVisible = FALSE;
			} else {
				steps->layerVisible = visible;
				yBtm += (box.yMax - box.yMin) + _metrics.sepV;
			}
			steps->layer->establecerVisible(steps->layerVisible);
		}
	}
	//Reset data btn
	{
		if(_resetData.btn != NULL){
			_resetData.btn->organizarContenido(_metrics.formWidth, 0.0f, FALSE, NULL, 0.0f);
			{
				const float yBtmBeforeBtn = yBtm;
				const NBCajaAABB box = _resetData.btn->cajaAABBLocal();
				yBtm += _metrics.sepV;
				_resetData.btn->establecerTraslacion(_lastBoxUsable.xMin - box.xMin + (((_lastBoxUsable.xMax - _lastBoxUsable.xMin) - (box.xMax - box.xMin)) * 0.5f), yBtm - box.yMin);
				yBtm += (box.yMax - box.yMin) + _metrics.sepV;
				//Reset btm position
				yBtm = yBtmBeforeBtn;
			}
		}
	}
	//Contents
	{
		_contents.yTopBeforeContent = yTop;
		_contents.yBtmBeforeContent = yBtm;
		_contents.yBtmBeforeSteps	= yBtmBeforeSteps;
		SI32 s; for(s = (_contents.stacks.use - 1); s >= 0; s--){ //Todo: implement all
			STSceneInfoStack* stack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, s);
			SI32 i; for(i = (stack->contentStack.use - 1); i >= 0; i--){
				STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&stack->contentStack, STSceneInfoStackItm*, i);
				this->privContentOrganizeItm(itm);
			}
		}
	}
	//
	/*{
		const float widthInch = NBGestorEscena::anchoPulgadasAEscena(_iScene, 1.0f);
		const float widthPoint96 = NBGestorEscena::anchoPuntosAEscena(_iScene, 96.0f);
		const float widthPoint72 = NBGestorEscena::anchoPuntosAEscena(_iScene, 72.0f);
		//
		float yBtm = _lastBox.yMin + 32.0f;
		_tmpInchRect100->redimensionar(0, 0, widthInch, widthInch);
		_tmpInchRect100->establecerTraslacion(0, yBtm);
		yBtm += widthInch + 32;
		//
		_tmpInchRect96->redimensionar(0, 0, widthPoint96, widthPoint96);
		_tmpInchRect96->establecerTraslacion(0, yBtm);
		yBtm += widthPoint96 + 32;
		//
		_tmpInchRect72->redimensionar(0, 0, widthPoint72, widthPoint72);
		_tmpInchRect72->establecerTraslacion(0, yBtm);
		yBtm += widthPoint72 + 32;
	}*/
}

//Content metrics

STSceneInfoMetrics AUSceneIntro::privFormMetrics() const {
	STSceneInfoMetrics r;
	NBMemory_setZeroSt(r, STSceneInfoMetrics);
	/*{
		{
			const NBCajaAABB scnBox	= _lastBox;
			const float widthInch	= NBGestorEscena::anchoEscenaAPulgadas(iScene, (scnBox.xMax - scnBox.xMin));
			const float heightInch	= NBGestorEscena::altoEscenaAPulgadas(iScene, (scnBox.yMax - scnBox.yMin));
			const float diagInch	= sqrtf((widthInch * widthInch) + (heightInch * heightInch));
			PRINTF_INFO("Device window inches w(%f) h(%f), diag(%f).\n", widthInch, heightInch, diagInch);
#			if defined(__ANDROID__) || defined(__ENVIRONMENT_IPHONE_OS_VERSION_MIN_REQUIRED__)
			if(heightInch < 5.0f){ //iPhone SE is 4.87 x 2.31
				_inchsForFormMax = 2.5f;
			} else if(heightInch < 7.0f){ //iPhone 6S is 6.23 x 3.07
				_inchsForFormMax = 2.7f;
			} else { //Big tablet
				_inchsForFormMax = 3.0f;
			}
#			else //Ausume OS with resizable windows (bigger fonts)
			_inchsForFormMax = 3.0f;
#			endif
		}
	}*/
	//iPhoneSE: 4" (3.48 x 1.96)
	//iPhone6S: 4.7" (4.1 x 2.3)
	//iPhone6SPlus: 5.5" (4.79 x 2.7)
	//iPadMini: 7.9" (6.28 x 4.7)
	const NBTamano szInch	= NBGestorEscena::tamanoPulgadasPantalla(_iScene);
	const float diagInch2	= (szInch.ancho * szInch.ancho) + (szInch.alto * szInch.alto);
	float maxFrmInch		= 3.0f;
	if(diagInch2 <= (4.3 * 4.3)){
		maxFrmInch	= 2.5f;
		r.marginH	= r.marginV = NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.10f);
		r.sepH		= r.sepV = NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.10f);
	} else if(diagInch2 <= (6.0 * 6.0)){
		maxFrmInch	= 2.7f;
		r.marginH	= r.marginV = NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.15f);
		r.sepH		= r.sepV = NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.15f);
	} else {
		maxFrmInch	= 3.0f;
		r.marginH	= r.marginV = NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.20f);
		r.sepH		= r.sepV = NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.20f);
	}
	//From and content width
	if(maxFrmInch > szInch.ancho){
		r.formWidth	= NBGestorEscena::anchoPulgadasAEscena(_iScene, szInch.ancho * 0.90f);
		r.boxWidth	= r.formWidth - NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.10f);
	} else {
		r.formWidth	= NBGestorEscena::anchoPulgadasAEscena(_iScene, maxFrmInch);
		r.boxWidth	= r.formWidth - NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.10f);
	}
	return r;
}

float AUSceneIntro::privGetScaledSize(const NBCajaAABB fitOnBox, const NBCajaAABB objBox, const float rotation){
	float r = 1.0f;
	NBCajaAABB objBoxRot;
	NBCAJAAABB_INICIALIZAR(objBoxRot);
	NBCAJAAABB_ENVOLVER_CAJA_ROTADA(objBoxRot, objBox, 0, 0, rotation);
	const STNBSize rotSz = { objBoxRot.xMax - objBoxRot.xMin, objBoxRot.yMax - objBoxRot.yMin };
	if(rotSz.width != 0 && rotSz.height != 0){
		const STNBSize fitSz = { fitOnBox.xMax - fitOnBox.xMin, fitOnBox.yMax - fitOnBox.yMin };
		const STNBSize scales = { fitSz.width / rotSz.width, fitSz.height / rotSz.height };
		r = (scales.width < scales.height ? scales.width : scales.height);
		//PRINTF_INFO("rotSz(%.2f, %.2f) fitSz(%.2f, %.2f) scales(%.2f, %.2f) scale(%.2f).\n", rotSz.width, rotSz.height, fitSz.width, fitSz.height, scales.width, scales.height, r);
	}
	return r;
}

STNBAABox AUSceneIntro::privGetTexturePortion(const STNBAABox scnBox, const STNBSize texSz){
	STNBAABox texRelBox;
	if(texSz.width == 0 || texSz.height == 0){
		texRelBox.xMin = texRelBox.yMin = 0;
		texRelBox.xMax = texRelBox.yMax = 1;
	} else {
		const STNBSize scnSz		= { (scnBox.xMax - scnBox.xMin), (scnBox.yMax - scnBox.yMin) };
		const STNBSize texScales	= { texSz.width / scnSz.width, texSz.height / scnSz.height };
		const float texScale		= (texScales.width < texScales.height ? texScales.width : texScales.height);
		const STNBSize texRel		= { texScale / texScales.width, texScale / texScales.height };
		//Calculate
		texRelBox.xMin = (1.0f - texRel.width) * 0.5f;		NBASSERT(texRelBox.xMin >= -0.01f && texRelBox.xMin <= 1.01f)
		texRelBox.xMax = texRelBox.xMin + texRel.width;		NBASSERT(texRelBox.xMax >= -0.01f && texRelBox.xMax <= 1.01f)
		texRelBox.yMin = (1.0f - texRel.height) * 0.5f;		NBASSERT(texRelBox.yMin >= -0.01f && texRelBox.yMin <= 1.01f)
		texRelBox.yMax = texRelBox.yMin + texRel.height;	NBASSERT(texRelBox.yMax >= -0.01f && texRelBox.yMax <= 1.01f)
		//PRINTF_INFO("texSz(%.2f, %.2f) texScales(tex / scn): %.2f, %.2f; texRel(%f, %f) texRelBox-x(%.2f, %.2f)-y(%.2f, %.2f)-sz(+%.2f, +%.2f).\n", texSz.width, texSz.height, texScales.width, texScales.height, texRel.width, texRel.height, texRelBox.xMin, texRelBox.xMax, texRelBox.yMin, texRelBox.yMax, (texRelBox.xMax - texRelBox.xMin), (texRelBox.yMax - texRelBox.yMin));
		if(texRelBox.xMin < 0.0f) texRelBox.xMin = 0.0f; if(texRelBox.xMin > 1.0f) texRelBox.xMin = 1.0f;
		if(texRelBox.xMax < 0.0f) texRelBox.xMax = 0.0f; if(texRelBox.xMax > 1.0f) texRelBox.xMax = 1.0f;
		if(texRelBox.yMin < 0.0f) texRelBox.yMin = 0.0f; if(texRelBox.yMin > 1.0f) texRelBox.yMin = 1.0f;
		if(texRelBox.yMax < 0.0f) texRelBox.yMax = 0.0f; if(texRelBox.yMax > 1.0f) texRelBox.yMax = 1.0f;
	}
	return texRelBox;
}

//

STSceneInfoBtn* AUSceneIntro::privGetButtonByActionId(STSceneInfoStackItm* itm, const char* buttonActionId){
	STSceneInfoBtn* r = NULL;
	if(itm != NULL){
		STSceneInfoContent* content = &itm->content;
		if(content != NULL){
			STSceneInfoContentBase* base = &content->base;
			SI32 i; for(i = 0; i < base->buttons.use; i++){
				STSceneInfoBtn* btn = NBArray_itmPtrAtIndex(&base->buttons, STSceneInfoBtn, i);
				if(NBString_isEqual(&btn->optionId, buttonActionId)){
					r = btn;
					break;
				}
			}
		}
	}
	NBASSERT(r != NULL)
	return r;
}

void AUSceneIntro::privSetButtonVisibleByActionId(STSceneInfoStackItm* itm, const char* buttonActionId, const bool visible, const BOOL animate){
	STSceneInfoBtn* btn = this->privGetButtonByActionId(itm, buttonActionId);
	if(btn != NULL){
		btn->visibility = (visible ? ENSceneInfoBtnVisibility_Allways : ENSceneInfoBtnVisibility_Hidden);
		_animator->purgarAnimacionesDeObjeto(btn->btn);
		if(!animate){
			btn->btn->establecerVisible(visible);
			btn->btn->establecerMultiplicadorColor8(255, 255, 255, 255);
		} else {
			_animator->animarVisible(btn->btn, visible, 0.25f);
		}
	}
}

void AUSceneIntro::privSetContentGlassVisible(STSceneInfoStackItm* itm, const bool visible, const BOOL animate){
	if(itm != NULL){
		STSceneInfoContent* content	= &itm->content;
		const BOOL visibleAtStart	= content->formLayerGlassLayer->visible();
		const NBColor8 colorAtStart	= content->formLayerGlassLayer->multiplicadorColor8Func();
		_animator->quitarAnimacionesDeObjeto(content->formLayerGlassLayer);
		if(!animate){
			content->formLayerGlassLayer->establecerVisible(visible);
			content->formLayerGlassLayer->establecerMultiplicadorColor8(255, 255, 255, 255);
		} else {
			const float speedAlpha	= (256.0f * 4.0f);
			if(visible){
				//Show form-glass
				if(!visibleAtStart){
					content->formLayerGlassLayer->establecerMultiplicadorAlpha8(255);
					_animator->animarVisible(content->formLayerGlassLayer, true, 255.0f / speedAlpha);
				} else {
					_animator->animarColorMult(content->formLayerGlassLayer, 255, 255, 255, 255, (float)(255 - colorAtStart.a) / speedAlpha);
				}
			} else {
				//Hide form-glass
				if(visibleAtStart){
					_animator->animarVisible(content->formLayerGlassLayer, false, (float)colorAtStart.a / speedAlpha);
				}
			}
		}
	}
}

//Stack

STSceneInfoStack* AUSceneIntro::privCurStack(){
	STSceneInfoStack* r = NULL;
	if(_contents.stacks.use > 0){
		r = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, _contents.stacks.use - 1);
	}
	return r;
}

STSceneInfoStackItm* AUSceneIntro::privCurStackCurItm(){
	STSceneInfoStackItm* r = NULL;
	STSceneInfoStack* curStack = this->privCurStack();
	if(curStack != NULL){
		if(curStack->contentStack.use > 0){
			r = NBArray_itmValueAtIndex(&curStack->contentStack, STSceneInfoStackItm*, curStack->contentStack.use - 1);
		}
	}
	return r;
}

STSceneInfoContent* AUSceneIntro::privCurStackCurContent(){
	STSceneInfoContent* r = NULL;
	STSceneInfoStackItm* curItm = this->privCurStackCurItm();
	if(curItm != NULL){
		r = &curItm->content;
	}
	return r;
}

void SceneInfoContentBaseDef_init(STSceneInfoContentBaseDef* obj){
	NBMemory_setZeroSt(*obj, STSceneInfoContentBaseDef);
	obj->bgColor		= ENTSColor_Light;
	obj->backColor		= ENTSColor_Black;
	obj->showTopLogo	= TRUE;
	obj->showSteps		= TRUE;
	obj->showAsStep		= TRUE;
	obj->showResetDataBtn = FALSE;
	obj->titleCentered	= FALSE;
	NBString_init(&obj->shortName);
	NBString_init(&obj->title);
	NBString_init(&obj->explainTop);
	NBString_init(&obj->privacyTitle);
	NBString_init(&obj->privacyText);
	obj->privacyAlign	= ENNBTextLineAlignH_Count;
	obj->buttonsPos		= ENNBTextAlignV_Count;
	NBArray_init(&obj->buttons, sizeof(STSceneInfoBtnDef), NULL);
	NBArray_init(&obj->txtBoxes, sizeof(AUITextBox*), NULL);
	NBArray_init(&obj->touchables, sizeof(AUEscenaObjeto*), NULL);
}

void SceneInfoContentBaseDef_release(STSceneInfoContentBaseDef* obj){
	NBString_release(&obj->shortName);
	NBString_release(&obj->title);
	NBString_release(&obj->explainTop);
	NBString_release(&obj->privacyTitle);
	NBString_release(&obj->privacyText);
	//
	NBArray_release(&obj->buttons);
	NBArray_release(&obj->txtBoxes);
	NBArray_release(&obj->touchables);
}

void AUSceneIntro::privStackPushContent(STSceneInfoStack* obj, const ENSceneInfoContent uid){
	AUFuenteRender* fntTitles	= TSFonts::font(ENTSFont_ContentTitle);
	AUFuenteRender* fntExplain	= TSFonts::font(ENTSFont_ContentMid);
	//
	AUFuenteRender* lnkFont		= TSFonts::font(ENTSFont_Btn);
	AUFuenteRender* btnFont		= TSFonts::font(ENTSFont_Btn);
	const float btnMarginIntH	= NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.07f); //Internal margin (top & bottom)
	AUTextura* tex				= NBGestorTexturas::texturaDesdeArchivo("thinstream/mesh.png#btn");
	//Test, create all
	const STNBSize sceneSz		= { (_lastBoxUsable.xMax - _lastBoxUsable.xMin), (_lastBoxUsable.yMax - _lastBoxUsable.yMin) };
	const ENTSScreenOrient sceneOrient = (sceneSz.width > sceneSz.height ? ENTSScreenOrient_Landscape : ENTSScreenOrient_Portrait);
	{
		STSceneInfoStackItm* itm = NBMemory_allocType(STSceneInfoStackItm);
		SceneInfoStackItm_init(itm);
		itm->content.uid = uid;
		//Form, title and explainations
		{
			STSceneInfoContentBaseDef baseDef;
			SceneInfoContentBaseDef_init(&baseDef);
			{
				STSceneInfoContentBase* base = &itm->content.base;
				itm->content.data = this->privContentCreate(obj->contentStack.use, itm->content.uid, itm->content.formLayer, &baseDef, sceneSz, sceneOrient);
				//Bahavoir
				{
					base->bgColor		= baseDef.bgColor;
					base->backColor		= baseDef.backColor;
					base->showTopLogo	= baseDef.showTopLogo;
					base->showSteps		= baseDef.showSteps;
					base->showAsStep	= baseDef.showAsStep;
					base->showResetDataBtn = baseDef.showResetDataBtn;
					base->titleCentered	= baseDef.titleCentered;
					base->buttonsPos	= baseDef.buttonsPos;
					base->privacyLnkAlign = baseDef.privacyAlign;
				}
				//Short name
				{
					NBString_set(&itm->content.base.shortName, baseDef.shortName.str);
				}
				//Privacy
				{
					if(baseDef.privacyTitle.length > 0){
						//Forcing the tittle of the privacy link
						NBString_set(&baseDef.privacyTitle, TSClient_getStr(_coreClt, "stpLnkMoreInfo", "More info"));
					}
					NBString_set(&itm->content.base.privacyTitle, baseDef.privacyTitle.str);
					NBString_set(&itm->content.base.privacyText, baseDef.privacyText.str);
				}
				//Title
				if(baseDef.title.length > 0){
					const STNBColor8 color = TSColors::colorDef(ENTSColor_MainColor)->normal;
					NBASSERT(base->title == NULL)
					base->title	= new(this) AUEscenaTexto(fntTitles);
					base->title->establecerAlineaciones(ENNBTextLineAlignH_Left, ENNBTextAlignV_FromBottom);
					base->title->establecerTexto(baseDef.title.str);
					base->title->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
					itm->content.layer->agregarObjetoEscena(base->title);
				}
				//Explain
				if(baseDef.explainTop.length > 0){
					NBASSERT(base->explainTop == NULL)
					base->explainTop	= new(this) AUEscenaTexto(fntExplain);
					base->explainTop->establecerAlineaciones(ENNBTextLineAlignH_Left, ENNBTextAlignV_FromBottom);
					base->explainTop->establecerTexto(baseDef.explainTop.str);
					base->explainTop->establecerMultiplicadorColor8(0, 0, 0, 255);
					itm->content.layer->agregarObjetoEscena(base->explainTop);
				}
				//Privacy link
				if(baseDef.privacyTitle.length > 0){
					NBASSERT(base->privacyLnk == NULL && base->privacyLnkLine == NULL)
					const STNBColor8 color = TSColors::colorDef(ENTSColor_Gray)->normal;
					const STNBColor8 colorIco = TSColors::colorDef(ENTSColor_Gray)->normal;
					//
					base->privacyLnk = new(this) AUEscenaTexto(lnkFont);
					base->privacyLnk->establecerTexto(baseDef.privacyTitle.str);
					base->privacyLnk->establecerEscuchadorTouches(this, this);
					base->privacyLnk->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
					itm->content.layer->agregarObjetoEscena(base->privacyLnk);
					//
					base->privacyLnkIco = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#info"));
					base->privacyLnkIco->establecerEscalacion(0.50f);
					base->privacyLnkIco->establecerEscuchadorTouches(this, this);
					base->privacyLnkIco->establecerMultiplicadorColor8(colorIco.r, colorIco.g, colorIco.b, colorIco.a);
					itm->content.layer->agregarObjetoEscena(base->privacyLnkIco);
					//
					base->privacyLnkLine = new(this) AUEscenaFigura(ENEscenaFiguraTipo_Linea);
					base->privacyLnkLine->agregarCoordenadaLocal(0, 0);
					base->privacyLnkLine->agregarCoordenadaLocal(0, 0);
					base->privacyLnkLine->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
					itm->content.layer->agregarObjetoEscena(base->privacyLnkLine);
				}
				//FormLayer; add this after every 'extra'; the form must be at the top
				itm->content.layer->agregarObjetoEscena(itm->content.formLayerLimit);
			}
			//Create buttons
			{
				SI32 i; for(i = 0 ; i < baseDef.buttons.use; i++){
					STSceneInfoBtnDef* def = NBArray_itmPtrAtIndex(&baseDef.buttons, STSceneInfoBtnDef, i);
					if((def->text.length > 0 || def->icon != NULL) && def->optionId.length > 0){
						STSceneInfoBtn btn;
						btn.bgColor		= def->bgColor;
						btn.fgColor		= def->fgColor;
						btn.visibility	= def->visibility;
						{
							const STSceneInfoColorDef* bgColor = TSColors::colorDef(def->bgColor);
							const STSceneInfoColorDef* fgColor = TSColors::colorDef(def->fgColor);
							NBColor8 bgs[3], fgs[3];
							NBCOLOR_ESTABLECER(bgs[0], bgColor->normal.r, bgColor->normal.g, bgColor->normal.b, bgColor->normal.a)
							NBCOLOR_ESTABLECER(bgs[1], bgColor->hover.r, bgColor->hover.g, bgColor->hover.b, bgColor->hover.a)
							NBCOLOR_ESTABLECER(bgs[2], bgColor->touched.r, bgColor->touched.g, bgColor->touched.b, bgColor->touched.a)
							NBCOLOR_ESTABLECER(fgs[0], fgColor->normal.r, fgColor->normal.g, fgColor->normal.b, fgColor->normal.a)
							NBCOLOR_ESTABLECER(fgs[1], fgColor->hover.r, fgColor->hover.g, fgColor->hover.b, fgColor->hover.a)
							NBCOLOR_ESTABLECER(fgs[2], fgColor->touched.r, fgColor->touched.g, fgColor->touched.b, fgColor->touched.a)
							AUIButton* obj = new(this) AUIButton();
							if(def->icon != NULL){
								obj->agregarIcono(def->icon, ENIBotonItemAlign_Center, 0.5f, 0, 0, fgs[0], fgs[1], fgs[2]);
							}
							if(def->text.length > 0){
								obj->agregarTextoMultilinea(btnFont, def->text.str, ENIBotonItemAlign_Center, 0.5f, 0, 0, fgs[0], fgs[1], fgs[2]);
							}
							obj->establecerFondo(true, tex);
							obj->establecerFondoColores(bgs[0], bgs[1], bgs[2]);
							obj->establecerMargenes(0.0f, 0.0f, btnMarginIntH, btnMarginIntH, 0.0f);
							obj->establecerSegsRetrasarOnTouch(0.0f);
							obj->establecerEscuchadorBoton(this);
							obj->organizarContenido(false);
							itm->content.btnsLayer->agregarObjetoEscena(obj);
							btn.btn = obj;
						}
						NBString_initWithStr(&btn.optionId, def->optionId.str);
						NBArray_addValue(&itm->content.base.buttons, btn);
					}
					//Release
					NBString_release(&def->text);
					NBString_release(&def->optionId);
				}
			}
			//TextBoxes
			{
				SI32 i; for(i = 0 ; i < baseDef.txtBoxes.use; i++){
					NBASSERT(NBArray_itmValueAtIndex(&baseDef.txtBoxes, AUITextBox*, i)->esClase(AUITextBox::idTipoClase))
					AUITextBox* txtBox = NBArray_itmValueAtIndex(&baseDef.txtBoxes, AUITextBox*, i);
					if(txtBox != NULL){
						txtBox->retener(NB_RETENEDOR_THIS);
						txtBox->establecerEscuchadorTextBox(this);
						NBArray_addValue(&itm->content.base.txtBoxes, txtBox);
					}
				}
			}
			//Touchables
			{
				SI32 i; for(i = 0 ; i < baseDef.touchables.use; i++){
					NBASSERT(NBArray_itmValueAtIndex(&baseDef.touchables, AUEscenaObjeto*, i)->esClase(AUEscenaObjeto::idTipoClase))
					AUEscenaObjeto* obj = NBArray_itmValueAtIndex(&baseDef.touchables, AUEscenaObjeto*, i);
					if(obj != NULL){
						obj->retener(NB_RETENEDOR_THIS);
						obj->establecerEscuchadorTouches(this, this);
						NBArray_addValue(&itm->content.base.touchables, obj);
					}
				}
			}
			SceneInfoContentBaseDef_release(&baseDef);
		}
		NBArray_addValue(&obj->contentStack, itm);
	}
}

//

BOOL AUSceneIntro::isValidIMEI(const char* str, const UI32 strLen, STNBString* dstFilteredDigits){
	BOOL r = FALSE;
	//Verify format.
	//The IMEI (15 decimal digits: 14 digits plus a check digit)
	//IMEISV (16 decimal digits: 14 digits plus two software version digits) includes information on the origin, model, and serial number of the device.
	if(strLen > 14){
		UI32 digitsFound = 0;
		SI32 i; for(i = 0 ; i < strLen && digitsFound < 20; i++){
			const char c = str[i];
			if(c >= '0' && c <= '9'){
				digitsFound++;
				if(dstFilteredDigits != NULL){
					NBString_concatByte(dstFilteredDigits, c);
				}
			} else if(c == '-' || c == ' '){
				//Accepted
			} else {
				//Not accepted
				if(dstFilteredDigits != NULL){
					NBString_empty(dstFilteredDigits);
				}
				break;
			}
		}
		if(digitsFound == 15 || digitsFound == 16){
			r = TRUE;
		}
	}
	return r;
}

//Content - CREATE

void* AUSceneIntro::privContentCreate(const UI32 curStackSize, const ENSceneInfoContent uid, AUEscenaContenedor* layer, STSceneInfoContentBaseDef* dstBaseDef, const STNBSize sceneSz, const ENTSScreenOrient orient){
	void* r = NULL;
	switch (uid) {
		case ENSceneInfoContent_NotifsAuth:
			{
				STSceneInfoContent_NotifsAuth* d = NBMemory_allocType(STSceneInfoContent_NotifsAuth);
				NBMemory_set(d, 0, sizeof(*d));
				//Base props
				if(dstBaseDef != NULL){
					//Behavoir
					{
						dstBaseDef->bgColor		= ENTSColor_Light;
						dstBaseDef->backColor	= (curStackSize > 0 || _listener->scenesStackSize() > 0 ? ENTSColor_Gray : ENTSColor_Count);
						dstBaseDef->showTopLogo	= TRUE;
						dstBaseDef->showSteps	= TRUE;
						dstBaseDef->showAsStep	= FALSE;
						dstBaseDef->privacyAlign = ENNBTextLineAlignH_Center;
						dstBaseDef->titleCentered = TRUE;
					}
					//Status
					{
						d->requestAuth	= FALSE;
						d->lastStatus	= NBMngrNotifs::getAuthStatus(ENAppNotifAuthQueryMode_CacheOnly);
					}
					//Short name (for step indicator)
					{
						const char* strLoc = TSClient_getStr(_coreClt, "stpNotifName", "Notif");
						NBString_set(&dstBaseDef->shortName, strLoc);
					}
					//Welcome
					{
						const char* strLoc = TSClient_getStr(_coreClt, "stpNotifTitle", "Notifications");
						NBString_set(&dstBaseDef->title, strLoc);
					}
					//Explain
					if(d->lastStatus == ENAppNotifAuthState_Authorized){
						const char* strLoc = TSClient_getStr(_coreClt, "stpNotifTitleEnabled", "You have enabled push notification.");
						NBString_set(&dstBaseDef->explainTop, strLoc);
					} else {
						const char* strLoc = TSClient_getStr(_coreClt, "stpNotifTitleAsk", "We can keep you up-to-date by sending push notifications for appointments and required documents. Please enable notifications.");
						NBString_set(&dstBaseDef->explainTop, strLoc);
					}
					//Buttons
					{
						if(d->lastStatus == ENAppNotifAuthState_Authorized){
							STSceneInfoBtnDef btn;
							btn.bgColor		= ENTSColor_MainColor;
							btn.fgColor		= ENTSColor_White;
							btn.visibility	= ENSceneInfoBtnVisibility_Allways;
							btn.icon		= NULL;
							{
								const char* strLoc = TSClient_getStr(_coreClt, "stpNotifOkBtn", "Continue");
								NBString_initWithStr(&btn.text, strLoc);
							}
							NBString_initWithStr(&btn.optionId, "continue");
							NBArray_addValue(&dstBaseDef->buttons, btn);
						} else {
							{
								STSceneInfoBtnDef btn;
								btn.bgColor		= ENTSColor_MainColor;
								btn.fgColor		= ENTSColor_White;
								btn.visibility	= ENSceneInfoBtnVisibility_Allways;
								btn.icon		= NULL;
								{
									const char* strLoc = TSClient_getStr(_coreClt, "stpNotifEnableBtn", "Enable notifications");
									NBString_initWithStr(&btn.text, strLoc);
								}
								NBString_initWithStr(&btn.optionId, "enable");
								NBArray_addValue(&dstBaseDef->buttons, btn);
							}
							{
								STSceneInfoBtnDef btn;
								btn.bgColor		= ENTSColor_GrayLight;
								btn.fgColor		= ENTSColor_Black;
								btn.visibility	= ENSceneInfoBtnVisibility_Allways;
								btn.icon		= NULL;
								{
									const char* strLoc = TSClient_getStr(_coreClt, "stpNotifSkipBtn", "Maybe later");
									NBString_initWithStr(&btn.text, strLoc);
								}
								NBString_initWithStr(&btn.optionId, "skip");
								NBArray_addValue(&dstBaseDef->buttons, btn);
							}
						}
					}
				}
				//Layer
				{
					d->formLayer = new(this) AUEscenaContenedorLimitado();
					layer->agregarObjetoEscena(d->formLayer);
					//ToDo: remove, this was moved to a button action
					/*if(!passBioEnabled){
					 NBArray_addValue(&dstBaseDef->touchables, d->formLayer);
					 }*/
				}
				//Icon
				{
					const STNBColor8 color = TSColors::colorDef(ENTSColor_MainColor)->normal;
					d->icon			= new AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo("thinstream/icons.png#bell"));
					d->icon->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
					d->icon->establecerEscalacion(2.0f);
					d->formLayer->agregarObjetoEscena(d->icon);
				}
				//Explain
				{
					AUFuenteRender* fnt = TSFonts::font(ENTSFont_ContentMid);
					const STNBColor8 color = TSColors::colorDef(ENTSColor_MainColor)->normal;
					d->explainWidth = (sceneSz.width * 0.75f);
					d->explain	= new(this) AUEscenaTexto(fnt);
					d->explain->establecerAlineaciones(ENNBTextLineAlignH_Center, ENNBTextAlignV_FromTop);
					if(d->lastStatus == ENAppNotifAuthState_Authorized){
						const char* strLoc = TSClient_getStr(_coreClt, "notifEnabledState", "Enabled");
						d->explain->establecerTexto(strLoc);
					} else {
						d->explain->establecerTexto("");
					}
					d->explain->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
					d->formLayer->agregarObjetoEscena(d->explain);
				}
				r = d;
			}
			break;
		case ENSceneInfoContent_TermsAccept:
			{
				STSceneInfoContent_TermsAccept* d = NBMemory_allocType(STSceneInfoContent_TermsAccept);
				NBMemory_set(d, 0, sizeof(*d));
				if(dstBaseDef != NULL){
					STNBString clinicName, firstNames, lastNames;
					NBString_init(&clinicName);
					NBString_init(&firstNames);
					NBString_init(&lastNames);
					{
						STNBStorageRecordRead ref = TSClient_getStorageRecordForRead(_coreClt, "client/_root/", "_current", NULL, TSClientRoot_getSharedStructMap());
						if(ref.data != NULL){
							STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
							if(priv != NULL){
								STTSClientRootReq* reqq = &priv->req;
								if(!NBString_strIsEmpty(reqq->clinicName)){
									NBString_set(&clinicName, reqq->clinicName);
								}
								if(!NBString_strIsEmpty(reqq->firstNames)){
									NBString_set(&firstNames, reqq->firstNames);
								}
								if(!NBString_strIsEmpty(reqq->lastNames)){
									NBString_set(&lastNames, reqq->lastNames);
								}
							}
						}
						TSClient_returnStorageRecordFromRead(_coreClt, &ref);
					}
					//Behavoir
					{
						dstBaseDef->bgColor		= ENTSColor_Light;
						dstBaseDef->backColor	= (curStackSize > 0 || _listener->scenesStackSize() > 0 ? ENTSColor_Gray : ENTSColor_Count);
						dstBaseDef->showTopLogo = TRUE;
						dstBaseDef->showSteps	= TRUE;
						dstBaseDef->showAsStep	= TRUE;
						dstBaseDef->titleCentered = FALSE;
					}
					{
						//Short name (for step indicator)
						{
							const char* strLoc = TSClient_getStr(_coreClt, "stpConfirmName", "Confirm");
							NBString_set(&dstBaseDef->shortName, strLoc);
						}
						//Welcome
						{
							//const char* strLoc = TSClient_getStr(_coreClt, "stpTermsTitle", "Terms & Conditions");
							//NBString_set(&dstBaseDef->title, strLoc);
						}
						//Explain
						NBString_set(&dstBaseDef->explainTop, "");
					}
					{
						d->formLayer			= new(this) AUEscenaContenedorLimitado();
						layer->agregarObjetoEscena(d->formLayer);
					}
					//txtTittle
					/*{
						const char* strLoc = TSClient_getStr(_coreClt, "stpConfirmTitle", "WELCOME");
						AUFuenteRender* fntHuge	= TSFonts::font(ENTSFont_TitleHuge);
						const STNBColor8 color2 = TSColors::colorDef(ENTSColor_MainColor)->normal;
						d->txtTitle = new(this) AUEscenaTexto(fntHuge);
						d->txtTitle->establecerAlineacionH(ENNBTextLineAlignH_Center);
						d->txtTitle->establecerTexto(strLoc);
						d->txtTitle->establecerMultiplicadorColor8(color2.r, color2.g, color2.b, color2.a);
						d->formLayer->agregarObjetoEscena(d->txtTitle);
					}*/
					//txtExplainTop
					{
						const STNBColor8 color = TSColors::colorDef(ENTSColor_Black)->normal;
						const STNBColor8 color2 = TSColors::colorDef(ENTSColor_MainColor)->normal;
						AUFuenteRender* fntTitle = TSFonts::font(ENTSFont_ContentTitleBig);
						AUFuenteRender* fntTxt	= TSFonts::font(ENTSFont_ContentMid);
						STNBText text;
						NBText_init(&text);
						NBText_setFont(&text, fntTxt->getFontMetricsIRef(fntTxt->tamanoEscena()));
						NBText_setLineAlign(&text, ENNBTextLineAlignH_Center);
						{
							const char* strLoc = TSClient_getStr(_coreClt, "stpConfirmHello", "Hello ");
							NBText_pushFormat(&text);
							NBText_setColor(&text, color);
							NBText_appendText(&text, strLoc);
							if(strLoc[NBString_strLenBytes(strLoc) - 1] != ' '){
								NBText_appendText(&text, " ");
							}
							NBText_popFormat(&text);
						}
						{	
							NBText_pushFormat(&text);
							NBText_setColor(&text, color2);
							if(firstNames.length > 0){
								NBText_appendText(&text, firstNames.str);
							} else {
								NBText_appendText(&text, lastNames.str);
							}
							NBText_popFormat(&text);
						}
						{
							const char* strLoc = TSClient_getStr(_coreClt, "stpConfirmBody01", ",\nthis invitation was issued by:\n\n");
							NBText_pushFormat(&text);
							NBText_setColor(&text, color);
							NBText_appendText(&text, strLoc);
							if(strLoc[0] != ','){
								NBText_appendText(&text, ",");
							}
							if(strLoc[NBString_strLenBytes(strLoc) - 1] != '\n'){
								NBText_appendText(&text, "\n");
							}
							NBText_popFormat(&text);
						}
						//Clinic name
						{
							NBText_pushFormat(&text);
							NBText_setFont(&text, fntTitle->getFontMetricsIRef(fntTitle->tamanoEscena()));
							NBText_setColor(&text, color2);
							NBText_appendText(&text, clinicName.str);
							NBText_popFormat(&text);
						}
						//
						{
							const char* strLoc = TSClient_getStr(_coreClt, "stpConfirmBody02", "\n\nPlease read before proceeding:");
							NBText_pushFormat(&text);
							NBText_setColor(&text, color);
							if(strLoc[0] != '\n'){
								NBText_appendText(&text, "\n");
							}
							NBText_appendText(&text, strLoc);
							NBText_popFormat(&text);
						}
						//Patient name
						/*{
							NBText_pushFormat(&text);
							NBText_setColor(&text, color2);
							NBText_appendText(&text, firstNames.str);
							if(firstNames.length > 0 && lastNames.length > 0){
								NBText_appendText(&text, " ");
							}
							NBText_appendText(&text, lastNames.str);
							NBText_popFormat(&text);
						}*/
						/*{
							const char* strLoc = TSClient_getStr(_coreClt, "stpConfirmBody03", " and you agree with our terms and policies of the service.");
							NBText_pushFormat(&text);
							NBText_setColor(&text, color);
							if(strLoc[0] != ' '){
								NBText_appendText(&text, " ");
							}
							NBText_appendText(&text, strLoc);
							NBText_popFormat(&text);
						}*/
						{
							d->txtExplainTop = new(this) AUEscenaTexto(fntTxt);
							d->txtExplainTop->establecerAlineacionH(ENNBTextLineAlignH_Center);
							d->txtExplainTop->establecerTexto(&text);
							d->formLayer->agregarObjetoEscena(d->txtExplainTop);
						}
						NBText_release(&text);
					}
					//Terms link
					{
						const STNBColor8 color = TSColors::colorDef(ENTSColor_Black)->normal;
						AUFuenteRender* fntTxt	= TSFonts::font(ENTSFont_ContentMid);
						const char* strLoc = TSClient_getStr(_coreClt, "linkTermsTxt", "Terms and conditions");
						//
						d->linkTerms.txt	= new(this) AUEscenaTexto(fntTxt);
						d->linkTerms.txt->establecerTexto(strLoc);
						d->linkTerms.txt->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
						NBArray_addValue(&dstBaseDef->touchables, d->linkTerms.txt);
						d->formLayer->agregarObjetoEscena(d->linkTerms.txt);
						//
						d->linkTerms.line	= new(this) AUEscenaFigura(ENEscenaFiguraTipo_Linea);
						d->linkTerms.line->agregarCoordenadaLocal(0.0f, 0.0f);
						d->linkTerms.line->agregarCoordenadaLocal(0.0f, 0.0f);
						d->linkTerms.line->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
						d->formLayer->agregarObjetoEscena(d->linkTerms.line);
					}
					//Privacy link
					{
						const STNBColor8 color = TSColors::colorDef(ENTSColor_Black)->normal;
						AUFuenteRender* fntTxt	= TSFonts::font(ENTSFont_ContentMid);
						const char* strLoc = TSClient_getStr(_coreClt, "linkPrivacyTxt", "Privacy policy");
						//
						d->linkPrivacy.txt	= new(this) AUEscenaTexto(fntTxt);
						d->linkPrivacy.txt->establecerTexto(strLoc);
						d->linkPrivacy.txt->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
						NBArray_addValue(&dstBaseDef->touchables, d->linkPrivacy.txt);
						d->formLayer->agregarObjetoEscena(d->linkPrivacy.txt);
						//
						d->linkPrivacy.line	= new(this) AUEscenaFigura(ENEscenaFiguraTipo_Linea);
						d->linkPrivacy.line->agregarCoordenadaLocal(0.0f, 0.0f);
						d->linkPrivacy.line->agregarCoordenadaLocal(0.0f, 0.0f);
						d->linkPrivacy.line->establecerMultiplicadorColor8(color.r, color.g, color.b, color.a);
						d->formLayer->agregarObjetoEscena(d->linkPrivacy.line);
					}
					//Buttons
					{
						//Accept
						{
							STSceneInfoBtnDef btn;
							btn.bgColor		= ENTSColor_MainColor;
							btn.fgColor		= ENTSColor_White;
							btn.visibility	= ENSceneInfoBtnVisibility_Allways;
							btn.icon		= NULL;
							{
								const char* strLoc = TSClient_getStr(_coreClt, "stpConfirmOkBtn", "Accept Invitation");
								NBString_initWithStr(&btn.text, strLoc);
							}
							NBString_initWithStr(&btn.optionId, "continue");
							NBArray_addValue(&dstBaseDef->buttons, btn);
						}
						//Cancel
						{
							STSceneInfoBtnDef btn;
							btn.bgColor		= ENTSColor_Light;
							btn.fgColor		= ENTSColor_Black;
							btn.visibility	= ENSceneInfoBtnVisibility_Allways;
							btn.icon		= NULL;
							{
								const char* strLoc = TSClient_getStr(_coreClt, "stpConfirmCancelBtn", "Decline / I am not this person");
								NBString_initWithStr(&btn.text, strLoc);
							}
							NBString_initWithStr(&btn.optionId, "cancel");
							NBArray_addValue(&dstBaseDef->buttons, btn);
						}
					}
					NBString_release(&clinicName);
					NBString_release(&firstNames);
					NBString_release(&lastNames);
				}
				r = d;
			}
			break;
#		ifdef NB_CONFIG_INCLUDE_ASSERTS
		default:
			NBASSERT(FALSE) //Unexpected content type
			break;
#		endif
	}
	return r;
}

//Content - UPDATE AFTER RESIZE

void AUSceneIntro::privContentUpdateAfterResize(const ENSceneInfoContent uid, void* data){
	{
		switch (uid) {
			case ENSceneInfoContent_NotifsAuth: break;
			case ENSceneInfoContent_TermsAccept: break;
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			default:
				NBASSERT(FALSE) //Unexpected content type
				break;
#			endif
		}
	}
}

//Content - DESTROY

void AUSceneIntro::privContentDestroy(const ENSceneInfoContent uid, void* data){
	//Do not worry about the layer's content
	//(will be automatically removed from scene)
	{
		switch (uid) {
			case ENSceneInfoContent_NotifsAuth:
				{
					STSceneInfoContent_NotifsAuth* d = (STSceneInfoContent_NotifsAuth*)data;
					if(d->formLayer != NULL)		d->formLayer->liberar(NB_RETENEDOR_THIS); d->formLayer = NULL;
					if(d->icon != NULL)				d->icon->liberar(NB_RETENEDOR_THIS); d->icon = NULL;
					if(d->explain != NULL)			d->explain->liberar(NB_RETENEDOR_THIS); d->explain = NULL;
					NBMemory_free(d);
				}
				break;
			case ENSceneInfoContent_TermsAccept:
				{
					STSceneInfoContent_TermsAccept*		d = (STSceneInfoContent_TermsAccept*)data;
					if(d->formLayer != NULL)		d->formLayer->liberar(NB_RETENEDOR_THIS); d->formLayer = NULL;
					//if(d->txtTitle != NULL)			d->txtTitle->liberar(NB_RETENEDOR_THIS); d->txtTitle = NULL;
					if(d->txtExplainTop != NULL)	d->txtExplainTop->liberar(NB_RETENEDOR_THIS); d->txtExplainTop = NULL;
					//
					if(d->linkTerms.txt != NULL)	d->linkTerms.txt->liberar(); d->linkTerms.txt = NULL;
					if(d->linkTerms.line != NULL)	d->linkTerms.line->liberar(); d->linkTerms.line = NULL;
					//
					if(d->linkPrivacy.txt != NULL)	d->linkPrivacy.txt->liberar(); d->linkPrivacy.txt = NULL;
					if(d->linkPrivacy.line != NULL)	d->linkPrivacy.line->liberar(); d->linkPrivacy.line = NULL;
					//
					NBMemory_free(d);
				}
				break;
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			default:
				NBASSERT(FALSE) //Unexpected content type
				break;
#			endif
		}
	}
}

//Content - Organize

void AUSceneIntro::privContentOrganizeItm(STSceneInfoStackItm* itm){
	const NBCajaAABB cajaEscena = _lastBox;
	const STNBSize sceneSz		= { (_lastBoxUsable.xMax - _lastBoxUsable.xMin), (_lastBoxUsable.yMax - _lastBoxUsable.yMin) };
	const STNBSize sceneSzInch	= { NBGestorEscena::anchoEscenaAPulgadas(_iScene, sceneSz.width), NBGestorEscena::altoEscenaAPulgadas(_iScene, sceneSz.height) };
	const float sceneDiagInch	= sqrtf((sceneSzInch.width * sceneSzInch.width) + (sceneSzInch.height * sceneSzInch.height));
	//
	const float yTop			= _contents.yTopBeforeContent;
	const float yBtmBeforeContent = _contents.yBtmBeforeContent;
	const float yBtmBeforeSteps	= _contents.yBtmBeforeSteps;
	const ENTSScreenOrient orient = (sceneSz.width > sceneSz.height || sceneSzInch.width >= 3.0f ? ENTSScreenOrient_Landscape : ENTSScreenOrient_Portrait);
	//
	STSceneInfoContent* content = &itm->content;
	float yBtm = (content->base.showSteps ? yBtmBeforeContent : yBtmBeforeSteps);
	float yBtmButtons = yBtm;
	//Buttons
	{
		STSceneInfoContentBase* base = &content->base;
		SI32 i; for(i = ((SI32)base->buttons.use - 1); i >= 0; i--){
			STSceneInfoBtn* btn = NBArray_itmPtrAtIndex(&base->buttons, STSceneInfoBtn, i);
			if(btn->btn != NULL){
				btn->btn->organizarContenido(_metrics.formWidth, 0.0f, false, NULL, 0.0f);
				const NBCajaAABB box = btn->btn->cajaAABBLocal();
				btn->btnPos = NBST_P(STNBPoint,  _lastBoxUsable.xMin - box.xMin + (((_lastBoxUsable.xMax - _lastBoxUsable.xMin) - (box.xMax - box.xMin)) * 0.5f), yBtm - box.yMin );
				btn->btn->establecerTraslacion(btn->btnPos.x, btn->btnPos.y);
				yBtm += (box.yMax - box.yMin) + (_metrics.sepV * 0.5f);
			}
		}
		yBtmButtons = yBtm;
	}
	//
	if(content->data != NULL){
		float txtHeight = 0.0f;
		//Resize Title and explain
		{
			STSceneInfoContentBase* base = &content->base;
			if(orient == ENTSScreenOrient_Portrait && !base->titleCentered){
				//Portrait (aligned with form)
				//ExplainTop
				if(base->explainTop != NULL){
					base->explainTop->establecerVisible(true); //in case were hiddcen by the animation
					base->explainTop->establecerAlineacionH(ENNBTextLineAlignH_Center);
					base->explainTop->organizarTexto(_metrics.boxWidth);
					const NBCajaAABB box = base->explainTop->cajaAABBLocal();
					if(box.xMin != box.xMax || box.yMin != box.yMax){
						txtHeight += (box.yMax - box.yMin) + (_metrics.sepV * 0.5f);
					}
				}
				//Title
				if(base->title != NULL){
					base->title->establecerVisible(true); //in case were hiddcen by the animation
					base->title->establecerAlineacionH(ENNBTextLineAlignH_Center);
					base->title->organizarTexto(_metrics.boxWidth);
					const NBCajaAABB box	= base->title->cajaAABBLocal();
					if(box.xMin != box.xMax || box.yMin != box.yMax){
						txtHeight += (box.yMax - box.yMin) + (_metrics.sepV * 0.5f);
					}
				}
				//Privacy link
				if(base->privacyLnk != NULL){
					base->privacyLnk->establecerVisible(true); //in case were hiddcen by the animation
					base->privacyLnk->establecerAlineacionH(base->privacyLnkAlign == ENNBTextLineAlignH_Count ? ENNBTextLineAlignH_Right : base->privacyLnkAlign);
					base->privacyLnk->organizarTexto(_metrics.boxWidth);
					const NBCajaAABB box	= base->privacyLnk->cajaAABBLocal();
					if(box.xMin != box.xMax || box.yMin != box.yMax){
						txtHeight += (box.yMax - box.yMin) + (_metrics.sepV * 0.5f);
					}
				}
			} else {
				//Landscape
				//ExplainTop
				if(base->explainTop != NULL){
					base->explainTop->establecerVisible(true); //in case were hiddcen by the animation
					base->explainTop->establecerAlineacionH(ENNBTextLineAlignH_Center);
					base->explainTop->organizarTexto(_metrics.boxWidth);
					if(base->explainTop->getLinesCount() > 1){
						base->explainTop->establecerAlineacionH(ENNBTextLineAlignH_Left);
						base->explainTop->organizarTexto(_metrics.boxWidth);
					}
					const NBCajaAABB box	= base->explainTop->cajaAABBLocal();
					if(box.xMin != box.xMax || box.yMin != box.yMax){
						txtHeight += (box.yMax - box.yMin) + (_metrics.sepV * 0.5f);
					}
				}
				//Title
				if(base->title != NULL){
					base->title->establecerVisible(true); //in case were hiddcen by the animation
					base->title->establecerAlineacionH(ENNBTextLineAlignH_Center);
					base->title->organizarTexto(_metrics.boxWidth);
					if(base->title->getLinesCount() > 1){
						base->title->establecerAlineacionH(ENNBTextLineAlignH_Left);
						base->title->organizarTexto(_metrics.boxWidth);
					}
					const NBCajaAABB box	= base->title->cajaAABBLocal();
					if(box.xMin != box.xMax || box.yMin != box.yMax){
						txtHeight += (box.yMax - box.yMin) + (_metrics.sepV * 0.5f);
					}
				}
				//Privacy link
				if(base->privacyLnk != NULL){
					base->privacyLnk->establecerVisible(true); //in case were hiddcen by the animation
					base->privacyLnk->establecerAlineacionH(base->privacyLnkAlign == ENNBTextLineAlignH_Count ? ENNBTextLineAlignH_Right : base->privacyLnkAlign);
					base->privacyLnk->organizarTexto(_metrics.boxWidth);
					const NBCajaAABB box	= base->privacyLnk->cajaAABBLocal();
					if(box.xMin != box.xMax || box.yMin != box.yMax){
						txtHeight += (box.yMax - box.yMin) + (_metrics.sepV * 0.5f);
					}
				}
			}
		}
		//Form layout
		this->privContentOrganize(itm, NBST_P(STNBSize, (float)_metrics.formWidth, (yTop - txtHeight) - yBtmButtons ), orient);
		this->privContentUpdateState(itm, NULL, ENSceneInfoUpdateStateType_Organize, FALSE);
		//Macro layout
		{
			STSceneInfoContentBase* base = &content->base;
			//Organize form
			{
				const NBCajaAABB box = content->formLayer->cajaAABBLocal();
				//To make the globalLayout believe that the form is always in position, even when we moved it to avoid been covered by the visualKeyboard.
				content->formLayerLimit->establecerLimites(box);
				content->formLayerLimitPos = NBST_P(STNBPoint,  0, 0 );
				content->formLayerLimit->establecerTraslacion(content->formLayerLimitPos.x, content->formLayerLimitPos.y);
				//
				content->formLayer->establecerVisible(true); //in case were hiddcen by the animation
				content->formLayerPos = NBST_P(STNBPoint, 0, 0); //(STNBPoint, - box.xMin + ((sceneSz.width - (box.xMax - box.xMin)) * 0.5f), -box.yMin + ((sceneSz.height - (box.yMax - box.yMin)) * 0.5f) );
				content->formLayer->establecerTraslacion(content->formLayerPos.x, content->formLayerPos.y);
			}
			//Organize Title, explain and privacy
			{
				const NBPunto frmPos	= content->formLayerLimit->traslacion();
				const NBCajaAABB frmBox = content->formLayerLimit->cajaAABBLocal();
				{
					NBASSERT(_metrics.formWidth >= _metrics.boxWidth)
					const float deltaFrmBox = (frmBox.xMax - frmBox.xMin) - _metrics.boxWidth;
					float yBottom = frmPos.y + frmBox.yMax + _metrics.sepV;
					//ExplainTop
					if(base->explainTop != NULL){
						const NBCajaAABB box			= base->explainTop->cajaAABBLocal();
						const ENNBTextLineAlignH alignH	= base->explainTop->alineacionH();
						const float relH				= (alignH == ENNBTextLineAlignH_Right ? 1.0f : alignH == ENNBTextLineAlignH_Center ? 0.5f : 0.0f);
						const float xPos				= frmPos.x + frmBox.xMin + (deltaFrmBox * 0.5f) - box.xMin + ((_metrics.boxWidth - (box.xMax - box.xMin)) * relH);
						base->explainTopPos				= NBST_P(STNBPoint,  xPos, yBottom - box.yMin);
						base->explainTop->establecerTraslacion(base->explainTopPos.x, base->explainTopPos.y);
						//PRINTF_INFO("ExplainBox x(%f, %f)-y(%f, %f): '%s'.\n", box.xMin, box.xMax, box.yMin, box.yMax, base->explainTop->texto());
						if(box.xMin != box.xMax || box.yMin != box.yMax){
							yBottom += (box.yMax - box.yMin) + (_metrics.sepV * 0.5f);
						}
					}
					//Title
					if(base->title != NULL){
						const NBCajaAABB box			= base->title->cajaAABBLocal();
						const ENNBTextLineAlignH alignH	= base->title->alineacionH();
						const float relH				= (alignH == ENNBTextLineAlignH_Right ? 1.0f : alignH == ENNBTextLineAlignH_Center ? 0.5f : 0.0f);
						const float xPos				= frmPos.x + frmBox.xMin + (deltaFrmBox * 0.5f) - box.xMin + ((_metrics.boxWidth - (box.xMax - box.xMin)) * relH);
						base->titlePos					= NBST_P(STNBPoint, xPos, yBottom - box.yMin );
						base->title->establecerTraslacion(base->titlePos.x, base->titlePos.y);
						//PRINTF_INFO("TitleBox x(%f, %f)-y(%f, %f): '%s'.\n", box.xMin, box.xMax, box.yMin, box.yMax, base->title->texto());
						if(box.xMin != box.xMax || box.yMin != box.yMax){
							yBottom += (box.yMax - box.yMin) + (_metrics.sepV * 0.5f);
						}
					}
					//Privacy link
					if(base->privacyLnk != NULL){
						const float	icoMarginI			= NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.04f);
						float yTop						= frmPos.y + frmBox.yMin - (_metrics.sepV * 0.5f);
						const NBCajaAABB box			= base->privacyLnk->cajaAABBLocal();
						const NBCajaAABB boxIco			= base->privacyLnkIco->cajaAABBLocal();
						const NBTamano scaleIco			= base->privacyLnkIco->escalacion();
						const ENNBTextLineAlignH alignH	= ENNBTextLineAlignH_Right; //base->privacyLnk->alineacionH();
						const float relH				= (alignH == ENNBTextLineAlignH_Right ? 1.0f : alignH == ENNBTextLineAlignH_Center ? 0.5f : 0.0f);
						const float xPos				= frmPos.x + frmBox.xMin + (deltaFrmBox * 0.5f) - box.xMin + ((_metrics.boxWidth - (box.xMax - box.xMin)) * relH); //- icoMarginI - ((boxIco.xMax - boxIco.xMin) * scaleIco.ancho)
						//LnkText
						base->privacyLnkPos		= NBST_P(STNBPoint, xPos, yTop - box.yMax);
						base->privacyLnk->establecerTraslacion(base->privacyLnkPos.x, base->privacyLnkPos.y);
						//LnkIcon
						base->privacyLnkIcoPos = NBST_P(STNBPoint, base->privacyLnkPos.x + box.xMin - icoMarginI - (boxIco.xMax * scaleIco.ancho), base->privacyLnkPos.y + box.yMax - (boxIco.yMax * scaleIco.alto) + ((((boxIco.yMax - boxIco.yMin) * scaleIco.alto) - (box.yMax - box.yMin)) * 0.5f));
						base->privacyLnkIco->establecerTraslacion(base->privacyLnkIcoPos.x, base->privacyLnkIcoPos.y);
						//LnkLine
						base->privacyLnkLinePos = NBST_P(STNBPoint, base->privacyLnkPos.x, base->privacyLnkPos.y + box.yMax - base->privacyLnk->fuenteRender()->ascendenteEscena() + base->privacyLnk->fuenteRender()->descendenteEscena());
						base->privacyLnkLine->moverVerticeHacia(0, box.xMin, 0.0f);
						base->privacyLnkLine->moverVerticeHacia(1, box.xMax, 0.0f);
						base->privacyLnkLine->establecerTraslacion(base->privacyLnkLinePos.x, base->privacyLnkLinePos.y);
					}
					
				}
			}
		}
		//Organize content-layer (normal state)
		{
			const NBCajaAABB box = content->layer->cajaAABBLocal();
			content->layerPos = NBST_P(STNBPoint,  _lastBoxUsable.xMin - box.xMin + ((sceneSz.width - (box.xMax - box.xMin)) * 0.5f), yBtm - box.yMin + (((yTop - yBtm) - (box.yMax - box.yMin)) * 0.5f));
			content->layer->establecerTraslacion(content->layerPos.x, content->layerPos.y);
			//Calculate content-scene-box
			content->layerSceneBox.xMin = content->layerPos.x + box.xMin;
			content->layerSceneBox.xMax = content->layerPos.x + box.xMax;
			content->layerSceneBox.yMin = content->layerPos.y + box.yMin;
			content->layerSceneBox.yMax = content->layerPos.y + box.yMax;
			//Calculate form-scene-box
			{
				const NBPunto frmPos	= content->formLayerLimit->traslacion();
				const NBCajaAABB frmBox = content->formLayerLimit->cajaAABBLocal();
				content->formLayerLimitSceneBox.xMin = content->layerPos.x + frmPos.x + frmBox.xMin;
				content->formLayerLimitSceneBox.xMax = content->layerPos.x + frmPos.x + frmBox.xMax;
				content->formLayerLimitSceneBox.yMin = content->layerPos.y + frmPos.y + frmBox.yMin;
				content->formLayerLimitSceneBox.yMax = content->layerPos.y + frmPos.y + frmBox.yMax;
				//Expand from-glass
				{
					NBCajaAABB localBox;
					localBox.xMin = cajaEscena.xMin - content->layerPos.x - frmPos.x;
					localBox.xMax = cajaEscena.xMax - content->layerPos.x - frmPos.x;
					localBox.yMin = cajaEscena.yMin - content->layerPos.y - frmPos.y;
					localBox.yMax = cajaEscena.yMax - content->layerPos.y - frmPos.y;
					content->formLayerGlass->redimensionar(localBox.xMin - 1.0f, localBox.yMin - 1.0f, (localBox.xMax - localBox.xMin) + 2.0f, (localBox.yMax - localBox.yMin) + 2.0f);
				}
			}
			//Move buttons (if space is available)
			if((sceneDiagInch >= 6.0f && content->base.buttonsPos == ENNBTextAlignV_Count) || content->base.buttonsPos != ENNBTextAlignV_Count){
				const NBPunto pos		= content->layer->traslacion();
				const NBCajaAABB box	= content->layer->cajaAABBLocal();
				const float yBottomForm	= pos.y + box.yMin;
				const float extraHeight	= (yBottomForm - yBtmButtons);
				if(extraHeight > 0.0f){
					const float relDeltaY = (content->base.buttonsPos == ENNBTextAlignV_FromBottom ? 0.0f : content->base.buttonsPos == ENNBTextAlignV_FromTop ? 1.0f : content->base.buttonsPos == ENNBTextAlignV_Center ? 0.5f : 0.75f);
					STSceneInfoContentBase* base = &content->base;
					SI32 i; for(i = ((SI32)base->buttons.use - 1); i >= 0; i--){
						STSceneInfoBtn* btn = NBArray_itmPtrAtIndex(&base->buttons, STSceneInfoBtn, i);
						if(btn->btn != NULL){
							btn->btnPos.y += extraHeight * relDeltaY;
							btn->btn->establecerTraslacion(btn->btnPos.x, btn->btnPos.y);
						}
					}
				}
			}
		}
		//Organize content-layer (adjusted-to-virtual-keyboard state)
		{
			const float marginV		= NBGestorEscena::altoPulgadasAEscena(_iScene, 0.05f);
			const float yMaxCovered = cajaEscena.yMin + _keybHeight + (_keybHeight != 0.0f ? marginV : 0.0f);
			//Move form (if affected by virtual keyboard)
			if(yMaxCovered <= content->formLayerLimitSceneBox.yMin){
				//The form is not touched (return to original position)
				content->formLayerPos	= content->formLayerLimitPos;
				content->formLayer->establecerTraslacion(content->formLayerPos.x, content->formLayerPos.y);
			} else {
				//The form was covered (move to visibble position)
				const float delta = (yMaxCovered - content->formLayerLimitSceneBox.yMin);
				content->formLayerPos.x = content->formLayerLimitPos.x;
				content->formLayerPos.y = content->formLayerLimitPos.y + delta;
				content->formLayer->establecerTraslacion(content->formLayerPos.x, content->formLayerPos.y);
			}
		}
	}
}

void AUSceneIntro::privContentOrganize(STSceneInfoStackItm* itm, const STNBSize availSz, const ENTSScreenOrient orient){
	if(itm != NULL){
		const ENSceneInfoContent uid = itm->content.uid;
		void* data = itm->content.data;
		switch (uid) {
			case ENSceneInfoContent_NotifsAuth:
				{
					STSceneInfoContent_NotifsAuth*	d = (STSceneInfoContent_NotifsAuth*)data;
					//Icon
					float yTop = 0.0f;
					if(d->icon != NULL){
						if(d->icon->visible()){
							const float scale		= d->icon->escalacion().ancho;
							const NBCajaAABB box	= d->icon->cajaAABBLocal();
							d->icon->establecerTraslacion(-(box.xMin * scale) + ((_metrics.formWidth - ((box.xMax - box.xMin) * scale)) * 0.5f), -box.yMax * scale);
							yTop -= ((box.yMax - box.yMin) * scale);
						}
					}
					if(d->explain != NULL){
						d->explainWidth = (_metrics.formWidth * 0.75f);
						d->explain->organizarTexto(d->explainWidth);
						const NBCajaAABB box = d->explain->cajaAABBLocal();
						yTop -= NBGestorEscena::altoPulgadasAEscena(_iScene, 0.05f);
						d->explain->establecerTraslacion(-box.xMin + ((_metrics.formWidth - (box.xMax - box.xMin)) * 0.5f), yTop - box.yMax);
						yTop -= d->explain->fuenteRender()->altoGuiaLineaEscena() * 3.0f;
					}
					d->formLayer->establecerLimites(0, _metrics.formWidth, yTop, 0);
				}
				break;
			case ENSceneInfoContent_TermsAccept:
				{
					STSceneInfoContent_TermsAccept* d = (STSceneInfoContent_TermsAccept*)data;
					const float marginV = NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.10f);
					float yTop = 0.0f;
					//Title text
					/*{
						const float formWidth = _metrics.formWidth;
						d->txtTitle->organizarTexto(formWidth);
						{
							const NBCajaAABB box = d->txtTitle->cajaAABBLocal();
							if(yTop != 0.0f) yTop -= marginV;
							d->txtTitle->establecerTraslacion(0.0f - box.xMin + ((formWidth - (box.xMax - box.xMin)) * 0.5f), yTop - box.yMax);
							yTop -= (box.yMax - box.yMin);
						}
					}*/
					//Explain text Top
					{
						const float formWidth = _metrics.formWidth;
						d->txtExplainTop->organizarTexto(formWidth);
						{
							const NBCajaAABB box = d->txtExplainTop->cajaAABBLocal();
							if(yTop != 0.0f) yTop -= marginV;
							d->txtExplainTop->establecerTraslacion(0.0f - box.xMin + ((formWidth - (box.xMax - box.xMin)) * 0.5f), yTop - box.yMax);
							yTop -= (box.yMax - box.yMin);
						}
					}
					//Terms link
					{
						const NBCajaAABB box = d->linkTerms.txt->cajaAABBLocal();
						const float width = (box.xMax - box.xMin);
						const float xLeft = (_metrics.formWidth - (box.xMax - box.xMin)) * 0.5f;
						if(yTop != 0.0f) yTop -= (marginV * 1.0f);
						d->linkTerms.txt->establecerTraslacion(xLeft - box.xMin, yTop - box.yMax);
						d->linkTerms.line->moverVerticeHacia(0, xLeft, yTop - (box.yMax - box.yMin));
						d->linkTerms.line->moverVerticeHacia(1, xLeft + width, yTop - (box.yMax - box.yMin));
						yTop -= (box.yMax - box.yMin);
					}
					//Privacy link
					{
						const NBCajaAABB box = d->linkPrivacy.txt->cajaAABBLocal();
						const float width = (box.xMax - box.xMin);
						const float xLeft = (_metrics.formWidth - (box.xMax - box.xMin)) * 0.5f;
						if(yTop != 0.0f) yTop -= (marginV * 1.0f);
						d->linkPrivacy.txt->establecerTraslacion(xLeft - box.xMin, yTop - box.yMax);
						d->linkPrivacy.line->moverVerticeHacia(0, xLeft, yTop - (box.yMax - box.yMin));
						d->linkPrivacy.line->moverVerticeHacia(1, xLeft + width, yTop - (box.yMax - box.yMin));
						yTop -= (box.yMax - box.yMin);
					}
				}
				break;
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			default:
				NBASSERT(FALSE) //Unexpected content type
				break;
#			endif
		}
	}
}

void AUSceneIntro::privContentUpdateState(STSceneInfoStackItm* itm, AUEscenaObjeto* obj, const ENSceneInfoUpdateStateType type, const BOOL animate){
	if(itm != NULL){
		const ENSceneInfoContent uid = itm->content.uid;
		void* data = itm->content.data;
		switch (uid) {
			case ENSceneInfoContent_NotifsAuth: break;
			case ENSceneInfoContent_TermsAccept: break;
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			default:
				NBASSERT(FALSE) //Unexpected content type
				break;
#			endif
		}
	}
}

void AUSceneIntro::privContentTick(STSceneInfoStackItm* itm, const float secs){
	if(itm != NULL){
		const ENSceneInfoContent uid = itm->content.uid;
		void* data = itm->content.data;
		switch (uid) {
			case ENSceneInfoContent_NotifsAuth:
				{
					STSceneInfoContent_NotifsAuth* d = (STSceneInfoContent_NotifsAuth*)data;
					//Detect status change
					{
						const ENAppNotifAuthState curState = NBMngrNotifs::getAuthStatus(ENAppNotifAuthQueryMode_CurrentState);
						switch (curState) {
							case ENAppNotifAuthState_Requesting:
							case ENAppNotifAuthState_QueringSettings:
								//Wait
								break;
							default:
								//Request
								{
									if(d->lastStatus == curState){
										//PRINTF_INFO("NotifAuthStatus remained %d.\n", d->lastStatus);
										if(d->requestAuth){
											PRINTF_INFO("NotifAuthStatus requesting auth.\n");
											NBMngrNotifs::getAuthStatus(ENAppNotifAuthQueryMode_UpdateCacheRequestIfNecesary);
											d->requestAuth = FALSE;
										} else {
											NBMngrNotifs::getAuthStatus(ENAppNotifAuthQueryMode_UpdateCache);
										}
									} else {
										PRINTF_INFO("NotifAuthStatus changed from %d to %d.\n", d->lastStatus, curState);
										if(curState == ENAppNotifAuthState_Authorized){
											const char* strLoc = TSClient_getStr(_coreClt, "notifEnabledState", "Enabled");
											d->explain->establecerTexto(strLoc, d->explainWidth);
											if(_listener->scenesStackSize() > 1){
												_listener->loadPreviousScene(ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
											} else {
												_listener->loadLobby(ENSceneAction_ReplaceAll, ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
											}
										}
										d->lastStatus = curState;
									}
								}
								break;
						}
					}
				}
				break;
			case ENSceneInfoContent_TermsAccept: break;
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			default:
				NBASSERT(FALSE) //Unexpected content type
				break;
#			endif
		}
	}
}

void AUSceneIntro::privContentTouched(STSceneInfoStackItm* itm, const AUEscenaObjeto* obj, const NBPunto posActualEscena){
	if(itm != NULL){
		const ENSceneInfoContent uid = itm->content.uid;
		void* data = itm->content.data;
		switch (uid) {
			case ENSceneInfoContent_NotifsAuth: break;
			case ENSceneInfoContent_TermsAccept:
				{
					STSceneInfoContent_TermsAccept* d = (STSceneInfoContent_TermsAccept*)data;
					if(obj == d->linkTerms.txt || obj == d->linkTerms.line){
						NBMngrOSTools::openUrl("https://reminder.thinstream.co/terms.php");
					} else if(obj == d->linkPrivacy.txt || obj == d->linkPrivacy.line){
						NBMngrOSTools::openUrl("https://reminder.thinstream.co/privacypolicy.php");
					}
				}
				break;
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			default:
				NBASSERT(FALSE) //Unexpected content type
				break;
#			endif
		}
	}
}

void AUSceneIntro::privConfirmFlowStart(){
	if(_reqInvitation.verif.requestId == 0 && !_reqInvitation.confirm.isBuilding && _reqInvitation.confirm.requestId == 0){
		BOOL willOverrideOther = FALSE, canHaveMoreCodes = FALSE;
		//Load values
		{
			STNBStorageRecordRead ref = TSClient_getStorageRecordForRead(_coreClt, "client/_root/", "_current", NULL, TSClientRoot_getSharedStructMap());
			if(ref.data != NULL){
				STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
				if(priv != NULL){
					STTSClientRootReq* reqq = &priv->req;
					willOverrideOther	= reqq->willOverrideOther;
					canHaveMoreCodes	= reqq->canHaveMoreCodes;
				}
			}
			TSClient_returnStorageRecordFromRead(_coreClt, &ref);
		}
		//Hide popup
		if(_reqInvitation.confirm.popConfirmRef != NULL){
			_listener->popHide(_reqInvitation.confirm.popConfirmRef);
			_listener->popRelease(_reqInvitation.confirm.popConfirmRef);
			_reqInvitation.confirm.popConfirmRef = NULL;
		}
		//Show pop up
		{
			const char* strLoc0 = TSClient_getStr(_coreClt, "identityConfirmTitle", "Confirm Identity");
			const char* strLoc1 = TSClient_getStr(_coreClt, "identityConfirmBody", "By continuing you are accepting our Terms and Conditions and confirming you are the owner of the invitation.");
			const char* strLoc2 = TSClient_getStr(_coreClt, "identityConfirmOverrideBody", "Note: this account is already linked to a device. After this action the previous link will be removed.");
			const char* strLoc3 = TSClient_getStr(_coreClt, "identityConfirmOkBtn", "Confirm");
			const char* strLoc4 = TSClient_getStr(_coreClt, "identityConfirmHideBtn", "No, Take me back");
			{
				STNBString str;
				NBString_init(&str);
				NBString_concat(&str, strLoc1);
				if(willOverrideOther){
					NBString_concat(&str, "\n\n");
					NBString_concat(&str, strLoc2);
				}
				{
					_reqInvitation.confirm.popConfirmRef = _listener->popCreate(ENScenePopMode_TextOnly, ENNBTextLineAlignH_Center, strLoc0, str.str, this, this, this, strLoc4, ENScenePopBtnColor_Clear);
					_listener->popAddOption(_reqInvitation.confirm.popConfirmRef, "continue", strLoc3, ENScenePopBtnColor_Main, ENIMsgboxBtnAction_AutohideAndNotify);
					_listener->popShow(_reqInvitation.confirm.popConfirmRef);
				}
				NBString_release(&str);
			}
		}
	}
}

void AUSceneIntro::privConfirmFlowConfirmed(){
	if(_reqInvitation.verif.requestId == 0 && !_reqInvitation.confirm.isBuilding && _reqInvitation.confirm.requestId == 0){
		if(TSClient_isDeviceKeySet(_coreClt)){
			//I already have a device-certificate, send the request
			this->privConfirmReqStart(NULL, 0, NULL, 0);
		} else {
			//I dont have a device-certificate, build a CSR first
			if(_reqInvitation.confirm.pkey != NULL){
				NBPKey_release(_reqInvitation.confirm.pkey);
				NBMemory_free(_reqInvitation.confirm.pkey);
				_reqInvitation.confirm.pkey = NULL;
			}
			if(_reqInvitation.confirm.csr != NULL){
				NBX509Req_release(_reqInvitation.confirm.csr);
				NBMemory_free(_reqInvitation.confirm.csr);
				_reqInvitation.confirm.csr = NULL;
			}
			//
			_reqInvitation.confirm.isBuilding		= FALSE;
			_reqInvitation.confirm.isBuildingLast	= FALSE;
			_reqInvitation.confirm.pkey = NBMemory_allocType(STNBPKey);
			_reqInvitation.confirm.csr	= NBMemory_allocType(STNBX509Req);
			NBPKey_init(_reqInvitation.confirm.pkey);
			NBX509Req_init(_reqInvitation.confirm.csr);
			if(!TSClient_asyncBuildPkeyAndCsr(_coreClt, &_reqInvitation.confirm.pkey, &_reqInvitation.confirm.csr, &_reqInvitation.confirm.isBuilding)){
				//Error
				this->privConfirmBuildingErrorPopShow();
				//
				_reqInvitation.confirm.isBuilding		= FALSE;
				_reqInvitation.confirm.isBuildingLast	= FALSE;
				//Failed
				NBPKey_release(_reqInvitation.confirm.pkey);
				NBMemory_free(_reqInvitation.confirm.pkey);
				_reqInvitation.confirm.pkey = NULL;
				//
				NBX509Req_release(_reqInvitation.confirm.csr);
				NBMemory_free(_reqInvitation.confirm.csr);
				_reqInvitation.confirm.csr = NULL;
			} else {
				//Building
				this->privConfirmBuildingPopShow();
			}
		}
	}
}

void AUSceneIntro::privConfirmBuildingPopShow(){
	const char* strLoc0 = TSClient_getStr(_coreClt, "deviceBuildingTitle", "Building keys");
	const char* strLoc1 = TSClient_getStr(_coreClt, "deviceBuildingBody", "Building your local encryption keys. Please wait, this may takes many seconds.");
	const char* strLoc2 = TSClient_getStr(_coreClt, "deviceBuildingHideBtn", "Hide");
	if(_reqInvitation.confirm.popBuildingRef != NULL){
		_listener->popHide(_reqInvitation.confirm.popBuildingRef);
		_listener->popRelease(_reqInvitation.confirm.popBuildingRef);
		_reqInvitation.confirm.popBuildingRef = NULL;
	}
	_reqInvitation.confirm.popBuildingRef = _listener->popCreate(ENScenePopMode_Working, ENNBTextLineAlignH_Center, strLoc0, strLoc1, this, this, this, strLoc2, ENScenePopBtnColor_Clear);
	_listener->popShow(_reqInvitation.confirm.popBuildingRef);
}

void AUSceneIntro::privConfirmBuildingErrorPopShow(){
	const char* strLoc0 = TSClient_getStr(_coreClt, "deviceBuildingErrTitle", "Error");
	const char* strLoc1 = TSClient_getStr(_coreClt, "deviceBuildingErrBody", "Something happened while building your local encryption keys. Please try again and contact us for support.");
	const char* strLoc2 = TSClient_getStr(_coreClt, "deviceBuildingHideBtn", "Hide");
	if(_reqInvitation.confirm.popBuildingRef != NULL){
		_listener->popHide(_reqInvitation.confirm.popBuildingRef);
		_listener->popRelease(_reqInvitation.confirm.popBuildingRef);
		_reqInvitation.confirm.popBuildingRef = NULL;
	}
	_reqInvitation.confirm.popBuildingRef = _listener->popCreate(ENScenePopMode_Error, ENNBTextLineAlignH_Center, strLoc0, strLoc1, this, this, this, strLoc2, ENScenePopBtnColor_Clear);
	_listener->popShow(_reqInvitation.confirm.popBuildingRef);
}


void AUSceneIntro::privContentAction(STSceneInfoStackItm* itm, const char* optionId){
	if(itm != NULL){
		const ENSceneInfoContent uid = itm->content.uid;
		void* data = itm->content.data;
		switch (uid) {
			case ENSceneInfoContent_NotifsAuth:
				{
					STSceneInfoContent_NotifsAuth* d = (STSceneInfoContent_NotifsAuth*)data;
					if(NBString_strIsEqual(optionId, "continue")){
						if(!this->privContentCurrentIsLast()){
							this->privContentShowNext(TRUE);
						} else {
							if(_listener->scenesStackSize() > 1){
								_listener->loadPreviousScene(ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
							} else {
								_listener->loadLobby(ENSceneAction_ReplaceAll, ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
							}
						}
					} else if(NBString_strIsEqual(optionId, "skip")){
						if(!this->privContentCurrentIsLast()){
							this->privContentShowNext(TRUE);
						} else {
							if(_listener->scenesStackSize() > 1){
								_listener->loadPreviousScene(ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
							} else {
								_listener->loadLobby(ENSceneAction_ReplaceAll, ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
							}
						}
					} else if(NBString_strIsEqual(optionId, "enable")){
						NBMngrNotifs::localEnable();
						d->requestAuth = TRUE;
					} else {
						NBASSERT(FALSE) //unexpected optionId
					}
				}
				break;
			case ENSceneInfoContent_TermsAccept:
				{
					//STSceneInfoContent_TermsAccept* d = (STSceneInfoContent_TermsAccept*)data;
					if(NBString_strIsEqual(optionId, "continue")){
						this->AUSceneIntro::privConfirmFlowStart();
					} else if(NBString_strIsEqual(optionId, "cancel")){
						//Remove verifSeqId (restart sequence)
						{
							STNBStorageRecordWrite ref = TSClient_getStorageRecordForWrite(_coreClt, "client/_root/", "_current", FALSE, NULL, TSClientRoot_getSharedStructMap());
							if(ref.data != NULL){
								STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
								if(priv != NULL){
									STTSClientRootReq* reqq = &priv->req;
									//verifSeqId
									if(reqq->verifSeqId != NULL){
										NBMemory_free(reqq->verifSeqId);
										reqq->verifSeqId = NULL;
										ref.privModified = TRUE;
									}
								}
							}
							TSClient_returnStorageRecordFromWrite(_coreClt, &ref);	
						}
						//Go back
						this->privContentShowPrev(1, TRUE, TRUE);
					} else {
						NBASSERT(FALSE) //unexpected optionId
					}
				}
				break;
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			default:
				NBASSERT(FALSE) //Unexpected content type
				break;
#			endif
		}
	}
}

void AUSceneIntro::privContentPopOptionSelected(STSceneInfoStackItm* itm, void* popRef, const char* optionId){
	/*if(itm != NULL){
		const ENSceneInfoContent uid = itm->content.uid;
		void* data = itm->content.data;
		switch (uid) {
			case ENSceneInfoContent_NotifsAuth: break;
			case ENSceneInfoContent_TermsAccept: break;
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			default:
				NBASSERT(FALSE) //Unexpected content type
				break;
#			endif
		}
	}*/
}

void AUSceneIntro::privContentRecordChanged(STSceneInfoStackItm* itm){
	if(itm != NULL){
		const ENSceneInfoContent uid = itm->content.uid;
		void* data = itm->content.data;
		switch (uid) {
			case ENSceneInfoContent_NotifsAuth: break;
			case ENSceneInfoContent_TermsAccept:
				{
					//STSceneInfoContent_TermsAccept* d = (STSceneInfoContent_TermsAccept*)data;
					BOOL newAccountLinked = FALSE;
					{
						STNBStorageRecordWrite ref = TSClient_getStorageRecordForWrite(_coreClt, "client/_root/", "_current", FALSE, NULL, TSClientRoot_getSharedStructMap());
						if(ref.data != NULL){
							STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
							if(priv != NULL){
								STTSClientRootReq* reqq = &priv->req;
								if(reqq->linkedToClinic){
									newAccountLinked		= TRUE;
									reqq->linkedToClinic	= FALSE; //clear flag
									reqq->isAdmin			= FALSE;
									ref.privModified		= TRUE;
								}
							}
						}
						TSClient_returnStorageRecordFromWrite(_coreClt, &ref);
					}
					//Advance next step
					if(newAccountLinked){
						if(NBMngrNotifs::getAuthStatus(ENAppNotifAuthQueryMode_CacheOnly) == ENAppNotifAuthState_Authorized){
							if(_listener->scenesStackSize() > 1){
								_listener->loadPreviousScene(ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
							} else {
								_listener->loadLobby(ENSceneAction_ReplaceAll, ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
							}
						} else {
							this->privContentPushNewStack(ENSceneInfoContent_NotifsAuth, TRUE);
						}
					}
				}
				break;
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			default:
				NBASSERT(FALSE) //Unexpected content type
				break;
#			endif
		}
	}
}

void AUSceneIntro::privContentShow(const ENSceneInfoContent uid, void* data, const float secsWait, const float secsAnim){
	{
		switch (uid) {
			case ENSceneInfoContent_NotifsAuth:
				{
					STTSContext* context = TSClient_getContext(_coreClt);
					STNBStorageRecordWrite ref = TSContext_getStorageRecordForWrite(context, "client/_device/", "_current", TRUE, NULL, TSDeviceLocal_getSharedStructMap());
					if(ref.data != NULL){
						STTSDeviceLocal* priv = (STTSDeviceLocal*)ref.data->priv.stData;
						if(priv != NULL){
							if(!priv->authNotifsRequested){
								priv->authNotifsRequested = TRUE;
								ref.privModified = TRUE;
							}
						}
					}
					TSContext_returnStorageRecordFromWrite(context, &ref);
				}
				break;
			case ENSceneInfoContent_TermsAccept: break;
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			default:
				NBASSERT(FALSE) //Unexpected content type
				break;
#			endif
		}
	}
}

//

void AUSceneIntro::privContentHide(const ENSceneInfoContent uid, void* data, const float secsWait, const float secsAnim){
	{
		switch (uid) {
			case ENSceneInfoContent_NotifsAuth: break;
			case ENSceneInfoContent_TermsAccept: break;
#			ifdef NB_CONFIG_INCLUDE_ASSERTS
			default:
				NBASSERT(FALSE) //Unexpected content type
				break;
#			endif
		}
	}
}

void AUSceneIntro::privContentShow(STSceneInfoStackItm* itm, const float secsAnim, const float secsWait){
	STSceneInfoContentAnim anim;
	{
		anim.stackItm	= itm;
		anim.animator	= new(this) AUAnimadorObjetoEscena();
		anim.isOutAnim	= FALSE;
	}
	STSceneInfoContent* content = &itm->content;
	STSceneInfoContentBase* base = &content->base;
	const float deltaH = NBGestorEscena::altoPulgadasAEscena(_iScene, 0.2f);
	//Show-Title
	if(base->title != NULL){
		base->title->establecerTraslacion(base->titlePos.x, base->titlePos.y + (deltaH * 3.0f));
		base->title->establecerVisible(false);
		base->title->establecerMultiplicadorAlpha8(255);
		anim.animator->animarPosicion(base->title, base->titlePos.x, base->titlePos.y, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
		anim.animator->animarVisible(base->title, true, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
	}
	//Show-Explain
	if(base->explainTop != NULL){
		base->explainTop->establecerTraslacion(base->explainTopPos.x, base->explainTopPos.y + deltaH);
		base->explainTop->establecerVisible(false);
		base->explainTop->establecerMultiplicadorAlpha8(255);
		anim.animator->animarPosicion(base->explainTop, base->explainTopPos.x, base->explainTopPos.y, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
		anim.animator->animarVisible(base->explainTop, true, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
	}
	//Show privacy link
	if(base->privacyLnk != NULL){
		base->privacyLnk->establecerTraslacion(base->privacyLnkPos.x, base->privacyLnkPos.y - deltaH);
		base->privacyLnk->establecerVisible(false);
		base->privacyLnk->establecerMultiplicadorAlpha8(255);
		anim.animator->animarPosicion(base->privacyLnk, base->privacyLnkPos.x, base->privacyLnkPos.y, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
		anim.animator->animarVisible(base->privacyLnk, true, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
	}
	if(base->privacyLnkIco != NULL){
		base->privacyLnkIco->establecerTraslacion(base->privacyLnkIcoPos.x, base->privacyLnkIcoPos.y - deltaH);
		base->privacyLnkIco->establecerVisible(false);
		base->privacyLnkIco->establecerMultiplicadorAlpha8(255);
		anim.animator->animarPosicion(base->privacyLnkIco, base->privacyLnkIcoPos.x, base->privacyLnkIcoPos.y, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
		anim.animator->animarVisible(base->privacyLnkIco, true, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
	}
	if(base->privacyLnkLine != NULL){
		base->privacyLnkLine->establecerTraslacion(base->privacyLnkLinePos.x, base->privacyLnkLinePos.y - deltaH);
		base->privacyLnkLine->establecerVisible(false);
		base->privacyLnkLine->establecerMultiplicadorAlpha8(255);
		anim.animator->animarPosicion(base->privacyLnkLine, base->privacyLnkLinePos.x, base->privacyLnkLinePos.y, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
		anim.animator->animarVisible(base->privacyLnkLine, true, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
	}
	//Show-buttons
	{
		
		SI32 i; for(i = ((SI32)base->buttons.use - 1); i >= 0; i--){
			STSceneInfoBtn* btn = NBArray_itmPtrAtIndex(&base->buttons, STSceneInfoBtn, i);
			btn->btn->establecerTraslacion(btn->btnPos.x, btn->btnPos.y - deltaH);
			btn->btn->establecerVisible(false);
			btn->btn->establecerMultiplicadorAlpha8(255);
			anim.animator->animarPosicion(btn->btn, btn->btnPos.x, btn->btnPos.y, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
			if(btn->visibility == ENSceneInfoBtnVisibility_Allways){
				anim.animator->animarVisible(btn->btn, true, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
			}
		}
	}
	//Show form
	{
		content->formLayer->establecerVisible(false);
		content->formLayer->establecerMultiplicadorAlpha8(255);
		anim.animator->animarVisible(content->formLayer, true, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
	}
	//Custom animation
	this->privContentShow(content->uid, content->data, secsWait, secsAnim);
	//Add layers
	{
		if(content->layer->contenedor() != _contents.lyr){
			_contents.lyr->agregarObjetoEscena(content->layer);
		}
		if(content->btnsLayer->contenedor() != _contents.lyr){
			_contents.lyr->agregarObjetoEscena(content->btnsLayer);
		}
	}
	//Add animation
	NBArray_addValue(&_contents.anims, anim);
	//PRINTF_INFO("Animations total: %d.\n", _contents.anims.use);
}

void AUSceneIntro::privContentHide(STSceneInfoStackItm* itm, const float secsAnim, const float secsWait){
	STSceneInfoContentAnim anim;
	{
		anim.stackItm	= itm;
		anim.animator	= new(this) AUAnimadorObjetoEscena();
		anim.isOutAnim	= TRUE;
	}
	STSceneInfoContent* content = &itm->content;
	STSceneInfoContentBase* base = &content->base;
	const float deltaH = NBGestorEscena::altoPulgadasAEscena(_iScene, 0.2f);
	//Hide-Title
	if(base->title != NULL){
		anim.animator->animarPosicion(base->title, base->titlePos.x, base->titlePos.y + (deltaH * 3.0f), secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
		anim.animator->animarVisible(base->title, false, secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
	}
	//Hide-Explain
	if(base->explainTop != NULL){
		anim.animator->animarPosicion(base->explainTop, base->explainTopPos.x, base->explainTopPos.y + deltaH, secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
		anim.animator->animarVisible(base->explainTop, false, secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
	}
	//Hide privacy link
	if(base->privacyLnk != NULL){
		anim.animator->animarPosicion(base->privacyLnk, base->privacyLnkPos.x, base->privacyLnkPos.y - deltaH, secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
		anim.animator->animarVisible(base->privacyLnk, false, secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
	}
	if(base->privacyLnkIco != NULL){
		anim.animator->animarPosicion(base->privacyLnkIco, base->privacyLnkIcoPos.x, base->privacyLnkIcoPos.y - deltaH, secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
		anim.animator->animarVisible(base->privacyLnkIco, false, secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
	}
	if(base->privacyLnkLine != NULL){
		anim.animator->animarPosicion(base->privacyLnkLine, base->privacyLnkLinePos.x, base->privacyLnkLinePos.y - deltaH, secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
		anim.animator->animarVisible(base->privacyLnkLine, false, secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
	}
	//Hide-buttons
	{
		
		SI32 i; for(i = ((SI32)base->buttons.use - 1); i >= 0; i--){
			STSceneInfoBtn* btn = NBArray_itmPtrAtIndex(&base->buttons, STSceneInfoBtn, i);
			anim.animator->animarPosicion(btn->btn, btn->btnPos.x, btn->btnPos.y - deltaH, secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
			//if(btn->visibility == ENSceneInfoBtnVisibility_Allways){ //ToDo: determine if this must be evaluated
			anim.animator->animarVisible(btn->btn, false, secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
			//}
		}
	}
	//Hide form
	{
		anim.animator->animarVisible(content->formLayer, false, secsAnim, ENAnimPropVelocidad_Acelerada3, secsWait);
	}
	//Custom animation
	this->privContentHide(content->uid, content->data, secsWait, secsAnim);
	//Add animation
	NBArray_addValue(&_contents.anims, anim);
	//PRINTF_INFO("ANimations total: %d.\n", _contents.anims.use);
}

void AUSceneIntro::privContentExchangeFromTo(STSceneInfoStackItm* curItm, STSceneInfoStackItm* newItm){
	float secsWait			= 0.0f;
	const float secsAnim	= 0.25f;
	const float deltaW		= NBGestorEscena::anchoPulgadasAEscena(_iScene, 0.2f);
	BOOL showedBg = TRUE, showBg = TRUE;
	BOOL showedTopLogo = FALSE, showTopLogo = FALSE;
	BOOL showedBack = FALSE, showBack = FALSE;
	BOOL showedSteps = FALSE, showSteps = _contents.mainStack->stepsIndicators.layerVisible;
	BOOL showedResetDataBtn = FALSE, showResetDataBtn = FALSE;
	STNBColor8 bgColored = TSColors::colorDef(ENTSColor_Light)->normal, bgColor = TSColors::colorDef(ENTSColor_Light)->normal;
	STNBColor8 backColored = TSColors::colorDef(ENTSColor_Black)->normal, backColor = TSColors::colorDef(ENTSColor_Black)->normal;
	//Hide previous
	if(curItm != NULL){
		STSceneInfoContent* content = &curItm->content;
		this->privContentHide(curItm, secsAnim, secsWait);
		showedTopLogo	= content->base.showTopLogo;
		showedBack		= (content->base.backColor < ENTSColor_Count /*&& curIdx > 0*/);
		backColored		= TSColors::colorDef(content->base.backColor)->normal;
		showedBg		= (content->base.bgColor < ENTSColor_Count);
		bgColored		= TSColors::colorDef(content->base.bgColor)->normal;
		secsWait		+= secsAnim;
		showedSteps		= (content->base.showSteps && _contents.mainStack->stepsIndicators.layerVisible);
		showedResetDataBtn = content->base.showResetDataBtn;
	}
	//Show new
	if(newItm != NULL){
		STSceneInfoContent* content = &newItm->content;
		this->privContentUpdateState(newItm, NULL, ENSceneInfoUpdateStateType_Organize, FALSE);
		this->privContentShow(newItm, secsAnim, secsWait);
		showTopLogo		= content->base.showTopLogo;
		showBack		= (content->base.backColor < ENTSColor_Count /*&& newIdx > 0*/);
		backColor		= TSColors::colorDef(content->base.backColor)->normal;
		showBg			= (content->base.bgColor < ENTSColor_Count);
		bgColor			= TSColors::colorDef(content->base.bgColor)->normal;
		showSteps		= (content->base.showSteps && _contents.mainStack->stepsIndicators.layerVisible);
		showResetDataBtn = content->base.showResetDataBtn;
	}
	//Iterface objects
	{
		_back.layerColor = backColor;
		//Bg
		{
			if(showedBg != showBg){
				if(showBg){
					//Show-back btn
					_bg->establecerVisible(false);
					_bg->establecerMultiplicadorColor8(bgColor.r, bgColor.g, bgColor.b, bgColor.a);
					_animator->animarVisible(_bg, true, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
				} else {
					//Hide-back btn
					_animator->animarVisible(_bg, false, secsAnim, ENAnimPropVelocidad_Acelerada3, 0.0f);
				}
			} else {
				//Change color
				if(bgColored.r != bgColor.r || bgColored.g != bgColor.g || bgColored.b != bgColor.b || bgColored.a != bgColor.a){
					_animator->animarColorMult(_bg, bgColor.r, bgColor.g, bgColor.b, bgColor.a, secsAnim, ENAnimPropVelocidad_Acelerada3, 0.0f);
				}
			}
			//Update OS-bar style
			{
				const SI32 colorAvg = ((SI32)bgColor.r + (SI32)bgColor.g + (SI32)bgColor.b) / 3;
				if(showBg){
					if(colorAvg < 100){
						_barStyle = ENStatusBarStyle_Light;
					} else {
						_barStyle = ENStatusBarStyle_Dark;
					}
					if(_atFocus){
						NBMngrOSTools::setBarStyle(_barStyle);
					}
				}
			}
		}
		//Back-btn
		{
			if(showedBack != showBack){
				if(showBack){
					//Show-back btn
					_back.layer->establecerVisible(false);
					_back.layer->establecerMultiplicadorColor8(backColor.r, backColor.g, backColor.b, backColor.a);
					_back.layer->establecerTraslacion(_back.layerPos.x + deltaW, _back.layerPos.y);
					_animator->animarPosicion(_back.layer, _back.layerPos.x, _back.layerPos.y, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
					_animator->animarVisible(_back.layer, true, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
				} else {
					//Hide-back btn
					_animator->animarPosicion(_back.layer, _back.layerPos.x - deltaW, _back.layerPos.y, secsAnim, ENAnimPropVelocidad_Acelerada3, 0.0f);
					_animator->animarVisible(_back.layer, false, secsAnim, ENAnimPropVelocidad_Acelerada3, 0.0f);
				}
			} else {
				//Change color
				if(backColored.r != backColor.r || backColored.g != backColor.g || backColored.b != backColor.b || backColored.a != backColor.a){
					_animator->animarColorMult(_back.layer, backColor.r, backColor.g, backColor.b, backColor.a, secsAnim, ENAnimPropVelocidad_Acelerada3, 0.0f);
				}
			}
		}
		//TopLogo
		{
			if(showedTopLogo != showTopLogo){
				if(showTopLogo){
					//Show-topLogo
					_topLogo.sprite->establecerVisible(false);
					_topLogo.sprite->establecerMultiplicadorAlpha8(255);
					_topLogo.sprite->establecerTraslacion(_topLogo.spritePos.x - deltaW, _topLogo.spritePos.y);
					_animator->animarPosicion(_topLogo.sprite, _topLogo.spritePos.x, _topLogo.spritePos.y, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
					_animator->animarVisible(_topLogo.sprite, true, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
				} else {
					//Hide-topLogo
					_animator->animarPosicion(_topLogo.sprite, _topLogo.spritePos.x + deltaW, _topLogo.spritePos.y, secsAnim, ENAnimPropVelocidad_Acelerada3, 0.0f);
					_animator->animarVisible(_topLogo.sprite, false, secsAnim, ENAnimPropVelocidad_Acelerada3, 0.0f);
				}
			}
		}
		//StepsIndicators
		{
			//Visibility
			if(showedSteps != showSteps){
				STSceneInfoStepsInds* steps = &_contents.mainStack->stepsIndicators;
				if(showSteps){
					//Show-steps
					steps->layer->establecerVisible(false);
					steps->layer->establecerMultiplicadorAlpha8(255);
					steps->layer->establecerTraslacion(steps->layerPos.x, steps->layerPos.y - deltaW);
					_animator->animarPosicion(steps->layer, steps->layerPos.x, steps->layerPos.y, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
					_animator->animarVisible(steps->layer, true, secsAnim, ENAnimPropVelocidad_Desacelerada3, secsWait);
				} else {
					//Hide-steps
					_animator->animarPosicion(steps->layer, steps->layerPos.x, steps->layerPos.y - deltaW, secsAnim, ENAnimPropVelocidad_Acelerada3, 0.0f);
					_animator->animarVisible(steps->layer, false, secsAnim, ENAnimPropVelocidad_Acelerada3, 0.0f);
				}
			}
			//Indicators colors
			if(_contents.mainStack->stepsIndicators.inds.use > 0){
				STSceneInfoStepsInds* steps = &_contents.mainStack->stepsIndicators;
				const SI32 mainStackIdx = _contents.mainStack->iCurStep; NBASSERT(mainStackIdx >= 0 && mainStackIdx < _contents.mainStack->stepsSequence.use)
				SI32 iStepNext = _contents.mainStack->stepsSequence.use;
				SI32 i; for(i = (steps->inds.use - 1); i >= 0; i--){
					STSceneInfoStepInd* ind = NBArray_itmPtrAtIndex(&steps->inds, STSceneInfoStepInd, i);
					NBASSERT(ind->iStep < iStepNext)
					SI32 iStepPrev = -1;
					if(i > 0){
						iStepPrev = NBArray_itmPtrAtIndex(&steps->inds, STSceneInfoStepInd, i - 1)->iStep;
					}
					NBColor8 colorC, colorL;
					if(iStepNext <= mainStackIdx){
						colorC.r = colorC.g = colorC.b = 0; colorC.a = 255; //Black circle
						colorL = colorC; //Black line
					} else if(iStepPrev < mainStackIdx){
						colorC.r = 217; colorC.g = 62; colorC.b = 43; colorC.a = 255; //Red circle
						colorL.r = colorL.g = colorL.b = 200; colorL.a = 255; //Gray line
					} else {
						colorC.r = colorC.g = colorC.b = 200; colorC.a = 255; //Gray circle
						colorL = colorC; //Gray line
					}
					//
					if(ind->circle != NULL) ind->circle->establecerMultiplicadorColor8(colorC);
					if(ind->text != NULL) ind->text->establecerMultiplicadorColor8(colorC);
					if(ind->lineToNext != NULL) ind->lineToNext->establecerMultiplicadorColor8(colorL);
					//
					iStepNext = ind->iStep;
				}
			}
		}
		//Reset data btn
		{
			if(_resetData.btn != NULL){
				_resetData.btn->setVisibleAndAwake(showResetDataBtn);
			}
		}
	}
}

void AUSceneIntro::privContentPushNewStack(const ENSceneInfoContent stepId, const BOOL ignoreIfAnimating){
	this->privContentPushNewStack(&stepId, 1, ignoreIfAnimating);
}

void AUSceneIntro::privContentPushNewStack(const ENSceneInfoContent* stepsId, const SI32 stepsIdSz, const BOOL ignoreIfAnimating){
	if(_contents.stacks.use > 0 && (!ignoreIfAnimating || this->_contents.anims.use == 0) && stepsIdSz > 0){
		STSceneInfoStack* curStack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, _contents.stacks.use - 1);
		if(curStack->contentStack.use > 0){
			STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&curStack->contentStack, STSceneInfoStackItm*, curStack->contentStack.use - 1);
			NBASSERT(itm->subStack == NULL)
			if(itm->subStack == NULL){
				PRINTF_INFO("Pushing substack with %d steps.\n", stepsIdSz);
				STSceneInfoStack* subStack	= NBMemory_allocType(STSceneInfoStack);
				SceneInfoStack_init(subStack);
				{
					AUFuenteRender* fntStepName	= TSFonts::font(ENTSFont_ContentMid);
					SI32 i; for(i = 0 ; i < stepsIdSz; i++){
						this->privAddStepToSequence(subStack, stepsId[i], &_contents.templatesStack, fntStepName);
					}
				}
				itm->subStack = subStack;
				//Push substack
				NBArray_addValue(&_contents.stacks, subStack);
				//Show content
				{
					STSceneInfoStackItm* curItm = NULL;
					STSceneInfoStackItm* newItm = NULL;
					if(curStack->contentStack.use > 0){
						curItm = NBArray_itmValueAtIndex(&curStack->contentStack, STSceneInfoStackItm*, curStack->contentStack.use - 1);
					}
					{
						this->privStackPushContent(subStack, stepsId[0]);
						NBASSERT(subStack->contentStack.use == 1 && subStack->iCurStep == -1)
						subStack->iCurStep++;
						newItm = NBArray_itmValueAtIndex(&subStack->contentStack, STSceneInfoStackItm*, subStack->contentStack.use - 1);
						this->privContentOrganizeItm(newItm);
					}
					NBASSERT(curItm != NULL && newItm != NULL)
					this->privContentExchangeFromTo(curItm, newItm);
				}
			}
		}
	}
}

void AUSceneIntro::privContentPopCurStack(const BOOL ignoreIfAnimating){
	if(!ignoreIfAnimating || this->_contents.anims.use == 0){
		if(_contents.stacks.use <= 1){
			//Pop to previous scene
			if(_listener != NULL){
				_listener->loadPreviousScene(ENSceneTransitionType_White, TS_TRANSITION_PARTIAL_ANIM_DURATION);
			}
		} else {
			STSceneInfoStack* curStack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, _contents.stacks.use - 1);
			STSceneInfoStack* prevStack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, _contents.stacks.use - 2);
			NBASSERT((curStack->iCurStep + 1) == curStack->contentStack.use)
			if(prevStack->contentStack.use > 0){
				STSceneInfoStackItm* curItm = NULL;
				STSceneInfoStackItm* newItm = NULL;
				if(curStack->contentStack.use > 0){
					curItm = NBArray_itmValueAtIndex(&curStack->contentStack, STSceneInfoStackItm*, curStack->contentStack.use - 1);
					//Remove from stack
					NBArray_removeItemAtIndex(&curStack->contentStack, curStack->contentStack.use - 1);
					curStack->iCurStep--;
				}
				{
					PRINTF_INFO("Moving to prev-stack (explicit pop).\n");
					//Go back to parent stack
					NBASSERT(curItm != NULL)
					if(prevStack->contentStack.use > 0){
						newItm = NBArray_itmValueAtIndex(&prevStack->contentStack, STSceneInfoStackItm*, prevStack->contentStack.use - 1);
						NBASSERT(newItm->subStack == curStack)
						newItm->subStack = NULL;
					}
					//Remove and release
					NBArray_removeItemAtIndex(&_contents.stacks, _contents.stacks.use - 1);
					//SceneInfoStack_release(curStack); //ToDo:
					//
					curStack = prevStack;
				}
				this->privContentExchangeFromTo(curItm, newItm);
			}
		}
	}
}

void AUSceneIntro::privContentPopCurStackAndNext(const BOOL ignoreIfAnimating){
	if(!ignoreIfAnimating || this->_contents.anims.use == 0){
		if(_contents.stacks.use <= 1){
			//Pop to previous scene
			if(_listener != NULL){
				_listener->loadPreviousScene(ENSceneTransitionType_White, TS_TRANSITION_PARTIAL_ANIM_DURATION);
			}
		} else {
			STSceneInfoStack* curStack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, _contents.stacks.use - 1);
			STSceneInfoStack* prevStack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, _contents.stacks.use - 2);
			NBASSERT((curStack->iCurStep + 1) == curStack->contentStack.use)
			if(prevStack->contentStack.use > 0){
				STSceneInfoStackItm* curItm = NULL;
				STSceneInfoStackItm* newItm = NULL;
				if(curStack->contentStack.use > 0){
					curItm = NBArray_itmValueAtIndex(&curStack->contentStack, STSceneInfoStackItm*, curStack->contentStack.use - 1);
					//Remove from stack
					NBArray_removeItemAtIndex(&curStack->contentStack, curStack->contentStack.use - 1);
					curStack->iCurStep--;
				}
				{
					PRINTF_INFO("Moving to prev-stack (explicit pop).\n");
					//Go back to parent stack (and load next content)
					NBASSERT(curItm != NULL)
					const UI32 stackUseBefore = prevStack->contentStack.use;
					//Unlink-subStack
					if(prevStack->contentStack.use > 0){
						STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&prevStack->contentStack, STSceneInfoStackItm*, prevStack->contentStack.use - 1);
						NBASSERT(itm->subStack == curStack)
						itm->subStack = NULL;
					}
					//Determine new item
					if((prevStack->iCurStep + 1) >= 0 && (prevStack->iCurStep + 1) < prevStack->stepsSequence.use){
						prevStack->iCurStep++;
						const ENSceneInfoContent uid = NBArray_itmValueAtIndex(&prevStack->stepsSequence, ENSceneInfoContent, prevStack->iCurStep);
						this->privStackPushContent(prevStack, uid);
						NBASSERT((stackUseBefore + 1) == prevStack->contentStack.use)
						newItm = NBArray_itmValueAtIndex(&prevStack->contentStack, STSceneInfoStackItm*, prevStack->contentStack.use - 1);
						this->privContentOrganizeItm(newItm);
						NBASSERT(newItm->subStack == NULL)
					} else if(prevStack->contentStack.use > 0){
						newItm = NBArray_itmValueAtIndex(&prevStack->contentStack, STSceneInfoStackItm*, prevStack->contentStack.use - 1);
						NBASSERT(newItm->subStack == NULL)
					}
					//Remove and release
					NBArray_removeItemAtIndex(&_contents.stacks, _contents.stacks.use - 1);
					//SceneInfoStack_release(curStack); //ToDo:
					//
					curStack = prevStack;
				}
				this->privContentExchangeFromTo(curItm, newItm);
			}
		}
	}
}

//

void AUSceneIntro::privContentShowPrev(const UI8 stepsBack, const BOOL ignoreIfAnimating, const BOOL ignoreIfBuildingId){
	if(ignoreIfBuildingId && (_reqInvitation.verif.requestId != 0 || _reqInvitation.confirm.isBuilding || _reqInvitation.confirm.requestId != 0)){
		//Is building (ignore action)
	} else if(stepsBack > 0 && _contents.stacks.use > 0 && (!ignoreIfAnimating || this->_contents.anims.use == 0)){
		STSceneInfoStack* curStack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, _contents.stacks.use - 1);
		NBASSERT((curStack->iCurStep + 1) == curStack->contentStack.use)
		if(curStack->contentStack.use > 0){
			BOOL willEmptyAll = FALSE;
			if(curStack->contentStack.use <= stepsBack){
				//Will flush the current atack
				if(_contents.stacks.use <= 1){
					//Will flush the only stack
					willEmptyAll = TRUE;
				}
			}
			if(willEmptyAll){
				//Pop to previous scene
				if(_listener != NULL){
					_listener->loadPreviousScene(ENSceneTransitionType_White, TS_TRANSITION_PARTIAL_ANIM_DURATION);
				}
			} else {
				STSceneInfoStackItm* curItm = NULL;
				STSceneInfoStackItm* newItm = NULL;
				if(curStack->contentStack.use > 0){
					curItm = NBArray_itmValueAtIndex(&curStack->contentStack, STSceneInfoStackItm*, curStack->contentStack.use - 1);
					//Remove from stack
					NBArray_removeItemAtIndex(&curStack->contentStack, curStack->contentStack.use - 1);
					curStack->iCurStep -= stepsBack;
				}
				if((curStack->contentStack.use + 1) > stepsBack){
					PRINTF_INFO("Moving to prev-content.\n");
					//Go back n-steps
					newItm = NBArray_itmValueAtIndex(&curStack->contentStack, STSceneInfoStackItm*, curStack->contentStack.use - stepsBack);
					//Remove jumped steps
					{
						UI8 i = 1;
						while(i < stepsBack){
							NBASSERT(curStack->contentStack.use > 0)
							STSceneInfoStackItm* jumpedItm = NBArray_itmValueAtIndex(&curStack->contentStack, STSceneInfoStackItm*, curStack->contentStack.use - 1);
							SceneInfoStackItm_release(jumpedItm, this);
							NBArray_removeItemAtIndex(&curStack->contentStack, curStack->contentStack.use - 1);
							i++;
						}
					}
				} else {
					PRINTF_INFO("Moving to prev-stack (back-button).\n");
					//Go back to parent stack
					if(_contents.stacks.use > 1){
						STSceneInfoStack* prevStack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, _contents.stacks.use - 2);
						NBASSERT(curItm != NULL)
						if(prevStack->contentStack.use > 0){
							newItm = NBArray_itmValueAtIndex(&prevStack->contentStack, STSceneInfoStackItm*, prevStack->contentStack.use - 1);
							NBASSERT(newItm->subStack == curStack)
							newItm->subStack = NULL;
						}
						//Remove and release
						NBArray_removeItemAtIndex(&_contents.stacks, _contents.stacks.use - 1);
						SceneInfoStack_release(curStack, this);
						//
						curStack = prevStack;
					}
				}
				this->privContentExchangeFromTo(curItm, newItm);
			}
		}
	}
}

void AUSceneIntro::privContentShowNext(const BOOL ignoreIfAnimating){
	if(_contents.stacks.use > 0 && (!ignoreIfAnimating || this->_contents.anims.use == 0)){
		STSceneInfoStack* curStack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, _contents.stacks.use - 1);
		if((curStack->iCurStep + 1) >= 0 && (curStack->iCurStep + 1) < curStack->stepsSequence.use){
			const UI32 stackUseBefore = curStack->contentStack.use;
			STSceneInfoStackItm* curItm = NULL;
			STSceneInfoStackItm* newItm = NULL;
			PRINTF_INFO("Moving to next-content.\n");
			//NewItem
			{
				curStack->iCurStep++;
				const ENSceneInfoContent uid = NBArray_itmValueAtIndex(&curStack->stepsSequence, ENSceneInfoContent, curStack->iCurStep);
				this->privStackPushContent(curStack, uid);
				NBASSERT((stackUseBefore + 1) == curStack->contentStack.use)
				newItm = NBArray_itmValueAtIndex(&curStack->contentStack, STSceneInfoStackItm*, curStack->contentStack.use - 1);
				this->privContentOrganizeItm(newItm);
			}
			//CurItm (read after modifing the stack)
			if(stackUseBefore > 0){
				curItm = NBArray_itmValueAtIndex(&curStack->contentStack, STSceneInfoStackItm*, stackUseBefore - 1);
			}
			//Exchange
			this->privContentExchangeFromTo(curItm, newItm);
		} else {
			//Pop to previous scene
			if(_listener != NULL){
				_listener->loadPreviousScene(ENSceneTransitionType_White, TS_TRANSITION_PARTIAL_ANIM_DURATION);
			}
		}
	}
}

BOOL AUSceneIntro::privContentCurrentIsLast(){
	BOOL r = FALSE;
	if(_contents.stacks.use > 0){
		STSceneInfoStack* curStack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, _contents.stacks.use - 1);
		if((curStack->iCurStep + 1) >= 0 && (curStack->iCurStep + 1) < curStack->stepsSequence.use){
			r = FALSE;
		} else {
			r = TRUE;
		}
	}
	return r;
}

//

void AUSceneIntro::privConfirmReqStart(const void* pkeyDER, const UI32 pkeyDERSz, const void* csrDER, const UI32 csrDERSz){
	STNBString verifSeqId;
	NBString_init(&verifSeqId);
	//Save key and retrieve verifSeqId
	{
		STNBStorageRecordWrite ref = TSClient_getStorageRecordForWrite(_coreClt, "client/_root/", "_current", TRUE, NULL, TSClientRoot_getSharedStructMap());
		if(ref.data != NULL){
			STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
			if(priv != NULL){
				STTSClientRootReq* reqq = &priv->req;
				//verifSeqId
				if(!NBString_strIsEmpty(reqq->verifSeqId)){
					NBString_set(&verifSeqId, reqq->verifSeqId);
				}
				//
				if(pkeyDER != NULL && pkeyDERSz > 0){
					NBString_strFreeAndNewBufferBytes((char**)&reqq->pkeyDER, (const char*)pkeyDER, pkeyDERSz);
					reqq->pkeyDERSz		= pkeyDERSz;
					ref.privModified	= TRUE;
				}
			}
		}
		TSClient_returnStorageRecordFromWrite(_coreClt, &ref);	
	}
	//Send request
	/*{
		const STNBStructMap* reqMap = TSPatientVerifCodeValidReq_getSharedStructMap();
		STTSPatientVerifCodeValidReq req;
		NBMemory_setZeroSt(req, STTSPatientVerifCodeValidReq);
		NBString_strFreeAndNewBuffer(&req.verifSeqId, verifSeqId.str);
		if(csrDER != NULL && csrDERSz > 0){
			NBString_strFreeAndNewBufferBytes((char**)&req.csrDER, (const char*)csrDER, csrDERSz);
			req.csrDERSz = csrDERSz;
		}
		{
			const char* strLoc = TSClient_getStr(_coreClt, "stpMailbxsReqConfirmActiveTitle", "Linking your account");
			_reqInvitation.verif.requestId = _listener->addRequestWithPop(ENReqPopHideMode_Auto, strLoc, ENTSRequestId_Invitation_validate, NULL, &req, sizeof(req), reqMap, TS_POPUPS_REQ_WAIT_SECS_BEFORE_REQ_LOCAL, 0.0f / *TS_POPUPS_REQ_WAIT_SECS_BEFORE_SHOW_LOCAL* /, this, this, this);
		}
		NBStruct_stRelease(reqMap, &req, sizeof(req));
	}*/
	//Hide pop
	if(_reqInvitation.confirm.popBuildingRef != NULL){
		_listener->popHide(_reqInvitation.confirm.popBuildingRef);
		_listener->popRelease(_reqInvitation.confirm.popBuildingRef);
		_reqInvitation.confirm.popBuildingRef = NULL;
	}
	if(_reqInvitation.confirm.popConfirmRef != NULL){
		_listener->popHide(_reqInvitation.confirm.popConfirmRef);
		_listener->popRelease(_reqInvitation.confirm.popConfirmRef);
		_reqInvitation.confirm.popConfirmRef = NULL;
	}
	NBString_release(&verifSeqId);
}

void AUSceneIntro::tickAnimacion(float segsTranscurridos){
	//Server
	if(_contents.anims.use == 0){ //Analyze when no animations executing
		//Building CSR progress
		if(_reqInvitation.confirm.isBuildingLast != _reqInvitation.confirm.isBuilding){
			_reqInvitation.confirm.isBuildingLast = _reqInvitation.confirm.isBuilding;
			if(!_reqInvitation.confirm.isBuilding){
				if(_reqInvitation.confirm.pkey == NULL || _reqInvitation.confirm.csr == NULL){
					this->privConfirmBuildingErrorPopShow();
				} else if(!NBPKey_isCreated(_reqInvitation.confirm.pkey) || ! NBX509Req_isCreated(_reqInvitation.confirm.csr)){
					this->privConfirmBuildingErrorPopShow();
				} else {
					STNBString pkeyDER, csrDER;
					NBString_init(&pkeyDER);
					NBString_init(&csrDER);
					if(!NBPKey_getAsDERString(_reqInvitation.confirm.pkey, &pkeyDER)){
						this->privConfirmBuildingErrorPopShow();
					} else if(!NBX509Req_getAsDERString(_reqInvitation.confirm.csr, &csrDER)){
						this->privConfirmBuildingErrorPopShow();
					} else {
						this->privConfirmReqStart(pkeyDER.str, pkeyDER.length, csrDER.str, csrDER.length);
					}
					NBString_release(&pkeyDER);
					NBString_release(&csrDER);
				}
			}
		}
	}
	//Contents
	{
		//Anims (remove animations and layers)
		{
			SI32 i; for(i = 0; i < _contents.anims.use; i++){
				STSceneInfoContentAnim* anim = NBArray_itmPtrAtIndex(&_contents.anims, STSceneInfoContentAnim, i);
				if(anim->animator->conteoAnimacionesEjecutando() == 0){
					//PRINTF_INFO("Animation #%d / %d ended (%s).\n", (i + 1), _contents.anims.use, (anim->isOutAnim ? "OUT" : "IN"));
					STSceneInfoContent* content = &anim->stackItm->content;
					//Animator
					if(anim->animator != NULL){
						anim->animator->purgarAnimaciones();
						anim->animator->liberar(NB_RETENEDOR_THIS);
						anim->animator = NULL;
					}
					//Layers
					{
						AUEscenaContenedor* lyrParent = (AUEscenaContenedor*)content->layer->contenedor();
						AUEscenaContenedor* btnsParent = (AUEscenaContenedor*)content->btnsLayer->contenedor();
						if(anim->isOutAnim){
							//Remove layers
							if(lyrParent != NULL) lyrParent->quitarObjetoEscena(content->layer);
							if(btnsParent != NULL) btnsParent->quitarObjetoEscena(content->btnsLayer);
						} else {
							//Add layers (in case were removed by previous out-animation)
							if(lyrParent != _contents.lyr) _contents.lyr->agregarObjetoEscena(content->layer);
							if(btnsParent != _contents.lyr) _contents.lyr->agregarObjetoEscena(content->btnsLayer);
						}
					}
					//Free memory //ToDo: implement
					/*SceneInfoStackItm_release(anim->stackItm);
					 NBMemory_free(anim->stackItm);
					 anim->stackItm = NULL;*/
					//Remove animation
					NBArray_removeItemAtIndex(&_contents.anims, i);
					i--;
				}
			}
		}
	}
	//Actions from record's change (while no animating)
	if(_contents.anims.use == 0 && _data.recordChanged){
		_data.recordChanged = FALSE;
		{
			STSceneInfoStackItm* itm = this->privCurStackCurItm();
			if(itm != NULL){
				this->privContentRecordChanged(itm);
			}
		}
	}
	//Tick current item (while no animating)
	if(_contents.anims.use == 0){ //animate only when actions are posible (not blocked by animations)
		STSceneInfoStackItm* itm = this->privCurStackCurItm();
		if(itm != NULL){
			this->privContentTick(itm, segsTranscurridos);
		}
	}
}

//

void AUSceneIntro::botonPresionado(AUIButton* obj){
	//
}

void AUSceneIntro::botonAccionado(AUIButton* obj){
	//Detect button
	if(obj == _resetData.btn){
		//Reset local data
		if(_resetData.popRef != NULL){
			_listener->popHide(_resetData.popRef);
			_listener->popRelease(_resetData.popRef);
			_resetData.popRef = NULL;
		}
		{
			const char* strLoc0 = TSClient_getStr(_coreClt, "resetDataTitle", "Reset app");
			const char* strLoc1 = TSClient_getStr(_coreClt, "resetDataText", "This action will reset this app to its original state by deleting your local data. Your final attestations will be kept. Do you want to continue?");
			const char* strLoc2 = TSClient_getStr(_coreClt, "resetDataBtnOk", "Reset local data");
			_resetData.popRef = _listener->popCreate(ENScenePopMode_Error, ENNBTextLineAlignH_Center, strLoc0, strLoc1, this, this, NULL, NULL, ENScenePopBtnColor_Clear);
			_listener->popAddOption(_resetData.popRef, "reset", strLoc2, ENScenePopBtnColor_Main, ENIMsgboxBtnAction_AutohideAndNotify);
			_listener->popShow(_resetData.popRef);
		}
	} else {
		BOOL keepSrching = TRUE;
		SI32 s; for(s = (_contents.stacks.use - 1); s >= 0 && keepSrching; s--){ //Todo: implement all
			STSceneInfoStack* stack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, s);
			SI32 i; for(i = (stack->contentStack.use - 1); i >= 0 && keepSrching; i--){
				STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&stack->contentStack, STSceneInfoStackItm*, i);
				STSceneInfoContent* content = &itm->content;
				STSceneInfoContentBase* base = &content->base;
				SI32 i; for(i = ((SI32)base->buttons.use - 1); i >= 0; i--){
					STSceneInfoBtn* btn = NBArray_itmPtrAtIndex(&base->buttons, STSceneInfoBtn, i);
					if(btn->btn != NULL && btn->btn == obj){
						this->privContentAction(itm, btn->optionId.str);
						keepSrching = FALSE;
						break;
					}
				}
			}
		}
	}
}

AUEscenaObjeto* AUSceneIntro::botonHerederoParaTouch(AUIButton* obj, const NBPunto &posInicialEscena, const NBPunto &posActualEscena){
	return NULL;
}

//

void AUSceneIntro::msgboxOptionSelected(AUIMsgBox* obj, const char* optionId){
	{
		STSceneInfoStackItm* itm = this->privCurStackCurItm();
		if(itm != NULL){
			this->privContentAction(itm, optionId);
		}
	}
}

void AUSceneIntro::msgboxCloseSelected(AUIMsgBox* obj){
	//
}

void AUSceneIntro::msgboxFocusObtained(AUIMsgBox* obj){
	//
}

void AUSceneIntro::msgboxFocusLost(AUIMsgBox* obj){
	//No need to remove, will be removed with step's layer
}

//

void AUSceneIntro::popOptionSelected(void* param, void* ref, const char* optionId){
	if(ref == _resetData.popRef){
		if(NBString_strIsEqual(optionId, "reset")){
			TSClient_resetLocalData(_coreClt);
			_listener->reloadInitialContent(ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
		}
	} else if(ref == _reqInvitation.confirm.popBuildingRef){
		//
	} else if(ref == _reqInvitation.confirm.popConfirmRef){
		if(NBString_strIsEqual(optionId, "continue")){
			this->privConfirmFlowConfirmed();
		}
	} else {
		{
			STSceneInfoStackItm* itm = this->privCurStackCurItm();
			if(itm != NULL){
				this->privContentPopOptionSelected(itm, ref, optionId);
			}
		}
	}
}

void AUSceneIntro::popFocusObtained(void* param, void* ref){
	//
}

void AUSceneIntro::popFocusLost(void* param, void* ref){
	if(ref == _resetData.popRef){
		_listener->popRelease(_resetData.popRef);
		_resetData.popRef = NULL;
	} else if(ref == _reqInvitation.confirm.popBuildingRef){
		_listener->popRelease(_reqInvitation.confirm.popBuildingRef);
		_reqInvitation.confirm.popBuildingRef = NULL;
	} else if(ref == _reqInvitation.confirm.popConfirmRef){
		_listener->popRelease(_reqInvitation.confirm.popConfirmRef);
		_reqInvitation.confirm.popConfirmRef = NULL;	
	}
}

//

void AUSceneIntro::reqPopEnded(const UI64 reqId, void* param, const STTSClientChannelRequest* req){
	if(req != NULL){
		/*if(_reqInvitation.verif.requestId == reqId){
			NBASSERT(req->reqId == ENTSRequestId_Invitation_validate)
			_reqInvitation.verif.requestId = 0;
		} else if(_reqInvitation.confirm.requestId == reqId){
			NBASSERT(req->reqId == ENTSRequestId_Invitation_validate)
			_reqInvitation.confirm.requestId = 0;
		}*/
	}
}

//

void AUSceneIntro::textboxFocoObtenido(AUITextBox* obj){
	//Detect textbox
	BOOL keepSrching = TRUE;
	SI32 s; for(s = (_contents.stacks.use - 1); s >= 0 && keepSrching; s--){ //Todo: implement all
		STSceneInfoStack* stack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, s);
		SI32 i; for(i = (stack->contentStack.use - 1); i >= 0 && keepSrching; i--){
			STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&stack->contentStack, STSceneInfoStackItm*, i);
			STSceneInfoContent* content = &itm->content;
			STSceneInfoContentBase* base = &content->base;
			SI32 i; for(i = 0; i < base->txtBoxes.use; i++){
				AUITextBox* txtBox = NBArray_itmValueAtIndex(&base->txtBoxes, AUITextBox*, i);
				if(txtBox == obj){
					this->privContentUpdateState(itm, obj, ENSceneInfoUpdateStateType_FocusGain, TRUE);
					keepSrching = FALSE;
					break;
				}
			}
		}
	}
}

void AUSceneIntro::textboxFocoPerdido(AUITextBox* obj, const ENNBKeybFocusLostType actionType){
	//Detect textbox
	BOOL keepSrching = TRUE;
	SI32 s; for(s = (_contents.stacks.use - 1); s >= 0 && keepSrching; s--){ //Todo: implement all
		STSceneInfoStack* stack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, s);
		SI32 i; for(i = (stack->contentStack.use - 1); i >= 0 && keepSrching; i--){
			STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&stack->contentStack, STSceneInfoStackItm*, i);
			STSceneInfoContent* content = &itm->content;
			STSceneInfoContentBase* base = &content->base;
			SI32 i; for(i = 0; i < base->txtBoxes.use; i++){
				AUITextBox* txtBox = NBArray_itmValueAtIndex(&base->txtBoxes, AUITextBox*, i);
				if(txtBox == obj){
					this->privContentUpdateState(itm, obj, ENSceneInfoUpdateStateType_FocusLost, TRUE);
					keepSrching = FALSE;
					break;
				}
			}
		}
	}
}

bool AUSceneIntro::textboxMantenerFocoAnteConsumidorDeTouch(AUITextBox* obj, AUEscenaObjeto* consumidorTouch){
	bool r = FALSE;
	//Detect textbox
	/*{
		SI32 i; for(i = 0; i < _contents.objs.use; i++){
			STSceneInfoContent* content = NBArray_itmPtrAtIndex(&_contents.objs, STSceneInfoContent, i);
			STSceneInfoContentBase* base = &content->base;
			SI32 i; for(i = 0; i < base->txtBoxes.use; i++){
				AUITextBox* txtBox = NBArray_itmValueAtIndex(&base->txtBoxes, AUITextBox*, i);
	 			if(txtBox != NULL){
	 				PRINTF_INFO("..");
	 				//this->privContentAction(itm, btn->optionId.str);
	 				break;
				}
			}
		}
	}*/
	return r;
}

void AUSceneIntro::textboxContenidoModificado(AUITextBox* obj, const char* strContenido){
	//Detect textbox
	{
		BOOL keepSrching = TRUE;
		SI32 s; for(s = (_contents.stacks.use - 1); s >= 0 && keepSrching; s--){ //Todo: implement all
			STSceneInfoStack* stack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, s);
			SI32 i; for(i = (stack->contentStack.use - 1); i >= 0 && keepSrching; i--){
				STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&stack->contentStack, STSceneInfoStackItm*, i);
				STSceneInfoContent* content = &itm->content;
				STSceneInfoContentBase* base = &content->base;
				SI32 i; for(i = 0; i < base->txtBoxes.use; i++){
					AUITextBox* txtBox = NBArray_itmValueAtIndex(&base->txtBoxes, AUITextBox*, i);
					if(txtBox == obj){
						this->privContentUpdateState(itm, obj, ENSceneInfoUpdateStateType_TextValueChanged, TRUE);
						keepSrching = FALSE;
						break;
					}
				}
			}
		}
	}
}

//File updated

void AUSceneIntro::recordChanged(void* param, const char* relRoot, const char* relPath){
	NBASSERT(param != NULL)
	//PRINTF_INFO("AUSceneIntro, record updated: '%s' '%s'.\n", relRoot, relPath);
	if(param != NULL){
		AUSceneIntro* obj = (AUSceneIntro*)param;
		if(NBString_strIsLike(relRoot, "client/_root/") && NBString_strIsLike(relPath, "_current")){
			obj->_data.recordChanged = TRUE;
		}
	}
}

//TOUCHES

void AUSceneIntro::touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto){
	if(objeto == _back.layer){
		_back.layer->establecerMultiplicadorColor8(_back.layerColor.r, _back.layerColor.g, _back.layerColor.b, _back.layerColor.a / 2);
	} else {
		BOOL keepSrching = TRUE;
		SI32 s; for(s = (_contents.stacks.use - 1); s >= 0 && keepSrching; s--){ //Todo: implement all
			STSceneInfoStack* stack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, s);
			SI32 i; for(i = (stack->contentStack.use - 1); i >= 0 && keepSrching; i--){
				STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&stack->contentStack, STSceneInfoStackItm*, i);
				STSceneInfoContent* content = &itm->content;
				if(content->base.privacyLnk == objeto || content->base.privacyLnkIco == objeto || content->base.privacyLnkLine == objeto){
					content->base.privacyLnk->establecerMultiplicadorAlpha8(255);
					content->base.privacyLnkIco->establecerMultiplicadorAlpha8(255);
					content->base.privacyLnkLine->establecerMultiplicadorAlpha8(255);
				} else {
					/*STNBArray* touchables = &content->base.touchables;
					SI32 i; for(i = 0; i < touchables->use; i++){
						AUEscenaObjeto* obj = NBArray_itmValueAtIndex(touchables, AUEscenaObjeto*, i);
						if(obj == objeto){
							this->privContentTouchStarted(itm, obj, posTouchEscena);
							keepSrching = FALSE;
							break;
						}
					}*/
				}
			}
		}
	}
}

void AUSceneIntro::touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	//
}

void AUSceneIntro::touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	if(objeto == _back.layer){
		_back.layer->establecerMultiplicadorColor8(_back.layerColor.r, _back.layerColor.g, _back.layerColor.b, _back.layerColor.a);
	} else {
		BOOL keepSrching = TRUE;
		SI32 s; for(s = (_contents.stacks.use - 1); s >= 0 && keepSrching; s--){ //Todo: implement all
			STSceneInfoStack* stack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, s);
			SI32 i; for(i = (stack->contentStack.use - 1); i >= 0 && keepSrching; i--){
				STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&stack->contentStack, STSceneInfoStackItm*, i);
				STSceneInfoContent* content = &itm->content;
				if(content->base.privacyLnk == objeto || content->base.privacyLnkIco == objeto || content->base.privacyLnkLine == objeto){
					content->base.privacyLnk->establecerMultiplicadorAlpha8(255);
					content->base.privacyLnkIco->establecerMultiplicadorAlpha8(255);
					content->base.privacyLnkLine->establecerMultiplicadorAlpha8(255);
				} else {
					/*STNBArray* touchables = &content->base.touchables;
					SI32 i; for(i = 0; i < touchables->use; i++){
						AUEscenaObjeto* obj = NBArray_itmValueAtIndex(touchables, AUEscenaObjeto*, i);
						if(obj == objeto){
							this->privContentTouchEnded(itm, obj, posActualEscena);
							keepSrching = FALSE;
							break;
						}
					}*/
				}
			}
		}
	}
	//
	const NBCajaAABB cajaEscena = objeto->cajaAABBEnEscena();
	if(NBCAJAAABB_INTERSECTA_PUNTO(cajaEscena, posActualEscena.x, posActualEscena.y) && !touch->cancelado){
		if(objeto == _back.layer){
			this->privContentShowPrev(1, TRUE, TRUE);
		} else {
			//Analyze touchables
			{
				BOOL keepSrching = TRUE;
				SI32 s; for(s = (_contents.stacks.use - 1); s >= 0 && keepSrching; s--){ //Todo: implement all
					STSceneInfoStack* stack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, s);
					SI32 i; for(i = (stack->contentStack.use - 1); i >= 0 && keepSrching; i--){
						STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&stack->contentStack, STSceneInfoStackItm*, i);
						STSceneInfoContent* content = &itm->content;
						if(content->base.privacyLnk == objeto || content->base.privacyLnkIco == objeto || content->base.privacyLnkLine == objeto){
							if(_popupRef != NULL){
								_listener->popHide(_popupRef);
								_listener->popRelease(_popupRef);
								_popupRef = NULL;
							}
							_popupRef = _listener->popCreate(ENScenePopMode_TextOnly, ENNBTextLineAlignH_Left, content->base.privacyTitle.str, content->base.privacyText.str, NULL, NULL, NULL, NULL, ENScenePopBtnColor_Clear);
							_listener->popShow(_popupRef);
						} else {
							STNBArray* touchables = &content->base.touchables;
							SI32 i; for(i = 0; i < touchables->use; i++){
								AUEscenaObjeto* obj = NBArray_itmValueAtIndex(touchables, AUEscenaObjeto*, i);
								if(obj == objeto){
									this->privContentTouched(itm, obj, posActualEscena);
									keepSrching = FALSE;
									break;
								}
							}
						}
					}
				}
			}
		}
	}
}

//TECLAS
bool AUSceneIntro::teclaPresionada(SI32 codigoTecla){
	return false;
}

bool AUSceneIntro::teclaLevantada(SI32 codigoTecla){
	return false;
}

bool AUSceneIntro::teclaEspecialPresionada(SI32 codigoTecla){
	return false;
}

//Visual keyboard
void AUSceneIntro::tecladoVisualAltoModificado(const float altoPixeles){
	const NBCajaAABB scnBox = _lastBox;
	const float kbSceneH	= NBGestorEscena::altoPuertoAGrupo(_iScene, ENGestorEscenaGrupo_GUI, altoPixeles);
	const float marginV		= NBGestorEscena::altoPulgadasAEscena(_iScene, 0.05f);
	const float yMaxCovered = scnBox.yMin + kbSceneH + (kbSceneH != 0.0f ? marginV : 0.0f);
	PRINTF_INFO("ViuaslKeyboard height: px(%f) scene(%f / %f, %d%%).\n", altoPixeles, kbSceneH, (scnBox.yMax - scnBox.yMin), (SI32)(100.0f * kbSceneH / (scnBox.yMax - scnBox.yMin)));
	//
	const float speedMove	= NBGestorEscena::anchoPulgadasAEscena(_iScene, 3.0f);
	//Move forms the amount of pixels obstructed
	{
		SI32 s; for(s = (_contents.stacks.use - 1); s >= 0; s--){ //Todo: implement all
			STSceneInfoStack* stack = NBArray_itmValueAtIndex(&_contents.stacks, STSceneInfoStack*, s);
			SI32 i; for(i = (stack->contentStack.use - 1); i >= 0; i--){
				STSceneInfoStackItm* itm = NBArray_itmValueAtIndex(&stack->contentStack, STSceneInfoStackItm*, i);
				STSceneInfoContent* content = &itm->content;
				//Move form
				if(yMaxCovered <= content->formLayerLimitSceneBox.yMin){
					//The form is not touched (return to original position)
					content->formLayerPos	= content->formLayerLimitPos;
					const NBPunto curPos	= content->formLayer->traslacion();
					const float distance	= NBPUNTO_DISTANCIA(curPos.x, curPos.y, content->formLayerPos.x, content->formLayerPos.y);
					_animator->quitarAnimacionesDeObjeto(content->formLayer);
					_animator->animarPosicion(content->formLayer, content->formLayerPos.x, content->formLayerPos.y, distance / speedMove);
				} else {
					//The form was covered (move to visibble position)
					const float delta = (yMaxCovered - content->formLayerLimitSceneBox.yMin);
					content->formLayerPos.x = content->formLayerLimitPos.x;
					content->formLayerPos.y = content->formLayerLimitPos.y + delta;
					const NBPunto curPos	= content->formLayer->traslacion();
					const float distance	= NBPUNTO_DISTANCIA(curPos.x, curPos.y, content->formLayerPos.x, content->formLayerPos.y);
					_animator->quitarAnimacionesDeObjeto(content->formLayer);
					_animator->animarPosicion(content->formLayer, content->formLayerPos.x, content->formLayerPos.y, distance / speedMove);
				}
			}
		}
	}
	//Save
	_keybHeight = kbSceneH;
}

//

AUOBJMETODOS_CLASESID_UNICLASE(AUSceneIntro)
AUOBJMETODOS_CLASESNOMBRES_UNICLASE(AUSceneIntro, "AUSceneIntro")
AUOBJMETODOS_CLONAR_NULL(AUSceneIntro)


