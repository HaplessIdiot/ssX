/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_cce.h,v 1.1 2000/06/17 00:03:04 martin Exp $ */
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
 *   Gareth Hughes <gareth@precisioninsight.com>
 *
 */

#ifndef _R128_CCE_H_
#define _R128_CCE_H_

#ifdef GLX_DIRECT_RENDERING

#include "r128_dri.h"
#include "r128_reg.h"
#include "r128_lock.h"
#include "r128_xmesa.h"

#include "xf86drmR128.h"

#define R128_DEFAULT_TOTAL_CCE_TIMEOUT 1000000 /* usecs */

typedef union {
    float f;
    int   i;
} floatTOint;

/* Insert an integer value into the CCE ring buffer. */
#define R128CCE(v)				\
do {						\
   r128ctx->CCEbuf[r128ctx->CCEcount] = (v);	\
   r128ctx->CCEcount++;				\
} while (0)

/* Insert an floating point value into the CCE ring buffer. */
#define R128CCEF(v)				\
do {						\
   floatTOint fTi;				\
   fTi.f = (v);					\
   r128ctx->CCEbuf[r128ctx->CCEcount] = fTi.i;	\
   r128ctx->CCEcount++;				\
} while (0)

/* Insert a type-[0123] packet header into the ring buffer */
#define R128CCE0(p,r,n)   R128CCE((p) | ((n) << 16) | ((r) >> 2))
#define R128CCE1(p,r1,r2) R128CCE((p) | (((r2) >> 2) << 11) | ((r1) >> 2))
#define R128CCE2(p)       R128CCE((p))
#define R128CCE3(p,n)     R128CCE((p) | ((n) << 16))

#define R128CCE_SUBMIT_PACKET()						  \
do {									  \
   r128SubmitPacketLocked( r128ctx, r128ctx->CCEbuf, r128ctx->CCEcount ); \
   r128ctx->CCEcount = 0;						  \
} while (0)

static __inline void r128SubmitPacketLocked( r128ContextPtr r128ctx,
					     CARD32 *buf, GLuint count )
{
   CARD32 *b;
   int c = count;
   int fd = r128ctx->r128Screen->driScreen->fd;
   int to = 0;
   int ret;

   do {
      b = buf + (count - c);
      ret = drmR128SubmitPacket( fd, b, &c, 0 );
   } while ( ( ret == -EBUSY ) && ( to++ < r128ctx->CCEtimeout ) );

   if ( ret < 0 ) {
      drmR128EngineReset( fd );
      fprintf( stderr, "Error: Could not submit packet... exiting\n" );
      exit( -1 );
   }
}

static __inline void r128WaitForIdleLocked( r128ContextPtr r128ctx )
{
    int fd = r128ctx->r128Screen->driScreen->fd;
    int to = 0;
    int ret;

    drmR128EngineFlush( fd );

    do {
	ret = drmR128WaitForIdle( fd );
    } while ( ( ret == -EBUSY ) && ( to++ < r128ctx->CCEtimeout ) );

    if ( ret < 0 ) {
	drmR128EngineReset( fd );
	fprintf( stderr, "Error: Rage 128 timed out... exiting\n" );
	exit( -1 );
    }
}

/* Vertex buffer handling functions:
 */
extern void r128InitVertexBuffers( r128ScreenPtr r128scrn );

typedef drmBufPtr (*r128_get_buffer_func)( r128ContextPtr r128ctx );
typedef void (*r128_fire_vertices_func)( r128ContextPtr r128ctx );

extern r128_get_buffer_func		r128GetBufferLocked;
extern r128_fire_vertices_func		r128FireVerticesLocked;

extern CARD32 *r128AllocVertexDwords( r128ContextPtr r128ctx, int dwords );

extern void r128GetEltBufLocked( r128ContextPtr r128ctx );
extern void r128FlushEltsLocked( r128ContextPtr r128ctx );
extern void r128FireEltsLocked( r128ContextPtr r128ctx,
				GLuint num_elts, GLuint discard );
extern void r128ReleaseBufLocked( r128ContextPtr r128ctx, drmBufPtr buffer );


static __inline void r128FlushVerticesLocked( r128ContextPtr r128ctx )
{
    if ( r128ctx->vert_buf && r128ctx->vert_buf->used ) {
       r128FireVerticesLocked( r128ctx );
    }
}

static __inline CARD32 *r128AllocVertexDwordsInlined( r128ContextPtr r128ctx,
						      int dwords )
{
    int bytes = dwords * 4;
    CARD32 *head;

    if ( !r128ctx->vert_buf ) {
	LOCK_HARDWARE( r128ctx );
#if 0
	if ( r128ctx->first_elt != r128ctx->next_elt ) {
	    r128FlushEltsLocked( r128ctx );
	}
#endif
	r128ctx->vert_buf = r128GetBufferLocked( r128ctx );

	UNLOCK_HARDWARE( r128ctx );
    } else if ( r128ctx->vert_buf->used + bytes > r128ctx->vert_buf->total ) {
	LOCK_HARDWARE( r128ctx );

	r128FlushVerticesLocked( r128ctx );
	r128ctx->vert_buf = r128GetBufferLocked( r128ctx );

	UNLOCK_HARDWARE( r128ctx );
    }

    head = (CARD32 *)((char *)r128ctx->vert_buf->address +
		      r128ctx->vert_buf->used);

    r128ctx->vert_buf->used += bytes;
    return head;
}

#define r128WaitForIdle( r128ctx )		\
do {						\
   LOCK_HARDWARE( r128ctx );			\
   r128WaitForIdleLocked( r128ctx );		\
   UNLOCK_HARDWARE( r128ctx );			\
} while (0)

#define r128FlushVertices( r128ctx )		\
do {						\
   LOCK_HARDWARE( r128ctx );			\
   r128FlushVerticesLocked( r128ctx );		\
   UNLOCK_HARDWARE( r128ctx );			\
} while (0)

#define r128FlushElts( r128ctx )		\
do {						\
   LOCK_HARDWARE( r128ctx );			\
   r128FlushEltsLocked( r128ctx );		\
   UNLOCK_HARDWARE( r128ctx );			\
} while (0)

#define FLUSH_BATCH( r128ctx )						\
do {									\
   if ( R128_DEBUG_FLAGS & DEBUG_VERBOSE_IOCTL )			\
      fprintf( stderr, "FLUSH_BATCH in %s\n", __FUNCTION__ );		\
   if ( r128ctx->vert_buf ) {						\
      r128FlushVertices( r128ctx );					\
   }									\
} while (0)

#endif
#endif /* _R128_CCE_H_ */
