/* $XFree86: $ */

/* Copyright (c) 1998 The XFree86 Project, Inc. */

#include "compiler.h"
#include "vga256.h"
#include "xf86.h"
#include "vga.h"
#include "vgaPCI.h"

#include "miline.h"

#include "xf86xaa.h"
#include "xf86local.h"

#include "mga.h"
#include "mga_reg.h"
#include "mga_map.h"
#include "mga_macros.h"

extern CARD32 MGAAtype[16];
extern CARD32 MGAAtypeNoBLK[16];

extern void MGASetClippingRectangle();
extern void MGAWriteBitmap();
extern void MGAFillRectStippled();
extern Bool MGAIsClipped;
extern Bool MGAUsePCIRetry;
extern Bool MGAUseBLKOpaqueExpansion;
extern Bool MGAIsMillennium2;
extern Bool MGAOneTimeClip;
extern int  MGAMaxFastBlitY;

static void MGANAME(SetupForScreenToScreenCopy)();
static void MGANAME(SubsequentScreenToScreenCopy)();
static void MGANAME(SubsequentScreenToScreenCopy_FastBlit)();
static void MGANAME(SetupForFillRectSolid)();
static void MGANAME(SubsequentFillRectSolid)();
static void MGANAME(SubsequentFillTrapezoidSolid)();
static void MGANAME(SubsequentTwoPointLine)();
static void MGANAME(SetupForCPUToScreenColorExpand)();
static void MGANAME(SubsequentCPUToScreenColorExpand)();
static void MGANAME(SetupForScreenToScreenColorExpand)();
static void MGANAME(SubsequentScreenToScreenColorExpand)();
static void MGANAME(SetupForDashedLine)();
static void MGANAME(SubsequentDashedTwoPointLine)();
static void MGANAME(SetupFor8x8PatternColorExpand)();
static void MGANAME(Subsequent8x8PatternColorExpand)();
static void MGANAME(Subsequent8x8PatternColorExpand_Additional)();
static void MGANAME(Subsequent8x8TrapezoidColorExpand)();
static void MGANAME(SetupForImageWrite)();
static void MGANAME(SubsequentImageWrite)();
static void MGANAME(FillSpansSolid)();
static void MGANAME(FillSpansSolid_DMA)();

static CARD32 MGADashPattern[4];

