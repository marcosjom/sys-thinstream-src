//
//  main.m
//  thinstream-osx
//
//  Created by Marcos Ortega on 8/5/18.
//

#include "visual/TSVisualPch.h"
#import <Cocoa/Cocoa.h>
#include "nb/core/NBMngrStructMaps.h"
#include "nb/net/NBSocket.h"
#include "AUApp.h"

//void testOCRPath(const char* filepath);

int main(int argc, const char * argv[]) {
	int r = 0;
	NBMngrProcess_init();
	NBMngrStructMaps_init();
	NBSocket_initEngine();
	NBSocket_initWSA();
	if(!AUApp::inicializarNucleo(AUAPP_BIT_MODULO_RED)){
		PRINTF_CONSOLE_ERROR("Could not init AUApp::core.\n");
		r = -1;
	} else {
		//Test imperfect search
		/*{
			const char* haystack = "Marcos JOSUE oRTEGA morales";
			const char* needle = "josuéa";
			if(!NBString_strContainsImperfect(haystack, needle, 1, ENStringFind_First)){
				PRINTF_INFO("Part('%s') NOT-found in: '%s'.\n", needle, haystack);
			} else {
				PRINTF_INFO("Part('%s') FOUND in: '%s'.\n", needle, haystack);
			}
		}*/
		//Test OCR on ID-photos
		/*{
			STNBBitmap bmp;
			NBBitmap_init(&bmp);
			NBPng_loadFromPath("/Users/mortegam/Desktop/_ocr_tests/IMG_6626.png", TRUE, &bmp, NULL);
			{
				STTSPhotoIdAnalyzer analyzer;
				SSPhotoIdAnalyzer_init(&analyzer);
				SSPhotoIdAnalyzer_setTmpFolderPath(&analyzer, NBGestorArchivos::rutaHaciaRecursoEnCache(""));
				//Config steps
				{
					SSPhotoIdAnalyzer_addStep(&analyzer, ENSSPhotoIdAnalyzerStep_OSD);
					SSPhotoIdAnalyzer_addStep(&analyzer, ENSSPhotoIdAnalyzerStep_NameParts);
				}
				//Config name-parts
				{
					SSPhotoIdAnalyzer_addNamePartBytes(&analyzer, "Marcos", NBString_strLenBytes("Marcos"));
					SSPhotoIdAnalyzer_addNamePartBytes(&analyzer, "JOSUE", NBString_strLenBytes("JOSUE"));
					SSPhotoIdAnalyzer_addNamePartBytes(&analyzer, "Ortega", NBString_strLenBytes("Ortega"));
					SSPhotoIdAnalyzer_addNamePartBytes(&analyzer, "mORALES", NBString_strLenBytes("mORALES"));
				}
				//Config bmp-versions
				{
					SSPhotoIdAnalyzer_addAttemptVersion(&analyzer, 85); //maxWhites
					SSPhotoIdAnalyzer_addAttemptVersion(&analyzer, 70); //maxWhites
					SSPhotoIdAnalyzer_addAttemptVersion(&analyzer, 100); //maxWhites
				}
				{
					STNBString dataPath;
					NBString_init(&dataPath);
					NBString_initWithStr(&dataPath, NBGestorArchivos::rutaHaciaRecursoEnPaquete(""));
					{
						if(!SSPhotoIdAnalyzer_feedBitmap(&analyzer, &bmp, NBST_P(STNBSizeI, 300, 300 ), "eng", dataPath.str)){
							PRINTF_CONSOLE_ERROR("Could not start PhotoIdAnalyzer.\n")
							NBASSERT(FALSE)
						} else {
							PRINTF_INFO("PhotoIdAnalyzer started.\n")
							SSPhotoIdAnalyzer_waitForAll(&analyzer);
							PRINTF_INFO("PhotoIdAnalyzer ended.\n")
							SSPhotoIdAnalyzer_writeBmpVersTo(&analyzer, "/Users/mortegam/Desktop/_ocr_tests/");
							PRINTF_INFO("PhotoIdAnalyzer saved bmpVers to: '%s'.\n", "/Users/mortegam/Desktop/_ocr_tests/");
						}
					}
					NBString_release(&dataPath);
				}
				SSPhotoIdAnalyzer_release(&analyzer);
			}
			NBBitmap_release(&bmp);
		}*/
		srand((unsigned)time(NULL));
		r = NSApplicationMain(argc, argv);
		AUApp::finalizarNucleo();
	}
	NBSocket_finishWSA();
	NBSocket_releaseEngine();
	NBMngrStructMaps_release();
	NBMngrProcess_release();
	return r;
}


