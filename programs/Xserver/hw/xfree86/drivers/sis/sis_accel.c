/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/sis/sis_accel.c,v 1.6 1999/04/17 07:06:53 dawes Exp $ */

#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

#include "xf86PciInfo.h"
#include "xf86Pci.h"

#include "miline.h"

#include "sis_regs.h"
#include "sis.h"

static void SiSSync(ScrnInfoPtr pScrn);

static void SiSSetupForFillRectSolid(ScrnInfoPtr pScrn, int color,
				int rop, unsigned int planemask);
static void SiSSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x,
				int y, int w, int h);

static void SiSSetupForScreenToScreenCopy(ScrnInfoPtr pScrn,
				int xdir, int ydir, int rop, 
                                unsigned int planemask,
				int transparency_color);
static void SiSSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn,
				int x1, int y1, int x2,
				int y2, int w, int h);

static void SiSSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int fg, int bg, 
				int rop, unsigned int planemask);
static void SiSSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, 
				int patternx, int patterny, int x, int y, 
				int w, int h);

static void SiSSetupForScreenToScreenColorExpandFill (ScrnInfoPtr pScrn,
       			int fg, int bg, 
				int rop, unsigned int planemask);

static void SiSSubsequentScreenToScreenColorExpandFill( ScrnInfoPtr pScrn,
				int x, int y, int w, int h,
				int srcx, int srcy, int offset );

static void
SiSInitializeAccelerator(ScrnInfoPtr pScrn)
{
    SISPtr pSiS = SISPTR(pScrn);
}

Bool
SiSAccelInit(ScreenPtr pScreen)
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    SISPtr pSiS = SISPTR(pScrn);
    BoxRec AvailFBArea;
    int offset;

    pSiS->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if (!infoPtr) return FALSE;

    SiSInitializeAccelerator(pScrn);

    infoPtr->Flags = PIXMAP_CACHE |
					 SYNC_AFTER_COLOR_EXPAND |
		     OFFSCREEN_PIXMAPS |
		     LINEAR_FRAMEBUFFER;
 
    infoPtr->Sync = SiSSync;

#if 1
    infoPtr->SolidFillFlags = NO_PLANEMASK;
    infoPtr->SetupForSolidFill = SiSSetupForFillRectSolid;
    infoPtr->SubsequentSolidFillRect = SiSSubsequentFillRectSolid;
#endif
    
    infoPtr->ScreenToScreenCopyFlags = NO_TRANSPARENCY | NO_PLANEMASK;
    infoPtr->SetupForScreenToScreenCopy = 	
				SiSSetupForScreenToScreenCopy;
    infoPtr->SubsequentScreenToScreenCopy = 		
				SiSSubsequentScreenToScreenCopy;

#if 1

    if (pScrn->bitsPerPixel != 24) {
	    infoPtr->Mono8x8PatternFillFlags =  GXCOPY_ONLY |
											CPU_TRANSFER_PAD_DWORD |
											SCANLINE_PAD_DWORD |
											NO_PLANEMASK | 
					HARDWARE_PATTERN_PROGRAMMED_BITS |
					HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
					BIT_ORDER_IN_BYTE_MSBFIRST;

    infoPtr->SetupForMono8x8PatternFill =
				SiSSetupForMono8x8PatternFill;
    infoPtr->SubsequentMono8x8PatternFillRect = 
				SiSSubsequentMono8x8PatternFillRect;
	};
#endif

#if 1
    if (pScrn->bitsPerPixel != 24) {
    infoPtr->ScreenToScreenColorExpandFillFlags =  GXCOPY_ONLY | 
					NO_PLANEMASK | 
					HARDWARE_PATTERN_PROGRAMMED_BITS |
					HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
					BIT_ORDER_IN_BYTE_MSBFIRST;

    infoPtr->SetupForScreenToScreenColorExpandFill =
				SiSSetupForScreenToScreenColorExpandFill;
    infoPtr->SubsequentScreenToScreenColorExpandFill = 
				SiSSubsequentScreenToScreenColorExpandFill;
    };
