//
//  Xserver.h
//  Xmaster-Cocoa
//
//  Created by am on Sat Jan 06 2001.
//

#import <Cocoa/Cocoa.h>

#include <Carbon/Carbon.h> // for Show/HideMenuBar()

// for NXEvent
#include <drivers/event_status_driver.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define hideKeycode 0
#define hideModifiers (NSCommandKeyMask | NSAlternateKeyMask)
#define DEFAULT_DISPLAY @"0"
#define MAX_DISPLAY 100

extern BOOL killed;

@interface Xserver : NSObject {
    // server state
    BOOL mRunning;
    BOOL mVisible;
    UInt32 mModifiers;

    // communication
    int mEventWriteFD;
    FILE *mOutputReadFile;

    // settings
    NSString *mDisplay;
}

- (id)init;

- (BOOL)translateEvent:(NSEvent *)anEvent;

- (void)getNXMouse:(NXEvent*)ev;
+ (void)append:(NSString*)value toEnv:(NSString*)name;

- (void)toggle;
- (void)show;
- (void)hide;
- (void)kill;
- (void)sendEvent:(NXEvent*)ev;
- (void)sendShowHide:(BOOL)show;

//NSApplication delegate
- (BOOL)applicationShouldTerminate:(NSApplication *)sender;
- (void)applicationDidFinishLaunching:(NSNotification *)aNotification;
- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag;
- (void)applicationWillResignActive:(NSNotification *)aNotification;

@end

// NX_APPDEFINED event subtypes for special commands to the X server
// clear modifiers: release all pressed modifier keys
// show: vt switch to X server; recapture screen and restore X drawing
// hide: vt switch away from X server; release screen and clip X drawing
// quit: kill the X server and release the display
    enum {
        kClearModifiers,
        kShow,
        kHide,
        kQuit
    };
