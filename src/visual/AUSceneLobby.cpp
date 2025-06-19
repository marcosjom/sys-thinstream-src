//
//  AUSceneLobby.cpp
//  thinstream
//
//  Created by Marcos Ortega on 9/2/18.
//

#include "visual/TSVisualPch.h"
#include "visual/AUSceneLobby.h"
#include "visual/AUSceneCol.h"
#include "visual/gui/TSVisualIcons.h"
#include "visual/TSColors.h"
#include "core/logic/TSClientRoot.h"
#include "core/logic/TSDeviceLocal.h"
#include "visual/viewer/TSVisualViewer.h"
//
#include "NBMngrOSTools.h"
#include "nb/2d/NBPng.h"
#include "nb/2d/NBJpeg.h"

#define TS_LOBBY_COLS_EXTRA_HEIGHT_TOP_BOTTOM_INCHES	1.0f	//Offscreen height on top and bottom of the screen to avoid objetcs to dissapear inside the screen area

AUSceneLobby::AUSceneLobby(STTSClient* coreClt, SI32 iScene, IScenesListener* scenes)
: AUAppEscena()
{
	NB_DEFINE_NOMBRE_PUNTERO(this, "AUSceneLobby")
	{
		NBMemory_setZero(_lobbyRoot);
		_lobbyRoot.iScene	= iScene;
		_lobbyRoot.coreClt	= coreClt;
		_lobbyRoot.scenes	= scenes;
		_lobbyRoot.lobby	= this;
	}
	//Orient
	{
		NBMemory_setZero(_orient);
	}
	//
#	ifdef CONFIG_NB_INCLUIR_VALIDACIONES_ASSERT
	_infoDebugMostrada	= new(this) AUCadenaMutable8();
#	endif
	//
	_animObjetos	= new(this) AUAnimadorObjetoEscena();
	_lyrContent		= new(this) AUEscenaContenedor(); NB_DEFINE_NOMBRE_PUNTERO(_lyrContent, "_lyrContent")
	_lyrDbg			= new(this) AUEscenaContenedor();
	_lyrBars		= new(this) AUEscenaContenedor();
	_lyrFront		= new(this) AUEscenaContenedor();
	//Calculate limits
	this->privCalculateLimits();
	//Stats
	{
		_stats.secsAccum	= 0.0f;
	}
	//Energy saving (disabled)
	/*{
		NBMemory_setZero(_energySaving);
		_energySaving.secsToShowWhenOthers	= (0.5f * 60.0f);
		_energySaving.secsToShowWhenAlone	= (3.0f * 60.0f);
	}*/
	//Bars
	{
		NBMemory_setZero(_bars.top);
		const NBCajaAABB cajaGUI	= _lastBox;
		const float altoExtraSup	= (_lastBox.yMax - _lastBoxUsable.yMax);
		const float altoExtraInf	= (_lastBoxUsable.yMin - _lastBox.yMin);
		UI32 i; for(i = 0; i < ENSceneLobbyBar_Count; i++){
			STSceneLobbyBar* bar = &_bars.data[i];
			NBMemory_setZeroSt(*bar, STSceneLobbyBar);
			bar->uid = (ENSceneLobbyBar)i; NBASSERT(bar->uid >= 0 && bar->uid < ENSceneLobbyBar_Count)
			bar->notVisible		= FALSE;
			bar->outOfScreen	= FALSE;
			NBColor8 color; NBCOLOR_ESTABLECER(color, 255, 255, 255, 255);
			switch(i) {
				case ENSceneLobbyBar_TopTitle:
					_bars.top		= new(this) AUSceneBarTop(&_lobbyRoot, (cajaGUI.xMax - cajaGUI.xMin), altoExtraSup);
					_bars.top->establecerEscuchadorTouches(this, this);
					//_bars.top->establecerEstado(ENBarraSupModo_Titulo, ENBarraSupIconoVolver_Volver, "Dental Robot", false);
					_bars.top->organizarContenido((cajaGUI.xMax - cajaGUI.xMin), 0.0f, false, NULL, 0.0f);
					_bars.top->setVisibleAndAwake(!bar->notVisible);
					_lyrBars->agregarObjetoEscena(_bars.top);
					bar->pos		= ENSceneLobbyBarPos_Top;
					bar->box		= _bars.top->cajaAABBLocalCalculada();
					bar->obj		= _bars.top;
					bar->itf		= _bars.top;
					break;
				case ENSceneLobbyBar_BottomCats:
					_bars.btm		= new(this) AUSceneBarBtm(&_lobbyRoot, (cajaGUI.xMax - cajaGUI.xMin), altoExtraInf);
					_bars.btm->setVisibleAndAwake(!bar->notVisible);
					_lyrBars->agregarObjetoEscena(_bars.btm);
					bar->pos		= ENSceneLobbyBarPos_Bottom;
					bar->box		= _bars.btm->cajaAABBLocalCalculada();
					bar->obj		= _bars.btm;
					bar->itf		= _bars.btm;
					break;
				default:
					NBASSERT(FALSE)
					break;
			}
		}
	}
	//Panels touch
	{
		_touch.first	= NULL;
		_touch.start.x	= _touch.start.y = 0.0f;
		_touch.iPanel	= -1;
	}
	//Panels grps
	{
		SI32 i; for(i = 0 ; i < ENLobbyPanelGrp_Count; i++){
			STLobbyPanelGrp* grp = &_panelGrps[i];
			NBMemory_setZeroSt(*grp, STLobbyPanelGrp);
			grp->xLeft	= 0.0f;
			grp->xDelta	= 0.0f;
			grp->width	= 0.0f;
			grp->lyr	= new(this) AUEscenaContenedor();
		}
	}
	//Panels
	{
		{
			SI32 i; for(i = 0; i < ENLobbyPanel_Count; i++){
				STLobbyPanel* pnl		= &_panels[i];
				const ENLobbyPanelGrp iGrp = ENLobbyPanelGrp_Main; //(i == ENLobbyPanel_Behind ? ENLobbyPanelGrp_Behind : i == ENLobbyPanel_MainLeft || i == ENLobbyPanel_MainCenter ? ENLobbyPanelGrp_Main : ENLobbyPanelGrp_Main);
				STLobbyPanelGrp* grp	= &_panelGrps[iGrp];
				pnl->iGrp				= iGrp;
				//ColRoot
				{
					STLobbyColRoot* colRoot = &pnl->colRoot;
					NBMemory_setZeroSt(*colRoot, STLobbyColRoot);
					colRoot->iScene		= _lobbyRoot.iScene;
					colRoot->coreClt	= _lobbyRoot.coreClt;
					colRoot->scenes		= _lobbyRoot.scenes;
					colRoot->lobby		= _lobbyRoot.lobby;
					colRoot->colGrp		= (ENLobbyPanel)i;
				}
				//Layer
				{
					pnl->lyr	= new(this) AUEscenaContenedor();
					pnl->lyr->establecerEscuchadorTouches(this, this);
					grp->lyr->agregarObjetoEscena(pnl->lyr);
				}
				pnl->xLeft		= 0.0f;
				pnl->xDelta		= 0.0f;
				pnl->width		= (_lastBox.xMax - _lastBox.xMin);
				//
				pnl->barTop		= NULL;
				pnl->barBtm		= NULL;
				if(i == ENLobbyPanel_MainLeft){
					pnl->barTop	= &_bars.data[ENSceneLobbyBar_TopTitle];
					pnl->barBtm	= &_bars.data[ENSceneLobbyBar_BottomCats];
				}
				//Panel
				{
					const SI32 colsStackSz = (i == ENLobbyPanel_MainLeft ? ENLobbyStack_Count : 1);
					const STNBColor8 bgColor = TSColors::colorDef(ENTSColor_Light /*(i == ENLobbyPanel_Behind ? ENTSColor_White : ENTSColor_Light)*/)->normal;
					NBMargenes margins; NBMemory_setZeroSt(margins, NBMargenes);
					pnl->obj		= new(this) AUSceneColsPanel(_lobbyRoot.iScene, pnl->lyr, (_lastBox.xMax - _lastBox.xMin), (_lastBox.yMax - _lastBox.yMin), margins, bgColor, colsStackSz);
					pnl->lyr->agregarObjetoEscena(pnl->obj);
				}
			}
		}
#		ifdef LIBRO_DEBUG_MOSTRAR_MARCAS_PANEL_VISIBLE
		{
			UI16 i;
			for(i = 0; i < ENLobbyPanel_Count; i++){
				STLobbyPanel* datosCol	= &_panels[i];
				datosCol->dbgMarcaPanelVisible	= new(this) AUEscenaSprite();
				datosCol->dbgMarcaPanelVisible->redimensionar(0.0f, 0.0f, 8.0f, 8.0f);
				datosCol->dbgMarcaPanelVisible->establecerMultiplicadorColor8(50, 255, 50, 255);
				_lyrDbg->agregarObjetoEscena(datosCol->dbgMarcaPanelVisible);
			}
			_dbgTxtCantConexionesSegsAcum = 0.0f;
			_dbgTxtCantConexiones = new(this) AUEscenaTexto(NBGestorJAR::fuenteTextura(_lobbyRoot.iScene, ENLibroFuente_Pequena));
			_dbgTxtCantConexiones->establecerAlineaciones(ENNBTextLineAlignH_Left, ENNBTextAlignV_FromBottom);
			_dbgTxtCantConexiones->establecerMultiplicadorColor8(50, 50, 255, 255);
			_lyrDbg->agregarObjetoEscena(_dbgTxtCantConexiones);
		}
#		endif
	}
	//
	this->privUpdatePanelsWidths();
	//
	{
		NBGestorEscena::establecerColorFondo(_lobbyRoot.iScene, 0.0f, 0.0f, 0.0f, 1.0f);
		NBGestorEscena::normalizaCajaProyeccionEscena(_lobbyRoot.iScene);
		NBGestorEscena::normalizaMatricesCapasEscena(_lobbyRoot.iScene);
	}
	//
	_enPrimerPlano					= false;
	this->escenaColocarEnPrimerPlano();
	//
	//Sync from root
	this->privSyncFromRootRecord();
	//
	//this->privPopupEncolar("Hola a todos!", "Este es un contenido de los mejores que he visto en el plazo de nueva era del nuevo milenio y la nueva decada de entre todas las cancioens que he visto venir n el nuevo testamento del difunto, falleciodo o por fallecer. Este es un contenido de los mejores que he visto en el plazo de nueva era del nuevo milenio y la nueva decada de entre todas las cancioens que he visto venir n el nuevo testamento del difunto, falleciodo o por fallecer. Este es un contenido de los mejores que he visto en el plazo de nueva era del nuevo milenio y la nueva decada de entre todas las cancioens que he visto venir n el nuevo testamento del difunto, falleciodo o por fallecer. Este es un contenido de los mejores que he visto en el plazo de nueva era del nuevo milenio y la nueva decada de entre todas las cancioens que he visto venir n el nuevo testamento del difunto, falleciodo o por fallecer.", "Tuani prix!", NULL, ENLibroPopupPos_Top, ENLibroPopupTipo_info);
	//
	this->privOrganizarContenido();
	//
	this->privAnimarEntradaEscena();
	//Cargar contenido inicial si hay mas de una columna
	this->privMostrarContenidoInicial();
	//Add root listener
	{
		STNBStorageCacheItmLstnr methds;
		NBMemory_setZeroSt(methds, STNBStorageCacheItmLstnr);
		methds.retain			= AUObjeto_retain_;
		methds.release			= AUObjeto_release_;
		methds.recordChanged	= AUSceneLobby::recordChanged;
		TSClient_addRecordListener(_lobbyRoot.coreClt, "client/_root/", "_current", this, &methds);
	}
}