void MGANAME(AccelInit)() 
{
    if(MGAchipset == PCI_CHIP_MGA2164_AGP || MGAchipset == PCI_CHIP_MGA2164)
	MGAIsMillennium2 = TRUE;

    if(OFLG_ISSET(OPTION_PCI_RETRY, &vga256InfoRec.options))
    	MGAUsePCIRetry = TRUE; 

    MGAUseBLKOpaqueExpansion = (MGAchipset != PCI_CHIP_MGA1064);

    xf86AccelInfoRec.Flags = 	BACKGROUND_OPERATIONS | 
				COP_FRAMEBUFFER_CONCURRENCY |
                             	DO_NOT_BLIT_STIPPLES |  
                            	LINE_PATTERN_MSBFIRST_LSBJUSTIFIED |
				PIXMAP_CACHE | 
				USE_TWO_POINT_LINE | 
				TWO_POINT_LINE_NOT_LAST |
				HARDWARE_CLIP_LINE | 
				MICROSOFT_ZERO_LINE_BIAS |
				DELAYED_SYNC;

    xf86AccelInfoRec.PatternFlags = HARDWARE_PATTERN_PROGRAMMED_BITS |
                             HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
                             HARDWARE_PATTERN_SCREEN_ORIGIN |
                             HARDWARE_PATTERN_BIT_ORDER_MSBFIRST |
                             HARDWARE_PATTERN_MONO_TRANSPARENCY;

    /* sync */
    xf86AccelInfoRec.Sync = MGAStormSync;

    /* screen to screen copy */
    xf86GCInfoRec.CopyAreaFlags = NO_TRANSPARENCY;
    xf86AccelInfoRec.SetupForScreenToScreenCopy =
        			MGANAME(SetupForScreenToScreenCopy);
    xf86AccelInfoRec.SubsequentScreenToScreenCopy =
        			MGANAME(SubsequentScreenToScreenCopy);

    /* solid filled rectangles */
    xf86AccelInfoRec.SetupForFillRectSolid = 
				MGANAME(SetupForFillRectSolid);
    xf86AccelInfoRec.SubsequentFillRectSolid =  
				MGANAME(SubsequentFillRectSolid);

    /* solid trapezoids */
    xf86AccelInfoRec.SubsequentFillTrapezoidSolid =
    				MGANAME(SubsequentFillTrapezoidSolid);

    /* solid lines */
    xf86AccelInfoRec.SetClippingRectangle = MGASetClippingRectangle;
    xf86AccelInfoRec.SubsequentTwoPointLine =
				MGANAME(SubsequentTwoPointLine);

    /* cpu to screen color expansion */
    xf86AccelInfoRec.ColorExpandFlags = SCANLINE_PAD_DWORD |
                                        CPU_TRANSFER_PAD_DWORD |
                                        BIT_ORDER_IN_BYTE_LSBFIRST |
					LEFT_EDGE_CLIPPING |
					LEFT_EDGE_CLIPPING_NEGATIVE_X; 
#ifdef __alpha__
    xf86AccelInfoRec.CPUToScreenColorExpandBase = (unsigned*)MGAMMIOBaseDENSE;
#else
    xf86AccelInfoRec.CPUToScreenColorExpandBase = (unsigned*)MGAMMIOBase;
#endif /* __alpha__ */
    xf86AccelInfoRec.CPUToScreenColorExpandRange = 0x1C00;

    xf86AccelInfoRec.SetupForCPUToScreenColorExpand = 
				MGANAME(SetupForCPUToScreenColorExpand);
    xf86AccelInfoRec.SubsequentCPUToScreenColorExpand = 
				MGANAME(SubsequentCPUToScreenColorExpand);

    xf86AccelInfoRec.SetupForScreenToScreenColorExpand =
    			MGANAME(SetupForScreenToScreenColorExpand);
    xf86AccelInfoRec.SubsequentScreenToScreenColorExpand =
    			MGANAME(SubsequentScreenToScreenColorExpand);

    /* dashed lines */
    xf86AccelInfoRec.SetupForDashedLine = MGANAME(SetupForDashedLine);
    xf86AccelInfoRec.SubsequentDashedTwoPointLine =
				MGANAME(SubsequentDashedTwoPointLine);
    xf86AccelInfoRec.LinePatternBuffer = (void *)MGADashPattern;
    xf86AccelInfoRec.LinePatternMaxLength = 128;

    /* 8x8 pattern fills */
    xf86AccelInfoRec.SetupFor8x8PatternColorExpand =
    			MGANAME(SetupFor8x8PatternColorExpand);
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand =
    			MGANAME(Subsequent8x8PatternColorExpand);
    xf86AccelInfoRec.Subsequent8x8TrapezoidColorExpand = 
			MGANAME(Subsequent8x8TrapezoidColorExpand);

    /* image writes */
    xf86AccelInfoRec.ImageWriteBase = 
			xf86AccelInfoRec.CPUToScreenColorExpandBase;
    xf86AccelInfoRec.ImageWriteRange = 
			xf86AccelInfoRec.CPUToScreenColorExpandRange;
    xf86AccelInfoRec.SetupForImageWrite = MGANAME(SetupForImageWrite);
    xf86AccelInfoRec.SubsequentImageWrite = MGANAME(SubsequentImageWrite);
    xf86AccelInfoRec.ImageWriteFlags = 	NO_TRANSPARENCY |
					LEFT_EDGE_CLIPPING |
					LEFT_EDGE_CLIPPING_NEGATIVE_X; 

    /* solid spans */
    xf86GCInfoRec.FillSpansSolid = xf86FillSpans;
    if((MGAchipset == PCI_CHIP_MGA2064) || !MGAUsePCIRetry)
    	xf86AccelInfoRec.FillSpansSolid = MGANAME(FillSpansSolid);
    else 
    	xf86AccelInfoRec.FillSpansSolid = MGANAME(FillSpansSolid_DMA);

    /* replacements */
    xf86AccelInfoRec.WriteBitmap = MGAWriteBitmap;
    xf86AccelInfoRec.FillRectStippled = MGAFillRectStippled;
    xf86AccelInfoRec.FillRectOpaqueStippled = MGAFillRectStippled; 
    xf86AccelInfoRec.FillRectTiled = xf86FillRectTiledImageWrite;

#if PSZ == 24
    /* Shotgun approach.  XAA should do some of these on it's own
	but it isn't getting all of them */
    xf86GCInfoRec.CopyAreaFlags |= NO_PLANEMASK;
    xf86GCInfoRec.FillPolygonSolidFlags |= NO_PLANEMASK;

    xf86GCInfoRec.PolyRectangleSolidZeroWidthFlags |= NO_PLANEMASK;
    xf86GCInfoRec.PolyLineSolidZeroWidthFlags |= NO_PLANEMASK;
    xf86GCInfoRec.PolySegmentSolidZeroWidthFlags |= NO_PLANEMASK;
    xf86GCInfoRec.PolyLineDashedZeroWidthFlags |= NO_PLANEMASK;
    xf86GCInfoRec.PolySegmentDashedZeroWidthFlags |= NO_PLANEMASK;

    xf86GCInfoRec.PolyGlyphBltNonTEFlags |= NO_PLANEMASK;
    xf86GCInfoRec.ImageGlyphBltNonTEFlags |= NO_PLANEMASK;
    xf86GCInfoRec.PolyGlyphBltTEFlags |= NO_PLANEMASK;
    xf86GCInfoRec.ImageGlyphBltTEFlags |= NO_PLANEMASK;

    xf86GCInfoRec.FillSpansSolidFlags |= NO_PLANEMASK;
    xf86GCInfoRec.FillSpansTiledFlags |= NO_PLANEMASK;
    xf86GCInfoRec.FillSpansStippledFlags |= NO_PLANEMASK;
    xf86GCInfoRec.FillSpansOpaqueStippledFlags |= NO_PLANEMASK;

    xf86GCInfoRec.PolyFillRectSolidFlags |= NO_PLANEMASK;
    xf86GCInfoRec.PolyFillRectTiledFlags |= NO_PLANEMASK;
    xf86GCInfoRec.PolyFillRectStippledFlags |= NO_PLANEMASK;
    xf86GCInfoRec.PolyFillRectOpaqueStippledFlags |= NO_PLANEMASK;

    xf86GCInfoRec.PolyFillArcSolidFlags |= NO_PLANEMASK;

    xf86AccelInfoRec.ImageWriteFlags |= NO_PLANEMASK;
    xf86AccelInfoRec.ColorExpandFlags |= NO_PLANEMASK;
#endif

    /* pixmap cache */
    {
	int cacheStart, cacheEnd, maxFastBlitMem;

	maxFastBlitMem = (MGAinterleave ? 4096 : 2048) * 1024;
	cacheStart = (vga256InfoRec.virtualY * vga256InfoRec.displayWidth
                            + MGAydstorg) * PSZ / 8;

 	cacheEnd = cacheStart + (128 * vga256InfoRec.displayWidth * PSZ / 8);

	if(cacheEnd > (vga256InfoRec.videoRam * 1024))
	    cacheEnd = (vga256InfoRec.videoRam * 1024);

	if(cacheEnd > maxFastBlitMem) {
	     MGAMaxFastBlitY = maxFastBlitMem / 
			(vga256InfoRec.displayWidth  * PSZ / 8);
    	}

   	xf86AccelInfoRec.PixmapCacheMemoryStart = cacheStart;  
    	xf86AccelInfoRec.PixmapCacheMemoryEnd = cacheEnd;
    }
}



