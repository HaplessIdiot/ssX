/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_storm.c,v 1.32 1998/10/25 07:12:10 dawes Exp $ */


/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

/* For correct __inline__ usage */
#include "compiler.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that use XAA need this */
#include "xaa.h"
#include "xaalocal.h"
#include "xf86fbman.h"
#include "miline.h"

#include "mga_bios.h"
#include "mga.h"
#include "mga_reg.h"
#include "mga_map.h"
#include "mga_macros.h"

static void MGANAME(SetupForScreenToScreenCopy)(ScrnInfoPtr pScrn, int xdir,
				int ydir, int rop, unsigned int planemask,
				int trans);
static void MGANAME(SubsequentScreenToScreenCopy)(ScrnInfoPtr pScrn,
				int srcX, int srcY, int dstX, int dstY,
				int w, int h);
static void MGANAME(SubsequentScreenToScreenCopy_FastBlit)(ScrnInfoPtr pScrn,
				int srcX, int srcY, int dstX, int dstY,
				int w, int h);
static void MGANAME(SetupForCPUToScreenColorExpandFill)(ScrnInfoPtr pScrn, int fg,
				int bg, int rop, unsigned int planemask);
static void MGANAME(SubsequentCPUToScreenColorExpandFill)(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft);
static void MGANAME(SetupForSolidFill)(ScrnInfoPtr pScrn, int color, int rop,
					unsigned int planemask);
static void MGANAME(SubsequentSolidFillRect)(ScrnInfoPtr pScrn,
					int x, int y, int w, int h);
static void MGANAME(SubsequentSolidFillTrap)(ScrnInfoPtr pScrn, int y, int h, 
				int left, int dxL, int dyL, int eL,
				int right, int dxR, int dyR, int eR);
static void MGANAME(SubsequentSolidHorVertLine) (ScrnInfoPtr pScrn,
                                int x, int y, int len, int dir);
static void MGANAME(SubsequentSolidTwoPointLine)(ScrnInfoPtr pScrn,
				int x1, int y1, int x2, int y2, int flags);
static void MGANAME(SetupForMono8x8PatternFill)(ScrnInfoPtr pScrn,
				int patx, int paty, int fg, int bg,
				int rop, unsigned int planemask);
static void MGANAME(SubsequentMono8x8PatternFillRect)(ScrnInfoPtr pScrn,
				int patx, int paty,
				int x, int y, int w, int h );
static void MGANAME(SubsequentMono8x8PatternFillRect_Additional)(
				ScrnInfoPtr pScrn, int patx, int paty,
				int x, int y, int w, int h );
static void MGANAME(SubsequentMono8x8PatternFillTrap)( ScrnInfoPtr pScrn,
				int patx, int paty, int y, int h, 
				int left, int dxL, int dyL, int eL, 
				int right, int dxR, int dyR, int eR);
static void MGANAME(SetupForImageWrite)(ScrnInfoPtr pScrn, int rop,
   				unsigned int planemask,
				int transparency_color, int bpp, int depth);
static void MGANAME(SubsequentImageWriteRect)(ScrnInfoPtr pScrn,
				int x, int y, int w, int h, int skipleft);
static void MGANAME(SetupForScreenToScreenColorExpandFill)(ScrnInfoPtr pScrn,
				int fg, int bg, int rop, 
				unsigned int planemask);
static void MGANAME(SubsequentScreenToScreenColorExpandFill)(ScrnInfoPtr pScrn,
				int x, int y, int w, int h,
				int srcx, int srcy, int skipleft);
static void MGANAME(SetupForImageRead)(ScrnInfoPtr pScrn, int bpp, int depth);
static void MGANAME(SubsequentImageReadRect)(ScrnInfoPtr pScrn,
				int x, int y, int w, int h);
static void MGANAME(SetupForDashedLine)(ScrnInfoPtr pScrn, int fg, int bg, 
				int rop, unsigned int planemask, int length,
    				unsigned char *pattern);
static void MGANAME(SubsequentDashedTwoPointLine)(ScrnInfoPtr pScrn,
				int x1, int y1, int x2, int y2, 
				int flags, int phase);

extern void MGAWriteBitmapColorExpand(ScrnInfoPtr pScrn, int x, int y,
				int w, int h, unsigned char *src, int srcwidth,
				int skipleft, int fg, int bg, int rop,
				unsigned int planemask);
extern void MGAFillColorExpandRects(ScrnInfoPtr pScrn, int fg, int bg, int rop,
				unsigned int planemask, int nBox, BoxPtr pBox,
				int xorg, int yorg, PixmapPtr pPix);
extern void MGASetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1,
				int x2, int y2);
extern void MGAFillSolidRectsDMA(ScrnInfoPtr pScrn, int fg, int rop, 
				unsigned int planemask, int nBox, BoxPtr pBox);
extern void MGAFillSolidSpansDMA(ScrnInfoPtr pScrn, int fg, int rop, 
				unsigned int planemask, int n, DDXPointPtr ppt,
 				int *pwidth, int fSorted);
extern void MGAFillMono8x8PatternRectsTwoPass(ScrnInfoPtr pScrn, int fg, int bg,
 				int rop, unsigned int planemask, int nBox,
 				BoxPtr pBox, int pattern0, int pattern1, 
				int xorigin, int yorigin);
extern void MGANonTEGlyphRenderer(ScrnInfoPtr pScrn, int x, int y, int n,
				NonTEGlyphPtr glyphs, BoxPtr pbox,
				int fg, int rop, unsigned int planemask);
extern void MGAValidatePolyArc(GCPtr, unsigned long, DrawablePtr);

