/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/cirrus/cir_hwcurs.c,v 1.2 1998/08/29 07:39:52 dawes Exp $ */

/* (c) Itai Nahshon */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "vgaHW.h"

#include "xaa.h"

#include "cir.h"

#define CURSORWIDTH	32
#define CURSORHEIGHT	32
#define CURSORSIZE      (CURSORWIDTH*CURSORHEIGHT/8)
    
static void
CIRSetCursorColors(ScrnInfoPtr pScrn, int bg, int fg) {
    CIRPtr pCir = CIRPTR(pScrn);
#ifdef CIR_DEBUG
    ErrorF("CIRSetCursorColors\n");
#endif
    outw(0x3C4, ((pCir->ModeReg.ExtVga[SR12] | 0x02) << 8) | 0x12);
    outb(0x3c8, 0x00); outb(0x3c9, 0x3f & (bg >> 18)); outb(0x3c9, 0x3f & (bg >> 10)); outb(0x3c9, 0x3f & (bg >> 2));
    outb(0x3c8, 0x0f); outb(0x3c9, 0x3f & (fg >> 18)); outb(0x3c9, 0x3f & (fg >> 10)); outb(0x3c9, 0x3f & (fg >> 2));
    outw(0x3C4, (pCir->ModeReg.ExtVga[SR12] << 8) | 0x12);
}

static void
CIRLoadSkewedCursor(unsigned char *memx, unsigned char *CursorBits,
                    int x, int y) {
    unsigned char mem[2*CURSORSIZE];
    unsigned char *p1, *p2;
    int i, j, m, a, b;

    if(x > 0) x = 0; else x = -x;
    if(y > 0) y = 0; else y = -y;

    a = (x+y*CURSORWIDTH)>>3;
    b = x & 7;

    /* Copy the skewed mask bits */
    p1 = mem;
    p2 = CursorBits+a;
    for(i = 0; i < CURSORSIZE-a-1; i++) {
        *p1++ = (p2[0] << b) | (p2[1] >> (8-b));
	p2++;
    }
    /* last mask byte */
    *p1++ = (p2[0] << b);

    /* Clear to end (bottom) of mask. */
    for(i = i+1; i < CURSORSIZE; i++) {
        *p1++ = 0;
    }

    /* Now copy the cursor bits */
    /* p1 is already right */
    p2 = CursorBits+CURSORSIZE+a;
    for(i = 0; i < CURSORSIZE-a-1; i++) {
        *p1++ = (p2[0] << b) | (p2[1] >> (8-b));
	p2++;
    }
    /* last cursor  byte */
    *p1++ = (p2[0] << b);

    /* Clear to end (bottom) of cursor. */
    for(i = i+1; i < CURSORSIZE; i++) {
        *p1++ = 0;
    }

    /* Clear the right unused area of the mask
       and cyrsor bits.  */
    p2 = mem + CURSORWIDTH/8 - (x>>3) - 1;
    for(i = 0; i < 2*CURSORHEIGHT; i++) {
       m = (-1)<<(x&7);
       p1 = p2;
       p2 += CURSORWIDTH/8;
       for(j = x>>3; j >= 0; j--) {
          *p1 &= m;
          m = 0;
	  p1++;
       }
    }
    memcpy(memx, mem, 2*CURSORSIZE);
}


static void
CIRSetCursorPosition(ScrnInfoPtr pScrn, int x, int y) {
    CIRPtr pCir = CIRPTR(pScrn);

#if 0
#ifdef CIR_DEBUG
    ErrorF("CIRSetCursorPosition %d %d\n", x, y);
#endif
#endif

    if(x < 0 || y < 0) {
        if(x+CURSORWIDTH <= 0 || y+CURSORHEIGHT <= 0) {
            outw(0x3C4, ((pCir->ModeReg.ExtVga[SR12] & ~0x01) << 8) | 0x12);
            return;
        }
        CIRLoadSkewedCursor(pCir->HWCursorBits, pCir->CursorBits, x, y);
        pCir->CursorIsSkewed = TRUE;
        if(x < 0) x = 0;
        if(y < 0) y = 0;
    }
    else if(pCir->CursorIsSkewed) {
	memcpy(pCir->HWCursorBits, pCir->CursorBits, 2*CURSORSIZE);
        pCir->CursorIsSkewed = FALSE;
    }
    outw(0x3C4, (pCir->ModeReg.ExtVga[SR12] << 8) | 0x12);
    outw(0x3C4, (x << 5) | 0x10);
    outw(0x3C4, (y << 5) | 0x11);
}