#endif

    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    offset = 0;
    if (pSiS->TurboQueue) offset = 32768;
    if (pSiS->HWCursor) offset = 16384;
    if (pSiS->HWCursor && pSiS->TurboQueue) offset = 65536;
    AvailFBArea.y2 = (pSiS->FbMapSize - offset) / (pScrn->displayWidth *
					    pScrn->bitsPerPixel / 8);

    xf86InitFBManager(pScreen, &AvailFBArea);

    return(XAAInit(pScreen, infoPtr));
}

static void 
SiSSync(ScrnInfoPtr pScrn) {
    SISPtr pSiS = SISPTR(pScrn);
    sisBLTSync;
}

static int sisALUConv[] =
{
    0x00,		       /* dest = 0; GXclear, 0 */
    0x88,		       /* dest &= src; GXand, 0x1 */
    0x44,		       /* dest = src & ~dest; GXandReverse, 0x2 */
    0xCC,		       /* dest = src; GXcopy, 0x3 */
    0x22,		       /* dest &= ~src; GXandInverted, 0x4 */
    0xAA,		       /* dest = dest; GXnoop, 0x5 */
    0x66,		       /* dest = ^src; GXxor, 0x6 */
    0xEE,		       /* dest |= src; GXor, 0x7 */
    0x11,		       /* dest = ~src & ~dest;GXnor, 0x8 */
    0x99,		       /*?? dest ^= ~src ;GXequiv, 0x9 */
    0x55,		       /* dest = ~dest; GXInvert, 0xA */
    0xDD,		       /* dest = src|~dest ;GXorReverse, 0xB */
    0x33,		       /* dest = ~src; GXcopyInverted, 0xC */
    0xBB,		       /* dest |= ~src; GXorInverted, 0xD */
    0x77,		       /*?? dest = ~src|~dest ;GXnand, 0xE */
    0xFF,		       /* dest = 0xFF; GXset, 0xF */
};

static void 
SiSSetupForFillRectSolid(ScrnInfoPtr pScrn, int color, int rop, 
			 unsigned int planemask)
{
    SISPtr pSiS = SISPTR(pScrn);

    sisSETFGCOLOR(color);
    sisSETROP(sisALUConv[rop & 0xF]);
    sisSETPITCH(pScrn->displayWidth * pScrn->bitsPerPixel / 8, 
		pScrn->displayWidth * pScrn->bitsPerPixel / 8);
    /*
     * If you don't support a write planemask, and have set the
     * appropriate flag, then the planemask can be safely ignored.
     * The same goes for the raster-op if only GXcopy is supported.
     */
    /*SETWRITEPLANEMASK(planemask);*/
}

static void 
SiSSubsequentFillRectSolid(ScrnInfoPtr pScrn, int x, int y, int w, int h)
{
    SISPtr pSiS = SISPTR(pScrn);
    int destaddr, op;

    destaddr = y * pScrn->displayWidth + x;
    op = sisCMDBLT | sisSRCFG | sisTOP2BOTTOM | sisLEFT2RIGHT;
    destaddr *= (pScrn->bitsPerPixel / 8);

    sisSETHEIGHTWIDTH(h-1, w * (pScrn->bitsPerPixel/8)-1);
    sisSETDSTADDR(destaddr);
    sisSETCMD(op);
    SiSSync(pScrn);
}

static void 
SiSSetupForScreenToScreenCopy(ScrnInfoPtr pScrn, int xdir, int ydir, 
				int rop, unsigned int planemask,
				int transparency_color)
{
    SISPtr pSiS = SISPTR(pScrn);
    sisSETPITCH(pScrn->displayWidth * pScrn->bitsPerPixel / 8, 
		pScrn->displayWidth * pScrn->bitsPerPixel / 8);
    sisSETROP(sisALUConv[rop & 0xF]);
    pSiS->Xdirection = xdir;
    pSiS->Ydirection = ydir;
}

