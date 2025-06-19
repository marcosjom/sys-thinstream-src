//
//  AUSceneLobby.hpp
//  thinstream
//
//  Created by Marcos Ortega on 9/2/18.
//

#ifndef AUSceneLobby_hpp
#define AUSceneLobby_hpp

#include "IScenesListener.h"
#include "visual/ILobbyListnr.h"
#include "visual/AUSceneContentI.h"
#include "visual/AUSceneBarTop.h"
#include "visual/AUSceneBarBtm.h"
#include "visual/AUSceneColsPanel.h"
#include "visual/home/TSVisualHome.h"

#ifdef CONFIG_NB_INCLUIR_VALIDACIONES_ASSERT
//#	define LIBRO_DEBUG_MOSTRAR_MARCAS_PANEL_VISIBLE
#endif

#define LIBRO_PULGADAS_SEPARACION_BARRA_ESTADO 0.0f //(0.10f / 8.0f)

#define STR_LIBRO_CONTENIDO_TIPO(TIPO) (TIPO == ENLobbyPanel_MainLeft ? "Centro" : TIPO == ENLobbyPanel_Behind ? "MenuIzq" : "LibroContenido_???")

//Bars

typedef struct STSceneLobbyBar_ {
	ENSceneLobbyBar		uid;
	ENSceneLobbyBarPos	pos;
	BOOL				outOfScreen;
	BOOL				notVisible;
	NBCajaAABB			box;
	AUEscenaObjeto*		obj;
	AUEscenaListaItemI*	itf;
} STSceneLobbyBar;

struct STLibroResultadoBsq {
	UI32 idElem;
	float factorRelevancia;
	//
	float sumaFactores;
	UI32 vecesFactores;
};

struct STLibroContenidoDef {
	AUEscenaObjeto* fondo;
	AUEscenaObjeto* contenido;
};

typedef enum ENLobbyPanelGrp_ {
	//ENLobbyPanelGrp_Behind = 0,	//Panel behind
	ENLobbyPanelGrp_Main = 0,		//Main area
	ENLobbyPanelGrp_Count
} ENLobbyPanelGrp;

typedef struct STLobbyPanelGrp_ {
	float				xLeft;
	float				xDelta;
	float				width;
	AUEscenaContenedor*	lyr;
	struct {
		float			secsDur;
		float			secsCur;
		float			xDeltaStart;
		BOOL			animIn;
	} anim;
} STLobbyPanelGrp;

typedef struct STLobbyPanel_ {
	STLobbyColRoot		colRoot;
	//
	ENLobbyPanelGrp		iGrp;
	float				xLeft;
	float				xDelta;
	float				width;
	AUEscenaContenedor*	lyr;
	STSceneLobbyBar*	barTop;
	STSceneLobbyBar*	barBtm;
	AUSceneColsPanel*	obj;
#	ifdef LIBRO_DEBUG_MOSTRAR_MARCAS_PANEL_VISIBLE
	AUEscenaSprite*		dbgMarcaPanelVisible;
#	endif
} STLobbyPanel;

typedef struct STLobbyMeetAttachVerState_ {
	UI32				attachId;
	UI32				version;
	STTSCypherJobRef*	cypherJob;
	STTSCypherDataKey	encKey;		//my encKey
	STTSCypherDataKey	plainKey;	//
} STLobbyMeetAttachVerState;

