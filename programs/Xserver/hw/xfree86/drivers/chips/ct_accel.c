/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/chips/ct_accel.c,v 1.21 1998/07/25 16:55:38 dawes Exp $ */
/*
 * Copyright 1996, 1997, 1998 by David Bateman <dbateman@ee.uts.edu.au>
 *   Modified 1997, 1998 by Nozomi Ytow
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the authors not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  The authors makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * THE AUTHORS DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE AUTHORS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#define CHIPS_SCREEN2SCREEN
#define CHIPS_SOLIDFILLRECT
#define CHIPS_COLOREXPANDFILL
#define CHIPS_PATTERNFILL
#define CHIPS_WRITEPIXMAP


/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "compiler.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that use XAA need this */
#include "xf86fbman.h"

/* The vga HW register stuff. Do we need this? */
#include "vgaHW.h"

/* Our driver specific include file */
#include "ct_driver.h"

#if (defined(__STDC__) && !defined(UNIXCPP)) || defined(ANSICPP)
#define CATNAME(prefix,subname) prefix##subname
#else
#define CATNAME(prefix,subname) prefix/**/subname
#endif

#ifdef CHIPS_MMIO
#ifdef CHIPS_HIQV
#include "ct_BltHiQV.h"
#define CTNAME(subname) CATNAME(CHIPSHiQV,subname)
#else
#include "ct_BlitMM.h"
#define CTNAME(subname) CATNAME(CHIPSMMIO,subname)
#endif
#else
#include "ct_Blitter.h"
#define CTNAME(subname) CATNAME(CHIPS,subname)
#endif


