//
//  Xserver.h
//
//  Created by Andreas Monitzer on January 6, 2001.
//

#import <Cocoa/Cocoa.h>

#include <drivers/event_status_driver.h>	// for NXEvent
#include <unistd.h>

@interface Xserver : NSObject {
    // server state
    BOOL serverDied;
    BOOL serverVisible;
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
- (void)toggle;
- (void)show;
- (void)hide;
- (void)kill;
- (void)sendNXEvent:(NXEvent*)ev;
- (void)sendShowHide:(BOOL)show;

- (IBAction)closeHelpAndShow:(id)sender;

// NSApplication delegate
- (BOOL)applicationShouldTerminate:(NSApplication *)sender;
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification;
- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag;
- (void)applicationWillResignActive:(NSNotification *)aNotification;

@end