Bool
MGANAME(AccelInit)(ScreenPtr pScreen) 
{
    XAAInfoRecPtr infoPtr;
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];
    MGAPtr pMga = MGAPTR(pScrn);
    int maxFastBlitMem;
    BoxRec AvailFBArea;

    pMga->AccelInfoRec = infoPtr = XAACreateInfoRec();
    if(!infoPtr) return FALSE;

    pMga->MaxFastBlitY = 0;

    switch (pMga->Chipset) {
    case PCI_CHIP_MGA2064:
    	pMga->AccelFlags = BLK_OPAQUE_EXPANSION | FASTBLT_BUG;
	break;
    case PCI_CHIP_MGA2164:
    case PCI_CHIP_MGA2164_AGP:
    	pMga->AccelFlags = BLK_OPAQUE_EXPANSION |
			   TRANSC_SOLID_FILL |
 			   USE_RECTS_FOR_LINES; 
        break;
    case PCI_CHIP_MGAG200:
    case PCI_CHIP_MGAG200_PCI:
        pMga->AccelFlags = TRANSC_SOLID_FILL |
			   TWO_PASS_COLOR_EXPAND;
        break;
    case PCI_CHIP_MGA1064:
	pMga->AccelFlags = 0;
        break;
    case PCI_CHIP_MGAG100:
    default:
	pMga->AccelFlags = MGA_NO_PLANEMASK;
        break;
    }

    if(pMga->HasSDRAM) {
	pMga->Atype = pMga->AtypeNoBLK = MGAAtypeNoBLK;
	pMga->AccelFlags &= ~TWO_PASS_COLOR_EXPAND;
    } else {
	pMga->Atype = MGAAtype;
	pMga->AtypeNoBLK = MGAAtypeNoBLK;
    }

    /* fill out infoPtr here */
    infoPtr->Flags = 	PIXMAP_CACHE | 
			MICROSOFT_ZERO_LINE_BIAS;

    if(pMga->Overlay8Plus24)
	infoPtr->FullPlanemask = ~0;

    /* sync */
    infoPtr->Sync = MGAStormSync;

    /* screen to screen copy */
    infoPtr->ScreenToScreenCopyFlags = NO_TRANSPARENCY;
    infoPtr->SetupForScreenToScreenCopy =
        	MGANAME(SetupForScreenToScreenCopy);
    infoPtr->SubsequentScreenToScreenCopy =
        	MGANAME(SubsequentScreenToScreenCopy);

    /* solid fills */
    infoPtr->SetupForSolidFill = MGANAME(SetupForSolidFill);
    infoPtr->SubsequentSolidFillRect = MGANAME(SubsequentSolidFillRect);
    infoPtr->SubsequentSolidFillTrap = MGANAME(SubsequentSolidFillTrap);

    /* solid lines */
    infoPtr->SolidLineFlags = HARDWARE_CLIP_LINE;
    infoPtr->SetupForSolidLine = infoPtr->SetupForSolidFill;
    infoPtr->SubsequentSolidHorVertLine =
		MGANAME(SubsequentSolidHorVertLine);
    infoPtr->SubsequentSolidTwoPointLine = 
		MGANAME(SubsequentSolidTwoPointLine);
    infoPtr->SetClippingRectangle = MGASetClippingRectangle;

    /* dashed lines */
    infoPtr->DashedLineFlags = HARDWARE_CLIP_LINE |
				LINE_PATTERN_MSBFIRST_LSBJUSTIFIED;
    infoPtr->SetupForDashedLine = MGANAME(SetupForDashedLine);
    infoPtr->SubsequentDashedTwoPointLine =  
		MGANAME(SubsequentDashedTwoPointLine);
    infoPtr->DashPatternMaxLength = 128;


    /* 8x8 mono patterns */
    infoPtr->Mono8x8PatternFillFlags = HARDWARE_PATTERN_PROGRAMMED_BITS |
					HARDWARE_PATTERN_PROGRAMMED_ORIGIN |
					HARDWARE_PATTERN_SCREEN_ORIGIN |
					BIT_ORDER_IN_BYTE_MSBFIRST;
    infoPtr->SetupForMono8x8PatternFill = MGANAME(SetupForMono8x8PatternFill);
    infoPtr->SubsequentMono8x8PatternFillRect = 
		MGANAME(SubsequentMono8x8PatternFillRect);
    infoPtr->SubsequentMono8x8PatternFillTrap = 
		MGANAME(SubsequentMono8x8PatternFillTrap);

    /* cpu to screen color expansion */
    infoPtr->CPUToScreenColorExpandFillFlags = 	CPU_TRANSFER_PAD_DWORD |
					SCANLINE_PAD_DWORD |
					BIT_ORDER_IN_BYTE_LSBFIRST |
					LEFT_EDGE_CLIPPING |
					LEFT_EDGE_CLIPPING_NEGATIVE_X |
					SYNC_AFTER_COLOR_EXPAND;
    if(pMga->ILOADBase) {
	infoPtr->ColorExpandRange = 0x800000;
	infoPtr->ColorExpandBase = pMga->ILOADBase;
    } else {
	infoPtr->ColorExpandRange = 0x1C00;
	infoPtr->ColorExpandBase = pMga->IOBase;
    }
    infoPtr->SetupForCPUToScreenColorExpandFill =
		MGANAME(SetupForCPUToScreenColorExpandFill);
    infoPtr->SubsequentCPUToScreenColorExpandFill =
		MGANAME(SubsequentCPUToScreenColorExpandFill);

    /* screen to screen color expansion */
    infoPtr->ScreenToScreenColorExpandFillFlags = BIT_ORDER_IN_BYTE_LSBFIRST;
    infoPtr->SetupForScreenToScreenColorExpandFill = 
		MGANAME(SetupForScreenToScreenColorExpandFill);
    infoPtr->SubsequentScreenToScreenColorExpandFill = 
		MGANAME(SubsequentScreenToScreenColorExpandFill);


    /* image writes */
    infoPtr->ImageWriteFlags = 	CPU_TRANSFER_PAD_DWORD |
				SCANLINE_PAD_DWORD |
				LEFT_EDGE_CLIPPING |
				LEFT_EDGE_CLIPPING_NEGATIVE_X |
				SYNC_AFTER_IMAGE_WRITE;
    if(pMga->ILOADBase) {
	infoPtr->ImageWriteRange = 0x800000;
	infoPtr->ImageWriteBase = pMga->ILOADBase;
    } else {
	infoPtr->ImageWriteRange = 0x1C00;
	infoPtr->ImageWriteBase = pMga->IOBase;
    }
    infoPtr->SetupForImageWrite = MGANAME(SetupForImageWrite);
    infoPtr->SubsequentImageWriteRect = MGANAME(SubsequentImageWriteRect);

#if 0
    /* image reads */
    infoPtr->ImageReadFlags = 	CPU_TRANSFER_PAD_DWORD |
				SCANLINE_PAD_DWORD;
    if(pMga->ILOADBase) {
	infoPtr->ImageReadRange = 0x800000;
	infoPtr->ImageReadBase = pMga->ILOADBase;
    } else {
	infoPtr->ImageReadRange = 0x1C00;
	infoPtr->ImageReadBase = pMga->IOBase;
    }
    infoPtr->SetupForImageRead = MGANAME(SetupForImageRead);
    infoPtr->SubsequentImageReadRect = MGANAME(SubsequentImageReadRect);
#endif

    /* midrange replacements */

    if(infoPtr->SetupForCPUToScreenColorExpandFill && 
			infoPtr->SubsequentCPUToScreenColorExpandFill) {
	infoPtr->FillColorExpandRects = MGAFillColorExpandRects; 
	infoPtr->WriteBitmap = MGAWriteBitmapColorExpand;
	infoPtr->NonTEGlyphRenderer = MGANonTEGlyphRenderer;  
    }

    if(pMga->ILOADBase && pMga->UsePCIRetry && infoPtr->SetupForSolidFill) {
	infoPtr->FillSolidRects = MGAFillSolidRectsDMA;
	infoPtr->FillSolidSpans = MGAFillSolidSpansDMA;
    }

    if(pMga->AccelFlags & TWO_PASS_COLOR_EXPAND) {
	if(infoPtr->SetupForMono8x8PatternFill)
	    infoPtr->FillMono8x8PatternRects = 
				MGAFillMono8x8PatternRectsTwoPass;
    }

    if(pMga->UsePCIRetry) {
	infoPtr->ValidatePolyArc = MGAValidatePolyArc;
	infoPtr->PolyArcMask = GCFunction | GCLineWidth | GCPlaneMask | 
				GCLineStyle | GCFillStyle;
    }

    if((pScrn->bitsPerPixel == 24) || (pMga->AccelFlags & MGA_NO_PLANEMASK)) {
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
    }

    
    maxFastBlitMem = (pMga->Interleave ? 4096 : 2048) * 1024;

    if(pMga->FbMapSize > maxFastBlitMem) {
	pMga->MaxFastBlitY = maxFastBlitMem / (pScrn->displayWidth * PSZ / 8);
    }


    AvailFBArea.x1 = 0;
    AvailFBArea.y1 = 0;
    AvailFBArea.x2 = pScrn->displayWidth;
    AvailFBArea.y2 = pMga->FbUsableSize / (pScrn->displayWidth * PSZ / 8);

    xf86InitFBManager(pScreen, &AvailFBArea); 

    return(XAAInit(pScreen, infoPtr));
}

