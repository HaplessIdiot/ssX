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
#ifndef _LINUX_VIDEO_H
#define _LINUX_VIDEO_H

#include "capture.h"

/* Alignment macro functions */
#define ALIGN_TO_32_BYTES(f)	     (((f) + 31) & ~31)

/* FOURCC definitions */
#define FOURCC_YUY2    0x32595559  /*YUY2*/
#define FOURCC_YV12    0x32315659  /*YV12*/
#define FOURCC_UYVY    0x59565955
#define FOURCC_YVYU    0x55595659
#define FOURCC_HQVSW   0x34565148  /*HQV4*/


/*
 * Structures for create surface
 */
typedef struct _SURFACEPARAM
{
    CARD32     dwHeight;    /* height of input source */
    CARD32     dwWidth;	    /* width of input source */
    CARD32     lPitch;	    /* The length to start of next line */
    CARD32     dwBackBuffers;	  /* number of back buffers */
    void *     lpSurface;	      /* pointer to the mem-map surface memory address */
    CARD32     dwFourCC;	      /* (FOURCC code)*/
} SURFACEPARAM;
typedef SURFACEPARAM * LPSURFACEPARAM;

typedef struct _LOCKPARAM
{
    CARD32     dwFourCC;
    CARD32     dwPhysicalBase;
    CAPDEVICE SWDevice;
} LOCKPARAM;
typedef LOCKPARAM * LPLOCKPARAM;


/*
 * structure for VIAVidUpdateOverlay() use
 */
typedef struct _BOXPARAM
{
    CARD32     x1;
    CARD32     y1;
    CARD32     x2;
    CARD32     y2;
} BOXPARAM;

typedef struct _UPDATEOVERLAYREC
{
    BOXPARAM	 DestBox;	  /* dest. video rect. */
    BOXPARAM	 SrcBox;	  /* src. video rect. */
    CARD32     dwFlags;		/* flags */
    CARD32     dwColorKey;
    CARD32     dwFourcc;
} UPDATEOVERLAYREC;
typedef UPDATEOVERLAYREC * LPUPDATEOVERLAYREC;

/* Definition for dwFlags */
#define OVERLAY_HIDE	   0x00000001
#define OVERLAY_SHOW	   0x00000002
#define OVERLAY_KEYDEST	   0x00000004

/*
 *  Use bob de-interlace method
 */
#define OVERLAY_BOB				0x00200000l


/*
 * inedtify that the surface memory is composed of interleaved fields.
 */
#define OVERLAY_INTERLEAVED			0x00800000l

typedef struct
{
    CARD32   dwWidth;
    CARD32   dwHeight;
    CARD32   dwOffset;
    CARD32   dwUVoffset;
    CARD32   dwFlipTime;
    CARD32   dwFlipTag;
    CARD32   dwStartAddr;
    CARD32   dwV1OriWidth;
    CARD32   dwV1OriHeight;
    CARD32   dwV1OriPitch;
    CARD32   dwV1SrcWidth;
    CARD32   dwV1SrcHeight;
    CARD32   dwV1SrcLeft;
    CARD32   dwV1SrcRight;
    CARD32   dwV1SrcTop;
    CARD32   dwV1SrcBot;
    CARD32   dwSPWidth;
    CARD32   dwSPHeight;
    CARD32   dwSPLeft;
    CARD32   dwSPRight;
    CARD32   dwSPTop;
    CARD32   dwSPBot;
    CARD32   dwSPOffset;
    CARD32   dwSPstartAddr;
    CARD32   dwDisplayPictStruct;
    CARD32   dwDisplayBuffIndex;	/* Display buffer Index. 0 to ( dwBufferNumber -1) */
    CARD32   dwFetchAlignment;
    CARD32   dwSPPitch;
    CARD32   dwHQVAddr[2];
    CARD32   dwHQVheapInfo;	    /* video memory heap of the HQV buffer */
    CARD32   dwVideoControl;	    /* video control flag */
    CARD32   dwminifyH;			   /* Horizontal minify factor */
    CARD32   dwminifyV;			   /* Vertical minify factor */
    CARD32   dwMpegDecoded;
} OVERLAYRECORD;

/*
 *  source video format
 */
typedef struct _VIDEOFORMAT
{
    CARD32	dwFlags;		/* video format flags */
    CARD32	dwFourCC;		/* (FOURCC code) */

} VIDEOFORMAT;
typedef VIDEOFORMAT * LPVIDEOFORMAT;

/****************************************************************************
 *
 * VIDEOFORMAT FLAGS
 *
 ****************************************************************************/

#define VF_FOURCC				0x00000004l
#define VF_RGB					0x00000040l


/*
 * Return value of DriverProc
 */

#define DP_OK				   0x00
#define DP_FAIL				    0x01
#define DP_FAIL_NO_X_WINDOW		    DP_FAIL +1
#define DP_FAIL_CANNOT_OPEN_VIDEO_DEVICE    DP_FAIL +2
#define DP_FAIL_CANNOT_USE_IOCTL	    DP_FAIL +3
#define DP_FAIL_CANNOT_CREATE_SURFACE	    DP_FAIL +4


#endif
