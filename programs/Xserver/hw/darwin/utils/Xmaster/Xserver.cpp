/*
 *  file: Xserver.cpp
 *  project: Xmaster
 *
 * Greg Parker (gparker@cs.stanford.edu) November 2000
 * This code is hereby released into the public domain.
 *
 */

#include <Carbon/Carbon.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

// for NXEvent
#include <drivers/event_status_driver.h>
#include "Keymapper.h"

#include "Xserver.h"

// Store the current mouse position in an NXEvent
static void GetNXMouse(NXEvent &ev) {
    Point qdpt;

    GetGlobalMouse(&qdpt);
    ev.location.x = qdpt.h;
    ev.location.y = qdpt.v;
}


// display must be valid and *not* have a prepended ':'
XServer::XServer(char *display) 
{
    if (display) {
        strncpy(mDisplay, display, MAX_DISPLAY);
        mDisplay[MAX_DISPLAY-1] = '\0';
    } else {
        strcpy(mDisplay, DEFAULT_DISPLAY);
    }
    
    mRunning = false;
    mVisible = false;
    mModifiers = 0;
    mEventWriteFD = -1;
    mOutputReadFile = NULL;
}


// Destructor does not terminate the X server process
XServer::~XServer() 
{
    if (mVisible) Hide();
    if (mEventWriteFD >= 0) close(mEventWriteFD);
    if (mOutputReadFile) pclose(mOutputReadFile);
}


// Append a string to the given enviroment variable
void appendenv(const char *name, const char *value) {
    char *oldvalue;
    char *newvalue;
    size_t len;
    
    // Append /usr/X11R6/bin to the path.
    oldvalue = getenv(name);
    len = oldvalue  ?  1+strlen(value)+strlen(oldvalue)  :  0;
    newvalue = (char *)malloc(len);
    if (newvalue) {
        newvalue[0] = '\0';
        if (oldvalue) {
            strcat(newvalue, oldvalue);
        }
        strcat(newvalue, value);
        setenv(name, newvalue, 1);
        free(newvalue);
    }
}


// Start an X server or find an existing one.
// Return false if there's no X server running afterwards.
Boolean XServer::Run() 
{
    if (mRunning) return true;
    
    char *home;
    char buffer[100+MAX_DISPLAY];
    char fifoName[100+MAX_DISPLAY];
    int i;
    
    // from Xserver/os/osutils.h through darwin.c
    #define LOCK_DIR "/tmp"
    #define LOCK_PREFIX "/.X"
    #define FIFO_SUFFIX "-fifo"
    
    sprintf(fifoName, LOCK_DIR LOCK_PREFIX "%s" FIFO_SUFFIX, mDisplay);
    
    // Change to user's home directory (so xterms etc. start there)
    home = getenv("HOME");
    if (home) chdir(home);
    
    // Add X binary directory to the PATH
    appendenv("PATH", ":/usr/X11R6/bin");
    
    // Is an X server already running? Try to open the fifo.
    // nonblock so we don't open stale fifo with no reader
    mEventWriteFD = open(fifoName, O_WRONLY | O_NONBLOCK, 0);
    if (mEventWriteFD >= 0) {
        printf("Xmaster: connecting to already-open Xserver on display :%s\n", 
                mDisplay);
    }
    else {
        // Run a new X server
        sprintf(buffer, "/usr/X11R6/bin/startx -- /usr/X11R6/bin/Xdarwin "
                " -quartz -fakebuttons "
                " :%s ", mDisplay);
                
        // popen: allows Xmaster to see X server's stdout and stderr
        // system: X server output goes to debug console
        // strcat(buffer, " 2>&1"); darwinReadFile = popen(buffer, "r");
        strcat(buffer, " &"); system(buffer);
        
        for (i = 0; i < 10; i++) {
            sleep(1);
            mEventWriteFD = open(fifoName, O_WRONLY | O_NONBLOCK, 0);
            if (mEventWriteFD < 0) {
                printf("Xmaster: couldn't open fifo '%s' "
                        "(errno=%d, attempt %d of %d)\n", 
                        fifoName, errno, i, 10);
            } else {
                break;
            }
        }
        if (mEventWriteFD < 0) {
            printf("Xmaster: giving up\n");
            printf("Xmaster: No X \n");
            return false;
        }
    }
    fcntl(mEventWriteFD, F_SETFL, 0); // clear O_NONBLOCK
    printf("Xmaster: xserver started, Xmasterfifo open\n");
    mRunning = true;
    mVisible = true;
    ClearModifiers();
    Hide(); // also updates modifiers
    return true;
}


// Show or hide the X server
void XServer::Toggle() 
{
    if (mVisible) Hide(); else Show();
}


// Show the X server on screen
void XServer::Show()
{
    if (mRunning && !mVisible) {
        SendShowHide(true);
        UpdateModifiers();
    }
}


// Hide the X server from the screen
void XServer::Hide()
{
    if (mRunning && mVisible) {
        SendShowHide(false);
        UpdateModifiers();
    }
}


// Kill the Xserver process
void XServer::Kill()
{
    if (! mRunning) return;
    if (mVisible) Hide();

    NXEvent ev;
    ev.type = NX_APPDEFINED;
    ev.data.compound.subType = XServer::kQuit;
    SendEvent(ev);
    mRunning = false;
}


