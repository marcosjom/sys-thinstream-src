//
//  TSVisualHome.h
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#ifndef TSVisualHome_h
#define TSVisualHome_h

#include "nb/core/NBCallback.h"
#include "AUEscenaContenedor.h"
#include "core/client/TSClient.h"
#include "visual/AUSceneContentI.h"
#include "visual/ILobbyListnr.h"
#include "visual/gui/TSVisualRowsGrp.h"
#include "visual/gui/TSVisualTag.h"
#include "visual/home/TSVisualHomeSourceRow.h"

typedef struct STVisualHomeRow_ {
	AUIButton*				btn;		//Container
	TSVisualHomeSourceRow*	content;	//Internal content (inside button)
} STVisualHomeRow;

typedef struct STVisualHomeGrp_ {
	BOOL				inViewPort;
	TSVisualRowsGrp*	obj;	//Internal content (inside button)
} STVisualHomeGrp;

class TSVisualHome;

class ITSVisualHomeListener {
	public:
		virtual ~ITSVisualHomeListener(){
			//
		}
		//
		virtual void homeStreamsSourceSelected(TSVisualHome* obj, STTSClientRequesterDeviceRef src, const char* srcRunId) = 0;
};

class TSVisualHome: public AUEscenaContenedorLimitado, public AUEscenaListaItemI, public AUSceneContentI, public NBAnimador, public IEscuchadorTouchEscenaObjeto, public IEscuchadorIBoton {
	public:
		TSVisualHome(const STLobbyColRoot* root, ITSVisualHomeListener* lstnr, const float anchoUsar, const float altoUsar, const NBMargenes margins, const NBMargenes marginsRows, AUAnimadorObjetoEscena* animObjetos);
		virtual				~TSVisualHome();
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
		ITSVisualHomeListener*	_lstnr;
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
			STNBArray			array;	//STVisualHomeGrp
			AUEscenaContenedor*	layer;
		} _grps;
		//
		void				privSyncAddGrpAndBtn(const char* grpName, AUIButton* btn, STNBArray* pendGrps, STNBArray* pendRows);
		void				privSyncAddGrpToEnd(const char* grpName, STNBArray* pendGrps);
		void				privSyncAddBtn(AUIButton* btn, STNBArray* pendRows);
		void				privSyncAddSource(const float width, STNBArray* pendRows, STTSClientRequesterDeviceRef src);
		void				privSyncSources(const float width);
		void				privOrganizarContenido(const float ancho, const float alto, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim);
};

#endif /* TSVisualHomes_h */
