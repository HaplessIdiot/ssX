/* $XFree86$ */

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

#endif /* _PANORAMIXSRV_H_ */
