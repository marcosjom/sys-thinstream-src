//
//  TSVisualRowsGrp.hpp
//  thinstream
//
//  Created by Marcos Ortega on 8/3/19.
//

#ifndef TSVisualRowsGrp_hpp
#define TSVisualRowsGrp_hpp

#include "AUEscenaContenedor.h"
#include "visual/AUSceneContentI.h"

typedef enum ENTSVisualRowsGrpBitsMsk_ {
	ENTSVisualRowsGrpBitsMsk_Empty		= 0,
	ENTSVisualRowsGrpBitsMsk_HideLines	= (0x1 << 0),
	ENTSVisualRowsGrpBitsMsk_All		= (0xFFFFFF)
} ENTSVisualRowsGrpBitsMsk;

typedef struct STVisualRowsGrp_ {
	UI32						grpID;
	STNBString					grpName;
	AUEscenaContenedor*			layerBtns;
	AUEscenaContenedor*			layerLines;
	AUEscenaSprite*				bg;
	AUEscenaTexto*				title;
} STVisualRowsGrp;

typedef struct STVisualRowsGrpItmOpt_ {
	STNBString		optionId;
	AUIButton*		btn;
} STVisualRowsGrpItmOpt;

typedef struct STVisualRowsGrpItm_ {
	AUEscenaContenedorLimitado*	lyr;
	AUEscenaObjeto*				obj;
	AUEscenaListaItemI*			itf;
	AUEscenaFigura*				line;
	BOOL						synced;		//used for sync* methods
	struct {
		SI32					dragDepth;
		float					relVisible;	//rel pan to make options visible
		float					width;		//total width of options
		STNBArray				arr;		//STVisualRowsGrpItmOpt
	} options;
} STVisualRowsGrpItm;

class TSVisualRowsGrp: public AUEscenaContenedorLimitado, public AUEscenaListaItemI, public NBAnimador {
	public:
		TSVisualRowsGrp(const SI32 iScene, const float anchoUsar, const float altoUsar, AUAnimadorObjetoEscena* animObjetos, AUFuenteRender* fntNames, AUFuenteRender* fntExtras, const char* grpName, const UI32 bitsMask);
		virtual				~TSVisualRowsGrp();
		//
		UI32				getGrpID() const;
		const char*			getGrpName() const;
		void				setGrpID(const UI32 grpID);
		void				setGrpName(const char* grpName);
		//
		SI32				getRowsCount() const;
		//
		void				setTitlePos(const BOOL fixed, const float xPos, const ENNBTextLineAlignH align);
		void				setTitleVisible(const BOOL visible);
		void				setLinesVisible(const BOOL visible);
		//Sync
		void				syncStart();
		SI32				syncAdd(AUEscenaObjeto* obj, AUEscenaListaItemI* itf);
		SI32				syncAddAtIdx(AUEscenaObjeto* obj, AUEscenaListaItemI* itf, const SI32 idx);
		void				syncEndByRemovingUnsynced();
		//Populate
		void				rowsEmpty();
		SI32				rowsAdd(AUEscenaObjeto* obj, AUEscenaListaItemI* itf);
		SI32				rowsAddAtIndex(AUEscenaObjeto* obj, AUEscenaListaItemI* itf, const SI32 idx);
		SI32				rowsIndexByObj(AUEscenaObjeto* obj);
		BOOL				rowsRemoveByObj(AUEscenaObjeto* obj);
		void				rowAddOption(const SI32 rowIdx, const char* optionId, AUIButton* optBtn);
		BOOL				rowDragOptionStartedByObj(AUEscenaObjeto* rowObj);
		BOOL				rowDragOptionApplyByObj(AUEscenaObjeto* rowObj, const float deltaX, const BOOL onlySimulate);
		BOOL				rowDragOptionEndedByObj(AUEscenaObjeto* rowObj);
		SI32				rowsVisibleCount() const;
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
		AUEscenaObjeto*		itemObjetoEscena();
		STListaItemDatos	refDatos();
		void				establecerRefDatos(const SI32 tipo, const SI32 valor);
		void				organizarContenido(const float ancho, const float altoVisible, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim);
		//
		AUOBJMETODOS_CLASESID_DEFINICION
		AUOBJMETODOS_CLASESNOMBRES_DEFINICION
		AUOBJMETODOS_CLONAR_DEFINICION
	protected:
		SI32					_iScene;
		STListaItemDatos		_refDatos;
		float					_anchoUsar;
		float					_altoUsar;
		AUAnimadorObjetoEscena* _animObjetos;
		AUFuenteRender*			_fntNames;
		AUFuenteRender*			_fntExtras;
		//
		BOOL					_isAnimating;
		BOOL					_multipleOptionsRows;
		UI32					_bitsMask;
		struct {
			BOOL				isVisible;
			BOOL				isPosFixed;
			float				pos;
			ENNBTextLineAlignH	align;
		} _title;
		//
		struct {
			UI32				addedCount;	//To determine the order of synced items
		} _sync;
		//
		struct {
			AUEscenaContenedor*	lyr;
			AUEscenaSprite*		bg;
			AUEscenaTexto*		txt;
			AUEscenaFigura*		line;
		} _empty;
		//
		STVisualRowsGrp			_grp;
		STNBArray				_itms;	//STVisualRowsGrpItm
		SI32					_itmsVisibleCount;
		//
		void				privOrganizarContenido(const float ancho, const float alto, const bool notificarCambioAltura, AUAnimadorObjetoEscena* animator, const float secsAnim);
		void				privOrganizeOptions(STVisualRowsGrpItm* itm);
};

#endif /* TSVisualRowsGrp_hpp */