static void 
SiSSubsequentScreenToScreenCopy(ScrnInfoPtr pScrn, int x1, int y1, int x2, 
				int y2, int w, int h)
{
    SISPtr pSiS = SISPTR(pScrn);
    int srcaddr, destaddr;
    int op ;

    op = sisCMDBLT | sisSRCVIDEO;
    if (pSiS->Ydirection == -1) {
	op |= sisBOTTOM2TOP;
        srcaddr = (y1 + h - 1) * pScrn->displayWidth;
	destaddr = (y2 + h - 1) * pScrn->displayWidth;
    } else {
	op |= sisTOP2BOTTOM;
        srcaddr = y1 * pScrn->displayWidth;
        destaddr = y2 * pScrn->displayWidth;
    }
    if (pSiS->Xdirection == -1) {
	op |= sisRIGHT2LEFT;
	srcaddr += x1 + w - 1;
	destaddr += x2 + w - 1;
    } else {
	op |= sisLEFT2RIGHT;
	srcaddr += x1;
	destaddr += x2;
    }
    srcaddr *= (pScrn->bitsPerPixel/8);
    destaddr *= (pScrn->bitsPerPixel/8);
    if ( ((pScrn->bitsPerPixel/8)>1) && (pSiS->Xdirection == -1) ) {
	srcaddr += (pScrn->bitsPerPixel/8)-1; 
	destaddr += (pScrn->bitsPerPixel/8)-1;
    }

    sisSETSRCADDR(srcaddr);
    sisSETDSTADDR(destaddr);
    sisSETHEIGHTWIDTH(h-1, w * (pScrn->bitsPerPixel/8)-1);
    sisSETCMD(op);
    SiSSync(pScrn);
}

static void 
SiSSetupForMono8x8PatternFill(ScrnInfoPtr pScrn, int patternx, int patterny, 
				int fg, int bg, int rop, unsigned int planemask)
{
    SISPtr pSiS = SISPTR(pScrn);
    unsigned int	*patternRegPtr ;
    int	       	i ;
    int 	dstpitch;
    int 	isTransparent = ( bg == -1 );

    dstpitch = pScrn->displayWidth * pScrn->bitsPerPixel / 8 ;
    /*
     * check transparency 
     */
    /* becareful with rop */
    if (isTransparent) {
	sisSETBGCOLOR(bg);
	sisSETFGCOLOR(fg);
	sisSETROPFG(0xf0); 	/* pat copy */
	sisSETROPBG(0xAA); 	/* dst */
    } else {
	sisSETBGCOLOR(bg);
	sisSETFGCOLOR(fg);
	sisSETROPFG(0xf0);	/* pat copy */
	sisSETROPBG(0xcc); 	/* copy */
    }
    sisBLTWAIT;
    sisSETPITCH(0, dstpitch);    
    sisSETSRCADDR(0);
    patternRegPtr =  (unsigned int *)sisSETPATREG();
    pSiS->sisPatternReg[0] = pSiS->sisPatternReg[2] = patternx ;
    pSiS->sisPatternReg[1] = pSiS->sisPatternReg[3] = patterny ;
    for ( i = 0 ; i < 16 /* sisPatternHeight */ ;   ) {
	    patternRegPtr[i++] = patternx ;
	    patternRegPtr[i++] = patterny ;
	}
}

