/* $XFree86$ */

/*
 *	Copyright 2000	Ani Joshi <ajoshi@unixbox.com>
 *
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation and
 * that the name of Ani Joshi not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Ani Joshi makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * ANI JOSHI DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ANI JOSHI BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */


#include "Xarch.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"
#include "xaa.h"
#include "xf86PciInfo.h"

#include "imstt.h"
#include "imstt_reg.h"



static void IMSTTSync(ScrnInfoPtr pScrn)
{
	IMSTTPtr iptr = IMSTTPTR(pScrn);
	IMSTTMMIO_VARS();

	while(INREG(IMSTT_SSTATUS) & 0x80);
	while(INREG(IMSTT_SSTATUS) & 0x40);
	return;
}


static void IMSTTSetupForSolidFill(ScrnInfoPtr pScrn, int color,
				   int rop, unsigned int planemask)
{
	IMSTTPtr iptr = IMSTTPTR(pScrn);
	IMSTTMMIO_VARS();

	OUTREG(IMSTT_BI, 0xffffffff);
	OUTREG(IMSTT_MBC, 0xffffffff);
	OUTREG(IMSTT_CLR, color);
	OUTREG(IMSTT_DP_OCTL, (pScrn->displayWidth * (pScrn->bitsPerPixel >> 3)));
}


static void IMSTTSubsequentSolidFillRect(ScrnInfoPtr pScrn,
					 int x, int y, int w, int h)
{
	IMSTTPtr iptr = IMSTTPTR(pScrn);
	IMSTTMMIO_VARS();

	OUTREG(IMSTT_DSA, y + x);
	OUTREG(IMSTT_CNT, (h << 16) | w);
	OUTREG(IMSTT_BLTCTL, 0x840);
}


static void IMSTTSetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir,
					    int ydir, int rop, unsigned int planemask,
					    int trans_color)
{
	IMSTTPtr iptr = IMSTTPTR(pScrn);
	IMSTTMMIO_VARS();
	unsigned long sp, dp, ll, cnt;

	iptr->bltctl = 0x05;
	ll = pScrn->displayWidth * (pScrn->bitsPerPixel >> 3);
	iptr->ll = ll;
	sp = ll << 16;
	cnt = pScrn->virtualY << 16;
	if (xdir >= 0) {
		dp = -(ll) & 0xffff;
		sp |= dp;
	}
	if (ydir >= 0) {
		iptr->bltctl |= 0x80;
		cnt |= -(pScrn->displayWidth) & 0xffff;
	} else {
		cnt |= pScrn->displayWidth;
	}

	OUTREG(IMSTT_SP, sp);
	OUTREG(IMSTT_CNT, cnt);
	OUTREG(IMSTT_DP_OCTL, dp);
}


static void IMSTTSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
					      int x1, int y1,
					      int x2, int y2,
					      int w, int h)
{
	IMSTTPtr iptr = IMSTTPTR(pScrn);
	IMSTTMMIO_VARS();

	OUTREG(IMSTT_S1SA, x1 * iptr->ll * y1);
	OUTREG(IMSTT_DSA, x2 * iptr->ll * y2);
	OUTREG(IMSTT_BLTCTL, iptr->bltctl);
}




Bool IMSTTAccelInit(ScreenPtr pScreen)
{
	ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
	IMSTTPtr iptr = IMSTTPTR(pScrn);
	XAAInfoRecPtr xaaptr;

	if (!(xaaptr = iptr->AccelInfoRec = XAACreateInfoRec()))
		return FALSE;

	xaaptr->Flags = (PIXMAP_CACHE | OFFSCREEN_PIXMAPS | LINEAR_FRAMEBUFFER);

	xaaptr->Sync = IMSTTSync;
	xaaptr->SetupForSolidFill = IMSTTSetupForSolidFill;
	xaaptr->SubsequentSolidFillRect = IMSTTSubsequentSolidFillRect;
	xaaptr->ScreenToScreenCopyFlags = 0;
	xaaptr->SetupForScreenToScreenCopy = IMSTTSetupForScreenToScreenCopy;
	xaaptr->SubsequentScreenToScreenCopy = IMSTTSubsequentScreenToScreenCopy;

	return XAAInit(pScreen, xaaptr);
}

