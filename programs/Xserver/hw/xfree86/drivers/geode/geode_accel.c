/* $XFree86$ */
/*
 * $Workfile: geode_accel.c $
 * $Revision$
 *
 * File Contents: This file is consists of main Xfree
 *                acceleration supported routines like solid fill used
 *                here.
 * Project:       Geode Xfree Frame buffer device driver.
 *
 *     
 */

/* 
 * NSC_LIC_ALTERNATIVE_PREAMBLE
 *
 * Revision 1.0
 *
 * National Semiconductor Alternative GPL-BSD License
 *
 * National Semiconductor Corporation licenses this software 
 * ("Software"):
 *
 * Geode Xfree frame buffer driver
 *
 * under one of the two following licenses, depending on how the 
 * Software is received by the Licensee.
 * 
 * If this Software is received as part of the Linux Framebuffer or
 * other GPL licensed software, then the GPL license designated 
 * NSC_LIC_GPL applies to this Software; in all other circumstances 
 * then the BSD-style license designated NSC_LIC_BSD shall apply.
 *
 * END_NSC_LIC_ALTERNATIVE_PREAMBLE */

/* NSC_LIC_BSD
 *
 * National Semiconductor Corporation Open Source License for 
 *
 * Geode Xfree frame buffer driver
 *
 * (BSD License with Export Notice)
 *
 * Copyright (c) 1999-2001
 * National Semiconductor Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 *   * Redistributions of source code must retain the above copyright 
 *     notice, this list of conditions and the following disclaimer. 
 *
 *   * Redistributions in binary form must reproduce the above 
 *     copyright notice, this list of conditions and the following 
 *     disclaimer in the documentation and/or other materials provided 
 *     with the distribution. 
 *
 *   * Neither the name of the National Semiconductor Corporation nor 
 *     the names of its contributors may be used to endorse or promote 
 *     products derived from this software without specific prior 
 *     written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * NATIONAL SEMICONDUCTOR CORPORATION OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE,
 * INTELLECTUAL PROPERTY INFRINGEMENT, OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * EXPORT LAWS: THIS LICENSE ADDS NO RESTRICTIONS TO THE EXPORT LAWS OF 
 * YOUR JURISDICTION. It is licensee's responsibility to comply with 
 * any export regulations applicable in licensee's jurisdiction. Under 
 * CURRENT (2001) U.S. export regulations this software 
 * is eligible for export from the U.S. and can be downloaded by or 
 * otherwise exported or reexported worldwide EXCEPT to U.S. embargoed 
 * destinations which include Cuba, Iraq, Libya, North Korea, Iran, 
 * Syria, Sudan, Afghanistan and any other country to which the U.S. 
 * has embargoed goods and services. 
 *
 * END_NSC_LIC_BSD */

/* NSC_LIC_GPL
 *
 * National Semiconductor Corporation Gnu General Public License for 
 *
 * Geode Xfree frame buffer driver
 *
 * (GPL License with Export Notice)
 *
 * Copyright (c) 1999-2001
 * National Semiconductor Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted under the terms of the GNU General 
 * Public License as published by the Free Software Foundation; either 
 * version 2 of the License, or (at your option) any later version  
 *
 * In addition to the terms of the GNU General Public License, neither 
 * the name of the National Semiconductor Corporation nor the names of 
 * its contributors may be used to endorse or promote products derived 
 * from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * NATIONAL SEMICONDUCTOR CORPORATION OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE, 
 * INTELLECTUAL PROPERTY INFRINGEMENT, OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE. See the GNU General Public License for more details. 
 *
 * EXPORT LAWS: THIS LICENSE ADDS NO RESTRICTIONS TO THE EXPORT LAWS OF 
 * YOUR JURISDICTION. It is licensee's responsibility to comply with 
 * any export regulations applicable in licensee's jurisdiction. Under 
 * CURRENT (2001) U.S. export regulations this software 
 * is eligible for export from the U.S. and can be downloaded by or 
 * otherwise exported or reexported worldwide EXCEPT to U.S. embargoed 
 * destinations which include Cuba, Iraq, Libya, North Korea, Iran, 
 * Syria, Sudan, Afghanistan and any other country to which the U.S. 
 * has embargoed goods and services. 
 *
 * You should have received a copy of the GNU General Public License 
 * along with this file; if not, write to the Free Software Foundation, 
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 *
 * END_NSC_LIC_GPL */

/*
 * Fixes by
 * Alan Hourihane <alanh@fairlite.demon.co.uk>
 */

/* Xfree86 header files */
#include "vgaHW.h"
#include "xf86.h"
#include "xf86_ansic.h"
#include "xaalocal.h"
#include "xf86fbman.h"
#include "miline.h"
#include "xf86_libc.h"
#include "xaarop.h"
#include "geode.h"

/* The following ROPs only use pattern and destination data.
 * They are used when the planemask specifies all planes (no mask).
 */