static void CTNAME(8SetupForSolidFill)(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void CTNAME(16SetupForSolidFill)(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void CTNAME(24SetupForSolidFill)(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void CTNAME(SubsequentSolidFillRect)(ScrnInfoPtr pScrn,
					int x, int y, int w, int h);
#ifndef CHIPS_HIQV
static void CTNAME(24SubsequentSolidFillRect)(ScrnInfoPtr pScrn,
					int x, int y, int w, int h);
#else
static void CTNAME(32SetupForSolidFill)(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void CTNAME(32SubsequentSolidFillRect)(ScrnInfoPtr pScrn,
					int x, int y, int w, int h);
#endif
static void CTNAME(SetupForScreenToScreenCopy)(ScrnInfoPtr pScrn, int xdir,
				int ydir, int rop, unsigned int planemask,
				int trans);
static void CTNAME(SubsequentScreenToScreenCopy)(ScrnInfoPtr pScrn,
				int srcX, int srcY, int dstX, int dstY,
				int w, int h);
static void CTNAME(SetupForCPUToScreenColorExpandFill)(ScrnInfoPtr pScrn, int fg,
				int bg, int rop, unsigned int planemask);
static void CTNAME(SubsequentCPUToScreenColorExpandFill)(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft);
static void CTNAME(SetupForMono8x8PatternFill)(ScrnInfoPtr pScrn,
				int patx, int paty, int fg, int bg,
				int rop, unsigned int planemask);
static void CTNAME(SubsequentMono8x8PatternFillRect)(ScrnInfoPtr pScrn,
				int patx, int paty,
				int x, int y, int w, int h );
static void CTNAME(SetupForColor8x8PatternFill)(ScrnInfoPtr pScrn,
				int patx, int paty, int rop,
				unsigned int planemask, int trans);
static void CTNAME(SubsequentColor8x8PatternFillRect)(ScrnInfoPtr pScrn,
				int patx, int paty,
				int x, int y, int w, int h );
#ifndef CHIPS_HIQV
static void CTNAME(SetupForImageWrite)(ScrnInfoPtr pScrn, int rop,
   				unsigned int planemask,
				int transparency_color, int bpp, int depth);
static void CTNAME(SubsequentImageWriteRect)(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft);
#else
static void  CTNAME(WritePixmap)(ScrnInfoPtr pScrn, int x, int y, int w, int h,
		unsigned char *src, int srcwidth, int rop,
		unsigned int planemask, int trans, int bpp, int depth);

#endif


/* Define a Macro to replicate a planemask 64 times and write to address
 * allocated for planemask pattern */
#define ctWRITEPLANEMASK(mask,addr) { \
    switch  (pScrn->bitsPerPixel) { \
    case 8: \
        if (cAcl->planemask != (mask&0xFF)) { \
            cAcl->planemask = (mask&0xFF); \
	    memset((unsigned char *)cPtr->FbBase + addr, (mask&0xFF), 64); \
	} \
	break; \
    case 16: \
        if (cAcl->planemask != (mask&0xFFFF)) { \
            cAcl->planemask = (mask&0xFFFF); \
	    {   int i; \
	        for (i = 0; i < 64; i++) { \
		    memcpy((unsigned char *)cPtr->FbBase + addr \
			   + i * 2, &mask, 2); \
	        } \
	    } \
	} \
	break; \
    } \
}				   

Bool 
CTNAME(AccelInit)(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    BoxRec AvailFBArea;

    cPtr->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if(!infoPtr) return FALSE;

    /*
     * Setup some global variables
     */
    cAcl->BytesPerPixel = pScrn->bitsPerPixel >> 3;
    cAcl->PitchInBytes = pScrn->displayWidth * cAcl->BytesPerPixel;

    /*
     * Set up the main acceleration flags.
     */
    infoPtr->Flags = PIXMAP_CACHE;

    /*
     * The following line installs a "Sync" function, that waits for
     * all coprocessor operations to complete.
     */
    infoPtr->Sync = CTNAME(Sync);

    /* 
     * Setup a Screen to Screen copy (BitBLT) primitive
     */  
#ifndef CHIPS_HIQV
    infoPtr->ScreenToScreenCopyFlags = NO_TRANSPARENCY;
    if (pScrn->bitsPerPixel == 24)
	infoPtr->ScreenToScreenCopyFlags |= NO_PLANEMASK;
#else
    infoPtr->ScreenToScreenCopyFlags = 0;
    if ((pScrn->bitsPerPixel == 24) || (pScrn->bitsPerPixel == 32))
	infoPtr->ScreenToScreenCopyFlags |= NO_PLANEMASK;

    /* A Chips and Technologies application notes says that some
     * 65550 have a bug that prevents 16bpp transparency. It probably
     * applies to 24 bpp as well (Someone with a 65550 care to check?).
     * Selection of this controlled in Probe.
     */
    if (!(cPtr->Flags & ChipsColorTransparency))
	infoPtr->ScreenToScreenCopyFlags |= NO_TRANSPARENCY;
#endif

#ifdef CHIPS_SCREEN2SCREEN
    infoPtr->SetupForScreenToScreenCopy = CTNAME(SetupForScreenToScreenCopy);
    infoPtr->SubsequentScreenToScreenCopy =
		CTNAME(SubsequentScreenToScreenCopy);
#endif

    /*
     * Install the low-level functions for drawing solid filled rectangles.
     */
    infoPtr->SolidFillFlags |= NO_PLANEMASK;
#ifdef CHIPS_SOLIDFILLRECT
    switch (pScrn->bitsPerPixel) {
    case 8 :
	infoPtr->SetupForSolidFill = CTNAME(8SetupForSolidFill);
	infoPtr->SubsequentSolidFillRect = CTNAME(SubsequentSolidFillRect);
        break;
    case 16 :
	infoPtr->SetupForSolidFill = CTNAME(16SetupForSolidFill);
	infoPtr->SubsequentSolidFillRect = CTNAME(SubsequentSolidFillRect);
        break;
    case 24 :
	infoPtr->SetupForSolidFill = CTNAME(24SetupForSolidFill);
#ifdef CHIPS_HIQV
	infoPtr->SubsequentSolidFillRect = CTNAME(SubsequentSolidFillRect);
#else
	/*
	 * The version of this function here uses three different
	 * algorithms in an attempt to maximise performance. One
	 * for RGB_EQUAL, another for !RGB_EQUAL && GXCOPY_ONLY
	 * and yet another for !RGB_EQUAL && !GXCOPY_ONLY. The
	 * first two versions use the 8bpp engine for the fill,
	 * whilst the second uses a framebuffer routine to create
	 * one scanline of the fill in off screen memory which is
	 * then used by a CopyArea function with a complex ROP.
	 */
	infoPtr->SubsequentSolidFillRect = CTNAME(24SubsequentSolidFillRect);
        if (cAcl->ScratchAddress < 0)
	    infoPtr->ScreenToScreenCopyFlags |= GXCOPY_ONLY;
#endif
        break;
#ifdef CHIPS_HIQV
    case 32:
        if (cAcl->ScratchAddress > 0) {
	    infoPtr->SetupForSolidFill = CTNAME(32SetupForSolidFill);
	    infoPtr->SubsequentSolidFillRect =
		CTNAME(32SubsequentSolidFillRect);
	}
        break;
#endif
    }
#endif  /* CHIPS_SOLIDFILLRECT*/

#ifdef CHIPS_HIQV
    /* At 32bpp we can't use the other acceleration */
    if (pScrn->bitsPerPixel == 32) goto chips_imagewrite;
#endif

    /*
     * Setup the functions that perform monochrome colour expansion
     */

#ifdef CHIPS_HIQV 
    infoPtr->CPUToScreenColorExpandFillFlags =
	BIT_ORDER_IN_BYTE_MSBFIRST | CPU_TRANSFER_PAD_QWORD |
	LEFT_EDGE_CLIPPING | LEFT_EDGE_CLIPPING_NEGATIVE_X;
        
    if (pScrn->bitsPerPixel == 24) 
	infoPtr->CPUToScreenColorExpandFillFlags |= NO_PLANEMASK;
#else
    infoPtr->CPUToScreenColorExpandFillFlags =
	BIT_ORDER_IN_BYTE_MSBFIRST | CPU_TRANSFER_PAD_DWORD;

    if (pScrn->bitsPerPixel == 24) 
	infoPtr->CPUToScreenColorExpandFillFlags |= TRIPLE_BITS_24BPP |
	    RGB_EQUAL | NO_PLANEMASK;
#endif

#ifdef CHIPS_COLOREXPANDFILL
    infoPtr->SetupForCPUToScreenColorExpandFill = CTNAME(SetupForCPUToScreenColorExpandFill);
    infoPtr->SubsequentCPUToScreenColorExpandFill =
		CTNAME(SubsequentCPUToScreenColorExpandFill);
#endif  /* CHIPS_COLOREXPANDFILL */

    infoPtr->ColorExpandBase = (unsigned char *)cAcl->BltDataWindow;
    infoPtr->ColorExpandRange = 64 * 1024;

    /* Mono 8x8 pattern fills */
    infoPtr->Mono8x8PatternFillFlags = NO_PLANEMASK | 
	BIT_ORDER_IN_BYTE_MSBFIRST | HARDWARE_PATTERN_SCREEN_ORIGIN;

#ifdef CHIPS_HIQV
    infoPtr->SetupForMono8x8PatternFill =
	CTNAME(SetupForMono8x8PatternFill);
    infoPtr->SubsequentMono8x8PatternFillRect =
	CTNAME(SubsequentMono8x8PatternFillRect);
#else
    if (pScrn->bitsPerPixel != 24) {
	infoPtr->SetupForMono8x8PatternFill =
	    CTNAME(SetupForMono8x8PatternFill);
	infoPtr->SubsequentMono8x8PatternFillRect =
	    CTNAME(SubsequentMono8x8PatternFillRect);
    }
#endif

    /* Color 8x8 pattern fills, must have a displayWidth divisible by 64 */
    if (!(pScrn->displayWidth % 64)) {
#ifdef CHIPS_HIQV
	infoPtr->Color8x8PatternFillFlags = NO_PLANEMASK | 
	    HARDWARE_PATTERN_SCREEN_ORIGIN;
	if (!(cPtr->Flags & ChipsColorTransparency))
	    infoPtr->Color8x8PatternFillFlags |= NO_TRANSPARENCY;
#else
	infoPtr->Color8x8PatternFillFlags = NO_PLANEMASK | 
	    HARDWARE_PATTERN_SCREEN_ORIGIN | NO_TRANSPARENCY;
#endif

	if (pScrn->bitsPerPixel != 24) {
	    infoPtr->SetupForColor8x8PatternFill =
		CTNAME(SetupForColor8x8PatternFill);
	    infoPtr->SubsequentColor8x8PatternFillRect =
		CTNAME(SubsequentColor8x8PatternFillRect);
	}
    }

#ifdef CHIPS_HIQV
chips_imagewrite:
#endif

#ifdef CHIPS_WRITEPIXMAP
    /* Setup for the Image Write functions */
#ifdef CHIPS_HIQV
    infoPtr->WritePixmapFlags = CPU_TRANSFER_PAD_QWORD | LEFT_EDGE_CLIPPING 
        | LEFT_EDGE_CLIPPING_NEGATIVE_X;

    if (!(cPtr->Flags & ChipsColorTransparency))
        infoPtr->WritePixmapFlags |= NO_TRANSPARENCY;
    if ((pScrn->bitsPerPixel == 24) || (pScrn->bitsPerPixel == 32))
	infoPtr->WritePixmapFlags |= NO_PLANEMASK;

    infoPtr->WritePixmap = CTNAME(WritePixmap);
#else
    infoPtr->SetupForImageWrite = CTNAME(SetupForImageWrite);
    infoPtr->SubsequentImageWriteRect = CTNAME(SubsequentImageWriteRect);
    infoPtr->ImageWriteBase = (unsigned char *)cAcl->BltDataWindow;
    infoPtr->ImageWriteRange = 64 * 1024;
    infoPtr->ImageWriteFlags = NO_TRANSPARENCY | CPU_TRANSFER_PAD_DWORD;
    if ((pScrn->bitsPerPixel == 24) || (pScrn->bitsPerPixel == 32))
        infoPtr->ImageWriteFlags |= NO_PLANEMASK;
#endif
#endif  /* CHIPS_WRITEPIXMAP */

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = cAcl->CacheEnd /
      (pScrn->displayWidth * cAcl->BytesPerPixel);

    xf86InitFBManager(pScreen, &AvailFBArea); 

    return(XAAInit(pScreen, infoPtr));
}

void
CTNAME(Sync)(ScrnInfoPtr pScrn)
{
#ifndef CHIPS_HIQV
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
#endif
    ctBLTWAIT;
}

static void
CTNAME(8SetupForSolidFill)(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    cAcl->CommandFlags = ChipsAluConv2[rop & 0xF] | ctTOP2BOTTOM 
	     | ctLEFT2RIGHT | ctPATSOLID | ctPATMONO;
    ctBLTWAIT;
    ctSETBGCOLOR8(color);
    ctSETFGCOLOR8(color);
    ctSETPITCH(0, cAcl->PitchInBytes);
}

static void
CTNAME(16SetupForSolidFill)(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    cAcl->CommandFlags = ChipsAluConv2[rop & 0xF] | ctTOP2BOTTOM 
	     | ctLEFT2RIGHT | ctPATSOLID | ctPATMONO;
    ctBLTWAIT;
    ctSETBGCOLOR16(color);
    ctSETFGCOLOR16(color);
    ctSETPITCH(0, cAcl->PitchInBytes);
}

#ifdef CHIPS_HIQV
static void
CTNAME(24SetupForSolidFill)(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    cAcl->CommandFlags = ChipsAluConv2[rop & 0xF] | ctTOP2BOTTOM 
	     | ctLEFT2RIGHT | ctPATSOLID | ctPATMONO;
    ctBLTWAIT;
    ctSETBGCOLOR24(color);
    ctSETFGCOLOR24(color);
    ctSETPITCH(0, cAcl->PitchInBytes);
}

static void
CTNAME(32SetupForSolidFill)(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    ctBLTWAIT;
    memset((unsigned char *)cPtr->FbBase + cAcl->ScratchAddress, 0xAA, 8);
    ctSETFGCOLOR16((color & 0xFFFF));
    ctSETBGCOLOR16(((color >> 16) & 0xFFFF));
    ctSETROP(ChipsAluConv2[rop & 0xF] | ctTOP2BOTTOM | ctLEFT2RIGHT |
      ctPATMONO);
    ctSETPATSRCADDR(cAcl->ScratchAddress);
    ctSETPITCH(1, cAcl->PitchInBytes);
}

static void
CTNAME(32SubsequentSolidFillRect)(ScrnInfoPtr pScrn, int x, int y, int w,
				  int h)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);

    unsigned int destaddr;
    destaddr = (y * pScrn->displayWidth + x) << 2;
    ctBLTWAIT;
    ctSETDSTADDR(destaddr);
    ctSETHEIGHTWIDTHGO(h, (w << 2));
}
#else

static void
CTNAME(24SetupForSolidFill)(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask)
{
    unsigned char pixel1, pixel2, pixel3;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
 
    cAcl->rgb24equal = ((((color & 0xFF) == ((color & 0xFF00) >> 8)) && 
		       ((color & 0xFF) == ((color & 0xFF0000) >> 16))));
    if (cAcl->rgb24equal) {
	cAcl->CommandFlags = ChipsAluConv2[rop & 0xF] | ctTOP2BOTTOM |
	         ctLEFT2RIGHT | ctPATSOLID | ctPATMONO;
	ctBLTWAIT;
	ctSETFGCOLOR8(color&0xFF);
	ctSETBGCOLOR8(color&0xFF);
	ctSETPITCH(0, cAcl->PitchInBytes);
    } else {
	cAcl->rop24bpp = rop;
	if (rop == GXcopy) {
	    pixel3 = color & 0xFF;
	    pixel2 = (color >> 8) & 0xFF;
	    pixel1 = (color >> 16) & 0xFF;
	    cAcl->fgpixel = pixel1;
	    cAcl->bgpixel = pixel2;
	    cAcl->fillindex = 0;
	    cAcl->fastfill = FALSE;

	    /* Test for the special case where two of the byte of the 
	     * 24bpp colour are the same. This can double the speed
	     */
	    if (pixel1 == pixel2) {
		cAcl->fgpixel = pixel3;
		cAcl->bgpixel = pixel1;
		cAcl->fastfill = TRUE;
		cAcl->fillindex = 1;
	    } else if (pixel1 == pixel3) { 
		cAcl->fgpixel = pixel2;
		cAcl->bgpixel = pixel1;
		cAcl->fastfill = TRUE;
		cAcl->fillindex = 2;
	    } else if (pixel2 == pixel3) { 
		cAcl->fastfill = TRUE;
	    } else {
		cAcl->xorpixel = pixel2 ^ pixel3;
	    }

	    cAcl->CommandFlags = ctSRCMONO | ctSRCSYSTEM | ctTOP2BOTTOM | 
	             ctLEFT2RIGHT;
	    ctBLTWAIT;
	    if (cAcl->fastfill) { 
		ctSETFGCOLOR8(cAcl->fgpixel);
	    }
	    ctSETBGCOLOR8(cAcl->bgpixel);
	    ctSETSRCADDR(0);
	    ctSETPITCH(0, cAcl->PitchInBytes);
	} else {
	    if (cAcl->color24bpp != color) {
		cAcl->color24bpp = color;
		cAcl->width24bpp = 0;
	    }
	    cAcl->rop24bpp = rop;
	    ctBLTWAIT;
	    ctSETROP(ctTOP2BOTTOM | ctLEFT2RIGHT | ChipsAluConv[rop & 0xF]);
	    ctSETPITCH(cAcl->PitchInBytes, cAcl->PitchInBytes);
	}
    }
}

static void
CTNAME(24SubsequentSolidFillRect)(ScrnInfoPtr pScrn, int x, int y, int w,
				  int h)
{
    static unsigned int dwords[3] = { 0x24499224, 0x92244992, 0x49922449};
    int srcaddr, destaddr, line, i;
    register int width;
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    if (cAcl->rgb24equal) {
	destaddr = y * pScrn->displayWidth + x;
	destaddr += destaddr << 1;
	ctBLTWAIT;
	ctSETROP(cAcl->CommandFlags);
	ctSETDSTADDR(destaddr);
	ctSETHEIGHTWIDTHGO(h, (w + (w << 1)));
    } else {
	if (cAcl->rop24bpp == GXcopy ) {
	    unsigned int *base = (unsigned int *)cAcl->BltDataWindow;
	    destaddr = y * cAcl->PitchInBytes + x * 3;
	    w *= 3;
	    width = ((w  + 31) & ~31) >> 5;
	    
	    ctBLTWAIT;
	    ctSETDSTADDR(destaddr);

	    if (!cAcl->fastfill) ctSETFGCOLOR8(cAcl->fgpixel);
	    ctSETROP(cAcl->CommandFlags | ChipsAluConv[GXcopy & 0xF]);
	    ctSETDSTADDR(destaddr);
	    if (cAcl->fastfill) {
		ctSETHEIGHTWIDTHGO(h, w);
		line = 0;
		while (line < h) {
		    base = (unsigned int *)cAcl->BltDataWindow;
		    for (i = 0; i < width; i++) {
			*base++ = dwords[((cAcl->fillindex + i) % 3)];
		    }
		    line++;
		}
	    } else {
		ctSETHEIGHTWIDTHGO(1, w);
		i = 0;
		while(i < width){
		    *base++ = dwords[(i++ % 3)];
		}
		for(line = 0; (h >> line ) > 1; line++){;}
		i = 0;
		ctBLTWAIT;
		ctSETFGCOLOR8(cAcl->xorpixel);
		ctSETROP(cAcl->CommandFlags | ChipsAluConv[GXxor & 0xF] |
			 ctBGTRANSPARENT);
		ctSETDSTADDR(destaddr);
		ctSETHEIGHTWIDTHGO(1, w);
		base = (unsigned int *)cAcl->BltDataWindow;
		while(i < width) {
		    *base++ = dwords[((++i) % 3)];
		}
		srcaddr = destaddr;
		if(line){
		    i = 0;
		    ctBLTWAIT;
		    ctSETROP(ctTOP2BOTTOM | ctLEFT2RIGHT |
			     ChipsAluConv[GXcopy & 0xF]);
		    ctSETPITCH(cAcl->PitchInBytes, cAcl->PitchInBytes);
		    ctSETSRCADDR(srcaddr);
	    
		    while(i < line){
		        destaddr = srcaddr + (cAcl->PitchInBytes << i);
			ctBLTWAIT;
			ctSETDSTADDR(destaddr);
			ctSETHEIGHTWIDTHGO((1 << i), w);
			i++;
		    }

		    if((1 <<  line)  < h){
			destaddr = srcaddr + (cAcl->PitchInBytes << line);
			ctBLTWAIT;
			ctSETDSTADDR(destaddr);
			ctSETHEIGHTWIDTHGO(h-(1 << line), w);
		    }

		    ctBLTWAIT;
		    ctSETROP(ctSRCMONO | ctSRCSYSTEM | ctTOP2BOTTOM |
			     ctLEFT2RIGHT | ChipsAluConv[GXcopy & 0xF]);
		    ctSETSRCADDR(0);
		    ctSETPITCH(0, cAcl->PitchInBytes);
		}
	    }
	} else {
	    register unsigned char *base;
	    if (cAcl->width24bpp < w) {
		base = (unsigned char *)cPtr->FbBase + cAcl->ScratchAddress + 
		    ((3 * cAcl->width24bpp + 3) & ~0x3);
		width = w - cAcl->width24bpp;
		ctBLTWAIT;
		/* Load of a single scanline into framebuffer */
		while (width > 0) {
		    *(unsigned int *)base = cAcl->color24bpp |
		      (cAcl->color24bpp << 24);
		    *(unsigned int *)(base + 4) = (cAcl->color24bpp >> 8) |
		      (cAcl->color24bpp << 16);
		    *(unsigned int *)(base + 8) = (cAcl->color24bpp >> 16) |
		      (cAcl->color24bpp << 8);
		    base += 12;
		    width -= 4;
		}
		cAcl->width24bpp = w - width;
	    }
	    line = 0;
	    destaddr = 3 * (y * pScrn->displayWidth + x);
	    while (line < h) {
		ctBLTWAIT;
		ctSETSRCADDR(cAcl->ScratchAddress);
		ctSETDSTADDR(destaddr);
		ctSETHEIGHTWIDTHGO(1, cAcl->BytesPerPixel * w);
		destaddr += (3 * pScrn->displayWidth);
		line++;
	    }
	}
    }
}
#endif

static void
CTNAME(SubsequentSolidFillRect)(ScrnInfoPtr pScrn, int x, int y, int w,
				  int h)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    int destaddr;

    destaddr = (y * pScrn->displayWidth + x) * cAcl->BytesPerPixel;
    ctBLTWAIT;
    ctSETROP(cAcl->CommandFlags);
    ctSETDSTADDR(destaddr);
    ctSETHEIGHTWIDTHGO(h, w * cAcl->BytesPerPixel);
}

