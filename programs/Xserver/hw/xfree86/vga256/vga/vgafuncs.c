/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/vga/vgafuncs.c,v 3.0 1994/07/24 11:58:55 dawes Exp $ */

/*
 * vgafuncs.c
 *
 * Initialise low level vga256 functions
 */
#include "vga256.h"

CfbfuncRec vga256LowlevFuncs = {
    vgaBitBlt,		/* default to the 2-bank version */
    vga256DoBitbltCopy,
    vga256FillRectSolidCopy,
    vga2568FillRectTransparentStippled32,
    vga2568FillRectOpaqueStippled32,
    vga256SegmentSS,
    vga256LineSS,
    vga256FillBoxSolid,
    vga256TEGlyphBlt8,
    vga256CopyPlane1to8,
    vga256SolidSpansGeneral,
};

