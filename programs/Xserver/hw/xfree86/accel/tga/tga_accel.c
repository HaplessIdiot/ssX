/*
 * Copyright 1996,1997 by Alan Hourihane, Wigan, England.
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
 * DEC TGA accelerated options.
 */

/* $XFree86: xc/programs/Xserver/hw/xfree86/accel/tga/tga_accel.c,v 3.1 1997/01/05 11:54:31 dawes Exp $ */

#include "cfb.h"
#include "xf86.h"
#include "miline.h"
#include "compiler.h"

#include "tga.h"
#include "tga_presets.h"
#include "tga_regs.h"
#include "xf86xaa.h"

void TGASync();
void TGASetupForFillRectSolid();
void TGASubsequentFillRectSolid();
void TGASetupForScreenToScreenCopy();
void TGASubsequentScreenToScreenCopy();

/*
 * The following function sets up the supported acceleration. Call it
 * from the FbInit() function in the SVGA driver.
 */
void TGAAccelInit() {

    xf86AccelInfoRec.Flags = BACKGROUND_OPERATIONS |
				PIXMAP_CACHE;

    xf86AccelInfoRec.Sync = TGASync;

#if 0
    xf86GCInfoRec.PolyFillRectSolidFlags = 0;

    xf86AccelInfoRec.SetupForFillRectSolid = TGASetupForFillRectSolid;
    xf86AccelInfoRec.SubsequentFillRectSolid = TGASubsequentFillRectSolid;

    xf86GCInfoRec.CopyAreaFlags = 0;

    xf86AccelInfoRec.SetupForScreenToScreenCopy =
       		TGASetupForScreenToScreenCopy;
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
	       	TGASubsequentScreenToScreenCopy;
#endif

    xf86AccelInfoRec.ServerInfoRec = &tgaInfoRec;

    xf86AccelInfoRec.PixmapCacheMemoryStart = tgaInfoRec.virtualY *
		tgaInfoRec.displayWidth * tgaInfoRec.bitsPerPixel / 8;

    xf86AccelInfoRec.PixmapCacheMemoryEnd = tgaInfoRec.videoRam * 1024-1024;
}

/*
 * This is the implementation of the Sync() function.
 */
void TGASync() {
	return;
	mb();
	while (TGA_READ_REG(TGA_CMD_STAT_REG) & 0x01);
}

/*
 * This is the implementation of the SetupForFillRectSolid function
 * that sets up the coprocessor for a subsequent batch for solid
 * rectangle fills.
 */

void TGASetupForFillRectSolid(color, rop, planemask)
    int color, rop;
    unsigned planemask;
{
	TGA_WRITE_REG(TGA_MODE_REG, BLOCKFILL | BPP8UNPACK | X11 | CAP_ENDS);
	TGA_WRITE_REG(TGA_RASTEROP_REG, BPP8UNPACK | rop);
	TGA_WRITE_REG(TGA_FOREGROUND_REG, color | (color << 8) | 
				(color << 16) | (color << 24));
	TGA_WRITE_REG(TGA_PLANEMASK_REG, planemask | (planemask << 8) |
				(planemask << 16) | (planemask << 24) );
}
/*
 * This is the implementation of the SubsequentForFillRectSolid function
 * that sends commands to the coprocessor to fill a solid rectangle of
 * the specified location and size, with the parameters from the SetUp
 * call.
 */
void TGASubsequentFillRectSolid(x, y, w, h)
    int x, y, w, h;
{
	TGA_WRITE_REG(TGA_ADDRESS_REG, y * tgaInfoRec.displayWidth + x);
}

/*
 * This is the implementation of the SetupForScreenToScreenCopy function
 * that sets up the coprocessor for a subsequent batch for solid
 * screen-to-screen copies. Remember, we don't handle transparency,
 * so the transparency color is ignored.
 */
static int blitxdir, blitydir;
 
void TGASetupForScreenToScreenCopy(xdir, ydir, rop, planemask,
transparency_color)
    int xdir, ydir;
    int rop;
    unsigned planemask;
    int transparency_color;
{
    int direction = 0;

#if 0
    if (xdir < 0) direction |= XNEG;
    if (ydir < 0) direction |= YNEG;
GUI STUFF
    blitxdir = xdir;
    blitydir = ydir;
#endif
}
/*
 * This is the implementation of the SubsequentForScreenToScreenCopy
 * that sends commands to the coprocessor to perform a screen-to-screen
 * copy of the specified areas, with the parameters from the SetUp call.
 * In this sample implementation, the direction must be taken into
 * account when calculating the addresses (with coordinates, it might be
 * a little easier).
 */
void TGASubsequentScreenToScreenCopy(x1, y1, x2, y2, w, h)
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
}
