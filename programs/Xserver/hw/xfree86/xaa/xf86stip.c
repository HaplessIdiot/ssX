/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xf86stip.c,v 3.4 1997/05/21 15:17:12 dawes Exp $ */

/*
 * Copyright 1996  The XFree86 Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * HARM HANEMAAYER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Written by Harm Hanemaayer (H.Hanemaayer@inter.nl.net).
 */

#include	"X.h"
#include	"Xmd.h"
#include	"Xproto.h"
/* PSZ doesn't matter. */
#define PSZ 8
#include	"cfb.h"
#include	"gcstruct.h"
#include	"windowstr.h"
#include	"scrnintstr.h"
#include	"pixmapstr.h"
#include	"regionstr.h"

#include "xf86.h"
#include "xf86xaa.h"
#include "xf86local.h"
#include "xf86expblt.h"

extern unsigned char byte_reversed[256];

static void FillStippledCPUToScreenColorExpand(
#if NeedFunctionPrototypes
    int x,
    int y,
    int w,
    int h,
    unsigned char *src,
    int srcwidth,
    int stipplewidth,
    int stippleheight,
    int srcx,
    int srcy,
    int bg,
    int fg,
    int rop,
    unsigned int planemask
#endif
);

static void FillStippledScreenToScreenColorExpand(
#if NeedFunctionPrototypes
    int x,
    int y,
    int w,
    int h,
    unsigned char *src,
    int srcwidth,
    int stipplewidth,
    int stippleheight,
    int srcx,
    int srcy,
    int bg,
    int fg,
    int rop,
    unsigned int planemask
#endif
);


static void xf86WriteStippleLeftEdge(x, y, w, h, src, srcwidth,
srcx, srcy, stipplewidth, stippleheight)
    int x, y, w, h;
    unsigned char *src;
    int srcwidth, srcx, srcy;
    int stipplewidth, stippleheight;
{
    int destaddr, i, shift;
    unsigned long dworddata;
    unsigned char *srcp;
    unsigned char *base;
    int bytecount;
    int srcyp;

    xf86AccelInfoRec.SubsequentCPUToScreenColorExpand(x, y, w, h, 0);

    base = (unsigned char *)xf86AccelInfoRec.CPUToScreenColorExpandBase;

    /* Calculate pointer to origin in stipple. */
    srcp = srcwidth * srcy + (srcx >> 3) + src;
    shift = (srcx & 7);
    srcyp = srcy;

    if (xf86AccelInfoRec.ColorExpandFlags & SCANLINE_PAD_BYTE) {
        bytecount = 0;
        for (i = 0; i < h; i++) {
	    if (xf86AccelInfoRec.ColorExpandFlags & BIT_ORDER_IN_BYTE_MSBFIRST)
		dworddata = (dworddata << 8) || 
		  (byte_reversed[(*srcp >> shift) & 0xFF]);
	    else
		dworddata = (dworddata << 8) || (*srcp >> shift);
	    srcyp++;
	    srcp += srcwidth;
	    if (srcyp >= stippleheight) {
	        srcyp = 0;
	        srcp = src + (srcx >> 3);
	    }
	    bytecount++;
	    if (bytecount == 4) {
 	        *(unsigned int *)base = dworddata;
 	        dworddata = 0;
	        if (!(xf86AccelInfoRec.ColorExpandFlags & CPU_TRANSFER_BASE_FIXED)) {
	            base += 4;
                    if (base >= (unsigned char *)
                    xf86AccelInfoRec.CPUToScreenColorExpandEndMarker)
                        base = (unsigned char *)
                            xf86AccelInfoRec.CPUToScreenColorExpandBase;
                }
            }
	}
	if (bytecount) {
	    *(unsigned int *)base = dworddata;
	    if (!(xf86AccelInfoRec.ColorExpandFlags & CPU_TRANSFER_BASE_FIXED))
	        base += 4;
	}
        if (xf86AccelInfoRec.ColorExpandFlags & CPU_TRANSFER_PAD_QWORD)
	    if ((unsigned long)((unsigned long)base -
	    (unsigned long)xf86AccelInfoRec.CPUToScreenColorExpandBase) & 0x4)
	        *(unsigned int *)base = 0;
    }
    else {
        /* DWORD padding of scanlines. */
        for (i = 0; i < h; i++) {
            if (xf86AccelInfoRec.ColorExpandFlags & TRIPLE_BITS_24BPP)
                if (xf86AccelInfoRec.ColorExpandFlags & BIT_ORDER_IN_BYTE_MSBFIRST)
	            dworddata = (byte_reversed_expand3[(*srcp >> shift) & 0xFF]);
	        else
	            dworddata = byte_expand3[(*srcp >> shift) & 0xFF];
	    else
                if (xf86AccelInfoRec.ColorExpandFlags & BIT_ORDER_IN_BYTE_MSBFIRST)
	            dworddata = (byte_reversed[(*srcp >> shift) & 0xFF]);
	        else
	            dworddata = *srcp >> shift;
	    srcyp++;
	    srcp += srcwidth;
	    if (srcyp >= stippleheight) {
	        srcyp = 0;
	        srcp = src + (srcx >> 3);
	    }
 	    *(unsigned int *)base = dworddata;
	    if (!(xf86AccelInfoRec.ColorExpandFlags & CPU_TRANSFER_BASE_FIXED)) {
	        base += 4;
                if (base >= (unsigned char *)
                xf86AccelInfoRec.CPUToScreenColorExpandEndMarker)
                    base = (unsigned char *)
                        xf86AccelInfoRec.CPUToScreenColorExpandBase;
            }
	}
        if (xf86AccelInfoRec.ColorExpandFlags & CPU_TRANSFER_PAD_QWORD)
            if (h & 0x1)
                *(unsigned int *)base = 0;
    }
}

