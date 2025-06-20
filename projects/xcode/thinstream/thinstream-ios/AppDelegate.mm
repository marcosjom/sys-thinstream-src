//
//  AppDelegate.m
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
#include "../../sys-thinstream-res/PaquetesC/paqFuentes.otf.h"
#include "../../sys-thinstream-res/PaquetesC/paqPNGx1.png.h"
#include "../../sys-thinstream-res/PaquetesC/paqPNGx2.png.h"
#include "../../sys-thinstream-res/PaquetesC/paqPNGx4.png.h"
#include "../../sys-thinstream-res/PaquetesC/paqPNGx8.png.h"
//
#import "AppDelegate.h"
//
#include "core/logic/TSClientRoot.h"

STTSCfg*			_cfg		= NULL;
STTSContext*	_context	= NULL;
STTSClient*			_client		= NULL;

@interface AppDelegate ()

@end

@implementation AppDelegate


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
	PRINTF_INFO("AUAppDelegate::application didFinishLaunchingWithOptions.\n");
	//Printing current locale
	{
		NSLog(@"[[NSLocale currentLocale] languageCode]: %@.\n", [[NSLocale currentLocale] languageCode]);
		//NSLog(@"[NSLocale availableLocaleIdentifiers]: %@.\n", [NSLocale availableLocaleIdentifiers]);
	}
	//Redirect output to stream
	{
		const char* fullpath = NULL;
		//Log file path
		{
			NSArray* allPaths = NSSearchPathForDirectoriesInDomains(NSCachesDirectory, NSUserDomainMask, YES);
			if(allPaths != nil){
				if([allPaths count] > 0){
					NSString* documentsDirectory = [allPaths objectAtIndex:0];
					if(documentsDirectory != nil){
						NSString* pathForLog = [documentsDirectory stringByAppendingPathComponent:@"thinstream.app.log.txt"];
						if(pathForLog != nil){
							fullpath = [pathForLog cStringUsingEncoding:NSUTF8StringEncoding];
							PRINTF_INFO("MAIN, output log file: '%s'.\n", fullpath);
						}
					}
				}
			}
		}
		//Log
		if(fullpath != NULL){
			if(fullpath[0] != '\0'){
				//STDIN_FILENO, STDOUT_FILENO, STDERR_FILENO
				const BOOL isConsole = isatty(STDERR_FILENO);
				//Load content
				if(_client != NULL){
					ENDRErrorLogSendMode errLogMode = ENDRErrorLogSendMode_Ask;
					{
						STNBStorageRecordRead ref = TSClient_getStorageRecordForRead(_client, "client/_root/", "_current", NULL, TSClientRoot_getSharedStructMap());
						if(ref.data != NULL){
							STTSClientRoot* priv = (STTSClientRoot*)ref.data->priv.stData;
							errLogMode = priv->errLogMode;
						}
						TSClient_returnStorageRecordFromRead(_client, &ref);
					}
					if(errLogMode == ENDRErrorLogSendMode_Ask || errLogMode == ENDRErrorLogSendMode_Allways){
						TSClient_logAnalyzeFile(_client, fullpath, isConsole);
					}
				}
				//Delete previous log file
				{
					NBGestorArchivos::eliminarArchivo(fullpath);
				}
				//Redirect stream
				if(isConsole){
					PRINTF_INFO("MAIN, isatty(STDERR) returned TRUE, output to console.\n");
				} else {
					PRINTF_INFO("MAIN, isatty(STDERR) returned FALSE, output to file.\n");
					freopen(fullpath, "a+", stderr);
				}
			}
		}
	}
	//
	UIScreen* screen			= [UIScreen mainScreen];
	CGRect sBounds				= [screen bounds];
	_window						= [[UIWindow alloc] initWithFrame: sBounds];
	_monoViewController			= [[AUIOSMonoViewController alloc] initWithApplication:application appName:"thinstreamApp" window:_window frame:sBounds];
	_window.rootViewController	= _monoViewController;
	//Enable framework methods
	{
		AUApp* app = [_monoViewController getApp];
		app->setApplication(application);
		app->setWindow((__bridge void*)_window);
		app->setViewController((__bridge void*)_monoViewController);
		app->setLaunchOptions((__bridge void*)launchOptions);
		AUApp_linkToInternalLibJpegRead();
		AUApp_linkToDefaultOSTools(app);
		AUApp_linkToDefaultOSSecure(app);
		AUApp_linkToDefaultPkgFilesystem(app);
		AUApp_linkToDefaultKeyboard(app);
		AUApp_linkToDefaultNotifSystem(app);
		AUApp_linkToDefaultAVCapture(app);
		AUApp_linkToDefaultStore(app);
		AUApp_linkToDefaultTelephony(app);
		AUApp_linkToDefaultBiometrics(app);
		AUApp_linkToDefaultPdfKit(app);
	}
	//Add my scenes to the app
	{
		const STAppScenasPkgLoadDef packages[] = {
			{ "paqFuentes.otf", PaquetesC_paqFuentes_otf_h, (sizeof(PaquetesC_paqFuentes_otf_h) / sizeof(PaquetesC_paqFuentes_otf_h[0])) }
			, { "paqPNGx8.png", PaquetesC_paqPNGx1_png_h, (sizeof(PaquetesC_paqPNGx1_png_h) / sizeof(PaquetesC_paqPNGx1_png_h[0])) }
			, { "paqPNGx4.png", PaquetesC_paqPNGx2_png_h, (sizeof(PaquetesC_paqPNGx2_png_h) / sizeof(PaquetesC_paqPNGx2_png_h[0])) }
			, { "paqPNGx2.png", PaquetesC_paqPNGx4_png_h, (sizeof(PaquetesC_paqPNGx4_png_h) / sizeof(PaquetesC_paqPNGx4_png_h[0])) }
			, { "paqPNGx1.png", PaquetesC_paqPNGx8_png_h, (sizeof(PaquetesC_paqPNGx8_png_h) / sizeof(PaquetesC_paqPNGx8_png_h[0])) }
		};
		_myApp		= new AUScenesAdmin([_monoViewController getApp], _client, [_monoViewController iScene], "", packages, (sizeof(packages) / sizeof(packages[0])));
		[_monoViewController setAppAdmin:_myApp];
	}
	//Add content to window
	{
		[_window addSubview:_monoViewController.view];
		[_window makeKeyAndVisible];
	}
	//Disable auto-lock
	[UIApplication sharedApplication].idleTimerDisabled = YES;
	//
	//Test encrypt/decrypt
	/*{
	 const char* salt		= "dksfh jkladsfhkl ahsdflh ajklshdf luawyeiuryiouwy eioruyeowiry oiuawyru yaiouy dfy dsjklfhkladsfhkl ahsdflhal shfklahsklfhasjk fhklshfkldhsf";
	 const UI32 saltSz		= AUCadenaLarga8::tamano(salt);
	 const char* origMsg		= "EVP_DecryptInit_ex(), EVP_DecryptUpdate() and EVP_DecryptFinal_ex() are the corresponding decryption operations.";
	 const UI32 origMsgSz	= AUCadenaLarga8::tamano(origMsg);
	 AUCadenaLargaMutable8* encrypt = new AUCadenaLargaMutable8();
	 if(!NBMngrOSSecure::encWithGKey((const BYTE*)origMsg, origMsgSz, (const BYTE*)salt, saltSz, 1005, encrypt)){
	 PRINTF_CONSOLE_ERROR("Unable to encrypt.\n");
	 } else {
	 PRINTF_INFO("Encrypted from %d to %d bytes.\n", origMsgSz, encrypt->tamano());
	 AUCadenaLargaMutable8* decrypt = new AUCadenaLargaMutable8();
	 if(!NBMngrOSSecure::decWithGKey((const BYTE*)encrypt->str(), encrypt->tamano(), (const BYTE*)salt, saltSz - 1, 1005, decrypt)){
	 PRINTF_CONSOLE_ERROR("Unable to decrypt.\n");
	 } else {
	 NBASSERT(decrypt->tamano() == origMsgSz)
	 NBASSERT(decrypt->esIgual(origMsg))
	 PRINTF_INFO("Decrypted from %d to %d bytes.\n", encrypt->tamano(), decrypt->tamano());
	 PRINTF_INFO("Original message: '%s'.\n", origMsg);
	 PRINTF_INFO("Encrypt message: '%s'.\n", encrypt->str());
	 PRINTF_INFO("Decrypt message: '%s'.\n", decrypt->str());
	 }
	 decrypt->liberar(NB_RETENEDOR_THIS);
	 }
	 encrypt->liberar(NB_RETENEDOR_THIS);
	 }*/
	//Printing current locale
	{
		NSLog(@"[[NSLocale currentLocale] languageCode]: %@.\n", [[NSLocale currentLocale] languageCode]);
		//NSLog(@"[NSLocale availableLocaleIdentifiers]: %@.\n", [NSLocale availableLocaleIdentifiers]);
	}
	return YES;
}