static void 
SiSSubsequentMono8x8PatternFillRect(ScrnInfoPtr pScrn, int patternx, 
				int patterny, int x, int y, int w, int h)
{
    SISPtr pSiS = SISPTR(pScrn);
    int 		dstaddr;
    register unsigned char 	*patternRegPtr ;
    register unsigned char 	*srcPatternRegPtr ;    
    register unsigned int 	*patternRegPtrL ;
    int			i, k ;
    unsigned short 	tmp;
    int			shift ;
    int 	op  = sisCMDCOLEXP | sisTOP2BOTTOM | sisLEFT2RIGHT | 
	              sisPATFG | sisSRCBG ;

    dstaddr = ( y * pScrn->displayWidth + x ) * pScrn->bitsPerPixel / 8;
    sisBLTWAIT;
    patternRegPtr = sisSETPATREG();
    srcPatternRegPtr = (unsigned char *)pSiS->sisPatternReg ;
    shift = 8-patternx ;
    for ( i = 0, k = patterny ; i < 8 ; i++, k++ ) {
	tmp = srcPatternRegPtr[k]<<8 | srcPatternRegPtr[k] ;
	tmp >>= shift ;
	patternRegPtr[i] = tmp & 0xff ;
    }
    patternRegPtrL = (unsigned int *)sisSETPATREG();
    for ( i = 2 ; i < 16 /* sisPatternHeight */;   ) {
	patternRegPtrL[i++] = patternRegPtrL[0];
	patternRegPtrL[i++] = patternRegPtrL[1];
    }

    sisSETDSTADDR(dstaddr);
    sisSETHEIGHTWIDTH(h-1, w*(pScrn->bitsPerPixel/8)-1);
    sisSETCMD(op);
/*    SiSSync(pScrn);*/
}
/*
 * setup for screen-to-screen color expansion
 */
static void 
SiSSetupForScreenToScreenColorExpandFill (ScrnInfoPtr pScrn,
	int fg, int bg, 
	int rop, unsigned int planemask)
{
    SISPtr pSiS = SISPTR(pScrn);
    int isTransparent = ( bg == -1 );

    /*ErrorF("SISSetupScreenToScreenColorExpand()\n");*/

    /*
     * check transparency 
     */
    /* becareful with rop */
/*    sisBLTWAIT; */
    if (isTransparent) {
	sisSETFGCOLOR(fg);
	sisSETROPFG(0xf0); 	/* pat copy */
	sisSETROPBG(0xAA); 	/* dst */
    } else {
	sisSETBGCOLOR(bg);
	sisSETFGCOLOR(fg);
	sisSETROPFG(0xf0);	/* pat copy */
	sisSETROPBG(0xcc); 	/* copy */
    }
}

/*
 * executing screen-to-screen color expansion
 */
static void 
SiSSubsequentScreenToScreenColorExpandFill( ScrnInfoPtr pScrn,
				int x, int y, int w, int h,
				int srcx, int srcy, int offset )
{
    SISPtr pSiS = SISPTR(pScrn);
    int destpitch = pScrn->displayWidth * pScrn->bitsPerPixel / 8 ;
    int srcaddr = srcy * destpitch *  + srcx ;
    int destaddr = y * destpitch + x * pScrn->bitsPerPixel / 8;
    int srcpitch ;
    int ww ;
    int widthTodo ;
    int	op ;

    op  = sisCMDCOLEXP | sisTOP2BOTTOM | sisLEFT2RIGHT | sisPATFG | sisSRCBG | sisCMDENHCOLEXP ;

/*    ErrorF("SISSubsequentScreenToScreenColorExpand()\n"); */
#define maxWidth 144
    /* can't expand more than maxWidth in one time.
       it's a work around for scanline greater than maxWidth 
     */
    destpitch = pScrn->displayWidth * pScrn->bitsPerPixel / 8 ;
    srcpitch =  ((w + 31)& ~31) /8 ;
    sisSETPITCH(srcpitch, destpitch);
    widthTodo = w ;
    do { 
	ww = widthTodo < maxWidth ? widthTodo : maxWidth ;
	sisSETDSTADDR(destaddr);
	sisSETSRCADDR(srcaddr);
	sisSETHEIGHTWIDTH(h-1, ww*(pScrn->bitsPerPixel / 8)-1);
	sisSETCMD(op);
	srcaddr += ww ;
	destaddr += ww*pScrn->bitsPerPixel / 8 ;
	widthTodo -= ww ;
    } while ( widthTodo > 0 ) ;
/*    SiSSync(pScrn); */
}