void
xf86FillRectStippledCPUToScreenColorExpand(pDrawable, pGC, nBoxInit, pBoxInit)
    DrawablePtr pDrawable;
    register GCPtr pGC;
    int nBoxInit;		       /* number of rectangles to fill */
    BoxPtr pBoxInit;		       /* Pointer to first rectangle to fill */
{
    PixmapPtr pPixmap;		       /* Pixmap of the area to draw */
    register int rectX1, rectX2;       /* points used to find the width */
    register int rectY1, rectY2;       /* points used to find the height */
    register int rectWidth;	       /* Width of the rect to be drawn */
    register int rectHeight;	       /* Height of the rect to be drawn */
    register BoxPtr pBox;	       /* current rectangle to fill */
    register int nBox;		       /* Number of rectangles to fill */
    int xoffset, yoffset;

    pPixmap = pGC->stipple;

    for (nBox = nBoxInit, pBox = pBoxInit; nBox > 0; nBox--, pBox++) {
	rectX1 = pBox->x1;
	rectY1 = pBox->y1;
	rectX2 = pBox->x2;
	rectY2 = pBox->y2;

	rectWidth = rectX2 - rectX1;
	rectHeight = rectY2 - rectY1;

	if ((rectWidth > 0) && (rectHeight > 0)) {
	    xoffset = (rectX1 - (pGC->patOrg.x + pDrawable->x))
	        % pPixmap->drawable.width;
	    if (xoffset < 0)
	        xoffset += pPixmap->drawable.width;
	    yoffset = (rectY1 - (pGC->patOrg.y + pDrawable->y))
	        % pPixmap->drawable.height;
	    if (yoffset < 0)
	        yoffset += pPixmap->drawable.height;
	    FillStippledCPUToScreenColorExpand(
	        rectX1, rectY1, rectWidth, rectHeight,
	        pPixmap->devPrivate.ptr, pPixmap->devKind,
	        pPixmap->drawable.width, pPixmap->drawable.height,
	        xoffset, yoffset,
	        (pGC->fillStyle == FillStippled) ? -1 : pGC->bgPixel,
	        pGC->fgPixel, pGC->alu, pGC->planemask
	        );
	}
    }	/* end for loop through each rectangle to draw */
    return;
}

