/*
 *  file: Xserver.h
 *  project: Xmaster
 *
 * Greg Parker (gparker@cs.stanford.edu) November 2000
 * This code is hereby released into the public domain.
 *
 */

#ifndef XSERVER_H
#define XSERVER_H

#include <Carbon/Carbon.h>

// for NXEvent
#include <drivers/event_status_driver.h>


#define MAX_DISPLAY 100
#define DEFAULT_DISPLAY "0" // no prepended ':'


class XServer {
public:
    XServer(char *display);
    ~XServer();
    
    Boolean Run();
    void Show();
    void Hide();
    void Toggle();
    void Kill();
    void UpdateModifiers();
    void UpdateModifiers(UInt32 modifiers);
    OSStatus SendKey(EventKind kind, UInt32 keyCode, UInt32 modifiers);
    OSStatus SendMouseEvent(EventRef event);
    Boolean IsRunning() { return mRunning; };
    Boolean IsVisible() { return mVisible; };
    
protected:
    void SendShowHide(Boolean show);
    void SendEvent(NXEvent ev);
    void ClearModifiers();


protected:
    // server state
    Boolean mRunning;
    Boolean mVisible;
    UInt32 mModifiers;
    
    // communication
    int mEventWriteFD;
    FILE *mOutputReadFile;

    // settings
    char mDisplay[MAX_DISPLAY];
    
// NX_APPDEFINED event subtypes for special commands to the X server
// clear modifiers: release all pressed modifier keys
// show: vt switch to X server; recapture screen and restore X drawing
// hide: vt switch away from X server; release screen and clip X drawing
// quit: kill the X server and release the display
protected:
    enum {
        kClearModifiers,
        kShow,
        kHide,
        kQuit
    };

};


#endif
