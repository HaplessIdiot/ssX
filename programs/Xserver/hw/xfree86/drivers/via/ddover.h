/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
 
#ifndef __DDOVER
#define __DDOVER

/*#define   XV_DEBUG      1*/     /* write log msg to /var/log/XFree86.0.log */

#ifdef XV_DEBUG
# define DBG_DD(x) (x)
#else
# define DBG_DD(x)
#endif

#define PLUS_HEIGHT 1 /*V003*/

typedef struct _YCBCRREC  {
  CARD32  dwY ;
  CARD32  dwCB;
  CARD32  dwCR;
} YCBCRREC;

/*
void DDOver_GetColorKey(LPDDHAL_UPDATEOVERLAYDATA lpUO, LPDDOVERLAYFX lpFX,
                        LPDDRAWI_DDRAWSURFACE_LCL lpDestSurf,
                        LPDDRAWI_DDRAWSURFACE_LCL lpSrcSurf,
                        unsigned long *  lpColorKey,unsigned long *   lpChromaKey,
                        unsigned long *  lpKeyLow  ,unsigned long *   lpKeyHigh,
                        unsigned long *  lpChromaLow  ,unsigned long *   lpChromaHigh);
*/

void viaDDOver_GetV1Format(unsigned long dwVideoFlag,LPDDPIXELFORMAT lpDPF, unsigned long * lpdwVidCtl,unsigned long * lpdwHQVCtl );
void viaDDOver_GetV3Format(unsigned long dwVideoFlag,LPDDPIXELFORMAT lpDPF, unsigned long * lpdwVidCtl,unsigned long * lpdwHQVCtl );
unsigned long viaDDOverHQV_GetFormat(LPDDPIXELFORMAT lpDPF, unsigned long dwVidCtl,unsigned long * lpdwHQVCtl );
unsigned long viaDDOver_GetSrcStartAddress (unsigned long dwVideoFlag,RECTL rSrc,RECTL rDest, unsigned long dwSrcPitch,LPDDPIXELFORMAT lpDPF,unsigned long * lpHQVoffset );
YCBCRREC viaDDOVer_GetYCbCrStartAddress(unsigned long dwVideoFlag,unsigned long dwStartAddr, unsigned long dwOffset,unsigned long dwUVoffset,unsigned long dwSrcPitch/*lpGbl->lPitch*/,unsigned long dwSrcHeight/*lpGbl->wHeight*/);
unsigned long viaDDOVER_HQVCalcZoomWidth(unsigned long dwVideoFlag, unsigned long srcWidth , unsigned long dstWidth,
                           unsigned long * lpzoomCtl, unsigned long * lpminiCtl, unsigned long * lpHQVfilterCtl, unsigned long * lpHQVminiCtl,unsigned long * lpHQVzoomflag);
unsigned long viaDDOVER_HQVCalcZoomHeight (unsigned long srcHeight,unsigned long dstHeight,
                             unsigned long * lpzoomCtl, unsigned long * lpminiCtl, unsigned long * lpHQVfilterCtl, unsigned long * lpHQVminiCtl,unsigned long * lpHQVzoomflag);
unsigned long viaDDOver_GetFetch(unsigned long dwVideoFlag,LPDDPIXELFORMAT lpDPF,unsigned long dwSrcWidth,unsigned long dwDstWidth,unsigned long dwOriSrcWidth,unsigned long * lpHQVsrcFetch);
void viaDDOver_GetDisplayCount(unsigned long dwVideoFlag,LPDDPIXELFORMAT lpDPF,unsigned long dwSrcWidth,unsigned long * lpDisplayCountW);
#endif /*End of  __DDOVER*/