void
xf86FillRectStippledScreenToScreenColorExpand(pDrawable, pGC, nBoxInit, pBoxInit)
    DrawablePtr pDrawable;
    register GCPtr pGC;
    int nBoxInit;		       /* number of rectangles to fill */
    BoxPtr pBoxInit;		       /* Pointer to first rectangle to fill */
{
    PixmapPtr pPixmap;		       /* Pixmap of the area to draw */
    register int rectX1, rectX2;       /* points used to find the width */
    register int rectY1, rectY2;       /* points used to find the height */
    register int rectWidth;	       /* Width of the rect to be drawn */
    register int rectHeight;	       /* Height of the rect to be drawn */
    register BoxPtr pBox;	       /* current rectangle to fill */
    register int nBox;		       /* Number of rectangles to fill */
    int xoffset, yoffset;

    pPixmap = pGC->stipple;

    for (nBox = nBoxInit, pBox = pBoxInit; nBox > 0; nBox--, pBox++) {
	rectX1 = pBox->x1;
	rectY1 = pBox->y1;
	rectX2 = pBox->x2;
	rectY2 = pBox->y2;

	rectWidth = rectX2 - rectX1;
	rectHeight = rectY2 - rectY1;

	if ((rectWidth > 0) && (rectHeight > 0)) {
	    xoffset = (rectX1 - (pGC->patOrg.x + pDrawable->x))
	        % pPixmap->drawable.width;
	    if (xoffset < 0)
	        xoffset += pPixmap->drawable.width;
	    yoffset = (rectY1 - (pGC->patOrg.y + pDrawable->y))
	        % pPixmap->drawable.height;
	    if (yoffset < 0)
	        yoffset += pPixmap->drawable.height;
	    FillStippledScreenToScreenColorExpand(
	        rectX1, rectY1, rectWidth, rectHeight,
	        pPixmap->devPrivate.ptr, pPixmap->devKind,
	        pPixmap->drawable.width, pPixmap->drawable.height,
	        xoffset, yoffset,
	        (pGC->fillStyle == FillStippled) ? -1 : pGC->bgPixel,
	        pGC->fgPixel, pGC->alu, pGC->planemask
	        );
	}
    }	/* end for loop through each rectangle to draw */
    return;
}

