//
//  Xserver.h
//
//  Created by Andreas Monitzer on January 6, 2001.
//
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/Xserver.h,v 1.3 2001/03/15 22:24:27 torrey Exp $ */

#import <Cocoa/Cocoa.h>

#include <drivers/event_status_driver.h>	// for NXEvent
#include <unistd.h>
#include <stdlib.h>

@interface Xserver : NSObject {
    // server state
    NSLock *serverLock;
    BOOL serverVisible;
    BOOL appQuitting;
    UInt32 mouseState;

    // communication
    int eventWriteFD;

    // Aqua interface
    IBOutlet NSPanel *helpWindow;
    IBOutlet id startupHelpButton;
}

- (id)init;

- (BOOL)translateEvent:(NSEvent *)anEvent;

- (void)getNXMouse:(NXEvent*)ev;
+ (void)append:(NSString*)value toEnv:(NSString*)name;

- (void)run;
- (void)startClients;
- (void)toggle;
- (void)show;
- (void)hide;
- (void)kill;
- (void)readPasteboard;
- (void)writePasteboard;
- (void)sendNXEvent:(NXEvent*)ev;
- (void)sendShowHide:(BOOL)show;

- (IBAction)closeHelpAndShow:(id)sender;

// NSApplication delegate
- (BOOL)applicationShouldTerminate:(NSApplication *)sender;
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification;
- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag;
- (void)applicationWillResignActive:(NSNotification *)aNotification;
- (void)applicationWillBecomeActive:(NSNotification *)aNotification;

@end