static const int windowsROPpat[16] = {
   0x00, 0xA0, 0x50, 0xF0, 0x0A, 0xAA, 0x5A, 0xFA,
   0x05, 0xA5, 0x55, 0xF5, 0x0F, 0xAF, 0x5F, 0xFF
};

/* The following ROPs use source data to specify a planemask.
 * If the planemask (src) is one, then the result is the appropriate
 * combination of pattern and destination data.  If the planemask (src)
 * is zero, then the result is always just destination data.
 */

static const int windowsROPsrcMask[16] = {
   0x22, 0xA2, 0x62, 0xE2, 0x2A, 0xAA, 0x6A, 0xEA,
   0x26, 0xA6, 0x66, 0xE6, 0x2E, 0xAE, 0x6E, 0xEE
};

/* The following ROPs use pattern data to specify a planemask.
 * If the planemask (pat) is one, then the result is the appropriate
 * combination of source and destination data.  If the planemask (pat)
 * is zero, then the result is always just destination data.
 */

static const int windowsROPpatMask[16] = {
   0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
   0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA
};

/* STATIC VARIABLES FOR THIS FILE
 * Used to maintain state between setup and rendering calls.
 */

static int GeodeTransparent;
static int GeodeTransColor;
static int Geodedstx;
static int Geodedsty;
static int Geodesrcx;
static int Geodesrcy;
static int Geodewidth;
static int Geodeheight;
static int Geodebpp;
static int GeodeCounter;

/* DECLARATION OF ROUTINES THAT ARE HOOKED */

Bool GeodeAccelInit(ScreenPtr pScreen);
void GeodeAccelSync(ScrnInfoPtr pScreenInfo);
void GeodeSetupForFillRectSolid(ScrnInfoPtr pScreenInfo, int color, int rop,
				unsigned int planemask);
void GeodeSubsequentFillRectSolid(ScrnInfoPtr pScreenInfo, int x, int y,
				  int w, int h);
void GeodeSetupFor8x8PatternMonoExpand(ScrnInfoPtr pScreenInfo,
				       int patternx, int patterny, int fg,
				       int bg, int rop,
				       unsigned int planemask);
void GeodeSubsequent8x8PatternMonoExpand(ScrnInfoPtr pScreenInfo,
					 int patternx, int patterny, int x,
					 int y, int w, int h);
void GeodeSetupForScreenToScreenCopy(ScrnInfoPtr pScreenInfo, int xdir,
				     int ydir, int rop,
				     unsigned int planemask,
				     int transparency_color);
void GeodeSubsequentScreenToScreenCopy(ScrnInfoPtr pScreenInfo, int x1,
				       int y1, int x2, int y2, int w, int h);
void GeodeSetupForSolidLine(ScrnInfoPtr pScreenInfo, int color, int rop,
			    unsigned int planemask);
void GeodeSetupForDashedLine(ScrnInfoPtr pScrn, int fg, int bg, int rop,
			     unsigned int planemask, int length,
			     unsigned char *pattern);
void GeodeSubsequentBresenhamLine(ScrnInfoPtr pScreenInfo, int x1, int y1,
				  int absmaj, int absmin, int err, int len,
				  int octant);
void GeodeSubsequentHorVertLine(ScrnInfoPtr pScreenInfo, int x, int y,
				int len, int dir);

void GeodeSetupForScanlineImageWrite(ScrnInfoPtr pScreenInfo,
				     int rop, unsigned int planemask,
				     int transparency_color, int bpp,
				     int depth);

void GeodeSubsequentScanlineImageWriteRect(ScrnInfoPtr pScreenInfo,
					   int x, int y, int w, int h,
					   int skipleft);

void GeodeSubsequentImageWriteScanline(ScrnInfoPtr pScreenInfo, int bufno);

#if 0
void GeodeFillCacheBltRects(ScrnInfoPtr pScrn,
			    int rop, unsigned int planemask,
			    int nBox, BoxPtr pBox, int xorg, int yorg,
			    XAACacheInfoPtr pCache);
#endif

static XAAInfoRecPtr localRecPtr;

