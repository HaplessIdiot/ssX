/* $XFree86$ */

#ifndef _ANTI_H
#define _ANTI_H

typedef struct {
    CreateGCProcPtr	CreateGC;
    Bool 		(*PolyTextFunc)(PTclosurePtr);
    Bool 		(*ImageTextFunc)(ITclosurePtr);
} AntiScreenRec, *AntiScreenPtr;

typedef struct {
    int 		NumPixels;
    unsigned long 	*Pixels;
    GCFuncs		*wrapFuncs;
    Bool		clientData;
    unsigned long	fg;
    unsigned long	bg;
} AntiGCRec, *AntiGCPtr;

extern int XAntiGCIndex;
extern int XAntiScreenIndex;

void XAntiExtensionInit(void);


#define XANTI_GET_GC_PRIVATE(gc) \
	(AntiGCPtr)((gc)->devPrivates[XAntiGCIndex].ptr)
#define XANTI_GET_SCREEN_PRIVATE(draw) \
	(AntiScreenPtr)((draw)->pScreen->devPrivates[XAntiScreenIndex].ptr)
#define XANTI_GET_FONT_PRIVATE(font) 0 /* for now */

#endif /* _ANTI_H */

