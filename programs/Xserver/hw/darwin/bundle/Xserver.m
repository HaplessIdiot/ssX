//
//  Xserver.m
//
//  This class handles the interaction between the Cocoa front-end
//  and the Darwin X server thread.
//
//  Created by Andreas Monitzer on January 6, 2001.
//
/* $XFree86: xc/programs/Xserver/hw/darwin/bundle/Xserver.m,v 1.20 2001/07/15 01:57:35 torrey Exp $ */

#import "Xserver.h"
#import "Preferences.h"
#import "quartzShared.h"

#import "XWindow.h"

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
extern int main(int argc, char *argv[], char *envp[]);
extern void HideMenuBar(void);
extern void ShowMenuBar(void);

static NSPortMessage *signalMessage;

@implementation Xserver

- (id)init
{
    self=[super init];

    serverLock = [[NSLock alloc] init];
    clientTask = nil;
    serverRunning = NO;
    serverVisible = NO;
    rootlessMenuBarVisible = YES;
    appQuitting = NO;
    mouseState = 0;
    eventWriteFD = quartzEventWriteFD;

    // set up a port to safely send messages to main thread from server thread
    signalPort = [[NSPort port] retain];
    signalMessage = [[NSPortMessage alloc] initWithSendPort:signalPort
                    receivePort:signalPort components:nil];

    // set up receiving end
    [signalPort setDelegate:self];
    [[NSRunLoop currentRunLoop] addPort:signalPort
                                forMode:NSDefaultRunLoopMode];
    [[NSRunLoop currentRunLoop] addPort:signalPort
                                forMode:NSModalPanelRunLoopMode];

    return self;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    // Quit if the X server is not running
    if ([serverLock tryLock])
        return NSTerminateNow;

    if ([clientTask isRunning] || !quartzStartClients) {
        int but;

        [self hide];
        but = NSRunAlertPanel(NSLocalizedString(@"Quit X server?",@""),
                              NSLocalizedString(@"Quitting the X server will terminate any running X Window programs.",@""),
                              NSLocalizedString(@"Quit",@""),
                              NSLocalizedString(@"Cancel",@""),
                              nil);

        switch (but) {
            case NSAlertDefaultReturn:		// quit
                break;
            case NSAlertAlternateReturn:	// cancel
                return NSTerminateCancel;
        }
    }

    appQuitting = YES;
    [self killServer];

    // Wait until the X server shuts down
    return NSTerminateLater;
}

// returns YES when event was handled
- (BOOL)translateEvent:(NSEvent *)anEvent
{
    NXEvent ev;
    static BOOL mouse1Pressed = NO;
    unsigned int adjustedModifiers;

    if (!serverRunning)
        return NO;

    // Check for switch keypress
    // Ignore caps lock iff the swich key preference does not use caps lock.
    if ([Preferences modifiers] & NSAlphaShiftKeyMask) {
        adjustedModifiers = [anEvent modifierFlags];
    } else {
        adjustedModifiers = [anEvent modifierFlags] & ~NSAlphaShiftKeyMask;
    }
    if (([anEvent type] == NSKeyDown) && (![anEvent isARepeat]) &&
        ([anEvent keyCode] == [Preferences keyCode]) &&
        (adjustedModifiers == [Preferences modifiers]))
    {
        [self toggle];
        return YES;
    }

    if(!serverVisible && !quartzRootless)
        return NO;

    [self getNXMouse:&ev];
    ev.type=[anEvent type];
    ev.flags=[anEvent modifierFlags];
    switch(ev.type) {
        case NSLeftMouseUp:
            if (quartzRootless && !mouse1Pressed) {
                // MouseUp after MouseDown in menu - ignore
                return NO;
            }
            mouse1Pressed = NO;
            break;
        case NSLeftMouseDown:
            if (quartzRootless &&
                ! ([anEvent window] &&
                   [[anEvent window] isKindOfClass:[XWindow class]])) {
                // Click in non X window - ignore
                return NO;
            }
            mouse1Pressed = YES;
        case NSMouseMoved:
            break;
        case NSLeftMouseDragged:
        case NSRightMouseDragged:
        case 27:        // undocumented high button MouseDragged event
            ev.type=NSMouseMoved;
            break;
        case NSSystemDefined:
            if (![anEvent subtype]==7)
                return NO; // we only use multibutton mouse events
            if ([anEvent data1] & 1)
                return NO; // skip mouse button 1 events
            if (mouseState==[anEvent data2])
                return NO; // ignore double events
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
        case 25:        // undocumented MouseDown
        case 26:        // undocumented MouseUp
            // Hide these from AppKit to avoid its log messages
            return YES;
        default:
            return NO;
    }

    [self sendNXEvent:&ev];

    // Rootless: Send first NSLeftMouseDown to windows and views so window
    // ordering can be suppressed.
    // Don't pass further events - they (incorrectly?) bring the window
    // forward no matter what.
    if (quartzRootless  &&
        ([anEvent type]==NSLeftMouseDown || [anEvent type]==NSLeftMouseUp) &&
        [anEvent clickCount] == 1 &&
        [anEvent window] &&
        [[anEvent window] isKindOfClass:[XWindow class]])
    {
        return NO;
    }

    return YES;
}