/*----------------------------------------------------------------------------
 * GeodeAccelInit.
 *
 * Description	:This function sets up the supported acceleration routines and
 *                 appropriate flags.
 *
 * Parameters:
 *      pScreen	:Screeen pointer structure.
 *
 * Returns		:TRUE on success and FALSE on Failure
 *
 * Comments		:This function is called in GeodeScreenInit in
                     geode_driver.c to set  * the acceleration.
*----------------------------------------------------------------------------
*/
Bool
GeodeAccelInit(ScreenPtr pScreen)
{
   GeodePtr pGeode;
   ScrnInfoPtr pScreenInfo;
   BoxRec AvailBox;
   RegionRec OffscreenRegion;
   unsigned long offset;

   pScreenInfo = xf86Screens[pScreen->myNum];
   pGeode = GEODEPTR(pScreenInfo);

   /* Getting the pointer for acceleration Inforecord */
   pGeode->AccelInfoRec = localRecPtr = XAACreateInfoRec();

   /* SET ACCELERATION FLAGS */
   localRecPtr->Flags = PIXMAP_CACHE | OFFSCREEN_PIXMAPS | LINEAR_FRAMEBUFFER;

   /* HOOK SYNCRONIZATION ROUTINE */
   localRecPtr->Sync = GeodeAccelSync;

   /* HOOK FILLED RECTANGLES */
   localRecPtr->SetupForSolidFill = GeodeSetupForFillRectSolid;
   localRecPtr->SubsequentSolidFillRect = GeodeSubsequentFillRectSolid;
   localRecPtr->SolidFillFlags = 0;

   /* HOOK 8x8 COLOR EXPAND PATTERNS */
   localRecPtr->SetupForMono8x8PatternFill =
	 GeodeSetupFor8x8PatternMonoExpand;
   localRecPtr->SubsequentMono8x8PatternFillRect =
	 GeodeSubsequent8x8PatternMonoExpand;
   localRecPtr->Mono8x8PatternFillFlags = BIT_ORDER_IN_BYTE_MSBFIRST |
	 HARDWARE_PATTERN_PROGRAMMED_BITS | HARDWARE_PATTERN_SCREEN_ORIGIN;

   /* HOOK SCREEN TO SCREEN COPIES
    * * Set flag to only allow copy if transparency is enabled.
    */
   localRecPtr->SetupForScreenToScreenCopy = GeodeSetupForScreenToScreenCopy;
   localRecPtr->SubsequentScreenToScreenCopy =
	 GeodeSubsequentScreenToScreenCopy;

   localRecPtr->ScreenToScreenCopyFlags = GXCOPY_ONLY | NO_TRANSPARENCY;

   /* HOOK BRESENHAM SOLID LINES */
   /* Do not hook unless flag can be set preventing use of planemask. */
   localRecPtr->SolidLineFlags = NO_PLANEMASK;
   localRecPtr->SetupForSolidLine = GeodeSetupForSolidLine;
   localRecPtr->SubsequentSolidBresenhamLine = GeodeSubsequentBresenhamLine;
   localRecPtr->SubsequentSolidHorVertLine = GeodeSubsequentHorVertLine;
   localRecPtr->SolidBresenhamLineErrorTermBits = 15;

#if 0
   /* HOOK BRESENHAM DASHED LINES */
   localRecPtr->DashedLineFlags = NO_PLANEMASK;
   localRecPtr->DashPatternMaxLength = 64;
   localRecPtr->SetupForDashedLine = GeodeSetupForDashedLine;
   localRecPtr->SubsequentDashedBresenhamLine = GeodeSubsequentBresenhamLine;
   localRecPtr->DashedBresenhamLineErrorTermBits = 15;
#endif

   /*
    * ImageWrite.
    *
    * SInce this uses off-screen scanline buffers, it is only of use when
    * complex ROPs are used. But since the current XAA pixmap cache code
    * only works when an ImageWrite is provided, the NO_GXCOPY flag is
    * temporarily disabled.
    */
   if (pGeode->AccelImageWriteBufferOffsets) {
      localRecPtr->ScanlineImageWriteFlags = NO_GXCOPY | NO_TRANSPARENCY;
      localRecPtr->NumScanlineImageWriteBuffers = pGeode->NoOfImgBuffers;
      localRecPtr->SetupForScanlineImageWrite =
	    GeodeSetupForScanlineImageWrite;
      localRecPtr->SubsequentScanlineImageWriteRect =
	    GeodeSubsequentScanlineImageWriteRect;
      localRecPtr->SubsequentImageWriteScanline =
	    GeodeSubsequentImageWriteScanline;

      localRecPtr->ScanlineImageWriteBuffers =
	    pGeode->AccelImageWriteBufferOffsets;

      offset = (unsigned long)pGeode->AccelImageWriteBufferOffsets[0] -
	    (unsigned long)pGeode->FBBase;
      Geodesrcy = offset / pGeode->Pitch;

      Geodesrcx = offset & (pGeode->Pitch - 1);
      Geodesrcx >>= (Geodebpp >> 3) - 1;
   }

   /* CACHE COPY FUNCTION
    * *
    * *    localRecPtr->FillCacheBltRects = GeodeFillCacheBltRects;
    * *    localRecPtr->FillCacheBltRectsFlags = NO_TRANSPARENCY;
    */

   /* SET UP GRAPHICS MEMORY AVAILABLE FOR PIXMAP CACHE */
   AvailBox.x1 = 0;
   AvailBox.x2 = pScreenInfo->displayWidth;
   AvailBox.y1 = pGeode->OffscreenStartOffset / pGeode->Pitch;
   AvailBox.y2 = AvailBox.y1 +
	 ((pGeode->OffscreenSize / pGeode->Pitch /
	   (pScreenInfo->bitsPerPixel >> 3)));

   REGION_INIT(pScreen, &OffscreenRegion, &AvailBox, 2);
   if (!xf86InitFBManagerRegion(pScreen, &OffscreenRegion)) {
      xf86DrvMsg(pScreenInfo->scrnIndex, X_ERROR,
		 "Region (%d,%d,%d,%d) at 0x%08lX not provided for pixmap caching.\n",
		 OffscreenRegion.extents.x1,
		 OffscreenRegion.extents.y1,
		 OffscreenRegion.extents.x2,
		 OffscreenRegion.extents.y2, OffscreenRegion.data);
      xf86DrvMsg(pScreenInfo->scrnIndex, X_ERROR,
		 "Couln't initialize the offscreen memory manager.\n");
   } else {
      xf86DrvMsg(pScreenInfo->scrnIndex, X_PROBED,
		 "Region (%d,%d,%d,%d) at 0x%08lX provided for pixmap caching.\n",
		 OffscreenRegion.extents.x1,
		 OffscreenRegion.extents.y1,
		 OffscreenRegion.extents.x2,
		 OffscreenRegion.extents.y2, OffscreenRegion.data);
   }
   REGION_UNINIT(pScreen, &OffscreenRegion);

   /* TEXT IS NOT HOOKED */
   /* Actually faster to not accelerate it.  To accelerate, the data */
   /* is copied from the font data to a buffer by the OS, then the */
   /* driver copies it from the buffer to the BLT buffer, then the */
   /* rendering engine reads it from the BLT buffer.  May be worthwhile */
   /* to hook text later with the second generation graphics unit that */
   /* can take the data directly. */

   return (XAAInit(pScreen, localRecPtr));
}

