//
//  AUScenesTransition.cpp
//  thinstream-test
//
//  Created by Marcos Ortega on 8/5/18.
//

#include "visual/TSVisualPch.h"
#include "visual/AUScenesTransition.h"
#include "visual/TSLogoMetrics.h"

AUScenesTransition::AUScenesTransition(const SI32 iScene, AUAppEscenasAdmin* escuchador, const bool animarEntrada, const float secsEachAnim) : AUAppTransicion(){
	NB_DEFINE_NOMBRE_PUNTERO(this, "AUScenesTransition")
	_iScene						= iScene;
	_listener					= escuchador;
	_secsEachAnim				= secsEachAnim;
	_data.etapaCarga			= (ENSceneTransitionStage)((SI32)ENSceneTransitionStage_Ninguna + 1);
	_data.animarEntrada			= animarEntrada;
	_data.permitidoGirarPantallaAntesDeIniciarTransicion = false;
	_data.segsAnimando			= 0.0f;
	_data.segsAnimar			= 0.0f;
	//
	_data.layer					= new(this) AUEscenaContenedor();
	//Bg
	{
		_data.bg = new(this) AUEscenaSprite();
		_data.layer->agregarObjetoEscena(_data.bg);
	}
	//Logo
	{
		const NBCajaAABB cajaCortina = NBGestorEscena::cajaProyeccionGrupo(_iScene, ENGestorEscenaGrupo_Cortina);
		const STTSLaunchLogo logoVer = DRLogoMetrics::getLaunchscreenLogoSzVersion(cajaCortina, ENLogoType_Isotype);
		STNBString strTmp;
		NBString_init(&strTmp);
		NBString_empty(&strTmp);
		NBString_concat(&strTmp, "thinstream/iso/thinstream.iso.gray@");
		NBString_concatFloat(&strTmp, logoVer.szVersion.pathScale, (logoVer.szVersion.pathScale == (float)((SI32)logoVer.szVersion.pathScale) ? 0 : 1));
		NBString_concat(&strTmp, "x.png");
		{
			_data.logo = new(this) AUEscenaSprite(NBGestorTexturas::texturaDesdeArchivo(strTmp.str));
			_data.layer->agregarObjetoEscena(_data.logo);
		}
		NBString_release(&strTmp);
	}
	//
	this->privOrganizarCortinas();
	//
	NBGestorEscena::agregarEscuchadorCambioPuertoVision(_iScene, this);
}

AUScenesTransition::~AUScenesTransition(){
	NBGestorEscena::quitarEscuchadorCambioPuertoVision(_iScene, this);
	if(_data.layer != NULL){
		NBGestorEscena::quitarObjetoCapa(_iScene, _data.layer);
		_data.layer->liberar(NB_RETENEDOR_THIS);
		_data.layer = NULL;
	}
	if(_data.bg != NULL) _data.bg->liberar(NB_RETENEDOR_THIS); _data.bg = NULL;
	if(_data.logo != NULL) _data.logo->liberar(NB_RETENEDOR_THIS); _data.logo = NULL;
}

void AUScenesTransition::puertoDeVisionModificado(const SI32 iEscena, const STNBViewPortSize before, const STNBViewPortSize after){
	this->privOrganizarCortinas();
}

void AUScenesTransition::privOrganizarCortinas(){
	const NBCajaAABB scnBox = NBGestorEscena::cajaProyeccionGrupo(_iScene, ENGestorEscenaGrupo_Cortina);
	//Bg
	{
		_data.bg->redimensionar(scnBox.xMin, scnBox.yMin, scnBox.xMax - scnBox.xMin, scnBox.yMax - scnBox.yMin);
	}
	//Logo
	{
		const STTSLaunchLogo logoVer = DRLogoMetrics::getLaunchscreenLogoSzVersion(scnBox, ENLogoType_Isotype);
		const NBCajaAABB box		= _data.logo->cajaAABBLocal();
		const STNBSize scale		= { (logoVer.sceneSize.width / (box.xMax - box.xMin)), (logoVer.sceneSize.height / (box.yMax - box.yMin)) };
		_data.logo->establecerEscalacion(scale.width, scale.height);
		_data.logo->establecerTraslacion(logoVer.scenePos.x - (box.xMin * scale.width), logoVer.scenePos.y - (box.yMin * scale.height));
	}
}

//

bool AUScenesTransition::ejecutandoTransicion(){
	return (_data.etapaCarga != ENSceneTransitionStage_Ninguna);
}

