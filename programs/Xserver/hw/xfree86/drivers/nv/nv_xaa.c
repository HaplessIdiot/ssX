/* $XConsortium: nv_driver.c /main/3 1996/10/28 05:13:37 kaleb $ */
 /***************************************************************************\
|*                                                                           *|
|*       Copyright 1993-1999 NVIDIA, Corporation.  All rights reserved.      *|
|*                                                                           *|
|*     NOTICE TO USER:   The source code  is copyrighted under  U.S. and     *|
|*     international laws.  Users and possessors of this source code are     *|
|*     hereby granted a nonexclusive,  royalty-free copyright license to     *|
|*     use this code in individual and commercial software.                  *|
|*                                                                           *|
|*     Any use of this source code must include,  in the user documenta-     *|
|*     tion and  internal comments to the code,  notices to the end user     *|
|*     as follows:                                                           *|
|*                                                                           *|
|*       Copyright 1993-1999 NVIDIA, Corporation.  All rights reserved.      *|
|*                                                                           *|
|*     NVIDIA, CORPORATION MAKES NO REPRESENTATION ABOUT THE SUITABILITY     *|
|*     OF  THIS SOURCE  CODE  FOR ANY PURPOSE.  IT IS  PROVIDED  "AS IS"     *|
|*     WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.  NVIDIA, CORPOR-     *|
|*     ATION DISCLAIMS ALL WARRANTIES  WITH REGARD  TO THIS SOURCE CODE,     *|
|*     INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY, NONINFRINGE-     *|
|*     MENT,  AND FITNESS  FOR A PARTICULAR PURPOSE.   IN NO EVENT SHALL     *|
|*     NVIDIA, CORPORATION  BE LIABLE FOR ANY SPECIAL,  INDIRECT,  INCI-     *|
|*     DENTAL, OR CONSEQUENTIAL DAMAGES,  OR ANY DAMAGES  WHATSOEVER RE-     *|
|*     SULTING FROM LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION     *|
|*     OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,  ARISING OUT OF     *|
|*     OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOURCE CODE.     *|
|*                                                                           *|
|*     U.S. Government  End  Users.   This source code  is a "commercial     *|
|*     item,"  as that  term is  defined at  48 C.F.R. 2.101 (OCT 1995),     *|
|*     consisting  of "commercial  computer  software"  and  "commercial     *|
|*     computer  software  documentation,"  as such  terms  are  used in     *|
|*     48 C.F.R. 12.212 (SEPT 1995)  and is provided to the U.S. Govern-     *|
|*     ment only as  a commercial end item.   Consistent with  48 C.F.R.     *|
|*     12.212 and  48 C.F.R. 227.7202-1 through  227.7202-4 (JUNE 1995),     *|
|*     all U.S. Government End Users  acquire the source code  with only     *|
|*     those rights set forth herein.                                        *|
|*                                                                           *|
 \***************************************************************************/

/* Hacked together from mga driver and 3.3.4 NVIDIA driver by
   Jarno Paananen <jpaana@s2.org> */

/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/nv/nv_xaa.c,v 1.1 1999/08/01 07:20:59 dawes Exp $ */

#include "nv_include.h"
#include "xaalocal.h"

#include "nvreg.h"
#include "nvvga.h"

/*
 * Macro to define valid rectangle.
 */
#define NV_RECT_VALID(rr)  (((rr).x1 < (rr).x2) && ((rr).y1 < (rr).y2))


/*
 * 2D render flag for 3D code.
 */
int rivaRendered2D = 0;

/*
 * Opaque monochrome bits.
 */
unsigned int rivaOpaqueMonochrome;

/*
 * Set scissor clip rect.
 */
static void NVSetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1,
                                   int x2, int y2)
{
    int height = y2-y1 + 1;
    int width  = x2-x1 + 1;
    NVPtr pNv = NVPTR(pScrn);

    rivaRendered2D = 1;
    RIVA_FIFO_FREE(pNv->riva, Clip, 2);
    pNv->riva.Clip->TopLeft     = (y1     << 16) | (x1 & 0xffff);
    pNv->riva.Clip->WidthHeight = (height << 16) | width;
}

static void NVDisableClipping(ScrnInfoPtr pScrn)
{
    NVSetClippingRectangle(pScrn, 0, 0, 0x7FFF, 0x7FFF);
}

