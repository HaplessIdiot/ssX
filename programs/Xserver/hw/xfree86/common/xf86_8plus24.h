/* $XFree86$ */

#ifndef _XF86_8PLUS_24_H
#define _XF86_8PLUS_24_H

typedef struct {
   void (*FillSolidRects)(
	WindowPtr pWin,
	unsigned long fg,
	unsigned long planemask,
	RegionPtr pReg
   );
   void (*FillTiledRects)(
	WindowPtr pWin,
	PixmapPtr pPix,
	int xorg, int yorg,
	unsigned long planemask,
	RegionPtr pReg
   );
   void (*CopyAreas)(
	WindowPtr pWin,
	unsigned long planemask,
	DDXPointPtr ppnts,
	RegionPtr pReg
   );
} OverlayFBfuncs, *OverlayFBfuncsPtr;

extern OverlayFBfuncs diOverlayFBfuncs;

Bool
xf86Overlay8Plus24Init (
   ScreenPtr pScreen,
   unsigned char key,
   unsigned long keyColor,
   Bool destructive,
   OverlayFBfuncsPtr
);

#endif /* _XF86_8PLUS_24_H */