bool AUScenesTransition::tickTransicion(float segsTranscurridos){
	NBColor colorSolidoDestino; NBCOLOR_ESTABLECER(colorSolidoDestino, 0.0f, 0.0f, 0.0f, 1.0f)
	if(_data.etapaCarga == ENSceneTransitionStage_Ninguna){
		//nada
	} else if(_data.etapaCarga == ENSceneTransitionStage_AparecerIni){
		//Color de fondo
		_data.segsAnimando	= 0.0f;
		_data.segsAnimar	= _secsEachAnim;
		NBColor8 colorLuz; NBCOLOR_ESTABLECER(colorLuz, 255, 255, 255, 255)
		NBGestorEscena::agregarObjetoCapa(_iScene, ENGestorEscenaGrupo_Cortina, _data.layer, colorLuz);
		if(_data.animarEntrada){
			this->privOrganizarSegunAvanceSalida(1.0f, FALSE);
			_data.etapaCarga = (ENSceneTransitionStage)(_data.etapaCarga + 1);
		} else {
			this->privOrganizarSegunAvanceSalida(0.0f, TRUE);
			_data.etapaCarga = (ENSceneTransitionStage)(_data.etapaCarga + 2);
			this->tickTransicion(segsTranscurridos);
		}
	} else if(_data.etapaCarga == ENSceneTransitionStage_Aparecer){
		_data.segsAnimando += segsTranscurridos;
		if(_data.segsAnimando < _data.segsAnimar){
			this->privOrganizarSegunAvanceSalida(1.0f - (_data.segsAnimando / _data.segsAnimar), FALSE);
		} else {
			this->privOrganizarSegunAvanceSalida(0.0f, FALSE);
			_data.etapaCarga = (ENSceneTransitionStage)(_data.etapaCarga + 1);
		}
	} else if(_data.etapaCarga == ENSceneTransitionStage_LiberarViejo){
		NBGestorAnimadores::establecerAnimadorActivo(false);
		//Liberar escena actual
		_listener->escenasLiberar();
		_data.etapaCarga = (ENSceneTransitionStage)(_data.etapaCarga + 1);
		this->tickTransicion(segsTranscurridos);
	} else if(_data.etapaCarga == ENSceneTransitionStage_CrearNuevoLiberarRecursosSinUso){
		//Cargar escena y precargar sus recursos
		NBGestorTexturas::modoCargaTexturasPush(ENGestorTexturaModo_cargaInmediata);
		_listener->escenasCargar();
		NBGestorTexturas::modoCargaTexturasPop();
		//Color de fondo
		_listener->escenasColocarEnPrimerPlano();
		_data.segsAnimando	= 0.0f;
		_data.segsAnimar	= _secsEachAnim;
		//
		NBGestorAnimadores::establecerAnimadorActivo(true);
		_listener->escenasLiberarRecursosSinUso();
		//Cargar recursos pendientes
#		ifndef CONFIG_NB_UNSUPPORT_AUDIO_IO
		while(NBGestorSonidos::conteoBufferesPendientesDeCargar()!=0){ NBGestorSonidos::cargarBufferesPendientesDeCargar(9999); }
#		endif
		while(NBGestorTexturas::texPendienteOrganizarConteo()!=0){ NBGestorTexturas::texPendienteOrganizarProcesar(9999); }
		NBGestorTexturas::generarMipMapsDeTexturas();
		_data.etapaCarga = (ENSceneTransitionStage)(_data.etapaCarga + 1);
	} else if(_data.etapaCarga == ENSceneTransitionStage_Desaparecer){
		_data.segsAnimando += segsTranscurridos;
		if(_data.segsAnimando < _data.segsAnimar){
			this->privOrganizarSegunAvanceSalida(_data.segsAnimando / _data.segsAnimar, TRUE);
		} else {
			this->privOrganizarSegunAvanceSalida(1.0f, TRUE);
			_data.etapaCarga = (ENSceneTransitionStage)(_data.etapaCarga + 1);
		}
	} else if(_data.etapaCarga == ENSceneTransitionStage_DesaparecerFin){
		NBGestorEscena::quitarObjetoCapa(_iScene, _data.layer);
		_data.etapaCarga = ENSceneTransitionStage_Ninguna;
#	ifdef CONFIG_NB_GESTOR_MEMORIA_IMPLEMENTAR_GRUPOS_ZONAS_MEMORIA
		NBGestorMemoria::liberarZonasSinUso();
#	endif
	}
	return false;
}

void AUScenesTransition::privOrganizarSegunAvanceSalida(const float avance, const BOOL isOut){
	NBASSERT(avance >= 0.0f && avance <= 1.0f)
	if(FALSE){
		//------------------------
		//-- One mixed animation
		//------------------------
		const float relAlpha = 0.50f;
		if(avance < relAlpha){
			const float rel = (avance / relAlpha); NBASSERT(rel >= 0.0f && rel <= 1.0f)
			_data.bg->establecerMultiplicadorAlpha8(255.0f * (1.0f - (rel /* * rel * rel */)));
			_data.bg->setVisibleAndAwake(TRUE);
			_data.logo->establecerMultiplicadorAlpha8(255);
			_data.logo->setVisibleAndAwake(TRUE);
		} else {
			const float rel = ((avance - relAlpha) / (1.0f - relAlpha)); NBASSERT(rel >= 0.0f && rel <= 1.0f)
			_data.bg->establecerMultiplicadorAlpha8(0);
			_data.bg->setVisibleAndAwake(FALSE);
			_data.logo->establecerMultiplicadorAlpha8(255.0f * (1.0f - (rel /* * rel * rel*/)));
			_data.logo->setVisibleAndAwake(TRUE);
		}
	} else {
		const float rel = avance; NBASSERT(rel >= 0.0f && rel <= 1.0f)
		_data.bg->establecerMultiplicadorAlpha8(255.0f * (1.0f - (rel /* * rel * rel */)));
		_data.bg->setVisibleAndAwake(TRUE);
		_data.logo->establecerMultiplicadorAlpha8(255.0f * (1.0f - (rel /* * rel * rel */)));
		_data.logo->setVisibleAndAwake(TRUE);
	}
}

AUOBJMETODOS_CLASESID_UNICLASE(AUScenesTransition)
AUOBJMETODOS_CLASESNOMBRES_UNICLASE(AUScenesTransition, "AUScenesTransition")
AUOBJMETODOS_CLONAR_NULL(AUScenesTransition)