/*----------------------------------------------------------------------------
 * GeodeAccelSync.
 *
 * Description	:This function is called to syncronize with the graphics
 *               engine and it waits the graphic engine is idle.This is
 *               required before allowing   direct access to the
 *               framebuffer.
 * Parameters.
 *   pScreenInfo:Screeen info pointer structure.
 *
 * Returns		:none
 *
 * Comments		:This function is called on geode_video routines.
*----------------------------------------------------------------------------
*/
void
GeodeAccelSync(ScrnInfoPtr pScreenInfo)
{
   GFX(wait_until_idle());
}

/*----------------------------------------------------------------------------
 * GeodeSetupForFillRectSolid.
 *
 * Description	:This routine is called to setup the solid pattern
 *               color for   future  rectangular fills or vectors.
 *
 * Parameters.
 * pScreenInfo
 *		Ptr		:Screen handler pointer having screen information.
 *    color     :Specifies the color to be filled up in defined area.
 *    rop       :Specifies the raster operation value.
 *   planemask	:Specifies the masking value based rop srcdata.
 *
 * Returns		:none
 *
 * Comments		:none
 *
*----------------------------------------------------------------------------
*/
void
GeodeSetupForFillRectSolid(ScrnInfoPtr pScreenInfo,
			   int color, int rop, unsigned int planemask)
{
   GFX(set_solid_pattern((unsigned long)color));

   /* CHECK IF PLANEMASK IS NOT USED (ALL PLANES ENABLED) */
   if (planemask == (unsigned int)-1) {
      /* USE NORMAL PATTERN ROPs IF ALL PLANES ARE ENABLED */
      GFX(set_raster_operation(windowsROPpat[rop & 0x0F]));
   } else {
      /* SELECT ROP THAT USES SOURCE DATA FOR PLANEMASK */
      GFX(set_solid_source((unsigned long)planemask));
      GFX(set_raster_operation(windowsROPsrcMask[rop & 0x0F]));
   }
}

 /*----------------------------------------------------------------------------
 * GeodeSubsequentFillRectSolid.
 *
 * Description	:This routine is used to fill the rectangle of previously
 *               specified  solid pattern.
 *
 * Parameters.
 *  pScreenInfo :Screen handler pointer having screen information.
 *     x and y	:Specifies the x and y co-ordinatesarea.
 *      w and h	:Specifies width and height respectively.
 *
 * Returns		:none
 *
 * Comments		:desired pattern can be set before this function by
 *               gfx_set_solid_pattern.
 * Sample application uses:
 *   - Window backgrounds. 
 *   - x11perf: rectangle tests (-rect500).
 *   - x11perf: fill trapezoid tests (-trap100).
 *   - x11perf: horizontal line segments (-hseg500).
 *----------------------------------------------------------------------------
*/
void
GeodeSubsequentFillRectSolid(ScrnInfoPtr pScreenInfo, int x, int y, int w,
			     int h)
{
   /* SIMPLY PASS THE PARAMETERS TO THE DURANGO ROUTINE */
   GeodePtr pGeode;

   pGeode = GEODEPTR(pScreenInfo);

   if (pGeode->TV_Overscan_On) {
      x += pGeode->TVOx;
      y += pGeode->TVOy;
   }

   GFX(pattern_fill((unsigned short)x, (unsigned short)y,
		    (unsigned short)w, (unsigned short)h));
}

