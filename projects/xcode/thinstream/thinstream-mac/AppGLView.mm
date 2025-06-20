//
//  AppGLView.m
//  thinstream-osx
//
//  Created by Marcos Ortega on 8/5/18.
//

#include "visual/TSVisualPch.h"
#import "AppGLView.h"
#include "visual/TSFonts.h"
//
#include "../../sys-thinstream-res/PaquetesC/paqFuentes.otf.h"
#include "../../sys-thinstream-res/PaquetesC/paqPNGx1.png.h"
#include "../../sys-thinstream-res/PaquetesC/paqPNGx2.png.h"
#include "../../sys-thinstream-res/PaquetesC/paqPNGx4.png.h"
#include "../../sys-thinstream-res/PaquetesC/paqPNGx8.png.h"
//
#include "../../sys-thinstream-res/PaquetesC/thinstream_ca.cert.der.h"
#include "../../sys-thinstream-res/PaquetesC/thinstream.cfg.app.json.h"
#include "../../sys-thinstream-res/PaquetesC/thinstream.cfg.app.locale.json.h"
//
#define LEER_PRECACHE		false
#define LEER_CACHE			false
#define ESCRIBIR_CACHE		false
#define PAQUETES_RUTA_BASE	""

//General notes about obj-c files (*.m, *.mm):
// @autoreleasepool {...} are important or releasing objetcs and memory.

@interface AppGLView (InternalMethods)

- (id)initcializarGL:(NSRect)frame;
- (CVReturn)getFrameForTime:(const CVTimeStamp *)outputTime;
- (void)drawFrame;

@end

@implementation AppGLView

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *now, const CVTimeStamp *outputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext){
	// go back to Obj-C for easy access to instance variables
	CVReturn result = [(AppGLView*)displayLinkContext getFrameForTime:outputTime];
	return result;
}

- (id)initWithCoder:(NSCoder*)aDecoder {
	self = [super initWithCoder:aDecoder];
	if (self){
		self = [self initcializarGL: [self frame]];
	}
	return self;
}

- (id)initWithFrame:(NSRect)frame {
	self = [self initcializarGL: frame];
	return self;
}

- (id)initcializarGL:(NSRect)frame {
	//PRINTF_INFO("-----initcializarGL.\n")
	id r = nil;
	//Init members
	{
		_displayLink	= nil;
		_fileToLoad 	= [[NSMutableString alloc] init];
		_app			= NULL;
		//
		_cfg			= NULL;
		_context		= NULL;
		_client			= NULL;
		NBThreadMutex_init(&_glMutex);
	}
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
		//TSCfg_loadFromFilePath(cfg, cfgPath);
		{
			STTSCfg* cfg = NBMemory_allocType(STTSCfg);
			TSCfg_init(cfg);
			if(!TSCfg_loadFromJsonStrBytes(cfg, (const char*)PaquetesC_thinstream_cfg_app_json_h, (sizeof(PaquetesC_thinstream_cfg_app_json_h) / sizeof(PaquetesC_thinstream_cfg_app_json_h[0])))){
				PRINTF_CONSOLE_ERROR("MAIN, could not load config file from memory.\n");
			} else {
				PRINTF_INFO("MAIN, config file from memory.\n");
				/*{
					const SI32 sz = (sizeof(PaquetesC_thinstream_cfg_app_json_h) / sizeof(PaquetesC_thinstream_cfg_app_json_h[0])); 
					STNBString str;
					NBString_initWithSz(&str, sz + 1, 64, 0.10f);
					{
						NBString_concatBytes(&str, (const char*)PaquetesC_thinstream_cfg_app_json_h, (sizeof(PaquetesC_thinstream_cfg_app_json_h) / sizeof(PaquetesC_thinstream_cfg_app_json_h[0])));
						PRINTF_INFO("MAIN, config: \n%s\n", str.str);
					}
					NBString_release(&str);
				}*/
				//NSString* localePath2 = [[NSBundle mainBundle] pathForResource:@"thinstream.cfg.app.locale" ofType:@"json"];
				//const char* localePath = [localePath2 UTF8String];
				const char* rootPath = rootPath2.str;
				STTSContext* context = NBMemory_allocType(STTSContext);
				TSContext_init(context);
				TSContext_setNotifMode(context, ENNBStorageNotifMode_Manual);
				if(!TSContext_prepare(context, cfg)){
					PRINTF_CONSOLE_ERROR("MAIN, TSContext_prepare failed.\n");
				} else if(!TSContext_loadLocaleStrBytes(context, (const char*)PaquetesC_thinstream_cfg_app_locale_json_h, (sizeof(PaquetesC_thinstream_cfg_app_locale_json_h) / sizeof(PaquetesC_thinstream_cfg_app_locale_json_h[0])))){
					PRINTF_CONSOLE_ERROR("MAIN, could not load context locale for config from memory.\n");
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
						}
						//Tmp: forcing a language
						//TSContext_pushPreferedLang(context, "de");
						//TSContext_pushPreferedLang(context, "en");
					}
					if(!TSContext_start(context)){
						PRINTF_CONSOLE_ERROR("MAIN, TSContext_start failed.\n");
					} else {
						//Start client
						STTSClient* client = NBMemory_allocType(STTSClient);
						TSClient_init(client);
						if(!TSClient_prepare(client, rootPath, context, cfg, PaquetesC_thinstream_ca_cert_der_h, (sizeof(PaquetesC_thinstream_ca_cert_der_h) / sizeof(PaquetesC_thinstream_ca_cert_der_h[0])))){
							PRINTF_CONSOLE_ERROR("MAIN, could not start client with config from memory.\n");
						} else {
							_cfg = cfg; cfg = NULL;
							_context = context; context = NULL;
							_client = client; client = NULL;
						}
						if(client != NULL){
							TSClient_release(client);
							NBMemory_free(client);
							client = NULL;
						}
					}
				}
				if(context != NULL){
					TSContext_stop(context);
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
		}
		NBString_release(&rootPath2);
	}
	// context setup
	{
		NSOpenGLPixelFormatAttribute attribs[] = {
			//NSOpenGLPFAWindow,			//If present, this attribute indicates that only renderers that are capable of rendering to a window are considered. This attribute is implied if neither NSOpenGLPFAFullScreen nor NSOpenGLPFAOffScreen is specified.
			NSOpenGLPFAColorSize, 32,
			//NSOpenGLPFAAccelerated,		//If present, this attribute indicates that only hardware-accelerated renderers are considered. If not present, accelerated renderers are still preferred.
			NSOpenGLPFADoubleBuffer,
			//NSOpenGLPFASingleRenderer,	//If present, this attribute indicates that a single rendering engine is chosen. On systems with multiple screens, this disables OpenGL’s ability to drive different monitors through different graphics accelerator cards with a single context. This attribute is not generally useful.
			0
		};
		NSOpenGLPixelFormat* windowedPixelFormat = [[NSOpenGLPixelFormat alloc] initWithAttributes:attribs];
		if (windowedPixelFormat == nil) {
			NSLog(@"Unable to create windowed pixel format.");
			exit(0);
		}
		r = [super initWithFrame:frame pixelFormat:windowedPixelFormat];
		if (r == nil) {
			NSLog(@"Unable to create a windowed OpenGL context.");
			exit(0);
		}
		[self setWantsBestResolutionOpenGLSurface:YES];
		[windowedPixelFormat release];
	}
	//
	// set synch to VBL to eliminate tearing
	GLint    vblSynch = 1; [[self openGLContext] setValues:&vblSynch forParameter:NSOpenGLCPSwapInterval];
	// set up the display link
	{
		CVDisplayLinkCreateWithActiveCGDisplays(&_displayLink);
		CVDisplayLinkSetOutputCallback(_displayLink, MyDisplayLinkCallback, self);
		CGLContextObj cglContext = (CGLContextObj)[[self openGLContext] CGLContextObj];
		CGLPixelFormatObj cglPixelFormat = (CGLPixelFormatObj)[[self pixelFormat] CGLPixelFormatObj];
		CVDisplayLinkSetCurrentCGDisplayFromOpenGLContext(_displayLink, cglContext, cglPixelFormat);
		if(_app != nil){
			PRINTF_INFO("initcializarGL, CVDisplayLinkStart");
			CVDisplayLinkStart(_displayLink);
		}
		// Register to be notified when the window closes so we can stop the displaylink
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(windowWillClose:)
													 name:NSWindowWillCloseNotification
												   object:[self window]];
	}
	return r;
}

