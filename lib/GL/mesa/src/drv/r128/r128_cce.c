/* $XFree86: xc/lib/GL/mesa/src/drv/r128/r128_cce.c,v 1.1 2000/06/17 00:03:04 martin Exp $ */
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
 *   Gareth Hughes <gareth@precisioninsight.com>
 *
 */

#include "r128_init.h"
#include "r128_mesa.h"
#include "r128_xmesa.h"
#include "r128_context.h"
#include "r128_lock.h"
#include "r128_state.h"
#include "r128_reg.h"
#include "r128_cce.h"

#define R128_TIMEOUT        2000000

r128_get_buffer_func		r128GetBufferLocked;
r128_fire_vertices_func		r128FireVerticesLocked;


static void r128RingStatus( r128ContextPtr r128ctx )
{
   r128ScreenPtr r128scrn = r128ctx->r128Screen;
   unsigned char *R128MMIO = r128scrn->mmio;

   fprintf( stderr, "GUI_STAT           = 0x%08x\n",
	    (unsigned int) INREG( R128_GUI_STAT ) );
   fprintf( stderr, "PM4_STAT           = 0x%08x\n",
	    (unsigned int) INREG( R128_PM4_STAT ) );
   fprintf( stderr, "PM4_BUFFER_DL_WPTR = 0x%08x\n",
	    (unsigned int) INREG( R128_PM4_BUFFER_DL_WPTR ) );
   fprintf( stderr, "PM4_BUFFER_DL_RPTR = 0x%08x\n",
	    (unsigned int) INREG( R128_PM4_BUFFER_DL_RPTR ) );
   fprintf( stderr, "ringWrite          = 0x%08x\n",
	    r128scrn->SAREA->ringWrite );
   fprintf( stderr, "*ringReadPtr       = 0x%08x\n",
	    *r128scrn->ringReadPtr );
}


/* Get a new VB from the pool of vertex buffers in AGP space.
 */
static drmBufPtr r128_get_buffer_locked( r128ContextPtr r128ctx )
{
   int fd = r128ctx->r128Screen->driScreen->fd;
   int num = 0;
   int index = 0;
   int size = 0;
   int to = 0;
   drmBufPtr buf = NULL;

   while ( !buf && ( to++ < r128ctx->CCEtimeout ) ) {
      num = drmR128GetVertexBuffers( fd, 1, &index, &size );

      if ( num > 0 ) {
	 buf = &r128ctx->r128Screen->vbBufs->list[index];
	 buf->used = 0;
#if ENABLE_PERF_BOXES
	 /* Bump the performance counter */
	 r128ctx->c_vertexBuffers++;
#endif
	 return buf;
      }
   }

   if ( !buf ) {
      drmR128EngineReset( fd );
      fprintf( stderr, "Error: Could not get new VB... exiting\n" );
      exit( -1 );
   }

   return buf;
}

static void r128_fire_vertices_locked( r128ContextPtr r128ctx )
{
   int vertsize = r128ctx->vertsize;
   int format = r128ctx->vc_format;
   int index = r128ctx->vert_buf->idx;
   int offset = r128ctx->r128Screen->vbOffset +
      index * r128ctx->r128Screen->vbBufSize;
   int size = r128ctx->vert_buf->used / (vertsize * sizeof(GLuint));
   int fd = r128ctx->r128Screen->driScreen->fd;
   int to = 0;
   int ret;
   CARD32 prim;

   prim = R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST;

   /* Send the vertex buffer to the hardware */
   BEGIN_CLIP_LOOP( r128ctx );
   {
      R128CCE3( R128_CCE_PACKET3_3D_RNDR_GEN_INDX_PRIM, 3 );
      R128CCE( offset );
      R128CCE( size );
      R128CCE( format );
      R128CCE( prim | R128_CCE_VC_CNTL_PRIM_WALK_LIST |
	       (size << R128_CCE_VC_CNTL_NUM_SHIFT) );

      R128CCE_SUBMIT_PACKET();
   }
   END_CLIP_LOOP( r128ctx );

   /* Tell the kernel to release the vertex buffer */
   do {
      ret = drmR128FlushVertexBuffers( fd, 1, &index, &size, 0 );
   } while ( ( ret == -EBUSY ) && ( to++ < r128ctx->CCEtimeout ) );

   if ( ret < 0 ) {
      drmR128EngineReset( fd );
      fprintf( stderr, "Error: Could not flush VB... exiting\n" );
      exit( -1 );
   }

   r128ctx->vert_buf = NULL;
}

static drmBufPtr r128_get_ring_locked( r128ContextPtr r128ctx )
{
   r128ScreenPtr r128scrn = r128ctx->r128Screen;
   drmBufPtr buf = NULL;

   buf = (drmBufPtr) malloc( sizeof(*buf) );

   buf->idx = -666;
   buf->total = r128scrn->vbBufSize;
   buf->used = 0;
   buf->address = (drmAddress *) malloc( r128scrn->vbBufSize );

#if ENABLE_PERF_BOXES
   /* Bump the performance counter */
   r128ctx->c_vertexBuffers++;
#endif

   return buf;
}