/*
 * Screen-to-screen BitBLT.
 *
 */
 
static void
CTNAME(SetupForScreenToScreenCopy)(ScrnInfoPtr pScrn, int xdir, int ydir,
				int rop, unsigned int planemask, int trans)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    cAcl->CommandFlags = 0;
    
    /* Set up the blit direction. */
    if (ydir < 0)
	cAcl->CommandFlags |= ctBOTTOM2TOP;
    else
	cAcl->CommandFlags |= ctTOP2BOTTOM;
    if (xdir < 0)
	cAcl->CommandFlags |= ctRIGHT2LEFT;
    else
	cAcl->CommandFlags |= ctLEFT2RIGHT;
#ifdef CHIPS_HIQV
    if (trans != -1) {
	cAcl->CommandFlags |= ctCOLORTRANSENABLE | ctCOLORTRANSROP |
	    ctCOLORTRANSNEQUAL;
	ctBLTWAIT;
        switch (pScrn->bitsPerPixel) {
        case 8:
	    ctSETBGCOLOR8(trans);
	    break;
        case 16:
	    ctSETBGCOLOR16(trans);
	    break;
        case 24:
	    ctSETBGCOLOR24(trans);
	    break;
        }
    } else
#endif
    ctBLTWAIT;
    if ((pScrn->bitsPerPixel == 8 && (planemask & 0xFF) == 0xFF) ||
    (pScrn->bitsPerPixel == 16 && (planemask & 0xFFFF) == 0xFFFF) ||
    (pScrn->bitsPerPixel == 24 && (planemask & 0xFFFFFF) == 0xFFFFFF) ||
    (pScrn->bitsPerPixel == 32))
    {
	ctSETROP(cAcl->CommandFlags | ChipsAluConv[rop & 0xF]);
    } else {
	ctSETROP(cAcl->CommandFlags | ChipsAluConv3[rop & 0xF]);
	ctSETPATSRCADDR(cAcl->ScratchAddress);
	ctWRITEPLANEMASK(planemask, cAcl->ScratchAddress);
    }
    ctSETPITCH(cAcl->PitchInBytes, cAcl->PitchInBytes);
}