//openURL, iOS8 and before
- (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation {
	AUApp* myApp = [_monoViewController getApp];
	NSLog(@"application openURL (iOS8-): %@.\n", url);
	return (myApp->broadcastOpenUrl([[url absoluteString] UTF8String], NULL, 0) ? YES : NO);
}

//openURL, iOS9+
- (BOOL)application:(UIApplication *)app openURL:(NSURL *)url options:(NSDictionary *)options {
	//sourceApplication: options[UIApplicationOpenURLOptionsSourceApplicationKey]
	//annotation: options[UIApplicationOpenURLOptionsAnnotationKey]
	NSLog(@"application openURL (iOS9+): %@.\n", url);
	AUApp* myApp = [_monoViewController getApp];
	return (myApp->broadcastOpenUrl([[url absoluteString] UTF8String], NULL, 0) ? YES : NO);
}

//Notifications

//didReceiveLocalNotification, iOS 9 and before
- (void)application:(UIApplication *)application didReceiveLocalNotification:(UILocalNotification *)notif {
	PRINTF_INFO("application didReceiveLocalNotification.\n");
	AUApp* app = [_monoViewController getApp];
	app->appLocalNotifReceived((__bridge void*)notif);
}

//didRegisterForRemoteNotificationsWithDeviceToken, iOS 3
- (void)application:(UIApplication *)application didRegisterForRemoteNotificationsWithDeviceToken:(NSData *)deviceToken {
	//This gets called by request or automatically when an deice is restored
	const void* token = [deviceToken bytes];
	const UI32 tokenSz = (UI32)[deviceToken length];
	NBMngrNotifs::remoteSetToken(token, tokenSz);
}

//didRegisterForRemoteNotificationsWithDeviceToken, iOS 3
- (void)application:(UIApplication *)application didFailToRegisterForRemoteNotificationsWithError:(NSError *)error {
	NBMngrNotifs::remoteSetToken(NULL, 0);
}

- (void)applicationWillResignActive:(UIApplication *)application {
	// Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
	// Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
}


- (void)applicationDidEnterBackground:(UIApplication *)application {
	// Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
	// If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}


- (void)applicationWillEnterForeground:(UIApplication *)application {
	// Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
}


- (void)applicationDidBecomeActive:(UIApplication *)application {
	// Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
	//Disable auto-lock
	[UIApplication sharedApplication].idleTimerDisabled = YES;
}


- (void)applicationWillTerminate:(UIApplication *)application {
	// Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end