/*
 * Set pattern. Internal routine. The upper bits of the colors
 * are the ALPHA bits.  0 == transparency.
 */
static void NVSetPattern(NVPtr pNv, int clr0, int clr1, int pat0,
                         int pat1)
{
    rivaRendered2D = 1;
    RIVA_FIFO_FREE(pNv->riva, Patt, 5);
    pNv->riva.Patt->Shape         = 0; /* 0 = 8X8, 1 = 64X1, 2 = 1X64 */
    pNv->riva.Patt->Color0        = clr0;
    pNv->riva.Patt->Color1        = clr1;
    pNv->riva.Patt->Monochrome[0] = pat0;
    pNv->riva.Patt->Monochrome[1] = pat1;
}

/*
 * Set ROP.  Translate X rop into ROP3.  Internal routine.
 */
static int currentRop = -1;
static void NVSetRopSolid(NVPtr pNv, int rop)
{
    static int ropTrans[16] = 
    {
        0x00, /* GXclear        */
        0x88, /* Gxand          */
        0x44, /* GXandReverse   */
        0xCC, /* GXcopy         */
        0x22, /* GXandInverted  */
        0xAA, /* GXnoop         */
        0x66, /* GXxor          */
        0xEE, /* GXor           */
        0x11, /* GXnor          */
        0x99, /* GXequiv        */
        0x55, /* GXinvert       */
        0xDD, /* GXorReverse    */
        0x33, /* GXcopyInverted */
        0xBB, /* GXorInverted   */
        0x77, /* GXnand         */
        0xFF  /* GXset          */
    };
    
    if (currentRop != rop)
    {
        if (currentRop > 16)
            NVSetPattern(pNv, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
                         0xFFFFFFFF);
        currentRop     = rop;
        rivaRendered2D = 1;
        RIVA_FIFO_FREE(pNv->riva, Rop, 1);
        pNv->riva.Rop->Rop3 = ropTrans[rop];
    }
}

static void NVSetRopPattern(NVPtr pNv, int rop)
{
    static int ropTrans[16] = 
    {
        0x00, /* GXclear        */
        0xA0, /* Gxand          */
        0x0A, /* GXandReverse   */
        0xF0, /* GXcopy         */
        0x30, /* GXandInverted  */
        0xAA, /* GXnoop         */
        0x3A, /* GXxor          */
        0xFA, /* GXor           */
        0x03, /* GXnor          */
        0xA0, /* Gxequiv        */
        0x0F, /* GXinvert       */
        0xAF, /* GXorReverse    */
        0x33, /* GXcopyInverted */
        0xBB, /* GXorInverted   */
        0xF3, /* GXnand         */
        0xFF  /* GXset          */
    };

    if (currentRop != rop + 16)
    {
        currentRop     = rop + 16; /* +16 is important */
        rivaRendered2D = 1;
        RIVA_FIFO_FREE(pNv->riva, Rop, 1);
        pNv->riva.Rop->Rop3 = ropTrans[rop];
    }
}

/*
 * Fill solid rectangles.
 */                                           
static void NVSetupForSolidFill(ScrnInfoPtr pScrn,
                                int color, int rop, unsigned planemask)
{
    NVPtr pNv = NVPTR(pScrn);

    NVSetRopSolid(pNv, rop);
    rivaRendered2D = 1;
    RIVA_FIFO_FREE(pNv->riva, Bitmap, 1);
    pNv->riva.Bitmap->Color1A = color;
}

static void NVSubsequentSolidFillRect(ScrnInfoPtr pScrn,
                                      int x, int y, int w, int h)
{
    NVPtr pNv = NVPTR(pScrn);

    RIVA_FIFO_FREE(pNv->riva, Bitmap, 2);
    pNv->riva.Bitmap->UnclippedRectangle[0].TopLeft     = (x << 16) | y; 
    pNv->riva.Bitmap->UnclippedRectangle[0].WidthHeight = (w << 16) | h;
}

/*
 * Screen to screen BLTs.
 */
static void NVSetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
                                         int xdir, int ydir, int rop,
                                         unsigned planemask,
                                         int transparency_color)
{
    rivaRendered2D = 1;
    NVSetRopSolid(NVPTR(pScrn), rop);
}