- (void) windowWillClose:(NSNotification*)notification {
	//Note: this notification could be related to a panel or dialog window.
}

- (void)awakeFromNib {
	//PRINTF_INFO("-----awakeFromNib.\n")
}

- (BOOL)prepareNBApp {
	BOOL r = FALSE;
	//PRINTF_INFO("-----prepareNBApp.\n")
	if(_client != NULL && _app == NULL){
		//Start engine
		STAppCallbacks appCallbacks;
		AUApp::inicializarCallbacks(&appCallbacks);
		_app		= new AUApp(&appCallbacks, "thinstream", false /*permitirActividadRedEnBG*/);
		//
		AUApp_linkToInternalLibJpegRead();
		AUApp_linkToDefaultOSTools(_app);
		AUApp_linkToDefaultOSSecure(_app);
		AUApp_linkToDefaultPkgFilesystem(_app);
		AUApp_linkToDefaultKeyboard(_app);
		AUApp_linkToDefaultNotifSystem(_app);
		AUApp_linkToDefaultAVCapture(_app);
		//AUApp_linkToDefaultStore(_app);
		AUApp_linkToDefaultTelephony(_app);
		AUApp_linkToDefaultBiometrics(_app);
		//PDFRender
		//AUApp_linkToDefaultPdfKit(_app);
		//2021-05-25: disabled because MacOS requested ARM support and PDFium was not compiled fot it yet.
		//if(AUAppGlueApplePdfKit::isSupported()){
			//Use Apple PDFRender engine (MacOS 10.12+)
			NBMngrPdfKit::setGlue(_app, AUAppGlueApplePdfKit::create);
		/*} else {
			//Use PDFium engine
			NBMngrPdfKit::setGlue(_app, AUAppGluePdfium::create);
		}*/
		//
		if(!_app->inicializarMultimedia(LEER_PRECACHE, LEER_CACHE, ESCRIBIR_CACHE, true /*initGraphics*/, 60 /*screenFreq*/)){
			PRINTF_CONSOLE_ERROR("No se pudo inicializar el motor grafico\n"); NBASSERT(false);
		} else {
#			ifndef CONFIG_NB_UNSUPPORT_AUDIO_IO
			NBGestorSonidos::establecerGrupoAudioHabilitado(ENAudioGrupo_Efectos, false);
			NBGestorSonidos::establecerGrupoAudioHabilitado(ENAudioGrupo_Musica, false);
#			endif
			//Sync layer scale with backing scale (enable high-res)
			[[self layer] setContentsScale:[[self window] backingScaleFactor]];
			//Bounds and scales
			NSRect viewBounds			= [self bounds]; //view
			NSRect layerBounds			= [[self layer] bounds]; //logic
			NSRect backingBounds		= [self convertRectToBacking:viewBounds]; //OpenGL
			const float layerScale		= [[self layer] contentsScale]; //logic
			const float backingScale	= [[self window] backingScaleFactor]; //OpenGL
			const float sizeScale		= (layerScale == 0.0f ? 1.0f : layerScale);
			//Screen density
			//Reference: Monitor Benq G2220HDA a (1920 x 1080)pix es (102.5 x 102.5)dpi
			NSScreen* curScreen	= [[self window] screen];
			NSDictionary* desc	= [curScreen deviceDescription];
			const NSSize sz		= [[desc objectForKey:NSDeviceSize] sizeValue];
			NSSize dpi			= [[desc objectForKey:NSDeviceResolution] sizeValue];
			if(dpi.width < 72.0f) dpi.width = 72.0f;
			if(dpi.height < 72.0f) dpi.height = 72.0f;
			PRINTF_INFO("Staring-with-screen dpi(%f, %f) x scale(%f), size(%f, %f) x scale(%f).\n", dpi.width, dpi.height, sizeScale, sz.width, sz.height, sizeScale);
			PRINTF_INFO("prepareNBApp,\n  =    viewBounds(%f, %f)-(+%f, +%f)\n  =   layerBounds(%f, %f)-(+%f, +%f)\n  = backingBounds(%f, %f)-(+%f, +%f)\n  = layerScale(%f) backingScale(%f).\n", viewBounds.origin.x, viewBounds.origin.y, viewBounds.size.width, viewBounds.size.height, layerBounds.origin.x, layerBounds.origin.y, layerBounds.size.width, layerBounds.size.height, backingBounds.origin.x, backingBounds.origin.y, backingBounds.size.width, backingBounds.size.height, layerScale, backingScale);
			//Loaded dpi
			{
				_loadedScale	= sizeScale;
				_loadedDPI		= NBST_P(STNBSize, (float)dpi.width, (float)dpi.height);
			}
			//
			NBTamanoI wSize; NBTAMANO_ESTABLECER(wSize, (layerBounds.size.width * sizeScale), (layerBounds.size.height * sizeScale));
			NBTamano wDPI; NBTAMANO_ESTABLECER(wDPI, dpi.width, dpi.height)
			NBTamano wPPI; NBTAMANO_ESTABLECER(wPPI, (dpi.width * 100.0f) / 72.0f, (dpi.height * 100.0f) / 72.0f)
			if(!_app->inicializarVentana(wSize, wPPI, wPPI, ENGestorEscenaDestinoGl_Heredado)){
				PRINTF_CONSOLE_ERROR("No se pudo inicializar la ventana.\n");
			} else {
				NBASSERT(_app->indiceEscenaRender() >= 0)
				const STAppScenasPkgLoadDef paquetes[] = {
					{ "paqFuentes.otf", PaquetesC_paqFuentes_otf_h, (sizeof(PaquetesC_paqFuentes_otf_h) / sizeof(PaquetesC_paqFuentes_otf_h[0])) }
					, { "paqPNGx8.png", PaquetesC_paqPNGx1_png_h, (sizeof(PaquetesC_paqPNGx1_png_h) / sizeof(PaquetesC_paqPNGx1_png_h[0])) }
					, { "paqPNGx4.png", PaquetesC_paqPNGx2_png_h, (sizeof(PaquetesC_paqPNGx2_png_h) / sizeof(PaquetesC_paqPNGx2_png_h[0])) }
					, { "paqPNGx2.png", PaquetesC_paqPNGx4_png_h, (sizeof(PaquetesC_paqPNGx4_png_h) / sizeof(PaquetesC_paqPNGx4_png_h[0])) }
					, { "paqPNGx1.png", PaquetesC_paqPNGx8_png_h, (sizeof(PaquetesC_paqPNGx8_png_h) / sizeof(PaquetesC_paqPNGx8_png_h[0])) }
				};
				/*const STAppScenasPkgLoadDef paquetes[] = {
					{ "paqFuentes.otf", NULL, 0 }
					, { "paqPNGx8.png", NULL, 0 }
					, { "paqPNGx4.png", NULL, 0 }
					, { "paqPNGx2.png", NULL, 0 }
					, { "paqPNGx1.png", NULL, 0 }
				};*/
				_scenes = new AUScenesAdmin(_app, _client, _app->indiceEscenaRender(), PAQUETES_RUTA_BASE, paquetes, (sizeof(paquetes) / sizeof(paquetes[0])));
				if(!_app->inicializarJuego(_scenes)){
					PRINTF_CONSOLE_ERROR("No se pudo inicializar el app.\n");
				} else {
					PRINTF_INFO("App iniciada!\n");
					if(_displayLink != nil){
						PRINTF_INFO("prepareNBApp, CVDisplayLinkStart");
						CVDisplayLinkStart(_displayLink);
					}
					r = TRUE;
				}
			}
		}
	}
	return r;
}

