/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cir_span.h,v 1.3 1998/01/24 16:58:00 hohndel Exp $ */





/* $XConsortium: cir_span.h /main/5 1996/02/21 18:04:10 kaleb $ */
/*
 * Definitions for span functions in cir_span.s/cir_span.c
 */
void CirrusLatchCopySpans(
#if NeedFunctionPrototypes
	unsigned char *,
	unsigned char *,
	int,
	int,
	int
#endif			  
);

void CirrusColorExpandWriteSpans(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int,
    int,
    int,
    int
#endif
);

void CirrusColorExpandWriteStippleSpans(
#if NeedFunctionPrototypes
    unsigned char *,
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

void CirrusLatchWriteTileSpans(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int
#endif
);