static void
FillStippledCPUToScreenColorExpand(x, y, w, h, src, srcwidth,
stipplewidth, stippleheight, srcx, srcy, bg, fg, rop, planemask)
    int x, y, w, h;
    unsigned char *src;
    int srcwidth;
    int stipplewidth, stippleheight;
    int srcx, srcy;
    int bg, fg;
    int rop;
    unsigned int planemask;
{
    int skipleft;
    unsigned char *srcp;
    unsigned char *base;
    unsigned int *(*DrawStippleScanlineFunc)(
#if NeedNestedPrototypes
	unsigned int *base,
	unsigned char *src,	       /* Pointer to stipple bitmap. */
	int srcwidth,		       /* Width of stipple bitmap in bytes. */
	int stipplewidth,	       /* Width of stipple in pixels. */
	int srcoffset,		       /* The offset in bytes into the stipple */
    /* of the first pixel. */
	int w			       /* Width of scanline in pixels. */
#endif
    );

    if (xf86AccelInfoRec.ColorExpandFlags & ONLY_TRANSPARENCY_SUPPORTED) {
        if (bg != -1) {
	    /* First fill-in the background. */
	    xf86AccelInfoRec.SetupForFillRectSolid(bg, rop, planemask);
	    xf86AccelInfoRec.SubsequentFillRectSolid(x, y, w, h);
	    if (xf86AccelInfoRec.Flags & BACKGROUND_OPERATIONS)
	      xf86AccelInfoRec.Sync();
	}
	xf86AccelInfoRec.SetupForCPUToScreenColorExpand(
	    -1, fg, rop, planemask);
    } else
	xf86AccelInfoRec.SetupForCPUToScreenColorExpand(
	    bg, fg, rop, planemask);

    if (xf86AccelInfoRec.ColorExpandFlags & BIT_ORDER_IN_BYTE_MSBFIRST)
	if (xf86AccelInfoRec.ColorExpandFlags & TRIPLE_BITS_24BPP)
	    if (xf86AccelInfoRec.ColorExpandFlags & CPU_TRANSFER_BASE_FIXED)
		DrawStippleScanlineFunc = xf86DrawStippleScanline3MSBFirstFixedBase;
	    else
	    	DrawStippleScanlineFunc = xf86DrawStippleScanline3MSBFirst;
	else
	    if (xf86AccelInfoRec.ColorExpandFlags & CPU_TRANSFER_BASE_FIXED)
		DrawStippleScanlineFunc = xf86DrawStippleScanlineMSBFirstFixedBase;
	    else
	    	DrawStippleScanlineFunc = xf86DrawStippleScanlineMSBFirst;
     else
	if (xf86AccelInfoRec.ColorExpandFlags & TRIPLE_BITS_24BPP)
	    if (xf86AccelInfoRec.ColorExpandFlags & CPU_TRANSFER_BASE_FIXED)
		DrawStippleScanlineFunc = xf86DrawStippleScanline3FixedBase;
	    else
		DrawStippleScanlineFunc = xf86DrawStippleScanline3;
	else
	    if (xf86AccelInfoRec.ColorExpandFlags & CPU_TRANSFER_BASE_FIXED)
		DrawStippleScanlineFunc = xf86DrawStippleScanlineFixedBase;
	    else
		DrawStippleScanlineFunc = xf86DrawStippleScanline;

    if ((xf86AccelInfoRec.ColorExpandFlags & LEFT_EDGE_CLIPPING)
    && ((xf86AccelInfoRec.ColorExpandFlags & LEFT_EDGE_CLIPPING_NEGATIVE_X)
    /*
     * When LEFT_EDGE_CLIPPING_NEGATIVE_X is not defined, we can only
     * only allow this if the x-coordinate remains on-screen.
     */
    || (x - (srcx & 7) >= 0))) {
	skipleft = (srcx & 7);
	srcx = srcx & (~7);    /* Aligned. */
	x -= skipleft;
	w += skipleft;
    }
    else {
        /* Handle the left edge (skipleft pixels wide). */ 
        skipleft = 8 - (srcx & 7);
        if (skipleft == 8)
            skipleft = 0;
        if (skipleft) {
	    /* Draw left edge. */
	    int done;
	    done = 0;
	    if (skipleft > (stipplewidth - srcx))
	        skipleft = stipplewidth - srcx;
	    if (skipleft >= w) {
	        skipleft = w;
	        done = 1;
	    }
	    xf86WriteStippleLeftEdge(x, y, skipleft, h, src, srcwidth,
	        srcx, srcy, stipplewidth, stippleheight);
	    if (done) {
		SET_SYNC_FLAG;
	        return;
	    }
	    if (!(xf86AccelInfoRec.Flags & NO_SYNC_AFTER_CPU_COLOR_EXPAND))
	        xf86AccelInfoRec.Sync();
	}
        x += skipleft;
        srcx += skipleft;
	if (srcx == stipplewidth) srcx = 0;
        w -= skipleft;
        skipleft = 0;
    }

    xf86AccelInfoRec.SubsequentCPUToScreenColorExpand(x, y, w, h, skipleft);

    base = (unsigned char *)xf86AccelInfoRec.CPUToScreenColorExpandBase;

    /* Calculate pointer to scanline in bitmap. */
    srcp = srcwidth * srcy + src;
 
    if (xf86AccelInfoRec.ColorExpandFlags & SCANLINE_PAD_DWORD) {
	int i;
	for (i = 0; i < h; i++) {
	    if (xf86AccelInfoRec.ColorExpandFlags & CPU_TRANSFER_BASE_FIXED)
	        (*DrawStippleScanlineFunc)(
	            (unsigned int *)base, srcp,
	            srcwidth, stipplewidth, srcx / 8,
	            w);
	    else {
	        base = (unsigned char *)(*DrawStippleScanlineFunc)(
	            (unsigned int *)base, srcp,
	            srcwidth, stipplewidth, srcx / 8,
	            w);
		if (base >= (unsigned char *)
		    xf86AccelInfoRec.CPUToScreenColorExpandEndMarker)
		base = (unsigned char *)
		    xf86AccelInfoRec.CPUToScreenColorExpandBase;
	    }
	    srcy++;
	    srcp += srcwidth;
	    if (srcy >= stippleheight) {
	        srcy = 0;
	        srcp = src;
	    }
	}
        if (xf86AccelInfoRec.ColorExpandFlags & CPU_TRANSFER_PAD_QWORD) {
            if (xf86AccelInfoRec.ColorExpandFlags & TRIPLE_BITS_24BPP) {
                if (((((w * 3 + 31) & ~31) >> 5) * h) & 0x1)
                    *(unsigned int *)base = 0;
            }
	    else
               if (((((w + 31) & ~31) >> 5) * h) & 0x1)
	            *(unsigned int *)base = 0;
	}
    }

    xf86AccelInfoRec.Sync();
}

/*
 * This function uses two or more buffers in video memory.
 */

