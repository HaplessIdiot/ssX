/* $XConsortium: cfbfuncs.h,v 1.1 94/03/28 21:44:53 dpw Exp $ */
/* $XFree86$ */
/*
 * cfbfuncs.h
 */


#ifndef _CFBFUNCS_H
#define _CFBFUNCS_H

#include "gcstruct.h"
extern GCOps cfbTEOps1Rect, cfbTEOps, cfbNonTEOps1Rect, cfbNonTEOps;

typedef struct _Cfbfunc{
    void (*vgaBitblt)();
    void (*doBitbltCopy)(
#if NeedFunctionPrototype
		DrawablePtr,
		DrawablePtr,
		int,
		RegionPtr,
		DDXPointPtr,
		unsigned long
#endif
);
    void (*fillRectSolidCopy)();
    void (*fillRectTransparentStippled32)();
    void (*fillRectOpaqueStippled32)();
    void (*segmentSS)(
#if NeedFunctionPrototype
		DrawablePtr,
		GCPtr,
		int,
		xSegment
#endif
);
    void (*lineSS)(
#if NeedFunctionPrototype
		DrawablePtr,
		GCPtr,
		int,
		int,
		DDXPointPtr
#endif
);
    void (*fillBoxSolid)(
#if NeedFunctionPrototype
		DrawablePtr,
		int,
		BoxPtr,
		unsigned long,
		unsigned long,
		int
#endif
);
    void (*teGlyphBlt8)();
    void (*copyPlane1to8)(
#if NeedFunctionPrototype
		DrawablePtr,
		DrawablePtr,
		int,
		RegionPtr,
		DDXPointPtr,
		unsigned long
#endif
);
} CfbfuncRec, *CfbfuncPtr;

extern CfbfuncRec cfbLowlevFuncs;

/*
 * This doesn't really seem like the right place for this, but these
 * functions do seem to be tied to the low level funcs structure, it
 * fits OK.
 */

void speedupcfbDoBitbltCopy(
#if NeedFunctionPrototype
    DrawablePtr,	/* pDrawable */
    int,
    RegionPtr,
    DDXPointPtr,
    unsigned long
#endif
);

void speedupcfbSegmentSS(
#if NeedFunctionPrototype
    DrawablePtr,	/* pDrawable */
    GCPtr,		/* pGC */
    int,		/* nseg */
    xSegment		/* pSeg */
#endif
);

void speedupcfbLineSS(
#if NeedFunctionPrototype
    DrawablePtr,	/* pDrawable */
    GCPtr,		/* pGC */
    int,		/* mode */
    int,		/* npt */
    DDXPointPtr		/* pptInit */
#endif
);
#endif /* _CFBFUNCS_H */