typedef struct STLobbyMeetState_ {
	char*	meetingId;
	BOOL	isClosed;
	UI32	lastMsgId;				//includes system messages
	UI32	lastMsgIdHuman;			//only humans messages
	UI32	myLastMsgIdRead;		//includes system messages
	UI32	myLastMsgIdReadHuman;	//only humans messages
	//Data
	struct {
		BOOL				isSynced;
		STTSCypherDataKey	encKey;
		STTSCypherData		nameEnc;
		STNBString			name;
		BOOL				nameLoaded;
		STTSCypherJobRef*	cypherJob;
		float				secsPendToEval;
		STNBArraySorted		anns; //STTSAnnKey
	} data;
	//Vouchers
	struct {
		UI32				ammNewVouchsToNotify;
	} vouchers;
	//Attest auto-download
	struct {
		struct {
			UI32	attachId;
			UI32	attachVer;
			SI64	pos;
			SI64	size;
			UI64	timestamp;
			STNBString filename;
			STTSCypherDataKey encKey;
		} finl;
		float	secsPendToEval;
		STTSCypherJobRef*	cypherJob;
	} attest;
	//Anns upload
	struct {
		BOOL		allSent;
		struct {
			UI64	curReqId;
			struct {
				UI64	time;	//last attempt time
				BOOL	wasSuccess;	//last attempt was success?
			} last;
		} req;
	} annsPend;
	//AttachVers
	struct {
		STNBArray	arr;	//STLobbyMeetAttachVerState
	} attachsVers;
} STLobbyMeetState;

//

class AUSceneLobby;

typedef struct STLobbyUploadTask_ {
	AUSceneLobby*	obj;
	BOOL			addPreAuthKey;
	STNBFile		file;
	STNBString		filepath;
	STNBString		meetingId;
} STLobbyUploadTask;

void STLobbyUploadTask_free(STLobbyUploadTask* task);

//

class AUSceneLobby: public AUAppEscena, public NBAnimador, public IEscuchadorCambioPuertoVision, public IEscuchadorTouchEscenaObjeto, public ILobbyListnr, public IPopListener, public IReqPopListener, public ITSVisualHomeListener {
public:
	AUSceneLobby(STTSClient* coreClt, SI32 iScene, IScenesListener* scenes);
	virtual				~AUSceneLobby();
	//
	bool				escenaEnPrimerPlano();
	void				escenaColocarEnPrimerPlano();
	void				escenaQuitarDePrimerPlano();
	//Orientations
	void				escenaGetOrientations(UI32* dstMask, ENAppOrientationBit* dstPrefered, ENAppOrientationBit* dstToApplyOnce, BOOL* dstAllowAutoRotate);
	bool				escenaPermitidoGirarPantalla();
	bool				escenaEstaAnimandoSalida();
	void				escenaAnimarEntrada();
	void				escenaAnimarSalida();
	//
	void				appFocoExclusivoObtenido();
	void				appFocoExclusivoPerdido();
	void				appProcesarNotificacionLocal(const char* grp, const SI32 localId, const char* objTip, const SI32 objId);
	//
	void				puertoDeVisionModificado(const SI32 iEscena, const STNBViewPortSize before, const STNBViewPortSize after);
	//Records
	static void			recordChanged(void* param, const char* relRoot, const char* relPath);
	//
	void				tickAnimacion(float segsTranscurridos);
	//
	void				lobbyFocusBehindPanel();
	void				lobbyHiddeBehindPanel();
	void				lobbyFocusStack(const ENLobbyStack iStack, const BOOL allowPopAllAction);
	void				lobbyBack(const BOOL ignoreSubstacks);
	BOOL				lobbyTopBarGetStatusDef(STTSSceneBarStatus* dst);
	UI32				lobbyTopBarGetStatusRightIconMask();
	void				lobbyTopBarTouched();
	void				lobbyTopBarSubstatusTouched();
	void				lobbyPushTermsAndConds();
	void				lobbyPushPrivacy();
	void				lobbyPushHome();
	//
	void				lobbyExecIconCmd(const SI32 iIcon);
	void				lobbyApplySearchFilter(const char** wordsLwr, const SI32 wordsLwrSz);
	//IPopListener
	void				popOptionSelected(void* param, void* ref, const char* optionId);
	void				popFocusObtained(void* param, void* ref);
	void				popFocusLost(void* param, void* ref);
	//IReqPopListener
	void				reqPopEnded(const UI64 reqId, void* param, const STTSClientChannelRequest* req);
	//ITSVisualHomeListener
	void				homeStreamsSourceSelected(TSVisualHome* obj, STTSClientRequesterDeviceRef src, const char* srcRunId);
	//TOUCHES
	void				touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto);
	void				touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
	void				touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
	//TECLAS
	bool				teclaPresionada(SI32 codigoTecla);
	bool				teclaLevantada(SI32 codigoTecla);
	bool				teclaEspecialPresionada(SI32 codigoTecla);
	//
	AUOBJMETODOS_CLASESID_DEFINICION
	AUOBJMETODOS_CLASESNOMBRES_DEFINICION
	AUOBJMETODOS_CLONAR_DEFINICION