#if PSZ == 8

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


Bool 
MGAStormAccelInit(ScreenPtr pScreen)
{ 
    ScrnInfoPtr pScrn = xf86Screens[pScreen->myNum];

    switch( pScrn->bitsPerPixel )
    {
    case 8:
    	return Mga8AccelInit(pScreen);
    case 16:
    	return Mga16AccelInit(pScreen);
    case 24:
    	return Mga24AccelInit(pScreen);
    case 32:
    	return Mga32AccelInit(pScreen);
    }
    return FALSE;
}



void
MGAStormSync(ScrnInfoPtr pScrn) 
{
    MGAPtr pMga = MGAPTR(pScrn);
    
    if(pMga->AccelFlags & CLIPPER_ON) {
	DISABLE_CLIP();
    } else while(MGAISBUSY());

    /* flush cache before a read (mga-1064g 5.1.6) */
    OUTREG8(MGAREG_CRTC_INDEX, 0); 
}

void MGAStormEngineInit(ScrnInfoPtr pScrn)
{
    long maccess = 0;
    MGAPtr pMga = MGAPTR(pScrn);
    if (pMga->Chipset == PCI_CHIP_MGAG100)
    	maccess = 1 << 14;
    
    switch( pScrn->bitsPerPixel )
    {
    case 8:
        break;
    case 16:
	/* set 16 bpp, turn off dithering, turn on 5:5:5 pixels */
        maccess |= 1 + (1 << 30) + (1 << 31);
        break;
    case 24:
        maccess |= 3;
        break;
    case 32:
        maccess |= 2;
        break;
    }
    
    WAITFIFO(10);
    OUTREG(MGAREG_PITCH, pScrn->displayWidth);
    OUTREG(MGAREG_YDSTORG, pMga->YDstOrg);
    OUTREG(MGAREG_MACCESS, maccess);
    pMga->PlaneMask = ~0;
    if(!(pMga->AccelFlags & MGA_NO_PLANEMASK))
	OUTREG(MGAREG_PLNWT, pMga->PlaneMask);
    pMga->FgColor = 1;
    OUTREG(MGAREG_FCOL, pMga->FgColor);
    pMga->BgColor = 0;
    OUTREG(MGAREG_BCOL, pMga->BgColor);
    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_BLIT);

    /* put clipping in a know state */
    OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);	/* (maxX << 16) | minX */ 
    OUTREG(MGAREG_YTOP, 0x00000000);	/* minPixelPointer */ 
    OUTREG(MGAREG_YBOT, 0x007FFFFF);	/* maxPixelPointer */ 
    pMga->AccelFlags &= ~CLIPPER_ON;
}

void MGASetClippingRectangle(
   ScrnInfoPtr pScrn,
   int x1, int y1, int x2, int y2
){
    MGAPtr pMga = MGAPTR(pScrn);

    WAITFIFO(3);
    OUTREG(MGAREG_CXBNDRY,(x2 << 16) | x1);      
    OUTREG(MGAREG_YTOP, (y1 * pScrn->displayWidth) + pMga->YDstOrg); 
    OUTREG(MGAREG_YBOT, (y2 * pScrn->displayWidth) + pMga->YDstOrg); 
    pMga->AccelFlags |= CLIPPER_ON;
}


#endif


	/*********************************************\
	|            Screen-to-Screen Copy            |
	\*********************************************/

#define BLIT_LEFT	1
#define BLIT_UP		4

static void 
MGANAME(SetupForScreenToScreenCopy)(
    ScrnInfoPtr pScrn,
    int xdir, int ydir,
    int rop,
    unsigned int planemask,
    int trans )
{
    MGAPtr pMga = MGAPTR(pScrn);

    pMga->AccelInfoRec->SubsequentScreenToScreenCopy = 
		MGANAME(SubsequentScreenToScreenCopy);
    pMga->BltScanDirection = 0;
    if(ydir == -1) pMga->BltScanDirection |= BLIT_UP;
    WAITFIFO(4); 
    if(xdir == -1) {
	pMga->BltScanDirection |= BLIT_LEFT;
    	OUTREG(MGAREG_DWGCTL, pMga->AtypeNoBLK[rop] | MGADWG_SHIFTZERO | 
			MGADWG_BITBLT | MGADWG_BFCOL);
    } else {
	if(pMga->HasFBitBlt && (rop == GXcopy)) 
	   pMga->AccelInfoRec->SubsequentScreenToScreenCopy = 
		MGANAME(SubsequentScreenToScreenCopy_FastBlit);
    	OUTREG(MGAREG_DWGCTL, pMga->AtypeNoBLK[rop] | MGADWG_SHIFTZERO | 
			MGADWG_BITBLT | MGADWG_BFCOL);
    }
    OUTREG(MGAREG_SGN, pMga->BltScanDirection);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_AR5, ydir * pScrn->displayWidth);
}


static void 
MGANAME(SubsequentScreenToScreenCopy)(
    ScrnInfoPtr pScrn,
    int srcX, int srcY, int dstX, int dstY, int w, int h
)
{
    int start, end;
    MGAPtr pMga = MGAPTR(pScrn);

    if(pMga->BltScanDirection & BLIT_UP) {
	srcY += h - 1;
	dstY += h - 1;
    }

    w--;
    start = end = XYADDRESS(srcX, srcY);

    if(pMga->BltScanDirection & BLIT_LEFT) start += w;
    else end += w; 
   
    WAITFIFO(4); 
    OUTREG(MGAREG_AR0, end);
    OUTREG(MGAREG_AR3, start);
    OUTREG(MGAREG_FXBNDRY, ((dstX + w) << 16) | dstX);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (dstY << 16) | h);
}


