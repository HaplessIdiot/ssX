/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/cirrus/cir_span.h,v 3.0 1994/08/20 07:36:37 dawes Exp $ */

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
    void *,
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
    void *,
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