#if PSZ == 8

/*
   GXclear, GXand, 
   GXandReverse, GXcopy,
   GXandInverted, GXnoop, 
   GXxor, GXor, 
   GXnor, GXequiv, 
   GXinvert, GXorReverse, 
   GXcopyInverted, GXorInverted, 
   GXnand, GXset
*/


CARD32 MGAAtype[16] = {
   MGADWG_RPL  | 0x00000000, MGADWG_RSTR | 0x00080000, 
   MGADWG_RSTR | 0x00040000, MGADWG_BLK  | 0x000c0000,
   MGADWG_RSTR | 0x00020000, MGADWG_RSTR | 0x000a0000, 
   MGADWG_RSTR | 0x00060000, MGADWG_RSTR | 0x000e0000,
   MGADWG_RSTR | 0x00010000, MGADWG_RSTR | 0x00090000, 
   MGADWG_RSTR | 0x00050000, MGADWG_RSTR | 0x000d0000,
   MGADWG_RPL  | 0x00030000, MGADWG_RSTR | 0x000b0000, 
   MGADWG_RSTR | 0x00070000, MGADWG_RPL  | 0x000f0000
};


CARD32 MGAAtypeNoBLK[16] = {
   MGADWG_RPL  | 0x00000000, MGADWG_RSTR | 0x00080000, 
   MGADWG_RSTR | 0x00040000, MGADWG_RPL  | 0x000c0000,
   MGADWG_RSTR | 0x00020000, MGADWG_RSTR | 0x000a0000, 
   MGADWG_RSTR | 0x00060000, MGADWG_RSTR | 0x000e0000,
   MGADWG_RSTR | 0x00010000, MGADWG_RSTR | 0x00090000, 
   MGADWG_RSTR | 0x00050000, MGADWG_RSTR | 0x000d0000,
   MGADWG_RPL  | 0x00030000, MGADWG_RSTR | 0x000b0000, 
   MGADWG_RSTR | 0x00070000, MGADWG_RPL  | 0x000f0000
};

void 
MGAStormAccelInit() {
    switch( vgaBitsPerPixel )
    {
    case 8:
    	Mga8AccelInit();
    	break;
    case 16:
    	Mga16AccelInit();
    	break;
    case 24:
    	Mga24AccelInit();
    	break;
    case 32:
    	Mga32AccelInit();
    	break;
    }
}

Bool MGAIsClipped = FALSE;
Bool MGAOneTimeClip = FALSE;
Bool MGAUsePCIRetry = FALSE;
Bool MGAUseBLKOpaqueExpansion;
Bool MGAIsMillennium2 = FALSE;
int  MGAMaxFastBlitY = 0;

void 
MGAStormSync()
{
    if(MGAIsClipped) { 
	/* we don't need to sync after a cpu->screen color expand.
		we merely need to reset the clipping box */
	WAITFIFO(1);
    	OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);
	MGAIsClipped = FALSE;
    } else while(MGAISBUSY());

    /* flush cache before a read (mga-1064g 5.1.6) */
    OUTREG8(MGAREG_CRTC_INDEX, 0); 
}

void 
MGAStormEngineInit()
{
    CARD32 maccess = 0;
    
    switch( vgaBitsPerPixel )
    {
    case 8:
        break;
    case 16:
	/* set 16 bpp, turn off dithering, turn on 5:5:5 pixels */
        maccess = 1 + (1 << 30) + (1 << 31);
        break;
    case 24:
        maccess = 3;
        break;
    case 32:
        maccess = 2;
        break;
    }

    WAITFIFO(8);
    OUTREG(MGAREG_PITCH, vga256InfoRec.displayWidth);
    OUTREG(MGAREG_YDSTORG, MGAydstorg);
    OUTREG(MGAREG_MACCESS, maccess);
    OUTREG(MGAREG_PLNWT, ~0);
    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_BLIT);

    /* put clipping in a know state */
    OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);	/* (maxX << 16) | minX */ 
    OUTREG(MGAREG_YTOP, 0x00000000);	/* minPixelPointer */ 
    OUTREG(MGAREG_YBOT, 0x007FFFFF);	/* maxPixelPointer */ 
}

void MGASetClippingRectangle(x1, y1, x2, y2)
   int x1, y1, x2, y2;
{
    WAITFIFO(3);
    OUTREG(MGAREG_CXBNDRY,(x2 << 16) | x1);	 
    OUTREG(MGAREG_YTOP, (y1 * xf86AccelInfoRec.FramebufferWidth) + MGAydstorg);	
    OUTREG(MGAREG_YBOT, (y2 * xf86AccelInfoRec.FramebufferWidth) + MGAydstorg);	
    MGAOneTimeClip = TRUE;
}


#endif /* PSZ == 8 */




	/*********************************************\
	|            Screen-to-Screen Copy            |
	\*********************************************/

#define BLIT_LEFT	1
#define BLIT_UP		4

static CARD32 BltScanDirection;