- (void)prepareOpenGL {
	//PRINTF_INFO("-----prepareOpenGL.\n")
	[super prepareOpenGL];
	//
	if(_app == NULL){
		[self prepareNBApp];
	} else {
		PRINTF_CONSOLE_ERROR("-----prepareOpenGL when _app != NULL\n");
	}
}

- (BOOL) abrirArchivo:(NSString*)rutaArchivo {
	PRINTF_INFO("application:openURL: '%s'.\n", [rutaArchivo UTF8String]);
	[_fileToLoad setString:rutaArchivo];
	return YES;
}

- (CVReturn)getFrameForTime:(const CVTimeStamp *)outputTime {
	//PRINTF_INFO("-----getFrameForTime.\n");
	// deltaTime is unused in this bare bones demo, but here's how to calculate it using display link info
	//deltaTime = 1.0 / (outputTime->rateScalar * (double)outputTime->videoTimeScale / (double)outputTime->videoRefreshPeriod);
	[self drawFrame];
	return kCVReturnSuccess;
}

- (void)viewDidChangeBackingProperties {
	//PRINTF_INFO("-----viewDidChangeBackingProperties\n");
	[super viewDidChangeBackingProperties];
	[[self layer] setContentsScale:[[self window] backingScaleFactor]];
	//
	if(_app == NULL){
		PRINTF_CONSOLE_ERROR("-----viewDidChangeBackingProperties when _app == NULL\n");
		//[self prepareNBApp];
	}
	//
	[self reshape];
}