static void NVSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1,
                                           int x2, int y2, int w, int h)
{
    NVPtr pNv = NVPTR(pScrn);

    RIVA_FIFO_FREE(pNv->riva, Blt, 3);
    pNv->riva.Blt->TopLeftSrc  = (y1 << 16) | x1;
    pNv->riva.Blt->TopLeftDst  = (y2 << 16) | x2;
    pNv->riva.Blt->WidthHeight = (h  << 16) | w;
}


/*
 * Fill 8x8 monochrome pattern rectangles.  patternx and patterny are
 * the overloaded pattern bits themselves. The pattern colors don't
 * support 565, only 555. Hack around it.
 */
static void NVSetupForMono8x8PatternFill(ScrnInfoPtr pScrn,
    	                                 int patternx, int patterny,
                                         int fg, int bg, int rop,
                                         unsigned planemask)
{
    NVPtr pNv = NVPTR(pScrn);

    NVSetRopPattern(pNv, rop);
    rivaRendered2D = 1;
    if (pScrn->bitsPerPixel == 16 && pScrn->weight.green == 6)
    {
        fg = ((fg & 0x0000F800) << 8)
           | ((fg & 0x000007E0) << 5)
           | ((fg & 0x0000001F) << 3)
           |        0xFF000000;
        if (bg != -1)
            bg = ((bg & 0x0000F800) << 8)
               | ((bg & 0x000007E0) << 5)
               | ((bg & 0x0000001F) << 3)
               |        0xFF000000;
        else
            bg = 0;
    }
    else
    {
	fg |= rivaOpaqueMonochrome;
	bg  = (bg == -1) ? 0 : bg | rivaOpaqueMonochrome;
    };
    NVSetPattern(pNv, fg, bg, patternx, patterny);
    RIVA_FIFO_FREE(pNv->riva, Bitmap, 1);
    pNv->riva.Bitmap->Color1A = fg;
}

static void NVSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn,
                                               int patternx, int patterny,
                                               int x, int y, int w, int h)
{
    NVPtr pNv = NVPTR(pScrn);

    RIVA_FIFO_FREE(pNv->riva, Bitmap, 2);
    pNv->riva.Bitmap->UnclippedRectangle[0].TopLeft     = (x << 16) | y;
    pNv->riva.Bitmap->UnclippedRectangle[0].WidthHeight = (w << 16) | h;
}



/*
 * Synchronise with graphics engine.  Make sure it is idle before returning.
 * Should attempt to yield CPU if busy for awhile.
 *
 * Also used in an evil hack with the color expansion
 */
void NVSync(ScrnInfoPtr pScrn)
{
    NVPtr pNv = NVPTR(pScrn);

    while (pNv->riva.Busy(&pNv->riva));
}

/* Color expansion */
static void
NVSetupForScanlineCPUToScreenColorExpandFill(ScrnInfoPtr pScrn,
                                             int fg, int bg, int rop, 
                                             unsigned int planemask)
{
    NVPtr pNv = NVPTR(pScrn);

    rivaRendered2D = 1;

    NVSetRopSolid(pNv, rop);

    if ( bg == -1 )
    {
        /* Transparent case */
        bg = 0x80000000;
        pNv->expandFifo = (unsigned char*)&pNv->riva.Bitmap->MonochromeData1C;
    }
    else
    {
        pNv->expandFifo = (unsigned char*)&pNv->riva.Bitmap->MonochromeData01E;
        if (pScrn->bitsPerPixel == 16 && pScrn->weight.green == 6)
        {
            bg = ((bg & 0x0000F800) << 8)
               | ((bg & 0x000007E0) << 5)
               | ((bg & 0x0000001F) << 3)
               |        0xFF000000;
        }
        else
        {
            bg  |= rivaOpaqueMonochrome;
        };
    }
    pNv->FgColor = fg;
    pNv->BgColor = bg;
}

