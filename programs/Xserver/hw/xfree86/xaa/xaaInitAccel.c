/* $XFree86: xc/programs/Xserver/hw/xfree86/xaa/xaaInitAccel.c,v 1.2 1998/07/25 16:58:48 dawes Exp $ */

#include "misc.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xf86_OSproc.h"

#include "X.h"
#include "scrnintstr.h"
#include "xf86str.h"
#include "xaa.h"
#include "xaalocal.h"
#include "xf86fbman.h"


Bool
XAAInitAccel(ScreenPtr pScreen, XAAInfoRecPtr infoRec)
{
    int index = pScreen->myNum;
    ScrnInfoPtr pScrn = xf86Screens[index];
    Bool HaveScreenToScreenCopy = FALSE;
    Bool HaveColorExpansion = FALSE;
    Bool HaveScanlineColorExpansion = FALSE;
    Bool HaveSolidFillRect = FALSE;
    Bool HaveMono8x8PatternFillRect = FALSE;
    Bool HaveColor8x8PatternFillRect = FALSE;
    Bool HaveSolidFillTrap = FALSE;
    Bool HaveMono8x8PatternFillTrap = FALSE;
    Bool HaveColor8x8PatternFillTrap = FALSE;
    Bool HaveSolidTwoPointLine = FALSE;
    Bool HaveSolidBresenhamLine = FALSE;
    Bool HaveSolidHorVertLine = FALSE;
    Bool HaveDashedTwoPointLine = FALSE;
    Bool HaveDashedBresenhamLine = FALSE;
    Bool HaveDashedHorVertLine = FALSE;
    Bool HaveClipper = FALSE;
    Bool HaveImageWriteRect = FALSE;
    Bool HaveScanlineImageWriteRect = FALSE;
    Bool HaveScreenToScreenColorExpandCopy = FALSE;

    infoRec->pScrn = pScrn;
    infoRec->NeedToSync = FALSE;

    /* must have a Sync function */
    if(!infoRec->Sync) return FALSE;

    infoRec->FullPlanemask =  (1 << pScrn->depth) - 1;

    xf86DrvMsg(index, X_INFO, 
	"Using XFree86 Acceleration Architecture (XAA)\n");


    /************** Low Level *************/

    /**** CopyArea ****/

    if(infoRec->SetupForScreenToScreenCopy &&
       infoRec->SubsequentScreenToScreenCopy) 
	HaveScreenToScreenCopy = TRUE;

    /**** Solid Filled Rects ****/

    if(infoRec->SetupForSolidFill && infoRec->SubsequentSolidFillRect) {
		HaveSolidFillRect = TRUE;
	if(infoRec->SubsequentSolidFillTrap)
		HaveSolidFillTrap = TRUE;
    }

    /**** Solid lines ****/
    if(infoRec->SetupForSolidLine) {
	if(infoRec->SubsequentSolidTwoPointLine)
	    HaveSolidTwoPointLine = TRUE;
	if(infoRec->SubsequentSolidBresenhamLine)
	    HaveSolidBresenhamLine = TRUE;

	if(infoRec->SubsequentSolidHorVertLine)
	    HaveSolidHorVertLine = TRUE;
	else if(HaveSolidTwoPointLine) {
	    infoRec->SubsequentSolidHorVertLine = 
			XAASolidHorVertLineAsTwoPoint;
	    HaveSolidHorVertLine = TRUE;
	} else if(HaveSolidBresenhamLine) {
	    infoRec->SubsequentSolidHorVertLine = 
			XAASolidHorVertLineAsBresenham;
	    HaveSolidHorVertLine = TRUE;
	}
    } else if(HaveSolidFillRect) {
	infoRec->SetupForSolidLine = infoRec->SetupForSolidFill;
	infoRec->SubsequentSolidHorVertLine = XAASolidHorVertLineAsRects;
	HaveSolidHorVertLine = TRUE;
    }


    /**** Dashed lines ****/
    if(infoRec->SetupForDashedLine && infoRec->DashPatternMaxLength) {
	if(infoRec->SubsequentDashedTwoPointLine)
	    HaveDashedTwoPointLine = TRUE;
	if(infoRec->SubsequentDashedBresenhamLine)
	    HaveDashedBresenhamLine = TRUE;

	if(infoRec->SubsequentDashedHorVertLine)
	    HaveDashedHorVertLine = TRUE;
#if 0
	else if(HaveDashedTwoPointLine) {
	    infoRec->SubsequentDashedHorVertLine = 
			XAADashedHorVertLineAsTwoPoint;
	    HaveDashedHorVertLine = TRUE;
	} else if(HaveDashedBresenhamLine) {
	    infoRec->SubsequentDashedHorVertLine = 
			XAADashedHorVertLineAsBresenham;
	    HaveDashedHorVertLine = TRUE;
	} 
#endif
    }

    if(infoRec->SetClippingRectangle)
	HaveClipper = TRUE;

    /**** 8x8 Mono Pattern Filled Rects ****/

   if(infoRec->SetupForMono8x8PatternFill &&
			infoRec->SubsequentMono8x8PatternFillRect) {
	HaveMono8x8PatternFillRect = TRUE;
	if(infoRec->SubsequentMono8x8PatternFillTrap)
		HaveMono8x8PatternFillTrap = TRUE;

        if(infoRec->Mono8x8PatternFillFlags & 
				HARDWARE_PATTERN_PROGRAMMED_BITS) {
	    infoRec->CanDoMono8x8 = TRUE;
	} else {	/* others require caching */
           int min_pitch;
	   infoRec->PixmapCacheFlags |= CACHE_MONO_8x8;

	   switch(pScrn->bitsPerPixel) {
	   case 32: min_pitch = 2; break;
	   case 24: min_pitch = 3; break;
	   case 16: min_pitch = 4; break;
	   default: min_pitch = 8; break;
	   }
 
           if(min_pitch > infoRec->MonoPatternPitch)
		infoRec->MonoPatternPitch = min_pitch;

	   if(infoRec->Mono8x8PatternFillFlags & 
				HARDWARE_PATTERN_PROGRAMMED_ORIGIN) {
		if(!infoRec->CacheWidthMono8x8Pattern ||
		   !infoRec->CacheHeightMono8x8Pattern) {
			infoRec->CacheWidthMono8x8Pattern = 
						infoRec->MonoPatternPitch;
			infoRec->CacheHeightMono8x8Pattern = 1;
		}
	   } else {
		int numPerLine = 128/infoRec->MonoPatternPitch;

		if(!infoRec->CacheWidthMono8x8Pattern ||
		   !infoRec->CacheHeightMono8x8Pattern) {
			infoRec->CacheWidthMono8x8Pattern = 
				numPerLine * infoRec->MonoPatternPitch;
			infoRec->CacheHeightMono8x8Pattern = 
				(64 + numPerLine - 1)/numPerLine;
		}
	   }
	}
   }


    /**** 8x8 Color Pattern Filled Rects ****/

   if(infoRec->SetupForColor8x8PatternFill &&
      infoRec->SubsequentColor8x8PatternFillRect) {
	HaveColor8x8PatternFillRect = TRUE;
	if(infoRec->SubsequentColor8x8PatternFillTrap)
		HaveColor8x8PatternFillTrap = TRUE;

	infoRec->PixmapCacheFlags |= CACHE_COLOR_8x8;

	if(infoRec->Color8x8PatternFillFlags & 
				HARDWARE_PATTERN_PROGRAMMED_ORIGIN) {
	    if(!infoRec->CacheWidthColor8x8Pattern ||
	       !infoRec->CacheHeightColor8x8Pattern) {
		infoRec->CacheWidthColor8x8Pattern = 64;
		infoRec->CacheHeightColor8x8Pattern = 1;
	    }
	} else {
	    if(!infoRec->CacheWidthColor8x8Pattern ||
	       !infoRec->CacheHeightColor8x8Pattern) {
		infoRec->CacheWidthColor8x8Pattern = 128;
		infoRec->CacheHeightColor8x8Pattern = 8;
	    }
	}
   }

    /**** Color Expansion ****/


    if(infoRec->SetupForColorExpandFill && infoRec->ColorExpandBase &&
       infoRec->SubsequentColorExpandFillRect) {
	int dwordsNeeded = pScrn->virtualX;

	infoRec->ColorExpandRange >>= 2;	/* convert to DWORDS */
	HaveColorExpansion = TRUE;	   

	if(infoRec->ColorExpandFillFlags & LEFT_EDGE_CLIPPING_NEGATIVE_X)
	    dwordsNeeded += 31;
	dwordsNeeded = (dwordsNeeded + 31) >> 5;
	if(dwordsNeeded > infoRec->ColorExpandRange)
	   infoRec->ColorExpandFillFlags |= CPU_TRANSFER_BASE_FIXED;	
    } 

    /**** Scanline Color Expansion ****/
  
    if(infoRec->SetupForScanlineColorExpandFill &&
       infoRec->SubsequentScanlineColorExpandFillRect &&
       infoRec->SubsequentColorExpandScanline &&
       infoRec->ScanlineColorExpandBuffers && 
       (infoRec->NumScanlineColorExpandBuffers > 0))
	HaveScanlineColorExpansion = TRUE;


    /**** Screen to Screen Color Expansion ****/

    if(infoRec->SetupForScreenToScreenColorExpandCopy &&
       infoRec->SubsequentScreenToScreenColorExpandCopy)
	HaveScreenToScreenColorExpandCopy = TRUE;

    /**** Image Writes ****/

    if(infoRec->SetupForImageWrite && infoRec->ImageWriteBase &&
       infoRec->SubsequentImageWriteRect) {

	infoRec->ImageWriteRange >>= 2;	/* convert to DWORDS */
	if(infoRec->ImageWriteFlags & CPU_TRANSFER_BASE_FIXED)
	   infoRec->ImageWriteRange = 0;
	HaveImageWriteRect = TRUE;	
    } 

    /**** Scanline Image Writes ****/
  
    if(infoRec->SetupForScanlineImageWrite &&
       infoRec->SubsequentScanlineImageWriteRect &&
       infoRec->SubsequentImageWriteScanline &&
       infoRec->ScanlineImageWriteBuffers && 
       (infoRec->NumScanlineImageWriteBuffers > 0))
	HaveScanlineImageWriteRect = TRUE;


    if(HaveScreenToScreenCopy)
	xf86ErrorF("\tScreen to screen bit blits\n");
    if(HaveSolidFillRect)
	xf86ErrorF("\tSolid filled rectangles\n");
    if(HaveSolidFillTrap)
	xf86ErrorF("\tSolid filled trapezoids\n");
    if(HaveMono8x8PatternFillRect)
	xf86ErrorF("\t8x8 mono pattern filled rectangles\n");
    if(HaveMono8x8PatternFillTrap)
	xf86ErrorF("\t8x8 mono pattern filled trapezoids\n");
    if(HaveColor8x8PatternFillRect)
	xf86ErrorF("\t8x8 color pattern filled rectangles\n");
    if(HaveColor8x8PatternFillTrap)
	xf86ErrorF("\t8x8 color pattern filled trapezoids\n");

    if(HaveColorExpansion)
	xf86ErrorF("\tCPU to Screen color expansion\n");
    else if(HaveScanlineColorExpansion)
	xf86ErrorF("\tIndirect CPU to Screen color expansion\n");

    if(HaveScreenToScreenColorExpandCopy)
	xf86ErrorF("\tScreen to Screen color expansion\n");

    if(HaveSolidTwoPointLine || HaveSolidBresenhamLine)
	xf86ErrorF("\tSolid Lines\n");
    else if(HaveSolidHorVertLine)
	xf86ErrorF("\tSolid Horizontal and Vertical Lines\n");

    if(HaveDashedTwoPointLine || HaveDashedBresenhamLine)
	xf86ErrorF("\tDashed Lines\n");
    else if(HaveDashedHorVertLine)
	xf86ErrorF("\tDashed Horizontal and Vertical Lines\n");

    if(HaveImageWriteRect)
	xf86ErrorF("\tImage Writes\n");
    else if(HaveScanlineImageWriteRect)
	xf86ErrorF("\tScanline Image Writes\n");



    /************** Mid Level *************/

    /**** ScreenToScreenBitBlt ****/

    if(infoRec->ScreenToScreenBitBlt) {
	xf86ErrorF("\tDriver provided ScreenToScreenBitBlt replacement\n");
    } else if(HaveScreenToScreenCopy) {
	infoRec->ScreenToScreenBitBlt = XAAScreenToScreenBitBlt;
	infoRec->ScreenToScreenBitBltFlags = infoRec->ScreenToScreenCopyFlags;
    }

    /**** FillSolidRects ****/

    if(infoRec->FillSolidRects) {
	xf86ErrorF("\tDriver provided FillSolidRects replacement\n");
    } else if(HaveSolidFillRect) {
	infoRec->FillSolidRects = XAAFillSolidRects;
	infoRec->FillSolidRectsFlags = infoRec->SolidFillFlags;
    }


    /**** FillSolidSpans ****/

    if(infoRec->FillSolidSpans) {
	xf86ErrorF("\tDriver provided FillSolidSpans replacement\n");
    } else if(HaveSolidFillRect) {
	infoRec->FillSolidSpans = XAAFillSolidSpans;
	infoRec->FillSolidSpansFlags = infoRec->SolidFillFlags;
    }


    /**** FillMono8x8PatternRects ****/

    if(infoRec->FillMono8x8PatternRects) {
	xf86ErrorF("\tDriver provided FillMono8x8PatternRects replacement\n");
    } else if(HaveMono8x8PatternFillRect) {
	infoRec->FillMono8x8PatternRects = 
	  (infoRec->Mono8x8PatternFillFlags & HARDWARE_PATTERN_SCREEN_ORIGIN) ?
	  XAAFillMono8x8PatternRectsScreenOrigin :
	  XAAFillMono8x8PatternRects; 

	infoRec->FillMono8x8PatternRectsFlags = 
			infoRec->Mono8x8PatternFillFlags;      
    }

    /**** FillMono8x8PatternSpans ****/

    if(infoRec->FillMono8x8PatternSpans) {
	xf86ErrorF("\tDriver provided FillMono8x8PatternSpans replacement\n");
    } else if(HaveMono8x8PatternFillRect) {
	infoRec->FillMono8x8PatternSpans = 
	  (infoRec->Mono8x8PatternFillFlags & HARDWARE_PATTERN_SCREEN_ORIGIN) ?
	  XAAFillMono8x8PatternSpansScreenOrigin:
	  XAAFillMono8x8PatternSpans; 

	infoRec->FillMono8x8PatternSpansFlags = 
		infoRec->Mono8x8PatternFillFlags;      
    }

    /**** FillColor8x8Rects ****/

    if(infoRec->FillColor8x8PatternRects) {
	xf86ErrorF("\tDriver provided FillColor8x8PatternRects replacement\n");
    } else if(HaveColor8x8PatternFillRect) {
	infoRec->FillColor8x8PatternRects = 
	  (infoRec->Color8x8PatternFillFlags & HARDWARE_PATTERN_SCREEN_ORIGIN) ?
	  XAAFillColor8x8PatternRectsScreenOrigin :
	  XAAFillColor8x8PatternRects; 

	infoRec->FillColor8x8PatternRectsFlags = 
			infoRec->Color8x8PatternFillFlags;      
    }

    /**** FillColor8x8Spans ****/

    if(infoRec->FillColor8x8PatternSpans) {
	xf86ErrorF("\tDriver provided FillColor8x8PatternSpans replacement\n");
    } else if(HaveColor8x8PatternFillRect) {
	infoRec->FillColor8x8PatternSpans = 
	  (infoRec->Color8x8PatternFillFlags & HARDWARE_PATTERN_SCREEN_ORIGIN) ?
	  XAAFillColor8x8PatternSpansScreenOrigin:
	  XAAFillColor8x8PatternSpans; 

	infoRec->FillColor8x8PatternSpansFlags = 
		infoRec->Color8x8PatternFillFlags;      
    }


    /**** FillCacheBltRects ****/

    if(infoRec->FillCacheBltRects) {
	xf86ErrorF("\tDriver provided FillCacheBltRects replacement\n");
    } else if(HaveScreenToScreenCopy) {
	infoRec->FillCacheBltRects = XAAFillCacheBltRects;
	infoRec->FillCacheBltRectsFlags = infoRec->ScreenToScreenCopyFlags;     
    }

    /**** FillCacheBltSpans ****/

    if(infoRec->FillCacheBltSpans) {
	xf86ErrorF("\tDriver provided FillCacheBltSpans replacement\n");
    } else if(HaveScreenToScreenCopy) {
	infoRec->FillCacheBltSpans = XAAFillCacheBltSpans;
	infoRec->FillCacheBltSpansFlags = infoRec->ScreenToScreenCopyFlags;     
    }


    /**** FillCacheExpandRects ****/

    if(infoRec->FillCacheExpandRects) {
	xf86ErrorF("\tDriver provided FillCacheExpandRects replacement\n");
    } else if(HaveScreenToScreenColorExpandCopy) {
	infoRec->FillCacheExpandRects = XAAFillCacheExpandRects;
	infoRec->FillCacheExpandRectsFlags = 
		infoRec->ScreenToScreenColorExpandCopyFlags;     
    }
   	
    /**** FillCacheExpandSpans ****/

    if(infoRec->FillCacheExpandSpans) {
	xf86ErrorF("\tDriver provided FillCacheExpandSpans replacement\n");
    } else if(HaveScreenToScreenColorExpandCopy) {
	infoRec->FillCacheExpandSpans = XAAFillCacheExpandSpans;
	infoRec->FillCacheExpandSpansFlags = 
		infoRec->ScreenToScreenColorExpandCopyFlags;     
    }


    /**** FillColorExpandRects ****/

    if(infoRec->FillColorExpandRects) {
	xf86ErrorF("\tDriver provided FillColorExpandRects replacement\n");
    } else if(HaveColorExpansion) {
#if 0
	if (infoRec->ColorExpandFillFlags & TRIPLE_BITS_24BPP) {
	    if(infoRec->ColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST) {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->FillColorExpandRects = 
			XAAFillColorExpandRects3MSBFirstFixedBase;
		else
		    infoRec->FillColorExpandRects = 
			XAAFillColorExpandRects3MSBFirst;
	    } else {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->FillColorExpandRects = 
			XAAFillColorExpandRects3LSBFirstFixedBase;
		else
		    infoRec->FillColorExpandRects = 
			XAAFillColorExpandRects3LSBFirst;
	    }
	} else 
#endif
	{
	    if(infoRec->ColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST) {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->FillColorExpandRects = 
			XAAFillColorExpandRectsMSBFirstFixedBase;
		else
		    infoRec->FillColorExpandRects = 
				XAAFillColorExpandRectsMSBFirst;
	    } else {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->FillColorExpandRects = 
			XAAFillColorExpandRectsLSBFirstFixedBase;
		else
		    infoRec->FillColorExpandRects = 
			XAAFillColorExpandRectsLSBFirst;
	    }
	}
	infoRec->FillColorExpandRectsFlags = 
	  infoRec->ColorExpandFillFlags & ~TRANSPARENCY_ONLY;
    } else if(HaveScanlineColorExpansion) {
#if 0
	if (infoRec->ScanlineColorExpandFillFlags & TRIPLE_BITS_24BPP) {
	    if(infoRec->ScanlineColorExpandFillFlags & 
					BIT_ORDER_IN_BYTE_MSBFIRST)
		infoRec->FillColorExpandRects = 
		    XAAFillColorExpandRects3MSBFirst;
	    else
		infoRec->FillColorExpandRects = 
		    XAAFillColorExpandRects3LSBFirst;
	} else 
#endif
	{
	    if(infoRec->ScanlineColorExpandFillFlags & 
					BIT_ORDER_IN_BYTE_MSBFIRST)
		infoRec->FillColorExpandRects = 
		    XAAFillScanlineColorExpandRectsMSBFirst;
	    else
		infoRec->FillColorExpandRects = 
		    XAAFillScanlineColorExpandRectsLSBFirst;
	}
	infoRec->FillColorExpandRectsFlags = 
		infoRec->ScanlineColorExpandFillFlags & ~TRANSPARENCY_ONLY;
    }

    /**** FillColorExpandSpans ****/

    if(infoRec->FillColorExpandSpans) {
	xf86ErrorF("\tDriver provided FillColorExpandSpans replacement\n");
    } else if(HaveColorExpansion) {
#if 0
	if (infoRec->ColorExpandFillFlags & TRIPLE_BITS_24BPP) {
	    if(infoRec->ColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST) {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->FillColorExpandSpans = 
			XAAFillColorExpandSpans3MSBFirstFixedBase;
		else
		    infoRec->FillColorExpandSpans = 
			XAAFillColorExpandSpans3MSBFirst;
	    } else {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->FillColorExpandSpans = 
			XAAFillColorExpandSpans3LSBFirstFixedBase;
		else
		    infoRec->FillColorExpandSpans = 
			XAAFillColorExpandSpans3LSBFirst;
	    }
	} else 
#endif
	{
	    if(infoRec->ColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST) {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->FillColorExpandSpans = 
			XAAFillColorExpandSpansMSBFirstFixedBase;
		else
		    infoRec->FillColorExpandSpans = 
				XAAFillColorExpandSpansMSBFirst;
	    } else {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->FillColorExpandSpans = 
			XAAFillColorExpandSpansLSBFirstFixedBase;
		else
		    infoRec->FillColorExpandSpans = 
			XAAFillColorExpandSpansLSBFirst;
	    }
	}
	infoRec->FillColorExpandSpansFlags = 
	  infoRec->ColorExpandFillFlags & ~TRANSPARENCY_ONLY;
    } else if(HaveScanlineColorExpansion) {
#if 0
	if (infoRec->ScanlineColorExpandFillFlags & TRIPLE_BITS_24BPP) {
	    if(infoRec->ScanlineColorExpandFillFlags & 
					BIT_ORDER_IN_BYTE_MSBFIRST)
		infoRec->FillColorExpandSpans = 
		    XAAFillColorExpandSpans3MSBFirst;
	    else
		infoRec->FillColorExpandSpans = 
		    XAAFillColorExpandSpans3LSBFirst;
	} else 
#endif
	{
	    if(infoRec->ScanlineColorExpandFillFlags & 
					BIT_ORDER_IN_BYTE_MSBFIRST)
		infoRec->FillColorExpandSpans = 
		    XAAFillScanlineColorExpandSpansMSBFirst;
	    else
		infoRec->FillColorExpandSpans = 
		    XAAFillScanlineColorExpandSpansLSBFirst;
	}
	infoRec->FillColorExpandSpansFlags = 
		infoRec->ScanlineColorExpandFillFlags & ~TRANSPARENCY_ONLY;
    }



    /**** WriteBitmap ****/

    if(infoRec->WriteBitmap) {
	xf86ErrorF("\tDriver provided WriteBitmap replacement\n");
    } else if(HaveColorExpansion) {
	if (infoRec->ColorExpandFillFlags & TRIPLE_BITS_24BPP) {
	    if(infoRec->ColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST) {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->WriteBitmap = 
			XAAWriteBitmapColorExpand3MSBFirstFixedBase;
		else
		    infoRec->WriteBitmap = XAAWriteBitmapColorExpand3MSBFirst;
	    } else {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->WriteBitmap = 
			XAAWriteBitmapColorExpand3LSBFirstFixedBase;
		else
		    infoRec->WriteBitmap = XAAWriteBitmapColorExpand3LSBFirst;
	    }
	} else {
	    if(infoRec->ColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST) {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->WriteBitmap = 
			XAAWriteBitmapColorExpandMSBFirstFixedBase;
		else
		    infoRec->WriteBitmap = XAAWriteBitmapColorExpandMSBFirst;
	    } else {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->WriteBitmap = 
			XAAWriteBitmapColorExpandLSBFirstFixedBase;
		else
		    infoRec->WriteBitmap = XAAWriteBitmapColorExpandLSBFirst;
	    }
	}
	infoRec->WriteBitmapFlags = 
	  infoRec->ColorExpandFillFlags & ~TRANSPARENCY_ONLY;
    } else if(HaveScanlineColorExpansion) {
	if (infoRec->ColorExpandFillFlags & TRIPLE_BITS_24BPP) {
	    if(infoRec->ScanlineColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST)
		infoRec->WriteBitmap = 
		    XAAWriteBitmapScanlineColorExpand3MSBFirst;
	    else
		infoRec->WriteBitmap = 
		    XAAWriteBitmapScanlineColorExpand3LSBFirst;
	} else {
	    if(infoRec->ScanlineColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST)
		infoRec->WriteBitmap = 
		    XAAWriteBitmapScanlineColorExpandMSBFirst;
	    else
		infoRec->WriteBitmap = 
		    XAAWriteBitmapScanlineColorExpandLSBFirst;
	}
	infoRec->WriteBitmapFlags = 
		infoRec->ScanlineColorExpandFillFlags & ~TRANSPARENCY_ONLY;
    }


    
    /**** TE Glyphs ****/

    if(infoRec->TEGlyphRenderer) {
	xf86ErrorF("\tDriver provided TEGlyphRenderer replacement\n");
    } else if(HaveColorExpansion) {
        if (infoRec->ColorExpandFillFlags & TRIPLE_BITS_24BPP) {
	    if(infoRec->ColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST) {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->TEGlyphRenderer = 
			XAATEGlyphRenderer3MSBFirstFixedBase;
		else
		    infoRec->TEGlyphRenderer = XAATEGlyphRenderer3MSBFirst;
	    } else {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->TEGlyphRenderer = 
			XAATEGlyphRenderer3LSBFirstFixedBase;
		else
		    infoRec->TEGlyphRenderer = XAATEGlyphRenderer3LSBFirst;
	    }
	} else {
	    if(infoRec->ColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST) {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->TEGlyphRenderer = 
			XAATEGlyphRendererMSBFirstFixedBase;
		else
		    infoRec->TEGlyphRenderer = XAATEGlyphRendererMSBFirst;
	    } else {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED)
		    infoRec->TEGlyphRenderer = 
			XAATEGlyphRendererLSBFirstFixedBase;
		else
		    infoRec->TEGlyphRenderer = XAATEGlyphRendererLSBFirst;
	    }
	}
	infoRec->TEGlyphRendererFlags = infoRec->ColorExpandFillFlags;

	if(HaveSolidFillRect)
	   infoRec->TEGlyphRendererFlags &= ~TRANSPARENCY_ONLY;

    } else if(HaveScanlineColorExpansion) {
        if (infoRec->ColorExpandFillFlags & TRIPLE_BITS_24BPP) {
	    if(infoRec->ScanlineColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST)
		infoRec->TEGlyphRenderer = 
		    XAATEGlyphRendererScanline3MSBFirst;
	    else
		infoRec->TEGlyphRenderer = 
		    XAATEGlyphRendererScanline3LSBFirst;
	} else {
	    if(infoRec->ScanlineColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST)
		infoRec->TEGlyphRenderer = 
		    XAATEGlyphRendererScanlineMSBFirst;
	    else
		infoRec->TEGlyphRenderer = 
		    XAATEGlyphRendererScanlineLSBFirst;
	}

	infoRec->TEGlyphRendererFlags = infoRec->ScanlineColorExpandFillFlags;

	if(HaveSolidFillRect)
	   infoRec->TEGlyphRendererFlags &= ~TRANSPARENCY_ONLY;
    }


    /**** NonTE Glyphs ****/

    if(infoRec->NonTEGlyphRenderer) {
	xf86ErrorF("\tDriver provided NonTEGlyphRenderer replacement\n");
    } else if(HaveColorExpansion) {
	if (infoRec->ColorExpandFillFlags & TRIPLE_BITS_24BPP) {
	    if(infoRec->ColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST) {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED) 
		    infoRec->NonTEGlyphRenderer = 
			XAANonTEGlyphRenderer3MSBFirstFixedBase;
		else
		    infoRec->NonTEGlyphRenderer =
			XAANonTEGlyphRenderer3MSBFirst;
	    } else {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED) 
		    infoRec->NonTEGlyphRenderer = 
			XAANonTEGlyphRenderer3LSBFirstFixedBase;
		else 
		    infoRec->NonTEGlyphRenderer =
			XAANonTEGlyphRenderer3LSBFirst;
	    }
	} else {
	    if(infoRec->ColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST) {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED) 
		    infoRec->NonTEGlyphRenderer = 
			XAANonTEGlyphRendererMSBFirstFixedBase;
		else
		    infoRec->NonTEGlyphRenderer =
			XAANonTEGlyphRendererMSBFirst;
	    } else {
		if(infoRec->ColorExpandFillFlags & CPU_TRANSFER_BASE_FIXED) 
		    infoRec->NonTEGlyphRenderer = 
			XAANonTEGlyphRendererLSBFirstFixedBase;
		else 
		    infoRec->NonTEGlyphRenderer =
			XAANonTEGlyphRendererLSBFirst;
	    }
	}
	infoRec->NonTEGlyphRendererFlags = infoRec->ColorExpandFillFlags;

	if(HaveSolidFillRect)
	   infoRec->NonTEGlyphRendererFlags &= ~TRANSPARENCY_ONLY;

    } else if(HaveScanlineColorExpansion) {
	if (infoRec->ColorExpandFillFlags & TRIPLE_BITS_24BPP) {
	    if(infoRec->ScanlineColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST)
		infoRec->NonTEGlyphRenderer = 
			XAANonTEGlyphRendererScanline3MSBFirst;
	    else
		infoRec->NonTEGlyphRenderer = 
			XAANonTEGlyphRendererScanline3LSBFirst;
	} else {
	    if(infoRec->ScanlineColorExpandFillFlags & BIT_ORDER_IN_BYTE_MSBFIRST)
		infoRec->NonTEGlyphRenderer = 
			XAANonTEGlyphRendererScanlineMSBFirst;
	    else
		infoRec->NonTEGlyphRenderer = 
			XAANonTEGlyphRendererScanlineLSBFirst;
	  
	}

	infoRec->NonTEGlyphRendererFlags = 
			infoRec->ScanlineColorExpandFillFlags;
	
	if(HaveSolidFillRect)
	   infoRec->NonTEGlyphRendererFlags &= ~TRANSPARENCY_ONLY;
    }
    

    /**** WritePixmap ****/

    if(infoRec->WritePixmap) {
	xf86ErrorF("\tDriver provided WritePixmap replacement\n");
    } else if(HaveImageWriteRect) {
	infoRec->WritePixmap = XAAWritePixmap;
	infoRec->WritePixmapFlags = infoRec->ImageWriteFlags;
    } else if(HaveScanlineImageWriteRect) {
	infoRec->WritePixmap = XAAWritePixmapScanline;
	infoRec->WritePixmapFlags = infoRec->ScanlineImageWriteFlags;
    }


    /************** GC Level *************/

    /**** CopyArea ****/

    if(infoRec->CopyArea) {
	xf86ErrorF("\tDriver provided GC level CopyArea replacement\n");
    } else if(infoRec->ScreenToScreenBitBlt) {
	infoRec->CopyArea = XAACopyArea;
	infoRec->CopyAreaFlags = infoRec->ScreenToScreenBitBltFlags;
    }

    if(infoRec->CopyPlane) {
	xf86ErrorF("\tDriver provided GC level CopyPlane replacement\n");
    } else if(infoRec->WriteBitmap && 
		!(infoRec->WriteBitmapFlags & TRANSPARENCY_ONLY)) {
	infoRec->CopyPlane = XAACopyPlaneColorExpansion;
	infoRec->CopyPlaneFlags = infoRec->WriteBitmapFlags;
    }

    if(infoRec->PushPixelsSolid) {
	xf86ErrorF("\tDriver provided GC level PushPixelsSolid replacement\n");
    } else if(infoRec->WriteBitmap &&
		!(infoRec->WriteBitmapFlags & NO_TRANSPARENCY)) {
	infoRec->PushPixelsSolid = XAAPushPixelsSolidColorExpansion;
	infoRec->PushPixelsFlags = infoRec->WriteBitmapFlags;
    }

    if(infoRec->FillSolidRects) {
	if(!infoRec->PolyFillRectSolid) {
	    infoRec->PolyFillRectSolid = XAAPolyFillRectSolid;
	    infoRec->PolyFillRectSolidFlags = infoRec->FillSolidRectsFlags;
	}
    }
    if(infoRec->FillSolidSpans) {
	if(!infoRec->FillSpansSolid) {
	    infoRec->FillSpansSolid = XAAFillSpansSolid;
	    infoRec->FillSpansSolidFlags = infoRec->FillSolidSpansFlags;
	}
    }


    if(infoRec->FillMono8x8PatternRects) {
	if(!infoRec->PolyFillRectMono8x8Pattern) {
	    infoRec->PolyFillRectMono8x8Pattern = XAAPolyFillRectMono8x8Pattern;
	    infoRec->PolyFillRectMono8x8PatternFlags =
				infoRec->FillMono8x8PatternRectsFlags;
	}
    }
    if(infoRec->FillMono8x8PatternSpans) {
	if(!infoRec->FillSpansMono8x8Pattern) {
	    infoRec->FillSpansMono8x8Pattern = XAAFillSpansMono8x8Pattern;
	    infoRec->FillSpansMono8x8PatternFlags =
				infoRec->FillMono8x8PatternSpansFlags;
	}
    }


    if(infoRec->FillColor8x8PatternRects) {
	if(!infoRec->PolyFillRectColor8x8Pattern) {
	    infoRec->PolyFillRectColor8x8Pattern = 
				XAAPolyFillRectColor8x8Pattern;
	    infoRec->PolyFillRectColor8x8PatternFlags =
				infoRec->FillColor8x8PatternRectsFlags;
	}
    }
    if(infoRec->FillColor8x8PatternSpans) {
	if(!infoRec->FillSpansColor8x8Pattern) {
	    infoRec->FillSpansColor8x8Pattern = 
				XAAFillSpansColor8x8Pattern;
	    infoRec->FillSpansColor8x8PatternFlags =
				infoRec->FillColor8x8PatternSpansFlags;
	}
    }


    if(infoRec->FillCacheBltRects) {
	if(!infoRec->PolyFillRectCacheBlt) {
	    infoRec->PolyFillRectCacheBlt = XAAPolyFillRectCacheBlt;
	    infoRec->PolyFillRectCacheBltFlags =
				infoRec->FillCacheBltRectsFlags;
	}
    }
    if(infoRec->FillCacheBltSpans) {
	if(!infoRec->FillSpansCacheBlt) {
	    infoRec->FillSpansCacheBlt = XAAFillSpansCacheBlt;
	    infoRec->FillSpansCacheBltFlags =
				infoRec->FillCacheBltSpansFlags;
	}
    }

    if(infoRec->FillColorExpandRects) {
	if(!infoRec->PolyFillRectColorExpand) {
	    infoRec->PolyFillRectColorExpand = XAAPolyFillRectColorExpand;
	    infoRec->PolyFillRectColorExpandFlags =
				infoRec->FillColorExpandRectsFlags;
	}
    }
    if(infoRec->FillColorExpandSpans) {
	if(!infoRec->FillSpansColorExpand) {
	    infoRec->FillSpansColorExpand = XAAFillSpansColorExpand;
	    infoRec->FillSpansColorExpandFlags =
				infoRec->FillColorExpandSpansFlags;
	}
    }

    if(infoRec->FillCacheExpandRects) {
	if(!infoRec->PolyFillRectCacheExpand) {
	    infoRec->PolyFillRectCacheExpand = XAAPolyFillRectCacheExpand;
	    infoRec->PolyFillRectCacheExpandFlags =
				infoRec->FillCacheExpandRectsFlags;
	}
    }
    if(infoRec->FillCacheExpandSpans) {
	if(!infoRec->FillSpansCacheExpand) {
	    infoRec->FillSpansCacheExpand = XAAFillSpansCacheExpand;
	    infoRec->FillSpansCacheExpandFlags =
				infoRec->FillCacheExpandSpansFlags;
	}
    }


    if(infoRec->TEGlyphRenderer &&
	!(infoRec->TEGlyphRendererFlags & NO_TRANSPARENCY)) {

	if(!infoRec->PolyText8TE) {
	    infoRec->PolyText8TE = XAAPolyText8TEColorExpansion;
	    infoRec->PolyText8TEFlags = infoRec->TEGlyphRendererFlags;
	}

	if(!infoRec->PolyText16TE) {
	    infoRec->PolyText16TE = XAAPolyText16TEColorExpansion;
	    infoRec->PolyText16TEFlags = infoRec->TEGlyphRendererFlags;
	}

	if(!infoRec->PolyGlyphBltTE) {
	    infoRec->PolyGlyphBltTE = XAAPolyGlyphBltTEColorExpansion;
	    infoRec->PolyGlyphBltTEFlags = infoRec->TEGlyphRendererFlags;
	}

    }

    if(infoRec->TEGlyphRenderer &&
	!(infoRec->TEGlyphRendererFlags & TRANSPARENCY_ONLY)) {

	if(!infoRec->ImageText8TE) {
	    infoRec->ImageText8TE = XAAImageText8TEColorExpansion;
	    infoRec->ImageText8TEFlags = infoRec->TEGlyphRendererFlags;
	}

	if(!infoRec->ImageText16TE) {
	    infoRec->ImageText16TE = XAAImageText16TEColorExpansion;
	    infoRec->ImageText16TEFlags = infoRec->TEGlyphRendererFlags;
	}

	if(!infoRec->ImageGlyphBltTE) {
	    infoRec->ImageGlyphBltTE = XAAImageGlyphBltTEColorExpansion;
	    infoRec->ImageGlyphBltTEFlags = infoRec->TEGlyphRendererFlags;
	}

    }

    if(infoRec->NonTEGlyphRenderer &&
	!(infoRec->NonTEGlyphRendererFlags & NO_TRANSPARENCY)) {

	if(!infoRec->PolyText8NonTE) {
	    infoRec->PolyText8NonTE = XAAPolyText8NonTEColorExpansion;
	    infoRec->PolyText8NonTEFlags = infoRec->NonTEGlyphRendererFlags;
	}

	if(!infoRec->PolyText16NonTE) {
	    infoRec->PolyText16NonTE = XAAPolyText16NonTEColorExpansion;
	    infoRec->PolyText16NonTEFlags = infoRec->NonTEGlyphRendererFlags;
	}
	if(!infoRec->PolyGlyphBltNonTE) {
	    infoRec->PolyGlyphBltNonTE = XAAPolyGlyphBltNonTEColorExpansion;
	    infoRec->PolyGlyphBltNonTEFlags = infoRec->NonTEGlyphRendererFlags;
	}

    }


    if(infoRec->NonTEGlyphRenderer &&
	!(infoRec->NonTEGlyphRendererFlags & TRANSPARENCY_ONLY) &&
	!(infoRec->NonTEGlyphRendererFlags & NO_TRANSPARENCY)) {

	if(!infoRec->ImageText8NonTE) {
	    infoRec->ImageText8NonTE = XAAImageText8NonTEColorExpansion;
	    infoRec->ImageText8NonTEFlags = infoRec->NonTEGlyphRendererFlags;
	}

	if(!infoRec->ImageText16NonTE) {
	    infoRec->ImageText16NonTE = XAAImageText16NonTEColorExpansion;
	    infoRec->ImageText16NonTEFlags = infoRec->NonTEGlyphRendererFlags;
	}

	if(!infoRec->ImageGlyphBltNonTE) {
	    infoRec->ImageGlyphBltNonTE = XAAImageGlyphBltNonTEColorExpansion;
	    infoRec->ImageGlyphBltNonTEFlags = infoRec->NonTEGlyphRendererFlags;
	}

    }

    if(!infoRec->PolyRectangleThinSolid && HaveSolidHorVertLine) {
	infoRec->PolyRectangleThinSolid = XAAPolyRectangleThinSolid;
	infoRec->PolyRectangleThinSolidFlags = infoRec->SolidLineFlags;
    }

    if(!infoRec->FillPolygonSolid && HaveSolidFillRect) {
	infoRec->FillPolygonSolid = XAAFillPolygonSolid;
	infoRec->FillPolygonSolidFlags = infoRec->SolidFillFlags;
    }

    if(!infoRec->FillPolygonMono8x8Pattern && HaveMono8x8PatternFillRect) {
	infoRec->FillPolygonMono8x8Pattern = XAAFillPolygonMono8x8Pattern;
	infoRec->FillPolygonMono8x8PatternFlags = 
				infoRec->Mono8x8PatternFillFlags;
    }

    if(!infoRec->FillPolygonCacheExpand && HaveScreenToScreenColorExpandCopy) {
	infoRec->FillPolygonCacheExpand = XAAFillPolygonCacheExpand;
	infoRec->FillPolygonCacheExpandFlags = 
				infoRec->ScreenToScreenColorExpandCopyFlags;
    }

    if(!infoRec->FillPolygonCacheBlt && HaveScreenToScreenCopy) {
	infoRec->FillPolygonCacheBlt = XAAFillPolygonCacheBlt;
	infoRec->FillPolygonCacheBltFlags = 
				infoRec->ScreenToScreenCopyFlags;
    }


    if(!infoRec->PolyFillArcSolid && HaveSolidFillRect) {
	infoRec->PolyFillArcSolid = XAAPolyFillArcSolid;
	infoRec->PolyFillArcSolidFlags = infoRec->SolidFillFlags;
    }

    if(!infoRec->PolylinesWideSolid && HaveSolidFillRect) {
	infoRec->PolylinesWideSolid = XAAPolylinesWideSolid;
	infoRec->PolylinesWideSolidFlags = 
			infoRec->SolidFillFlags | GXCOPY_ONLY;
    }

    /************  Validation Functions **************/

    if(!infoRec->ValidateCopyArea && infoRec->CopyArea) {
	infoRec->CopyAreaMask = GCWhenForced;
	if(infoRec->CopyAreaFlags & GXCOPY_ONLY)
	    infoRec->CopyAreaMask |= GCFunction;
	if(infoRec->CopyAreaFlags & NO_PLANEMASK)
	    infoRec->CopyAreaMask |= GCPlaneMask;
	infoRec->ValidateCopyArea = XAAValidateCopyArea;
    }

    if(!infoRec->ValidateCopyPlane && infoRec->CopyPlane) {
	infoRec->CopyPlaneMask = GCWhenForced;
	if(infoRec->CopyPlaneFlags & GXCOPY_ONLY)
	    infoRec->CopyPlaneMask |= GCFunction;
	if(infoRec->CopyPlaneFlags & NO_PLANEMASK)
	    infoRec->CopyPlaneMask |= GCPlaneMask;
	if(infoRec->CopyPlaneFlags & RGB_EQUAL)
	    infoRec->CopyPlaneMask |= GCForeground | GCBackground;
	infoRec->ValidateCopyPlane = XAAValidateCopyPlane;
    }

    if(!infoRec->ValidatePushPixels && infoRec->PushPixelsSolid) {
	infoRec->PushPixelsMask = GCFillStyle;
	if((infoRec->PushPixelsFlags & GXCOPY_ONLY) ||
		(infoRec->PushPixelsFlags & TRANSPARENCY_GXCOPY_ONLY))
	    infoRec->PushPixelsMask |= GCFunction;
	if(infoRec->PushPixelsFlags & NO_PLANEMASK)
	    infoRec->PushPixelsMask |= GCPlaneMask;
	if(infoRec->PushPixelsFlags & RGB_EQUAL)
	    infoRec->PushPixelsMask |= GCForeground;
	infoRec->ValidatePushPixels = XAAValidatePushPixels;
    } 

    /* By default XAA assumes the FillSpans, PolyFillRects, FillPolygon
	and PolyFillArcs have the same restrictions.  If you supply GC 
	level replacements for any of these and alter this relationship 
	you may need to supply replacement validation routines */

    if(!infoRec->ValidateFillSpans && 
	(infoRec->FillSpansSolid || infoRec->FillSpansMono8x8Pattern ||
	infoRec->FillSpansColor8x8Pattern || infoRec->FillSpansColorExpand ||
	infoRec->FillSpansCacheBlt)) {

        int compositeFlags = 	infoRec->FillSpansSolidFlags |
				infoRec->FillSpansMono8x8PatternFlags |
				infoRec->FillSpansColor8x8PatternFlags |
				infoRec->FillSpansColorExpandFlags |
				infoRec->FillSpansCacheBltFlags;

	infoRec->FillSpansMask = GCFillStyle | GCTile | GCStipple;

	if(compositeFlags & GXCOPY_ONLY)
	    infoRec->FillSpansMask |= GCFunction;
	if(compositeFlags & NO_PLANEMASK)
	    infoRec->FillSpansMask |= GCPlaneMask;
	if(compositeFlags & RGB_EQUAL)
	    infoRec->FillSpansMask |= GCForeground;
	infoRec->ValidateFillSpans = XAAValidateFillSpans;
    }

    /* By default XAA only provides Validations for the GlyphBlt
	functions and not the text higher up. This is because the
	Text8/16 and GlyphBlt are linked.  If you break this linkage,
	you may need to have the driver supply its own Validation
	routines */
 
    if(!infoRec->ValidatePolyGlyphBlt && 
	(infoRec->PolyGlyphBltTE || infoRec->PolyGlyphBltNonTE)) {
        int compositeFlags = 	infoRec->PolyGlyphBltTEFlags |
				infoRec->PolyGlyphBltNonTEFlags;
 
	infoRec->PolyGlyphBltMask = GCFillStyle | GCFont;
	if((compositeFlags & GXCOPY_ONLY) ||
		(infoRec->PolyGlyphBltNonTEFlags & TRANSPARENCY_GXCOPY_ONLY))
	    infoRec->PolyGlyphBltMask |= GCFunction;
	if(compositeFlags & NO_PLANEMASK)
	    infoRec->PolyGlyphBltMask |= GCPlaneMask;
	if(compositeFlags & RGB_EQUAL)
	    infoRec->PolyGlyphBltMask |= GCForeground;
	infoRec->ValidatePolyGlyphBlt = XAAValidatePolyGlyphBlt;
    }

    if(!infoRec->ValidateImageGlyphBlt && 
	(infoRec->ImageGlyphBltTE || infoRec->ImageGlyphBltNonTE)) {
        int compositeFlags = 	infoRec->ImageGlyphBltTEFlags |	
				infoRec->ImageGlyphBltNonTEFlags;

	infoRec->ImageGlyphBltMask = GCFont;
	if(compositeFlags & NO_PLANEMASK)
	    infoRec->ImageGlyphBltMask |= GCPlaneMask;
	if(compositeFlags & RGB_EQUAL) {
	    infoRec->ImageGlyphBltMask |= GCForeground;
	    if (!HaveSolidFillRect || (infoRec->FillSolidRectsFlags & RGB_EQUAL))
		infoRec->ImageGlyphBltMask |= GCBackground;
	}
	infoRec->ValidateImageGlyphBlt = XAAValidateImageGlyphBlt;
    }

    /* By default XAA only provides a Validation function for the 
	Polylines and does segments and polylines at the same time */

    if(!infoRec->ValidatePolylines && infoRec->ValidateFillSpans) {
	int compositeFlags = 	infoRec->PolyRectangleThinSolidFlags |
				infoRec->PolylinesWideSolidFlags;

	infoRec->ValidatePolylines = XAAValidatePolylines;
	infoRec->PolylinesMask = 
		infoRec->FillSpansMask | GCLineStyle | GCLineWidth;

	if(compositeFlags & NO_PLANEMASK)
	    infoRec->PolylinesMask |= GCPlaneMask;
	if(compositeFlags & GXCOPY_ONLY)
	    infoRec->PolylinesMask |= GCFunction;
	if(compositeFlags & RGB_EQUAL)
	    infoRec->PolylinesMask |= GCForeground;
    }




    if(!infoRec->StippledFillChooser)
	infoRec->StippledFillChooser = XAAStippledFillChooser;
    
    if(!infoRec->OpaqueStippledFillChooser)
	infoRec->OpaqueStippledFillChooser = XAAOpaqueStippledFillChooser;

    if(!infoRec->TiledFillChooser)
	infoRec->TiledFillChooser = XAATiledFillChooser;


    /**** Setup the pixmap cache ****/


    if(infoRec->WriteBitmapToCache) {}
    else if(infoRec->WriteBitmap && 
	!(infoRec->WriteBitmapFlags & TRANSPARENCY_ONLY))
	infoRec->WriteBitmapToCache = XAAWriteBitmapToCache;
    else {/* until we get framebuffer versions */
	infoRec->PixmapCacheFlags |= DO_NOT_BLIT_STIPPLES;
    }   


    if(infoRec->WritePixmapToCache) {}
    else if(infoRec->WritePixmap && !(infoRec->WritePixmapFlags & NO_GXCOPY))
	infoRec->WritePixmapToCache = XAAWritePixmapToCache;
    else {/* until we get framebuffer versions */
	infoRec->Flags &= ~PIXMAP_CACHE;
    }   


    if(infoRec->WriteMono8x8PatternToCache) {}
    else if(infoRec->PixmapCacheFlags & CACHE_MONO_8x8) {
	if(infoRec->WritePixmapToCache)
	  infoRec->WriteMono8x8PatternToCache = XAAWriteMono8x8PatternToCache;
	else
	   infoRec->PixmapCacheFlags |= ~CACHE_MONO_8x8;
    }

    if(infoRec->WriteColor8x8PatternToCache) {}
    else if(infoRec->PixmapCacheFlags & CACHE_COLOR_8x8) {
	if(infoRec->WritePixmapToCache && infoRec->WriteBitmapToCache)
	  infoRec->WriteColor8x8PatternToCache = XAAWriteColor8x8PatternToCache;
	else
	   infoRec->PixmapCacheFlags |= ~CACHE_COLOR_8x8;
    }


    if(!infoRec->CacheTile && infoRec->WritePixmapToCache)
	infoRec->CacheTile = XAACacheTile;
    if(!infoRec->CacheMonoStipple && infoRec->WritePixmapToCache)
	infoRec->CacheMonoStipple = XAACacheMonoStipple;
    if(!infoRec->CacheStipple && infoRec->WriteBitmapToCache)
	infoRec->CacheStipple = XAACacheStipple;
    if(!infoRec->CacheMono8x8Pattern && infoRec->WriteMono8x8PatternToCache)
	infoRec->CacheMono8x8Pattern = XAACacheMono8x8Pattern;
    if(!infoRec->CacheColor8x8Pattern && infoRec->WriteColor8x8PatternToCache)
	infoRec->CacheColor8x8Pattern = XAACacheColor8x8Pattern;

    if((infoRec->Flags & PIXMAP_CACHE) && !infoRec->InitPixmapCache) {
	infoRec->InitPixmapCache = XAAInitPixmapCache;
	infoRec->ClosePixmapCache = XAAClosePixmapCache;
    }
    
    return TRUE;
}