- (void)reshape {
	//PRINTF_INFO("-----reshape\n");
	if(_app == NULL){
		PRINTF_CONSOLE_ERROR("-----reshape when _app == NULL\n");
		//[self prepareNBApp];
	} else {
		NSRect viewBounds			= [self bounds]; //view
		NSRect layerBounds			= [[self layer] bounds]; //logic
		NSRect backingBounds		= [self convertRectToBacking:viewBounds]; //OpenGL
		const float layerScale		= [[self layer] contentsScale]; //logic
		const float backingScale	= [[self window] backingScaleFactor]; //OpenGL
		const float sizeScale		= (layerScale == 0.0f ? 1.0f : layerScale);
		//PRINTF_INFO("-----reshape\n  =    viewBounds(%f, %f)-(+%f, +%f)\n  =   layerBounds(%f, %f)-(+%f, +%f)\n  = backingBounds(%f, %f)-(+%f, +%f)\n  = layerScale(%f) backingScale(%f).\n", viewBounds.origin.x, viewBounds.origin.y, viewBounds.size.width, viewBounds.size.height, layerBounds.origin.x, layerBounds.origin.y, layerBounds.size.width, layerBounds.size.height, backingBounds.origin.x, backingBounds.origin.y, backingBounds.size.width, backingBounds.size.height, layerScale, backingScale);
		{
			NSScreen* curScreen	= [[self window] screen];
			NSDictionary* desc	= [curScreen deviceDescription];
			const NSSize sz		= [[desc objectForKey:NSDeviceSize] sizeValue];
			NSSize dpi			= [[desc objectForKey:NSDeviceResolution] sizeValue]; //in 1/72 points-per-inches
			if(dpi.width < 72.0f) dpi.width = 72.0f;
			if(dpi.height < 72.0f) dpi.height = 72.0f;
		}
		NSScreen* curScreen			= [[self window] screen];
		NSDictionary* desc			= [curScreen deviceDescription];
		const NSSize sz				= [[desc objectForKey:NSDeviceSize] sizeValue];
		NSSize dpi					= [[desc objectForKey:NSDeviceResolution] sizeValue]; //in 1/72 points-per-inches
		if(dpi.width < 72.0f) dpi.width = 72.0f;
		if(dpi.height < 72.0f) dpi.height = 72.0f;
		//PRINTF_INFO("-----reshape\n  =    cur-sceen-dpi(%f, %f)-size(%f, %f).\n", dpi.width, dpi.height, sz.width, sz.height);
		const NSRect backingRect	= [curScreen convertRectToBacking:layerBounds];
		const STNBSize szInchs		= { (float)(layerBounds.size.width / dpi.width), (float)(layerBounds.size.height / dpi.height) };
		const STNBSize ppi			= { (float)(backingRect.size.width / szInchs.width), (float)(backingRect.size.height / szInchs.height)  }; //pixel-per-inches
		//PRINTF_INFO("-----reshape\n  =    szInchs(%.2f, %.2f)-diag(%.2f) cur-sceen-ppi(%f, %f)-size(%f, %f).\n", szInchs.width, szInchs.height, sqrtf((szInchs.width * szInchs.width) + (szInchs.height * szInchs.height)), ppi.width, ppi.height, backingRect.size.width, backingRect.size.height);
		//
		NSOpenGLContext *currentContext = [self openGLContext];
		if([currentContext view] != nil){
			[currentContext makeCurrentContext];
			// remember to lock the context before we touch it since display link is threaded
			CGLLockContext((CGLContextObj)[currentContext CGLContextObj]);
			// let the context know we've changed size
			[[self openGLContext] update];
			//Relative auto-resize
			/*if(_app != NULL){
				NBASSERT(_loadedScale > 0 && _loadedDPI.width > 0 && _loadedDPI.height > 0)
				const float relWinScale = (_loadedScale / sizeScale);
				NBTamanoI wSize; NBTAMANO_ESTABLECER(wSize, (layerBounds.size.width * sizeScale), (layerBounds.size.height * sizeScale));
				NBTamano wDPI; NBTAMANO_ESTABLECER(wDPI, dpi.width, dpi.height) //in 1/72 points-per-inches
				NBTamano wDPIScale; NBTAMANO_ESTABLECER(wDPIScale, _loadedDPI.width / wDPI.ancho, _loadedDPI.height / wDPI.alto) //in 1/72 points-per-inches
				NBTamano wDPIToUse; NBTAMANO_ESTABLECER(wDPIToUse, (wDPI.ancho * relWinScale) / wDPIScale.ancho, (wDPI.alto * relWinScale) / wDPIScale.alto);
				NBTamano wPPIToUse; NBTAMANO_ESTABLECER(wPPIToUse, (wDPIToUse.ancho * 100.f) / 72.0f, (wDPIToUse.alto * 100.0f) / 72.0f);
				PRINTF_INFO("-----reshape\n  =    wSize(%d, %d) x scale(%f) => [relWinScaleVsLoad(%f)]\n  =    wDPI(%f, %f) wDPIToUse(%f, %f).\n", wSize.ancho, wSize.alto, sizeScale, relWinScale, wDPI.ancho, wDPI.alto, wDPIToUse.ancho, wDPIToUse.alto);
				_app->notificarRedimensionVentana(wSize, relWinScale, wPPIToUse, wPPIToUse);
			}*/
			//Simple resize
			if(_app != NULL){
				NBTamanoI wSize; NBTAMANO_ESTABLECER(wSize, (layerBounds.size.width * sizeScale), (layerBounds.size.height * sizeScale));
				NBTamano wDPI; NBTAMANO_ESTABLECER(wDPI, dpi.width, dpi.height)
				NBTamano wPPI; NBTAMANO_ESTABLECER(wPPI, (dpi.width * 100.0f) / 72.0f, (dpi.height * 100.0f) / 72.0f)
				_app->notificarRedimensionVentana(wSize, 1.0f, wPPI, wPPI);
			}
			//Loaded dpi
			{
				_loadedScale	= sizeScale;
				_loadedDPI		= NBST_P(STNBSize, (float)dpi.width, (float)dpi.height );
			}
			CGLUnlockContext((CGLContextObj)[currentContext CGLContextObj]);
		}
	}
	//super
	[super reshape];
}

- (void)drawRect:(NSRect)rect {
	//PRINTF_INFO("-----drawRect\n");
	[self drawFrame];
}

- (void)drawFrame {
	//PRINTF_INFO("-----drawFrame\n");
	NBThreadMutex_lock(&_glMutex);
	{
		NSOpenGLContext* currentContext = [self openGLContext];
		NSView* view = [currentContext view];
		if(view != nil){
			[currentContext makeCurrentContext];
			// must lock GL context because display link is threaded
			CGLLockContext((CGLContextObj)[currentContext CGLContextObj]);
			if(_app != NULL){
				//Bug en OSX (depende del SDK y el OS):
				//A veces el frameBuffer es undefined al arancar el App o al cambiar a pantalla completa.
				//Para evitar que el primer glClear produzca error, se debe verificar el estado del frameBuffer heredado (fbId=0)
				NBGestorGL::bindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
				const GLenum estadoFBO = NBGestorGL::checkFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
				if(estadoFBO == GL_FRAMEBUFFER_COMPLETE_EXT){
					@autoreleasepool {
						_app->tickJuego(ENAUAppTickTipo_SincPantalla, false);
					}
				}
			}
			[currentContext flushBuffer];
			CGLUnlockContext((CGLContextObj)[currentContext CGLContextObj]);
		}
	}
	NBThreadMutex_unlock(&_glMutex);
}

- (void)dealloc {
	if(_displayLink != nil){
		PRINTF_INFO("dealloc, CVDisplayLinkStop");
		CVDisplayLinkStop(_displayLink);
		CVDisplayLinkRelease(_displayLink);
		_displayLink = nil;
	}
	if(_app != NULL){
		_app->finalizarMultimedia();
		delete _app;
		_app = NULL;
	}
	[_fileToLoad release];
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
	[super dealloc];
}

// ------------------------------
// -- Manejo de eventos de teclado
// ------------------------------

- (void)keyDown:(NSEvent *)theEvent {
	[self interpretKeyEvents:[NSArray arrayWithObject:theEvent]];
}

// ------------------------------
// -- Manejo de eventos de mouse
// ------------------------------
- (BOOL)acceptsFirstResponder {
	return YES;
}


- (void)mouseDown:(NSEvent *)theEvent {
	const NSPoint viewPoint		= [self convertPoint:[theEvent locationInWindow] fromView:nil];	//screen
	const NSRect layerBounds	= [[self layer] bounds]; //layer-logic
	const NSPoint layerPoint	= [[self layer] convertPoint:viewPoint toLayer:nil];
	const float layerScale		= [[self layer] contentsScale];
	const float sizeScale		= (layerScale == 0.0f ? 1.0f : layerScale);
	const float yPosInv			= (layerBounds.size.height - layerPoint.y);
	//PRINTF_INFO("mouseDown viewPoint(%f, %f) curPoint(%f, %f) rel(%f, %f).\n", viewPoint.x, viewPoint.y, curPoint.x, yPosInv, (curPoint.x - curBounds.origin.x) / curBounds.size.width, (yPosInv - curBounds.origin.y) / curBounds.size.height);
	if(_app != NULL){
		if(_mousePresionado){
			NBGestorTouches::touchFinalizar(1, (layerPoint.x * sizeScale), (yPosInv * sizeScale), false);
		}
		NBGestorTouches::touchIniciar(1, (layerPoint.x * sizeScale), (yPosInv * sizeScale));
		_mousePresionado = true;
	}
}

