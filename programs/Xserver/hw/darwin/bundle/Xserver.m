//
//  Xserver.m
//
//  This class handles the interaction between the Cocoa front-end
//  and the Darwin X server thread.
//
//  Created by Andreas Monitzer on January 6, 2001.
//
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/Xserver.m,v 1.7 2001/04/05 06:08:46 torrey Exp $ */

#import "Xserver.h"
#import "Preferences.h"
#import "quartzShared.h"

// Macros to build the path name
#ifndef XBINDIR
#define XBINDIR /usr/X11R6/bin
#endif
#define STR(s) #s
#define XSTRPATH(s) STR(s)
#define XPATH(file) XSTRPATH(XBINDIR) "/" STR(file)

extern int argcGlobal;
extern char **argvGlobal;
extern char **envpGlobal;

@implementation Xserver

- (id)init {
    self=[super init];

    serverLock = [[NSLock alloc] init];
    serverVisible = NO;
    appQuitting = NO;
    mouseState = 0;
    eventWriteFD = quartzEventWriteFD;

    return self;
}

- (BOOL)applicationShouldTerminate:(NSApplication *)sender {
    int but;

    if ([serverLock tryLock])
        return YES;
    if (serverVisible)
        [self hide];

    but = NSRunAlertPanel(NSLocalizedString(@"Quit X server?",@""),
                          NSLocalizedString(@"Quitting the X server will terminate any running X Window programs.",@""),
                          NSLocalizedString(@"Quit",@""),
                          NSLocalizedString(@"Cancel",@""),
                          nil);

    switch (but) {
        case NSAlertDefaultReturn:		// quit
            appQuitting = YES;
            [self kill];
            // Try to wait until the X server shuts down
            [serverLock lockBeforeDate:[NSDate dateWithTimeIntervalSinceNow:10]];
            return YES;
        case NSAlertAlternateReturn:		// cancel
            break;
    }
    return NO;
}

// returns YES when event was handled
- (BOOL)translateEvent:(NSEvent *)anEvent {
    NXEvent ev;

    if(([anEvent type]==NSKeyDown) && (![anEvent isARepeat]) &&
       ([anEvent keyCode]==[Preferences keyCode]) &&
       ([anEvent modifierFlags]==[Preferences modifiers])) {
        [self toggle];
        return YES;
    }

    if(!serverVisible)
        return NO;

    [self getNXMouse:&ev];
    ev.type=[anEvent type];
    ev.flags=[anEvent modifierFlags];
    switch(ev.type) {
        case NSLeftMouseDown:
        case NSLeftMouseUp:
        case NSMouseMoved:
            break;
        case NSLeftMouseDragged:
        case NSRightMouseDragged:
            ev.type=NSMouseMoved;
            break;
        case NSSystemDefined:
            if(([anEvent subtype]==7) && ([anEvent data1] & 1))
                return YES; // skip mouse button 1 events
            if(mouseState==[anEvent data2])
                return YES; // ignore double events
            ev.data.compound.subType=[anEvent subtype];
            ev.data.compound.misc.L[0]=[anEvent data1];
            ev.data.compound.misc.L[1]=mouseState=[anEvent data2];
            break;
        case NSScrollWheel:
            ev.data.scrollWheel.deltaAxis1=[anEvent deltaY];
            break;
        case NSKeyDown:
        case NSKeyUp:
            ev.data.key.keyCode = [anEvent keyCode];
            ev.data.key.repeat = [anEvent isARepeat];
            break;
        case NSFlagsChanged:
            ev.data.key.keyCode = [anEvent keyCode];
            break;
        default:
            return YES;
    }

    [self sendNXEvent:&ev];

    return YES;
}

- (void)getNXMouse:(NXEvent*)ev {
    NSPoint pt=[NSEvent mouseLocation];
    ev->location.x=(int)(pt.x);
    ev->location.y=[[NSScreen mainScreen] frame].size.height-(int)(pt.y); // invert mouse
}

