/* $XConsortium: mach8cache.h,v 1.1 94/03/28 21:10:27 dpw Exp $ */
void mach8GlyphWrite(
#if NeedFunctionPrototypes
    int /*x*/,
    int /*y*/,
    int /*count*/,
    int /*numRects*/,
    unsigned char */*chars*/,
    CacheFont8Ptr /*fentry*/,
    GCPtr /*pGC*/,
    BoxPtr /*pBox*/
#endif
);

int mach8NoCPolyText(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    char */*chars*/,
    Bool /*is8bit*/
#endif
);

int mach8NoCImageText(
#if NeedFunctionPrototypes
    DrawablePtr /*pDraw*/,
    GCPtr /*pGC*/,
    int /*x*/,
    int /*y*/,
    int /*count*/,
    char */*chars*/,
    Bool /*is8bit*/
#endif
);

void mach8CacheMoveBlock(
#if NeedFunctionPrototypes
    int /*srcx*/,
    int /*srcy*/,
    int /*dstx*/,
    int /*dsty*/,
    int /*h*/,
    int /*len*/,
    unsigned int /*bitplane*/
#endif
);


void mach8ImageOpStipple(
#if NeedFunctionPrototypes
    int /*x*/,
    int /*y*/,
    int /*w*/,
    int /*h*/,
    unsigned char */*psrc*/,
    int /*pw*/,
    int /*ph*/,
    int /*pox*/,
    int /*poy*/,
    int /*pwidth*/,
    int /*fgPixel*/,
    int /*bgPixel*/,
    short /*alu*/,
    short /*planemask*/
#endif
);
