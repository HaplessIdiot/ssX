/* xf86drmR128.c -- User-level interface to Rage 128 DRM device
 * Created: Sun Apr  9 18:13:54 2000 by kevin@precisioninsight.com
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

#ifdef XFree86Server
# include "xf86.h"
# include "xf86_OSproc.h"
# include "xf86_ansic.h"
# include "xf86Priv.h"
# define _DRM_MALLOC xalloc
# define _DRM_FREE   xfree
# ifndef XFree86LOADER
#  include <sys/stat.h>
#  include <sys/mman.h>
# endif
#else
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <ctype.h>
# include <fcntl.h>
# include <errno.h>
# include <signal.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/ioctl.h>
# include <sys/mman.h>
# include <sys/time.h>
# ifdef DRM_USE_MALLOC
#  define _DRM_MALLOC malloc
#  define _DRM_FREE   free
extern int xf86InstallSIGIOHandler(int fd, void (*f)(int, void *), void *);
extern int xf86RemoveSIGIOHandler(int fd);
# else
#  include <Xlibint.h>
#  define _DRM_MALLOC Xmalloc
#  define _DRM_FREE   Xfree
# endif
#endif

/* Not all systems have MAP_FAILED defined */
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#include "xf86drm.h"
#include "xf86drmR128.h"
#include "drm.h"

int drmR128InitCCE(int fd, drmR128Init *info)
{
    drm_r128_init_t init;
   
    memset(&init, 0, sizeof(drm_r128_init_t));

    init.func                = R128_INIT_CCE;
    init.sarea_priv_offset   = info->sarea_priv_offset;
    init.is_pci              = info->is_pci;
    init.cce_mode            = info->cce_mode;
    init.cce_fifo_size       = info->cce_fifo_size;
    init.cce_secure          = info->cce_secure;
    init.ring_size           = info->ring_size;
    init.usec_timeout        = info->usec_timeout;

    init.fb_offset           = info->fb_offset;
    init.agp_ring_offset     = info->agp_ring_offset;
    init.agp_read_ptr_offset = info->agp_read_ptr_offset;
    init.agp_vertbufs_offset = info->agp_vertbufs_offset;
    init.agp_indbufs_offset  = info->agp_indbufs_offset;
    init.agp_textures_offset = info->agp_textures_offset;
    init.mmio_offset         = info->mmio_offset;

    if (ioctl(fd, DRM_IOCTL_R128_INIT, &init)) return -errno;

    return 0;
}

int drmR128CleanupCCE(int fd)
{
    drm_r128_init_t init;

    memset(&init, 0, sizeof(drm_r128_init_t));

    init.func = R128_CLEANUP_CCE;

    if (ioctl(fd, DRM_IOCTL_R128_INIT, &init)) return -errno;

    return 0;
}

int drmR128EngineReset(int fd)
{
    if (ioctl(fd, DRM_IOCTL_R128_RESET, NULL)) return -errno;

    return 0;
}

int drmR128EngineFlush(int fd)
{
    if (ioctl(fd, DRM_IOCTL_R128_FLUSH, NULL)) return -errno;

    return 0;
}

int drmR128CCEWaitForIdle(int fd)
{
    if (ioctl(fd, DRM_IOCTL_R128_CCEIDL, NULL)) return -errno;

    return 0;
}

int drmR128SubmitPackets(int fd, CARD32 *buffer, int *count, int flags)
{
    drm_r128_packet_t packet;
    int               ret;

    memset(&packet, 0, sizeof(drm_r128_packet_t));

    packet.count = *count;
    packet.flags = flags;

    while (packet.count > 0) {
	packet.buffer = buffer + (*count - packet.count);
	ret = ioctl(fd, DRM_IOCTL_R128_PACKET, &packet);
	if (ret < 0 && ret != -EAGAIN) {
	    *count = packet.count;
	    return -errno;
	}
    }

    *count = 0;
    return 0;
}

int drmR128GetVertexBuffers(int fd, int count, int *indices, int *sizes)
{
    drm_r128_vertex_t v;

    v.send_count      = 0;
    v.send_indices    = NULL;
    v.send_sizes      = NULL;
    v.prim            = DRM_R128_PRIM_NONE;
    v.request_count   = count;
    v.request_indices = indices;
    v.request_sizes   = sizes;
    v.granted_count   = 0;

    if (ioctl(fd, DRM_IOCTL_R128_VERTEX, &v)) return -errno;

    return v.granted_count;
}

int drmR128FlushVertexBuffers(int fd, int count, int *indices,
			      int *sizes, drmR128PrimType prim)
{
    drm_r128_vertex_t v;

    v.send_count      = count;
    v.send_indices    = indices;
    v.send_sizes      = sizes;
    v.prim            = prim;
    v.request_count   = 0;
    v.request_indices = NULL;
    v.request_sizes   = NULL;
    v.granted_count   = 0;

    if (ioctl(fd, DRM_IOCTL_R128_VERTEX, &v) < 0) return -errno;

    return 0;
}
