/* $XFree86: xc/include/extensions/panoramiXsrv.h,v 1.2 1999/06/27 14:07:20 dawes Exp $ */

#ifndef _PANORAMIXSRV_H_
#define _PANORAMIXSRV_H_

extern int PanoramiXNumScreens;
extern PanoramiXWindow *PanoramiXWinRoot;
extern PanoramiXGC *PanoramiXGCRoot;
extern PanoramiXCmap *PanoramiXCmapRoot;
extern PanoramiXPmap *PanoramiXPmapRoot;
extern PanoramiXData *panoramiXdataPtr;
extern int PanoramiXPixWidth;
extern int PanoramiXPixHeight;
extern PanoramiXCDT PanoramiXColorDepthTable[MAXSCREENS];
extern RegionRec PanoramiXScreenRegion[MAXSCREENS];

extern void PanoramiXConsolidate(void);
extern Bool PanoramiXCreateConnectionBlock(void);
extern Bool PanoramiXCreateScreenRegion(WindowPtr);

/*
 * Free list flags added as pre-test before running through list to free ids
 */
extern Bool PanoramiXWinRootFreeable;
extern Bool PanoramiXGCRootFreeable;
extern Bool PanoramiXCmapRootFreeable;
extern Bool PanoramiXPmapRootFreeable;

extern void XineramaGetImageData(
    DrawablePtr *pDrawables,
    int left,
    int top,
    int width, 
    int height,
    unsigned int format,
    unsigned long planemask,
    char *data,
    int pitch,
    Bool isRoot
);

#endif /* _PANORAMIXSRV_H_ */
