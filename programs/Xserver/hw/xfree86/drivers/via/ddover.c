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
#include "xf86.h"

#include "via.h"
#include "ddmpeg.h"
#include "via_privIoctl.h" /* for VIAGRAPHICINFO & custom ioctl command */

#include "ddover.h"

/* E X T E R N	 G L O B A L S--------------------------------------------------------------*/

extern OVERLAYRECORD	overlayRecordV1;
extern VIAGRAPHICINFO gVIAGraphicInfo;	   /*2D information*/

/* F U N C T I O N ----------------------------------------------------------*/

void DDOver_GetV1Format(CARD32 dwVideoFlag,LPVIDEOFORMAT lpVF, CARD32 * lpdwVidCtl,CARD32 * lpdwHQVCtl )
{

   if (lpVF->dwFlags & VF_FOURCC)
   {
       *lpdwVidCtl |= V1_COLORSPACE_SIGN;
       switch (lpVF->dwFourCC) {
       case FOURCC_YV12:
	    if (dwVideoFlag&VIDEO_HQV_INUSE)
	    {
		*lpdwVidCtl |= (V1_YUV422 | V1_SWAP_HW_HQV );
		*lpdwHQVCtl |= HQV_SRC_SW|HQV_YUV420|HQV_ENABLE|HQV_SW_FLIP;
	    }
	    else
	    {
		*lpdwVidCtl |= V1_YCbCr420;
	    }
	    break;
       case FOURCC_YUY2:
	    DBG_DD(ErrorF("DDOver_GetV1Format : FOURCC_YUY2\n"));
	    if (dwVideoFlag&VIDEO_HQV_INUSE)
	    {
		*lpdwVidCtl |= (V1_YUV422 | V1_SWAP_HW_HQV );
		*lpdwHQVCtl |= HQV_SRC_SW|HQV_YUV422|HQV_ENABLE|HQV_SW_FLIP;
	    }
	    else
	    {
		*lpdwVidCtl |= V1_YUV422;
	    }
	    break;

       default :
	    DBG_DD(ErrorF("DDOver_GetV1Format : Invalid FOURCC format :(0x%lx)in V1!\n", lpVF->dwFourCC));
	    *lpdwVidCtl |= V1_YUV422;
	    break;
       }
   }
}

CARD32 DDOver_GetSrcStartAddress (CARD32 dwVideoFlag,BOXPARAM SrcBox,BOXPARAM DestBox, CARD32 dwSrcPitch,LPVIDEOFORMAT lpVF,CARD32 * lpHQVoffset )
{
   CARD32 dwOffset=0;
   CARD32 dwHQVsrcWidth=0,dwHQVdstWidth=0;
   CARD32 dwHQVsrcHeight=0,dwHQVdstHeight=0;
   CARD32 dwHQVSrcTopOffset=0,dwHQVSrcLeftOffset=0;

   dwHQVsrcWidth = (CARD32)(SrcBox.x2 - SrcBox.x1);
   dwHQVdstWidth = (CARD32)(DestBox.x2 - DestBox.x1);
   dwHQVsrcHeight = (CARD32)(SrcBox.y2 - SrcBox.y1);
   dwHQVdstHeight = (CARD32)(DestBox.y2 - DestBox.y1);

   if ( (SrcBox.x1 != 0) || (SrcBox.y1 != 0) )
   {

       if (lpVF->dwFlags & VF_FOURCC)
       {
	   switch (lpVF->dwFourCC)
	   {
	    case FOURCC_YUY2:
	    case FOURCC_UYVY:
	    case FOURCC_YVYU:
		DBG_DD(ErrorF("GetSrcStartAddress : FOURCC format :(0x%lx)\n", lpVF->dwFourCC));
		if (dwVideoFlag&VIDEO_HQV_INUSE)
		{
		    dwOffset = (((SrcBox.y1&~3) * (dwSrcPitch)) +
			       ((SrcBox.x1 << 1)&~31));
		    if (dwHQVsrcHeight>dwHQVdstHeight)
		    {
			dwHQVSrcTopOffset = ((SrcBox.y1&~3) * dwHQVdstHeight / dwHQVsrcHeight)* dwSrcPitch;
		    }
		    else
		    {
			dwHQVSrcTopOffset = (SrcBox.y1&~3) * (dwSrcPitch);
		    }

		    if (dwHQVsrcWidth>dwHQVdstWidth)
		    {
			dwHQVSrcLeftOffset = ((SrcBox.x1 << 1)&~31) * dwHQVdstWidth / dwHQVsrcWidth;
		    }
		    else
		    {
			dwHQVSrcLeftOffset = (SrcBox.x1 << 1)&~31 ;
		    }
		    *lpHQVoffset = dwHQVSrcTopOffset+dwHQVSrcLeftOffset;
		}
		else
		{
		    dwOffset = ((SrcBox.y1 * dwSrcPitch) +
			       ((SrcBox.x1 << 1)&~15));
		}
		break;

	    case FOURCC_YV12:
		if (dwVideoFlag&VIDEO_HQV_INUSE)
		{
		    dwOffset = (((SrcBox.y1&~3) * (dwSrcPitch<<1)) +
				((SrcBox.x1 << 1)&~31));
		}
		else
		{
		    dwOffset = ((((SrcBox.y1&~3) * dwSrcPitch) +
				SrcBox.x1)&~31) ;
		    if (SrcBox.y1 >0)
		    {
			overlayRecordV1.dwUVoffset = (((((SrcBox.y1&~3)>>1) * dwSrcPitch) +
				       SrcBox.x1)&~31) >>1;
		    }
		    else
		    {
			overlayRecordV1.dwUVoffset = dwOffset >>1 ;
		    }
		}
		break;

	    default:
		break;
	    }
       }
   }
   else
   {
	overlayRecordV1.dwUVoffset = dwOffset = 0;
   }

   return dwOffset;
}