protected:
	STLobbyRoot			_lobbyRoot;
	//
	AUAnimadorObjetoEscena* _animObjetos;
	AUEscenaContenedor*	_lyrContent;
	AUEscenaContenedor*	_lyrDbg;
	AUEscenaContenedor*	_lyrBars;
	AUEscenaContenedor*	_lyrFront;
	//Bars (left, top, bottom, ...)
	struct {
		STSceneLobbyBar	data[ENSceneLobbyBar_Count];
		NBCajaAABB		topBox;
		AUSceneBarTop*	top;
		AUSceneBarBtm*	btm;
	} _bars;
	//Orientation
	struct {
		UI32				mask;
		ENAppOrientationBit	pref;
		ENAppOrientationBit	orientApplyOnce;	//Orientation to automatically apply when autorate is enabled)
	} _orient;
	//
	STLobbyPanelGrp		_panelGrps[ENLobbyPanelGrp_Count];
	STLobbyPanel		_panels[ENLobbyPanel_Count];
	//Stats
	struct {
		float			secsAccum;
	} _stats;
	//Energy saving (disabled)
	/*struct {
		float			secsToShowWhenOthers;	//When secondary devices are online
		float			secsToShowWhenAlone;	//When no secondary devices are online
		BOOL			trigered;
	} _energySaving;*/
	//scroll
	struct {
		STGTouch*		first;
		STNBPoint		start;
		SI32			iPanel;
		float			deltaXStart;
	} _touch;
	//
	bool				_enPrimerPlano;
	NBCajaAABB			_lastBox;			//Caja escena completa
	NBCajaAABB			_lastBoxUsable;		//Caja escena sin el espacio de la barra de estado y la publicidad
	//
#	ifdef LIBRO_DEBUG_MOSTRAR_MARCAS_PANEL_VISIBLE
	float				_dbgTxtCantConexionesSegsAcum;
	AUEscenaTexto*		_dbgTxtCantConexiones;
#	endif
#	ifdef CONFIG_NB_INCLUIR_VALIDACIONES_ASSERT
	AUCadenaMutable8*	_infoDebugMostrada;
#	endif
	//
	void				privUpdatePanelsWidths();
	void				privOrganizarContenido();
	void				privOrganizeBars(const BOOL animate);
	void				privUpdatePanelsPos();
	void				privOrganizarSegunCambioAdornosInferiores();
	void				privAnimarEntradaEscena();
	void				privAnimarSalidaEscena();
	void				privLibroMostrarTerminoDetalles(const UI32 pIdTermino, AUArregloNativoP<UI32>* lstIdsTermsMostrando, AUArregloNativoP<UI32>* idComentariosMostrar, const bool animarEntradaNuevo, const bool animarSalidaUltimo, const bool swapConUltimoContenido);
	//
	void				privCalculateLimits();
	void				privScrollTrasladarContenido(const ENLobbyPanel contenidoEnScroll, const float xTraslado, const bool moverAccesorios, const bool esScrollManual);
	//
	void				privContenidoPush(const ENLobbyPanel iCol, const ENLobbyStack iStack, AUEscenaObjeto* objAgregar, AUSceneContentI* itfCol, AUEscenaListaItemI* itfItm, const bool actEstadoBarraSup, const bool animarEntradaNuevo, const bool animarSalidaUltimo, const bool intercambiarConUltimo, const ENSceneColStackPushMode pushMode, const ENSceneColMarginsMode marginsMode);
	bool				privContenidoPop(const ENLobbyPanel iCol);
	void				privActualizarBarraInfSegunObjTipo(AUObjeto* obj);
	void				privMostrarContenidoInicial();
	//Sync from root
	void				privSyncFromRootRecord();
};

#endif /* AUSceneLobby_hpp */