void 
MGANAME(SetupForScreenToScreenCopy)(xdir, ydir, rop, planemask, trans_color)
    int xdir, ydir;
    int rop;
    unsigned planemask;
    int trans_color;
{
    xf86AccelInfoRec.SubsequentScreenToScreenCopy = 
		MGANAME(SubsequentScreenToScreenCopy);
    REPLICATE(planemask);
    BltScanDirection = 0;
    if(ydir == -1) BltScanDirection |= BLIT_UP;
    WAITFIFO(4);
    if(xdir == -1) {
	BltScanDirection |= BLIT_LEFT;
    	OUTREG(MGAREG_DWGCTL, MGAAtypeNoBLK[rop] | MGADWG_SHIFTZERO | 
			MGADWG_BITBLT | MGADWG_BFCOL);
    } else {
	if(MGAusefbitblt && (rop == GXcopy)) 
	   xf86AccelInfoRec.SubsequentScreenToScreenCopy = 
		MGANAME(SubsequentScreenToScreenCopy_FastBlit);
    	OUTREG(MGAREG_DWGCTL, MGAAtypeNoBLK[rop] | MGADWG_SHIFTZERO | 
			MGADWG_BITBLT | MGADWG_BFCOL);
    }
    OUTREG(MGAREG_SGN, BltScanDirection);
#if PSZ != 24
    OUTREG(MGAREG_PLNWT, planemask);
#endif
    OUTREG(MGAREG_AR5, ydir * xf86AccelInfoRec.FramebufferWidth);
}


void 
MGANAME(SubsequentScreenToScreenCopy)(srcX, srcY, dstX, dstY, w, h)
    int srcX, srcY, dstX, dstY, w, h;
{
    register int start, end;
 
    if(BltScanDirection & BLIT_UP) {
	srcY += h - 1;
	dstY += h - 1;
    }

    w--;
    start = end = XYADDRESS(srcX, srcY);

    if(BltScanDirection & BLIT_LEFT) start += w;
    else end += w; 
   
    WAITFIFO(4);
    OUTREG(MGAREG_AR0, end);
    OUTREG(MGAREG_AR3, start);
    OUTREG(MGAREG_FXBNDRY, ((dstX + w) << 16) | dstX);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (dstY << 16) | h);
}

void 
MGANAME(SubsequentScreenToScreenCopy_FastBlit)(srcX, srcY, dstX, dstY, w, h)
    int srcX, srcY, dstX, dstY, w, h;
{
    register int start, end;

    if(BltScanDirection & BLIT_UP) {
	srcY += h - 1;
	dstY += h - 1;
    }

    w--;
    start = XYADDRESS(srcX, srcY);
    end = start + w;

    /* we assume the driver asserts screen pitches such that
	we can always use fastblit for scrolling */
    if(
#if PSZ == 32
        !((srcX ^ dstX) & 31)
#elif PSZ == 16
        !((srcX ^ dstX) & 63)
#else
        !((srcX ^ dstX) & 127)
#endif
    	) {
	if(MGAMaxFastBlitY) {
	   if(((srcY + h) > MGAMaxFastBlitY) ||
				((dstY + h) > MGAMaxFastBlitY)) 
	   goto FASTBLIT_BAILOUT;
	}

	/* Millennium 1 fastblit bug fix */
        if(!MGAIsMillennium2) {
    	   int fxright = dstX + w;
#if PSZ == 8
           if((dstX & (1 << 6)) && (((fxright >> 6) - (dstX >> 6)) & 7) == 7) {
		fxright |= 1 << 6;
#elif PSZ == 16
           if((dstX & (1 << 5)) && (((fxright >> 5) - (dstX >> 5)) & 7) == 7) {
		fxright |= 1 << 5;
#elif PSZ == 24
           if(((dstX * 3) & (1 << 6)) && 
                 ((((fxright * 3 + 2) >> 6) - ((dstX * 3) >> 6)) & 7) == 7) {
		fxright = ((fxright * 3 + 2) | (1 << 6)) / 3;
#elif PSZ == 32
           if((dstX & (1 << 4)) && (((fxright >> 4) - (dstX >> 4)) & 7) == 7) {
		fxright |= 1 << 4;
#endif
		WAITFIFO(8);
		OUTREG(MGAREG_CXRIGHT, dstX + w);
		OUTREG(MGAREG_DWGCTL, 0x040A400C);
		OUTREG(MGAREG_AR0, end);
		OUTREG(MGAREG_AR3, start);
		OUTREG(MGAREG_FXBNDRY, (fxright << 16) | dstX);
		OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (dstY << 16) | h);
		OUTREG(MGAREG_DWGCTL, MGAAtypeNoBLK[GXcopy] | 
			MGADWG_SHIFTZERO | MGADWG_BITBLT | MGADWG_BFCOL);
		OUTREG(MGAREG_CXRIGHT, 0xFFFF);
	    	return;
	    }
	}
	
    	WAITFIFO(6);
    	OUTREG(MGAREG_DWGCTL, 0x040A400C);
    	OUTREG(MGAREG_AR0, end);
    	OUTREG(MGAREG_AR3, start);
    	OUTREG(MGAREG_FXBNDRY, ((dstX + w) << 16) | dstX);
    	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (dstY << 16) | h);
    	OUTREG(MGAREG_DWGCTL, MGAAtypeNoBLK[GXcopy] | MGADWG_SHIFTZERO | 
			MGADWG_BITBLT | MGADWG_BFCOL);
	return;
    }  

FASTBLIT_BAILOUT:
   
    WAITFIFO(4);
    OUTREG(MGAREG_AR0, end);
    OUTREG(MGAREG_AR3, start);
    OUTREG(MGAREG_FXBNDRY, ((dstX + w) << 16) | dstX);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (dstY << 16) | h);
}


	/*********************************************\
	|            Solid Filled Rectangles          |
	\*********************************************/

static CARD32 FilledRectCMD;
static CARD32 SolidLineCMD;