YCBCRREC DDOVer_GetYCbCrStartAddress(CARD32 dwVideoFlag,CARD32 dwStartAddr, CARD32 dwOffset,CARD32 dwUVoffset,CARD32 dwSrcPitch/*lpGbl->lPitch*/,CARD32 dwSrcHeight/*lpGbl->wHeight*/)
{
   YCBCRREC YCbCr;

   if (dwVideoFlag&VIDEO_HQV_INUSE)
   {
       YCbCr.dwY   =  dwStartAddr;
       YCbCr.dwCB  =  dwStartAddr + dwSrcPitch * dwSrcHeight ;
       YCbCr.dwCR  =  dwStartAddr + dwSrcPitch * dwSrcHeight
			 + dwSrcPitch * (dwSrcHeight >>2);
   }
   else
   {
       YCbCr.dwY   =  dwStartAddr+dwOffset;
       YCbCr.dwCB  =  dwStartAddr + dwSrcPitch * dwSrcHeight
			 + dwUVoffset;
       YCbCr.dwCR  =  dwStartAddr + dwSrcPitch * dwSrcHeight
			 + dwSrcPitch * (dwSrcHeight >>2)
			 + dwUVoffset;
   }
   return YCbCr;
}


CARD32 DDOVER_HQVCalcZoomWidth(CARD32 dwVideoFlag, CARD32 srcWidth , CARD32 dstWidth,
			   CARD32 * lpzoomCtl, CARD32 * lpminiCtl, CARD32 * lpHQVfilterCtl, CARD32 * lpHQVminiCtl,CARD32 * lpHQVzoomflag)
{
    CARD32 dwTmp;

    if (srcWidth == dstWidth)
    {
	*lpHQVfilterCtl |= HQV_H_FILTER_DEFAULT;
    }
    else
    {

	if (srcWidth < dstWidth) {
	    /* zoom in*/
	    *lpzoomCtl = srcWidth*0x0800 / dstWidth;
	    *lpzoomCtl = (((*lpzoomCtl) & 0x7FF) << 16) | V1_X_ZOOM_ENABLE;
	    *lpminiCtl |= ( V1_X_INTERPOLY );  /* set up interpolation*/
	    *lpHQVzoomflag = 1;
	    *lpHQVfilterCtl |= HQV_H_FILTER_DEFAULT ;
	} else if (srcWidth > dstWidth) {
	    /* zoom out*/
	    CARD32 srcWidth1;

	    dwTmp = dstWidth*0x0800*0x400 / srcWidth;
	    dwTmp = dwTmp / 0x400 + ((dwTmp & 0x3ff)?1:0);

	    *lpHQVminiCtl = (dwTmp & 0x7FF)| HQV_H_MINIFY_ENABLE;


	    srcWidth1 = srcWidth >> 1;
	    if (srcWidth1 <= dstWidth) {
		*lpminiCtl |= V1_X_DIV_2+V1_X_INTERPOLY;
		if (dwVideoFlag&VIDEO_1_INUSE)
		{
		    overlayRecordV1.dwFetchAlignment = 3;
		    overlayRecordV1.dwminifyH = 2;
		}
		*lpHQVfilterCtl |= HQV_H_TAP4_121;
	    }
	    else {
		srcWidth1 >>= 1;

		if (srcWidth1 <= dstWidth) {
		    *lpminiCtl |= V1_X_DIV_4+V1_X_INTERPOLY;
		    if (dwVideoFlag&VIDEO_1_INUSE)
		    {
			overlayRecordV1.dwFetchAlignment = 7;
			overlayRecordV1.dwminifyH = 4;
		    }
		    *lpHQVfilterCtl |= HQV_H_TAP4_121;
		}
		else {
		    srcWidth1 >>= 1;

		    if (srcWidth1 <= dstWidth) {
			*lpminiCtl |= V1_X_DIV_8+V1_X_INTERPOLY;
			if (dwVideoFlag&VIDEO_1_INUSE)
			{
			    overlayRecordV1.dwFetchAlignment = 15;
			    overlayRecordV1.dwminifyH = 8;
			}
			*lpHQVfilterCtl |= HQV_H_TAP8_12221;
		    }
		    else {
			srcWidth1 >>= 1;

			if (srcWidth1 <= dstWidth) {
			    *lpminiCtl |= V1_X_DIV_16+V1_X_INTERPOLY;
			    if (dwVideoFlag&VIDEO_1_INUSE)
			    {
				overlayRecordV1.dwFetchAlignment = 15;
				overlayRecordV1.dwminifyH = 16;
			    }
			    *lpHQVfilterCtl |= HQV_H_TAP8_12221;
			}
			else {
			    *lpminiCtl |= V1_X_DIV_16+V1_X_INTERPOLY;
			    if (dwVideoFlag&VIDEO_1_INUSE)
			    {
				overlayRecordV1.dwFetchAlignment = 15;
				overlayRecordV1.dwminifyH = 16;
			    }
			    *lpHQVfilterCtl |= HQV_H_TAP8_12221;
			}
		    }
		}
	    }

	    *lpHQVminiCtl |= HQV_HDEBLOCK_FILTER;

	    if (srcWidth1 < dstWidth) {
		*lpzoomCtl = (srcWidth1-2)*0x0800 / dstWidth;
		*lpzoomCtl = ((*lpzoomCtl & 0x7FF) << 16) | V1_X_ZOOM_ENABLE;
	    }
	}
    }

    return ~DP_FAIL;
}