- (void)mouseUp:(NSEvent *)theEvent {
	const NSPoint viewPoint		= [self convertPoint:[theEvent locationInWindow] fromView:nil];	//screen
	const NSRect layerBounds	= [[self layer] bounds]; //layer-logic
	const NSPoint layerPoint	= [[self layer] convertPoint:viewPoint toLayer:nil];
	const float layerScale		= [[self layer] contentsScale];
	const float sizeScale		= (layerScale == 0.0f ? 1.0f : layerScale);
	const float yPosInv			= (layerBounds.size.height - layerPoint.y);
	//PRINTF_INFO("mouseUp viewPoint(%f, %f) curPoint(%f, %f) rel(%f, %f).\n", viewPoint.x, viewPoint.y, curPoint.x, yPosInv, (curPoint.x - curBounds.origin.x) / curBounds.size.width, (yPosInv - curBounds.origin.y) / curBounds.size.height);
	if(_app != NULL){
		if(_mousePresionado){
			NBGestorTouches::touchFinalizar(1, (layerPoint.x * sizeScale), (yPosInv * sizeScale), false);
		}
		_mousePresionado = false;
	}
}

- (void)mouseDragged:(NSEvent *)theEvent {
	const NSPoint viewPoint		= [self convertPoint:[theEvent locationInWindow] fromView:nil];	//screen
	const NSRect layerBounds	= [[self layer] bounds]; //layer-logic
	const NSPoint layerPoint	= [[self layer] convertPoint:viewPoint toLayer:nil];
	const float layerScale		= [[self layer] contentsScale];
	const float sizeScale		= (layerScale == 0.0f ? 1.0f : layerScale);
	const float yPosInv			= (layerBounds.size.height - layerPoint.y);
	//PRINTF_INFO("mouseDragged viewPoint(%f, %f) curPoint(%f, %f) rel(%f, %f).\n", viewPoint.x, viewPoint.y, curPoint.x, yPosInv, (curPoint.x - curBounds.origin.x) / curBounds.size.width, (yPosInv - curBounds.origin.y) / curBounds.size.height);
	if(_app != NULL){
		if(_mousePresionado){
			NBGestorTouches::touchMover(1, (layerPoint.x * sizeScale), (yPosInv * sizeScale));
		}
	}
}

- (void)mouseMoved:(NSEvent *)theEvent {
	const NSPoint viewPoint		= [self convertPoint:[theEvent locationInWindow] fromView:nil];	//screen
	const NSRect layerBounds	= [[self layer] bounds]; //layer-logic
	const NSPoint layerPoint	= [[self layer] convertPoint:viewPoint toLayer:nil];
	const float layerScale		= [[self layer] contentsScale];
	const float sizeScale		= (layerScale == 0.0f ? 1.0f : layerScale);
	const float yPosInv			= (layerBounds.size.height - layerPoint.y);
	//PRINTF_INFO("mouseDragged viewPoint(%f, %f) curPoint(%f, %f) rel(%f, %f).\n", viewPoint.x, viewPoint.y, curPoint.x, yPosInv, (curPoint.x - curBounds.origin.x) / curBounds.size.width, (yPosInv - curBounds.origin.y) / curBounds.size.height);
	if(_app != NULL){
		NBGestorTouches::hoverMover(1, (layerPoint.x * sizeScale), (yPosInv * sizeScale));
	}
}

- (void)scrollWheel:(NSEvent *)theEvent {
	const NSPoint viewPoint		= [self convertPoint:[theEvent locationInWindow] fromView:nil];	//screen
	const NSRect layerBounds	= [[self layer] bounds]; //layer-logic
	const NSPoint layerPoint	= [[self layer] convertPoint:viewPoint toLayer:nil];
	const float layerScale		= [[self layer] contentsScale];
	const float sizeScale		= (layerScale == 0.0f ? 1.0f : layerScale);
	const float yPosInv			= (layerBounds.size.height - layerPoint.y);
	//NSLog(@"scrollWheel viewPoint(%f, %f) delta(%f, %f) deltaZ(%f): %@.\n", viewPoint.x, viewPoint.y, [theEvent deltaX], [theEvent deltaY], [theEvent deltaZ], theEvent);
	if(_app != NULL){
		const float deltaScale	= 5.0f;
		const STNBPoint posPort	= { (float)(layerPoint.x * sizeScale), (float)(yPosInv * sizeScale) };
		const STNBSize scrollSz	= { (float)([theEvent deltaX] * sizeScale * deltaScale), -(float)([theEvent deltaY] * sizeScale * deltaScale)};
		NBGestorTouches::scrollAdd(posPort, scrollSz, FALSE);
	}
}

- (void)magnifyWithEvent:(NSEvent *)theEvent {
	NSLog(@"magnifyWithEvent: '%@'.\n", theEvent);
	const NSPoint viewPoint		= [self convertPoint:[theEvent locationInWindow] fromView:nil];	//screen
	const NSRect layerBounds	= [[self layer] bounds]; //layer-logic
	const NSPoint layerPoint	= [[self layer] convertPoint:viewPoint toLayer:nil];
	const float layerScale		= [[self layer] contentsScale];
	const float sizeScale		= (layerScale == 0.0f ? 1.0f : layerScale);
	const float yPosInv			= (layerBounds.size.height - layerPoint.y);
	//NSLog(@"scrollWheel viewPoint(%f, %f) delta(%f, %f) deltaZ(%f): %@.\n", viewPoint.x, viewPoint.y, [theEvent deltaX], [theEvent deltaY], [theEvent deltaZ], theEvent);
	if(_app != NULL){
		//const float deltaScale	= 5.0f;
		const STNBPoint posPort	= { (float)(layerPoint.x * sizeScale), (float)(yPosInv * sizeScale) };
		//const STNBSize scrollSz	= { ([theEvent deltaX] * sizeScale * deltaScale), -([theEvent deltaY] * sizeScale * deltaScale)};
		NBGestorTouches::magnifyAdd(posPort, [theEvent magnification], FALSE);
	}
}