static void
NVSubsequentScanlineCPUToScreenColorExpandFill(ScrnInfoPtr pScrn, int x,
                                               int y, int w, int h,
                                               int skipleft)
{
    int bw;
    NVPtr pNv = NVPTR(pScrn);
    
    bw = (w + 31) & ~31;
    pNv->expandWidth = bw >> 5;

    if ( pNv->BgColor == 0x80000000 )
    {
        /* Use faster transparent method */
        RIVA_FIFO_FREE(pNv->riva, Bitmap, 5);
        pNv->riva.Bitmap->ClipC.TopLeft     = (y << 16) | ((x+skipleft) & 0xFFFF);
        pNv->riva.Bitmap->ClipC.BottomRight = ((y+h) << 16) | ((x+w)&0xffff);
        pNv->riva.Bitmap->Color1C           = pNv->FgColor;
        pNv->riva.Bitmap->WidthHeightC      = (h << 16) | bw;
        pNv->riva.Bitmap->PointC            = (y << 16) | (x & 0xFFFF);
    }
    else
    {
        /* Opaque */
        RIVA_FIFO_FREE(pNv->riva, Bitmap, 7);
        pNv->riva.Bitmap->ClipE.TopLeft     = (y << 16) | ((x+skipleft) & 0xFFFF);
        pNv->riva.Bitmap->ClipE.BottomRight = ((y+h) << 16) | ((x+w)&0xffff);
        pNv->riva.Bitmap->Color0E           = pNv->BgColor;
        pNv->riva.Bitmap->Color1E           = pNv->FgColor;
        pNv->riva.Bitmap->WidthHeightInE  = (h << 16) | bw;
        pNv->riva.Bitmap->WidthHeightOutE = (h << 16) | bw;
        pNv->riva.Bitmap->PointE          = (y << 16) | (x & 0xFFFF);
    }

    if ( pNv->expandBuffer == NULL )
    {
        /* Using fifo writes, set it up */
        pNv->expandRows = h;
	while (pNv->riva.Busy(&pNv->riva));
        RIVA_FIFO_FREE(pNv->riva, Bitmap, pNv->expandWidth);
    }
}


static void
NVSubsequentColorExpandScanline(ScrnInfoPtr pScrn, int bufno)
{
    NVPtr pNv = NVPTR(pScrn);

    int t = pNv->expandWidth;
    unsigned int *pbits = (unsigned int*)pNv->expandBuffer;
    unsigned int *d = (unsigned int*)pNv->expandFifo;
    
    while (pNv->riva.Busy(&pNv->riva));
    while(t >= 4) 
    {    
	RIVA_FIFO_FREE(pNv->riva, Bitmap, 4); 
	d[0] = pbits[0]; /* burst */ 
	d[1] = pbits[1]; 
	d[2] = pbits[2]; 
	d[3] = pbits[3]; 
	t -= 4; d += 4; pbits += 4; 
    }
     
    while(t--) 
    {
	RIVA_FIFO_FREE(pNv->riva, Bitmap, 1); 
	*d++ = *pbits++; 
    }
}

static void
NVSubsequentColorExpandScanlineFifo(ScrnInfoPtr pScrn, int bufno)
{
    NVPtr pNv = NVPTR(pScrn);

    if ( --pNv->expandRows )
    {
	while (pNv->riva.Busy(&pNv->riva));
        RIVA_FIFO_FREE(pNv->riva, Bitmap, pNv->expandWidth);
    }
    
}


/* Image writes */
static void
NVSetupForScanlineImageWrite(ScrnInfoPtr pScrn, int rop,
                             unsigned int planemask, int transparency_color,
                             int bpp, int depth)
{
    NVPtr pNv = NVPTR(pScrn);

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Setup\n"
	       "rop %i\n"
	       "planemask %i\n"
	       "transparency_color %i\n"
	       "bpp %i\n"
	       "depth %i\n",
	       rop, planemask, transparency_color, bpp, depth);
    
    rivaRendered2D = 1;
    NVSetRopSolid(pNv, rop);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "-Done\n");
}

static void
NVSubsequentScanlineImageWriteRect(ScrnInfoPtr pScrn, int x, int y,
                                   int w, int h, int skipleft)
{
    NVPtr pNv = NVPTR(pScrn);
    int bpp = pScrn->bitsPerPixel/8;
    int bw = ((w * bpp) + 3) & ~3;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Subsequent\n"
	       "x %i\n"
	       "y %i\n"
	       "w %i\n"
	       "h %i\n"
	       "skip %i\n"
	       "bw %i\n"
	       "bpp %i\n",
	       x, y, w, h, skipleft, bw, bpp);

    pNv->expandWidth = bw >> 2;

    RIVA_FIFO_FREE(pNv->riva, Pixmap, 3);
    pNv->riva.Pixmap->TopLeft         = (y << 16) | (x & 0xFFFF);
    pNv->riva.Pixmap->WidthHeight     = (h << 16) | w;
    pNv->riva.Pixmap->WidthHeightIn   = (h << 16) | w;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "-Done\n");
}


