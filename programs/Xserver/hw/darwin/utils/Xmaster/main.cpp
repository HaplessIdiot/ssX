/*
 *  file: main.cpp
 *  project: Xmaster
 *
 * Greg Parker (gparker@cs.stanford.edu) November 2000
 * This code is hereby released into the public domain.
 *
 */

/*
bugs:
    gui needs recompile to change settings
    gui should ask user whether to kill xserver on quit
    send multiple mouse buttons as NX_SYSDEFINED events
*/

#include <Carbon/Carbon.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

// for NXEvent
#include <drivers/event_status_driver.h>

#include "XServer.h"
#include "Keymapper.h"

XServer *xserver;

Boolean killed = false;


// SIGPIPE handler for death of X server and close of event fifo
// suck
// signals do not preempt the Carbon event loop
// the signal handler is only called when some other event comes in
// Is QuitApplicationEventLoop safe here?
void SIGPIPEhandler(int sigraised) {
    killed = true;
    QuitApplicationEventLoop();
    #warning fixme signals dont stop the event loop wait
}




static UInt32 hideModifiers = cmdKey | optionKey | controlKey | shiftKey;
static UInt32 hideKeycode = 0; // 'a'

// Keyboard event handler
// Traps the magic keypress to toggle the X server.
// Sends modifier changes to the X server regardless of visibility.
// Sends keypresses to the X server if it is visible.
static OSStatus KeyboardHandler(
    EventHandlerCallRef nextHandler,
    EventRef            event,
    void                *userData) 
{
    UInt32 keyCode;
    OSStatus result;
    UInt32 modifiers; 
    
    if (GetEventClass(event) != kEventClassKeyboard) return noErr;
    
    result = GetEventParameter(event, kEventParamKeyCode, 
                            typeUInt32, NULL,      // type
                            sizeof(keyCode), NULL,  // size
                            &keyCode);
    if (result) return result;
    if (keyCode > 0xffff) {
        printf("xmaster: big key code %d 0x%x\n", (int)keyCode, (int)keyCode);
        return noErr;
    }

    result = GetEventParameter(event, kEventParamKeyModifiers, 
                               typeUInt32, NULL, sizeof(modifiers),
                               NULL, &modifiers);
    if (result) return noErr;

    if (hideKeycode == keyCode  &&  (modifiers & hideModifiers) == hideModifiers) {
        // vt switch key - toggle server on keyDown
        if (GetEventKind(event) == kEventRawKeyDown) {
            xserver->Toggle();
        }
        return noErr;
    }
    else if (GetEventKind(event) == kEventRawKeyModifiersChanged) {
        // send modifier changes always
    	xserver->UpdateModifiers(modifiers);
        return noErr;
    }
    else if (! xserver->IsVisible()) {
        // no-op if not VT switch and not modifier change and not visible
        return eventNotHandledErr; 
    }
    else {
        return xserver->SendKey(GetEventKind(event), keyCode, modifiers);
    }
}


// Mouse event handler
// Sends mouse move, button, wheel events to X server if it is visible.
static OSStatus MouseHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    
    if (! xserver->IsVisible()) {
        return eventNotHandledErr; // no-op if not visible
    }
    
    if (GetEventClass(event) != kEventClassMouse) {
        // wrong event type?!
        return eventNotHandledErr;
    }

    return xserver->SendMouseEvent(event);
}



// Command handler
// Show the X server when the big X button is pressed.
static OSStatus CommandHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    if (GetEventKind(event) == kEventCommandProcess) {
        HICommand command;
        GetEventParameter(event, kEventParamDirectObject, 
                            typeHICommand, NULL, sizeof(command), NULL, 
                            &command);
        if (command.commandID == 'go X') {
            // the big X button
            xserver->Show();
            return noErr;
        }
    }
    return CallNextEventHandler(nextHandler, event);
}


