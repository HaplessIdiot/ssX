/* (c) Itai Nahshon */

/* $XFree86$ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

#include "xf86Pci.h"
#include "xf86PciInfo.h"

#include "vgaHW.h"

#include "cir.h"

#define minb(p) MMIO_IN8(hwp->MMIOBase, hwp->MMIOOffset + (p))
#define moutb(p,v) \
	MMIO_OUT8(hwp->MMIOBase, hwp->MMIOOffset + (p),(v))

static void CIRSync(ScrnInfoPtr pScrn)
{
    vgaHWPtr hwp = VGAHWPTR(pScrn);
#ifdef CIR_DEBUG
    ErrorF("CIRSync mm\n");
#endif

    moutb(0x3CE, 0x31);
    while(minb(0x3CF) & 0x01)
        ;

    return;
}

static void
CIRSetupForScreenToScreenCopy(
        ScrnInfoPtr pScrn,
        int xdir, int ydir,
        int rop,
        unsigned int planemask,
        int trans_color) {

    CIRPtr pCir = CIRPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    int pitch = pScrn->displayWidth * pScrn->bitsPerPixel / 8;

    moutb(0x3CE, 0x31);
    while(minb(0x3CF) & 0x01)
        ;

#ifdef CIR_DEBUG
    ErrorF("CIRSetupForScreenToScreenCopy xdir=%d ydir=%d rop=%x planemask=%x trans_color=%x\n",
            xdir, ydir, rop, planemask, trans_color);
#endif

    moutb(0x3CE, 0x32); moutb(0x3CF, 0x0d);

    /* Set dest pitch */
    moutb(0x3CE, 0x24);
    moutb(0x3CF, pitch & 0xff);
    moutb(0x3CE, 0x25);
    moutb(0x3CF, (pitch >> 8) & 0x1f);
    /* Set source pitch */
    moutb(0x3CE, 0x26);
    moutb(0x3CF, pitch & 0xff);
    moutb(0x3CE, 0x27);
    moutb(0x3CF, (pitch >> 8) & 0x1f);
}

static void
CIRSubsequentScreenToScreenCopy(
        ScrnInfoPtr pScrn,
        int x1, int y1,
        int x2, int y2,
        int w, int h) {

    CIRPtr pCir = CIRPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    int source, dest;
    int  hh, ww;
    int decrement = 0;
    int pitch = pScrn->displayWidth * pScrn->bitsPerPixel / 8;

    moutb(0x3CE, 0x31);
    while(minb(0x3CF) & 0x01)
        ;

    ww = (w * pScrn->bitsPerPixel / 8) - 1;
    hh = h - 1;
    /* Width */
    moutb(0x3CE, 0x20);
    moutb(0x3CF, ww & 0xff);
    moutb(0x3CE, 0x21);
    moutb(0x3CF, (ww >> 8)& 0x1f);
    /* Height */
    moutb(0x3CE, 0x22);
    moutb(0x3CF, hh & 0xff);
    moutb(0x3CE, 0x23);
    moutb(0x3CF, (hh >> 8) & 0x07);

    dest = y2 * pitch + x2 * pScrn->bitsPerPixel / 8;
    source = y1 * pitch + x1 * pScrn->bitsPerPixel / 8;
    if(dest > source) {
        decrement = 1;
        dest += hh * pitch + ww;
        source += hh * pitch + ww;
    }
    /* dest */
    moutb(0x3CE, 0x28);
    moutb(0x3CF, dest & 0xff);
    moutb(0x3CE, 0x29);
    moutb(0x3CF, (dest >> 8) & 0xff);
    moutb(0x3CE, 0x2A);
    moutb(0x3CF, (dest >> 16) & 0x3f);
    /* source */
    moutb(0x3CE, 0x2C);
    moutb(0x3CF, source & 0xff);
    moutb(0x3CE, 0x2D);
    moutb(0x3CF, (source >> 8) & 0xff);
    moutb(0x3CE, 0x2E);
    moutb(0x3CF, (source >> 16) & 0x3f);

#ifdef CIR_DEBUG
    ErrorF("CIRSubsequentScreenToScreenCopy x1=%d y1=%d x2=%d y2=%d w=%d h=%d\n",
           x1, y1, x2, y2, w, h);
    ErrorF("CIRSubsequentScreenToScreenCopy s=%d d=%d ww=%d hh=%d\n",
           source, dest, ww, hh);
#endif

    moutb(0x3CE, 0x30);
    moutb(0x3CF, decrement);
    moutb(0x3CE, 0x31);
    moutb(0x3CF, 2);
}