static void 
MGANAME(SubsequentScreenToScreenCopy_FastBlit)(
    ScrnInfoPtr pScrn,
    int srcX, int srcY, int dstX, int dstY, int w, int h
)
{
    int start, end;
    MGAPtr pMga = MGAPTR(pScrn);

    if(pMga->BltScanDirection & BLIT_UP) {
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
	if(pMga->MaxFastBlitY) {
	   if(pMga->BltScanDirection & BLIT_UP) {
		if((srcY >= pMga->MaxFastBlitY) ||
				(dstY >= pMga->MaxFastBlitY)) 
			goto FASTBLIT_BAILOUT;
	   } else {
		if(((srcY + h) > pMga->MaxFastBlitY) ||
				((dstY + h) > pMga->MaxFastBlitY)) 
			goto FASTBLIT_BAILOUT;
	   }
	}

	/* Millennium 1 fastblit bug fix */
        if(pMga->AccelFlags & FASTBLT_BUG) {
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
		OUTREG(MGAREG_DWGCTL, pMga->AtypeNoBLK[GXcopy] | 
			MGADWG_SHIFTZERO | MGADWG_BITBLT | MGADWG_BFCOL);
		OUTREG(MGAREG_CXRIGHT, 0xFFFF);
	    	return;
	    } /* } } } (preserve pairs for pair matching) */
	}
	
   	WAITFIFO(6); 
    	OUTREG(MGAREG_DWGCTL, 0x040A400C);
    	OUTREG(MGAREG_AR0, end);
    	OUTREG(MGAREG_AR3, start);
    	OUTREG(MGAREG_FXBNDRY, ((dstX + w) << 16) | dstX);
    	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (dstY << 16) | h);
    	OUTREG(MGAREG_DWGCTL, pMga->AtypeNoBLK[GXcopy] | MGADWG_SHIFTZERO | 
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

        
        /******************\
	|   Solid Fills    |
	\******************/

static void 
MGANAME(SetupForSolidFill)(
	ScrnInfoPtr pScrn,
	int color,
	int rop,
	unsigned int planemask )
{
    MGAPtr pMga = MGAPTR(pScrn);

#if PSZ == 24
    if(!RGBEQUAL(color))
    pMga->FilledRectCMD = MGADWG_TRAP | MGADWG_SOLID | MGADWG_ARZERO | 
		    MGADWG_SGNZERO | MGADWG_SHIFTZERO | 
		    MGADWG_BMONOLEF | pMga->AtypeNoBLK[rop];
    else
#endif
    pMga->FilledRectCMD = MGADWG_TRAP | MGADWG_SOLID | MGADWG_ARZERO | 
		    MGADWG_SGNZERO | MGADWG_SHIFTZERO | 
		    MGADWG_BMONOLEF | pMga->Atype[rop];

    pMga->SolidLineCMD =  MGADWG_SOLID | MGADWG_SHIFTZERO | MGADWG_BFCOL | 
		    pMga->AtypeNoBLK[rop];

    if(pMga->AccelFlags & TRANSC_SOLID_FILL) 
	pMga->FilledRectCMD |= MGADWG_TRANSC;

    WAITFIFO(3);
    SET_FOREGROUND(color);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_DWGCTL, pMga->FilledRectCMD);
}    

static void 
MGANAME(SubsequentSolidFillRect)(
	ScrnInfoPtr pScrn,
	int x, int y, int w, int h )
{
    MGAPtr pMga = MGAPTR(pScrn);

    WAITFIFO(2);
    OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | x);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}   

static void 
MGANAME(SubsequentSolidFillTrap)(ScrnInfoPtr pScrn, int y, int h, 
	int left, int dxL, int dyL, int eL,
	int right, int dxR, int dyR, int eR )
{
    MGAPtr pMga = MGAPTR(pScrn);
    int sdxl = (dxL < 0);
    int ar2 = sdxl? dxL : -dxL;
    int sdxr = (dxR < 0);
    int ar5 = sdxr? dxR : -dxR;
    
    WAITFIFO(11);
    OUTREG(MGAREG_DWGCTL, 
		pMga->FilledRectCMD & ~(MGADWG_ARZERO | MGADWG_SGNZERO));
    OUTREG(MGAREG_AR0, dyL);
    OUTREG(MGAREG_AR1, ar2 - eL);
    OUTREG(MGAREG_AR2, ar2);
    OUTREG(MGAREG_AR4, ar5 - eR);
    OUTREG(MGAREG_AR5, ar5);
    OUTREG(MGAREG_AR6, dyR);
    OUTREG(MGAREG_SGN, (sdxl << 1) | (sdxr << 5));
    OUTREG(MGAREG_FXBNDRY, ((right + 1) << 16) | left);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
    OUTREG(MGAREG_DWGCTL, pMga->FilledRectCMD);
}

	/***************\
	|  Solid Lines  |
	\***************/


static void 
MGANAME(SubsequentSolidHorVertLine) (
    ScrnInfoPtr pScrn,
    int x, int y, 
    int len, int dir
){
    MGAPtr pMga = MGAPTR(pScrn);

    if(dir == DEGREES_0) {
	WAITFIFO(2);
	OUTREG(MGAREG_FXBNDRY, ((x + len) << 16) | x);
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | 1);
    } else if(pMga->AccelFlags & USE_RECTS_FOR_LINES) {
	WAITFIFO(2);
	OUTREG(MGAREG_FXBNDRY, ((x + 1) << 16) | x);
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | len);
    } else {
	WAITFIFO(4);
	OUTREG(MGAREG_DWGCTL, pMga->SolidLineCMD | MGADWG_AUTOLINE_OPEN);
	OUTREG(MGAREG_XYSTRT, (y << 16) | x);
	OUTREG(MGAREG_XYEND + MGAREG_EXEC, ((y + len) << 16) | x);
	OUTREG(MGAREG_DWGCTL, pMga->FilledRectCMD); 
    }
}


static void 
MGANAME(SubsequentSolidTwoPointLine)(
   ScrnInfoPtr pScrn,
   int x1, int y1, int x2, int y2, int flags
){
    MGAPtr pMga = MGAPTR(pScrn);

    WAITFIFO(4);
    OUTREG(MGAREG_DWGCTL, pMga->SolidLineCMD | 
        ((flags & OMIT_LAST) ? MGADWG_AUTOLINE_OPEN : MGADWG_AUTOLINE_CLOSE));
    OUTREG(MGAREG_XYSTRT, (y1 << 16) | (x1 & 0xFFFF));
    OUTREG(MGAREG_XYEND + MGAREG_EXEC, (y2 << 16) | (x2 & 0xFFFF));
    OUTREG(MGAREG_DWGCTL, pMga->FilledRectCMD); 
   
    if(pMga->AccelFlags & CLIPPER_ON) {
        WAITFIFO(3);
        OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);     /* (maxX << 16) | minX */ 
        OUTREG(MGAREG_YTOP, 0x00000000);        /* minPixelPointer */ 
        OUTREG(MGAREG_YBOT, 0x007FFFFF);        /* maxPixelPointer */ 
        pMga->AccelFlags &= ~CLIPPER_ON;
    }
}



	/***************************\
	|   8x8 Mono Pattern Fills  |
	\***************************/