- (void)smartMagnifyWithEvent:(NSEvent *)theEvent {
	//Double tap with two fingers on trackpad
	NSLog(@"smartMagnifyWithEvent: '%@'.\n", theEvent);
	const NSPoint viewPoint		= [self convertPoint:[theEvent locationInWindow] fromView:nil];	//screen
	const NSRect layerBounds	= [[self layer] bounds]; //layer-logic
	const NSPoint layerPoint	= [[self layer] convertPoint:viewPoint toLayer:nil];
	const float layerScale		= [[self layer] contentsScale];
	const float sizeScale		= (layerScale == 0.0f ? 1.0f : layerScale);
	const float yPosInv			= (layerBounds.size.height - layerPoint.y);
	//NSLog(@"scrollWheel viewPoint(%f, %f) delta(%f, %f) deltaZ(%f): %@.\n", viewPoint.x, viewPoint.y, [theEvent deltaX], [theEvent deltaY], [theEvent deltaZ], theEvent);
	if(_app != NULL){
		//const float deltaScale	= 5.0f;
		const STNBPoint posPort	= { (float)(layerPoint.x * sizeScale), (float)(yPosInv * sizeScale) };
		//const STNBSize scrollSz	= { ([theEvent deltaX] * sizeScale * deltaScale), -([theEvent deltaY] * sizeScale * deltaScale)};
		NBGestorTouches::magnifyAdd(posPort, 0.0f, TRUE);
	}
}

//----------------------
//-- NSTextInputClient
//----------------------

//-- NSTextInputClient - Handling Marked Text

- (BOOL)hasMarkedText {
	BOOL r = NO;
	NBGestorTeclas::entradaLockForBatch();
	{
		PRINTF_INFO("<<<NSTextInputClient>>> unmarkText.\n");
		const NBRangoI rng = NBGestorTeclas::entradaRangoMarcadoAbs();
		if(rng.tamano > 0){
			r = YES;
		}
	}
	NBGestorTeclas::entradaUnlockFromBatch();
	return r;
}

- (NSRange)markedRange {
	NSRange r;
	NBGestorTeclas::entradaLockForBatch();
	{
		const NBRangoI rng = NBGestorTeclas::entradaRangoMarcadoAbs();
		PRINTF_INFO("<<<NSTextInputClient>>> markedRange(%d, %d).\n", rng.inicio, rng.tamano);
		r.location	= rng.inicio;
		r.length	= rng.tamano;
	}
	NBGestorTeclas::entradaUnlockFromBatch();
	return r;
}

- (NSRange)selectedRange {
	NSRange r;
	NBGestorTeclas::entradaLockForBatch();
	{
		const NBRangoI rng = NBGestorTeclas::entradaRangoSeleccionAbs();
		PRINTF_INFO("<<<NSTextInputClient>>> selectedRange(%d, %d).\n", rng.inicio, rng.tamano);
		r.location	= rng.inicio;
		r.length	= rng.tamano;
	}
	NBGestorTeclas::entradaUnlockFromBatch();
	return r;
}

- (void)setMarkedText:(id)string selectedRange:(NSRange)selectedRange replacementRange:(NSRange)replacementRange {
	NBGestorTeclas::entradaLockForBatch();
	{
		PRINTF_INFO("<<<NSTextInputClient>>> setMarkedText selectedRange(%d, %d) replacementRange(%d, %d).\n", (SI32)selectedRange.location, (SI32)selectedRange.length, (SI32)replacementRange.location, (SI32)replacementRange.length);
		NBGestorTeclas::entradaRangoMarcadoEstablecer((SI32)selectedRange.location, (SI32)selectedRange.length);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

- (void)unmarkText {
	NBGestorTeclas::entradaLockForBatch();
	{
		PRINTF_INFO("<<<NSTextInputClient>>> unmarkText.\n");
		NBGestorTeclas::entradaRangoDesmarcar();
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

- (NSArray<NSAttributedStringKey> *)validAttributesForMarkedText {
	return [NSArray<NSAttributedStringKey> array];
}

//-- NSTextInputClient - Storing Text

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
	NSAttributedString* r = nil;
	NBGestorTeclas::entradaLockForBatch();
	{
		PRINTF_INFO("<<<NSTextInputClient>>> attributedSubstringForProposedRange range(%d, %d).\n", (SI32)range.location, (SI32)range.length);
		AUCadenaMutable8* dst = new AUCadenaMutable8();
		NBGestorTeclas::entradaTextoEnCharDefsContenido((SI32)range.location, (SI32)range.length, dst);
		r = [[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:dst->str()]];
		[r autorelease];
		dst->liberar(NB_RETENEDOR_THIS);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
	return r;
}

- (void)insertText:(id)string replacementRange:(NSRange)replacementRange {
	NBGestorTeclas::entradaLockForBatch();
	{
		if(replacementRange.location == NSNotFound){
			NSLog(@"<<<NSTextInputClient>>> insertText(%@) replacementRange(NSNotFound).\n", string);
			NBGestorTeclas::entradaIntroducirTexto([string UTF8String], FALSE);
		} else {
			NSLog(@"<<<NSTextInputClient>>> insertText(%@) replacementRange(%d, %d).\n", string, (SI32)replacementRange.location, (SI32)replacementRange.length);
			NBGestorTeclas::entradaTextoEnCharDefsReemplazar((SI32)replacementRange.location, (SI32)replacementRange.length, [string UTF8String], ENTextRangeSet_None);
		}
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

//-- NSTextInputClient - Getting Character Coordinates

- (NSUInteger)characterIndexForPoint:(NSPoint)point {
	NSUInteger r = 0;
	NBGestorTeclas::entradaLockForBatch();
	{
		NSLog(@"<<<NSTextInputClient>>> characterIndexForPoint(%f, %f).\n", point.x, point.y);
		r = NBGestorTeclas::entradaIndiceCharDefMasCercano(point.x, point.y);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
	return r;
}

- (NSRect)firstRectForCharacterRange:(NSRange)range actualRange:(NSRangePointer)actualRange {
	NSRect r;
	NBGestorTeclas::entradaLockForBatch();
	{
		NSLog(@"<<<NSTextInputClient>>> firstRectForCharacterRange(%d, %d).\n", (UI32)range.location, (UI32)range.length);
		const NBRectangulo rect = NBGestorTeclas::entradaPrimerRectanguloParaCharDefs((UI32)range.location, (UI32)range.length);
		r.origin.x		= rect.x;
		r.origin.y		= rect.y;
		r.size.width	= rect.ancho;
		r.size.height	= rect.alto;
	}
	NBGestorTeclas::entradaUnlockFromBatch();
	return r;
}

//-- NSTextInputClient - Binding Keystrokes

- (void)doCommandBySelector:(SEL)selector {
	if([self respondsToSelector:selector]){
		[self performSelector:selector];
	} else {
		NSLog(@"<<<NSTextInputClient>>> doCommandBySelector(%@) ignored.\n", NSStringFromSelector(selector));
	}
}

//-- NSTextInputClient - Optional Methods

- (NSAttributedString *)attributedString {
	NSAttributedString* r = nil;
	NBGestorTeclas::entradaLockForBatch();
	{
		PRINTF_INFO("<<<NSTextInputClient>>> attributedString.\n");
		AUCadenaMutable8* dst = new AUCadenaMutable8();
		NBGestorTeclas::entradaTexto(dst);
		r = [[NSAttributedString alloc] initWithString:[NSString stringWithUTF8String:dst->str()]];
		[r autorelease];
		dst->liberar(NB_RETENEDOR_THIS);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
	return r;
}

/*- (CGFloat)fractionOfDistanceThroughGlyphForPoint:(NSPoint)point {
	NSLog(@"<<<NSTextInputClient>>> fractionOfDistanceThroughGlyphForPoint(%f, %f).\n", point.x, point.y);
}*/

/*- (CGFloat)baselineDeltaForCharacterAtIndex:(NSUInteger)anIndex {
	NSLog(@"<<<NSTextInputClient>>> baselineDeltaForCharacterAtIndex(%u).\n", anIndex);
}*/

/*- (NSInteger)windowLevel {
	NSLog(@"<<<NSTextInputClient>>> windowLevel.\n");
}*/

/*- (BOOL)drawsVerticallyForCharacterAtIndex:(NSUInteger)charIndex {
	NSLog(@"<<<NSTextInputClient>>> drawsVerticallyForCharacterAtIndex(%u).\n", charIndex);
}*/

//----------------------
//-- NSStandardKeyBindingResponding
//----------------------

-(void) cancelOperation:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> cancelOperation.\n");
}

-(void) capitalizeWord:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> capitalizeWord.\n");
}

-(void) centerSelectionInVisibleArea:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> centerSelectionInVisibleArea.\n");
}

-(void) changeCaseOfLetter:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> changeCaseOfLetter.\n");
}

-(void) complete:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> complete.\n");
}

-(void) deleteBackward:(id)sender {
	NBGestorTeclas::entradaLockForBatch();
	{
		NSLog(@"deleteBackward.\n");
		NBGestorTeclas::entradaBackspace(false);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) deleteBackwardByDecomposingPreviousCharacter:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> deleteBackwardByDecomposingPreviousCharacter.\n");
}

-(void) deleteForward:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> deleteForward.\n");
}

-(void) deleteToBeginningOfLine:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> deleteToBeginningOfLine.\n");
}

-(void) deleteToBeginningOfParagraph:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> deleteToBeginningOfParagraph.\n");
}