// Append a string to the given enviroment variable
+ (void)append:(NSString*)value toEnv:(NSString*)name {
    setenv([name cString],
        [[[NSString stringWithCString:getenv([name cString])]
            stringByAppendingString:value] cString],1);
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    // Start the X server thread
    [NSThread detachNewThreadSelector:@selector(run) toTarget:self withObject:nil];

    // Start the X clients if started from GUI
    if (quartzStartClients)
        [NSThread detachNewThreadSelector:@selector(startClients) toTarget:self withObject:nil];

    // Make sure the menu bar gets drawn
    [NSApp setWindowsNeedUpdate:YES];

    // Display the help splash screen or show the X server
    if ([Preferences startupHelp]) {
        [self sendShowHide:NO];
        [helpWindow makeKeyAndOrderFront:self];
    } else {
        [self closeHelpAndShow:self];
    }
}

// Run the X server thread
- (void)run {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    [serverLock lock];
    main(argcGlobal, argvGlobal, envpGlobal);
    serverVisible = NO;
    [serverLock unlock];
    [pool release];
    if (!appQuitting)
        [NSApp terminate:nil];	// quit if we aren't already
}

// Start the X clients in a separate thread
- (void)startClients {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    char *home;

    // Change to user's home directory (so xterms etc. start there)
    home = getenv("HOME");
    if (home) chdir(home);

    [Xserver append:@":" toEnv:@"PATH"];
    [Xserver append:@XSTRPATH(XBINDIR) toEnv:@"PATH"];
    system(XPATH(startx -- -idle &));
    // FIXME: quit when startx dies
    [pool release];
}

// Close the help splash screen and show the X server
- (IBAction)closeHelpAndShow:(id)sender {
    int helpVal;

    helpVal = [startupHelpButton intValue];
    [Preferences setStartupHelp:helpVal];
    [Preferences saveToDisk];

    [helpWindow close];

    serverVisible = YES;
    [self sendShowHide:YES];
    [NSApp activateIgnoringOtherApps:YES];
}

// Show or hide the X server
- (void)toggle {
    if (serverVisible)
        [self hide];
    else
        [self show];
}

// Show the X server on screen
- (void)show {
    if (!serverVisible) {
        [self sendShowHide:YES];
    }
}

// Hide the X server from the screen
- (void)hide {
    if (serverVisible) {
        [self sendShowHide:NO];
    }
}

// Kill the Xserver process
- (void)kill {
    NXEvent ev;

    if (serverVisible)
        [self hide];

    ev.type = NX_APPDEFINED;
    ev.data.compound.subType = kXDarwinQuit;
    [self sendNXEvent:&ev];
}

// Tell the X server to show or hide itself.
// This ignores the current X server visible state.
- (void)sendShowHide:(BOOL)show {
    NXEvent ev;

    [self getNXMouse:&ev];
    ev.type = NX_APPDEFINED;

    if (show) {
        ev.data.compound.subType = kXDarwinShow;
        [self sendNXEvent:&ev];

        // inform the X server of the current modifier state
        ev.flags = [[NSApp currentEvent] modifierFlags];
        ev.data.compound.subType = kXDarwinUpdateModifiers;
        [self sendNXEvent:&ev];

        // put the pasteboard into the X cut buffer
        [self readPasteboard];
    } else {
        // put the X cut buffer on the pasteboard
        [self writePasteboard];

        ev.data.compound.subType = kXDarwinHide;
        [self sendNXEvent:&ev];
    }

    serverVisible = show;
}

// Tell the X server to read from the pasteboard into the X cut buffer
- (void)readPasteboard 
{
    NXEvent ev;

    ev.type = NX_APPDEFINED;
    ev.data.compound.subType = kXDarwinReadPasteboard;
    [self sendNXEvent:&ev];
}

// Tell the X server to write the X cut buffer into the pasteboard
- (void)writePasteboard 
{
    NXEvent ev;

    ev.type = NX_APPDEFINED;
    ev.data.compound.subType = kXDarwinWritePasteboard;
    [self sendNXEvent:&ev];
}

- (void)sendNXEvent:(NXEvent*)ev {
    if (write(eventWriteFD, ev, sizeof(*ev)) == sizeof(*ev))
        return;
    ErrorF("Bad write to event pipe.\n");
    // FIXME: handle bad writes better?
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag {
    [self show];
    return NO;
}

- (void)applicationWillResignActive:(NSNotification *)aNotification {
    [self hide];
}

- (void)applicationWillBecomeActive:(NSNotification *)aNotification {
    [self readPasteboard];
}

@end
