/*
 * file: Keymapper.h
 * project: Xmaster
 *
 * Object wrapper around darwinKeyMapping.
 *
 * Greg Parker (gparker@cs.stanford.edu) November 2000
 * This code is hereby released into the public domain.
 *
 */

#ifndef KEYMAPPER_H
#define KEYMAPPER_H

#include <Carbon/Carbon.h>

class Keymapper {
public:
    static void Init();
    static UInt32 TranslateModifiers(UInt32 carbonModifiers);
    static UInt32 TranslateKeycode(UInt32 carbonKeycode);
    static UInt32 ModifierKeycode(UInt32 carbonModifierMask);
    
protected:
    // Carbon->NX modifiers
    enum {mask=0, nx=1, side=2};
    static UInt32 modlist[8][3];
        
        
private:
    // private contructor to prevent construction
    Keymapper() { };
};

#endif