void 
MGANAME(SetupForFillRectSolid)(color, rop, planemask)
    int color, rop;
    unsigned planemask;
{
#if PSZ == 24
    if(!RGBEQUAL(color))
    FilledRectCMD = MGADWG_TRAP | MGADWG_SOLID | MGADWG_ARZERO | 
		    MGADWG_SGNZERO | MGADWG_SHIFTZERO | 
		    MGADWG_BMONOLEF | MGAAtypeNoBLK[rop];
    else
#endif
    FilledRectCMD = MGADWG_TRAP | MGADWG_SOLID | MGADWG_ARZERO | 
		    MGADWG_SGNZERO | MGADWG_SHIFTZERO | 
		    MGADWG_BMONOLEF | MGAAtype[rop];

    SolidLineCMD =  MGADWG_SOLID | MGADWG_SHIFTZERO | MGADWG_BFCOL | 
		    MGAAtypeNoBLK[rop];

    if(MGAIsMillennium2) FilledRectCMD |= MGADWG_TRANSC;

    REPLICATE24(color);
    REPLICATE(planemask);
    WAITFIFO(3);
    OUTREG(MGAREG_FCOL, color);
#if PSZ != 24
    OUTREG(MGAREG_PLNWT, planemask);
#endif
    OUTREG(MGAREG_DWGCTL, FilledRectCMD);
}


void 
MGANAME(SubsequentFillRectSolid)(x, y, w, h)
    int x, y, w, h;
{
    WAITFIFO(2);
    OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | x);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}

	/******************************\
	|       Solid Trapezoids       |
	\******************************/


void 
MGANAME(SubsequentFillTrapezoidSolid)(y, h, left, dxL, dyL, eL, 
						right, dxR, dyR, eR)
    int y, h, left, dxL, dyL, eL, right, dxR, dyR, eR;
{
    int sdxl = (dxL < 0);
    int ar2 = sdxl? dxL : -dxL;
    int sdxr = (dxR < 0);
    int ar5 = sdxr? dxR : -dxR;
    
    WAITFIFO(11);
    OUTREG(MGAREG_DWGCTL, FilledRectCMD & ~(MGADWG_ARZERO | MGADWG_SGNZERO));
    OUTREG(MGAREG_AR0, dyL);
    OUTREG(MGAREG_AR1, ar2 - eL);
    OUTREG(MGAREG_AR2, ar2);
    OUTREG(MGAREG_AR4, ar5 - eR);
    OUTREG(MGAREG_AR5, ar5);
    OUTREG(MGAREG_AR6, dyR);
    OUTREG(MGAREG_SGN, (sdxl << 1) | (sdxr << 5));
    OUTREG(MGAREG_FXBNDRY, ((right + 1) << 16) | left);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
    OUTREG(MGAREG_DWGCTL, FilledRectCMD);
}


	/********************************\
	|         Two Point Lines        |
	\********************************/

void
MGANAME(SubsequentTwoPointLine)(x1, y1, x2, y2, bias)
   int x1, y1, x2, y2, bias;
{
    if(MGAIsMillennium2 && (x1 == x2)) {
    	WAITFIFO(2);
    	OUTREG(MGAREG_FXBNDRY, ((x1 + 1) << 16) | x1);
    	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y1 << 16) | (y2 - y1 + 1));
	return;
    }

    WAITFIFO(4);
    OUTREG(MGAREG_DWGCTL, SolidLineCMD | 
	((bias & 0x100) ? MGADWG_AUTOLINE_OPEN : MGADWG_AUTOLINE_CLOSE));
    OUTREG(MGAREG_XYSTRT, (y1 << 16) | x1);
    OUTREG(MGAREG_XYEND + MGAREG_EXEC, (y2 << 16) | x2);
    OUTREG(MGAREG_DWGCTL, FilledRectCMD); 
   
    if(MGAOneTimeClip) {
    	WAITFIFO(3);
	OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);	/* (maxX << 16) | minX */ 
	OUTREG(MGAREG_YTOP, 0x00000000);	/* minPixelPointer */ 
	OUTREG(MGAREG_YBOT, 0x007FFFFF);	/* maxPixelPointer */ 
	MGAOneTimeClip = FALSE;
    }
}


	/********************************************\
	|        CPU to Screen Color Expansion       |
	\********************************************/