// Fill in NXEvent with mouse coordinates, inverting y coordinate
- (void)getNXMouse:(NXEvent*)ev
{
    NSPoint pt=[NSEvent mouseLocation];

    ev->location.x=(int)(pt.x);
    ev->location.y=[[NSScreen mainScreen] frame].size.height - (int)(pt.y);
}

// Append a string to the given enviroment variable
+ (void)append:(NSString*)value toEnv:(NSString*)name {
    setenv([name cString],
        [[[NSString stringWithCString:getenv([name cString])]
            stringByAppendingString:value] cString],1);
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Block SIGPIPE
    // SIGPIPE repeatably killed the (rootless) server when closing a
    // dozen xterms in rapid succession. Those SIGPIPEs should have been
    // sent to the X server thread, which ignores them, but somehow they
    // ended up in this thread instead.
    {
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);
        // pthread_sigmask not implemented yet
        // pthread_sigmask(SIG_BLOCK, &set, NULL);
        sigprocmask(SIG_BLOCK, &set, NULL);
    }

    if (quartzRootless == -1) {
        // The display mode was not set from the command line.
        // Show mode pick panel?
        if ([Preferences modeWindow]) {
            [modeWindow makeKeyAndOrderFront:nil];
        } else {
            // Otherwise use default mode
            quartzRootless = [Preferences rootless];
            [self startX];
        }
    } else {
        [self startX];
    }
}

// Start the X server thread and the client process
- (void)startX
{
    [modeWindow close];

    // Start the X server thread
    [NSThread detachNewThreadSelector:@selector(run) toTarget:self
              withObject:nil];
    serverRunning = YES;

    // Start the X clients if started from GUI
    if (quartzStartClients) {
        char *home;
        char xinitrcbuf[PATH_MAX];
        NSString *path = [NSString stringWithCString:XPATH(xinit)];
        NSString *server = [NSString stringWithCString:XPATH(XDarwinStartup)];
        NSString *client, *displayName;
        BOOL hasClient = YES;
        NSArray *args;

        // Register to receive notification when the client task finishes
        [[NSNotificationCenter defaultCenter] addObserver:self
                selector:@selector(clientTaskDone:)
                name:NSTaskDidTerminateNotification
                object:nil];

        // Change to user's home directory (so xterms etc. start there)
        home = getenv("HOME");
        if (home)
            chdir(home);
        else
            home = "";

        // Add X binary directory to path
        [Xserver append:@":" toEnv:@"PATH"];
        [Xserver append:@XSTRPATH(XBINDIR) toEnv:@"PATH"];

        displayName = [NSString localizedStringWithFormat:@":%d",
                                [Preferences display]];

        // Find the client init file to use
        snprintf(xinitrcbuf, PATH_MAX, "%s/.xinitrc", home);
        if (access(xinitrcbuf, F_OK)) {
            snprintf(xinitrcbuf, PATH_MAX, XSTRPATH(XINITDIR) "/xinitrc");
            if (access(xinitrcbuf, F_OK)) {
                hasClient = NO;
            }
        }
        if (hasClient) {
            client = [NSString stringWithCString:xinitrcbuf];
            args = [NSArray arrayWithObjects:client, @"--", server,
                            displayName, @"-idle", nil];
        } else {
            args = [NSArray arrayWithObjects:@"--", server, displayName,
                            @"-idle", nil];
        }

        // Launch a new task to run start X clients
        clientTask = [NSTask launchedTaskWithLaunchPath:path arguments:args];
    }

    if (quartzRootless) {
        // There is no help window for rootless; just start
        [helpWindow close];
        [self sendShowHide:YES];
    } else {
        // Show the X switch window if not using dock icon switching
        if (![Preferences dockSwitch])
            [switchWindow orderFront:nil];

        if ([Preferences startupHelp]) {
            // display the full screen mode help
            [self sendShowHide:NO];
            [helpWindow makeKeyAndOrderFront:nil];
        } else {
            // start running full screen and make sure X is visible
            ShowMenuBar();
            [self closeHelpAndShow:nil];
        }
    }
}

// Run the X server thread
- (void)run
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    [serverLock lock];
    main(argcGlobal, argvGlobal, envpGlobal);
    serverVisible = NO;
    [serverLock unlock];
    [pool release];
    QuartzMessageMainThread(kQuartzServerDied);
}

// Full screen mode was picked in the mode pick panel
- (IBAction)startFullScreen:(id)sender
{
    [Preferences setModeWindow:[startupModeButton intValue]];
    [Preferences saveToDisk];
    quartzRootless = FALSE;
    [self startX];
}