AUSceneLobby::~AUSceneLobby(){
#	ifdef CONFIG_NB_INCLUIR_VALIDACIONES_ASSERT
	if(_infoDebugMostrada != NULL) _infoDebugMostrada->liberar(NB_RETENEDOR_THIS); _infoDebugMostrada = NULL;
#	endif
	//
	this->escenaQuitarDePrimerPlano();
	if(_animObjetos!=NULL) _animObjetos->liberar(NB_RETENEDOR_THIS); _animObjetos = NULL;
	if(_lyrContent != NULL) _lyrContent->liberar(NB_RETENEDOR_THIS); _lyrContent = NULL;
	if(_lyrDbg != NULL) _lyrDbg->liberar(NB_RETENEDOR_THIS); _lyrDbg = NULL;
	if(_lyrBars != NULL) _lyrBars->liberar(NB_RETENEDOR_THIS); _lyrBars = NULL;
	if(_lyrFront != NULL) _lyrFront->liberar(NB_RETENEDOR_THIS); _lyrFront = NULL;
	//
#	ifdef LIBRO_DEBUG_MOSTRAR_MARCAS_PANEL_VISIBLE
	{
		UI16 i;
		for(i = 0; i < ENLobbyPanel_Count; i++){
			STLobbyPanel* datosEstado	= &_panels[i];
			if(datosEstado->dbgMarcaPanelVisible != NULL){
				datosEstado->dbgMarcaPanelVisible->liberar(NB_RETENEDOR_THIS);
				datosEstado->dbgMarcaPanelVisible = NULL;
			}
		}
		_dbgTxtCantConexiones->liberar(NB_RETENEDOR_THIS);
		_dbgTxtCantConexiones = NULL;
	}
#	endif
	//Remove listener
	{
		TSClient_removeRecordListener(_lobbyRoot.coreClt, "client/_root/", "_current", this);
	}
	//Left col
	//{
	//	if(_leftColMenu != NULL) _leftColMenu->liberar(NB_RETENEDOR_THIS); _leftColMenu = NULL;
	//}
	//Panels
	{
		SI32 i; for(i = 0; i < ENLobbyPanel_Count; i++){
			STLobbyPanel* pnl = &_panels[i];
			if(pnl->lyr != NULL) pnl->lyr->liberar(NB_RETENEDOR_THIS); pnl->lyr = NULL;
			if(pnl->obj != NULL) pnl->obj->liberar(NB_RETENEDOR_THIS); pnl->obj = NULL;
			NBMemory_setZeroSt(*pnl, STLobbyPanel);
		}
	}
	//Panels grps
	{
		SI32 i; for(i = 0 ; i < ENLobbyPanelGrp_Count; i++){
			STLobbyPanelGrp* grp = &_panelGrps[i];
			if(grp->lyr != NULL) grp->lyr->liberar(NB_RETENEDOR_THIS); grp->lyr = NULL;
			NBMemory_setZeroSt(*grp, STLobbyPanelGrp);
		}
	}
	//Bars
	{
		if(_bars.top != NULL) _bars.top->liberar(NB_RETENEDOR_THIS); _bars.top = NULL;
		if(_bars.btm != NULL) _bars.btm->liberar(NB_RETENEDOR_THIS); _bars.btm = NULL;
	}
	_lobbyRoot.coreClt = NULL;
}

//

void AUSceneLobby::privMostrarContenidoInicial(){
	//Load home
	if(_bars.btm->stacksVisibleCount() > 0){
		const ENLobbyStack iStack = (ENLobbyStack)_bars.btm->stacksVisibleIdxToGlobalIdx(0); //First visible stack
		this->lobbyFocusStack(iStack, FALSE); //allowPopAllAction
	}
}

//