static void r128_fire_ring_locked( r128ContextPtr r128ctx )
{
   int vertsize = r128ctx->vertsize;
   int format = r128ctx->vc_format;
   int size = r128ctx->vert_buf->used / (vertsize * sizeof(GLuint));
   drmBufPtr buf = r128ctx->vert_buf;
   int dwords = buf->used >> 2;
   CARD32 prim;

   prim = R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST;

   /* Send the vertex buffer to the hardware */
   BEGIN_CLIP_LOOP( r128ctx );
   {
      R128CCE3( R128_CCE_PACKET3_3D_RNDR_GEN_PRIM, dwords + 1 );
      R128CCE( format );
      R128CCE( prim | R128_CCE_VC_CNTL_PRIM_WALK_RING |
	       (size << R128_CCE_VC_CNTL_NUM_SHIFT) );

      memcpy( &r128ctx->CCEbuf[r128ctx->CCEcount], buf->address, buf->used );
      r128ctx->CCEcount += dwords;

      R128CCE_SUBMIT_PACKET();
   }
   END_CLIP_LOOP( r128ctx );

   free( buf->address );
   free( buf );

   r128ctx->vert_buf = NULL;
}

/* Allocate some space in the current vertex buffer.  If the current
 * buffer is full, flush it and grab another one.
 */
CARD32 *r128AllocVertexDwords( r128ContextPtr r128ctx, int dwords )
{
	return r128AllocVertexDwordsInlined( r128ctx, dwords );
}


/* ================================================================ */


void r128GetEltBufLocked( r128ContextPtr r128ctx )
{
#if 0
   r128ctx->elt_buf = r128GetBufferLocked( r128ctx );
#endif
}

void r128FireEltsLocked( r128ContextPtr r128ctx,
			 GLuint num_elts, GLuint discard )
{
#if 0
   r128ScreenPtr r128scrn = r128ctx->r128Screen;
   int vertsize = r128ctx->vertsize;
   int format = r128ctx->vc_format;

   int index = r128ctx->elt_buf->idx;
   int offset = r128scrn->vbOffset;
   int total = r128scrn->vbRgn.size / (vertsize * sizeof(GLuint));
   int size = r128ctx->elt_buf->used / (vertsize * sizeof(GLuint));

   int fd = r128scrn->driScreen->fd;
   int to = 0;
   int ret;
   CARD32 prim;

   fprintf( stderr, "%s: ofs=0x%x sz=0x%x tot=%d elts=%d discard=%d\n",
	    __FUNCTION__, offset, size, total, num_elts, discard );

   prim = R128_CCE_VC_CNTL_PRIM_TYPE_TRI_LIST;

   /* Send the vertex buffer to the hardware */
   BEGIN_CLIP_LOOP( r128ctx );
   {
      R128CCE3( R128_CCE_PACKET3_3D_RNDR_GEN_INDX_PRIM, 3 );
      R128CCE( offset );
      R128CCE( size );
      R128CCE( format );
      R128CCE( prim | R128_CCE_VC_CNTL_PRIM_WALK_IND |
	       (num_elts << R128_CCE_VC_CNTL_NUM_SHIFT) );

      /* GTH: We really need indirect buffers for this... */

      R128CCE_SUBMIT_PACKET();
   }
   END_CLIP_LOOP( r128ctx );

   if ( discard ) {
      /* Tell the kernel to release the vertex buffer */
      do {
	 ret = drmR128FlushVertexBuffers( fd, 1, &index, &size, 0 );
      } while ( ( ret == -EBUSY ) && ( to++ < r128ctx->CCEtimeout ) );

      if ( ret < 0 ) {
	 drmR128EngineReset( fd );
	 fprintf( stderr, "Error: Could not flush VB... exiting\n" );
	 exit( -1 );
      }
   }

   r128ctx->vert_buf = NULL;
#endif
}

void r128FlushEltsLocked( r128ContextPtr r128ctx )
{
#if 0
   if ( r128ctx->first_elt != r128ctx->next_elt ) {
      r128FireEltsLocked( r128ctx,
			  (GLuint)r128ctx->next_elt -
			  (GLuint)r128ctx->first_elt,
			  0 );
      r128ctx->first_elt = r128ctx->next_elt;
   }
#endif
}

void r128ReleaseBufLocked( r128ContextPtr r128ctx, drmBufPtr buffer )
{
#if 0
   int index;
   int size;
   int fd = r128ctx->r128Screen->driScreen->fd;
   int to = 0;
   int ret;

   if ( !buffer ) return;

   index = buffer->idx;
   size = buffer->used = 0;

   /* Tell the kernel to release the vertex buffer */
   do {
      ret = drmR128FlushVertexBuffers( fd, 1, &index, &size, 0 );
   } while ( ( ret == -EBUSY ) && ( to++ < r128ctx->CCEtimeout ) );

   if ( ret < 0 ) {
      drmR128EngineReset( fd );
      fprintf( stderr, "Error: Could not release VB... exiting\n" );
      exit( -1 );
   }
#endif
}


void r128InitVertexBuffers( r128ScreenPtr r128scrn )
{
   if ( !r128scrn->IsPCI ) {
      r128GetBufferLocked = r128_get_buffer_locked;
      r128FireVerticesLocked = r128_fire_vertices_locked;
   } else {
      r128GetBufferLocked = r128_get_ring_locked;
      r128FireVerticesLocked = r128_fire_ring_locked;
   }

   /* Provide a fallback for Rage 128 Pro cards until we fix the lockups */
   if (getenv("LIBGL_USE_RING_VB")) {
      r128GetBufferLocked = r128_get_ring_locked;
      r128FireVerticesLocked = r128_fire_ring_locked;
   }
}