// Rootless mode was picked in the mode pick panel
- (IBAction)startRootless:(id)sender
{
    [Preferences setModeWindow:[startupModeButton intValue]];
    [Preferences saveToDisk];
    quartzRootless = TRUE;
    [self startX];
}

// Close the help splash screen and show the X server
- (IBAction)closeHelpAndShow:(id)sender
{
    if (sender) {
        int helpVal = [startupHelpButton intValue];
        [Preferences setStartupHelp:helpVal];
        [Preferences saveToDisk];
    }
    [helpWindow close];

    serverVisible = YES;
    [self sendShowHide:YES];
    [NSApp activateIgnoringOtherApps:YES];
}

// Show the X server when sent message from GUI
- (IBAction)showAction:(id)sender
{
    if (serverRunning)
        [self sendShowHide:YES];
}

// Show or hide the X server or menu bar in rootless mode
- (void)toggle
{
    if (quartzRootless) {
        if (rootlessMenuBarVisible)
            HideMenuBar();
        else
            ShowMenuBar();
        rootlessMenuBarVisible = !rootlessMenuBarVisible;
    } else {
        if (serverVisible)
            [self hide];
        else
            [self show];
    }
}

// Show the X server on screen
- (void)show
{
    if (!serverVisible && serverRunning) {
        [self sendShowHide:YES];
    }
}

// Hide the X server from the screen
- (void)hide
{
    if (serverVisible && serverRunning) {
        [self sendShowHide:NO];
    }
}

// Kill the Xserver thread
- (void)killServer
{
    NXEvent ev;

    if (serverVisible)
        [self hide];

    ev.type = NX_APPDEFINED;
    ev.data.compound.subType = kXDarwinQuit;
    [self sendNXEvent:&ev];
}

// Tell the X server to show or hide itself.
// This ignores the current X server visible state.
- (void)sendShowHide:(BOOL)show
{
    NXEvent ev;

    [self getNXMouse:&ev];
    ev.type = NX_APPDEFINED;

    if (show) {
        QuartzCapture();
        if (!quartzRootless)
            HideMenuBar();
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
        if (!quartzRootless)
            ShowMenuBar();
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

- (void)sendNXEvent:(NXEvent*)ev
{
    int bytesWritten;

    if (quartzRootless  &&
        (ev->type == NSLeftMouseDown  ||  ev->type == NSLeftMouseUp  ||
        (ev->type == NSSystemDefined && ev->data.compound.subType == 7)))
    {
        // mouse button event - send mouseMoved to this position too
        // X gets confused if it gets a click that isn't at the last
        // reported mouse position.
        NXEvent moveEvent = *ev;
        moveEvent.type = NSMouseMoved;
        [self sendNXEvent:&moveEvent];
    }

    bytesWritten = write(eventWriteFD, ev, sizeof(*ev));
    if (bytesWritten == sizeof(*ev))
        return;
    NSLog(@"Bad write to event pipe.");
    // FIXME: handle bad writes better?
}

// Handle messages from the X server thread
- (void)handlePortMessage:(NSPortMessage *)portMessage
{
    unsigned msg = [portMessage msgid];

    switch(msg) {
        case kQuartzServerHidden:
            // FIXME: This hack is necessary (but not completely effective)
            // since Mac OS X 10.0.2
            [NSCursor unhide];
            break;
        case kQuartzServerDied:
            serverRunning = NO;
            if (appQuitting) {
                // If we quit before the clients start, they may sit and wait
                // for the X server to start. Kill them instead.
                if ([clientTask isRunning])
                    [clientTask terminate];
                [NSApp replyToApplicationShouldTerminate:YES];
            } else {
                [NSApp terminate:nil];	// quit if we aren't already
            }
            break;
        default:
            NSLog(@"Unknown message from server thread.");
    }
}

// Quit the X server when the X client task finishes
- (void)clientTaskDone:(NSNotification *)aNotification
{
    // Make sure it was the client task that finished
    if (![clientTask isRunning]) {
        int status = [[aNotification object] terminationStatus];

        if (status != 0)
            NSLog(@"X client task terminated abnormally.");

        if (!appQuitting)
            [NSApp terminate:nil];	// quit if we aren't already
    }
}

// Called when the user clicks the application icon,
// but not when Cmd-Tab is used.
// Rootless: Don't switch until applicationWillBecomeActive.
- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication
            hasVisibleWindows:(BOOL)flag
{
    if ([Preferences dockSwitch] && !quartzRootless) {
        [self show];
    }
    return NO;
}

- (void)applicationWillResignActive:(NSNotification *)aNotification
{
    [self hide];
}

- (void)applicationWillBecomeActive:(NSNotification *)aNotification
{
    if (quartzRootless)
        [self show];
}

@end

// Send a message to the main thread, which calls handlePortMessage in
// response. Must only be called from the X server thread because
// NSPort is not thread safe.
void QuartzMessageMainThread(unsigned msg)
{
    [signalMessage setMsgid:msg];
    [signalMessage sendBeforeDate:[NSDate distantPast]];
}