static void
NVSubsequentImageWriteScanline(ScrnInfoPtr pScrn, int bufno)
{
    NVPtr pNv = NVPTR(pScrn);

    int t = pNv->expandWidth;
    unsigned int *pbits = (unsigned int*)pNv->expandBuffer;
    unsigned int *d = (unsigned int*)&pNv->riva.Pixmap->Pixels;

    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "Write %i\n", t);
    
    while (pNv->riva.Busy(&pNv->riva));
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "- Synced\n");

    do
    {
	RIVA_FIFO_FREE(pNv->riva, Pixmap, 1); 
	*d = *pbits++; 
    }while(--t);
    xf86DrvMsg(pScrn->scrnIndex, X_INFO, "-Done\n");
}



/* Initialize XAA acceleration info */
Bool
NVAccelInit(ScreenPtr pScreen) 
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    NVPtr pNv = NVPTR(pScrn);
    BoxRec AvailFBArea;
    
    pNv->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if(!infoPtr) return FALSE;

    /* fill out infoPtr here */
/*    infoPtr->Flags = 	PIXMAP_CACHE | 
			OFFSCREEN_PIXMAPS |
			LINEAR_FRAMEBUFFER |
			MICROSOFT_ZERO_LINE_BIAS;
*/
    infoPtr->Flags = 	LINEAR_FRAMEBUFFER | PIXMAP_CACHE | OFFSCREEN_PIXMAPS;

    /* Initialize pixmap cache */
    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = pNv->FbUsableSize / (pScrn->displayWidth * pScrn->bitsPerPixel / 8);
    xf86InitFBManager(pScreen, &AvailFBArea);
    
    /* sync */
    infoPtr->Sync = NVSync;

    /* solid fills */
    infoPtr->SolidFillFlags = NO_PLANEMASK;
    infoPtr->SetupForSolidFill = NVSetupForSolidFill;
    infoPtr->SubsequentSolidFillRect = NVSubsequentSolidFillRect;
/*    infoPtr->SubsequentSolidFillTrap = NVSubsequentSolidFillTrap; */

    /* screen to screen copy */
    infoPtr->ScreenToScreenCopyFlags = NO_TRANSPARENCY | NO_PLANEMASK;
    infoPtr->SetupForScreenToScreenCopy = NVSetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy = NVSubsequentScreenToScreenCopy;

    /* 8x8 mono patterns */
    /*
     * Set pattern opaque bits based on pixel format.
     */
    switch (pScrn->bitsPerPixel)
    {
        case 8:
            rivaOpaqueMonochrome = 0xFFFFFF00;
            break;
        case 15:
        case 16:
            rivaOpaqueMonochrome = (pScrn->weight.green == 5) ? 0xFFFF8000 : 0xFFFF0000;
            break;
        default:
            rivaOpaqueMonochrome = 0xFF000000;
    }

    infoPtr->Mono8x8PatternFillFlags = HARDWARE_PATTERN_SCREEN_ORIGIN |
        HARDWARE_PATTERN_PROGRAMMED_BITS | NO_PLANEMASK;
    infoPtr->SetupForMono8x8PatternFill = NVSetupForMono8x8PatternFill;
    infoPtr->SubsequentMono8x8PatternFillRect =
        NVSubsequentMono8x8PatternFillRect;
    /*infoPtr->SubsequentMono8x8PatternFillTrap = 
      NVSubsequentMono8x8PatternFillTrap; */

    /* clipping */
    infoPtr->SetClippingRectangle = NVSetClippingRectangle;
    infoPtr->DisableClipping = NVDisableClipping;
    infoPtr->ClippingFlags =    /*HARDWARE_CLIP_SOLID_FILL |
                                  HARDWARE_CLIP_MONO_8x8_FILL | */
				HARDWARE_CLIP_SCREEN_TO_SCREEN_COPY;

    /* Color expansion */
    infoPtr->ScanlineCPUToScreenColorExpandFillFlags = 
        BIT_ORDER_IN_BYTE_LSBFIRST | NO_PLANEMASK | CPU_TRANSFER_PAD_DWORD |
        LEFT_EDGE_CLIPPING | LEFT_EDGE_CLIPPING_NEGATIVE_X;

    infoPtr->NumScanlineColorExpandBuffers = 1;
    infoPtr->SetupForScanlineCPUToScreenColorExpandFill =
        NVSetupForScanlineCPUToScreenColorExpandFill;
    infoPtr->SubsequentScanlineCPUToScreenColorExpandFill = 
        NVSubsequentScanlineCPUToScreenColorExpandFill;

    pNv->expandFifo = (unsigned char*)&pNv->riva.Bitmap->MonochromeData01E;

    if ( (pNv->riva.Architecture == 3) || 1 )
    {
        /* Riva 128(ZX) can't use direct fifo writes, use buffer */
        pNv->expandBuffer = xnfalloc((pScrn->virtualX + 62)
                                     * pScrn->bitsPerPixel / 8);
        infoPtr->ScanlineColorExpandBuffers = &pNv->expandBuffer;
        infoPtr->SubsequentColorExpandScanline = 
            NVSubsequentColorExpandScanline;
    }
    else
    {
        /* TNT(2) can */
	pNv->expandBuffer = NULL;
        infoPtr->ScanlineColorExpandBuffers = &pNv->expandFifo;
        infoPtr->SubsequentColorExpandScanline = 
            NVSubsequentColorExpandScanlineFifo;
    }
