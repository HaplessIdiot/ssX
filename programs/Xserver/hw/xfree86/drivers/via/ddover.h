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
/* $XFree86$ */
/*#define   XV_DEBUG	  1*/	  /* write log msg to /var/log/XFree86.0.log */

#ifdef XV_DEBUG
  #define DBG_DD(x) (x)
#else
  #define DBG_DD(x)
#endif

#ifndef __DDOVER
#define __DDOVER


typedef struct _YCBCRREC  {
  CARD32  dwY ;
  CARD32  dwCB;
  CARD32  dwCR;
} YCBCRREC;

void DDOver_GetV1Format(CARD32 dwVideoFlag,LPVIDEOFORMAT lpVF, CARD32 * lpdwVidCtl,CARD32 * lpdwHQVCtl );
CARD32 DDOverHQV_GetFormat(LPVIDEOFORMAT lpVF, CARD32 dwVidCtl,CARD32 * lpdwHQVCtl );
CARD32 DDOver_GetSrcStartAddress (CARD32 dwVideoFlag,BOXPARAM SrcBox,BOXPARAM DestBox, CARD32 dwSrcPitch,LPVIDEOFORMAT lpVF,CARD32 * lpHQVoffset );
YCBCRREC DDOVer_GetYCbCrStartAddress(CARD32 dwVideoFlag,CARD32 dwStartAddr, CARD32 dwOffset,CARD32 dwUVoffset,CARD32 dwSrcPitch/*lpGbl->lPitch*/,CARD32 dwSrcHeight/*lpGbl->wHeight*/);
CARD32 DDOVER_HQVCalcZoomWidth(CARD32 dwVideoFlag, CARD32 srcWidth , CARD32 dstWidth,
			   CARD32 * lpzoomCtl, CARD32 * lpminiCtl, CARD32 * lpHQVfilterCtl, CARD32 * lpHQVminiCtl,CARD32 * lpHQVzoomflag);
CARD32 DDOVER_HQVCalcZoomHeight (CARD32 srcHeight,CARD32 dstHeight,
			     CARD32 * lpzoomCtl, CARD32 * lpminiCtl, CARD32 * lpHQVfilterCtl, CARD32 * lpHQVminiCtl,CARD32 * lpHQVzoomflag);
CARD32 DDOver_GetFetch(CARD32 dwVideoFlag,LPVIDEOFORMAT lpVF,CARD32 dwSrcWidth,CARD32 dwDstWidth,CARD32 dwOriSrcWidth,CARD32 * lpHQVsrcFetch);
void DDOver_GetDisplayCount(CARD32 dwVideoFlag,LPVIDEOFORMAT lpVF,CARD32 dwSrcWidth,CARD32 * lpDisplayCountW);
#endif /*End of	 __DDOVER*/
