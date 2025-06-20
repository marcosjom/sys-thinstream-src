//
//  AppGLView.h
//  thinstream-osx
//
//  Created by Marcos Ortega on 8/5/18.
//

#import <Cocoa/Cocoa.h>
#import <QuartzCore/QuartzCore.h> // for display link (requires CoreVideo.framework)
#include "AUApp.h"
#include "visual/AUScenesAdmin.h"
#include "core/TSContext.h"
#include "core/config/TSCfg.h"
#include "core/client/TSClient.h"

@interface AppGLView : NSOpenGLView<NSTextInputClient> {
@private
	CVDisplayLinkRef	_displayLink;
	NSMutableString*	_fileToLoad;
	AUApp*				_app;
	float				_loadedScale;	//layer scale during load (used to scale every screen-change)
	STNBSize			_loadedDPI;		//dots-per-inch set during load (scaled every screen-change)
	bool				_mousePresionado;
	AUScenesAdmin*		_scenes;
	STTSCfg*			_cfg;
	STTSContext*	_context;
	STTSClient*			_client;
	STNBThreadMutex		_glMutex;
};

//@property (assign) IBOutlet NSWindow* windw;

@end