/*----------------------------------------------------------------------------
 * GeodeSetupFor8x8PatternMonoExpand
 *
 * Description	:This routine is called to fill the monochrome pattern of
 *                 8x8.
 * Parameters.
 * 	pScreenInfo :Screen handler pointer having screen information.
 *      patternx:This is set from on rop data.
 *      patterny:This is set based on rop data.
 *       fg		:Specifies the foreground color
 *       bg     :Specifies the background color
 *	planemask	:Specifies the value of masking from rop data

 * Returns		:none.
 *
 * Comments     :none.
 *
*----------------------------------------------------------------------------
*/
void
GeodeSetupFor8x8PatternMonoExpand(ScrnInfoPtr pScreenInfo,
				  int patternx, int patterny, int fg,
				  int bg, int rop, unsigned int planemask)
{
   int trans = (bg == -1);

   /* LOAD PATTERN COLORS AND DATA */
   GFX(set_mono_pattern((unsigned long)bg, (unsigned long)fg,
			(unsigned long)patternx, (unsigned long)patterny,
			(unsigned char)trans));

   /* CHECK IF PLANEMASK IS NOT USED (ALL PLANES ENABLED) */
   if (planemask == (unsigned int)-1) {
      /* USE NORMAL PATTERN ROPs IF ALL PLANES ARE ENABLED */
      GFX(set_raster_operation(windowsROPpat[rop & 0x0F]));
   } else {
      /* SELECT ROP THAT USES SOURCE DATA FOR PLANEMASK */
      GFX(set_solid_source((unsigned long)planemask));
      GFX(set_raster_operation(windowsROPsrcMask[rop & 0x0F]));
   }
}

/*----------------------------------------------------------------------------
 * GeodeSubsequent8x8PatternMonoExpand
 *
 * Description	:This routine is called to fill  a ractanglethe
 *                 monochrome pattern of previusly loaded pattern.
 *
 * Parameters.
 *	pScreenInfo	:Screen handler pointer having screen information.
 *  patternx	:This is set from on rop data.
 *  patterny	:This is set based on rop data.
 *       fg		:Specifies the foreground color
 *       bg		:Specifies the background color
 *  planemask	:Specifies the value of masking from rop data

 * Returns		:none
 *
 * Comments		:The patterns specified is ignored inside the function
 * Sample application uses:
 *   - Patterned desktops
 *   - x11perf: stippled rectangle tests (-srect500).
 *   - x11perf: opaque stippled rectangle tests (-osrect500).
*----------------------------------------------------------------------------
*/
void
GeodeSubsequent8x8PatternMonoExpand(ScrnInfoPtr pScreenInfo,
				    int patternx, int patterny, int x, int y,
				    int w, int h)
{
   GeodePtr pGeode;

   pGeode = GEODEPTR(pScreenInfo);

   if (pGeode->TV_Overscan_On) {
      x += pGeode->TVOx;
      y += pGeode->TVOy;
   }

   /* SIMPLY PASS THE PARAMETERS TO THE DURANGO ROUTINE */
   /* Ignores specified pattern. */
   GFX(pattern_fill((unsigned short)x, (unsigned short)y,
		    (unsigned short)w, (unsigned short)h));
}

/*----------------------------------------------------------------------------
 * GeodeSetupForScreenToScreenCopy
 *
 * Description	:This function is used to set up the planemask and raster
 *                 for future Bliting functionality.
 *
 * Parameters:
 *	pScreenInfo :Screen handler pointer having screen information.
 *      xdir	:This is set based on rop data.
 *      ydir    :This is set based on rop data.
 *      rop		:sets the raster operation
 *	transparency:tobeadded
 *  planemask	:Specifies the value of masking from rop data

 * Returns		:none
 *
 * Comments		:The patterns specified is ignored inside the function
*----------------------------------------------------------------------------
*/
void
GeodeSetupForScreenToScreenCopy(ScrnInfoPtr pScreenInfo,
				int xdir, int ydir, int rop,
				unsigned int planemask,
				int transparency_color)
{
   GFX(set_solid_pattern((unsigned long)planemask));
   /* SET RASTER OPERATION FOR USING PATTERN AS PLANE MASK */
   GFX(set_raster_operation(windowsROPpatMask[rop & 0x0F]));

   /* SAVE TRANSPARENCY FLAG */
   GeodeTransparent = (transparency_color == -1) ? 0 : 1;
   GeodeTransColor = transparency_color;
}

