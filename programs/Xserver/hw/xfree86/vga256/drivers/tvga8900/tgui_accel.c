/* $XFree86: xc/programs/Xserver/hw/xfree86/vga256/drivers/tvga8900/tgui_accel.c,v 3.0 1996/11/18 13:18:26 dawes Exp $ */

/*
 * Copyright 1996 by Alan Hourihane, Wigan, England.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Alan Hourihane not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Alan Hourihane makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * ALAN HOURIHANE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL ALAN HOURIHANE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 * Author:  Alan Hourihane, alanh@fairlite.demon.co.uk
 * 
 * Trident TGUI accelerated options.
 */

#include "vga256.h"
#include "xf86.h"
#include "vga.h"
#include "miline.h"
#include "compiler.h"

#include "xf86xaa.h"

extern unsigned char *tguiMMIOBase;
extern int TVGAchipset;
extern int TGUIRops[16];
extern int GE_OP;
#include "t89_driver.h"
#include "tgui_ger.h"
#include "tgui_mmio.h"

#ifdef TRIDENT_MMIO

#define MMIONAME(x)			x##MMIO

#define TGUISync			MMIONAME(TGUISync)
#define TGUIAccelInit			MMIONAME(TGUIAccelInit)
#define TGUISetupForFillRectSolid	MMIONAME(TGUISetupForFillRectSolid)
#define TGUISubsequentFillRectSolid	MMIONAME(TGUISubsequentFillRectSolid)
#define TGUISetupForScreenToScreenCopy	MMIONAME(TGUISetupForScreenToScreenCopy)
#define TGUISubsequentScreenToScreenCopy	MMIONAME(TGUISubsequentScreenToScreenCopy)
#define TGUISubsequentBresenhamLine		MMIONAME(TGUISubsequentBresenhamLine)
#define TGUISetupForCPUToScreenColorExpand	MMIONAME(TGUISetupForCPUToScreenColorExpand)
#define TGUISubsequentCPUToScreenColorExpand	MMIONAME(TGUISubsequentCPUToScreenColorExpand)
#define TGUISetupForScreenToScreenColorExpand	MMIONAME(TGUISetupForScreenToScreenColorExpand)
#define TGUISubsequentScreenToScreenColorExpand	MMIONAME(TGUISubsequentScreenToScreenColorExpand)
#define TGUISetupForFill8x8Pattern		MMIONAME(TGUISetupForFill8x8Pattern)
#define TGUISubsequentFill8x8Pattern		MMIONAME(TGUISubsequentFill8x8Pattern)

#endif

void TGUISync();
void TGUISetupForFillRectSolid();
void TGUISubsequentFillRectSolid();
void TGUISetupForScreenToScreenCopy();
void TGUISubsequentScreenToScreenCopy();
void TGUISubsequentBresenhamLine();
void TGUISetupForCPUToScreenColorExpand();
void TGUISubsequentCPUToScreenColorExpand();
void TGUISetupForScreenToScreenColorExpand();
void TGUISubsequentScreenToScreenColorExpand();
void TGUISetupForFill8x8Pattern();
void TGUISubsequentFill8x8Pattern();

/*
 * The following function sets up the supported acceleration. Call it
 * from the FbInit() function in the SVGA driver.
 */
