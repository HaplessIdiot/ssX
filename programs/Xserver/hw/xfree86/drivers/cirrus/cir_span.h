/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cir_span.h,v 1.1 1997/03/06 23:15:33 hohndel Exp $ */





/* $XConsortium: cir_span.h /main/5 1996/02/21 18:04:10 kaleb $ */
/*
 * Definitions for span functions in cir_span.s
 */

#ifdef AVOID_ASM_ROUTINES
#define __FTYPE__ static
#else
#define __FTYPE__ extern
#endif

__FTYPE__ void CirrusColorExpandWriteSpans(
#if NeedFunctionPrototypes
#ifndef AVOID_ASM_ROUTINES
    void *,
#else
    unsigned char *,
#endif
    int,
    int,
    int,
    int,
    int,
    int,
    int
#endif
);

__FTYPE__ void CirrusColorExpandWriteStippleSpans(
#if NeedFunctionPrototypes
#ifndef AVOID_ASM_ROUTINES
    void *,
#else
    unsigned char *,
#endif
    int,
    int,
    int,
    int,
    int,
    int,
    unsigned long,
    int
#endif
);

__FTYPE__ void CirrusLatchWriteTileSpans(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int
#endif
);

#undef __FTYPE__