static void 
MGANAME(SetupForMono8x8PatternFill)(
   ScrnInfoPtr pScrn,
   int patx, int paty,
   int fg, int bg,
   int rop,
   unsigned int planemask )
{
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;

    pMga->PatternRectCMD = MGADWG_TRAP | MGADWG_ARZERO | MGADWG_SGNZERO | 
						MGADWG_BMONOLEF;

    infoRec->SubsequentMono8x8PatternFillRect = 
		MGANAME(SubsequentMono8x8PatternFillRect);
    
    if(bg == -1) {
#if PSZ == 24
    	if(!RGBEQUAL(fg))
            pMga->PatternRectCMD |= MGADWG_TRANSC | pMga->AtypeNoBLK[rop];
	else
#endif
            pMga->PatternRectCMD |= MGADWG_TRANSC | pMga->Atype[rop];

	WAITFIFO(5);
    } else {
#if PSZ == 24
	if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION) && RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
	if(pMga->AccelFlags & BLK_OPAQUE_EXPANSION) 
#endif
        	pMga->PatternRectCMD |= pMga->Atype[rop];
	else
        	pMga->PatternRectCMD |= pMga->AtypeNoBLK[rop];
	WAITFIFO(6);
    	SET_BACKGROUND(bg);
    }

    SET_FOREGROUND(fg);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_DWGCTL, pMga->PatternRectCMD);
    OUTREG(MGAREG_PAT0, patx);
    OUTREG(MGAREG_PAT1, paty);
}



static void 
MGANAME(SubsequentMono8x8PatternFillRect)(
    ScrnInfoPtr pScrn,
    int patx, int paty,
    int x, int y, int w, int h )
{
    MGAPtr pMga = MGAPTR(pScrn);
    
    WAITFIFO(3);
    OUTREG(MGAREG_SHIFT, (paty << 4) | patx);
    OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | x);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
    pMga->AccelInfoRec->SubsequentMono8x8PatternFillRect = 
		MGANAME(SubsequentMono8x8PatternFillRect_Additional);
}

static void 
MGANAME(SubsequentMono8x8PatternFillRect_Additional)(
    ScrnInfoPtr pScrn,
    int patx, int paty,
    int x, int y, int w, int h )
{
    MGAPtr pMga = MGAPTR(pScrn);
    WAITFIFO(2);
    OUTREG(MGAREG_FXBNDRY, ((x + w) << 16) | x);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}


static void 
MGANAME(SubsequentMono8x8PatternFillTrap)(
    ScrnInfoPtr pScrn,
    int patx, int paty,	
    int y, int h, 
    int left, int dxL, int dyL, int eL, 
    int right, int dxR, int dyR, int eR
){
    MGAPtr pMga = MGAPTR(pScrn);

    int sdxl = (dxL < 0) ? (1<<1) : 0;
    int ar2 = sdxl? dxL : -dxL;
    int sdxr = (dxR < 0) ? (1<<5) : 0;
    int ar5 = sdxr? dxR : -dxR;

    WAITFIFO(12);
    OUTREG(MGAREG_SHIFT, (paty << 4) | patx);
    OUTREG(MGAREG_DWGCTL, 
	pMga->PatternRectCMD & ~(MGADWG_ARZERO | MGADWG_SGNZERO));
    OUTREG(MGAREG_AR0, dyL);
    OUTREG(MGAREG_AR1, ar2 - eL);
    OUTREG(MGAREG_AR2, ar2);
    OUTREG(MGAREG_AR4, ar5 - eR);
    OUTREG(MGAREG_AR5, ar5);
    OUTREG(MGAREG_AR6, dyR);
    OUTREG(MGAREG_SGN, sdxl | sdxr);
    OUTREG(MGAREG_FXBNDRY, ((right + 1) << 16) | left);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
    OUTREG(MGAREG_DWGCTL, pMga->PatternRectCMD);
}

	/***********************\
	|   Color Expand Rect   |
	\***********************/


static void 
MGANAME(SetupForCPUToScreenColorExpandFill)(
	ScrnInfoPtr pScrn,
	int fg, int bg,
	int rop,
	unsigned int planemask )
{
    MGAPtr pMga = MGAPTR(pScrn);

    CARD32 mgaCMD = MGADWG_ILOAD | MGADWG_LINEAR | MGADWG_SGNZERO | 
			MGADWG_SHIFTZERO | MGADWG_BMONOLEF;
        
    if(bg == -1) {
#if PSZ == 24
    	if(!RGBEQUAL(fg))
            mgaCMD |= MGADWG_TRANSC | pMga->AtypeNoBLK[rop];
	else
#endif
            mgaCMD |= MGADWG_TRANSC | pMga->Atype[rop];

	WAITFIFO(3);
    } else {
#if PSZ == 24
	if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION) && 
		RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
	if(pMga->AccelFlags & BLK_OPAQUE_EXPANSION) 
#endif
        	mgaCMD |= pMga->Atype[rop];
	else
        	mgaCMD |= pMga->AtypeNoBLK[rop];
	WAITFIFO(4);
    	SET_BACKGROUND(bg);
    }

    SET_FOREGROUND(fg);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_DWGCTL, mgaCMD);
}       

static void 
MGANAME(SubsequentCPUToScreenColorExpandFill)(
	ScrnInfoPtr pScrn,
	int x, int y, int w, int h,
	int skipleft
){
    MGAPtr pMga = MGAPTR(pScrn);

    pMga->AccelFlags |= CLIPPER_ON;
    WAITFIFO(5);
    OUTREG(MGAREG_CXBNDRY, ((x + w - 1) << 16) | ((x + skipleft) & 0xFFFF));
    w = (w + 31) & ~31;     /* source is dword padded */
    OUTREG(MGAREG_AR0, (w * h) - 1);
    OUTREG(MGAREG_AR3, 0);  /* crashes occasionally without this */
    OUTREG(MGAREG_FXBNDRY, ((x + w - 1) << 16) | (x & 0xFFFF));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}

	/*******************\
	|   Image Writes    |
	\*******************/


static void MGANAME(SetupForImageWrite)(	
   ScrnInfoPtr pScrn,
   int rop,
   unsigned int planemask,
   int transparency_color,
   int bpp, int depth
){
    MGAPtr pMga = MGAPTR(pScrn);

    WAITFIFO(3);
    OUTREG(MGAREG_AR5, 0);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_DWGCTL, MGADWG_ILOAD | MGADWG_BFCOL | MGADWG_SHIFTZERO |
			MGADWG_SGNZERO | pMga->AtypeNoBLK[rop]);
}


static void MGANAME(SubsequentImageWriteRect)(
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   int skipleft
){
    MGAPtr pMga = MGAPTR(pScrn);

    pMga->AccelFlags |= CLIPPER_ON;

    WAITFIFO(5);
    OUTREG(MGAREG_CXBNDRY, 0xFFFF0000 | ((x + skipleft) & 0xFFFF));
    OUTREG(MGAREG_AR0, w - 1);
    OUTREG(MGAREG_AR3, 0);
    OUTREG(MGAREG_FXBNDRY, ((x + w - 1) << 16) | (x & 0xFFFF));
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}

	/*****************\
	|   Image Reads   |
	\*****************/

static void MGANAME(SetupForImageRead)(
    ScrnInfoPtr pScrn, 
    int bpp, int depth
){
    MGAPtr pMga = MGAPTR(pScrn);
    WAITFIFO(2);
    OUTREG(MGAREG_AR5, pScrn->displayWidth);
    OUTREG(MGAREG_DWGCTL, MGADWG_IDUMP | MGADWG_BU32RGB | MGADWG_SHIFTZERO |
			MGADWG_SGNZERO | 0x000c0000);
}

