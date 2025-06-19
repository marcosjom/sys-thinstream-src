//
//  AUSceneLogoMetrics.cpp
//  thinstream-test
//
//  Created by Marcos Ortega on 8/5/18.
//

#include "visual/TSVisualPch.h"
#include "visual/TSLogoMetrics.h"

//

static STTSLaunchLogoSzVersion _launchLogosVersions[] = {
	{ "thinstream/logo/thinstream.logo@0.5x.png", 0.5f }
	, { "thinstream/logo/thinstream.logo@1x.png", 1.0f }
	, { "thinstream/logo/thinstream.logo@2x.png", 2.0f }
	, { "thinstream/logo/thinstream.logo@3x.png", 3.0f }
	, { "thinstream/logo/thinstream.logo@4x.png", 4.0f }
	, { "thinstream/logo/thinstream.logo@5x.png", 5.0f }
	, { "thinstream/logo/thinstream.logo@6x.png", 6.0f }
	, { "thinstream/logo/thinstream.logo@7x.png", 7.0f }
	, { "thinstream/logo/thinstream.logo@8x.png", 8.0f }
	, { "thinstream/logo/thinstream.logo@9x.png", 9.0f }
	, { "thinstream/logo/thinstream.logo@10x.png", 10.0f }
};

static STTSLaunchLogoSzVersion _launchIsosVersions[] = {
	{ "thinstream/iso/thinstream.iso@0.5x.png", 0.5f }
	, { "thinstream/iso/thinstream.iso@1x.png", 1.0f }
	, { "thinstream/iso/thinstream.iso@2x.png", 2.0f }
	, { "thinstream/iso/thinstream.iso@3x.png", 3.0f }
	, { "thinstream/iso/thinstream.iso@4x.png", 4.0f }
	, { "thinstream/iso/thinstream.iso@5x.png", 5.0f }
	, { "thinstream/iso/thinstream.iso@6x.png", 6.0f }
	, { "thinstream/iso/thinstream.iso@7x.png", 7.0f }
	, { "thinstream/iso/thinstream.iso@8x.png", 8.0f }
	, { "thinstream/iso/thinstream.iso@9x.png", 9.0f }
	, { "thinstream/iso/thinstream.iso@10x.png", 10.0f }
};

//ENLogoType			type;
//STNBLaunchCfgOrient	orients[ENTSScreenOrient_Count];

static const STNBLaunchCfgType _launchCfgTypes[] = {
	{
		ENLogoType_Logotype
		, {
			{
				ENTSScreenOrient_Portrait
				, { 0.50f, 0.50f }
				, { 0.90f , 1.00f }
				, _launchLogosVersions
				, sizeof(_launchLogosVersions) / sizeof(_launchLogosVersions[0])
			}
			, {
				ENTSScreenOrient_Landscape
				, { 0.50f, 0.50f }
				, { 0.75f, 1.00f }
				, _launchLogosVersions
				, sizeof(_launchLogosVersions) / sizeof(_launchLogosVersions[0])
			}
		}
	}
	, {
		ENLogoType_Isotype
		, {
			{
				ENTSScreenOrient_Portrait
				, { 0.50f, 0.50f }
				, { 0.33f , 1.00f }
				, _launchIsosVersions
				, sizeof(_launchIsosVersions) / sizeof(_launchIsosVersions[0])
			}
			, {
				ENTSScreenOrient_Landscape
				, { 0.50f, 0.50f }
				, { 1.00f, 0.33f }
				, _launchIsosVersions
				, sizeof(_launchIsosVersions) / sizeof(_launchIsosVersions[0])
			}
		}
	}
};

//


//Logo descriptions

STTSLogoDesc2D DRLogoMetrics::getLogoDesc2D(const ENLogoType type){
	STTSLogoDesc2D r;
	NBMemory_setZeroSt(r, STTSLogoDesc2D);
	switch (type) {
		case ENLogoType_Logotype:
			r = NBST_P(STTSLogoDesc2D
				, type
				, NBST_P(STNBSize,  3600, 448 ) //total-size
			);
			break;
		case ENLogoType_Isotype:
			r = NBST_P(STTSLogoDesc2D
				, type
				, NBST_P(STNBSize,  480, 480 ) //total-size
			);
			break;
		default:
			NBASSERT(FALSE)
			break;
	}
	return r;
}

