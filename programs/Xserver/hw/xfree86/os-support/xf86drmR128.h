/* xf86drm.h -- OS-independent header for DRM user-level library interface
 * Created: Sun Apr  9 18:16:28 2000 by kevin@precisioninsight.com
 *
 * Copyright 1999, 2000 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Kevin E. Martin <kevin@precisioninsight.com>
 *
 * $XFree86$
 *
 */

#ifndef _XF86DRI_R128_H_
#define _XF86DRI_R128_H_

/* WARNING: If you change any of these defines, make sure to change
 * the kernel include file as well (r128_drm.h)
 */

typedef struct _drmR128Init {
    int sarea_priv_offset;
    int is_pci;
    int cce_mode;
    int cce_fifo_size;
    int cce_secure;
    int ring_size;
    int usec_timeout;

    int fb_offset;
    int agp_ring_offset;
    int agp_read_ptr_offset;
    int agp_vertbufs_offset;
    int agp_indbufs_offset;
    int agp_textures_offset;
    int mmio_offset;
} drmR128Init;

typedef enum {
    DRM_R128_PRIM_NONE		= 0x0001,
    DRM_R128_PRIM_POINT		= 0x0002,
    DRM_R128_PRIM_LINE		= 0x0004,
    DRM_R128_PRIM_POLY_LINE	= 0x0008,
    DRM_R128_PRIM_TRI_LIST	= 0x0010,
    DRM_R128_PRIM_TRI_FAN	= 0x0020,
    DRM_R128_PRIM_TRI_STRIP	= 0x0040,
    DRM_R128_PRIM_TRI_TYPE2	= 0x0080
} drmR128PrimType;


extern int drmR128InitCCE(int fd, drmR128Init *info);
extern int drmR128CleanupCCE(int fd);
extern int drmR128EngineReset(int fd);
extern int drmR128EngineFlush(int fd);
extern int drmR128WaitForIdle(int fd);
extern int drmR128SubmitPacket(int fd, CARD32 *buffer, int *count, int flags);
extern int drmR128GetVertexBuffers(int fd, int count, int *indices,
				   int *sizes);
extern int drmR128FlushVertexBuffers(int fd, int count, int *indices,
				     int *sizes, drmR128PrimType prim);

#endif