static void MGANAME(SubsequentImageReadRect)(
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h
){
    MGAPtr pMga = MGAPTR(pScrn);
    int start = XYADDRESS(x, y);

    WAITFIFO(6);
    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_GENERAL);
    OUTREG(MGAREG_AR0, start + w - 1);
    OUTREG(MGAREG_AR3, start);
    OUTREG(MGAREG_FXBNDRY, (w - 1) << 16);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_BLIT);
} 

	/***************************\
	|      Dashed  Lines        |
	\***************************/


void
MGANAME(SetupForDashedLine)(
    ScrnInfoPtr pScrn, 
    int fg, int bg, int rop,
    unsigned int planemask,
    int length,
    unsigned char *pattern
){
    MGAPtr pMga = MGAPTR(pScrn);
    CARD32 *DashPattern = (CARD32*)pattern;
    CARD32 NiceDashPattern = DashPattern[0];
    int dwords = (length + 31) >> 5;

    pMga->DashCMD = MGADWG_BFCOL | pMga->AtypeNoBLK[rop];
    pMga->StyleLen = length - 1;

    if(bg == -1) {
        pMga->DashCMD |= MGADWG_TRANSC;
	WAITFIFO(dwords + 2);
    } else {
	WAITFIFO(dwords + 3);
	SET_BACKGROUND(bg);
    }
    SET_PLANEMASK(planemask);
    SET_FOREGROUND(fg);

    /* We see if we can draw horizontal lines as 8x8 pattern fills.
	This is worthwhile since the pattern fills can use block mode
	and the default X pattern is 8 pixels long.  The forward pattern
	is the top scanline, the backwards pattern is the next one. */
    switch(length) {
	case 2:	NiceDashPattern |= NiceDashPattern << 2;
	case 4:	NiceDashPattern |= NiceDashPattern << 4;
	case 8:	NiceDashPattern |= byte_reversed[NiceDashPattern] << 16;
		NiceDashPattern |= NiceDashPattern << 8;
		pMga->NiceDashCMD = MGADWG_TRAP | MGADWG_ARZERO | 
				    MGADWG_SGNZERO | MGADWG_BMONOLEF;
     		pMga->AccelFlags |= NICE_DASH_PATTERN;
   		if(bg == -1) {
#if PSZ == 24
    		   if(!RGBEQUAL(fg))
            		pMga->NiceDashCMD |= MGADWG_TRANSC | pMga->AtypeNoBLK[rop];
		   else
#endif
           		pMga->NiceDashCMD |= MGADWG_TRANSC | pMga->Atype[rop];
    		} else {
#if PSZ == 24
		   if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION) && 
					RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
		   if(pMga->AccelFlags & BLK_OPAQUE_EXPANSION) 
#endif
        		pMga->NiceDashCMD |= pMga->Atype[rop];
		   else
        		pMga->NiceDashCMD |= pMga->AtypeNoBLK[rop];
    		}
		OUTREG(MGAREG_SRC0, NiceDashPattern);
		break;
	default: pMga->AccelFlags &= ~NICE_DASH_PATTERN;
		switch (dwords) {
		case 4:  OUTREG(MGAREG_SRC3, DashPattern[3]);
		case 3:  OUTREG(MGAREG_SRC2, DashPattern[2]);
		case 2:	 OUTREG(MGAREG_SRC1, DashPattern[1]);
		default: OUTREG(MGAREG_SRC0, DashPattern[0]);
		}
    }
}


void
MGANAME(SubsequentDashedTwoPointLine)(
    ScrnInfoPtr pScrn, 
    int x1, int y1, int x2, int y2, 
    int flags, int phase
){
    MGAPtr pMga = MGAPTR(pScrn);

    if((pMga->AccelFlags & NICE_DASH_PATTERN) && (y1 == y2)) {
    	WAITFIFO(4);
    	OUTREG(MGAREG_DWGCTL, pMga->NiceDashCMD);
	if(x2 < x1) {
	   if(flags & OMIT_LAST) x2++;
   	   OUTREG(MGAREG_SHIFT, ((-y1 & 0x07) << 4) | 
				((7 - phase - x1) & 0x07)); 
   	   OUTREG(MGAREG_FXBNDRY, ((x1 + 1) << 16) | x2);
    	} else {
 	   if(!flags) x2++;
   	   OUTREG(MGAREG_SHIFT, (((1 - y1) & 0x07) << 4) | 
				((phase - x1) & 0x07)); 
     	   OUTREG(MGAREG_FXBNDRY, (x2 << 16) | x1);
	}	
    	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y1 << 16) | 1);
	return;
    }

    WAITFIFO(4);
    OUTREG(MGAREG_SHIFT, (pMga->StyleLen << 16 ) | (pMga->StyleLen - phase)); 
    OUTREG(MGAREG_DWGCTL, pMga->DashCMD | 
	((flags & OMIT_LAST) ? MGADWG_AUTOLINE_OPEN : MGADWG_AUTOLINE_CLOSE));
    OUTREG(MGAREG_XYSTRT, (y1 << 16) | (x1 & 0xFFFF));
    OUTREG(MGAREG_XYEND + MGAREG_EXEC, (y2 << 16) | (x2 & 0xFFFF));
   
    if(pMga->AccelFlags & CLIPPER_ON) {
        WAITFIFO(3);
        OUTREG(MGAREG_CXBNDRY, 0xFFFF0000);     /* (maxX << 16) | minX */ 
        OUTREG(MGAREG_YTOP, 0x00000000);        /* minPixelPointer */ 
        OUTREG(MGAREG_YBOT, 0x007FFFFF);        /* maxPixelPointer */ 
        pMga->AccelFlags &= ~CLIPPER_ON;
    }
}





	/***********************************\
	|  Screen to Screen Color Expansion |
	\***********************************/


static void 
MGANAME(SetupForScreenToScreenColorExpandFill)(
   ScrnInfoPtr pScrn,
   int fg, int bg, 
   int rop,
   unsigned int planemask
){
    MGAPtr pMga = MGAPTR(pScrn);
    CARD32 mgaCMD = MGADWG_BITBLT | MGADWG_SGNZERO | MGADWG_SHIFTZERO;
        
    if(bg == -1) {
#if PSZ == 24
    	if(!RGBEQUAL(fg))
            mgaCMD |= MGADWG_TRANSC | pMga->AtypeNoBLK[rop];
	else
#endif
            mgaCMD |= MGADWG_TRANSC | pMga->Atype[rop];

	WAITFIFO(4);
    } else {
#if PSZ == 24
	if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION) && 
		RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
	if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION)) 
#endif
        	mgaCMD |= pMga->Atype[rop];
	else
        	mgaCMD |= pMga->AtypeNoBLK[rop];
	WAITFIFO(5);
    	SET_BACKGROUND(bg);
    }

    SET_FOREGROUND(fg);
    SET_PLANEMASK(planemask);
    OUTREG(MGAREG_AR5, pScrn->displayWidth * PSZ);
    OUTREG(MGAREG_DWGCTL, mgaCMD);

}