static void
CTNAME(SubsequentScreenToScreenCopy)(ScrnInfoPtr pScrn, int srcX, int srcY,
				int dstX, int dstY, int w, int h)
{ 
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    unsigned int srcaddr, destaddr;
#ifdef CHIPS_HIQV
    if (cAcl->CommandFlags & ctBOTTOM2TOP) {
        srcaddr = (srcY + h - 1) * pScrn->displayWidth;
	destaddr = (dstY + h - 1) * pScrn->displayWidth;
    } else {
        srcaddr = srcY * pScrn->displayWidth;
        destaddr = dstY * pScrn->displayWidth;
    }
    if (cAcl->CommandFlags & ctRIGHT2LEFT) {
	srcaddr = ( srcaddr + srcX + w ) * cAcl->BytesPerPixel - 1 ;
	destaddr = ( destaddr + dstX + w ) * cAcl->BytesPerPixel - 1;
    } else {
	srcaddr = (srcaddr + srcX) * cAcl->BytesPerPixel;
	destaddr = (destaddr + dstX) * cAcl->BytesPerPixel;
    }
#else
    if (cAcl->CommandFlags & ctTOP2BOTTOM) {
        srcaddr = srcY * pScrn->displayWidth;
        destaddr = dstY * pScrn->displayWidth;
    } else {
        srcaddr = (srcY + h - 1) * pScrn->displayWidth;
	destaddr = (dstY + h - 1) * pScrn->displayWidth;
    }
    if (cAcl->CommandFlags & ctLEFT2RIGHT) {
	srcaddr = (srcaddr + srcX) * cAcl->BytesPerPixel;
	destaddr = (destaddr + dstX) * cAcl->BytesPerPixel;
    } else {
	srcaddr = ( srcaddr + srcX + w ) * cAcl->BytesPerPixel - 1 ;
	destaddr = ( destaddr + dstX + w ) * cAcl->BytesPerPixel - 1;
    }
#endif
    ctBLTWAIT;
    ctSETSRCADDR(srcaddr);
    ctSETDSTADDR(destaddr);
    ctSETHEIGHTWIDTHGO(h, w * cAcl->BytesPerPixel);
}


