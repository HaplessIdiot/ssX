/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaacursor.h,v 1.1.2.2 1998/07/19 13:22:13 dawes Exp $ */

#ifndef _XAACURSOR_H
#define _XAACURSOR_H

#include "mipointrst.h"

typedef struct _XAACursorInfoRec {
    ScrnInfoPtr pScrn;
    int Flags;
    int MaxWidth;
    int MaxHeight;
    void (*SetCursorColors)(ScrnInfoPtr pScrn, int bg, int fg);
    void (*SetCursorPosition)(ScrnInfoPtr pScrn, int x, int y);
    void (*LoadCursorImage)(ScrnInfoPtr pScrn, unsigned char *bits);
    void (*HideCursor)(ScrnInfoPtr pScrn);
    void (*ShowCursor)(ScrnInfoPtr pScrn);
    unsigned char* (*RealizeCursor)(struct _XAACursorInfoRec *, CursorPtr);
    Bool (*UseHWCursor)(ScreenPtr, CursorPtr);

} XAACursorInfoRec, *XAACursorInfoPtr;

typedef struct {
    Bool			SWCursor;
    Bool			isUp;
    short			HotX;
    short			HotY;
    short			x;
    short			y;
    CursorPtr			CurrentCursor;
    XAACursorInfoPtr		CursorInfoPtr;
    CloseScreenProcPtr          CloseScreen;
    RecolorCursorProcPtr	RecolorCursor;
    InstallColormapProcPtr	InstallColormap;
    QueryBestSizeProcPtr	QueryBestSize;
    miPointerSpriteFuncPtr	spriteFuncs;
    Bool			PalettedCursor;
    ColormapPtr			pInstalledMap;
    Bool                	(*SwitchMode)(int, DisplayModePtr,int);
    Bool                	(*EnterVT)(int, int);
    void                	(*LeaveVT)(int, int);
} XAACursorScreenRec, *XAACursorScreenPtr;

Bool XAAInitCursor(
   ScreenPtr pScreen, 
   XAACursorInfoPtr infoPtr
);

XAACursorInfoPtr XAACreateCursorInfoRec(void);
void XAADestroyCursorInfoRec(XAACursorInfoPtr);


void XAASetCursor(ScreenPtr pScreen, CursorPtr pCurs, int x, int y);
void XAAMoveCursor(ScreenPtr pScreen, int x, int y);
void XAARecolorCursor(ScreenPtr pScreen, CursorPtr pCurs, Bool displayed);
Bool XAAInitHardwareCursor(ScreenPtr pScreen, XAACursorInfoPtr infoPtr);

extern int XAACursorScreenIndex;

#define HARDWARE_CURSOR_INVERT_MASK 			0x00000001
#define HARDWARE_CURSOR_AND_SOURCE_WITH_MASK		0x00000002
#define HARDWARE_CURSOR_SWAP_SOURCE_AND_MASK		0x00000004
#define HARDWARE_CURSOR_SOURCE_MASK_NOT_INTERLEAVED	0x00000008
#define HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_1	0x00000010
#define HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_8	0x00000020
#define HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_16	0x00000040
#define HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_32	0x00000080
#define HARDWARE_CURSOR_SOURCE_MASK_INTERLEAVE_64	0x00000100
#define HARDWARE_CURSOR_TRUECOLOR_AT_8BPP		0x00000200
#define HARDWARE_CURSOR_BIT_ORDER_MSBFIRST		0x00000400


#endif /* _XAACURSOR_H */