static void 
MGANAME(SubsequentScreenToScreenColorExpandFill)(
   ScrnInfoPtr pScrn,
   int x, int y, int w, int h,
   int srcx, int srcy, 
   int skipleft
){
    MGAPtr pMga = MGAPTR(pScrn);
    int start, end;

    start = (XYADDRESS(srcx, srcy) * PSZ) + skipleft;
    end = start + w - 1;

    WAITFIFO(4);
    OUTREG(MGAREG_AR3, start);
    OUTREG(MGAREG_AR0, end);
    OUTREG(MGAREG_FXBNDRY, ((x + w - 1) << 16) | x);
    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y << 16) | h);
}


#if PSZ == 8


static __inline__ CARD32* MoveDWORDS(
   register CARD32* dest,
   register CARD32* src,
   register int dwords
)
{
     while(dwords & ~0x03) {
        dest[0] = src[0];
        dest[1] = src[1];
        dest[2] = src[2];
        dest[3] = src[3];
        src += 4;
        dest += 4;
        dwords -= 4;
     }  

     if(!dwords) return(dest);
     dest[0] = src[0];
     if(dwords == 1) return(dest+1);
     dest[1] = src[1];
     if(dwords == 2) return(dest+2);
     dest[2] = src[2];
     return(dest+3);
}

#define MAX_BLIT_DWORDS 	 (0x40000 >> 5)

void
MGAWriteBitmapColorExpand(
    ScrnInfoPtr pScrn,
    int x, int y, int w, int h,
    unsigned char *src,
    int srcwidth,
    int skipleft,
    int fg, int bg,
    int rop,
    unsigned int planemask 
)
{
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;
    CARD32* destptr = (CARD32*)infoRec->ColorExpandBase;
    CARD32* maxptr;
    int dwords, maxlines, count;

    (*infoRec->SetupForCPUToScreenColorExpandFill)(
				pScrn, fg, bg, rop, planemask);

    w += skipleft;
    x -= skipleft;
    dwords = (w + 31) >> 5;
    maxlines = MAX_BLIT_DWORDS / dwords;
    maxptr = destptr + infoRec->ColorExpandRange - dwords;
     
    while(h > maxlines) {
    	(*infoRec->SubsequentCPUToScreenColorExpandFill)
			(pScrn, x, y, w, maxlines, skipleft);
	count = maxlines;
	while(count--) {
	    destptr = MoveDWORDS(destptr, (CARD32*)src, dwords);
	    src += srcwidth;
	    if(destptr > maxptr) 
		destptr = (CARD32*)infoRec->ColorExpandBase;
	}
	h -= maxlines;
	y += maxlines;
    }
    	
    (*infoRec->SubsequentCPUToScreenColorExpandFill)(
				pScrn, x, y, w, h, skipleft);

    while(h--) {
	destptr = MoveDWORDS(destptr, (CARD32*)src, dwords);
	src += srcwidth;
	if(destptr > maxptr) 
		destptr = (CARD32*)infoRec->ColorExpandBase;
    }
    DISABLE_CLIP();
    SET_SYNC_FLAG(infoRec);
}


void 
MGAFillColorExpandRects(
   ScrnInfoPtr pScrn,
   int fg, int bg, int rop,
   unsigned int planemask,
   int nBox,
   BoxPtr pBox,
   int xorg, int yorg,
   PixmapPtr pPix
){
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;
    CARD32 *destptr, *maxptr;
    StippleScanlineProcPtr StippleFunc;
    unsigned char *src = (unsigned char*)pPix->devPrivate.ptr;
    int stipplewidth = pPix->drawable.width;
    int stippleheight = pPix->drawable.height;
    int srcwidth = pPix->devKind;
    int dwords, h, y, maxlines, count, srcx, srcy;
    unsigned char *srcp;

    if(stipplewidth <= 32) {
	if(stipplewidth & (stipplewidth - 1))	
	    StippleFunc = XAAStippleScanlineFuncLSBFirst[1];
	else	
	    StippleFunc = XAAStippleScanlineFuncLSBFirst[0];
    } else
    	StippleFunc = XAAStippleScanlineFuncLSBFirst[2];

    (*infoRec->SetupForCPUToScreenColorExpandFill)(
				pScrn, fg, bg, rop, planemask);

    while(nBox--) {
	dwords = (pBox->x2 - pBox->x1 + 31) >> 5;
	destptr = (CARD32*)infoRec->ColorExpandBase;
	maxptr = destptr + infoRec->ColorExpandRange - dwords;
	maxlines = MAX_BLIT_DWORDS / dwords;
	y = pBox->y1;
	h = pBox->y2 - y;

	srcy = (pBox->y1 - yorg) % stippleheight;
	if(srcy < 0) srcy += stippleheight;
	srcx = (pBox->x1 - xorg) % stipplewidth;
	if(srcx < 0) srcx += stipplewidth;

	srcp = (srcwidth * srcy) + src;

	while(h > maxlines) {
           (*infoRec->SubsequentCPUToScreenColorExpandFill)(pScrn, 
			pBox->x1, y, pBox->x2 - pBox->x1, maxlines, 0);
	   count = maxlines;
	   while(count--) {
		destptr = (*StippleFunc)(
			destptr, (CARD32*)srcp, srcx, stipplewidth, dwords);
		if(destptr > maxptr) 
		    destptr = (CARD32*)infoRec->ColorExpandBase;
		srcy++;
		srcp += srcwidth;
		if (srcy >= stippleheight) {
		   srcy = 0;
		   srcp = src;
		}
	   }
	   h -= maxlines;
	   y += maxlines;
	} 

      	(*infoRec->SubsequentCPUToScreenColorExpandFill)(pScrn, 
			pBox->x1, y , pBox->x2 - pBox->x1, h, 0);

	while(h--) {
	    destptr = (*StippleFunc)(
		destptr, (CARD32*)srcp, srcx, stipplewidth, dwords);
	    if(destptr > maxptr) 
		destptr = (CARD32*)infoRec->ColorExpandBase;
	    srcy++;
	    srcp += srcwidth;
	    if (srcy >= stippleheight) {
		srcy = 0;
		srcp = src;
	    }
	}

	pBox++;
    }
    DISABLE_CLIP();
    SET_SYNC_FLAG(infoRec);
}


void
MGAFillSolidRectsDMA(
    ScrnInfoPtr pScrn,
    int	fg, int rop,
    unsigned int planemask,
    int		nBox, 		/* number of rectangles to fill */
    BoxPtr	pBox  		/* Pointer to first rectangle to fill */
){
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;
    CARD32 *base = (CARD32*)pMga->ILOADBase;

    SET_SYNC_FLAG(infoRec);
    (*infoRec->SetupForSolidFill)(pScrn, fg, rop, planemask);

    if(nBox & 1) {
	OUTREG(MGAREG_FXBNDRY, ((pBox->x2) << 16) | pBox->x1);
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, 
		(pBox->y1 << 16) | (pBox->y2 - pBox->y1));
	nBox--; pBox++;
    }

    if(!nBox) return;

    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_GENERAL);
    while(nBox) {
	base[0] = DMAINDICIES(MGAREG_FXBNDRY, MGAREG_YDSTLEN + MGAREG_EXEC,
                MGAREG_FXBNDRY, MGAREG_YDSTLEN + MGAREG_EXEC);
	base[1] = ((pBox->x2) << 16) | pBox->x1;
	base[2] = (pBox->y1 << 16) | (pBox->y2 - pBox->y1);
	pBox++;
	base[3] = ((pBox->x2) << 16) | pBox->x1;
	base[4] = (pBox->y1 << 16) | (pBox->y2 - pBox->y1);
	pBox++;
	base += 5; nBox -= 2;
    }
    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_BLIT);  
}