static void
FillStippledScreenToScreenColorExpand(x, y, w, h, src, srcwidth,
stipplewidth, stippleheight, srcx, srcy, bg, fg, rop, planemask)
    int x, y, w, h;
    unsigned char *src;
    int srcwidth;
    int stipplewidth, stippleheight;
    int srcx, srcy;
    int bg, fg;
    int rop;
    unsigned int planemask;
{
    int bytewidth;		       /* Area width in bytes. */
    int bitmapwidth;
    unsigned char *srcp;
    unsigned char *base;
    int offset, endoffset;
    int i;
    unsigned int *(*DrawStippleScanlineFunc)(
#if NeedNestedPrototypes
	unsigned int *base,
	unsigned char *src,	       /* Pointer to stipple bitmap. */
	int srcwidth,		       /* Width of stipple bitmap in bytes. */
	int stipplewidth,	       /* Width of stipple in pixels. */
	int srcoffset,		       /* The offset in bytes into the stipple */
				       /* of the first pixel. */
	int w			       /* Width of scanline in pixels. */
#endif
    );

    if (xf86AccelInfoRec.ColorExpandFlags & ONLY_TRANSPARENCY_SUPPORTED) {
        if (bg != -1) {
	    /* First fill-in the background. */
	    xf86AccelInfoRec.SetupForFillRectSolid(bg, rop, planemask);
	    xf86AccelInfoRec.SubsequentFillRectSolid(x, y, w, h);
	    if (xf86AccelInfoRec.Flags & BACKGROUND_OPERATIONS)
	      xf86AccelInfoRec.Sync();
	}
        xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand(
            x, y, w, h, -1, fg, rop, planemask);
    }
    else
        xf86AccelInfoRec.SetupForScanlineScreenToScreenColorExpand(
            x, y, w, h, bg, fg, rop, planemask);

    if (xf86AccelInfoRec.ColorExpandFlags & BIT_ORDER_IN_BYTE_MSBFIRST)
        if (xf86AccelInfoRec.ColorExpandFlags & TRIPLE_BITS_24BPP)
	    DrawStippleScanlineFunc = xf86DrawStippleScanline3MSBFirst;
	else
	    DrawStippleScanlineFunc = xf86DrawStippleScanlineMSBFirst;
    else
        if (xf86AccelInfoRec.ColorExpandFlags & TRIPLE_BITS_24BPP)
	    DrawStippleScanlineFunc = xf86DrawStippleScanline3;
	else
	    DrawStippleScanlineFunc = xf86DrawStippleScanline;

    /* Calculate pointer to scanline in bitmap. */
    srcp = srcwidth * srcy + src;
 
    /* Be careful about the offset into the leftmost source byte. */
    if ((srcx & 7) != 0)
        w += (srcx & 7);
    /* Number of stipple bytes to be written for each bitmap scanline. */
    bytewidth = (w + 7) / 8;
    /* Calculate the non-expanded bitmap width rounded up to 32-bit words, */
    /* in units of pixels. */
    bitmapwidth = ((bytewidth + 3) / 4) * 32;

    endoffset = (bitmapwidth / 8) * xf86AccelInfoRec.PingPongBuffers;
    offset = 0;

    for (i = 0; i < h; i++) {
	if (!(xf86AccelInfoRec.Flags & COP_FRAMEBUFFER_CONCURRENCY))
	    xf86AccelInfoRec.Sync();
	(*DrawStippleScanlineFunc)((unsigned int *)
	    (xf86AccelInfoRec.ScratchBufferBase + offset),
	    srcp, srcwidth, stipplewidth, srcx / 8, w);
	xf86AccelInfoRec.SubsequentScanlineScreenToScreenColorExpand(
	    (xf86AccelInfoRec.ScratchBufferAddr + offset) * 8 + (srcx & 7));
        srcy++;
	srcp += srcwidth;
	if (srcy >= stippleheight) {
	    srcy = 0;
	    srcp = src;
	}
        /*
         * There is a number of buffers -- while the first one is being
         * blitted, the next one is initialized, and then the next,
         * and so on.
         */
	offset += bitmapwidth / 8;
	if (offset == endoffset)
	    offset = 0;
    }

    if (xf86AccelInfoRec.Flags & BACKGROUND_OPERATIONS)
        SET_SYNC_FLAG;
}