void AUSceneLobby::privContenidoPush(const ENLobbyPanel iCol, const ENLobbyStack iStack, AUEscenaObjeto* objAgregar, AUSceneContentI* itfCol, AUEscenaListaItemI* itfItm, const bool actEstadoBarraSup, const bool animarEntradaNuevo, const bool animarSalidaUltimo, const bool intercambiarConUltimo, const ENSceneColStackPushMode pushMode, const ENSceneColMarginsMode marginsMode){
	BOOL r = FALSE;
	STLobbyPanel* panel = &_panels[iCol];
	panel->obj->setCurrentStack(iStack, ENSceneColSide_Top, ENSceneColSide_Btm);
	if(panel->obj->getCurrentStackSz() <= 0){
		//No animation
		r = panel->obj->push(objAgregar, itfCol, itfItm, ENSceneColSide_Count, ENSceneColSide_Count, pushMode, marginsMode);
	} else {
		//Enter from a side
		r = panel->obj->push(objAgregar, itfCol, itfItm, ENSceneColSide_Left, ENSceneColSide_Right, pushMode, marginsMode);
	}
	//Update bars
	if(r){
		//Update btm bar
		if(iCol == ENLobbyPanel_MainLeft){
			_bars.btm->establecerSeleccionActual(iStack);
		}
	}
	//
#	ifdef  CONFIG_NB_INCLUIR_VALIDACIONES_ASSERT
	/*{
		SI32 i; const SI32 conteo = _panels[iCol].subcolumnas->conteo;
		for(i = 1; i < conteo; i++){
			AUSceneCol* col	= (AUSceneCol*)_panels[iCol].subcolumnas->elem(i);
			NBASSERT(col->pilaContenido()->conteo <= 1); //Las columnas posteriores deben tener solo un elemento maximo
		}
	}*/
#	endif
}

bool AUSceneLobby::privContenidoPop(const ENLobbyPanel iCol){
	bool r = false;
	STLobbyPanel* panel = &_panels[iCol];
	if(panel->obj->getCurrentStackSz() > 1){
		r = panel->obj->pop(ENSceneColSide_Right, ENSceneColSide_Left);
	}
	return r;
}

void AUSceneLobby::privActualizarBarraInfSegunObjTipo(AUObjeto* obj){
	//
}

//

bool AUSceneLobby::escenaEnPrimerPlano(){
	return _enPrimerPlano;
}

void AUSceneLobby::escenaColocarEnPrimerPlano(){
	if(!_enPrimerPlano){
		NBColor8 coloBlanco; NBCOLOR_ESTABLECER(coloBlanco, 255, 255, 255, 255);
		NBGestorEscena::agregarObjetoCapa(_lobbyRoot.iScene, ENGestorEscenaGrupo_GUI, _lyrContent, coloBlanco);
		NBGestorEscena::agregarObjetoCapa(_lobbyRoot.iScene, ENGestorEscenaGrupo_GUI, _lyrDbg, coloBlanco);
		NBGestorEscena::agregarObjetoCapa(_lobbyRoot.iScene, ENGestorEscenaGrupo_GUI, _lyrBars, coloBlanco);
		NBGestorEscena::agregarObjetoCapa(_lobbyRoot.iScene, ENGestorEscenaGrupo_GUI, _lyrFront, coloBlanco);
		NBGestorEscena::establecerColorFondo(_lobbyRoot.iScene, 1.0f, 1.0f, 1.0f, 1.0f);
		NBGestorEscena::agregarEscuchadorCambioPuertoVision(_lobbyRoot.iScene, this);
		NBGestorAnimadores::agregarAnimador(this, this);
		if(_animObjetos!=NULL) _animObjetos->reanudarAnimacion();
		//Set status bar style
		{
			NBMngrOSTools::setBarStyle(ENStatusBarStyle_Dark);
		}
		_enPrimerPlano = true;
	}
}

void AUSceneLobby::escenaQuitarDePrimerPlano(){
	if(_enPrimerPlano){
		NBGestorAnimadores::quitarAnimador(this);
		NBGestorEscena::quitarObjetoCapa(_lobbyRoot.iScene, _lyrContent);
		NBGestorEscena::quitarObjetoCapa(_lobbyRoot.iScene, _lyrDbg);
		NBGestorEscena::quitarObjetoCapa(_lobbyRoot.iScene, _lyrBars);
		NBGestorEscena::quitarObjetoCapa(_lobbyRoot.iScene, _lyrFront);
		NBGestorEscena::quitarEscuchadorCambioPuertoVision(_lobbyRoot.iScene, this);
		if(_animObjetos!=NULL) _animObjetos->detenerAnimacion();
		_enPrimerPlano = false;
	}
}

void AUSceneLobby::escenaGetOrientations(UI32* dstMask, ENAppOrientationBit* dstPrefered, ENAppOrientationBit* dstToApplyOnce, BOOL* dstAllowAutoRotate){
	if(dstMask != NULL) *dstMask = _orient.mask;
	if(dstPrefered != NULL) *dstPrefered = _orient.pref;
	if(dstToApplyOnce != NULL){ *dstToApplyOnce = _orient.orientApplyOnce; _orient.orientApplyOnce = (ENAppOrientationBit)0; }
	if(dstAllowAutoRotate != NULL) *dstAllowAutoRotate = TRUE;
}

bool AUSceneLobby::escenaPermitidoGirarPantalla(){
	return true;
}

bool AUSceneLobby::escenaEstaAnimandoSalida(){
	if(_animObjetos!=NULL) return (_animObjetos->conteoAnimacionesEjecutando()!=0);
	return false;
}

void AUSceneLobby::escenaAnimarEntrada(){
	//
}

void AUSceneLobby::escenaAnimarSalida(){
	//
}

//

void AUSceneLobby::appFocoExclusivoObtenido(){
	
}

void AUSceneLobby::appFocoExclusivoPerdido(){
	
}

void AUSceneLobby::appProcesarNotificacionLocal(const char* grp, const SI32 localId, const char* objTip, const SI32 objId){
	//
}

//

void AUSceneLobby::puertoDeVisionModificado(const SI32 iEscena, const STNBViewPortSize before, const STNBViewPortSize after){
	if(iEscena == _lobbyRoot.iScene){ //Evitar la notificacion del redimensionamiento de la escena-publicidad (render-to-texture)
		//Calculate limits
		this->privCalculateLimits();
		/*if(_leftColMenu != NULL){
			const float altoExtraSup	= (_lastBox.yMax - _lastBoxUsable.yMax);
			//const float altoExtraInf	= (_lastBoxUsable.yMin - _lastBox.yMin);
			_leftColMenu->establecerAltoExtra(altoExtraSup + NBGestorEscena::altoPulgadasAEscena(_lobbyRoot.iScene, LIBRO_PULGADAS_SEPARACION_BARRA_ESTADO));
		}*/
		_animObjetos->purgarAnimaciones();
		this->privOrganizarContenido();
		PRINTF_INFO("Nuevo puertoDeVision(%d, %d)-(%d, %d).\n", after.rect.x, after.rect.y, after.rect.width, after.rect.height);
		//Cargar contenido inicial si hay mas de una columna
		//this->privMostrarContenidoInicial();
	}
}

void AUSceneLobby::privAnimarEntradaEscena(){
	//
}

void AUSceneLobby::privAnimarSalidaEscena(){
	//
}