void
MGAFillSolidSpansDMA(
   ScrnInfoPtr pScrn,
   int fg, int rop,
   unsigned int planemask,
   int n,
   DDXPointPtr ppt,
   int *pwidth, int fSorted 
){
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;
    CARD32 *base = (CARD32*)pMga->ILOADBase;

    SET_SYNC_FLAG(infoRec);
    (*infoRec->SetupForSolidFill)(pScrn, fg, rop, planemask);

    if(n & 1) {
	OUTREG(MGAREG_FXBNDRY, ((ppt->x + *pwidth) << 16) | ppt->x);
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (ppt->y << 16) | 1);
	ppt++; pwidth++; n--;
    }

    if(!n) return;
    if(n > 838860) n = 838860;  /* maximum number we have room for */

    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_GENERAL);
    while(n) {
	base[0] = DMAINDICIES(MGAREG_FXBNDRY, MGAREG_YDSTLEN + MGAREG_EXEC,
                MGAREG_FXBNDRY, MGAREG_YDSTLEN + MGAREG_EXEC);
	base[1] = ((ppt->x + *(pwidth++)) << 16) | ppt->x;
	base[2] = (ppt->y << 16) | 1;
	ppt++;
	base[3] = ((ppt->x + *(pwidth++)) << 16) | ppt->x;
	base[4] = (ppt->y << 16) | 1;
	ppt++;
	base += 5; n -= 2;
    }
    OUTREG(MGAREG_OPMODE, MGAOPM_DMA_BLIT);  
}


void
MGAFillMono8x8PatternRectsTwoPass(
    ScrnInfoPtr pScrn,
    int	fg, int bg, int rop,
    unsigned int planemask,
    int	nBoxInit,
    BoxPtr pBoxInit,
    int pattern0, int pattern1,
    int xorg, int yorg
){
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;
    int	nBox, SecondPassColor;
    BoxPtr pBox;

    if((rop == GXcopy) && (bg != -1)) {
	SecondPassColor = bg;
	bg = -1;
    } else SecondPassColor = -1;

    WAITFIFO(1);
    OUTREG(MGAREG_SHIFT, (((-yorg) & 0x07) << 4) | ((-xorg) & 0x07));

SECOND_PASS:

    nBox = nBoxInit;
    pBox = pBoxInit;

    (*infoRec->SetupForMono8x8PatternFill)(pScrn, pattern0, pattern1,
					fg, bg, rop, planemask);

    while(nBox--) {
	WAITFIFO(2);
	OUTREG(MGAREG_FXBNDRY, ((pBox->x2) << 16) | pBox->x1);
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, 
			(pBox->y1 << 16) | (pBox->y2 - pBox->y1));
	pBox++;
    }

    if(SecondPassColor != -1) {
	fg = SecondPassColor;
	SecondPassColor = -1;
	pattern0 = ~pattern0;
	pattern1 = ~pattern1;
	goto SECOND_PASS;
    }

    SET_SYNC_FLAG(infoRec);
}

void MGANonTEGlyphRenderer(
   ScrnInfoPtr pScrn,
   int x, int y, int n,
   NonTEGlyphPtr glyphs,
   BoxPtr pbox,
   int fg, int rop,
   unsigned int planemask
){
    MGAPtr pMga = MGAPTR(pScrn);
    XAAInfoRecPtr infoRec = pMga->AccelInfoRec;
    int x1, x2, y1, y2, i, h, skiptop, dwords, maxlines;
    unsigned char *src;

    (*infoRec->SetupForCPUToScreenColorExpandFill)(
					pScrn, fg, -1, rop, planemask);
    WAITFIFO(1);
    OUTREG(MGAREG_CXBNDRY, ((pbox->x2 - 1) << 16) | pbox->x1);      

    for(i = 0; i < n; i++, glyphs++) {
	if(!glyphs->srcwidth) continue;

	y1 = y - glyphs->yoff;
	y2 = y1 + glyphs->height;
	if(y1 < pbox->y1) {
	    skiptop = pbox->y1 - y1;
	    y1 = pbox->y1;
	} else skiptop = 0;
	if(y2 > pbox->y2) y2 = pbox->y2;

	h = y2 - y1;
	if(h <= 0) continue;

	src = glyphs->bits + (skiptop * glyphs->srcwidth);

	dwords = glyphs->srcwidth >> 2;  /* dwords per line */
	x1 = x + glyphs->start;
	x2 = x1 + (dwords << 5);

	maxlines = min(MAX_BLIT_DWORDS, infoRec->ColorExpandRange);
	maxlines /= dwords; 
     
	while(h > maxlines) {
	    WAITFIFO(4);
	    OUTREG(MGAREG_AR0, (dwords * maxlines << 5) - 1);
	    OUTREG(MGAREG_AR3, 0);
	    OUTREG(MGAREG_FXBNDRY, ((x2 - 1) << 16) | (x1 & 0xFFFF));
	    OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y1 << 16) | h);
	    MoveDWORDS((CARD32*)infoRec->ColorExpandBase, 
				(CARD32*)src, dwords * maxlines);
	    src += dwords * maxlines << 2;
	    h -= maxlines;
	    y1 += maxlines;
	}
    	
	dwords *= h; 	/* total dwords */
	WAITFIFO(4);
	OUTREG(MGAREG_AR0, (dwords << 5) - 1);
	OUTREG(MGAREG_AR3, 0);
	OUTREG(MGAREG_FXBNDRY, ((x2 - 1) << 16) | (x1 & 0xFFFF));
	OUTREG(MGAREG_YDSTLEN + MGAREG_EXEC, (y1 << 16) | h);

	MoveDWORDS((CARD32*)infoRec->ColorExpandBase, (CARD32*)src, dwords);
    }  

    DISABLE_CLIP();
    SET_SYNC_FLAG(infoRec);
}

void
MGAValidatePolyArc(
   GCPtr 	pGC,
   unsigned long changes,
   DrawablePtr pDraw
){
   MGAPtr pMga = MGAPTR(xf86Screens[pGC->pScreen->myNum]);

   if((pMga->AccelFlags & MGA_NO_PLANEMASK) & (pGC->planemask != ~0))
	return;

   if(!pGC->lineWidth && (pGC->fillStyle == FillSolid) &&
	(pGC->lineStyle == LineSolid) && 
	((pGC->alu != GXcopy) || (pGC->planemask != ~0))) {
	pGC->ops->PolyArc = MGAPolyArcThinSolid;
   }
}

#endif