//void testOCRBitmap(const STNBBitmap* bmp, const char* filepath);

/*void testOCRPath(const char* filepath){
	STNBBitmap bmp;
	NBBitmap_init(&bmp);
	if(!NBPng_loadFromPath(filepath, TRUE, &bmp, NULL)){
		PRINTF_CONSOLE_ERROR("Could not load bmp: '%s'.\n", filepath);
		NBASSERT(FALSE)
	} else {
		//Process
		testOCRBitmap(&bmp, filepath);
		//Convert to GRAY, save and process
		{
			const STNBBitmapProps props = NBBitmap_getProps(&bmp);
			if(props.color != ENNBBitmapColor_GRIS8){
				STNBBitmap bmpGray;
				NBBitmap_init(&bmpGray);
				if(!NBBitmap_createWithBitmap(&bmpGray, ENNBBitmapColor_GRIS8, &bmp, NBST_P(STNBColor8, 255, 255, 255, 255 ))){
					PRINTF_CONSOLE_ERROR("Could not create gray-bmp: '%s'.\n", filepath);
					NBASSERT(FALSE)
				} else {
					STNBString pathGray;
					NBString_initWithStr(&pathGray, filepath);
					NBString_concat(&pathGray, ".gray.png");
					if(!NBPng_saveToPath(&bmpGray, pathGray.str, ENPngCompressLvl_4)){
						PRINTF_CONSOLE_ERROR("Could not save gray-bmp: '%s'.\n", pathGray.str);
						NBASSERT(FALSE)
					}
					//Create saturated-100 (anything over 100 will be white)
					{
						STNBBitmap bmpSat;
						NBBitmap_init(&bmpSat);
						if(!NBBitmap_createWithBitmap(&bmpSat, ENNBBitmapColor_GRIS8, &bmpGray, NBST_P(STNBColor8, 255, 255, 255, 255 ))){
							PRINTF_CONSOLE_ERROR("Could not create gray-bmp: '%s'.\n", filepath);
							NBASSERT(FALSE)
						} else {
							STNBString pathSat;
							NBString_initWithStr(&pathSat, filepath);
							NBString_concat(&pathSat, ".gray.sat100.png");
							//Pre-process image
							{
								const STNBBitmapProps props = NBBitmap_getProps(&bmpSat);
								BYTE* data = NBBitmap_getData(&bmpSat);
								SI32 y, x;
								for(y = 0; y < props.size.height; y++){
									BYTE* pix = &data[y * props.bytesPerLine];
									for(x = 0; x < props.size.width; x++){
										if(*pix > 100) *pix = 255;
										pix++;
									}
								}
							}
							//Save
							if(!NBPng_saveToPath(&bmpSat, pathSat.str, ENPngCompressLvl_4)){
								PRINTF_CONSOLE_ERROR("Could not save gray-sat100-bmp: '%s'.\n", pathSat.str);
								NBASSERT(FALSE)
							}
							//Process-saturated
							{
								testOCRBitmap(&bmpSat, pathSat.str);
							}
							NBString_release(&pathSat);
						}
						NBBitmap_release(&bmpSat);
					}
					//Create saturated-80 (anything over 80 will be white)
					{
						STNBBitmap bmpSat;
						NBBitmap_init(&bmpSat);
						if(!NBBitmap_createWithBitmap(&bmpSat, ENNBBitmapColor_GRIS8, &bmpGray, NBST_P(STNBColor8, 255, 255, 255, 255 ))){
							PRINTF_CONSOLE_ERROR("Could not create gray-bmp: '%s'.\n", filepath);
							NBASSERT(FALSE)
						} else {
							STNBString pathSat;
							NBString_initWithStr(&pathSat, filepath);
							NBString_concat(&pathSat, ".gray.sat080.png");
							//Pre-process image
							{
								const STNBBitmapProps props = NBBitmap_getProps(&bmpSat);
								BYTE* data = NBBitmap_getData(&bmpSat);
								SI32 y, x;
								for(y = 0; y < props.size.height; y++){
									BYTE* pix = &data[y * props.bytesPerLine];
									for(x = 0; x < props.size.width; x++){
										if(*pix > 80) *pix = 255;
										pix++;
									}
								}
							}
							//Save
							if(!NBPng_saveToPath(&bmpSat, pathSat.str, ENPngCompressLvl_4)){
								PRINTF_CONSOLE_ERROR("Could not save gray-sat080-bmp: '%s'.\n", pathSat.str);
								NBASSERT(FALSE)
							}
							//Process-saturated
							{
								testOCRBitmap(&bmpSat, pathSat.str);
							}
							NBString_release(&pathSat);
						}
						NBBitmap_release(&bmpSat);
					}
					//Create saturated-70 (anything over 70 will be white)
					{
						STNBBitmap bmpSat;
						NBBitmap_init(&bmpSat);
						if(!NBBitmap_createWithBitmap(&bmpSat, ENNBBitmapColor_GRIS8, &bmpGray, NBST_P(STNBColor8, 255, 255, 255, 255 ))){
							PRINTF_CONSOLE_ERROR("Could not create gray-bmp: '%s'.\n", filepath);
							NBASSERT(FALSE)
						} else {
							STNBString pathSat;
							NBString_initWithStr(&pathSat, filepath);
							NBString_concat(&pathSat, ".gray.sat070.png");
							//Pre-process image
							{
								const STNBBitmapProps props = NBBitmap_getProps(&bmpSat);
								BYTE* data = NBBitmap_getData(&bmpSat);
								SI32 y, x;
								for(y = 0; y < props.size.height; y++){
									BYTE* pix = &data[y * props.bytesPerLine];
									for(x = 0; x < props.size.width; x++){
										if(*pix > 70) *pix = 255;
										pix++;
									}
								}
							}
							//Save
							if(!NBPng_saveToPath(&bmpSat, pathSat.str, ENPngCompressLvl_4)){
								PRINTF_CONSOLE_ERROR("Could not save gray-sat070-bmp: '%s'.\n", pathSat.str);
								NBASSERT(FALSE)
							}
							//Process-saturated
							{
								testOCRBitmap(&bmpSat, pathSat.str);
							}
							NBString_release(&pathSat);
						}
						NBBitmap_release(&bmpSat);
					}
					NBString_release(&pathGray);
				}
				NBBitmap_release(&bmpGray);
			}
		}
	}
	NBBitmap_release(&bmp);
}*/

/*void testOCRBitmap(const STNBBitmap* bmp, const char* filepath){
	NBTHREAD_CLOCK start = NBThread_clock();
	STNBOcr ocr;
	NBOcr_init(&ocr);
	if(!NBOcr_feedBitmap(&ocr, bmp, "eng", "/Users/mortegam/Downloads/tessdata")){
		PRINTF_CONSOLE_ERROR("Could not feed bmp to ocr: '%s'.\n", filepath);
		NBASSERT(FALSE)
	} else {
		STNBString text;
		NBString_init(&text);
		NBOcr_getTextFound(&ocr, &text);
		PRINTF_INFO("Text found on '%s':\n%s\n---------\n(%.2f secs)\n", filepath, (text.length > 0 ? text.str : "(empty)"), (float)(NBThread_clock() - start) / (float)NBThread_clocksPerSec());
		NBString_release(&text);
		
	}
	NBOcr_release(&ocr);
}*/
