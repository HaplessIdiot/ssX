/*
 *  file: Keymapper.cpp
 *  project: Xmaster
 *
 * Object wrapper around darwinKeyMapping.
 *
 * Greg Parker (gparker@cs.stanford.edu) November 2000
 * This code is hereby released into the public domain.
 *
 */

#include "Keymapper.h"
#include "darwinKeyMapping.h"
#include <drivers/event_status_driver.h>
#include <IOKit/hidsystem/ev_keymap.h>

UInt32 Keymapper::modlist[8][3] = {
    // carbon mask, nx index, 0=left/1=right
    {cmdKey, NX_MODIFIERKEY_COMMAND, 0}, 
    {alphaLock, NX_MODIFIERKEY_ALPHALOCK, 0}, 
    {shiftKey, NX_MODIFIERKEY_SHIFT, 0}, 
    {optionKey, NX_MODIFIERKEY_ALTERNATE, 0}, 
    {controlKey, NX_MODIFIERKEY_CONTROL, 0},
    {rightShiftKey, NX_MODIFIERKEY_SHIFT, 1},
    {rightOptionKey, NX_MODIFIERKEY_ALTERNATE, 1},
    {rightControlKey, NX_MODIFIERKEY_CONTROL, 1}
};


void Keymapper::Init() {
    ::InitKeyMap();
}


// translate Carbon modifier flags to NX modifiers
// NX modifiers do not distinguish between left and right - the 
// event's keycode needs to specify the correct side.
UInt32 Keymapper::TranslateModifiers(UInt32 carbonModifiers) 
{
    UInt32 nxModifiers = 0;
    
    if (carbonModifiers & cmdKey)          nxModifiers |= NX_COMMANDMASK;
    if (carbonModifiers & alphaLock)       nxModifiers |= NX_ALPHASHIFTMASK;
    if (carbonModifiers & shiftKey  ||
        carbonModifiers & rightShiftKey)   nxModifiers |= NX_SHIFTMASK;
    if (carbonModifiers & optionKey  ||
        carbonModifiers & rightOptionKey)  nxModifiers |= NX_ALTERNATEMASK;
    if (carbonModifiers & controlKey  || 
        carbonModifiers & rightControlKey) nxModifiers |= NX_CONTROLMASK;

    return nxModifiers;
}


// Translate Carbon->NX keycode
// The only change is the arrow keys.
UInt32 Keymapper::TranslateKeycode(UInt32 carbonKeycode) 
{
    UInt32 nxKeycode = carbonKeycode;
    
    // FIXME: This should use device independent character codes
    // not virtual keycodes
    if (carbonKeycode >= 0x7b  &&  carbonKeycode <= 0x7e) {
        // arrow keys
        nxKeycode = ::ArrowKeycode(carbonKeycode-0x7b);
    }
    
    return nxKeycode;
}


// Get the NX keycode of a Carbon modifier
UInt32 Keymapper::ModifierKeycode(UInt32 carbonModifierMask) 
{
    for (int i = 0; i < 8; i++) {
        if (modlist[i][mask] == carbonModifierMask) {
            return ::ModifierKeycode(modlist[i][nx], modlist[i][side]);
        }
    }
    return 0xffffffff;
}
