/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/mga/mga_storm.c,v 1.22 1998/08/30 04:49:42 dawes Exp $ */


/* All drivers should typically include these */
#include "xf86.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"

/* Drivers that need to access the PCI config space directly need this */
#include "xf86Pci.h"

/* Drivers for PCI hardware need this */
#include "xf86PciInfo.h"

/* Drivers that use XAA need this */
#include "xaa.h"
#include "xaalocal.h"
#include "xf86fbman.h"
#include "miline.h"

#include "vgaHWmmio.h"

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

extern void MGAWriteBitmapColorExpand(ScrnInfoPtr pScrn, int x, int y,
				int w, int h, unsigned char *src, int srcwidth,
				int skipleft, int fg, int bg, int rop,
				unsigned int planemask);
extern void MGAFillColorExpandRects(ScrnInfoPtr pScrn, int fg, int bg, int rop,
				unsigned int planemask, int nBox, BoxPtr pBox,
				int xorg, int yorg, PixmapPtr pPix);
extern void MGASetClippingRectangle(ScrnInfoPtr pScrn, int x1, int y1,
				int x2, int y2);

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
    case PCI_CHIP_MGA1064:
    default:
	pMga->AccelFlags = 0;
        break;
    }
  

    /* fill out infoPtr here */
    infoPtr->Flags = 	PIXMAP_CACHE | 
			MICROSOFT_ZERO_LINE_BIAS;

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
    /* NOTE: don't disable this without disabling WriteBitmap and
		FillColorExpandRects */
    infoPtr->CPUToScreenColorExpandFillFlags = 	CPU_TRANSFER_PAD_DWORD |
					SCANLINE_PAD_DWORD |
					BIT_ORDER_IN_BYTE_LSBFIRST |
					LEFT_EDGE_CLIPPING |
					LEFT_EDGE_CLIPPING_NEGATIVE_X |
					SYNC_AFTER_COLOR_EXPAND;
    infoPtr->ColorExpandRange = 0x1C00;
    infoPtr->ColorExpandBase = pMga->IOBase;
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
    infoPtr->ImageWriteRange = 0x1C00;
    infoPtr->ImageWriteBase = pMga->IOBase;
    infoPtr->SetupForImageWrite = MGANAME(SetupForImageWrite);
    infoPtr->SubsequentImageWriteRect = MGANAME(SubsequentImageWriteRect);

#if 0
    /* image reads */
    infoPtr->ImageReadFlags = 	CPU_TRANSFER_PAD_DWORD |
				SCANLINE_PAD_DWORD;
    infoPtr->ImageReadRange = 0x1C00;
    infoPtr->ImageReadBase = pMga->IOBase;
    infoPtr->SetupForImageRead = MGANAME(SetupForImageRead);
    infoPtr->SubsequentImageReadRect = MGANAME(SubsequentImageReadRect);
#endif

    /* midrange replacements */

    infoPtr->WriteBitmap = MGAWriteBitmapColorExpand;
    infoPtr->FillColorExpandRects = MGAFillColorExpandRects; 

#if PSZ == 24
    infoPtr->ImageWriteFlags |= NO_PLANEMASK;
    infoPtr->ScreenToScreenCopyFlags |= NO_PLANEMASK;
    infoPtr->CPUToScreenColorExpandFillFlags |= NO_PLANEMASK;
    infoPtr->WriteBitmapFlags |= NO_PLANEMASK;
    infoPtr->SolidFillFlags |= NO_PLANEMASK;
    infoPtr->SolidLineFlags |= NO_PLANEMASK;
    infoPtr->Mono8x8PatternFillFlags |= NO_PLANEMASK; 
    infoPtr->FillColorExpandRectsFlags |= NO_PLANEMASK; 
    infoPtr->ScreenToScreenColorExpandFillFlags |= NO_PLANEMASK;
#endif

    
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
    
    switch( pScrn->bitsPerPixel )
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
    
    WAITFIFO(10);
    OUTREG(MGAREG_PITCH, pScrn->displayWidth);
    OUTREG(MGAREG_YDSTORG, pMga->YDstOrg);
    OUTREG(MGAREG_MACCESS, maccess);
    pMga->PlaneMask = ~0;
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
    	OUTREG(MGAREG_DWGCTL, MGAAtypeNoBLK[rop] | MGADWG_SHIFTZERO | 
			MGADWG_BITBLT | MGADWG_BFCOL);
    } else {
	if(pMga->HasFBitBlt && (rop == GXcopy)) 
	   pMga->AccelInfoRec->SubsequentScreenToScreenCopy = 
		MGANAME(SubsequentScreenToScreenCopy_FastBlit);
    	OUTREG(MGAREG_DWGCTL, MGAAtypeNoBLK[rop] | MGADWG_SHIFTZERO | 
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
	   if(((srcY + h) > pMga->MaxFastBlitY) ||
				((dstY + h) > pMga->MaxFastBlitY)) 
	   goto FASTBLIT_BAILOUT;
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
		OUTREG(MGAREG_DWGCTL, MGAAtypeNoBLK[GXcopy] | 
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
		    MGADWG_BMONOLEF | MGAAtypeNoBLK[rop];
    else
#endif
    pMga->FilledRectCMD = MGADWG_TRAP | MGADWG_SOLID | MGADWG_ARZERO | 
		    MGADWG_SGNZERO | MGADWG_SHIFTZERO | 
		    MGADWG_BMONOLEF | MGAAtype[rop];

    pMga->SolidLineCMD =  MGADWG_SOLID | MGADWG_SHIFTZERO | MGADWG_BFCOL | 
		    MGAAtypeNoBLK[rop];

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
            pMga->PatternRectCMD |= MGADWG_TRANSC | MGAAtypeNoBLK[rop];
	else
#endif
            pMga->PatternRectCMD |= MGADWG_TRANSC | MGAAtype[rop];

	WAITFIFO(5);
    } else {
#if PSZ == 24
	if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION) && RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
	if(pMga->AccelFlags & BLK_OPAQUE_EXPANSION) 
#endif
        	pMga->PatternRectCMD |= MGAAtype[rop];
	else
        	pMga->PatternRectCMD |= MGAAtypeNoBLK[rop];
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
            mgaCMD |= MGADWG_TRANSC | MGAAtypeNoBLK[rop];
	else
#endif
            mgaCMD |= MGADWG_TRANSC | MGAAtype[rop];

	WAITFIFO(3);
    } else {
#if PSZ == 24
	if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION) && 
		RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
	if(pMga->AccelFlags & BLK_OPAQUE_EXPANSION) 
#endif
        	mgaCMD |= MGAAtype[rop];
	else
        	mgaCMD |= MGAAtypeNoBLK[rop];
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
			MGADWG_SGNZERO | MGAAtypeNoBLK[rop]);
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
            mgaCMD |= MGADWG_TRANSC | MGAAtypeNoBLK[rop];
	else
#endif
            mgaCMD |= MGADWG_TRANSC | MGAAtype[rop];

	WAITFIFO(4);
    } else {
#if PSZ == 24
	if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION) && 
		RGBEQUAL(fg) && RGBEQUAL(bg)) 
#else
	if((pMga->AccelFlags & BLK_OPAQUE_EXPANSION)) 
#endif
        	mgaCMD |= MGAAtype[rop];
	else
        	mgaCMD |= MGAAtypeNoBLK[rop];
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

#endif