CARD32 DDOVER_HQVCalcZoomHeight (CARD32 srcHeight,CARD32 dstHeight,
			     CARD32 * lpzoomCtl, CARD32 * lpminiCtl, CARD32 * lpHQVfilterCtl, CARD32 * lpHQVminiCtl,CARD32 * lpHQVzoomflag)
{
    CARD32 dwTmp;
    if (gVIAGraphicInfo.dwExpand)
    {
	dstHeight = dstHeight + 1;
    }

    if (srcHeight < dstHeight)
    {
	/* zoom in*/
	dwTmp = srcHeight * 0x0400 / dstHeight;
	*lpzoomCtl |= ((dwTmp & 0x3FF) | V1_Y_ZOOM_ENABLE);
	*lpminiCtl |= (V1_Y_INTERPOLY | V1_YCBCR_INTERPOLY);
	*lpHQVzoomflag = 1;
	*lpHQVfilterCtl |= HQV_V_TAP4_121;
    }
    else if (srcHeight == dstHeight)
    {
	*lpHQVfilterCtl |= HQV_V_TAP4_121;
    }
    else if (srcHeight > dstHeight)
    {
	/* zoom out*/
	CARD32 srcHeight1;

	dwTmp = dstHeight*0x0800*0x400 / srcHeight;
	dwTmp = dwTmp / 0x400 + ((dwTmp & 0x3ff)?1:0);

	*lpHQVminiCtl |= ((dwTmp& 0x7FF)<<16)|HQV_V_MINIFY_ENABLE;

	srcHeight1 = srcHeight >> 1;
	if (srcHeight1 <= dstHeight)
	{
	    *lpminiCtl |= V1_Y_DIV_2;
	    *lpHQVfilterCtl |= HQV_V_TAP4_121 ;
	}
	else
	{
	    srcHeight1 >>= 1;
	    if (srcHeight1 <= dstHeight)
	    {
		*lpminiCtl |= V1_Y_DIV_4;
		*lpHQVfilterCtl |= HQV_V_TAP4_121 ;
	    }
	    else
	    {
		srcHeight1 >>= 1;

		if (srcHeight1 <= dstHeight)
		{
		    *lpminiCtl |= V1_Y_DIV_8;
		    *lpHQVfilterCtl |= HQV_V_TAP8_12221;
		}
		else
		{
		    srcHeight1 >>= 1;

		    if (srcHeight1 <= dstHeight)
		    {
			*lpminiCtl |= V1_Y_DIV_16;
			*lpHQVfilterCtl |= HQV_V_TAP8_12221;
		    }
		    else
		    {
			*lpminiCtl |= V1_Y_DIV_16;
			*lpHQVfilterCtl |= HQV_V_TAP8_12221;
		    }
		}
	    }
	}

	*lpHQVminiCtl |= HQV_VDEBLOCK_FILTER;

	if (srcHeight1 < dstHeight)
	{
	    dwTmp = srcHeight1 * 0x0400 / dstHeight;
	    *lpzoomCtl |= ((dwTmp & 0x3FF) | V1_Y_ZOOM_ENABLE);
	    *lpminiCtl |= ( V1_Y_INTERPOLY|V1_YCBCR_INTERPOLY);
	}
    }

    return ~DP_FAIL;
}