void 
MGANAME(SetupForCPUToScreenColorExpand)(bg, fg, rop, planemask)
    int bg, fg, rop;
    unsigned planemask;
{
    CARD32 mgaCMD = MGADWG_ILOAD | MGADWG_LINEAR | MGADWG_SGNZERO | 
			MGADWG_SHIFTZERO | MGADWG_BMONOLEF;
        
    REPLICATE24(fg);
    REPLICATE(planemask);

    if(bg == -1) {
#if PSZ == 24
    	if(!RGBEQUAL(fg))
            mgaCMD |= MGADWG_TRANSC | MGAAtypeNoBLK[rop];
	else
#endif
            mgaCMD |= MGADWG_TRANSC | MGAAtype[rop];

	WAITFIFO(3);
    } else {
#if PSZ == 24
	if(MGAUseBLKOpaqueExpansion && RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
	if(MGAUseBLKOpaqueExpansion) 
#endif
        	mgaCMD |= MGAAtype[rop];
	else
        	mgaCMD |= MGAAtypeNoBLK[rop];
        REPLICATE24(bg);
	WAITFIFO(4);
    	OUTREG(MGAREG_BCOL, bg);
    }

    OUTREG(MGAREG_FCOL, fg);
#if PSZ != 24
    OUTREG(MGAREG_PLNWT, planemask);
#endif
    OUTREG(MGAREG_DWGCTL, mgaCMD);
}

void MGANAME(SubsequentCPUToScreenColorExpand)(x, y, w, h, skipleft)
    int x, y, w, h, skipleft;
{
    MGAIsClipped = TRUE;
    WAITFIFO(5);
    OUTREG(MGAREG_CXBNDRY, ((x + w - 1) << 16) | ((x + skipleft) & 0xFFFF));
    w = (w + 31) & ~31;     /* source is dword padded */
    OUTREG(MGAREG_AR0, (w * h) - 1);
    OUTREG(MGAREG_AR3, 0);  /* crashes occasionally without this */
    OUTREG(MGAREG_FXBNDRY, ((x + w - 1) << 16) | (x & 0xFFFF));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}

	/***************************************\
	|   Screen to Screen Color Expansion	|
	\***************************************/

void 
MGANAME(SetupForScreenToScreenColorExpand)(bg, fg, rop, planemask)
    int bg, fg, rop;
    unsigned planemask;
{
    CARD32 mgaCMD = MGADWG_BITBLT | MGADWG_SGNZERO | MGADWG_SHIFTZERO;
        
    REPLICATE24(fg);
    REPLICATE(planemask);

    if(bg == -1) {
#if PSZ == 24
    	if(!RGBEQUAL(fg))
            mgaCMD |= MGADWG_TRANSC | MGAAtypeNoBLK[rop];
	else
#endif
            mgaCMD |= MGADWG_TRANSC | MGAAtype[rop];

	WAITFIFO(4);
    } else {
#if PSZ == 24
	if(MGAUseBLKOpaqueExpansion && RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
	if(MGAUseBLKOpaqueExpansion) 
#endif
        	mgaCMD |= MGAAtype[rop];
	else
        	mgaCMD |= MGAAtypeNoBLK[rop];
        REPLICATE24(bg);
	WAITFIFO(5);
    	OUTREG(MGAREG_BCOL, bg);
    }

    OUTREG(MGAREG_FCOL, fg);
#if PSZ != 24
    OUTREG(MGAREG_PLNWT, planemask);
#endif
    OUTREG(MGAREG_AR5, xf86AccelInfoRec.FramebufferWidth * PSZ);
    OUTREG(MGAREG_DWGCTL, mgaCMD);
}

void MGANAME(SubsequentScreenToScreenColorExpand)(srcx, srcy, x, y, w, h)
    int srcx, srcy, x, y, w, h;
{
    register int start, end;

    start = (XYADDRESS(0, srcy) * PSZ) + srcx;
    end = start + w - 1;

    WAITFIFO(4);
    OUTREG(MGAREG_AR3, start);
    OUTREG(MGAREG_AR0, end);
    OUTREG(MGAREG_FXBNDRY, ((x + w - 1) << 16) | x);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}


	/***************************\
	|      Dashed  Lines        |
	\***************************/

static int mgaStylelen;
static Bool NiceDashPattern;
static CARD32 NiceDashCMD;
static CARD32 DashCMD;

void
MGANAME(SetupForDashedLine)(fg, bg, rop, planemask, size)
  int fg, bg, rop;
  unsigned int planemask;
  int size;
{
    int dwords = (size + 31) >> 5;
    DashCMD = MGADWG_BFCOL | MGAAtypeNoBLK[rop];

    mgaStylelen = size - 1;
    REPLICATE(fg);
    REPLICATE(planemask);

    /* We see if we can draw horizontal lines as 8x8 pattern fills.
	This is worthwhile since the pattern fills can use block mode
	and the default X pattern is 8 pixels long.  The forward pattern
	is the top scanline, the backwards pattern is the next one. */
    switch(size) {
	case 2:	MGADashPattern[0] |= MGADashPattern[0] << 2;
	case 4:	MGADashPattern[0] |= MGADashPattern[0] << 4;
	case 8:	MGADashPattern[0] &= 0xFF;
		MGADashPattern[0] |= byte_reversed[MGADashPattern[0]] << 16;
		MGADashPattern[0] |= MGADashPattern[0] << 8;
		NiceDashCMD = MGADWG_TRAP | MGADWG_ARZERO | MGADWG_SGNZERO | 
				 MGADWG_BMONOLEF;
     		NiceDashPattern = TRUE;
   		if(bg == -1) {
#if PSZ == 24
    		   if(!RGBEQUAL(fg))
            		NiceDashCMD |= MGADWG_TRANSC | MGAAtypeNoBLK[rop];
		   else
#endif
           		NiceDashCMD |= MGADWG_TRANSC | MGAAtype[rop];
    		} else {
#if PSZ == 24
		   if(MGAUseBLKOpaqueExpansion && RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
		   if(MGAUseBLKOpaqueExpansion) 
#endif
        		NiceDashCMD |= MGAAtype[rop];
		   else
        		NiceDashCMD |= MGAAtypeNoBLK[rop];
    		}
		break;
	default: NiceDashPattern = FALSE;
    }

    if(bg == -1) {
        DashCMD |= MGADWG_TRANSC;
	WAITFIFO(dwords + 2);
    } else {
        REPLICATE(bg);
	WAITFIFO(dwords + 3);
    	OUTREG(MGAREG_BCOL, bg);
    }

#if PSZ != 24
    OUTREG(MGAREG_PLNWT, planemask);
#endif
    OUTREG(MGAREG_FCOL, fg);

    switch (dwords) {
	case 4:  OUTREG(MGAREG_SRC3, MGADashPattern[3]);
	case 3:  OUTREG(MGAREG_SRC2, MGADashPattern[2]);
	case 2:	 OUTREG(MGAREG_SRC1, MGADashPattern[1]);
	default: OUTREG(MGAREG_SRC0, MGADashPattern[0]);
    }
}

	/************************************\
	|      Dashed Two Point Lines        |
	\************************************/


void
MGANAME(SubsequentDashedTwoPointLine)(x1, y1, x2, y2, bias, start)
   int x1, y1, x2, y2, bias, start;
{

    if(NiceDashPattern && (y1 == y2)) {
    	WAITFIFO(4);
    	OUTREG(MGAREG_DWGCTL, NiceDashCMD);
	if(x2 < x1) {
	   if(bias & 0x100) x2++;
   	   OUTREG(MGAREG_SHIFT, ((-y1 & 0x07) << 4) | 
				((x2 - start) & 0x07)); 
   	   OUTREG(MGAREG_FXBNDRY, ((x1 + 1) << 16) | x2);
    	} else {
 	   if(!(bias & 0x100)) x2++;
   	   OUTREG(MGAREG_SHIFT, (((1 - y1) & 0x07) << 4) | 
				((start - x1) & 0x07)); 
     	   OUTREG(MGAREG_FXBNDRY, (x2 << 16) | x1);
	}	
    	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y1 << 16) | 1);
	return;
    }

    WAITFIFO(4);
    OUTREG(MGAREG_SHIFT, (mgaStylelen << 16 ) | (mgaStylelen - start)); 
    OUTREG(MGAREG_DWGCTL, DashCMD | 
	((bias & 0x100) ? MGADWG_AUTOLINE_OPEN : MGADWG_AUTOLINE_CLOSE));
    OUTREG(MGAREG_XYSTRT, (y1 << 16) | x1);
    OUTREG(MGAREG_XYEND + MGAREG_EXEC, (y2 << 16) | x2);
   
    if(MGAOneTimeClip) {
    	WAITFIFO(3);
	OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);	/* (maxX << 16) | minX */ 
	OUTREG(MGAREG_YTOP, 0x00000000);	/* minPixelPointer */ 
	OUTREG(MGAREG_YBOT, 0x007FFFFF);	/* maxPixelPointer */ 
	MGAOneTimeClip = FALSE;
    }
}


   		/****************************\
		|    8x8 Rectangular Fill    |
		\****************************/

