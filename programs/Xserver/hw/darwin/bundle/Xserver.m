//
//  Xserver.m
//
//  This class handles the interaction between the Cocoa front-end
//  and the Darwin X server thread.
//
//  Created by Andreas Monitzer on January 6, 2001.
//

#import "Xserver.h"
#import "Preferences.h"
#import "../quartzShared.h"

extern int argcGlobal;
extern char **argvGlobal;
extern char **envpGlobal;

@implementation Xserver

- (id)init {
    self=[super init];

    serverDied = NO;
    serverVisible = NO;
    mouseState = 0;
    eventWriteFD = gDarwinEventWriteFD;

    return self;
}

- (BOOL)applicationShouldTerminate:(NSApplication *)sender {
    int but;

    if (serverVisible) {
        [self hide];
    }

    if (serverDied) {
        return YES;
    } else {
        but = NSRunAlertPanel(@"Quit X server?", 
                              @"Quitting the X server will terminate any running X Window programs.", 
                              @"Quit", @"Cancel", nil);

        switch (but) {
          case NSAlertDefaultReturn:		// quit
            [self kill];
            if (eventWriteFD >= 0) close(eventWriteFD);
            return YES;
          case NSAlertAlternateReturn:		// cancel
            break;
        }
        return NO;
    }
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
    main(argcGlobal, argvGlobal, envpGlobal);
    serverDied = YES;
    [NSApp terminate:nil];
}

// Close the help splash screen and show the X server
- (IBAction)closeHelpAndShow:(id)sender {
    int helpVal;

    helpVal = [startupHelpButton intValue];
    [Preferences setStartupHelp:helpVal];
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

    } else {
        ev.data.compound.subType = kXDarwinHide;
        [self sendNXEvent:&ev];
    }

    serverVisible = show;
}

- (void)sendNXEvent:(NXEvent*)ev {
    if (write(eventWriteFD, ev, sizeof(*ev)) == sizeof(*ev))
        return;
    // FIXME: handle bad writes?
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag {
    [self show];
    return NO;
}

- (void)applicationWillResignActive:(NSNotification *)aNotification {
    [self hide];
}

@end