STTSLaunchLogo DRLogoMetrics::getLaunchscreenLogoSzVersion(const NBCajaAABB screenBox, const ENLogoType type){
	STTSLaunchLogo r;
	NBMemory_setZeroSt(r, STTSLaunchLogo);
	//Determine orientation
	const float screenW	= (screenBox.xMax - screenBox.xMin);
	const float screenH	= (screenBox.yMax - screenBox.yMin);
	const ENTSScreenOrient orient = (screenW <= screenH ? ENTSScreenOrient_Portrait : ENTSScreenOrient_Landscape);
	const STNBLaunchCfgType* lauchTypeCfg	= &_launchCfgTypes[type]; NBASSERT(lauchTypeCfg->type == type)
	const STNBLaunchCfgOrient* launchCfg	= &lauchTypeCfg->orients[orient]; NBASSERT(launchCfg->orient == orient)
	//
	{
		STNBFilesystem* fs = NBGestorArchivos::getFilesystem();
		AUCadenaMutable8* pngPath = new AUCadenaMutable8();
		SI32 s; for(s = 0; s < launchCfg->imgSzVersLen; s++){
			const STTSLaunchLogoSzVersion* szVersion = &launchCfg->imgSzVers[s];
			//Get file path
			UI8 escalaBase2		= 1;
			float escalaParaHD	= 1.0f;
			NBGestorTexturas::rutaArchivoTextura(pngPath, &escalaBase2, &escalaParaHD, szVersion->path);
			//Peek bitmap
			{
                STNBFileRef pngFile = NBFile_alloc(NULL);
				if(!NBFilesystem_open(fs, pngPath->str(), ENNBFileMode_Read, pngFile)){
					NBASSERT(FALSE)
				} else {
					STNBBitmap bmp;
					NBBitmap_init(&bmp);
					NBFile_lock(pngFile);
					if(!NBPng_loadFromFile(pngFile, FALSE, &bmp, NULL)){
						NBASSERT(FALSE)
					} else {
						const STNBBitmapProps props = NBBitmap_getProps(&bmp);
						if(r.szVersion.path == NULL || ((props.size.width * escalaParaHD) <= (screenW * launchCfg->relMaxSz.width) && (props.size.height * escalaParaHD) <= (screenH * launchCfg->relMaxSz.height))){
							r.orient			= orient;
							r.szVersion			= *szVersion;
							r.sceneSize.width	= (props.size.width * escalaParaHD);
							r.sceneSize.height	= (props.size.height * escalaParaHD);
							r.scenePos.x		= screenBox.xMin + ((screenW - r.sceneSize.width) * launchCfg->relPos.x);
							r.scenePos.y		= screenBox.yMin + ((screenH - r.sceneSize.height) * (1.0f - launchCfg->relPos.y));
						}
					}
					NBFile_unlock(pngFile);
					NBBitmap_release(&bmp);
				}
				NBFile_release(&pngFile);
			}
		}
		pngPath->liberar(NB_RETENEDOR_NULL);
		pngPath = NULL;
	}
	return r;
}

STTSLaunchLogo DRLogoMetrics::getLogoSzVersionForSize(const STNBSize sizeMax, const ENLogoType type){
	STTSLaunchLogo r;
	NBMemory_setZeroSt(r, STTSLaunchLogo);
	//Determine orientation
	const ENTSScreenOrient orient = ENTSScreenOrient_Portrait;
	const STNBLaunchCfgType* lauchTypeCfg	= &_launchCfgTypes[type]; NBASSERT(lauchTypeCfg->type == type)
	const STNBLaunchCfgOrient* launchCfg	= &lauchTypeCfg->orients[orient]; NBASSERT(launchCfg->orient == orient)
	//
	{
		STNBFilesystem* fs = NBGestorArchivos::getFilesystem();
		AUCadenaMutable8* pngPath = new AUCadenaMutable8();
		SI32 s; for(s = 0; s < launchCfg->imgSzVersLen; s++){
			const STTSLaunchLogoSzVersion* szVersion = &launchCfg->imgSzVers[s];
			//Get file path
			UI8 escalaBase2		= 1;
			float escalaParaHD	= 1.0f;
			NBGestorTexturas::rutaArchivoTextura(pngPath, &escalaBase2, &escalaParaHD, szVersion->path);
			//Peek bitmap
			{
                STNBFileRef pngFile = NBFile_alloc(NULL);
				if(!NBFilesystem_open(fs, pngPath->str(), ENNBFileMode_Read, pngFile)){
					NBASSERT(FALSE)
				} else {
					STNBBitmap bmp;
					NBBitmap_init(&bmp);
					NBFile_lock(pngFile);
					if(!NBPng_loadFromFile(pngFile, FALSE, &bmp, NULL)){
						NBASSERT(FALSE)
					} else {
						const STNBBitmapProps props = NBBitmap_getProps(&bmp);
						if(r.szVersion.path == NULL || ((props.size.width * escalaParaHD) <= sizeMax.width && (props.size.height * escalaParaHD) <= sizeMax.height)){
							r.orient			= orient;
							r.szVersion			= *szVersion;
							r.sceneSize.width	= (props.size.width * escalaParaHD);
							r.sceneSize.height	= (props.size.height * escalaParaHD);
							r.scenePos.x		= 0;
							r.scenePos.y		= 0;
						}
					}
					NBFile_unlock(pngFile);
					NBBitmap_release(&bmp);
				}
				NBFile_release(&pngFile);
			}
		}
		pngPath->liberar(NB_RETENEDOR_NULL);
		pngPath = NULL;
	}
	return r;
}
