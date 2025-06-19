//
//  TSVisualViewerStream.h
//  thinstream
//
//  Created by Marcos Ortega on 6/3/19.
//

#ifndef TSVisualViewerStream_h
#define TSVisualViewerStream_h

#include "core/client/TSClient.h"
#include "AUEscenaContenedor.h"
#include "visual/TSFonts.h"
#include "visual/ILobbyListnr.h"
#include "visual/gui/TSVisualStatus.h"

class TSVisualViewerStream;

class ITSVisualViewerStreamListener {
	public:
		virtual ~ITSVisualViewerStreamListener(){
			//
		}
		virtual void dummy(TSVisualViewerStream* obj) = 0;
};

class TSVisualViewerStream: public AUEscenaContenedorLimitado, public AUEscenaListaItemI, public NBAnimador, public IPopListener {
	public:
		TSVisualViewerStream(const STLobbyColRoot* root, const STTSStreamsServiceDesc* serviceDesc, const STTSStreamDesc* strmDesc, const float anchoUsar, const float altoUsar, ITSVisualViewerStreamListener* lstnr, STNBCallback* parentCallback);
		virtual				~TSVisualViewerStream();
		//
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
		void				sync(const STTSStreamsServiceDesc* serviceDesc, const STTSStreamDesc* strmDesc);
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
		ITSVisualViewerStreamListener* _lstnr;
		STNBCallback			_prntResizedCallbck;
		//data
		struct {
			STTSStreamsServiceDesc serviceDesc;
			STTSStreamDesc		strmDesc;
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
		void				privSync(const STTSStreamsServiceDesc* serviceDesc, const STTSStreamDesc* strmDesc);
		void				privOrganizarContenido(const float ancho, const float alto, const bool notificarCambioAltura);
};

#endif /* TSVisualViewerStream_h */
