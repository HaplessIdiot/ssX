/* $XFree86$ */
/*
 * cfbfuncs.c
 *
 * Initialise low level cfb functions
 * 
 * To coax et4_driver.c into cooperation--GGL 
 */

#include "cfb.h"
#include "cfbfuncs.h"

GCFuncs vga256TEOps1Rect, vga256NonTEOps1Rect, vga256TEOps, vga256NonTEOps;

CfbfuncRec vga256LowlevFuncs;

void speedupvga256FillRectSolidCopy(){FatalError("FillRectSolidCopy");}

void speedupvga256DoBitbltCopy(){FatalError("DoBitbltCopy");}

void speedupvga256SegmentSS(){FatalError("SegmentSS");}

void speedupvga256LineSS(){FatalError("LineSS");}

void speedupvga256TEGlyphBlt8(){FatalError("TEGlyphBlt8");}

void speedupvga2568FillRectTransparentStippled32()
     {FatalError("FillRectTransparentStippled32");}

void speedupvga256FillBoxSolid(){FatalError("FillBoxSolid");}

void speedupvga2568FillRectOpaqueStippled32()
     {FatalError("FillRectOpqueStippled32");}