void XServer::UpdateModifiers()
{
    UpdateModifiers(GetCurrentKeyModifiers());
}


// Send modifier key updates to the X server.
void XServer::UpdateModifiers(UInt32 newModifiers)
{
    UInt32 new_on_flags;
    UInt32 new_off_flags;
    NXEvent ev;
    int i;
    UInt32 modlist[] = {
        cmdKey, alphaLock, shiftKey, optionKey, controlKey, 
        rightShiftKey, rightOptionKey, rightControlKey 
    };
    
    new_on_flags = newModifiers & ~mModifiers;
    new_off_flags = ~newModifiers & mModifiers;
    
    ev.type = NX_FLAGSCHANGED;
    ev.flags = Keymapper::TranslateModifiers(newModifiers);
    GetNXMouse(ev);
    mModifiers = newModifiers;

    #warning fixme make sure optionKey is *not* set when rightOptionKey is pressed
    #warning fixme right*Key doesnt appear to be set ever
    #warning fixme multi-modifier change might not actually work 

    for (i = 0; i < 8; i++) {
        if (new_on_flags & modlist[i] || new_off_flags & modlist[i]) {
            ev.data.key.keyCode = Keymapper::ModifierKeycode(modlist[i]);
            SendEvent(ev);
        }
    }
}


// Send a key event to the X server.
// keyCode and modifiers are Carbon.
OSStatus XServer::SendKey(EventKind kind, UInt32 keyCode, UInt32 modifiers)
{
    if (! mRunning) return eventNotHandledErr;
    
    NXEvent ev;
    GetNXMouse(ev);
    ev.flags = Keymapper::TranslateModifiers(modifiers);
    ev.data.key.keyCode = Keymapper::TranslateKeycode(keyCode);

    switch (kind) {
        case kEventRawKeyDown:
            ev.type = NX_KEYDOWN;
            ev.data.key.repeat = false;
            break;
        case kEventRawKeyUp:
            ev.type = NX_KEYUP;
            break;
        case kEventRawKeyRepeat:
            ev.type = NX_KEYDOWN;
            ev.data.key.repeat = true;
            break;
        default:
            return eventNotHandledErr;
    }
    
    SendEvent(ev);
    return noErr;
}


// Send a mouse event to the X server.
OSStatus XServer::SendMouseEvent(EventRef event)
{
    if (! mRunning) return eventNotHandledErr;
    
    OSStatus result;
    NXEvent ev;
    UInt32 scrollAxis;
    Point mousePos;
    UInt32 modifiers;

    result = GetEventParameter(event, kEventParamMouseLocation, 
                            typeQDPoint, NULL,      // type
                            sizeof(Point), NULL,  // size
                            &mousePos);
    if (result) return result;
    ev.location.x = mousePos.h;
    ev.location.y = mousePos.v;

    result = GetEventParameter(event, kEventParamKeyModifiers, 
                               typeUInt32, NULL, sizeof(UInt32),
                               NULL, &modifiers);
    if (result) return result;
    ev.flags = Keymapper::TranslateModifiers(modifiers);

    switch (GetEventKind(event)) {
        case kEventMouseDown:
            #warning fixme choose correct button for mousedown event
            ev.type = NX_LMOUSEDOWN;
            break;
            
        case kEventMouseUp:
            #warning fixme choose correct button for mouseup event
            ev.type = NX_LMOUSEUP;
            break;
            
        case kEventMouseMoved:
        case kEventMouseDragged:
            ev.type = NX_MOUSEMOVED;
            break;
            
        case kEventMouseWheelMoved:
            ev.type = NX_SCROLLWHEELMOVED;
            // Xserver only understands vertical axis
            result = GetEventParameter(event, kEventParamMouseWheelAxis, 
                                        typeMouseWheelAxis, NULL, 
                                        sizeof(UInt32), NULL, &scrollAxis);
            if (result) return result;
            if (scrollAxis == kEventMouseWheelAxisY) return noErr; 
            // Xserver doesn't do horiz wheel
            result = GetEventParameter(event, kEventParamMouseWheelDelta, 
                                       typeLongInteger, NULL,
                                       sizeof(UInt32), NULL, 
                                       &ev.data.scrollWheel.deltaAxis1);
            if (result) return result;
            break;

        default:
            return eventNotHandledErr;
            break;
    }

    SendEvent(ev);
    return noErr;
}

// Tell the X server to show or hide itself.
// This ignores the current X server visible state.
void XServer::SendShowHide(Boolean show)
{
    if (! mRunning) return;
    
    NXEvent ev;
    GetNXMouse(ev);
    
    if (show) {
        ev.data.compound.subType = XServer::kShow;
        HideMenuBar();
    } else {
        ev.data.compound.subType = XServer::kHide;
        ShowMenuBar();
    }

    ev.type = NX_APPDEFINED;    
    SendEvent(ev);
    mVisible = show;
}

void XServer::SendEvent(NXEvent ev)
{
    write(mEventWriteFD, &ev, sizeof(ev));
    // fixme handle failure
}


// Tell the X server to release any pressed modifier keys.
void XServer::ClearModifiers() 
{
    if (! mRunning) return;

    NXEvent ev;
    ev.type = NX_APPDEFINED;
    ev.data.compound.subType = XServer::kClearModifiers;
    SendEvent(ev);
}
