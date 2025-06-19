//
//  TSVisualHomeSourceRow.h
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#ifndef TSVisualHomeSourceRow_h
#define TSVisualHomeSourceRow_h

#include "core/client/TSClient.h"
#include "AUEscenaContenedor.h"
#include "visual/TSFonts.h"
#include "visual/ILobbyListnr.h"
#include "visual/gui/TSVisualStatus.h"

class TSVisualHomeSourceRow;

class ITSVisualHomeSourceRowListener {
	public:
		virtual ~ITSVisualHomeSourceRowListener(){
			//
		}
		virtual void dummy(TSVisualHomeSourceRow* obj) = 0;
};

class TSVisualHomeSourceRow: public AUEscenaContenedorLimitado, public AUEscenaListaItemI, public NBAnimador, public IPopListener {
	public:
		TSVisualHomeSourceRow(const STLobbyColRoot* root, STTSClientRequesterDeviceRef src, const float anchoUsar, const float altoUsar, ITSVisualHomeSourceRowListener* lstnr, STNBCallback* parentCallback);
		virtual				~TSVisualHomeSourceRow();
		//
		BOOL				isSameSource(STTSClientRequesterDeviceRef src) const;
		STTSClientRequesterDeviceRef getSourceRef() const;
		BOOL				isSearchMatch(const char** wordsLwr, const SI32 wordsLwrSz);
		//AUEscenaListaItemI
		AUEscenaObjeto*		itemObjetoEscena();
		STListaItemDatos	refDatos();
		void				establecerRefDatos(const SI32 tipo, const SI32 valor);
		void				organizarContenido(const float ancho, const float altoVisible, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim);
		//IPopListener
		void				popOptionSelected(void* param, void* ref, const char* optionId);
		void				popFocusObtained(void* param, void* ref);
		void				popFocusLost(void* param, void* ref);
		//
		void				setResizedCallback(STNBCallback* parentCallback);
		void				setTouchInheritor(AUEscenaObjeto* touchInheritor);
		//
		void				tickAnimacion(float segsTranscurridos);
		//
		static void			recordChanged(void* param, const char* relRoot, const char* relPath);
		static void			requesterSourceConnStateChanged(void* param, const STTSClientRequesterDeviceRef source, const ENTSClientRequesterDeviceConnState state);
		static void			requesterSourceDescChanged(void* param, const STTSClientRequesterDeviceRef source);
		//
		AUOBJMETODOS_CLASESID_DEFINICION
		AUOBJMETODOS_CLASESNOMBRES_DEFINICION
		AUOBJMETODOS_CLONAR_DEFINICION
	protected:
		STLobbyColRoot			_root;
		STListaItemDatos		_refDatos;
		AUEscenaObjeto*			_touchInheritor;
		float					_anchoUsar;
		float					_altoUsar;
		SI32					_yTopLast;	//integer to allow an error-margin
		ITSVisualHomeSourceRowListener* _lstnr;
		STNBCallback			_prntResizedCallbck;
		//data
		struct {
			BOOL				isConnDirty;
			BOOL				isDescDirty;
			STTSClientRequesterDeviceRef src;
		} _data;
		//gui
		struct {
			NBMargenes			margins;
			float				marginI;
			AUEscenaTexto*		server;
			AUEscenaTexto*		port;
			//connecting
			struct {
				float			rot;
				AUEscenaSprite*	ico;
			} connecting;
			//counts
			struct {
				AUEscenaTexto*	live;
				AUEscenaTexto*	storage;
			} counts;
		} _gui;
		//
		void				privSync();
		void				privOrganizarContenido(const float ancho, const float alto, const bool notificarCambioAltura);
};

#endif /* TSVisualHomeSourceRow_h */
