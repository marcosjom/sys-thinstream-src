//
//  AppDelegate.m
//  thinstream-osx
//
//  Created by Marcos Ortega on 8/5/18.
//

#include "visual/TSVisualPch.h"
#import "AppDelegate.h"

@interface AppDelegate ()

@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	//
}

/*- (BOOL)application:(NSApplication *)theApplication openFile:(NSString *)filename {
	//ToDo
	return NO;
}*/

- (void)applicationWillTerminate:(NSNotification *)aNotification {
	// Insert code here to tear down your application
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)theApplication {
	return YES;
}

/*- (BOOL)application:(NSApplication *)sender openFile:(NSString *)filename {
	return NO;
}*/

/*- (BOOL)application:(id)sender openFileWithoutUI:(NSString *)filename {
	return NO;
}*/

@end