/*----------------------------------------------------------------------------
 * GeodeSubsquentScreenToScreenCopy
 *
 * Description	:This function is called to perform a screen to screen
 *                 BLT  using the previously  specified planemask,raster
 *                 operation and  * transparency flag
 *
 * Parameters.
 * 	pScreenInfo :Screen handler pointer having screen information.
 *  	x1	 	:x -coordinates of the source window
 *      y1      :y-co-ordinates of the source window
 *      x2		:x -coordinates of the destination window
 *      y2      :y-co-ordinates of the destination window
 *      w	    :Specifies width of the window to be copied
 *      h       :Height of the window to be copied.
 * Returns		:none
 *
 * Comments		:The patterns specified is ignored inside the function
 * Sample application uses (non-transparent):
 *   - Moving windows.
 *   - x11perf: scroll tests (-scroll500).
 *   - x11perf: copy from window to window (-copywinwin500).
 *
 * No application found using transparency.
*----------------------------------------------------------------------------
*/
void
GeodeSubsequentScreenToScreenCopy(ScrnInfoPtr pScreenInfo,
				  int x1, int y1, int x2, int y2, int w,
				  int h)
{
   GeodePtr pGeode;

   pGeode = GEODEPTR(pScreenInfo);

   if (pGeode->TV_Overscan_On) {
      x1 += pGeode->TVOx;
      y1 += pGeode->TVOy;
      x2 += pGeode->TVOx;
      y2 += pGeode->TVOy;
   }

   if (GeodeTransparent) {
      /* CALL ROUTINE FOR TRANSPARENT SCREEN TO SCREEN BLT
       * * Should only be called for the "copy" raster operation.
       */
      GFX(screen_to_screen_xblt((unsigned short)x1, (unsigned short)y1,
				(unsigned short)x2, (unsigned short)y2,
				(unsigned short)w, (unsigned short)h,
				(unsigned short)GeodeTransColor));
   } else {
      /* CALL ROUTINE FOR NORMAL SCREEN TO SCREEN BLT */
      GFX(screen_to_screen_blt((unsigned short)x1, (unsigned short)y1,
			       (unsigned short)x2, (unsigned short)y2,
			       (unsigned short)w, (unsigned short)h));
   }
}

void
GeodeSetupForScanlineImageWrite(ScrnInfoPtr pScreenInfo,
				int rop, unsigned int planemask,
				int transparency_color, int bpp, int depth)
{
   GFX(set_solid_pattern((unsigned long)planemask));
   /* SET RASTER OPERATION FOR USING PATTERN AS PLANE MASK */
   GFX(set_raster_operation(windowsROPpatMask[rop & 0x0F]));

   /* SAVE TRANSPARENCY FLAG */
   GeodeTransparent = (transparency_color == -1) ? 0 : 1;
   GeodeTransColor = transparency_color;
   Geodebpp = bpp;
}

void
GeodeSubsequentScanlineImageWriteRect(ScrnInfoPtr pScreenInfo,
				      int x, int y, int w, int h,
				      int skipleft)
{
   GeodePtr pGeode;

   pGeode = GEODEPTR(pScreenInfo);

   Geodedstx = x;
   Geodedsty = y;

   if (pGeode->TV_Overscan_On) {
      Geodedstx += pGeode->TVOx;
      Geodedsty += pGeode->TVOy;
   }

   Geodewidth = w;
   Geodeheight = h;

   GeodeCounter = 0;
}

void
GeodeSubsequentImageWriteScanline(ScrnInfoPtr pScreenInfo, int bufno)
{

   GeodePtr pGeode;
   int blt_height = 0;
   char blit = FALSE;

   pGeode = GEODEPTR(pScreenInfo);

   GeodeCounter++;

   if ((Geodeheight <= pGeode->NoOfImgBuffers) &&
       (GeodeCounter == Geodeheight)) {
      blit = TRUE;
      blt_height = Geodeheight;
   } else if ((Geodeheight > pGeode->NoOfImgBuffers)
	      && (GeodeCounter == pGeode->NoOfImgBuffers)) {
      blit = TRUE;
      Geodeheight -= pGeode->NoOfImgBuffers;
      blt_height = pGeode->NoOfImgBuffers;
   } else
      return;

   if (blit) {

      blit = FALSE;

      GeodeCounter = 0;

      if (GeodeTransparent) {
	 /* CALL ROUTINE FOR TRANSPARENT SCREEN TO SCREEN BLT
	  * * Should only be called for the "copy" raster operation.
	  */
	 GFX(screen_to_screen_xblt((unsigned short)Geodesrcx,
				   (unsigned short)Geodesrcy,
				   (unsigned short)Geodedstx,
				   (unsigned short)Geodedsty,
				   (unsigned short)Geodewidth,
				   (unsigned short)blt_height,
				   (unsigned short)GeodeTransColor));
      } else {
	 /* CALL ROUTINE FOR NORMAL SCREEN TO SCREEN BLT */
	 GFX(screen_to_screen_blt((unsigned short)Geodesrcx,
				  (unsigned short)Geodesrcy,
				  (unsigned short)Geodedstx,
				  (unsigned short)Geodedsty,
				  (unsigned short)Geodewidth,
				  (unsigned short)blt_height));
      }
      Geodedsty += blt_height;
      GeodeAccelSync(pScreenInfo);
   }

}