static void
CIRLoadCursorImage(ScrnInfoPtr pScrn, unsigned char *bits) {
    CIRPtr pCir = CIRPTR(pScrn);

#ifdef CIR_DEBUG
    ErrorF("CIRLoadCursorImage\n");
#endif

    pCir->CursorBits = bits;
    memcpy(pCir->HWCursorBits, bits, 2*CURSORSIZE);
    pCir->ModeReg.ExtVga[SR13] = 0x3f;
    outw(0x3C4, 0x3f13);
}

static void
CIRHideCursor(ScrnInfoPtr pScrn) {
    CIRPtr pCir = CIRPTR(pScrn);

#ifdef CIR_DEBUG
    ErrorF("CIRHideCursor\n");
#endif
    pCir->ModeReg.ExtVga[SR12] &= ~0x01;
    outw(0x3C4, (pCir->ModeReg.ExtVga[SR12] << 8) | 0x12);
}

static void
CIRShowCursor(ScrnInfoPtr pScrn) {
    CIRPtr pCir = CIRPTR(pScrn);

#ifdef CIR_DEBUG
    ErrorF("CIRShowCursor\n");
#endif
    pCir->ModeReg.ExtVga[SR12] |= 0x01;
    outw(0x3C4, (pCir->ModeReg.ExtVga[SR12] << 8) | 0x12);
}

static Bool
CIRUseHWCursor(ScreenPtr pScreen, CursorPtr pCurs) {
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
#ifdef CIR_DEBUG
    ErrorF("CIRUseHWCursor\n");
#endif
    if(pScrn->bitsPerPixel < 8)
       return FALSE;

    return TRUE;
}

Bool 
CIRHWCursorInit(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    CIRPtr pCir = CIRPTR(pScrn);
    xf86CursorInfoPtr infoPtr;

#ifdef CIR_DEBUG
    ErrorF("CIRHWCursorInit\n");
#endif

    infoPtr = xf86CreateCursorInfoRec();
    if(!infoPtr) return FALSE;
    
    pCir->CursorInfoRec = infoPtr;
    pCir->HWCursorBits = pCir->FbBase + 1024*pScrn->videoRam - 2*CURSORSIZE;
    pCir->CursorIsSkewed = FALSE;
    pCir->CursorBits = NULL;

    infoPtr->MaxWidth = CURSORWIDTH;
    infoPtr->MaxHeight = CURSORHEIGHT;
    infoPtr->Flags = HARDWARE_CURSOR_BIT_ORDER_MSBFIRST |
		     HARDWARE_CURSOR_TRUECOLOR_AT_8BPP |
		     HARDWARE_CURSOR_SOURCE_MASK_NOT_INTERLEAVED;
    infoPtr->SetCursorColors = CIRSetCursorColors;
    infoPtr->SetCursorPosition = CIRSetCursorPosition;
    infoPtr->LoadCursorImage = CIRLoadCursorImage;
    infoPtr->HideCursor = CIRHideCursor;
    infoPtr->ShowCursor = CIRShowCursor;
    infoPtr->UseHWCursor = CIRUseHWCursor;
    infoPtr->RealizeCursor = NULL;

#ifdef CIR_DEBUG
    ErrorF("CIRHWCursorInit before xf86InitCursor\n");
#endif

    return(xf86InitCursor(pScreen, infoPtr));
}