// Application event handler
// Hide X server when Xmaster is deactivated.
// todo: Ask user and optionally hide X server when app is quit (here? or after RunEventLoop?)
static OSStatus AppHandler(EventHandlerCallRef nextHandler, EventRef event, void *userData)
{
    switch (GetEventKind(event)) {
        case kEventAppActivated:
            // ShowXServer();
            break;
            
        case kEventAppDeactivated:
            // hide running&visible X server, if any
            xserver->Hide();
            break;
            
        case kEventAppQuit:
            break;
            
        case kEventAppLaunchNotification:
            break;
            
        default:
            break;
    }
    
    return CallNextEventHandler(nextHandler, event);
}




void DoRun() {
    EventTypeSpec mouseEvents[] = {
        { kEventClassMouse, kEventMouseDown },
        { kEventClassMouse, kEventMouseUp },
        { kEventClassMouse, kEventMouseMoved },
        { kEventClassMouse, kEventMouseDragged },
        { kEventClassMouse, kEventMouseWheelMoved }
    };
    EventTypeSpec keyboardEvents[] = {
        { kEventClassKeyboard, kEventRawKeyDown },
        { kEventClassKeyboard, kEventRawKeyRepeat }, 
        { kEventClassKeyboard, kEventRawKeyUp }, 
        { kEventClassKeyboard, kEventRawKeyModifiersChanged }, 
    };
    EventTypeSpec commandEvents[] = {
        { kEventClassCommand, kEventCommandProcess }
    };
    EventTypeSpec appEvents[] = {
        { kEventClassApplication, kEventAppActivated },
        { kEventClassApplication, kEventAppDeactivated },
        { kEventClassApplication, kEventAppQuit },
        { kEventClassApplication, kEventAppLaunchNotification }
    };
    
    OSStatus err;
    char display[] = "0";
        
    // register SIGPIPE handler
    signal(SIGPIPE, SIGPIPEhandler);
    
    Keymapper::Init();

    // Install event handlers:
    // raw keyboard & mouse events get passed to the X server.

    err = InstallEventHandler(GetApplicationEventTarget(), 
                                NewEventHandlerUPP(MouseHandler),
                                sizeof(mouseEvents)/sizeof(EventTypeSpec),
                                mouseEvents, NULL, NULL);
    if (err) return;
    
    err = InstallEventHandler(GetApplicationEventTarget(), 
                                NewEventHandlerUPP(KeyboardHandler),
                                sizeof(keyboardEvents)/sizeof(EventTypeSpec),
                                keyboardEvents, NULL, NULL);
    if (err) return;

    err = InstallEventHandler(GetApplicationEventTarget(), 
                                NewEventHandlerUPP(CommandHandler),
                                sizeof(commandEvents)/sizeof(EventTypeSpec),
                                commandEvents, NULL, NULL);
    if (err) return;

    err = InstallEventHandler(GetApplicationEventTarget(), 
                                NewEventHandlerUPP(AppHandler),
                                sizeof(appEvents)/sizeof(EventTypeSpec),
                                appEvents, NULL, NULL);
    if (err) return;


    xserver = new XServer(display);
    xserver->Run();
    
    RunApplicationEventLoop();
    
    printf("event loop done\n");
    delete xserver;
}




int main(int argc, char* argv[])
{
    IBNibRef 		nibRef;
    WindowRef 		window;
    
    OSStatus		err;

    // Create a Nib reference passing the name of the nib file (without the .nib extension)
    // CreateNibReference only searches into the application bundle.
    err = CreateNibReference(CFSTR("main"), &nibRef);
    require_noerr( err, CantGetNibRef );
    
    // Once the nib reference is created, set the menu bar. "MainMenu" is the name of the menu bar
    // object. This name is set in InterfaceBuilder when the nib is created.
    err = SetMenuBarFromNib(nibRef, CFSTR("MainMenu"));
    require_noerr( err, CantSetMenuBar );
    
    // Then create a window. "MainWindow" is the name of the window object. This name is set in 
    // InterfaceBuilder when the nib is created.
    err = CreateWindowFromNib(nibRef, CFSTR("MainWindow"), &window);
    require_noerr( err, CantCreateWindow );

    // We don't need the nib reference anymore.
    DisposeNibReference(nibRef);
    
    // The window was created hidden so show it.
    ShowWindow( window );
    
    // Call the event loop
    DoRun();

CantCreateWindow:
CantSetMenuBar:
CantGetNibRef:
	return err;
}

