/* $XConsortium: cfbfuncs.c,v 1.1 94/03/28 21:44:47 dpw Exp $ */
/*
 * cfbfuncs.c
 *
 * Initialise low level cfb functions
 */

#include "cfb.h"
#include "cfbfuncs.h"

extern void vgaBitBlt();
extern void vgacfbFillBoxSolid();

CfbfuncRec cfbLowlevFuncs = {
    vgaBitBlt,				/* default to the 2-bank version */
    cfbDoBitbltCopy,
    cfbFillRectSolidCopy,
    cfb8FillRectTransparentStippled32,
    cfb8FillRectOpaqueStippled32,
    cfbSegmentSS,
    cfbLineSS,
    vgacfbFillBoxSolid,
    cfbTEGlyphBlt8,
};