/*----------------------------------------------------------------------------
 * GeodeSetupForSolidLine
 *
 * Description	:This function is used setup the solid line color for
 *               future line draws.
 *
 *
 * Parameters.
 * 	pScreenInfo :Screen handler pointer having screen information.
 *      color	:Specifies the color value od line
 *      rop     :Specifies rop values.
 *  Planemask	:Specifies planemask value.
 * Returns		:none
 *
 * Comments		:none
*----------------------------------------------------------------------------
*/
void
GeodeSetupForSolidLine(ScrnInfoPtr pScreenInfo,
		       int color, int rop, unsigned int planemask)
{
   /* LOAD THE SOLID PATTERN COLOR */
   GFX(set_solid_pattern((unsigned long)color));

   /* USE NORMAL PATTERN ROPs IF ALL PLANES ARE ENABLED */
   GFX(set_raster_operation(windowsROPpat[rop & 0x0F]));
}

unsigned short vector_mode_table[] = {
   VM_MAJOR_INC | VM_MINOR_INC | VM_X_MAJOR,
   VM_MAJOR_INC | VM_MINOR_INC | VM_Y_MAJOR,
   VM_MAJOR_INC | VM_Y_MAJOR,
   VM_MINOR_INC | VM_X_MAJOR,
   VM_X_MAJOR,
   VM_Y_MAJOR,
   VM_MINOR_INC | VM_Y_MAJOR,
   VM_MAJOR_INC | VM_X_MAJOR,
};

/*---------------------------------------------------------------------------
 * GeodeSubsequentBresenhamLine
 *
 * Description	:This function is used to render a vector using the
 *                 specified bresenham parameters.
 *
 * Parameters:
 * pScreenInfo 	:Screen handler pointer having screen information.
 *      x1  	:Specifies the starting x position
 *      y1      :Specifies starting y possition
 *      absmaj	:Specfies the Bresenman absolute major.
 *	  absmin	:Specfies the Bresenman absolute minor.
 *	  err       :Specifies the bresenham err term.
 *	  len       :Specifies the length of the vector interms of pixels.
 *	  octant    :not used in this function,may be added for standard
 *                    interface.
 * Returns		:none
 *
 * Comments     :none
 * Sample application uses:
 *   - Window outlines on window move.
 *   - x11perf: line segments (-seg500).
*----------------------------------------------------------------------------
*/
void
GeodeSubsequentBresenhamLine(ScrnInfoPtr pScreenInfo,
			     int x1, int y1, int absmaj, int absmin, int err,
			     int len, int octant)
{
   unsigned short flags;
   int octant_reg;
   int axial, init, diag;
   GeodePtr pGeode;

   pGeode = GEODEPTR(pScreenInfo);

   if (pGeode->TV_Overscan_On) {
      x1 += pGeode->TVOx;
      y1 += pGeode->TVOy;
   }

   /*ErrorF("BresenhamLine(%d, %d, %d, %d, %d, %d, %d)\n",
    * x1, y1, absmaj, absmin, err, len, octant);
    */

   /* DETERMINE YMAJOR AND DIRECTION FLAGS */
   if (octant & YMAJOR) {
      if (!(octant & YDECREASING)) {
	 if (!(octant & XDECREASING))
	    octant_reg = 1;
	 else
	    octant_reg = 2;
      } else {
	 if (!(octant & XDECREASING))
	    octant_reg = 6;
	 else
	    octant_reg = 5;
      }
   } else {
      if (!(octant & YDECREASING)) {
	 if (!(octant & XDECREASING))
	    octant_reg = 0;
	 else
	    octant_reg = 3;
      } else {
	 if (!(octant & XDECREASING))
	    octant_reg = 7;
	 else
	    octant_reg = 4;
      }
   }

   flags = vector_mode_table[octant_reg & 7];

   /* DETERMINE BRESENHAM PARAMETERS */

   axial = ((int)absmin << 1);
   init = axial - (int)absmaj;
   diag = init - (int)absmaj;

   /* ADJUST INITIAL ERROR
    * * Adjust by -1 for certain directions so that the vector 
    * * hits the same pixels when drawn in either direction.
    * * The Gamma value is assumed to account for the initial 
    * * error adjustment for clipped lines.
    */

   init += err;

   /* CALL ROUTINE TO DRAW VECTOR */

   GFX(bresenham_line((unsigned short)x1, (unsigned short)y1,
		      (unsigned short)len, (unsigned short)init,
		      (unsigned short)axial, (unsigned short)diag,
		      (unsigned short)flags);
	 )
}

/*---------------------------------------------------------------------------
 * GeodeSubsequentHorVertLine
 *
 * This routine is called to render a vector using the specified Bresenham
 * parameters.  
 *
 * Sample application uses:
 *   - Window outlines on window move.
 *   - x11perf: line segments (-hseg500).
 *   - x11perf: line segments (-vseg500).
 *---------------------------------------------------------------------------
 */
