//
//  AppDelegate.h
//  thinstream-ios
//
//  Created by Marcos Ortega on 8/5/18.
//

#import <UIKit/UIKit.h>
#include "AUIOSMonoViewController.h"
#include "visual/AUScenesAdmin.h"
#include "core/config/TSCfg.h"
#include "core/TSContext.h"

extern STTSCfg*			_cfg;
extern STTSContext*	_context;
extern STTSClient*		_client;

@interface AppDelegate : UIResponder <UIApplicationDelegate> {
	AUIOSMonoViewController*	_monoViewController;
	AUScenesAdmin*				_myApp;
}

@property (strong, nonatomic) UIWindow *window;

@end