void AUSceneLobby::privCalculateLimits(){
	const NBCajaAABB box = NBGestorEscena::cajaProyeccionGrupo(_lobbyRoot.iScene, ENGestorEscenaGrupo_GUI);
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
			const float padScn	= NBGestorEscena::altoPuertoAGrupo(_lobbyRoot.iScene, ENGestorEscenaGrupo_GUI, padPxs);
			_lastBoxUsable.yMax	-= padScn;
		}
		//Remove btm bar
		{
			const float padPxs	= NBMngrOSTools::getWindowBtmPaddingPxs();
			const float padScn	= NBGestorEscena::altoPuertoAGrupo(_lobbyRoot.iScene, ENGestorEscenaGrupo_GUI, padPxs);
			_lastBoxUsable.yMin	+= padScn;
		}
		//Remove bottom space if big screen
		/*{
			NBTamano scnSize;		NBCAJAAABB_TAMANO(scnSize, _lastBox)
			const NBTamano inchSz	= NBGestorEscena::tamanoEscenaAPulgadas(_lobbyRoot.iScene, scnSize);
			const float inchDiag	= sqrtf((inchSz.ancho * inchSz.ancho) + (inchSz.alto * inchSz.alto));
			if(inchDiag >= 4.5f){
				//iPhone XR: 6.1 inch
				//iPhone 7: 4.7 inch
				//iPhone SE: 4.0 inch
				_lastBoxUsable.yMin += NBGestorEscena::altoPulgadasAEscena(_lobbyRoot.iScene, 0.15f);
			}
		}*/
	}
}

void AUSceneLobby::privUpdatePanelsWidths(){
	const float widthScn	= (_lastBox.xMax - _lastBox.xMin);
	//const float heightScn	= (_lastBox.yMax - _lastBox.yMin);
	//const NBTamano szInch	= NBGestorEscena::tamanoEscenaAPulgadas(_lobbyRoot.iScene, widthScn, heightScn);
	/* Tamanos de pantallas en pulgadas
	 Android Young:			1.87 x 2.75 pulgadas
	 iPodTouch / iPhone:	2.00 x 3.00 pulgadas
	 iPhone 5:				2.00 x 3.50 pulgadas
	 iPad:					5.81 x 7.75 pulgadas
	 iPad Mini:				4.70 x 6.28 pulgadas*/
	//
	{
		const float w75p		= (float)((SI32)(widthScn * 0.75f));
		const float wInch2_5	= NBGestorEscena::anchoPulgadasAEscena(_lobbyRoot.iScene, 2.5f);
		SI32 i; for(i = 0; i < ENLobbyPanel_Count; i++){
			STLobbyPanel* panel = &_panels[i];
			switch (i) {
				//case ENLobbyPanel_Behind:
				//	panel->width	= (w75p < wInch2_5 ? w75p : wInch2_5);
				//	break;
				case ENLobbyPanel_MainLeft:
				case ENLobbyPanel_MainCenter:
					panel->width	= widthScn;
					break;
				default:
					NBASSERT(FALSE)
					break;
			}
		}
	}
}

//

void AUSceneLobby::privOrganizarSegunCambioAdornosInferiores(){
	NBCajaAABB cajaMenuInf; NBMemory_setZeroSt(cajaMenuInf, NBCajaAABB);
	float hBtnBar			= (cajaMenuInf.yMax - cajaMenuInf.yMin);
	const float altoUsar	= hBtnBar; //(altoMenuInf > _tecladoAltoEscena ? altoMenuInf : _tecladoAltoEscena);
	//ToDo: implement
	SI32 i; for(i = 0; i < ENLobbyPanel_Count; i++){
		//STLobbyPanel* panel = &_panels[i];
	}
}

void AUSceneLobby::privOrganizarContenido(){
	const float widthScn	= (_lastBox.xMax - _lastBox.xMin);
	const float heightScn	= (_lastBox.yMax - _lastBox.yMin);
	//Offscreen height on top and bottom of the screen to avoid objetcs to dissapear inside the screen area
	const float extraHeightTopBtm = (float)((SI32)NBGestorEscena::altoPulgadasAEscena(_lobbyRoot.iScene, TS_LOBBY_COLS_EXTRA_HEIGHT_TOP_BOTTOM_INCHES));
	//
	this->privUpdatePanelsWidths();
	//
	this->privOrganizeBars(FALSE);
	//
	//Organize panels grps
	{
		float grpLeftX[ENLobbyPanelGrp_Count];
		NBMemory_setZero(grpLeftX);
		//Organize panels
		{
			//PRINTF_INFO("_lastBox-x(%f, %f)-y(%f, %f).\n", _lastBox.xMin, _lastBox.xMax, _lastBox.yMin, _lastBox.yMax);
			SI32 i; for(i = 0; i < ENLobbyPanel_Count; i++){
				STLobbyPanel* pnl = &_panels[i];
				NBMargenes margins;
				NBMemory_setZeroSt(margins, NBMargenes);
				margins.top = margins.bottom = extraHeightTopBtm; //To avoid alpha on objects
				if(pnl->barTop != NULL){
					if(pnl->barTop->obj != NULL && !pnl->barTop->notVisible && !pnl->barTop->outOfScreen){
						const NBCajaAABB box	= pnl->barTop->obj->cajaAABBLocal();
						margins.top				+= (box.yMax - box.yMin);
					}
				}
				if(pnl->barBtm != NULL){
					if(pnl->barBtm->obj != NULL  && !pnl->barBtm->notVisible && !pnl->barBtm->outOfScreen){
						const NBCajaAABB box	= pnl->barBtm->obj->cajaAABBLocal();
						margins.bottom			+= (box.yMax - box.yMin);
					}
				}
				pnl->obj->organize(pnl->width, extraHeightTopBtm + heightScn + extraHeightTopBtm, margins);
				{
					const NBCajaAABB box = pnl->lyr->cajaAABBLocal();
					pnl->xLeft = grpLeftX[pnl->iGrp];
					pnl->lyr->establecerTraslacion(pnl->xLeft + pnl->xDelta - box.xMin, _lastBox.yMax + extraHeightTopBtm - box.yMax);
					//PRINTF_INFO("Moved lobby-panel to(%f, %f) from-box-x(%f, %f)-y(%f, %f).\n", pnl->lyr->traslacion().x, pnl->lyr->traslacion().y, box.xMin, box.xMax, box.yMin, box.yMax);
					//Advance in grp
					grpLeftX[pnl->iGrp] += pnl->width;
				}
			}
		}
		{
			SI32 i; for(i = 0; i < ENLobbyPanelGrp_Count; i++){
				STLobbyPanelGrp* grp = &_panelGrps[i];
				switch (i) {
					//case ENLobbyPanelGrp_Behind:
					//	grp->xLeft	= _lastBox.xMin;
					//	grp->width	= grpLeftX[i];
					//	break;
					case ENLobbyPanelGrp_Main:
						grp->xLeft	= _lastBox.xMin;
						grp->width	= widthScn;
						break;
					default:
						NBASSERT(FALSE)
						break;
				}
				{
					const NBCajaAABB box = grp->lyr->cajaAABBLocal();
					grp->lyr->establecerTraslacion(grp->xLeft + grp->xDelta - box.xMin, _lastBox.yMax + extraHeightTopBtm - box.yMax);
					//PRINTF_INFO("Moved lobby-panel-grp to(%f, %f) from-box-x(%f, %f)-y(%f, %f).\n", grp->lyr->traslacion().x, grp->lyr->traslacion().y, box.xMin, box.xMax, box.yMin, box.yMax);
				}
			}
		}
		//
		this->privUpdatePanelsPos();
	}
	//-----------------------
	//-- Organizar capa GUI
	//-----------------------
	//Senales debug
#	ifdef LIBRO_DEBUG_MOSTRAR_MARCAS_PANEL_VISIBLE
	{
		UI16 i; float xLeft = cajaGUI.xMin + 8.0f;
		for(i = 0; i < ENLobbyPanel_Count; i++){
			STLobbyPanel* datosCol	= &_panels[i];
			if(datosCol->dbgMarcaPanelVisible != NULL){
				const NBCajaAABB cajaSenal = datosCol->dbgMarcaPanelVisible->cajaAABBLocal();
				datosCol->dbgMarcaPanelVisible->establecerTraslacion(xLeft - cajaSenal.xMin, cajaGUI.yMin + 8.0f - cajaSenal.yMin);
				xLeft += (cajaSenal.xMax - cajaSenal.xMin) + 4.0f;
			}
		}
		_dbgTxtCantConexiones->establecerTraslacion(xLeft, cajaGUI.yMin + 8.0f);
	}
#	endif
}