static CARD32 PatternRectCMD;

void 
MGANAME(SetupFor8x8PatternColorExpand)(patternx, patterny, bg, fg,
                                            rop, planemask)
    unsigned patternx, patterny, planemask;
    int bg, fg, rop;
{
    PatternRectCMD = MGADWG_TRAP | MGADWG_ARZERO | MGADWG_SGNZERO | 
						MGADWG_BMONOLEF;

    xf86AccelInfoRec.Subsequent8x8PatternColorExpand = 
		MGANAME(Subsequent8x8PatternColorExpand);

    REPLICATE24(fg);
    REPLICATE(planemask);
    
    if(bg == -1) {
#if PSZ == 24
    	if(!RGBEQUAL(fg))
            PatternRectCMD |= MGADWG_TRANSC | MGAAtypeNoBLK[rop];
	else
#endif
            PatternRectCMD |= MGADWG_TRANSC | MGAAtype[rop];

	WAITFIFO(5);
    } else {
#if PSZ == 24
	if(MGAUseBLKOpaqueExpansion && RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
	if(MGAUseBLKOpaqueExpansion) 
#endif
        	PatternRectCMD |= MGAAtype[rop];
	else
        	PatternRectCMD |= MGAAtypeNoBLK[rop];
        REPLICATE24(bg);
	WAITFIFO(6);
    	OUTREG(MGAREG_BCOL, bg);
    }

    OUTREG(MGAREG_FCOL, fg);
#if PSZ != 24
    OUTREG(MGAREG_PLNWT, planemask);
#endif
    OUTREG(MGAREG_DWGCTL, PatternRectCMD);
    OUTREG(MGAREG_PAT0, patternx);
    OUTREG(MGAREG_PAT1, patterny);
}

void 
MGANAME(Subsequent8x8PatternColorExpand)(patternx, patterny, x, y, w, h)
    unsigned patternx, patterny;
    int x, y, w, h;
{
    WAITFIFO(3);
    OUTREG(MGAREG_SHIFT, (patterny << 4) | patternx);
    OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | x);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
    xf86AccelInfoRec.Subsequent8x8PatternColorExpand = 
		MGANAME(Subsequent8x8PatternColorExpand_Additional);
}

void 
MGANAME(Subsequent8x8PatternColorExpand_Additional)(patternx, patterny, x, y, w, h)
    unsigned patternx, patterny;
    int x, y, w, h;
{
    WAITFIFO(2);
    OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | x);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}



   		/****************************\
		|    8x8 Trapezoidal Fill    |
		\****************************/


void 
MGANAME(Subsequent8x8TrapezoidColorExpand)(patternx, patterny, y, h, 
	left, dxL, dyL, eL, right, dxR, dyR, eR)
    int y, h, left, dxL, dyL, eL, right, dxR, dyR, eR;
{
    int sdxl = (dxL < 0);
    int ar2 = sdxl? dxL : -dxL;
    int sdxr = (dxR < 0);
    int ar5 = sdxr? dxR : -dxR;
    
    WAITFIFO(12);
    OUTREG(MGAREG_SHIFT, (patterny << 4) | patternx);
    OUTREG(MGAREG_DWGCTL, PatternRectCMD & ~(MGADWG_ARZERO | MGADWG_SGNZERO));
    OUTREG(MGAREG_AR0, dyL);
    OUTREG(MGAREG_AR1, ar2 - eL);
    OUTREG(MGAREG_AR2, ar2);
    OUTREG(MGAREG_AR4, ar5 - eR);
    OUTREG(MGAREG_AR5, ar5);
    OUTREG(MGAREG_AR6, dyR);
    OUTREG(MGAREG_SGN, (sdxl << 1) | (sdxr << 5));
    OUTREG(MGAREG_FXBNDRY, ((right + 1) << 16) | left);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
    OUTREG(MGAREG_DWGCTL, PatternRectCMD);
}


		/*******************\
		|   Image Writes    |
		\*******************/