void TGUIAccelInit() {

    xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS |
				HARDWARE_PATTERN_TRANSPARENCY |
				HARDWARE_PATTERN_ALIGN_64 |
				PIXMAP_CACHE;

    xf86AccelInfoRec.Sync = TGUISync;

#if 0
    xf86GCInfoRec.PolyFillRectSolidFlags = NO_PLANEMASK;

    xf86AccelInfoRec.SetupForFillRectSolid = TGUISetupForFillRectSolid;
    xf86AccelInfoRec.SubsequentFillRectSolid = TGUISubsequentFillRectSolid;

    /* Bresenham Lines */
    xf86AccelInfoRec.SubsequentBresenhamLine = TGUISubsequentBresenhamLine;
#endif

    xf86GCInfoRec.CopyAreaFlags = NO_PLANEMASK;

    xf86AccelInfoRec.SetupForScreenToScreenCopy =
       		TGUISetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
	       	TGUISubsequentScreenToScreenCopy;

    /* Fill 8x8 Pattern */
    xf86AccelInfoRec.SetupForFill8x8Pattern = TGUISetupForFill8x8Pattern;
    xf86AccelInfoRec.SubsequentFill8x8Pattern = TGUISubsequentFill8x8Pattern;

    /* Color Expansion */
    xf86AccelInfoRec.ColorExpandFlags = VIDEO_SOURCE_GRANULARITY_DWORD |
					BIT_ORDER_IN_BYTE_MSBFIRST |
					SCANLINE_PAD_DWORD |
					CPU_TRANSFER_PAD_DWORD;
#if 0
    xf86AccelInfoRec.SetupForCPUToScreenColorExpand = 
	TGUISetupForCPUToScreenColorExpand;
    xf86AccelInfoRec.SubsequentCPUToScreenColorExpand = 
	TGUISubsequentCPUToScreenColorExpand;
#endif
    xf86AccelInfoRec.SetupForScreenToScreenColorExpand = 
	TGUISetupForScreenToScreenColorExpand;
    xf86AccelInfoRec.SubsequentScreenToScreenColorExpand = 
	TGUISubsequentScreenToScreenColorExpand;

    xf86InitPixmapCache(
        &vga256InfoRec,
        vga256InfoRec.virtualY * vga256InfoRec.displayWidth *
            vga256InfoRec.bitsPerPixel / 8,
        vga256InfoRec.videoRam * 1024 - 1024
    );
}

/*
 * This is the implementation of the Sync() function.
 */
void TGUISync() {
	int count = 0, timeout = 0;
	int busy;

	for (;;) {
		BLTBUSY(busy);
		if (busy != GE_BUSY) return;
		count++;
		if (count == 10000000) {
			ErrorF("Trident: BitBLT engine time-out.\n");
			count = 9990000;
			timeout++;
			if (timeout == 8) {
				/* Reset BitBLT Engine */
				TGUI_STATUS(0x00);
				return;
			}
		}
	}
}

/*
 * This is the implementation of the SetupForFillRectSolid function
 * that sets up the coprocessor for a subsequent batch for solid
 * rectangle fills.
 */

void TGUISetupForFillRectSolid(color, rop, planemask)
    int color, rop;
    unsigned planemask;
{
	TGUI_FCOLOUR(color);
	TGUI_BCOLOUR(color);
	TGUI_FMIX(TGUIRops[rop]);
}
/*
 * This is the implementation of the SubsequentForFillRectSolid function
 * that sends commands to the coprocessor to fill a solid rectangle of
 * the specified location and size, with the parameters from the SetUp
 * call.
 */
void TGUISubsequentFillRectSolid(x, y, w, h)
    int x, y, w, h;
{
	TGUISync();
	TGUI_DRAWFLAG(SOLIDFILL | SRCMONO | SCR2SCR);
	TGUI_DIM_XY(w,h);
	TGUI_DEST_XY(x,y);
	TGUI_OPERMODE(GE_OP);
	TGUI_COMMAND(GE_BLT);
}

/*
 * This is the implementation of the SetupForScreenToScreenCopy function
 * that sets up the coprocessor for a subsequent batch for solid
 * screen-to-screen copies. Remember, we don't handle transparency,
 * so the transparency color is ignored.
 */
static int blitxdir, blitydir;
 
void TGUISetupForScreenToScreenCopy(xdir, ydir, rop, planemask,
transparency_color)
    int xdir, ydir;
    int rop;
    unsigned planemask;
    int transparency_color;
{
    int direction = 0;

    if (xdir < 0) direction |= XNEG;
    if (ydir < 0) direction |= YNEG;
    if (transparency_color != -1)
    {
	TGUI_FCOLOUR(transparency_color);
	direction |= TRANS_ENABLE;
    }
    TGUI_DRAWFLAG(direction | SCR2SCR);
    TGUI_FMIX(TGUIRops[rop]);
    blitxdir = xdir;
    blitydir = ydir;
}
/*
 * This is the implementation of the SubsequentForScreenToScreenCopy
 * that sends commands to the coprocessor to perform a screen-to-screen
 * copy of the specified areas, with the parameters from the SetUp call.
 * In this sample implementation, the direction must be taken into
 * account when calculating the addresses (with coordinates, it might be
 * a little easier).
 */
