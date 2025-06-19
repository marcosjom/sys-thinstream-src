//
//  AUScenesTransition.hpp
//  thinstream-test
//
//  Created by Marcos Ortega on 8/5/18.
//

#ifndef AUScenesTransition_hpp
#define AUScenesTransition_hpp

#include "AUAppTransicion.h"
#include "AUAppEscenasAdmin.h"
#include "nb/2d/NBBezier3.h"

//Etapas de la transicion de escena con color de fondo solido
typedef enum ENSceneTransitionStage_ {
	ENSceneTransitionStage_Ninguna = -1,
	ENSceneTransitionStage_AparecerIni = 0,
	ENSceneTransitionStage_Aparecer,
	ENSceneTransitionStage_LiberarViejo,
	ENSceneTransitionStage_CrearNuevoLiberarRecursosSinUso,
	ENSceneTransitionStage_Desaparecer,
	ENSceneTransitionStage_DesaparecerFin
} ENSceneTransitionStage;

typedef struct STScenesTransition_ {
	ENSceneTransitionStage	etapaCarga;
	bool					animarEntrada;
	bool					permitidoGirarPantallaAntesDeIniciarTransicion;
	float					segsAnimando;
	float					segsAnimar;
	//
	AUEscenaContenedor*		layer;
	AUEscenaSprite*			bg;
	AUEscenaSprite*			logo;
} STScenesTransition;

class AUScenesTransition: public AUAppTransicion, public IEscuchadorCambioPuertoVision {
public:
	AUScenesTransition(const SI32 iScene, AUAppEscenasAdmin* escuchador, const bool animarEntrada, const float secsEachAnim);
	virtual ~AUScenesTransition();
	//
	virtual void		puertoDeVisionModificado(const SI32 iEscena, const STNBViewPortSize before, const STNBViewPortSize after);
	//
	virtual bool		ejecutandoTransicion();
	virtual bool		tickTransicion(float segsTranscurridos);
	//
	AUOBJMETODOS_CLASESID_DEFINICION
	AUOBJMETODOS_CLASESNOMBRES_DEFINICION
	AUOBJMETODOS_CLONAR_DEFINICION
protected:
	SI32				_iScene;
	AUAppEscenasAdmin*	_listener;
	STScenesTransition	_data;
	float				_secsEachAnim;
	//
	void				privOrganizarCortinas();
	void				privOrganizarSegunAvanceSalida(const float avance, const BOOL isOut);
};

#endif /* AUScenesTransition_hpp */
