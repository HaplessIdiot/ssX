/* $XFree86$ */
/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

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
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 *
 */

#ifndef __RADEON_IOCTL_H__
#define __RADEON_IOCTL_H__

#ifdef GLX_DIRECT_RENDERING

#include "radeon_dri.h"
#include "radeon_lock.h"

#include "xf86drm.h"
#include "xf86drmRadeon.h"

#define RADEON_BUFFER_MAX_DWORDS	(RADEON_BUFFER_SIZE / sizeof(CARD32))


extern drmBufPtr radeonGetBufferLocked( radeonContextPtr rmesa );
extern void radeonEmitPrim( radeonContextPtr rmesa );
extern void radeonFlushPrims( radeonContextPtr rmesa );
extern void radeonFlushPrimsLocked( radeonContextPtr rmesa );
extern void radeonFlushPrimsGetBuffer( radeonContextPtr rmesa );
extern void radeonFireBlitLocked( radeonContextPtr rmesa,
				  drmBufPtr buffer,
				  GLint offset, GLint pitch, GLint format,
				  GLint x, GLint y,
				  GLint width, GLint height );
extern void radeonCopyBuffer( const __DRIdrawablePrivate *drawable );
extern void radeonPageFlip( const __DRIdrawablePrivate *drawable );
extern void radeonWaitForIdleLocked( radeonContextPtr rmesa );
extern void radeonPerformanceCounters( radeonContextPtr rmesa );
extern void radeonPerformanceBoxesLocked( radeonContextPtr rmesa );
extern void radeonInitIoctlFuncs( GLcontext *ctx );
extern void radeonReleaseRetainedBuffer( radeonContextPtr rmesa );


/* ================================================================
 * Helper macros:
 */

/* Can accomodate several state changes and primitive changes without
 * actually firing the buffer.
 */
#define RADEON_STATECHANGE( rmesa, flag )				\
do {									\
   if ( 0 ) radeonPrintDirty( __FUNCTION__, flag );			\
   if ( rmesa->dma.low != rmesa->dma.last )				\
      radeonEmitPrim( rmesa );						\
   rmesa->state.hw.dirty |= flag;					\
} while (0)


/* Fire the buffered vertices no matter what.
 */
#define RADEON_FIREVERTICES( rmesa )					\
do {									\
   if ( rmesa->store.primnr || rmesa->dma.low != rmesa->dma.last ) {	\
      if ( 0 )								\
	 fprintf( stderr, "RADEON_FIREVERTICES in %s\n",__FUNCTION__ );	\
      radeonFlushPrims( rmesa );					\
   }									\
} while (0)


static __inline void *radeonAllocDmaLow( radeonContextPtr rmesa,
					 int bytes, const char *where )
{
   if ( rmesa->dma.low + bytes > rmesa->dma.high ) {
      if (0) fprintf( stderr, "%s flush for %d (%d/%d/%d)\n",
		      where, bytes, rmesa->dma.last,
		      rmesa->dma.low, rmesa->dma.high );
      radeonFlushPrimsGetBuffer( rmesa );
   }

   {
      GLubyte *head = rmesa->dma.address + rmesa->dma.low;
      if (0) fprintf( stderr, "%s: alloc %d (%d/%d/%d)\n",
		      where, bytes, rmesa->dma.last,
		      rmesa->dma.low, rmesa->dma.high );
      rmesa->dma.low += bytes;
      return head;
   }
}

static __inline void *radeonAllocDmaHigh( radeonContextPtr rmesa, int bytes )
{
   if ( rmesa->dma.low + bytes > rmesa->dma.high )
      radeonFlushPrimsGetBuffer( rmesa );

   rmesa->dma.high -= bytes;
   return (void *)(rmesa->dma.address + rmesa->dma.high);
}



#endif
#endif /* __RADEON_IOCTL_H__ */