void TGUISubsequentScreenToScreenCopy(x1, y1, x2, y2, w, h)
    int x1, y1, x2, y2, w, h;
{
    if (blitydir < 0) {
        y1 = y1 + h - 1;
	y2 = y2 + h - 1;
    }
    if (blitxdir < 0) {
	x1 = x1 + w - 1;
	x2 = x2 + w - 1;
    }
    TGUISync();
    TGUI_SRC_XY(x1,y1);
    TGUI_DEST_XY(x2,y2);
    TGUI_DIM_XY(w,h);
    TGUI_OPERMODE(GE_OP);
    TGUI_COMMAND(GE_BLT);
}

void TGUISubsequentBresenhamLine(x1, y1, octant, err, e1, e2, length)
    int x1, y1, octant, err, e1, e2, length;
{
	int direction = 0;

	if (octant & YMAJOR)      direction |= YMAJ;
	if (octant & XDECREASING) direction |= XNEG;
	if (octant & YDECREASING) direction |= YNEG;
	TGUISync();
	TGUI_SRC_XY(e1,e2);
	TGUI_DEST_XY(x1,y1);
	TGUI_DIM_XY(err,length);
	TGUI_DRAWFLAG(STENCIL | SRCMONO | SCR2SCR | direction);
	TGUI_OPERMODE(GE_OP);
	TGUI_COMMAND(GE_BRESLINE);
}

void TGUISetupForCPUToScreenColorExpand(bg, fg, rop, planemask)
    int bg, fg, rop;
    unsigned planemask;
{
	int drawflag = 0;

	TGUISync();
	TGUI_FCOLOUR(fg);
        if (bg == -1)
		drawflag |= TRANS_ENABLE;
	else
		TGUI_BCOLOUR(bg);
	TGUI_DRAWFLAG(SRCMONO | drawflag);
	TGUI_FMIX(TGUIRops[rop]);
}

void TGUISubsequentCPUToScreenColorExpand(x, y, w, h, skipleft)
    int x, y, w, h, skipleft;
{
	TGUISync();
	TGUI_DEST_XY(x,y);
	TGUI_DIM_XY(w,h);
	TGUI_OPERMODE(GE_OP);
	TGUI_COMMAND(GE_BLT);
}

void TGUISetupForFill8x8Pattern(patternx, patterny, rop, planemask,
transparency_color)
    int patternx, patterny, rop;
    unsigned planemask;
    int transparency_color;
{
	int drawflag = 0;

	TGUISync();
	if (transparency_color != -1)
	{
		TGUI_FCOLOUR(transparency_color);
		drawflag |= TRANS_ENABLE;
	}
	TGUI_FMIX(TGUIRops[rop]); /* ROP */
	TGUI_DRAWFLAG(drawflag | PAT2SCR | SCR2SCR);
}

void TGUISubsequentFill8x8Pattern(patternx, patterny, x, y, w, h)
    int patternx, patterny;
    int x, y, w, h;
{
	TGUISync();
	TGUI_PATLOC(patterny * vga256InfoRec.displayWidth + patternx);
	TGUI_DEST_XY(x,y);
	TGUI_DIM_XY(w,h);
	TGUI_OPERMODE(GE_OP);
	TGUI_COMMAND(GE_BLT);
}

void TGUISetupForScreenToScreenColorExpand(bg, fg, rop, planemask)
    int bg, fg, rop;
    unsigned planemask;
{
	int drawflag = 0;

	TGUISync();
	TGUI_FCOLOUR(fg);
        if (bg == -1)
		drawflag |= TRANS_ENABLE;
	else
		TGUI_BCOLOUR(bg);
	TGUI_DRAWFLAG(SCR2SCR | SRCMONO | drawflag);
	TGUI_FMIX(TGUIRops[rop]);
}

void TGUISubsequentScreenToScreenColorExpand(srcx, srcy, x, y, w, h)
    int srcx, srcy, x, y, w, h;
{
	TGUISync();
	TGUI_SRC_XY(srcx,srcy);
	TGUI_DEST_XY(x,y);
	TGUI_DIM_XY(w,h);
	TGUI_OPERMODE(GE_OP);
	TGUI_COMMAND(GE_BLT);
}