void 
MGANAME(SetupForImageWrite)(rop, planemask, transparency_color)
   int rop;
   unsigned int planemask;
   int transparency_color;
{
    MGAIsClipped = TRUE;
    REPLICATE(planemask);
    WAITFIFO(3);
    OUTREG(MGAREG_AR5, 0);
#if PSZ != 24
    OUTREG(MGAREG_PLNWT, planemask);
#endif
    OUTREG(MGAREG_DWGCTL, MGADWG_ILOAD | MGADWG_BFCOL | MGADWG_SHIFTZERO |
			MGADWG_SGNZERO | MGAAtypeNoBLK[rop]);
}

void 
MGANAME(SubsequentImageWrite)(x, y, w, h, skipleft) 
   int x, y, w, h, skipleft;
{
    WAITFIFO(5);
    OUTREG(MGAREG_CXBNDRY, 0xFFFF0000 | ((x + skipleft) & 0xFFFF));
    OUTREG(MGAREG_AR0, w - 1);
    OUTREG(MGAREG_AR3, 0);
    OUTREG(MGAREG_FXBNDRY, ((x + w - 1) << 16) | (x & 0xFFFF));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}

		/***********************\
		|      Solid Spans      |
		\***********************/


void MGANAME(FillSpansSolid)(n, ppt, pwidth, fSorted, fg, rop, planemask)
   int n;
   DDXPointPtr ppt;
   int *pwidth;
   int fSorted;
   int fg;
   unsigned int planemask;
{
    CARD32 mgaCMD;

#if PSZ == 24
    if(!RGBEQUAL(fg))
    mgaCMD = MGADWG_TRAP | MGADWG_SOLID | MGADWG_ARZERO | 
		    MGADWG_SGNZERO | MGADWG_SHIFTZERO | 
		    MGADWG_BMONOLEF | MGAAtypeNoBLK[rop];
    else
#endif
    mgaCMD = MGADWG_TRAP | MGADWG_SOLID | MGADWG_ARZERO | 
		    MGADWG_SGNZERO | MGADWG_SHIFTZERO | 
		    MGADWG_BMONOLEF | MGAAtype[rop];

    if(MGAIsMillennium2) mgaCMD |= MGADWG_TRANSC;

    REPLICATE24(fg);
    REPLICATE(planemask);
    WAITFIFO(3);
    OUTREG(MGAREG_FCOL, fg);
#if PSZ != 24
    OUTREG(MGAREG_PLNWT, planemask);
#endif
    OUTREG(MGAREG_DWGCTL, mgaCMD);

    while(n--) {
    	WAITFIFO(2);
    	OUTREG(MGAREG_FXBNDRY, ((ppt->x + *pwidth) << 16) | ppt->x);
    	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (ppt->y << 16) | 1);
	ppt++; pwidth++;
    }
    SET_SYNC_FLAG;    
}


void MGANAME(FillSpansSolid_DMA)(n, ppt, pwidth, fSorted, fg, rop, planemask)
   int n;
   DDXPointPtr ppt;
   int *pwidth;
   int fSorted;
   int fg;
   unsigned int planemask;
{
    CARD32 mgaCMD;
    register CARD32* Base = 
		(CARD32*)xf86AccelInfoRec.CPUToScreenColorExpandBase;
    CARD32* MaxBase = Base + 352;

#if PSZ == 24
    if(!RGBEQUAL(fg))
    mgaCMD = MGADWG_TRAP | MGADWG_SOLID | MGADWG_ARZERO | 
		    MGADWG_SGNZERO | MGADWG_SHIFTZERO | 
		    MGADWG_BMONOLEF | MGAAtypeNoBLK[rop];
    else
#endif
    mgaCMD = MGADWG_TRAP | MGADWG_SOLID | MGADWG_ARZERO | 
		    MGADWG_SGNZERO | MGADWG_SHIFTZERO | 
		    MGADWG_BMONOLEF | MGAAtype[rop];

    if(MGAIsMillennium2) mgaCMD |= MGADWG_TRANSC;

    REPLICATE24(fg);
    REPLICATE(planemask);
    OUTREG(MGAREG_FCOL, fg);
#if PSZ != 24
    OUTREG(MGAREG_PLNWT, planemask);
#endif
    OUTREG(MGAREG_DWGCTL, mgaCMD);
    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_GENERAL);

    while(n & ~0x01) {
	Base[0] = DMAINDICIES(MGAREG_FXBNDRY, MGAREG_YDSTLEN + MGAREG_EXEC,
		MGAREG_FXBNDRY, MGAREG_YDSTLEN + MGAREG_EXEC);
    	Base[1] = ((ppt->x + *pwidth) << 16) | ppt->x;
    	Base[2] = (ppt->y << 16) | 1;
    	Base[3] = (((ppt + 1)->x + *(pwidth + 1)) << 16) | (ppt + 1)->x;
    	Base[4] = ((ppt + 1)->y << 16) | 1;
	ppt+=2; pwidth+=2; n-=2; Base+=5;
	if(Base > MaxBase) Base = (CARD32*)MGAMMIOBase;
    }
    if(n) {
    	OUTREG(MGAREG_FXBNDRY, ((ppt->x + *pwidth) << 16) | ppt->x);
    	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (ppt->y << 16) | 1);
    }

    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_BLIT);  
    SET_SYNC_FLAG;
}








