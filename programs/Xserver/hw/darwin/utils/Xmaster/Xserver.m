//
//  Xserver.m
//  Xmaster-Cocoa
//
//  Created by am on Sat Jan 06 2001.
//

#import "Xserver.h"

#import "Preferences.h"

@implementation Xserver

- (id)init {
    self=[super init];
    mDisplay=[[NSString alloc] initWithFormat:@"%d",[Preferences display],nil];

    mRunning = NO;
    mVisible = NO;
    mModifiers = 0;
    mEventWriteFD = -1;
    mOutputReadFile = NULL;
    mouseState = 0;
    
    return self;
}

- (BOOL)applicationShouldTerminate:(NSApplication *)sender {
    int but;
    
    if(!killed) {
        but=NSRunAlertPanel(@"Quit X-server?", @"If you leave it running, you can switch back to it by restarting Xmaster.", @"Quit", @"Cancel", @"Leave running");
    
        switch(but) {
        case NSAlertDefaultReturn:
            [self kill];
            if (mEventWriteFD >= 0) close(mEventWriteFD);
            if (mOutputReadFile) pclose(mOutputReadFile);
            [mDisplay release];
            return YES;
        case NSAlertOtherReturn:
            if (mVisible) [self hide];
            if (mEventWriteFD >= 0) close(mEventWriteFD);
            if (mOutputReadFile) pclose(mOutputReadFile);
            [mDisplay release];
            return YES;
        case NSAlertAlternateReturn:
            break;
        }
        return NO;
    } else
        return YES;
}

// returns YES when event was handled
- (BOOL)translateEvent:(NSEvent *)anEvent {
    NXEvent ev;

    if(!mVisible)
        return NO;
    
    if(([anEvent type]==NSKeyDown)&&(![anEvent isARepeat])&&([anEvent keyCode]==[Preferences keyCode])&&([anEvent modifierFlags]==[Preferences modifiers])) {
        [self toggle];
        return YES;
    }
    
    [self getNXMouse:&ev];
    ev.type=[anEvent type];
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
        if(([anEvent subtype]==7)&&([anEvent data1] & 1 == 1))
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
        ev.flags=[anEvent modifierFlags];
        break;
    case NSFlagsChanged:
        ev.data.key.keyCode = [anEvent keyCode];
        ev.flags=[anEvent modifierFlags];
        break;
    default:
        return YES;
    }
    
    [self sendEvent:&ev];
    
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
    char *home;
    char buffer[100+MAX_DISPLAY];
    char fifoName[100+MAX_DISPLAY];
    int i;

    if (mRunning) return;
    
    // from Xserver/os/osutils.h through darwin.c
    #define LOCK_DIR "/tmp"
    #define LOCK_PREFIX "/.X"
    #define FIFO_SUFFIX "-fifo"
    
    sprintf(fifoName, LOCK_DIR LOCK_PREFIX "%s" FIFO_SUFFIX, [mDisplay cString]);
    
    // Change to user's home directory (so xterms etc. start there)
    home = getenv("HOME");
    if (home) chdir(home);
    
    // Add X binary directory to the PATH
    [Xserver append:@":/usr/X11R6/bin" toEnv:@"PATH"];
    
    // Is an X server already running? Try to open the fifo.
    // nonblock so we don't open stale fifo with no reader
    mEventWriteFD = open(fifoName, O_WRONLY | O_NONBLOCK, 0);
    if (mEventWriteFD >= 0) {
        NSLog(@"Xmaster: connecting to already-open Xserver on display :%@", 
                mDisplay);
    }
    else {
        // Run a new X server
        if([Preferences fakeButtons])
            sprintf(buffer, "/usr/X11R6/bin/startx -- /usr/X11R6/bin/Xdarwin "
                    " -quartz -fakebuttons "
                    " :%s ", [mDisplay cString]);
        else
            sprintf(buffer, "/usr/X11R6/bin/startx -- /usr/X11R6/bin/Xdarwin "
                    " -quartz"
                    " :%s ", [mDisplay cString]);
                
        // popen: allows Xmaster to see X server's stdout and stderr
        // system: X server output goes to debug console
        // strcat(buffer, " 2>&1"); darwinReadFile = popen(buffer, "r");
        strcat(buffer, " &"); system(buffer);
        
        for (i = 0; i < 10; i++) {
            sleep(1);
            mEventWriteFD = open(fifoName, O_WRONLY | O_NONBLOCK, 0);
            if (mEventWriteFD < 0) {
                NSLog(@"Xmaster: couldn't open fifo '%s' (errno=%d, attempt %d of %d)", 
                        fifoName, errno, i, 10);
            } else {
                break;
            }
        }
        if (mEventWriteFD < 0) {
            NSLog(@"Xmaster: giving up");
            NSLog(@"Xmaster: No X");
            return;
        }
    }
    fcntl(mEventWriteFD, F_SETFL, 0); // clear O_NONBLOCK
    NSLog(@"Xmaster: xserver started, Xmasterfifo open");
    mRunning = YES;
    mVisible = YES;
    [self sendShowHide:YES];
    [NSApp activateIgnoringOtherApps:YES];
}

// Show or hide the X server
- (void)toggle {
    if (mVisible)
        [self hide];
    else
        [self show];
}

// Show the X server on screen
- (void)show {
    if (mRunning && !mVisible) {
        [self sendShowHide:YES];
    }
}

// Hide the X server from the screen
- (void)hide {
    if (mRunning && mVisible) {
        [self sendShowHide:NO];
    }
}

// Kill the Xserver process
- (void)kill {
    NXEvent ev;
    if (! mRunning) return;
    if (mVisible) [self hide];

    ev.type = NX_APPDEFINED;
    ev.data.compound.subType = kQuit;
    [self sendEvent:&ev];
    mRunning = NO;
}

// Tell the X server to show or hide itself.
// This ignores the current X server visible state.
- (void)sendShowHide:(BOOL)show {
    NXEvent ev;
    if (! mRunning) return;
    
    [self getNXMouse:&ev];
    
    if (show) {
        ev.data.compound.subType = kShow;
        HideMenuBar();
    } else {
        ev.data.compound.subType = kHide;
        ShowMenuBar();
    }

    ev.type = NX_APPDEFINED;    
    [self sendEvent:&ev];
    mVisible = show;
}

- (void)sendEvent:(NXEvent*)ev {
    write(mEventWriteFD, ev, sizeof(*ev));
    // fixme handle failure
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)theApplication hasVisibleWindows:(BOOL)flag {
    [self show];
    return NO;
}

- (void)applicationWillResignActive:(NSNotification *)aNotification { // never called?
    [self hide];
}

@end
