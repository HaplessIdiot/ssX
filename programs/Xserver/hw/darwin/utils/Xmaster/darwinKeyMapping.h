/*
  darwinKeyMapping.h
  Darwin key map builder
*/

#ifndef _DARWINKEYMAPPING_H
#define _DARWINKEYMAPPING_H

#ifdef __cplusplus
extern "C" {
#endif

// Build the Darwin key map.
void InitKeyMap(void);

// Return the Darwin keycode for the given modifier.
// modifier should be NX_MODIFIERKEY_*
// side is 0 for left, 1 for right
int ModifierKeycode(int modifier, int side);

// Return the Darwin keycode for the given arrow key
// arrows are 0..3 left,right,down,up
// this matches the Carbon keycode order
int ArrowKeycode(int arrow);


#ifdef __cplusplus
}
#endif

#endif