static void 
CTNAME(SetupForCPUToScreenColorExpandFill)(ScrnInfoPtr pScrn, int fg,
				int bg, int rop, unsigned int planemask)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    ctBLTWAIT;
    cAcl->CommandFlags = 0;
    if (bg == -1) {
	cAcl->CommandFlags |= ctBGTRANSPARENT;	/* Background = Destination */
        switch (pScrn->bitsPerPixel) {
        case 8:
	    ctSETFGCOLOR8(fg);
	    break;
        case 16:
	    ctSETFGCOLOR16(fg);
	    break;
        case 24:
#ifdef CHIPS_HIQV
	    ctSETFGCOLOR24(fg);
#else
	    ctSETFGCOLOR8(fg);
#endif
	    break;
        }
    }
    else {
        switch (pScrn->bitsPerPixel) {
        case 8:
	    ctSETBGCOLOR8(bg);
	    ctSETFGCOLOR8(fg);
	    break;
        case 16:
	    ctSETBGCOLOR16(bg);
	    ctSETFGCOLOR16(fg);
	    break;
        case 24:
#ifdef CHIPS_HIQV
	    ctSETBGCOLOR24(bg);
	    ctSETFGCOLOR24(fg);
#else
	    ctSETBGCOLOR8(bg);
	    ctSETFGCOLOR8(fg);
#endif
	    break;
        }
    }
   