-(void) deleteToEndOfLine:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> deleteToEndOfLine.\n");
}

-(void) deleteToEndOfParagraph:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> deleteToEndOfParagraph.\n");
}

-(void) deleteToMark:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> deleteToMark.\n");
}

-(void) deleteWordBackward:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> deleteWordBackward.\n");
}

-(void) deleteWordForward:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> deleteWordForward.\n");
}

/*-(void) doCommandBySelector:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> doCommandBySelector.\n");
}*/

-(void) indent:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> indent.\n");
}

-(void) insertBacktab:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> insertBacktab.\n");
}

-(void) insertContainerBreak:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> insertContainerBreak.\n");
}

-(void) insertDoubleQuoteIgnoringSubstitution:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> insertDoubleQuoteIgnoringSubstitution.\n");
}

-(void) insertLineBreak:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> insertLineBreak.\n");
}

-(void) insertNewline:(id)sender {
	NBGestorTeclas::entradaLockForBatch();
	{
		NSLog(@"insertNewline.\n");
		NBGestorTeclas::entradaIntroducirTexto("\n", false);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) insertNewlineIgnoringFieldEditor:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> insertNewlineIgnoringFieldEditor.\n");
}

-(void) insertParagraphSeparator:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> insertParagraphSeparator.\n");
}

-(void) insertSingleQuoteIgnoringSubstitution:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> insertSingleQuoteIgnoringSubstitution.\n");
}

-(void) insertTab:(id)sender {
	NBGestorTeclas::entradaLockForBatch();
	{
		NSLog(@"insertTab.\n");
		NBGestorTeclas::entradaIntroducirTexto("\t", false);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) insertTabIgnoringFieldEditor:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> insertTabIgnoringFieldEditor.\n");
}

-(void) insertText:(id)aString {
	NBGestorTeclas::entradaLockForBatch();
	{
		NSLog(@"insertText(%@).\n", aString);
		NBGestorTeclas::entradaIntroducirTexto([aString UTF8String], false);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) lowercaseWord:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> lowercaseWord.\n");
}

-(void) makeBaseWritingDirectionLeftToRight:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> makeBaseWritingDirectionLeftToRight.\n");
}

-(void) makeBaseWritingDirectionNatural:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> makeBaseWritingDirectionNatural.\n");
}

-(void) makeBaseWritingDirectionRightToLeft:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> makeBaseWritingDirectionRightToLeft.\n");
}

-(void) makeTextWritingDirectionLeftToRight:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> makeTextWritingDirectionLeftToRight.\n");
}

-(void) makeTextWritingDirectionNatural:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> makeTextWritingDirectionNatural.\n");
}

-(void) makeTextWritingDirectionRightToLeft:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> makeTextWritingDirectionRightToLeft.\n");
}

-(void) moveBackward:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveBackward.\n");
}

-(void) moveBackwardAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveBackwardAndModifySelection.\n");
}

-(void) moveDown:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveDown.\n");
}

-(void) moveDownAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveDownAndModifySelection.\n");
}

-(void) moveForward:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveForward.\n");
}

-(void) moveForwardAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveForwardAndModifySelection.\n");
}

-(void) moveLeft:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveLeft.\n");
	NBGestorTeclas::entradaLockForBatch();
	{
		const NBRangoI rng = NBGestorTeclas::entradaRangoSeleccion();
		if(rng.inicio > 0){
			NBGestorTeclas::entradaRangoSeleccionEstablecer(rng.inicio + rng.tamano - 1, 0);
		}
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) moveLeftAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveLeftAndModifySelection.\n");
	NBGestorTeclas::entradaLockForBatch();
	{
		const NBRangoI rng = NBGestorTeclas::entradaRangoSeleccion();
		if((rng.inicio + rng.tamano) > 0){
			NBGestorTeclas::entradaRangoSeleccionEstablecer(rng.inicio, rng.tamano - 1);
		}
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) moveParagraphBackwardAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveParagraphBackwardAndModifySelection.\n");
}

-(void) moveParagraphForwardAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveParagraphForwardAndModifySelection.\n");
}

