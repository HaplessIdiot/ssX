/* $XConsortium: xfsconf.h /main/2 1996/10/19 19:06:59 kaleb $ */





/* $XFree86: xc/programs/Xserver/hw/xfree86/XF86Setup/xfsconf.h,v 3.2 1996/12/27 06:54:27 dawes Exp $ */

extern char *XtMalloc(
#if NeedFunctionPrototypes
    unsigned int	/* size */
#endif
);

extern char *XtCalloc(
#if NeedFunctionPrototypes
    unsigned int	/* num */,
    unsigned int	/* size */
#endif
);

extern char *XtRealloc(
#if NeedFunctionPrototypes
    char*		/* ptr */,
    unsigned int	/* num */
#endif
);

extern void XtFree(
#if NeedFunctionPrototypes
    char*		/* ptr */
#endif
);