//Bars

void AUSceneLobby::privOrganizeBars(const BOOL animate){
	float curPos[ENSceneLobbyBarPos_Count];
	//Define positions and visibility
	{
		UI32 i; for(i = 0; i < ENSceneLobbyBarPos_Count; i++){
			//Set
			if(i == ENSceneLobbyBarPos_Top){
				curPos[i] = _lastBox.yMax;
			} else {
				NBASSERT(i == ENSceneLobbyBarPos_Bottom)
				curPos[i] = _lastBox.yMin;
			}
		}
	}
	//Apply poisitions
	{
		const float widthScn	= (_lastBox.xMax - _lastBox.xMin);
		const float heightScn	= (_lastBox.yMax - _lastBox.yMin);
		const float widthInch	= NBGestorEscena::anchoEscenaAPulgadas(_lobbyRoot.iScene, widthScn);
		const float heightInch	= NBGestorEscena::altoEscenaAPulgadas(_lobbyRoot.iScene, heightScn);
		const BOOL isLandscape	= (widthInch > heightInch);
		const BOOL isSmallScrn	= (((widthInch * widthInch) + (heightInch * heightInch)) < (7.0f * 7.0f)); //iPhone 11 Pro Max is 6.5 inches
		UI32 i; for(i = 0; i < ENSceneLobbyBar_Count; i++){
			STSceneLobbyBar* bar = &_bars.data[i];
			NBASSERT(bar->uid == i)
			bar->itf->organizarContenido((_lastBox.xMax - _lastBox.xMin), 0.0f, false, _animObjetos, 0.25f);
			bar->box		= bar->obj->cajaAABBLocalCalculada();
			bar->outOfScreen = (isLandscape && isSmallScrn);
			NBPunto pos;
			if(bar->pos == ENSceneLobbyBarPos_Top){
				pos.x = _lastBox.xMin - bar->box.xMin;
				if(!bar->outOfScreen){
					pos.y = curPos[bar->pos] - bar->box.yMax;
					curPos[bar->pos] -= (bar->box.yMax - bar->box.yMin);
				} else {
					if(bar->obj->esClase(AUSceneBarTop::idTipoClase)){
						const STNBAABox boxBgOnly = ((AUSceneBarTop*)bar->obj)->bgBox(); //Ignores the substatus-bar
						pos.y = _lastBox.yMax - boxBgOnly.yMin;
					} else {
						pos.y = _lastBox.yMax - bar->box.yMin;
					}
				}
			} else {
				NBASSERT(bar->pos == ENSceneLobbyBarPos_Bottom)
				pos.x = _lastBox.xMin - bar->box.xMin;
				if(!bar->outOfScreen){
					pos.y = curPos[bar->pos] - bar->box.yMin;
					curPos[bar->pos] += (bar->box.yMax - bar->box.yMin);
				} else {
					pos.y = _lastBox.yMin - bar->box.yMax;
				}
			}
			_animObjetos->quitarAnimacionesDeObjeto(bar->obj);
			if(animate){
				_animObjetos->animarPosicion(bar->obj, pos, 0.25f);
				if(!bar->obj->visible()){
					_animObjetos->animarVisible(bar->obj, !bar->notVisible, 0.25f);
				} else {
					_animObjetos->animarColorMult(bar->obj, 255, 255, 255, 255, 0.25f);
				}
			} else {
				bar->obj->establecerTraslacion(pos);
				bar->obj->setVisibleAndAwake(!bar->notVisible);
				bar->obj->establecerMultiplicadorAlpha8(255);
			}
		}
	}
	//To detect top bars size changes
	_bars.topBox = _bars.top->cajaAABBLocal();
}

void AUSceneLobby::privUpdatePanelsPos(){
	//Offscreen height on top and bottom of the screen to avoid objetcs to dissapear inside the screen area
	const float extraHeightTopBtm = (float)((SI32)NBGestorEscena::altoPulgadasAEscena(_lobbyRoot.iScene, TS_LOBBY_COLS_EXTRA_HEIGHT_TOP_BOTTOM_INCHES));
	//Move panels
	{
		SI32 i; for(i = 0; i < ENLobbyPanel_Count; i++){
			STLobbyPanel* pnl = &_panels[i];
			STLobbyPanelGrp* grp = &_panelGrps[pnl->iGrp];
			NBMargenes margins;
			NBMemory_setZeroSt(margins, NBMargenes);
			margins.top = margins.bottom = extraHeightTopBtm;
			if(pnl->barTop != NULL){
				if(pnl->barTop->obj != NULL && !pnl->barTop->notVisible && !pnl->barTop->outOfScreen){
					const NBCajaAABB box	= pnl->barTop->obj->cajaAABBLocal();
					margins.top				+= (box.yMax - box.yMin);
					pnl->barTop->obj->establecerTraslacion(grp->xLeft + grp->xDelta + pnl->xLeft + pnl->xDelta - box.xMin, _lastBox.yMax - box.yMax);
				}
			}
			if(pnl->barBtm != NULL){
				if(pnl->barBtm->obj != NULL  && !pnl->barBtm->notVisible && !pnl->barBtm->outOfScreen){
					const NBCajaAABB box	= pnl->barBtm->obj->cajaAABBLocal();
					margins.bottom			+= (box.yMax - box.yMin);
					pnl->barBtm->obj->establecerTraslacion(grp->xLeft + grp->xDelta + pnl->xLeft + pnl->xDelta - box.xMin, _lastBox.yMin - box.yMin);
				}
			}
			pnl->obj->updateMargins(margins);
			{
				const NBCajaAABB box = pnl->lyr->cajaAABBLocal();
				pnl->lyr->establecerTraslacion(pnl->xLeft + pnl->xDelta - box.xMin, _lastBox.yMax + extraHeightTopBtm - box.yMax);
			}
		}
	}
	//Move panels-grps
	{
		SI32 i; for(i = 0; i < ENLobbyPanelGrp_Count; i++){
			STLobbyPanelGrp* grp = &_panelGrps[i];
			{
				const NBCajaAABB box = grp->lyr->cajaAABBLocal();
				grp->lyr->establecerTraslacion(grp->xLeft + grp->xDelta - box.xMin, _lastBox.yMax + extraHeightTopBtm - box.yMax);
			}
		}
	}
	//Special conditions
	{
		//STLobbyPanelGrp* grpBhnd = &_panelGrps[ENLobbyPanelGrp_Behind];
		STLobbyPanelGrp* grpMain = &_panelGrps[ENLobbyPanelGrp_Main];
		//Add or remove from scene (behind-grp)
		/*{
			AUEscenaContenedor* parent = (AUEscenaContenedor*)grpBhnd->lyr->contenedor();
			if(grpMain->xDelta > 0.0f){
				if(parent != _lyrContent) _lyrContent->agregarObjetoEscenaEnIndice(grpBhnd->lyr, 0);
			} else {
				if(parent != NULL) parent->quitarObjetoEscena(grpBhnd->lyr);
			}
		}*/
		//Add or remove from scene (main-grp)
		{
			AUEscenaContenedor* parent = (AUEscenaContenedor*)grpMain->lyr->contenedor();
			if(TRUE){
				if(parent != _lyrContent) _lyrContent->agregarObjetoEscena(grpMain->lyr);
			} else {
				if(parent != NULL) parent->quitarObjetoEscena(grpMain->lyr);
			}
		}
		{
			const float relBhn = 0.0f; //grpMain->xDelta / grpBhnd->width;
			const UI8 color = (UI8)(255.0f - (205.0f * (relBhn < 0.0f ? 0.0f : relBhn > 1.0f ? 1.0f : relBhn)));
			grpMain->lyr->establecerMultiplicadorColor8(color, color, color, 255);
			_bars.top->establecerMultiplicadorColor8(color, color, color, 255);
			_bars.btm->establecerMultiplicadorColor8(color, color, color, 255);
		}
	}
}