-(void) moveRight:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveRight.\n");
	NBGestorTeclas::entradaLockForBatch();
	{
		const NBRangoI rng = NBGestorTeclas::entradaRangoSeleccion();
		NBGestorTeclas::entradaRangoSeleccionEstablecer(rng.inicio + rng.tamano + 1, 0);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) moveRightAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveRightAndModifySelection.\n");
	NBGestorTeclas::entradaLockForBatch();
	{
		const NBRangoI rng = NBGestorTeclas::entradaRangoSeleccion();
		NBGestorTeclas::entradaRangoSeleccionEstablecer(rng.inicio, rng.tamano + 1);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) moveToBeginningOfDocument:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToBeginningOfDocument.\n");
	NBGestorTeclas::entradaLockForBatch();
	{
		NBGestorTeclas::entradaRangoSeleccionEstablecer(0, 0);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) moveToBeginningOfDocumentAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToBeginningOfDocumentAndModifySelection.\n");
	NBGestorTeclas::entradaLockForBatch();
	{
		const NBRangoI rng = NBGestorTeclas::entradaRangoSeleccion();
		NBGestorTeclas::entradaRangoSeleccionEstablecer(rng.inicio, -rng.inicio);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) moveToBeginningOfLine:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToBeginningOfLine.\n");
}

-(void) moveToBeginningOfLineAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToBeginningOfLineAndModifySelection.\n");
}

-(void) moveToBeginningOfParagraph:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToBeginningOfParagraph.\n");
}

-(void) moveToBeginningOfParagraphAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToBeginningOfParagraphAndModifySelection.\n");
}

-(void) moveToEndOfDocument:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToEndOfDocument.\n");
	NBGestorTeclas::entradaLockForBatch();
	{
		const UI32 lastCharIdx = NBGestorTeclas::entradaIndiceCharDefUltimo();
		NBGestorTeclas::entradaRangoSeleccionEstablecer(lastCharIdx + 1, 0);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) moveToEndOfDocumentAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToEndOfDocumentAndModifySelection.\n");
	NBGestorTeclas::entradaLockForBatch();
	{
		const NBRangoI rng = NBGestorTeclas::entradaRangoSeleccion();
		const UI32 lastCharIdx = NBGestorTeclas::entradaIndiceCharDefUltimo();
		NBGestorTeclas::entradaRangoSeleccionEstablecer(rng.inicio, lastCharIdx + 1 - rng.inicio);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) moveToEndOfLine:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToEndOfLine.\n");
}

-(void) moveToEndOfLineAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToEndOfLineAndModifySelection.\n");
}

-(void) moveToEndOfParagraph:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToEndOfParagraph.\n");
}

-(void) moveToEndOfParagraphAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToEndOfParagraphAndModifySelection.\n");
}

-(void) moveToLeftEndOfLine:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToLeftEndOfLine.\n");
	NBGestorTeclas::entradaLockForBatch();
	{
		const NBRangoI rng		= NBGestorTeclas::entradaRangoSeleccion();
		const NBRangoI rngLn	= NBGestorTeclas::entradaRangoLineaDeCharDef(rng.inicio + rng.tamano);
		NBGestorTeclas::entradaRangoSeleccionEstablecer(rngLn.inicio, 0);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) moveToLeftEndOfLineAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToLeftEndOfLineAndModifySelection.\n");
	NBGestorTeclas::entradaLockForBatch();
	{
		const NBRangoI rng		= NBGestorTeclas::entradaRangoSeleccion();
		const NBRangoI rngLn	= NBGestorTeclas::entradaRangoLineaDeCharDef(rng.inicio + rng.tamano);
		NBGestorTeclas::entradaRangoSeleccionEstablecer(rng.inicio, rngLn.inicio - rng.inicio);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) moveToRightEndOfLine:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToRightEndOfLine.\n");
	NBGestorTeclas::entradaLockForBatch();
	{
		const NBRangoI rng		= NBGestorTeclas::entradaRangoSeleccion();
		const NBRangoI rngLn	= NBGestorTeclas::entradaRangoLineaDeCharDef(rng.inicio + rng.tamano);
		NBGestorTeclas::entradaRangoSeleccionEstablecer(rngLn.inicio + rngLn.tamano, 0);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) moveToRightEndOfLineAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveToRightEndOfLineAndModifySelection.\n");
	NBGestorTeclas::entradaLockForBatch();
	{
		const NBRangoI rng		= NBGestorTeclas::entradaRangoSeleccion();
		const NBRangoI rngLn	= NBGestorTeclas::entradaRangoLineaDeCharDef(rng.inicio + rng.tamano);
		NBGestorTeclas::entradaRangoSeleccionEstablecer(rng.inicio, rngLn.inicio + rngLn.tamano - rng.inicio);
	}
	NBGestorTeclas::entradaUnlockFromBatch();
}

-(void) moveUp:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveUp.\n");
}

-(void) moveUpAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveUpAndModifySelection.\n");
}

-(void) moveWordBackward:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveWordBackward.\n");
}

-(void) moveWordBackwardAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveWordBackwardAndModifySelection.\n");
}

-(void) moveWordForward:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveWordForward.\n");
}

-(void) moveWordForwardAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveWordForwardAndModifySelection.\n");
}

-(void) moveWordLeft:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveWordLeft.\n");
}

-(void) moveWordLeftAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveWordLeftAndModifySelection.\n");
}

-(void) moveWordRight:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveWordRight.\n");
}

-(void) moveWordRightAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> moveWordRightAndModifySelection.\n");
}

-(void) pageDown:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> pageDown.\n");
}

-(void) pageDownAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> pageDownAndModifySelection.\n");
}

-(void) pageUp:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> pageUp.\n");
}

-(void) pageUpAndModifySelection:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> pageUpAndModifySelection.\n");
}

-(void) quickLookPreviewItems:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> quickLookPreviewItems.\n");
}

-(void) scrollLineDown:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> scrollLineDown.\n");
}

-(void) scrollLineUp:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> scrollLineUp.\n");
}

-(void) scrollPageDown:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> scrollPageDown.\n");
}

-(void) scrollPageUp:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> scrollPageUp.\n");
}

-(void) scrollToBeginningOfDocument:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> scrollToBeginningOfDocument.\n");
}

-(void) scrollToEndOfDocument:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> scrollToEndOfDocument.\n");
}

-(void) selectAll:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> selectAll.\n");
}

-(void) selectLine:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> selectLine.\n");
}

-(void) selectParagraph:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> selectParagraph.\n");
}

-(void) selectToMark:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> selectToMark.\n");
}

-(void) selectWord:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> selectWord.\n");
}

-(void) setMark:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> setMark.\n");
}

-(void) swapWithMark:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> swapWithMark.\n");
}

-(void) transpose:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> transpose.\n");
}

-(void) transposeWords:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> transposeWords.\n");
}

-(void) uppercaseWord:(id)sender {
	NSLog(@"<<<NSTextInputClient>>> uppercaseWord.\n");
}

-(void) yank :(id)sender {
	NSLog(@"<<<NSTextInputClient>>> yank.\n");
}

@end

