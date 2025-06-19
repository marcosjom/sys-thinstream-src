//
//  DRRegister.cpp
//  thinstream
//
//  Created by Marcos Ortega on 8/5/18.
//



#include "visual/TSVisualPch.h"
#include "visual/TSVisualRegister.h"
#include "visual/TSVisual.h"

//
UI16 AUSceneIntro::idTipoClase = 0;
UI16 AUScenesAdmin::idTipoClase = 0;
UI16 AUScenesTransition::idTipoClase = 0;
UI16 AUSceneBarTop::idTipoClase = 0;
UI16 AUSceneBarBtm::idTipoClase = 0;
//
UI16 AUSceneLobby::idTipoClase = 0;
UI16 AUSceneCol::idTipoClase = 0;
UI16 AUSceneColsStack::idTipoClase = 0;
UI16 AUSceneColsPanel::idTipoClase = 0;
UI16 TSVisualTag::idTipoClase = 0;
UI16 TSVisualRowsGrp::idTipoClase = 0;
UI16 TSVisualStatus::idTipoClase = 0;
UI16 TSVisualIcons::idTipoClase = 0;
UI16 DRColumnOrchestor::idTipoClase = 0;
//
UI16 TSVisualHome::idTipoClase = 0;
UI16 TSVisualHomeSourceRow::idTipoClase = 0;
//
UI16 TSVisualViewer::idTipoClase = 0;
UI16 TSVisualViewerStream::idTipoClase = 0;

#ifndef NB_METODO_INICIALIZADOR_DEF
#	error "Missing include. Macro NB_METODO_INICIALIZADOR_DEF not defined."
#endif

NB_METODO_INICIALIZADOR_CUERPO(TSVisual_Register){
	printf("\n\n+++++++++++++ TSVisual_Register +++++++++++++++\n\n");
	//
	NBGestorAUObjetos::registrarClase("AUSceneIntro", &AUSceneIntro::idTipoClase);
	NBGestorAUObjetos::registrarClase("AUScenesAdmin", &AUScenesAdmin::idTipoClase);
	NBGestorAUObjetos::registrarClase("AUScenesTransition", &AUScenesTransition::idTipoClase);
	NBGestorAUObjetos::registrarClase("AUSceneBarTop", &AUSceneBarTop::idTipoClase);
	NBGestorAUObjetos::registrarClase("AUSceneBarBtm", &AUSceneBarBtm::idTipoClase);
	//
	NBGestorAUObjetos::registrarClase("AUSceneLobby", &AUSceneLobby::idTipoClase);
	NBGestorAUObjetos::registrarClase("AUSceneCol", &AUSceneCol::idTipoClase);
	NBGestorAUObjetos::registrarClase("AUSceneColsStack", &AUSceneColsStack::idTipoClase);
	NBGestorAUObjetos::registrarClase("AUSceneColsPanel", &AUSceneColsPanel::idTipoClase);
	//
	NBGestorAUObjetos::registrarClase("TSVisualTag", &TSVisualTag::idTipoClase);
	NBGestorAUObjetos::registrarClase("TSVisualRowsGrp", &TSVisualRowsGrp::idTipoClase);
	NBGestorAUObjetos::registrarClase("TSVisualStatus", &TSVisualStatus::idTipoClase);
	NBGestorAUObjetos::registrarClase("TSVisualIcons", &TSVisualIcons::idTipoClase);
	NBGestorAUObjetos::registrarClase("DRColumnOrchestor", &DRColumnOrchestor::idTipoClase);
	//
	NBGestorAUObjetos::registrarClase("TSVisualHome", &TSVisualHome::idTipoClase);
	NBGestorAUObjetos::registrarClase("TSVisualHomeSourceRow", &TSVisualHomeSourceRow::idTipoClase);
	//
	NBGestorAUObjetos::registrarClase("TSVisualViewer", &TSVisualViewer::idTipoClase);
	NBGestorAUObjetos::registrarClase("TSVisualViewerStream", &TSVisualViewerStream::idTipoClase);
};

