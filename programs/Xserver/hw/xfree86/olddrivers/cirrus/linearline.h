/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/linearline.h,v 1.1 1997/03/06 23:15:38 hohndel Exp $ */





/* $XConsortium: linearline.h /main/3 1996/02/21 18:04:29 kaleb $ */

/* linearline.c */
extern void LinearFramebufferVerticalLine(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int
#endif
);

extern void LinearFramebufferDualVerticalLine(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int
#endif
);

extern void LinearFramebufferSlopedLineLeft(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int,
    int
#endif
);

extern void LinearFramebufferSlopedLineRight(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int,
    int
#endif
);

extern void LinearFramebufferSlopedLineVerticalLeft(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int,
    int
#endif
);

extern void LinearFramebufferSlopedLineVerticalRight(
#if NeedFunctionPrototypes
    unsigned char *,
    int,
    int,
    int,
    int,
    int
#endif
);
