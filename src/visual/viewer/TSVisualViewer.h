//
//  TSVisualViewer.h
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#ifndef TSVisualViewer_h
#define TSVisualViewer_h

#include "nb/core/NBCallback.h"
#include "AUEscenaContenedor.h"
#include "core/client/TSClient.h"
#include "visual/AUSceneContentI.h"
#include "visual/ILobbyListnr.h"
#include "visual/gui/TSVisualRowsGrp.h"
#include "visual/gui/TSVisualTag.h"
#include "visual/viewer/TSVisualViewerStream.h"

typedef struct STVisualViewerRow_ {
	//uid
	struct {
		//Multiple paths and depth can be used to reach the same runId+streamId combination
		char*		runId;
		char*		streamId;
	} uid;
	AUIButton*				btn;		//Container
	TSVisualViewerStream*	content;	//Internal content (inside button)
} STVisualViewerRow;

typedef struct STVisualViewerGrp_ {
	BOOL				inViewPort;
	TSVisualRowsGrp*	obj;	//Internal content (inside button)
} STVisualViewerGrp;

class TSVisualViewer;

class ITSVisualViewerListener {
	public:
		virtual ~ITSVisualViewerListener(){
			//
		}
		//
		virtual void homeStreamsSourceSelected(TSVisualViewer* obj, STTSClientRequesterDeviceRef src, const char* srcRunId) = 0;
};

class TSVisualViewer: public AUEscenaContenedorLimitado, public AUEscenaListaItemI, public AUSceneContentI, public NBAnimador, public IEscuchadorTouchEscenaObjeto, public IEscuchadorIBoton {
	public:
		TSVisualViewer(const STLobbyColRoot* root, ITSVisualViewerListener* lstnr, STTSClientRequesterDeviceRef src, const char* srcRunId, const float anchoUsar, const float altoUsar, const NBMargenes margins, const NBMargenes marginsRows, AUAnimadorObjetoEscena* animObjetos);
		virtual				~TSVisualViewer();
		//AUEscenaListaItemI
		AUEscenaObjeto*		itemObjetoEscena();
		STListaItemDatos	refDatos();
		void				establecerRefDatos(const SI32 type, const SI32 value);
		void				organizarContenido(const float width, const float height, const bool notifyHeightChange, AUAnimadorObjetoEscena* animator, const float secsAnim);
		//
		static void			childResized(void* param);
		//AUSceneContentI
		void				setResizedCallback(STNBCallback* parentCallback);
		void				setTouchInheritor(AUEscenaObjeto* touchInheritor);
		BOOL				getStatusBarDef(STTSSceneBarStatus* dst);
		BOOL				execBarBackCmd();
		void				execBarIconCmd(const SI32 iIcon);
		void				applySearchFilter(const char** wordsLwr, const SI32 wordsLwrSz);
		void				applyAreaFilter(const STNBAABox outter, const STNBAABox inner);
		BOOL				applyScaledSize(const STNBSize sizeBase, const STNBSize sizeScaled, const bool notifyChange, AUAnimadorObjetoEscena* animator, const float secsAnim);
		STNBSize			getScaledSize();
		ENDRScrollSideH		getPreferedScrollH();
		ENDRScrollSideV		getPreferedScrollV();
		void*				anchorCreate(const STNBPoint lclPos);
		void*				anchorClone(const void* anchorRef);
		void*				anchorToFocusClone(float* dstSecsDur);
		void				anchorToFocusClear();
		STNBAABox			anchorGetBox(void* anchorRef, float* dstScalePref);
		void				anchorDestroy(void* anchorRef);
		void				scrollToTop(const float secs);
		//
		void				tickAnimacion(float segsTranscurridos);
		//
		static void			recordChanged(void* param, const char* relRoot, const char* relPath);
		static void			requesterSourcesUpdated(void* param, STTSClientRequesterRef reqRef);
		static void			requesterSourceConnStateChanged(void* param, const STTSClientRequesterDeviceRef source, const ENTSClientRequesterDeviceConnState state);
		static void			requesterSourceDescChanged(void* param, const STTSClientRequesterDeviceRef source);
		//Buttons
		void				botonPresionado(AUIButton* obj);
		void				botonAccionado(AUIButton* obj);
		AUEscenaObjeto*		botonHerederoParaTouch(AUIButton* obj, const NBPunto &posInicialEscena, const NBPunto &posActualEscena);
		//Touches
		void				touchIniciado(STGTouch* touch, const NBPunto &posTouchEscena, AUEscenaObjeto* objeto);
		void				touchMovido(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
		void				touchFinalizado(STGTouch* touch, const NBPunto &posInicialEscena, const NBPunto &posAnteriorEscena, const NBPunto &posActualEscena, AUEscenaObjeto* objeto);
		//
		AUOBJMETODOS_CLASESID_DEFINICION
		AUOBJMETODOS_CLASESNOMBRES_DEFINICION
		AUOBJMETODOS_CLONAR_DEFINICION
	protected:
		STLobbyColRoot			_root;
		ITSVisualViewerListener* _lstnr;
		STNBCallback			_prntResizedCallbck;
		STNBCallback			_chldResizedCallbck;
		SI32					_chldResizedCount;
		AUEscenaObjeto*			_touchInheritor;
		STListaItemDatos		_refDatos;
		float					_iconWidthRef;
		float					_anchoUsar;
		float					_altoUsar;
		float					_yTopLast;
		STNBAABox				_lastFilterInner;
		STNBAABox				_lastFilterOuter;
		NBMargenes				_margins;
		NBMargenes				_marginsRows;
		AUAnimadorObjetoEscena* _animObjetos;
		//
		AUFuenteRender*			_fntNames;
		AUFuenteRender*			_fntExtras;
		//No authtorized message
		struct {
			AUEscenaContenedor*	layer;
			AUEscenaSprite*		ico;
			AUEscenaTexto*		text;
		} _add;
		//Data
		struct {
			STTSClientRequesterDeviceRef src;
			STNBString srcRunId;
		} _data;
		//Buttons
		struct {
			//info
			struct {
				AUIButton*		termsAndConds;
				AUIButton*		privacy;
				AUIButton*		about;
			} info;
		} _buttons;
		//Sync
		struct {
			BOOL				isDirty;
		} _sync;
		struct {
			STNBArray			array;
		} _rows;
		struct {
			STNBArray			array;	//STVisualViewerGrp
			AUEscenaContenedor*	layer;
		} _grps;
		//
		void				privSyncAddGrpAndBtn(const char* grpName, AUIButton* btn, STNBArray* pendGrps, STNBArray* pendRows);
		void				privSyncAddGrpToEnd(const char* grpName, STNBArray* pendGrps);
		void				privSyncAddBtn(AUIButton* btn, STNBArray* pendRows);
		void				privSyncAddStreamOnce(const float width, STNBArray* pendRows, const STTSStreamsServiceDesc* service, const STTSStreamDesc* strmDesc);
		void				privSyncAddStreamsRecurive(const float width, STNBArray* pendRows, STTSClientRequesterDeviceRef rootSrcRef, const STTSStreamsServiceDesc* srcDesc);
		void				privSyncStreams(const float width);
		
	
		void				privOrganizarContenido(const float ancho, const float alto, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim);
};

#endif /* TSVisualViewers_h */