void
GeodeSubsequentHorVertLine(ScrnInfoPtr pScreenInfo,
			   int x, int y, int len, int dir)
{
   GeodePtr pGeode;

   pGeode = GEODEPTR(pScreenInfo);

   if (pGeode->TV_Overscan_On) {
      x += pGeode->TVOx;
      y += pGeode->TVOy;
   }
   GFX(pattern_fill((unsigned short)x, (unsigned short)y,
		    (unsigned short)((dir == DEGREES_0) ? len : 1),
		    (unsigned short)((dir == DEGREES_0) ? 1 : len)));
}

void
GeodeSetupForDashedLine(ScrnInfoPtr pScrn, int fg, int bg, int rop,
			unsigned int planemask, int length,
			unsigned char *pattern)
{
   int trans = (bg == -1);
   unsigned long *pat = (unsigned long *)pattern;

   /* LOAD PATTERN COLORS AND DATA */

   GFX(set_mono_pattern((unsigned long)bg, (unsigned long)fg,
			(unsigned long)pat, (unsigned long)pat,
			(unsigned char)trans));

   /* CHECK IF PLANEMASK IS NOT USED (ALL PLANES ENABLED) */

   if (planemask == (unsigned int)-1) {
      /* USE NORMAL PATTERN ROPs IF ALL PLANES ARE ENABLED */

      GFX(set_raster_operation(windowsROPpat[rop & 0x0F]));
   } else {
      /* SELECT ROP THAT USES SOURCE DATA FOR PLANEMASK */

      GFX(set_solid_source((unsigned long)planemask));
      GFX(set_raster_operation(windowsROPsrcMask[rop & 0x0F]));
   }
}

#if 0
void
GeodeFillCacheBltRects(ScrnInfoPtr pScrn, int rop, unsigned int planemask,
		       int nBox, BoxPtr pBox, int xorg, int yorg,
		       XAACacheInfoPtr pCache)
{
   XAAInfoRecPtr infoRec = GET_XAAINFORECPTR_FROM_SCRNINFOPTR(pScrn);
   int x, y;
   int phaseY, phaseX, skipleft, height, width, w, blit_w, blit_h, start;

   ErrorF("FillCacheBltRects\n");

   (*infoRec->SetupForScreenToScreenCopy) (pScrn, 1, 1, rop, planemask,
					   pCache->trans_color);

   while (nBox--) {
      y = pBox->y1;
      phaseY = (y - yorg) % pCache->orig_h;
      if (phaseY < 0)
	 phaseY += pCache->orig_h;
      phaseX = (pBox->x1 - xorg) % pCache->orig_w;
      if (phaseX < 0)
	 phaseX += pCache->orig_w;
      height = pBox->y2 - y;
      width = pBox->x2 - pBox->x1;
      start = phaseY ? (pCache->orig_h - phaseY) : 0;

      /* This is optimized for WRAM */

      if ((rop == GXcopy) && (height >= (pCache->orig_h + start))) {
	 w = width;
	 skipleft = phaseX;
	 x = pBox->x1;
	 blit_h = pCache->orig_h;

	 while (1) {
	    blit_w = pCache->w - skipleft;
	    if (blit_w > w)
	       blit_w = w;
	    (*infoRec->SubsequentScreenToScreenCopy) (pScrn,
						      pCache->x + skipleft,
						      pCache->y, x, y + start,
						      blit_w, blit_h);
	    w -= blit_w;
	    if (!w)
	       break;
	    x += blit_w;
	    skipleft = (skipleft + blit_w) % pCache->orig_w;
	 }
	 height -= blit_h;
	 if (start) {
	    (*infoRec->SubsequentScreenToScreenCopy) (pScrn,
						      pBox->x1, y + blit_h,
						      pBox->x1, y, width,
						      start);
	    height -= start;
	    y += start;
	 }
	 start = blit_h;

	 while (height) {
	    if (blit_h > height)
	       blit_h = height;
	    (*infoRec->SubsequentScreenToScreenCopy) (pScrn,
						      pBox->x1, y,
						      pBox->x1, y + start,
						      width, blit_h);
	    height -= blit_h;
	    start += blit_h;
	    blit_h <<= 1;
	 }
      } else {
	 while (1) {
	    w = width;
	    skipleft = phaseX;
	    x = pBox->x1;
	    blit_h = pCache->h - phaseY;
	    if (blit_h > height)
	       blit_h = height;

	    while (1) {
	       blit_w = pCache->w - skipleft;
	       if (blit_w > w)
		  blit_w = w;
	       (*infoRec->SubsequentScreenToScreenCopy) (pScrn,
							 pCache->x + skipleft,
							 pCache->y + phaseY,
							 x, y, blit_w,
							 blit_h);
	       w -= blit_w;
	       if (!w)
		  break;
	       x += blit_w;
	       skipleft = (skipleft + blit_w) % pCache->orig_w;
	    }
	    height -= blit_h;
	    if (!height)
	       break;
	    y += blit_h;
	    phaseY = (phaseY + blit_h) % pCache->orig_h;
	 }
      }
      pBox++;
   }

   SET_SYNC_FLAG(infoRec);
}
#endif
/* END OF FILE */