//

void AUSceneLobby::lobbyFocusBehindPanel(){
	//Show bnehind by:
	//Hidding center
	{
		STLobbyPanelGrp* grp	= &_panelGrps[ENLobbyPanelGrp_Main];
		grp->anim.secsDur		= 0.30f;
		grp->anim.secsCur		= 0.0f;
		grp->anim.xDeltaStart	= grp->xDelta;
		grp->anim.animIn		= FALSE;
	}
}

void AUSceneLobby::lobbyHiddeBehindPanel(){
	//Hidding center
	{
		STLobbyPanelGrp* grp	= &_panelGrps[ENLobbyPanelGrp_Main];
		grp->anim.secsDur		= 0.30f;
		grp->anim.secsCur		= 0.0f;
		grp->anim.xDeltaStart	= grp->xDelta;
		grp->anim.animIn		= TRUE;
	}
}

void AUSceneLobby::lobbyFocusStack(const ENLobbyStack iStack, const BOOL allowPopAllAction){
	PRINTF_INFO("lobbyFocusStack (%d).\n", iStack);
	//Update bottom bar
	_bars.btm->establecerSeleccionActual(iStack);
	{
		STLobbyPanel* pnl = &_panels[ENLobbyPanel_MainLeft];
		//Action
		if(pnl->obj->getCurrentStackIdx() != iStack){
			//Move to this stack
			pnl->obj->setCurrentStack(iStack, ENSceneColSide_Top, ENSceneColSide_Btm);
		} else {
			//Second time selected, pop all except top
			if(allowPopAllAction){
				while(pnl->obj->getCurrentStackSz() > 1){
					pnl->obj->pop(ENSceneColSide_Right, ENSceneColSide_Left);
				}
			}
		}
		//Load first if empty
		if(pnl->obj->getCurrentStackSz() <= 0){
			switch(iStack) {
				case ENLobbyStack_Home:
					this->lobbyPushHome();
					break;
				default:
					break;
			}
		}
	}
	//
	this->lobbyHiddeBehindPanel();
}

void AUSceneLobby::lobbyBack(const BOOL ignoreSubstacks){
	//PRINTF_INFO("lobbyBack.\n");
	BOOL consumed = FALSE;
	//Apply to panel
	if(!consumed && !ignoreSubstacks){
		STLobbyPanel* panel = &_panels[ENLobbyPanel_MainLeft];
		if(panel->obj->execBarBackCmd()){
			consumed = TRUE;
		}
	}
	//Apply to main (if not consumed)
	if(!consumed){
		//Back not consumed, execute action on panel
		if(!this->privContenidoPop(ENLobbyPanel_MainLeft)){
			if(_bars.btm->stacksVisibleCount() > 0){
				const ENLobbyStack iStack = (ENLobbyStack)_bars.btm->stacksVisibleIdxToGlobalIdx(0); //First visible stack
				this->lobbyFocusStack(iStack, TRUE);
				//this->lobbyFocusBehindPanel();
			}
		}
	}
}

BOOL AUSceneLobby::lobbyTopBarGetStatusDef(STTSSceneBarStatus* dst){
	return _bars.top->getStatusDef(dst);
}

UI32 AUSceneLobby::lobbyTopBarGetStatusRightIconMask(){
	return _bars.top->getStatusRightIconMask();
}

void AUSceneLobby::lobbyTopBarTouched(){
	STLobbyPanel* panel = &_panels[ENLobbyPanel_MainLeft];
	panel->obj->scrollToTop(0.50f);
}

void AUSceneLobby::lobbyTopBarSubstatusTouched(){
	if(!TSClient_syncIsReceiving(_lobbyRoot.coreClt)){
		const char* strLoc0 = TSClient_getStr(_lobbyRoot.coreClt, "topStatusTitle", "Status");
		const char* strLoc1 = TSClient_getStr(_lobbyRoot.coreClt, "topStatusConnectingExplain", "The app is attempting to connect to the main server, please check your Internet connection. Hide this message to verify the new status.");
		_lobbyRoot.scenes->popShowInfo(ENScenePopMode_TextOnly, ENNBTextLineAlignH_Adjust, strLoc0, strLoc1);
	} else {
		//Connected
	}
}

void AUSceneLobby::lobbyPushTermsAndConds(){
	NBMngrOSTools::openUrl("https://reminder.thinstream.co/terms.php");
}

void AUSceneLobby::lobbyPushPrivacy(){
	NBMngrOSTools::openUrl("https://reminder.thinstream.co/privacypolicy.php");
}

void AUSceneLobby::lobbyPushHome(){
	//PRINTF_INFO("lobbyPushHome.\n");
	NBMargenes margins;
	margins.top		= margins.left = margins.right = 0.0f;
	margins.bottom	= NBGestorEscena::anchoPulgadasAEscena(_lobbyRoot.iScene, 0.10f);
	//Push content
	{
		STLobbyPanel* pnl = &_panels[ENLobbyPanel_MainLeft];
		NBMargenes marginsRow;
		marginsRow.left = marginsRow.right = 0.0f; //NBGestorEscena::anchoPulgadasAEscena(_lobbyRoot.iScene, 0.10f);
		marginsRow.top = 0.0f; marginsRow.bottom = NBGestorEscena::anchoPulgadasAEscena(_lobbyRoot.iScene, 0.10f);
		TSVisualHome* content = new(this) TSVisualHome(&_panels[ENLobbyPanel_MainLeft].colRoot, this, (_lastBox.xMax - _lastBox.xMin), (_lastBox.yMax - _lastBox.yMin), margins, marginsRow, NULL);
		NBASSERT(pnl->obj->getCurrentStackIdx() >=0 && pnl->obj->getCurrentStackIdx() < ENLobbyStack_Count)
		this->privContenidoPush(ENLobbyPanel_MainLeft, (ENLobbyStack)pnl->obj->getCurrentStackIdx(), content, content, content, true, false, true, false, ENSceneColStackPushMode_Add, ENSceneColMarginsMode_IncludeInLimits);
		content->liberar(NB_RETENEDOR_THIS); content = NULL;
	}
	//Focus center panel
	this->lobbyHiddeBehindPanel();
}

//

void AUSceneLobby::lobbyExecIconCmd(const SI32 iIcon){
	STLobbyPanel* panel = &_panels[ENLobbyPanel_MainLeft];
	panel->obj->execBarIconCmd(iIcon);
}

void AUSceneLobby::lobbyApplySearchFilter(const char** wordsLwr, const SI32 wordsLwrSz){
	STLobbyPanel* panel = &_panels[ENLobbyPanel_MainLeft];
	panel->obj->applySearchFilter(wordsLwr, wordsLwrSz);
}

//IPopListener
void AUSceneLobby::popOptionSelected(void* param, void* ref, const char* optionId){
	//
}

void AUSceneLobby::popFocusObtained(void* param, void* ref){
	//
}

void AUSceneLobby::popFocusLost(void* param, void* ref){
	//
}

//IReqPopListener

void AUSceneLobby::reqPopEnded(const UI64 reqId, void* param, const STTSClientChannelRequest* req){
	//
}

//