#ifdef CHIPS_HIQV
    ctSETMONOCTL(ctDWORDALIGN);
#endif

    ctSETSRCADDR(0);

   if ((pScrn->bitsPerPixel == 8 && (planemask & 0xFF) == 0xFF) ||
    (pScrn->bitsPerPixel == 16 && (planemask & 0xFFFF) == 0xFFFF) ||
    (pScrn->bitsPerPixel == 24 && (planemask & 0xFFFFFF) == 0xFFFFFF))
    {
	ctSETROP(ctSRCSYSTEM | ctSRCMONO | ctTOP2BOTTOM | ctLEFT2RIGHT |
		 ChipsAluConv[rop & 0xF] | cAcl->CommandFlags);
    } else {
	ctSETROP(ctSRCSYSTEM | ctSRCMONO | ctTOP2BOTTOM | ctLEFT2RIGHT | 
		 ChipsAluConv3[rop & 0xF] | cAcl->CommandFlags);
	ctSETPATSRCADDR(cAcl->ScratchAddress);
	ctWRITEPLANEMASK(planemask, cAcl->ScratchAddress);
    }
    ctSETPITCH(0, cAcl->PitchInBytes);
}

static void
CTNAME(SubsequentCPUToScreenColorExpandFill)(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    int destaddr;

    destaddr = (y * pScrn->displayWidth + x + skipleft) * 
               cAcl->BytesPerPixel;
    ctBLTWAIT;
    ctSETDSTADDR(destaddr);
#ifdef CHIPS_HIQV
    ctSETMONOCTL(ctDWORDALIGN | ctCLIPLEFT(skipleft));
#endif
    ctSETHEIGHTWIDTHGO(h, (w - skipleft) * cAcl->BytesPerPixel);
}

static void
CTNAME(SetupForColor8x8PatternFill)(ScrnInfoPtr pScrn, int patx, int paty, 
				    int rop, unsigned int planemask,
				    int trans)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    unsigned int patternaddr;
    
    cAcl->CommandFlags = ChipsAluConv2[rop & 0xF] | ctTOP2BOTTOM |
		ctLEFT2RIGHT;
    patternaddr = (paty * pScrn->displayWidth + 
		   (patx & ~0x3F)) * cAcl->BytesPerPixel;
    cAcl->patternyrot = (patx & 0x3F) >> 3;

    ctBLTWAIT;
    ctSETPATSRCADDR(patternaddr);
#ifdef CHIPS_HIQV
    if (trans != -1) {
	cAcl->CommandFlags |= ctCOLORTRANSENABLE | ctCOLORTRANSROP |
	    ctCOLORTRANSNEQUAL;
        switch (pScrn->bitsPerPixel) {
        case 8:
	    ctSETBGCOLOR8(trans);
	    break;
        case 16:
	    ctSETBGCOLOR16(trans);
	    break;
        case 24:
	    ctSETBGCOLOR24(trans);
	    break;
        }
    } else
#endif
    ctSETPITCH(8 * cAcl->BytesPerPixel, cAcl->PitchInBytes);
}

static void 
CTNAME(SubsequentColor8x8PatternFillRect)(ScrnInfoPtr pScrn, int patx, int paty,
				 int x, int y, int w, int h)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    unsigned int destaddr;

    destaddr = (y * pScrn->displayWidth + x) * cAcl->BytesPerPixel;
    ctBLTWAIT;
    ctSETDSTADDR(destaddr);
