/* $XFree86$ */
/**************************************************************************

Copyright 1999, 2000 ATI Technologies Inc. and Precision Insight, Inc.,
                                               Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *
 */

#include "r128_dri.h"
#include "r128_reg.h"

#include "r128_init.h"
#include "r128_context.h"
#include "r128_xmesa.h"
#include "r128_cce.h"
#include "r128_tris.h"
#include "r128_vb.h"
#include "r128_fastpath.h"

#include <sys/mman.h>

/* Create the device specific screen private data struct */
r128ScreenPtr r128CreateScreen(__DRIscreenPrivate *sPriv)
{
    r128ScreenPtr    r128Screen;
    R128DRIPtr       r128DRIPriv = (R128DRIPtr)sPriv->pDevPriv;

    /* Allocate the private area */
    r128Screen = (r128ScreenPtr)Xmalloc(sizeof(*r128Screen));
    if (!r128Screen) return NULL;

    /* This is first since which regions we map depends on whether or
       not we are using a PCI card */
    r128Screen->IsPCI          = r128DRIPriv->IsPCI;

    r128Screen->mmioRgn.handle = r128DRIPriv->registerHandle;
    r128Screen->mmioRgn.size   = r128DRIPriv->registerSize;
    if (drmMap(sPriv->fd,
	       r128Screen->mmioRgn.handle,
	       r128Screen->mmioRgn.size,
	       (drmAddressPtr)&r128Screen->mmio)) {
	Xfree(r128Screen);
	return NULL;
    }

    if (!r128Screen->IsPCI) {
	r128Screen->ringRgn.handle = r128DRIPriv->ringHandle;
	r128Screen->ringRgn.size   = r128DRIPriv->ringMapSize;
	if (drmMap(sPriv->fd,
		   r128Screen->ringRgn.handle,
		   r128Screen->ringRgn.size,
		   (drmAddressPtr)&r128Screen->ring)) {
	    drmUnmap((drmAddress)r128Screen->mmio, r128Screen->mmioRgn.size);
	    Xfree(r128Screen);
	    return NULL;
	}

	r128Screen->ringReadRgn.handle = r128DRIPriv->ringReadPtrHandle;
	r128Screen->ringReadRgn.size   = r128DRIPriv->ringReadMapSize;
	if (drmMap(sPriv->fd,
		   r128Screen->ringReadRgn.handle,
		   r128Screen->ringReadRgn.size,
		   (drmAddressPtr)&r128Screen->ringReadPtr)) {
	    drmUnmap((drmAddress)r128Screen->ring, r128Screen->ringRgn.size);
	    drmUnmap((drmAddress)r128Screen->mmio, r128Screen->mmioRgn.size);
	    Xfree(r128Screen);
	    return NULL;
	}

	r128Screen->vbRgn.handle = r128DRIPriv->vbHandle;
	r128Screen->vbRgn.size   = r128DRIPriv->vbMapSize;
	r128Screen->vbOffset     = r128DRIPriv->vbOffset;
	if (drmMap(sPriv->fd,
		   r128Screen->vbRgn.handle,
		   r128Screen->vbRgn.size,
		   (drmAddressPtr)&r128Screen->vb)) {
	    drmUnmap((drmAddress)r128Screen->ringReadPtr,
		     r128Screen->ringReadRgn.size);
	    drmUnmap((drmAddress)r128Screen->ring, r128Screen->ringRgn.size);
	    drmUnmap((drmAddress)r128Screen->mmio, r128Screen->mmioRgn.size);
	    Xfree(r128Screen);
	    return NULL;
	}
	r128Screen->vbOffset     = r128DRIPriv->vbOffset;
	r128Screen->vbBufSize    = r128DRIPriv->vbBufSize;

	r128Screen->indRgn.handle = r128DRIPriv->indHandle;
	r128Screen->indRgn.size   = r128DRIPriv->indMapSize;
	if (drmMap(sPriv->fd,
		   r128Screen->indRgn.handle,
		   r128Screen->indRgn.size,
		   (drmAddressPtr)&r128Screen->ind)) {
	    drmUnmap((drmAddress)r128Screen->vb, r128Screen->vbRgn.size);
	    drmUnmap((drmAddress)r128Screen->ringReadPtr,
		     r128Screen->ringReadRgn.size);
	    drmUnmap((drmAddress)r128Screen->ring, r128Screen->ringRgn.size);
	    drmUnmap((drmAddress)r128Screen->mmio, r128Screen->mmioRgn.size);
	    Xfree(r128Screen);
	    return NULL;
	}

	r128Screen->agpTexRgn.handle = r128DRIPriv->agpTexHandle;
	r128Screen->agpTexRgn.size   = r128DRIPriv->agpTexMapSize;
	if (drmMap(sPriv->fd,
		   r128Screen->agpTexRgn.handle,
		   r128Screen->agpTexRgn.size,
		   (drmAddressPtr)&r128Screen->agpTex)) {
	    drmUnmap((drmAddress)r128Screen->ind,  r128Screen->indRgn.size);
	    drmUnmap((drmAddress)r128Screen->vb,   r128Screen->vbRgn.size);
	    drmUnmap((drmAddress)r128Screen->ringReadPtr,
		     r128Screen->ringReadRgn.size);
	    drmUnmap((drmAddress)r128Screen->ring, r128Screen->ringRgn.size);
	    drmUnmap((drmAddress)r128Screen->mmio, r128Screen->mmioRgn.size);
	    Xfree(r128Screen);
	    return NULL;
	}
	r128Screen->agpTexOffset   = r128DRIPriv->agpTexOffset;

	if (!(r128Screen->vbBufs = drmMapBufs(sPriv->fd))) {
	    drmUnmap((drmAddress)r128Screen->agpTex,
		     r128Screen->agpTexRgn.size);
	    drmUnmap((drmAddress)r128Screen->ind,  r128Screen->indRgn.size);
	    drmUnmap((drmAddress)r128Screen->vb,   r128Screen->vbRgn.size);
	    drmUnmap((drmAddress)r128Screen->ringReadPtr,
		     r128Screen->ringReadRgn.size);
	    drmUnmap((drmAddress)r128Screen->ring, r128Screen->ringRgn.size);
	    drmUnmap((drmAddress)r128Screen->mmio, r128Screen->mmioRgn.size);
	    Xfree(r128Screen);
	    return NULL;
	}
    }

    r128Screen->deviceID         = r128DRIPriv->deviceID;

    r128Screen->width            = r128DRIPriv->width;
    r128Screen->height           = r128DRIPriv->height;
    r128Screen->depth            = r128DRIPriv->depth;
    r128Screen->bpp              = r128DRIPriv->bpp;
    r128Screen->pixel_code       = (r128Screen->bpp != 16 ?
				    r128Screen->bpp :
				    r128Screen->depth);

    r128Screen->fb               = sPriv->pFB;
    r128Screen->fbOffset         = sPriv->fbOrigin;
    r128Screen->fbStride         = sPriv->fbStride;
    r128Screen->fbSize           = sPriv->fbSize;

    r128Screen->fbX              = r128DRIPriv->fbX;
    r128Screen->fbY              = r128DRIPriv->fbY;
    r128Screen->backX            = r128DRIPriv->backX;
    r128Screen->backY            = r128DRIPriv->backY;
    r128Screen->depthX           = r128DRIPriv->depthX;
    r128Screen->depthY           = r128DRIPriv->depthY;

    r128Screen->texOffset[R128_LOCAL_TEX_HEAP]     = (r128DRIPriv->textureY *
						      r128Screen->fbStride +
						      r128DRIPriv->textureX *
						      (r128Screen->bpp/8));
    r128Screen->texSize[R128_LOCAL_TEX_HEAP]       = r128DRIPriv->textureSize;
    r128Screen->log2TexGran[R128_LOCAL_TEX_HEAP]   = r128DRIPriv->log2TexGran;

    if (r128Screen->IsPCI) {
	r128Screen->texOffset[R128_AGP_TEX_HEAP]   = 0;
	r128Screen->texSize[R128_AGP_TEX_HEAP]     = 0;
	r128Screen->log2TexGran[R128_AGP_TEX_HEAP] = 0;
	r128Screen->NRTexHeaps                     = R128_NR_TEX_HEAPS-1;
    } else {
	r128Screen->texOffset[R128_AGP_TEX_HEAP]   = 0;
	r128Screen->texSize[R128_AGP_TEX_HEAP]     =
	    r128DRIPriv->agpTexMapSize;
	r128Screen->log2TexGran[R128_AGP_TEX_HEAP] =
	    r128DRIPriv->log2AGPTexGran;
	r128Screen->NRTexHeaps                     = R128_NR_TEX_HEAPS;
    }

#if 1
    /* FIXME: For testing only */
    if (getenv("LIBGL_SHOW_BUFFERS")) {
	r128Screen->backX        = 0;
	r128Screen->backY        = r128DRIPriv->height/2;
	r128Screen->depthX       = r128DRIPriv->width/2;
	r128Screen->depthY       = r128DRIPriv->height/2;
    }
#endif

    r128Screen->CCEMode          = r128DRIPriv->CCEMode;
    r128Screen->CCEFifoSize      = r128DRIPriv->CCEFifoSize;

    r128Screen->ringEntries      = r128DRIPriv->ringSize/sizeof(CARD32);
    if (!r128Screen->IsPCI) {
	r128Screen->ringStartPtr = (int *)r128Screen->ring;
	r128Screen->ringEndPtr   = (int *)(r128Screen->ring
					   + r128DRIPriv->ringSize);
    }

    r128Screen->MMIOFifoSlots    = 0;
    r128Screen->CCEFifoSlots     = 0;

    r128Screen->CCEFifoAddr      = R128_PM4_FIFO_DATA_EVEN;

    r128Screen->SAREA            = (R128SAREAPrivPtr)((char *)sPriv->pSAREA +
						      sizeof(XF86DRISAREARec));

    r128Screen->driScreen        = sPriv;

    r128InitVertexBuffers(r128Screen);

    r128FastPathInit();
    r128TriangleFuncsInit();
    r128SetupInit();

    return r128Screen;
}

/* Destroy the device specific screen private data struct */
void r128DestroyScreen(__DRIscreenPrivate *sPriv)
{
    r128ScreenPtr r128Screen = (r128ScreenPtr)sPriv->private;

    if (!r128Screen->IsPCI) {
	drmUnmapBufs(r128Screen->vbBufs);

	drmUnmap((drmAddress)r128Screen->agpTex, r128Screen->agpTexRgn.size);
	drmUnmap((drmAddress)r128Screen->ind,    r128Screen->indRgn.size);
	drmUnmap((drmAddress)r128Screen->vb,     r128Screen->vbRgn.size);
	drmUnmap((drmAddress)r128Screen->ringReadPtr,
		 r128Screen->ringReadRgn.size);
	drmUnmap((drmAddress)r128Screen->ring,   r128Screen->ringRgn.size);
    }
    drmUnmap((drmAddress)r128Screen->mmio,   r128Screen->mmioRgn.size);

    Xfree(r128Screen);
    sPriv->private = NULL;
}