static void
CIRSetupForSolidFill(
	ScrnInfoPtr pScrn,
	int color,
	int rop,
	unsigned int planemask) {

    CIRPtr pCir = CIRPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    int pitch = pScrn->displayWidth * pScrn->bitsPerPixel / 8;

    moutb(0x3CE, 0x31);
    while(minb(0x3CF) & 0x01)
        ;

#ifdef CIR_DEBUG
    ErrorF("CIRSetupForSolidFill color=%x rop=%x planemask=%x\n",
            color, rop, planemask);
#endif

    moutb(0x3CE, 0x32); moutb(0x3CF, 0x0d);

    moutb(0x3CE, 0x33);
    moutb(0x3CF, 0x04);
    moutb(0x3CE, 0x30);
    moutb(0x3CF, 0xC0|((pScrn->bitsPerPixel - 8) << 1));

    moutb(0x3CE, 0x01);
    moutb(0x3CF, color & 0xff);
    moutb(0x3CE, 0x11);
    moutb(0x3CF, (color >> 8) & 0xff);
    moutb(0x3CE, 0x13);
    moutb(0x3CF, (color >> 16) & 0xff);
    moutb(0x3CE, 0x15);
    moutb(0x3CF, 0);

    /* Set dest pitch */
    moutb(0x3CE, 0x24);
    moutb(0x3CF, pitch & 0xff);
    moutb(0x3CE, 0x25);
    moutb(0x3CF, (pitch >> 8) & 0x1f);
}

static void
CIRSubsequentSolidFillRect(
	ScrnInfoPtr pScrn,
	int x, int y, int w, int h) {

    CIRPtr pCir = CIRPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    int dest;
    int hh, ww;
    int pitch = pScrn->displayWidth * pScrn->bitsPerPixel / 8;

    moutb(0x3CE, 0x31);
    while(minb(0x3CF) & 0x01)
        ;

    ww = (w * pScrn->bitsPerPixel / 8) - 1;
    hh = h - 1;
    /* Width */
    moutb(0x3CE, 0x20);
    moutb(0x3CF, ww & 0xff);
    moutb(0x3CE, 0x21);
    moutb(0x3CF, (ww >> 8)& 0x1f);
    /* Height */
    moutb(0x3CE, 0x22);
    moutb(0x3CF, hh & 0xff);
    moutb(0x3CE, 0x23);
    moutb(0x3CF, (hh >> 8) & 0x07);

    dest = y * pitch + x * pScrn->bitsPerPixel / 8;
    /* dest */
    moutb(0x3CE, 0x28);
    moutb(0x3CF, dest & 0xff);
    moutb(0x3CE, 0x29);
    moutb(0x3CF, (dest >> 8) & 0xff);
    moutb(0x3CE, 0x2A);
    moutb(0x3CF, (dest >> 16) & 0x3f);

#ifdef CIR_DEBUG
    ErrorF("CIRSubsequentSolidFillRect x=%d y=%d w=%d h=%d\n",
           x, y, w, h);
#endif

    moutb(0x3CE, 0x31);
    moutb(0x3CF, 2);
}

Bool 
CIRXAAInitMMIO(ScreenPtr pScreen)
{
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    CIRPtr pCir = CIRPTR(pScrn);
    vgaHWPtr hwp = VGAHWPTR(pScrn);
    XAAInfoRecPtr XAAPtr;

#ifdef CIR_DEBUG
    ErrorF("CIRXAAInit\n");
#endif

    XAAPtr = XAACreateInfoRec();
    if(!XAAPtr) return FALSE;

    XAAPtr->SetupForScreenToScreenCopy = CIRSetupForScreenToScreenCopy;
    XAAPtr->SubsequentScreenToScreenCopy = CIRSubsequentScreenToScreenCopy;
    XAAPtr->ScreenToScreenCopyFlags = GXCOPY_ONLY|NO_TRANSPARENCY|NO_PLANEMASK;

    XAAPtr->SetupForSolidFill = CIRSetupForSolidFill;
    XAAPtr->SubsequentSolidFillRect = CIRSubsequentSolidFillRect;
    XAAPtr->SubsequentSolidFillTrap = NULL;
    XAAPtr->SolidFillFlags = GXCOPY_ONLY|NO_TRANSPARENCY|NO_PLANEMASK;

    XAAPtr->Sync = CIRSync;

    moutb(0x3CE, 0x0E); /* enable writes to gr33 */
    moutb(0x3CF, 0x20); /* enable writes to gr33 */

    if(!XAAInit(pScreen, XAAPtr))
       return FALSE;

    return TRUE;
}