#ifdef CHIPS_HIQV
    ctSETROP(cAcl->CommandFlags | (((y + cAcl->patternyrot) & 0x7) << 20));
#else
    ctSETROP(cAcl->CommandFlags | (((y + cAcl->patternyrot) & 0x7) << 16));
#endif
    ctSETHEIGHTWIDTHGO(h, w * cAcl->BytesPerPixel);
}

static void
CTNAME(SetupForMono8x8PatternFill)(ScrnInfoPtr pScrn, int patx, int paty, 
				int fg, int bg, int rop,
				unsigned int planemask)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    unsigned int patternaddr;

    cAcl->CommandFlags = ctPATMONO | ctTOP2BOTTOM | ctLEFT2RIGHT | 
	ChipsAluConv2[rop & 0xF];

    patternaddr = (paty * pScrn->displayWidth + patx) * cAcl->BytesPerPixel;
    ctBLTWAIT;
    ctSETPATSRCADDR(patternaddr);
    if (bg == -1) {
	cAcl->CommandFlags |= ctBGTRANSPARENT;	/* Background = Destination */
        switch (pScrn->bitsPerPixel) {
        case 8:
	    ctSETFGCOLOR8(fg);
	    break;
        case 16:
	    ctSETFGCOLOR16(fg);
	    break;
        case 24:
	    ctSETFGCOLOR24(fg);
	    break;
        }
    }
    else {
        switch (pScrn->bitsPerPixel) {
        case 8:
	    ctSETBGCOLOR8(bg);
	    ctSETFGCOLOR8(fg);
	    break;
        case 16:
	    ctSETBGCOLOR16(bg);
	    ctSETFGCOLOR16(fg);
	    break;
        case 24:
	    ctSETBGCOLOR24(bg);
	    ctSETFGCOLOR24(fg);
	    break;
        }
    }
#ifdef CHIPS_HIQV
    ctSETMONOCTL(ctDWORDALIGN);
#endif
    ctSETPITCH(1,cAcl->PitchInBytes);
}

static void
CTNAME(SubsequentMono8x8PatternFillRect)(ScrnInfoPtr pScrn, int patx, 
				int paty, int x, int y, int w, int h )
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    int destaddr;
    destaddr = (y * pScrn->displayWidth + x) * cAcl->BytesPerPixel;

    ctBLTWAIT;
    ctSETDSTADDR(destaddr);
#ifdef CHIPS_HIQV
    ctSETROP(cAcl->CommandFlags | ((y & 0x7) << 20));
#else
    ctSETROP(cAcl->CommandFlags | ((y & 0x7) << 16));
#endif
    ctSETHEIGHTWIDTHGO(h, w * cAcl->BytesPerPixel);
}

#ifndef	CHIPS_HIQV
static void
CTNAME(SetupForImageWrite)(ScrnInfoPtr pScrn, int rop, unsigned int planemask,
				int transparency_color, int bpp, int depth)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    cAcl->CommandFlags = ctSRCSYSTEM | ctTOP2BOTTOM | ctLEFT2RIGHT;
    ctBLTWAIT;
    if ((pScrn->bitsPerPixel == 8 && (planemask & 0xFF) == 0xFF) ||
    (pScrn->bitsPerPixel == 16 && (planemask & 0xFFFF) == 0xFFFF) ||
    (pScrn->bitsPerPixel == 24 && (planemask & 0xFFFFFF) == 0xFFFFFF) ||
    (pScrn->bitsPerPixel == 32))
    {
	ctSETROP(cAcl->CommandFlags | ChipsAluConv[rop & 0xF]);
    } else {
	ctSETROP(cAcl->CommandFlags | ChipsAluConv3[rop & 0xF]);
	ctSETPATSRCADDR(cAcl->ScratchAddress);
	ctWRITEPLANEMASK(planemask, cAcl->ScratchAddress);
    }
}

static void
CTNAME(SubsequentImageWriteRect)(ScrnInfoPtr pScrn, int x, int y, int w, int h,
				int skipleft)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);

    ctBLTWAIT;
    ctSETPITCH(((w * cAcl->BytesPerPixel + 3) & ~0x3), cAcl->PitchInBytes);
    ctSETSRCADDR(0);
    ctSETDSTADDR((y * pScrn->displayWidth + x) * cAcl->BytesPerPixel);
    ctSETHEIGHTWIDTHGO(h, w * cAcl->BytesPerPixel);
}

#else 
/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 * This software is furnished under license and may be used and copied only in 
 * accordance with the following terms and conditions.  Subject to these 
 * conditions, you may download, copy, install, use, modify and distribute 
 * this software in source and/or binary form. No title or ownership is 
 * transferred hereby.
 * 1) Any source code used, modified or distributed must reproduce and retain 
 *    this copyright notice and list of conditions as they appear in the 
 *    source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of Digital 
 *    Equipment Corporation. Neither the "Digital Equipment Corporation" name 
 *    nor any trademark or logo of Digital Equipment Corporation may be used 
 *    to endorse or promote products derived from this software without the 
 *    prior written permission of Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied warranties,
 *    including but not limited to, any implied warranties of merchantability,
 *    fitness for a particular purpose, or non-infringement are disclaimed. In
 *    no event shall DIGITAL be liable for any damages whatsoever, and in 
 *    particular, DIGITAL shall not be liable for special, indirect, 
 *    consequential, or incidental damages or damages for lost profits, loss 
 *    of revenue or loss of use, whether such damages arise in contract, 
 *    negligence, tort, under statute, in equity, at law or otherwise, even if
 *    advised of the possibility of such damage. 
 */

/* The code below comes from the idea supplied by the people at DEC, like
 * the copyright above says. But its had to go through a large evolution
 * to fit it into the new design for XFree 4.0
 */