CARD32 DDOver_GetFetch(CARD32 dwVideoFlag,LPVIDEOFORMAT lpVF,CARD32 dwSrcWidth,CARD32 dwDstWidth,CARD32 dwOriSrcWidth,CARD32 * lpHQVsrcFetch)
{
   CARD32 dwFetch=0;

   if (lpVF->dwFlags & VF_FOURCC)
   {
       DBG_DD(ErrorF("DDOver_GetFetch : FourCC= 0x%lx\n", lpVF->dwFourCC));
       switch (lpVF->dwFourCC) {
       case FOURCC_YV12:
	    if (dwVideoFlag&VIDEO_HQV_INUSE)
	    {
		*lpHQVsrcFetch = dwOriSrcWidth;
		if (dwDstWidth >= dwSrcWidth)
		    dwFetch = ((((dwSrcWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
		else
		    dwFetch = ((((dwDstWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
	    }
	    else
	    {
		dwFetch = (((dwSrcWidth + 0x1F)&~0x1f)>> V1_FETCHCOUNT_UNIT);
	    }
	    break;
       case FOURCC_UYVY:
       case FOURCC_YVYU:
       case FOURCC_YUY2:
	    if (dwVideoFlag&VIDEO_HQV_INUSE)
	    {
		*lpHQVsrcFetch = dwOriSrcWidth<<1;
		if (dwDstWidth >= dwSrcWidth)
		    dwFetch = ((((dwSrcWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
		else
		    dwFetch = ((((dwDstWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
	    }
	    else
	    {
		/*fetch one more quadword to avoid get less video data*/
		dwFetch = ((((dwSrcWidth<<1)+V1_FETCHCOUNT_ALIGNMENT)&~V1_FETCHCOUNT_ALIGNMENT) >> V1_FETCHCOUNT_UNIT)+1;
	    }
	    break;
       default :
	    DBG_DD(ErrorF("DDOver_GetFetch : Invalid FOURCC format :(0x%lx)in V1!\n", lpVF->dwFourCC));
	    break;
       }
   }

   /*Fix plannar mode problem*/
   if (dwFetch <4)
   {
	dwFetch = 4;
   }
   return dwFetch;
}

void DDOver_GetDisplayCount(CARD32 dwVideoFlag,LPVIDEOFORMAT lpVF,CARD32 dwSrcWidth,CARD32 * lpDisplayCountW)
{

   if (lpVF->dwFlags & VF_FOURCC)
   {
       switch (lpVF->dwFourCC) {
       case FOURCC_YV12:
       case FOURCC_UYVY:
       case FOURCC_YVYU:
       case FOURCC_YUY2:
	    if (dwVideoFlag&VIDEO_HQV_INUSE)
	    {
		*lpDisplayCountW = dwSrcWidth - 1;
	    }
	    else
	    {
		*lpDisplayCountW = dwSrcWidth - overlayRecordV1.dwminifyH;
	    }
	    break;
       default :
	    DBG_DD(ErrorF("DDOver_GetDisplayCount : Invalid FOURCC format :(0x%lx)in V1!\n", lpVF->dwFourCC));
	    if (dwVideoFlag&VIDEO_HQV_INUSE)
	    {
		*lpDisplayCountW = dwSrcWidth - 1;
	    }
	    else
	    {
		*lpDisplayCountW = dwSrcWidth - overlayRecordV1.dwminifyH;
	    }
	    break;
       }
   }
}