#if 0
    /* Image writes */
    infoPtr->ScanlineImageWriteFlags = 	CPU_TRANSFER_PAD_DWORD |
                                        SCANLINE_PAD_DWORD |
/*                                        LEFT_EDGE_CLIPPING |
	                                LEFT_EDGE_CLIPPING_NEGATIVE_X; */
                                        NO_PLANEMASK | NO_TRANSPARENCY |
                                        NO_GXCOPY;

    infoPtr->SetupForScanlineImageWrite = NVSetupForScanlineImageWrite;
    infoPtr->SubsequentScanlineImageWriteRect =
        NVSubsequentScanlineImageWriteRect;
    infoPtr->SubsequentImageWriteScanline = NVSubsequentImageWriteScanline;
    
    /* We reuse the same buffer as color expansion */
    infoPtr->NumScanlineImageWriteBuffers = 1;
    infoPtr->ScanlineImageWriteBuffers = &pNv->expandBuffer;
#endif    
#if 0
    /* Left-overs from mga driver */
    /* solid lines */
    infoPtr->SetupForSolidLine = infoPtr->SetupForSolidFill;
    infoPtr->SubsequentSolidHorVertLine =
		NVNAME(SubsequentSolidHorVertLine);
    infoPtr->SubsequentSolidTwoPointLine = 
		NVNAME(SubsequentSolidTwoPointLine);


    /* dashed lines */
    infoPtr->DashedLineFlags = LINE_PATTERN_MSBFIRST_LSBJUSTIFIED;
    infoPtr->SetupForDashedLine = NVNAME(SetupForDashedLine);
    infoPtr->SubsequentDashedTwoPointLine =  
		NVNAME(SubsequentDashedTwoPointLine);
    infoPtr->DashPatternMaxLength = 128;


    /* cpu to screen color expansion */
    infoPtr->CPUToScreenColorExpandFillFlags = 	CPU_TRANSFER_PAD_DWORD |
					SCANLINE_PAD_DWORD |
					BIT_ORDER_IN_BYTE_LSBFIRST |
					LEFT_EDGE_CLIPPING |
					LEFT_EDGE_CLIPPING_NEGATIVE_X |
					SYNC_AFTER_COLOR_EXPAND;
    if(pNv->ILOADBase) {
	infoPtr->ColorExpandRange = 0x800000;
	infoPtr->ColorExpandBase = pNv->ILOADBase;
    } else {
	infoPtr->ColorExpandRange = 0x1C00;
	infoPtr->ColorExpandBase = pNv->IOBase;
    }
    infoPtr->SetupForCPUToScreenColorExpandFill =
		NVNAME(SetupForCPUToScreenColorExpandFill);
    infoPtr->SubsequentCPUToScreenColorExpandFill =
		NVNAME(SubsequentCPUToScreenColorExpandFill);


    /* screen to screen color expansion */
    if(pNv->AccelFlags & USE_LINEAR_EXPANSION) {
	infoPtr->ScreenToScreenColorExpandFillFlags = 
						BIT_ORDER_IN_BYTE_LSBFIRST;
	infoPtr->SetupForScreenToScreenColorExpandFill = 
		NVNAME(SetupForScreenToScreenColorExpandFill);
	infoPtr->SubsequentScreenToScreenColorExpandFill = 
		NVNAME(SubsequentScreenToScreenColorExpandFill);
    } else {
#if PSZ != 24
    /* Alternate (but slower) planar expansions */
	infoPtr->SetupForScreenToScreenColorExpandFill = 
		NVNAME(SetupForPlanarScreenToScreenColorExpandFill);
	infoPtr->SubsequentScreenToScreenColorExpandFill = 
		NVNAME(SubsequentPlanarScreenToScreenColorExpandFill);
	infoPtr->CacheColorExpandDensity = PSZ;
	infoPtr->CacheMonoStipple = XAACachePlanarMonoStipple;
	/* It's faster to blit the stipples if you have fastbilt */
	if(pNv->HasFBitBlt)
	    infoPtr->ScreenToScreenColorExpandFillFlags = TRANSPARENCY_ONLY;
#endif
    }

    /* image reads */
    infoPtr->ImageReadFlags = 	CPU_TRANSFER_PAD_DWORD |
				SCANLINE_PAD_DWORD;
    if(pNv->ILOADBase) {
	infoPtr->ImageReadRange = 0x800000;
	infoPtr->ImageReadBase = pNv->ILOADBase;
    } else {
	infoPtr->ImageReadRange = 0x1C00;
	infoPtr->ImageReadBase = pNv->IOBase;
    }
    infoPtr->SetupForImageRead = NVNAME(SetupForImageRead);
    infoPtr->SubsequentImageReadRect = NVNAME(SubsequentImageReadRect);

    /* midrange replacements */

    if(infoPtr->SetupForCPUToScreenColorExpandFill && 
			infoPtr->SubsequentCPUToScreenColorExpandFill) {
	infoPtr->FillColorExpandRects = NVFillColorExpandRects; 
	infoPtr->WriteBitmap = NVWriteBitmapColorExpand;
        if(BITMAP_SCANLINE_PAD == 32)
	    infoPtr->NonTEGlyphRenderer = NVNonTEGlyphRenderer;  
    }

    if(pNv->ILOADBase && pNv->UsePCIRetry && infoPtr->SetupForSolidFill) {
	infoPtr->FillSolidRects = NVFillSolidRectsDMA;
	infoPtr->FillSolidSpans = NVFillSolidSpansDMA;
    }

    if(pNv->AccelFlags & TWO_PASS_COLOR_EXPAND) {
	if(infoPtr->SetupForMono8x8PatternFill)
	    infoPtr->FillMono8x8PatternRects = 
				NVFillMono8x8PatternRectsTwoPass;
    }

    infoPtr->ValidatePolyArc = NVValidatePolyArc;
    infoPtr->PolyArcMask = GCFunction | GCLineWidth | GCPlaneMask | 
				GCLineStyle | GCFillStyle;

    if((PSZ == 24) || (pNv->AccelFlags & NV_NO_PLANEMASK)) {
	infoPtr->ImageWriteFlags |= NO_PLANEMASK;
	infoPtr->ScreenToScreenCopyFlags |= NO_PLANEMASK;
	infoPtr->CPUToScreenColorExpandFillFlags |= NO_PLANEMASK;
	infoPtr->WriteBitmapFlags |= NO_PLANEMASK;
	infoPtr->SolidFillFlags |= NO_PLANEMASK;
	infoPtr->SolidLineFlags |= NO_PLANEMASK;
	infoPtr->DashedLineFlags |= NO_PLANEMASK;
	infoPtr->Mono8x8PatternFillFlags |= NO_PLANEMASK; 
	infoPtr->FillColorExpandRectsFlags |= NO_PLANEMASK; 
	infoPtr->ScreenToScreenColorExpandFillFlags |= NO_PLANEMASK;
	infoPtr->FillSolidRectsFlags |= NO_PLANEMASK;
	infoPtr->FillSolidSpansFlags |= NO_PLANEMASK;
	infoPtr->FillMono8x8PatternRectsFlags |= NO_PLANEMASK;
	infoPtr->NonTEGlyphRendererFlags |= NO_PLANEMASK;
	infoPtr->FillCacheBltRectsFlags |= NO_PLANEMASK;
    }
#endif

    NVSetClippingRectangle(pScrn, 0, 0, 0x7fff, 0x7fff);
    return(XAAInit(pScreen, infoPtr));
}

