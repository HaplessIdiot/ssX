/* $XFree86$ */
/*
 * Copyright 1999, 2000 ATI Technologies Inc., Markham, Ontario
 *		    and Precision Insight, Inc., Cedar Park, Texas.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation on
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *   Rickard E. Faith <faith@precisioninsight.com>
 *
 */

#ifndef _R128_DRI_
#define _R128_DRI_

#include <xf86drm.h>

/* DRI Driver defaults */
#define R128_DEFAULT_CCE_PIO_MODE R128_PM4_64PIO_64VCBM_64INDBM
#define R128_DEFAULT_CCE_BM_MODE  R128_PM4_64BM_64VCBM_64INDBM
#define R128_DEFAULT_AGP_MODE	  2
#define R128_DEFAULT_AGP_SIZE	  8 /* MB (must be a power of 2 and > 4MB) */
#define R128_DEFAULT_RING_SIZE	  1 /* MB (must be page aligned) */
#define R128_DEFAULT_VB_SIZE	  1 /* MB (must be page aligned) */
#define R128_DEFAULT_IND_SIZE	  1 /* MB (must be page aligned) */
#define R128_DEFAULT_AGP_TEX_SIZE 1 /* MB (must be page aligned) */

#define R128_DEFAULT_VB_BUF_SIZE  16384	 /* bytes */
#define R128_DEFAULT_CCE_TIMEOUT  10000	 /* usecs */

#define R128_AGP_MAX_MODE	  2

#define R128CCE_USE_RING_BUFFER(m)					  \
(((m) == R128_PM4_192BM) ||						  \
 ((m) == R128_PM4_128BM_64INDBM) ||					  \
 ((m) == R128_PM4_64BM_128INDBM) ||					  \
 ((m) == R128_PM4_64BM_64VCBM_64INDBM))

typedef struct {
    /* MMIO register data */
    drmHandle	  registerHandle;
    drmSize	  registerSize;

    /* CCE ring buffer data */
    drmHandle	  ringHandle;
    drmSize	  ringMapSize;
    int		  ringSize;

    /* CCE ring read pointer data */
    drmHandle	  ringReadPtrHandle;
    drmSize	  ringReadMapSize;

    /* CCE vertex buffer data */
    drmHandle	  vbHandle;
    drmSize	  vbMapSize;
    int		  vbOffset;
    int		  vbBufSize;

    /* CCE indirect buffer data */
    drmHandle	  indHandle;
    drmSize	  indMapSize;

    /* CCE AGP Texture data */
    drmHandle	  agpTexHandle;
    drmSize	  agpTexMapSize;
    int		  log2AGPTexGran;
    int		  agpTexOffset;

    /* DRI screen private data */
    int		  deviceID;	/* PCI device ID */
    int		  width;	/* Width in pixels of display */
    int		  height;	/* Height in scanlines of display */
    int		  depth;	/* Depth of display (8, 15, 16, 24) */
    int		  bpp;		/* Bit depth of display (8, 16, 24, 32) */

    int		  fbX;		/* Start of frame buffer */
    int		  fbY;
    int		  backX;	/* Start of shared back buffer */
    int		  backY;
    int		  depthX;	/* Start of shared depth buffer */
    int		  depthY;
    int		  textureX;	/* Start of texture data in frame buffer */
    int		  textureY;
    int		  textureSize;
    int		  log2TexGran;

    int		  IsPCI;	/* Current card is a PCI card */

    int		  CCEMode;	/* CCE mode that server/clients use */
    int		  CCEFifoSize;	/* Size of the CCE command FIFO */
} R128DRIRec, *R128DRIPtr;

#endif
