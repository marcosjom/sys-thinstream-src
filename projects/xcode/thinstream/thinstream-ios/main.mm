//
//  main.m
//  thinstream-ios
//
//  Created by Marcos Ortega on 8/5/18.
//

//PCH
#include "AUFrameworkBaseStdAfx.h"
#include "AUFrameworkMediaStdAfx.h"
#include "AUAppNucleoPrecompilado.h"
#include "AUAppNucleo.h"
//
#import <UIKit/UIKit.h>
#import "AppDelegate.h"
//
#include "../../sys-thinstream-res/PaquetesC/thinstream_ca.cert.der.h"
#include "../../sys-thinstream-res/PaquetesC/thinstream.cfg.app.json.h"
#include "../../sys-thinstream-res/PaquetesC/thinstream.cfg.app.locale.json.h"
//
#include "visual/TSVisualPch.h"
#include "nb/core/NBMngrStructMaps.h"
#include "nb/net/NBSocket.h"

int main(int argc, char * argv[]) {
	int r = 0;
	NBMngrProcess_init();
	NBMngrStructMaps_init();
	NBSocket_initEngine();
	NBSocket_initWSA();
	if(!AUApp::inicializarNucleo(AUAPP_BIT_MODULO_RED)){
		PRINTF_CONSOLE_ERROR("Could not init AUApp::core.\n");
		r = -1;
	} else {
		srand((unsigned)time(NULL));
		//Ignore SIGPIPE at process level (for unix-like systems)
#		ifndef _WIN32
		{
			struct sigaction act;
			act.sa_handler = SIG_IGN;
			sigemptyset(&act.sa_mask);
			act.sa_flags = 0;
			sigaction(SIGPIPE, &act, NULL);
		}
#		endif
		@autoreleasepool {
			_cfg		= NULL;
			_context	= NULL;
			_client		= NULL;
			//Load config and start context/client
			{
				STNBString rootPath2;
				NBString_init(&rootPath2);
				NBString_set(&rootPath2, [[[NSBundle mainBundle] resourcePath] UTF8String]);
				if(rootPath2.length > 0){
					if(rootPath2.str[rootPath2.length - 1] != '/'){
						NBString_concatByte(&rootPath2, '/');
					}
				}
				//NSString* cfgPath2 = [[NSBundle mainBundle] pathForResource:@"thinstream.cfg.app" ofType:@"json"];
				//const char* cfgPath = [cfgPath2 UTF8String];
				//TSCfg_loadFromFilePath(cfg, cfgPath)
				STTSCfg* cfg = NBMemory_allocType(STTSCfg);
				TSCfg_init(cfg);
				if(!TSCfg_loadFromJsonStrBytes(cfg, (const char*)PaquetesC_thinstream_cfg_app_json_h, (sizeof(PaquetesC_thinstream_cfg_app_json_h) / sizeof(PaquetesC_thinstream_cfg_app_json_h[0])))){
					PRINTF_CONSOLE_ERROR("MAIN, could not load config file from memory.\n");
				} else {
					PRINTF_INFO("MAIN, config file loaded: from memory.\n");
					//NSString* localePath2 = [[NSBundle mainBundle] pathForResource:@"thinstream.cfg.app.locale" ofType:@"json"];
					//const char* localePath = [localePath2 UTF8String];
					const char* rootPath = rootPath2.str;
					STTSContext* context = NBMemory_allocType(STTSContext);
					TSContext_init(context);
					TSContext_setNotifMode(context, ENNBStorageNotifMode_Manual);
					if(!TSContext_prepare(context, cfg)){
						PRINTF_CONSOLE_ERROR("MAIN, TSContext_prepare failed.\n");
					} else if(!TSContext_loadLocaleStrBytes(context, (const char*)PaquetesC_thinstream_cfg_app_locale_json_h, (sizeof(PaquetesC_thinstream_cfg_app_locale_json_h) / sizeof(PaquetesC_thinstream_cfg_app_locale_json_h[0])))){
						PRINTF_CONSOLE_ERROR("MAIN, could not load context locale from memory.\n");
					} else if(!TSContext_createRootFolders(context)){
						PRINTF_CONSOLE_ERROR("MAIN, could not create root folder.\n"); 
					} else {
						PRINTF_INFO("MAIN, context loaded.\n");
						//Load prefered languages
						{
							NSArray<NSString *>* lst = [NSLocale preferredLanguages];
							if(lst != nil){
								const SI32 sz = (SI32)[lst count];
								SI32 i; for(i = (sz - 1); i >= 0; i--){
									NSString* lang = [lst objectAtIndex:(NSUInteger)i];
									if(lang != nil){
										TSContext_pushPreferedLang(context, [lang UTF8String]);
									}
								}
								//Tmp: forcing a language
								//TSContext_pushPreferedLang(context, "de");
								//TSContext_pushPreferedLang(context, "fr");
							}
						}
						//Start client
						if(!TSContext_start(context)){
							PRINTF_CONSOLE_ERROR("MAIN, TSContext_start failed.\n");
						} else {
							STTSClient* client = NBMemory_allocType(STTSClient);
							TSClient_init(client);
							if(!TSClient_prepare(client, rootPath, context, cfg, PaquetesC_thinstream_ca_cert_der_h, (sizeof(PaquetesC_thinstream_ca_cert_der_h) / sizeof(PaquetesC_thinstream_ca_cert_der_h[0])))){
								PRINTF_CONSOLE_ERROR("MAIN, could not start client with config from memory.\n");
							} else {
								_cfg		= cfg; cfg = NULL;
								_context	= context; context = NULL;
								_client		= client; client = NULL;
							}
							if(client != NULL){
								TSClient_release(client);
								NBMemory_free(client);
								client = NULL;
							}
						}
					}
					if(context != NULL){
						TSContext_release(context);
						NBMemory_free(context);
						context = NULL;
					}
				}
				if(cfg != NULL){
					TSCfg_release(cfg);
					NBMemory_free(cfg);
					cfg = NULL;
				}
				//NBString_release(&rootPath2);
			}
			//Run
			if(_cfg != NULL && _context != NULL && _client != NULL){
				//
				//[[NSUserDefaults standardUserDefaults] setObject:[NSArray arrayWithObjects:@"de", @"en", @"fr", @"es", nil] forKey:@"AppleLanguages"];
				//[[NSUserDefaults standardUserDefaults] synchronize];
				//
				r = UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
			}
			//Release
			{
				if(_client != NULL){
					TSClient_release(_client);
					NBMemory_free(_client);
					_client = NULL;
				}
				if(_context != NULL){
					TSContext_stop(_context);
					TSContext_release(_context);
					NBMemory_free(_context);
					_context = NULL;
				}
				if(_cfg != NULL){
					TSCfg_release(_cfg);
					NBMemory_free(_cfg);
					_cfg = NULL;
				}
			}
		}
		AUApp::finalizarNucleo();
	}
	NBSocket_finishWSA();
	NBSocket_releaseEngine();
	NBMngrStructMaps_release();
	NBMngrProcess_release();
	return r;
}