static void
MoveDWORDS(register CARD32* dest, register CARD32* src, register int dwords )
{
     while(dwords & ~0x03) {
	*dest = *src;
	*(dest + 1) = *(src + 1);
	*(dest + 2) = *(src + 2);
	*(dest + 3) = *(src + 3);
	src += 4;
	dest += 4;
	dwords -= 4;
     }	
     switch(dwords){
     case 1:
       *dest = *src;
       break;
     case 2:
       *dest = *src;
       *(dest + 1) = *(src + 1);
       break;
     case 3:
       *dest = *src;
       *(dest + 1) = *(src + 1);
       *(dest + 2) = *(src + 2);
       break;
     }
}

#ifndef __GNUC__
#define __inline__ /**/
#endif
 
static __inline__ void 
MoveData(unsigned char *src, unsigned char *dest, int srcwidth, int window,
	 int h, int dwords)
{
    if(srcwidth == (dwords << 2)) {
	int decrement = window / dwords;
	while(h > decrement) {
	    MoveDWORDS((CARD32*)dest, (CARD32*)src, dwords * decrement);
	    src += (srcwidth * decrement);
	    h -= decrement;
	}
	if(h) {
	    MoveDWORDS((CARD32*)dest, (CARD32*)src, dwords * h);
	}
    } else {
	while(h--) {
	    MoveDWORDS((CARD32*)dest, (CARD32*)src, dwords);
	    src += srcwidth;
	}
    }
}

static void 
CTNAME(WritePixmap)(ScrnInfoPtr pScrn, int x, int y, int w, int h,
		unsigned char *src, int srcwidth, int rop,
		unsigned int planemask, int trans, int bpp, int depth)
{
    CHIPSPtr cPtr = CHIPSPTR(pScrn);
    CHIPSACLPtr cAcl = CHIPSACLPTR(pScrn);
    unsigned int bytesPerLine;
    unsigned int byteWidthSrc;
    int dwords; 
    int skipleft;

    bytesPerLine = w * cAcl->BytesPerPixel;

    byteWidthSrc = ((srcwidth * cAcl->BytesPerPixel + 3L) & ~0x3L);

    cAcl->CommandFlags = ctSRCSYSTEM | ctLEFT2RIGHT | ctTOP2BOTTOM;

    ctBLTWAIT;

    if (trans != -1) {
	cAcl->CommandFlags |= ctCOLORTRANSENABLE | ctCOLORTRANSROP |
	    ctCOLORTRANSNEQUAL;
        switch (pScrn->bitsPerPixel) {
        case 8:
	    ctSETBGCOLOR8(trans);
	    break;
        case 16:
	    ctSETBGCOLOR16(trans);
	    break;
        case 24:
	    ctSETBGCOLOR24(trans);
	    break;
        }
    }
    
    if ((pScrn->bitsPerPixel == 8 && (planemask & 0xFF) == 0xFF) ||
    (pScrn->bitsPerPixel == 16 && (planemask & 0xFFFF) == 0xFFFF) ||
    (pScrn->bitsPerPixel == 24 && (planemask & 0xFFFFFF) == 0xFFFFFF) ||
    (pScrn->bitsPerPixel == 32))
    {
	ctSETROP(cAcl->CommandFlags | ChipsAluConv[rop & 0xF]);
    } else {
	ctSETROP(cAcl->CommandFlags | ChipsAluConv3[rop & 0xF]);
	ctSETPATSRCADDR(cAcl->ScratchAddress);
	ctWRITEPLANEMASK(planemask, cAcl->ScratchAddress);
    }

    /*
     *
     *  CT6555X requires quad-word padding, but XAA provides double-word
     *  padding. If the width of a region to be transferred happens to be 
     *  quad-word aligned, the transfer is straightforward.  If the 
     *  region is double-word aligned, a pair of contiguous scanlines 
     *  is quad-word aligned.  In latter case, we can use interleaved 
     *  transfer twice.  It is faster than transfer line by line.
     *
     */

    skipleft = (unsigned int)src & 0x7;
    src = (unsigned char *)((unsigned int)src & ~0x7L);
    dwords = (((skipleft  + bytesPerLine + 0x7) & ~0x7)) >> 2;
    ctSETSRCADDR(skipleft);
    ctSETDSTADDR(y * cAcl->PitchInBytes + x * cAcl->BytesPerPixel);
	

    if ((byteWidthSrc & 0x7) == 0) {  /* quad-word aligned */

	ctSETPITCH(byteWidthSrc, cAcl->PitchInBytes);
	ctSETHEIGHTWIDTHGO(h, bytesPerLine);

	MoveData((unsigned char *)src, (unsigned char *)cAcl->BltDataWindow,
		 srcwidth, 16384, h, dwords);

    } else {
	unsigned int vert = h;

	h = (vert + 1) >> 1;

	ctSETPITCH(byteWidthSrc << 1, cAcl->PitchInBytes << 1);
	ctSETHEIGHTWIDTHGO(h, bytesPerLine);

	MoveData((unsigned char *)src, (unsigned char *)cAcl->BltDataWindow,
		 srcwidth<<1, 16384, h, dwords);

	h = vert  >> 1;
	src += srcwidth;
	y++;
	ctBLTWAIT;

	ctSETDSTADDR((y * pScrn->displayWidth + x) * cAcl->BytesPerPixel);
	ctSETHEIGHTWIDTHGO(h, bytesPerLine);

	MoveData((unsigned char *)src, (unsigned char *)cAcl->BltDataWindow,
		 srcwidth<<1, 16384, h, dwords);
    }
#if 0
    /* Not sure how safe it is to do this */
    cPtr->AccelInfoRec->NeedToSync = TRUE;
#else
    ctBLTWAIT;
#endif
}
#endif