void AUSceneLobby::homeStreamsSourceSelected(TSVisualHome* obj, STTSClientRequesterDeviceRef src, const char* srcRunId){
	//PRINTF_INFO("homeStreamsSourceSelected.\n");
	NBMargenes margins;
	margins.top		= margins.left = margins.right = 0.0f;
	margins.bottom	= NBGestorEscena::anchoPulgadasAEscena(_lobbyRoot.iScene, 0.10f);
	//Push content
	{
		STLobbyPanel* pnl = &_panels[ENLobbyPanel_MainLeft];
		NBMargenes marginsRow;
		marginsRow.left = marginsRow.right = 0.0f; //NBGestorEscena::anchoPulgadasAEscena(_lobbyRoot.iScene, 0.10f);
		marginsRow.top = 0.0f; marginsRow.bottom = NBGestorEscena::anchoPulgadasAEscena(_lobbyRoot.iScene, 0.10f);
		TSVisualViewer* content = new(this) TSVisualViewer(&_panels[ENLobbyPanel_MainLeft].colRoot, NULL, src, srcRunId, (_lastBox.xMax - _lastBox.xMin), (_lastBox.yMax - _lastBox.yMin), margins, marginsRow, NULL);
		NBASSERT(pnl->obj->getCurrentStackIdx() >=0 && pnl->obj->getCurrentStackIdx() < ENLobbyStack_Count)
		this->privContenidoPush(ENLobbyPanel_MainLeft, (ENLobbyStack)pnl->obj->getCurrentStackIdx(), content, content, content, true, false, true, false, ENSceneColStackPushMode_Add, ENSceneColMarginsMode_IncludeInLimits);
		content->liberar(NB_RETENEDOR_THIS); content = NULL;
	}
	//Focus center panel
	this->lobbyHiddeBehindPanel();
}

//ITSVisualRespsFormsListener

//Records

void AUSceneLobby::recordChanged(void* param, const char* relRoot, const char* relPath){
	AUSceneLobby* obj = (AUSceneLobby*)param;
	if(NBString_strIsEqual(relRoot, "client/_root/") && NBString_strIsEqual(relPath, "_current")){
		//Sync from root
		obj->privSyncFromRootRecord();
	}
}

void AUSceneLobby_addAttachVerKeyIfNew(STNBArray* arr, const UI32 attachId, const UI32 version, STTSCypherDataKey* encKey){
	BOOL fnd = FALSE;
	SI32 i; for(i = 0; i < arr->use; i++){
		STLobbyMeetAttachVerState* v = NBArray_itmPtrAtIndex(arr, STLobbyMeetAttachVerState, i);
		if(v->attachId == attachId && v->version == version){
			if(TSCypherDataKey_isEmpty(&v->encKey) && !TSCypherDataKey_isEmpty(encKey)){
				NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &v->encKey, sizeof(v->encKey));
				NBStruct_stClone(TSCypherDataKey_getSharedStructMap(), encKey, sizeof(*encKey), &v->encKey, sizeof(v->encKey));
			}
			fnd = TRUE;
		}
	}
	if(!fnd){
		STLobbyMeetAttachVerState v;
		NBMemory_setZeroSt(v, STLobbyMeetAttachVerState);
		v.attachId	= attachId;
		v.version	= version;
		NBStruct_stClone(TSCypherDataKey_getSharedStructMap(), encKey, sizeof(*encKey), &v.encKey, sizeof(v.encKey));
		NBArray_addValue(arr, v);
	}
}

STTSCypherDataKey* AUSceneLobby_getAttachVerKey(STTSClient* coreClt, STNBArray* arr, const UI32 attachId, const UI32 version){
	STTSCypherDataKey* r = NULL;
	SI32 i; for(i = 0; i < arr->use; i++){
		STLobbyMeetAttachVerState* v = NBArray_itmPtrAtIndex(arr, STLobbyMeetAttachVerState, i);
		if(v->attachId == attachId && v->version == version){
			if(TSCypherDataKey_isEmpty(&v->plainKey) && !TSCypherDataKey_isEmpty(&v->encKey)){
				if(v->cypherJob == NULL){
					//Start new decrypt job
					v->cypherJob = NBMemory_allocType(STTSCypherJobRef);
					TSCypherJob_init(v->cypherJob);
					TSCypherJob_openTaskWithKey(v->cypherJob, "allLoobyAttachVerData", ENTSCypherTaskType_Decrypt, &v->encKey);
					TSCypher_addAsyncJob(TSClient_getCypher(coreClt), v->cypherJob);
				} else {
					const ENTSCypherJobState state = TSCypherJob_getState(v->cypherJob);
					if(state == ENTSCypherJobState_Done || state == ENTSCypherJobState_Error){
						if(state == ENTSCypherJobState_Done){
							const STTSCypherTask* t = TSCypherJob_getTask(v->cypherJob, "allLoobyAttachVerData");
							if(t != NULL){
								if(!TSCypherDataKey_isEmpty(&t->keyPlain)){
									NBStruct_stRelease(TSCypherDataKey_getSharedStructMap(), &v->plainKey, sizeof(v->plainKey));
									NBStruct_stClone(TSCypherDataKey_getSharedStructMap(), &t->keyPlain, sizeof(t->keyPlain), &v->plainKey, sizeof(v->plainKey));
								}
							}
						}
						TSCypherJob_release(v->cypherJob);
						NBMemory_free(v->cypherJob);
						v->cypherJob = NULL;
					}
				}
			}
			r = &v->plainKey; //Already decrypted key
			break;
		}
	}
	return r;
}

//

void AUSceneLobby::tickAnimacion(float segsTranscurridos){
#	ifdef LIBRO_DEBUG_MOSTRAR_MARCAS_PANEL_VISIBLE
	//Animar cantidad de conexiones del ultimo segundo
	{
		_dbgTxtCantConexionesSegsAcum += segsTranscurridos;
		if(_dbgTxtCantConexionesSegsAcum > 0.5f){
			AUCadenaMutable8* strTmp = new(ENMemoriaTipo_Temporal) AUCadenaMutable8();
			strTmp->agregarConFormato("%d / %d - %.1fKu %.1fKd - %d / %d %.1fKu %.1fKd"
									  , NBGestorJAR::conexionesCantTotal(), NBGestorJAR::conexionesCantMax(), (float) NBGestorJAR::conexionesBytesEnviados() / 1024.0f, (float) NBGestorJAR::conexionesBytesRecibidos() / 1024.0f
									  , NBGestorJAR::conexionesCantTotalEnSegUlt(), NBGestorJAR::conexionesCantMaxEnSegUlt(), (float) NBGestorJAR::conexionesBytesEnviadosEnSegUlt() / 1024.0f, (float) NBGestorJAR::conexionesBytesRecibidosEnSegUlt() / 1024.0f);
			if(!strTmp->esIgual(_dbgTxtCantConexiones->texto())){
				_dbgTxtCantConexiones->establecerTexto(strTmp->str());
			}
			strTmp->liberar(NB_RETENEDOR_THIS);
			strTmp = NULL;
			//
			_dbgTxtCantConexionesSegsAcum = 0.0f;
		}
	}
#	endif
	//Panel scrolls
	{
		SI32 ammChanged = 0;
		SI32 i; for(i = 0; i < ENLobbyPanelGrp_Count; i++){
			STLobbyPanelGrp* grp = &_panelGrps[i];
			if(grp->anim.secsDur > 0.0f){
				BOOL hasTouchActive = FALSE;
				//Analyze active touch
				if(_touch.first != NULL && _touch.iPanel >= 0 && _touch.iPanel < ENLobbyPanel_Count){
					const STLobbyPanel* pnl = &_panels[_touch.iPanel];
					hasTouchActive = (pnl->iGrp == i ? TRUE : FALSE);
				}
				//Apply delta
				if(!hasTouchActive){
					grp->anim.secsCur += segsTranscurridos;
					{
						float rel = (grp->anim.secsCur / grp->anim.secsDur);
						if(rel >= 1.0){
							grp->anim.secsCur = 0.0f;
							grp->anim.secsDur = 0.0f;
							rel = 1.0;
						}
						//Apply deltaX
						{
							float deltaIn = 0.0f, deltaOut = 0.0f, deltaStart = grp->anim.xDeltaStart;
							//if(i == ENLobbyPanelGrp_Main){
							//	STLobbyPanelGrp* grpBhnd = &_panelGrps[ENLobbyPanelGrp_Behind];
							//	deltaIn = 0.0f;
							//	deltaOut = grpBhnd->width;
							//}
							if(grp->anim.animIn){
								grp->xDelta = (deltaStart + ((deltaIn - deltaStart) * rel * rel));
							} else {
								const float relInv = (1.0f - rel);
								grp->xDelta = (deltaStart + ((deltaOut - deltaStart) * (1.0f - (relInv * relInv))));
							}
							ammChanged++;
						}
					}
				}
			}
		}
		if(ammChanged > 0){
			this->privUpdatePanelsPos();
		}
	}
	//Update top-bar
	{
		UI32 connsAccumSec = 0;
		{
			_stats.secsAccum	+= segsTranscurridos;
			{
				const BOOL resetAccums = (_stats.secsAccum >= 1.0f ? TRUE : FALSE);
				connsAccumSec = TSClient_getStatsTryConnAccum(_lobbyRoot.coreClt, resetAccums);
				if(resetAccums){
					_stats.secsAccum = 0.0f;
				}
			}
		}
		//Title
		{
			STLobbyPanel* panel = &_panels[ENLobbyPanel_MainLeft];
			STTSSceneBarStatus barDef;
			NBMemory_setZeroSt(barDef, STTSSceneBarStatus);
			//Title and right icon
			{
				if(panel->obj->getStatusBarDef(&barDef)){
					//Apply orientation
					if((barDef.orientMask & ENAppOrientationBits_All) != 0){
						_orient.mask = (barDef.orientMask & ENAppOrientationBits_All);
					}
					if((barDef.orientPref & ENAppOrientationBits_All) != 0){
						_orient.pref = (ENAppOrientationBit)(barDef.orientPref & ENAppOrientationBits_All);
					}
					if((barDef.orientApplyOnce & ENAppOrientationBits_All) != 0){
						_orient.orientApplyOnce = (ENAppOrientationBit)(barDef.orientApplyOnce & ENAppOrientationBits_All);
					}
				}
			}
			//Force left icon
			{
				STLobbyPanel* pnl = &_panels[ENLobbyPanel_MainLeft];
				barDef.leftIconId = (pnl->obj->getCurrentStackSz() > 1 ? ENSceneBarTopIcon_Back : pnl->obj->getCurrentStackIdx() > 0 ? ENSceneBarTopIcon_Home : ENSceneBarTopIcon_Count /*ENSceneBarTopIcon_Settings*/);
			}
			//
			_bars.top->setStatusDef(barDef);
			NBStruct_stRelease(DRSceneBarStatus_getSharedStructMap(), &barDef, sizeof(barDef));
		}
		//Substatus
		{
			if(!TSClient_syncIsReceiving(_lobbyRoot.coreClt)){
				const char* strLoc = TSClient_getStr(_lobbyRoot.coreClt, "topStatusConnecting", "connecting...");
				const STNBColor8 bgColor = TSColors::colorDef(ENTSColor_GreenLight)->normal;
				const STNBColor8 fgColor = TSColors::colorDef(ENTSColor_Green)->normal;
				_bars.top->setSubstatus(ENSceneSubbarIcon_Sync, strLoc, bgColor, fgColor);
				_bars.top->setSubstatusVisible(TRUE);
			} else if(!TSClient_syncIsFirstDone(_lobbyRoot.coreClt)){
				const char* strLoc = TSClient_getStr(_lobbyRoot.coreClt, "topStatusSyncing", "syncing...");
				const STNBColor8 bgColor = TSColors::colorDef(ENTSColor_GreenLight)->normal;
				const STNBColor8 fgColor = TSColors::colorDef(ENTSColor_Green)->normal;
				_bars.top->setSubstatus(ENSceneSubbarIcon_Sync, strLoc, bgColor, fgColor);
				_bars.top->setSubstatusVisible(TRUE);
			} else {
				//Connected
				_bars.top->setSubstatusVisible(FALSE);
			}
		}
		//Detect top bar changes
		{
			const NBCajaAABB box = _bars.top->cajaAABBLocal();
			NBCajaAABB delta;
			delta.xMin	= (box.xMin - _bars.topBox.xMin);
			delta.xMax	= (box.xMax - _bars.topBox.xMax);
			delta.yMin	= (box.yMin - _bars.topBox.yMin);
			delta.yMax	= (box.yMax - _bars.topBox.yMax);
			if(delta.xMin <= -1.0f || delta.xMin >= 1.0f || delta.xMax <= -1.0f || delta.xMax >= 1.0f || delta.yMin <= -1.0f || delta.yMin >= 1.0f || delta.yMax <= -1.0f || delta.yMax >= 1.0f){
				_bars.topBox = box;
				this->privOrganizarContenido();
			}
		}
	}
}

//TOUCHES

void AUSceneLobby::touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto){
	//
}

void AUSceneLobby::touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	//
}

void AUSceneLobby::touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto){
	//
}

//TECLAS
bool AUSceneLobby::teclaPresionada(SI32 codigoTecla){
	return false;
}

bool AUSceneLobby::teclaLevantada(SI32 codigoTecla){
	return false;
}

bool AUSceneLobby::teclaEspecialPresionada(SI32 codigoTecla){
	bool r = false;
	switch (codigoTecla) {
		case AU_TECLA_REGRESAR:
			this->lobbyBack(FALSE);
			r = TRUE;
			break;
		/*case AU_TECLA_MENU:
			this->lobbyFocusBehindPanel();
			r = true;
			break;*/
		default:
			break;
	}
	return r;
}

//Sync from root

void AUSceneLobby::privSyncFromRootRecord(){
	/*UI32 countPatients = 0;
	//
	{
		STNBStorageRecordRead ref = TSClient_getStorageRecordForRead(_lobbyRoot.coreClt, "client/_root/", "_current", NULL, TSClientRoot_getSharedStructMap());
		if(ref.data != NULL){
			STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
			if(priv != NULL){
				if(priv->patients.patients != NULL && priv->patients.patientsSz > 0){
					UI32 i; for(i = 0; i < priv->patients.patientsSz; i++){
						const STTSDevicePatient* p = &priv->patients.patients[i];
						if(!NBString_strIsEmpty(p->patientId)){
							countPatients++;
						}
					}
				}
			}
		}
		TSClient_returnStorageRecordFromRead(_lobbyRoot.coreClt, &ref);
	}
	//
	if(countPatients == 0){
		_lobbyRoot.scenes->loadInfoSequence(ENSceneAction_Push, ENSceneInfoMode_Register, ENSceneTransitionType_Logo, TS_TRANSITION_PARTIAL_ANIM_DURATION);
	} else {
		//Update bottom bar style
		const BOOL havePatients	= (countPatients > 0);
		_bars.btm->updateStyle(havePatients, TRUE);
	}*/
}

//

AUOBJMETODOS_CLASESID_UNICLASE(AUSceneLobby)
AUOBJMETODOS_CLASESNOMBRES_UNICLASE(AUSceneLobby, "AUSceneLobby")
AUOBJMETODOS_CLONAR_NULL(AUSceneLobby)


