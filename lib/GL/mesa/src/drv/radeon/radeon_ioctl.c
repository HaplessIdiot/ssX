/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_ioctl.c,v 1.5 2002/02/22 21:45:00 dawes Exp $ */
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

#include "radeon_context.h"
#include "radeon_state.h"
#include "radeon_ioctl.h"

#include "mem.h"
#include "macros.h"
#include "swrast/swrast.h"

#define RADEON_TIMEOUT             512


/* =============================================================
 * Hardware vertex buffer handling
 */

/* Get a new VB from the pool of vertex buffers in AGP space.
 */
drmBufPtr radeonGetBufferLocked( radeonContextPtr rmesa )
{
   int fd = rmesa->dri.fd;
   int index = 0;
   int size = 0;
   drmDMAReq dma;
   drmBufPtr buf = NULL;
   int to = 0;
   int ret;

   dma.context = rmesa->dri.hwContext;
   dma.send_count = 0;
   dma.send_list = NULL;
   dma.send_sizes = NULL;
   dma.flags = 0;
   dma.request_count = 1;
   dma.request_size = RADEON_BUFFER_SIZE;
   dma.request_list = &index;
   dma.request_sizes = &size;
   dma.granted_count = 0;

   while ( !buf && ( to++ < RADEON_TIMEOUT ) ) {
      ret = drmDMA( fd, &dma );

      if ( ret == 0 ) {
	 buf = &rmesa->radeonScreen->buffers->list[index];
	 buf->used = 0;
	 /* Bump the performance counter */
	 rmesa->c_vertexBuffers++;
	 return buf;
      }
   }

   if ( !buf ) {
      UNLOCK_HARDWARE( rmesa );
      fprintf( stderr, "Error: Could not get new VB... exiting\n" );
      exit( -1 );
   }

   return buf;
}


static GLboolean intersect_rect( XF86DRIClipRectPtr out,
				 XF86DRIClipRectPtr a,
				 XF86DRIClipRectPtr b )
{
   *out = *a;
   if ( b->x1 > out->x1 ) out->x1 = b->x1;
   if ( b->y1 > out->y1 ) out->y1 = b->y1;
   if ( b->x2 < out->x2 ) out->x2 = b->x2;
   if ( b->y2 < out->y2 ) out->y2 = b->y2;
   if ( out->x1 >= out->x2 ) return GL_FALSE;
   if ( out->y1 >= out->y2 ) return GL_FALSE;
   return GL_TRUE;
}

static void emit_state( radeonContextPtr rmesa,
			drmRadeonState *dest,
			int dirty )
{
   struct radeon_state *state = &rmesa->state;

   if ( dirty & RADEON_UPLOAD_CONTEXT )
      memcpy( &dest->context, &state->hw.context, sizeof(dest->context) );

   if ( dirty & RADEON_UPLOAD_VERTFMT )
      memcpy( &dest->vertex, &state->hw.vertex, sizeof(dest->vertex) );

   if ( dirty & RADEON_UPLOAD_LINE )
      memcpy( &dest->line, &state->hw.line, sizeof(dest->line) );

   if ( dirty & RADEON_UPLOAD_BUMPMAP )
      memcpy( &dest->bumpmap, &state->hw.bumpmap, sizeof(dest->bumpmap) );

   if ( dirty & RADEON_UPLOAD_MASKS )
      memcpy( &dest->mask, &state->hw.mask, sizeof(dest->mask) );

   if ( dirty & RADEON_UPLOAD_VIEWPORT )
      memcpy( &dest->viewport, &state->hw.viewport, sizeof(dest->viewport) );

   if ( dirty & RADEON_UPLOAD_SETUP ) {
      memcpy( &dest->setup1, &state->hw.setup1, sizeof(dest->setup1) );
      memcpy( &dest->setup2, &state->hw.setup2, sizeof(dest->setup2) );
   }

   if ( dirty & RADEON_UPLOAD_MISC )
      memcpy( &dest->misc, &state->hw.misc, sizeof(dest->misc) );

   if ( dirty & RADEON_UPLOAD_ZBIAS )
      memcpy( &dest->zbias, &state->hw.zbias, sizeof(dest->zbias) );

   /* Assemble the texture state, combining the texture object and
    * texture environment state into the hardware texture unit state.
    */
   if ( dirty & RADEON_UPLOAD_TEX0 ) {
      radeonTexObjPtr t0 = state->texture.unit[0].texobj;

      dest->texture[0].pp_txfilter = t0->pp_txfilter | state->hw.texture[0].pp_txfilter;
      dest->texture[0].pp_txformat = t0->pp_txformat | RADEON_TXFORMAT_ST_ROUTE_STQ0;
      dest->texture[0].pp_txoffset = t0->pp_txoffset;
      dest->texture[0].pp_border_color = t0->pp_border_color;
      dest->texture[0].pp_txcblend = state->hw.texture[0].pp_txcblend;
      dest->texture[0].pp_txablend = state->hw.texture[0].pp_txablend;
      dest->texture[0].pp_tfactor = state->hw.texture[0].pp_tfactor;
   }

   if ( dirty & RADEON_UPLOAD_TEX1 ) {
      radeonTexObjPtr t1 = state->texture.unit[1].texobj;

      dest->texture[1].pp_txfilter = t1->pp_txfilter | state->hw.texture[1].pp_txfilter;
      dest->texture[1].pp_txformat = t1->pp_txformat | RADEON_TXFORMAT_ST_ROUTE_STQ1;
      dest->texture[1].pp_txoffset = t1->pp_txoffset;
      dest->texture[1].pp_border_color = t1->pp_border_color;
      dest->texture[1].pp_txcblend = state->hw.texture[1].pp_txcblend;
      dest->texture[1].pp_txablend = state->hw.texture[1].pp_txablend;
      dest->texture[1].pp_tfactor = state->hw.texture[1].pp_tfactor;
   }
}

#if 0
static void print_values( const char *name, const void *vals, int sz )
{
   const int *ivals = (const int *)vals;
   int i;

   for (i = 0; i < sz/4 ; i++)
      fprintf(stderr, "%s %d: 0x%x\n", name, i, ivals[i]);
}
#endif
/*
static void print_state( drmRadeonState *state )
{
   int dirty = state->dirty;

   if ( dirty & RADEON_UPLOAD_CONTEXT ) 
      print_values( "CONTEXT", &state->context, sizeof(state->context) );

   if ( dirty & RADEON_UPLOAD_VERTFMT )
      print_values( "VERTFMT", &state->vertex, sizeof(state->vertex) );

   if ( dirty & RADEON_UPLOAD_LINE )
      print_values( "LINE", &state->line, sizeof(state->line) );

   if ( dirty & RADEON_UPLOAD_BUMPMAP )
      print_values( "BUMPMAP", &state->bumpmap, sizeof(state->bumpmap) );

   if ( dirty & RADEON_UPLOAD_MASKS )
      print_values( "MASKS", &state->mask, sizeof(state->mask) );

   if ( dirty & RADEON_UPLOAD_VIEWPORT )
      print_values( "VIEWPORT", &state->viewport, sizeof(state->viewport) );

   if ( dirty & RADEON_UPLOAD_SETUP ) {
      print_values( "SETUP", &state->setup1, sizeof(state->setup1) );
      print_values( "SETUP2", &state->setup2, sizeof(state->setup2) );
   }

   if ( dirty & RADEON_UPLOAD_MISC )
      print_values( "MISC", &state->misc, sizeof(state->misc) );

   if ( dirty & RADEON_UPLOAD_ZBIAS )
      print_values( "ZBIAS", &state->zbias, sizeof(state->zbias) );

   if ( dirty & RADEON_UPLOAD_TEX0 ) 
      print_values( "TEX0", &state->texture[0], sizeof(state->texture[0]) );

   if ( dirty & RADEON_UPLOAD_TEX1 ) 
      print_values( "TEX1", &state->texture[1], sizeof(state->texture[1]) );
}
*/

static void emit_prim( radeonContextPtr rmesa )
{
   GLuint prim = rmesa->store.primnr++;
   GLuint dirty = rmesa->state.hw.dirty;

   rmesa->store.prim[prim].prim = rmesa->hw_primitive;
   rmesa->store.prim[prim].start = rmesa->dma.last;
   rmesa->store.prim[prim].finish = rmesa->dma.low;
   rmesa->store.prim[prim].vc_format = rmesa->vertex_format;

   if (rmesa->hw_primitive & RADEON_CP_VC_CNTL_PRIM_WALK_IND)
      rmesa->store.prim[prim].numverts = rmesa->dma.offset / 64;
   else
      rmesa->store.prim[prim].numverts = rmesa->num_verts;

   rmesa->num_verts = 0;
   rmesa->dma.last = rmesa->dma.low;




   /* Make sure we keep a copy of the initial state.
    */
   if (prim == 0) {
      dirty = RADEON_UPLOAD_CONTEXT_ALL;
      if (rmesa->state.texture.unit[0].texobj) dirty |= RADEON_UPLOAD_TEX0;
      if (rmesa->state.texture.unit[1].texobj) dirty |= RADEON_UPLOAD_TEX1;
   }


   if (dirty)
   {
      GLuint state = rmesa->store.statenr++;

      emit_state( rmesa, &rmesa->store.state[state], dirty );
/*        fprintf(stderr, "emit state %d, dirty %x rmesa->dirty %x\n", */
/*  	      state, dirty, rmesa->state.hw.dirty ); */
      rmesa->store.state[state].dirty = rmesa->state.hw.dirty;	/* override */
      rmesa->store.texture[0][state] = rmesa->state.texture.unit[0].texobj;
      rmesa->store.texture[1][state] = rmesa->state.texture.unit[1].texobj;
      rmesa->state.hw.dirty = 0;
/*        print_state( &rmesa->store.state[state] ); */
   }

   rmesa->store.prim[prim].stateidx = rmesa->store.statenr - 1;

/*     fprintf(stderr, "emit_prim %d hwprim 0x%x vfmt 0x%x %d..%d %d verts stateidx %x\n", */
/*  	   prim, */
/*  	   rmesa->store.prim[prim].prim, */
/*  	   rmesa->store.prim[prim].vc_format, */
/*  	   rmesa->store.prim[prim].start, */
/*  	   rmesa->store.prim[prim].finish, */
/*  	   rmesa->store.prim[prim].numverts, */
/*  	   rmesa->store.prim[prim].stateidx); */
}


void radeonFlushPrimsLocked( radeonContextPtr rmesa )
{
   XF86DRIClipRectPtr pbox = (XF86DRIClipRectPtr)rmesa->pClipRects;
   int nbox = rmesa->numClipRects;
   drmBufPtr buffer = rmesa->dma.buffer;
   RADEONSAREAPrivPtr sarea = rmesa->sarea;
   int fd = rmesa->dri.fd;
   int discard_sz = rmesa->dma.high - rmesa->dma.low < 4096;
   int discard = (rmesa->dma.retained != rmesa->dma.buffer &&
		  discard_sz);
   int i;

   if ( !nbox )
      rmesa->store.primnr = 0;
   else if ( nbox >= RADEON_NR_SAREA_CLIPRECTS ) {
      rmesa->upload_cliprects = 1;
      for ( i = 0 ; i < rmesa->store.statenr ; i++ )
	 rmesa->store.state[0].dirty |= rmesa->store.state[i].dirty;
      if ( !rmesa->store.texture[0][0] )
	 rmesa->store.state[0].dirty &= ~RADEON_UPLOAD_TEX0;
      if ( !rmesa->store.texture[1][0] )
	 rmesa->store.state[0].dirty &= ~RADEON_UPLOAD_TEX1;
   }


/*     fprintf(stderr, "%s: boxes: %d prims: %d states: %d vertexstore: 0x%x\n", */
/*  	   __FUNCTION__, */
/*  	   sarea->nbox, rmesa->store.primnr, rmesa->store.statenr, */
/*  	   rmesa->dma.low - rmesa->store.prim[0].start); */

   if ( !rmesa->upload_cliprects || !rmesa->store.primnr )
   {
      if ( nbox == 1 ) {
	 sarea->nbox = 0;
      } else {
	 sarea->nbox = nbox;
      }

/*        fprintf(stderr, "case a %d boxes %d prims %d states\n", */
/*  	      sarea->nbox, rmesa->store.primnr, rmesa->store.statenr); */
      if (discard || rmesa->store.primnr)
	 drmRadeonFlushPrims( fd, 
			       buffer->idx,
			       discard,
			       rmesa->store.statenr,
			       rmesa->store.state,
			       rmesa->store.primnr,
			       rmesa->store.prim);
   }
   else
   {
      for ( i = 0 ; i < nbox ; ) {
	 int nr = MIN2( i + RADEON_NR_SAREA_CLIPRECTS, nbox );
	 XF86DRIClipRectPtr b = sarea->boxes;
	 int discard_now = 0;

	 /* TODO: Precalculate this intersection:
	  */
	 if ( rmesa->state.scissor.enabled ) {
	    sarea->nbox = 0;

	    for ( ; i < nr ; i++ ) {
	       *b = pbox[i];
	       if ( intersect_rect( b, b, &rmesa->state.scissor.rect ) ) {
		  sarea->nbox++;
		  b++;
	       }
	    }

	    /* Culled?
	     */
	    if ( !sarea->nbox ) {
	       if ( nr < nbox ) continue;
	       rmesa->store.primnr = 0;
	    }
	 } else {
	    sarea->nbox = nr - i;
	    for ( ; i < nr ; i++) {
	       *b++ = pbox[i];
	    }
	 }

	 /* Finished with the buffer?
	  */
	 if ( nr == nbox ) {
	    discard_now = discard;
	 }

/*  	 fprintf(stderr, "case a %d boxes %d prims %d states, discard: %d\n", */
/*  		 sarea->nbox, rmesa->store.primnr, rmesa->store.statenr, discard); */
	 drmRadeonFlushPrims( fd, 
			       buffer->idx,
			       discard_now,
			       rmesa->store.statenr,
			       rmesa->store.state,
			       rmesa->store.primnr,
			       rmesa->store.prim);
      }
   }

   if (discard_sz) {
      rmesa->dma.buffer = 0;
      rmesa->dma.address = 0;
      rmesa->dma.low = 0;
      rmesa->dma.high = 0;
   }
   else {
      rmesa->dma.low = (rmesa->dma.low + 0x7) & ~0x7;  /* alignment */
   }
   rmesa->dma.last = rmesa->dma.low;
   rmesa->store.primnr = 0;
   rmesa->store.statenr = 0;
   rmesa->upload_cliprects = 0;
   rmesa->num_verts = 0;
}

void radeonFlushPrimsGetBuffer( radeonContextPtr rmesa )
{
   if (rmesa->dma.low != rmesa->dma.last)
      emit_prim( rmesa );

   LOCK_HARDWARE(rmesa);

   if (rmesa->dma.buffer) {
      rmesa->dma.low = rmesa->dma.high; /* force discard */
      rmesa->dma.last = rmesa->dma.low;
      radeonFlushPrimsLocked( rmesa );
   }

   rmesa->dma.buffer = radeonGetBufferLocked( rmesa );
   rmesa->dma.high = rmesa->dma.buffer->total;
   rmesa->dma.address = (GLubyte *)rmesa->dma.buffer->address;
   rmesa->dma.low = 0;
   rmesa->num_verts = 0;
   rmesa->dma.last = rmesa->dma.low;
   UNLOCK_HARDWARE(rmesa);
}


void radeonFlushPrims( radeonContextPtr rmesa )
{
   if (rmesa->dma.buffer) {
      if (rmesa->dma.low != rmesa->dma.last)
	 emit_prim( rmesa );

      LOCK_HARDWARE( rmesa );
      radeonFlushPrimsLocked( rmesa );
      UNLOCK_HARDWARE( rmesa );
   }
}

void radeonEmitPrim( radeonContextPtr rmesa )
{
   ASSERT(rmesa->dma.buffer);
   emit_prim( rmesa );

   if (rmesa->store.primnr == RADEON_MAX_PRIMS ||
       rmesa->store.statenr == RADEON_MAX_STATES) {
      LOCK_HARDWARE(rmesa);
      radeonFlushPrimsLocked(rmesa);
      UNLOCK_HARDWARE(rmesa);
   }
   else {
      rmesa->dma.low = (rmesa->dma.low + 0x7) & ~0x7;  /* alignment */
      rmesa->dma.last = rmesa->dma.low;
      rmesa->num_verts = 0;
   }
}


/* ================================================================
 * Texture uploads
 */

void radeonFireBlitLocked( radeonContextPtr rmesa, drmBufPtr buffer,
			   GLint offset, GLint pitch, GLint format,
			   GLint x, GLint y, GLint width, GLint height )
{
#if 0
   GLint ret;

   ret = drmRadeonTextureBlit( rmesa->dri.fd, buffer->idx,
			       offset, pitch, format,
			       x, y, width, height );

   if ( ret ) {
      UNLOCK_HARDWARE( rmesa );
      fprintf( stderr, "drmRadeonTextureBlit: return = %d\n", ret );
      exit( 1 );
   }
#endif
}


/* ================================================================
 * SwapBuffers with client-side throttling
 */

#define RADEON_MAX_OUTSTANDING	2

static void delay( void ) {
/* Prevent an optimizing compiler from removing a spin loop */
}

static int radeonWaitForFrameCompletion( radeonContextPtr rmesa )
{
   unsigned char *RADEONMMIO = rmesa->radeonScreen->mmio.map;
   RADEONSAREAPrivPtr sarea = rmesa->sarea;
   CARD32 frame;
   int wait = 0;
   int i;

   while ( 1 ) {
      frame = INREG( RADEON_LAST_FRAME_REG );
      if ( sarea->last_frame - frame <= RADEON_MAX_OUTSTANDING ) {
	 break;
      }
      wait++;
      /* Spin in place a bit so we aren't hammering the bus */
      for ( i = 0 ; i < 1024 ; i++ ) {
	 delay();
      }
   }

   return wait;
}

/* Copy the back color buffer to the front color buffer.
 */
void radeonCopyBuffer( const __DRIdrawablePrivate *dPriv )
{
   radeonContextPtr rmesa;
   GLint nbox, i, ret;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   rmesa = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "\n%s( %p )\n\n", __FUNCTION__, (void*)rmesa->glCtx );
   }

   RADEON_FIREVERTICES( rmesa );

   LOCK_HARDWARE( rmesa );

   nbox = rmesa->dri.drawable->numClipRects; /* must be in locked region */

   /* Throttle the frame rate -- only allow one pending swap buffers
    * request at a time.
    */
   if ( !radeonWaitForFrameCompletion( rmesa ) ) {
      rmesa->hardwareWentIdle = 1;
   } else {
      rmesa->hardwareWentIdle = 0;
   }

   nbox = dPriv->numClipRects;

   for ( i = 0 ; i < nbox ; ) {
      GLint nr = MIN2( i + RADEON_NR_SAREA_CLIPRECTS , nbox );
      XF86DRIClipRectPtr box = rmesa->dri.drawable->pClipRects;
      XF86DRIClipRectPtr b = rmesa->sarea->boxes;
      GLint n = 0;

      for ( ; i < nr ; i++ ) {
	 *b++ = box[i];
	 n++;
      }
      rmesa->sarea->nbox = n;

      ret = drmRadeonSwapBuffers( rmesa->dri.fd );

      if ( ret ) {
	 fprintf( stderr, "drmRadeonSwapBuffers: return = %d\n", ret );
	 UNLOCK_HARDWARE( rmesa );
	 exit( 1 );
      }
   }

   UNLOCK_HARDWARE( rmesa );

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT_ALL );
   if ( rmesa->state.texture.unit[0].texobj )
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_TEX0 );
   if ( rmesa->state.texture.unit[1].texobj )
      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_TEX1 );


   rmesa->upload_cliprects = 1;

   /* Log the performance counters if necessary */
   radeonPerformanceCounters( rmesa );
}

void radeonPageFlip( const __DRIdrawablePrivate *dPriv )
{
   radeonContextPtr rmesa;
   GLint ret;

   assert(dPriv);
   assert(dPriv->driContextPriv);
   assert(dPriv->driContextPriv->driverPrivate);

   rmesa = (radeonContextPtr) dPriv->driContextPriv->driverPrivate;

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "\n%s( %p ): page=%d\n\n",
	       __FUNCTION__, (void*)rmesa->glCtx, rmesa->currentPage );
   }

   RADEON_FIREVERTICES( rmesa );

   LOCK_HARDWARE( rmesa );

   /* Throttle the frame rate -- only allow one pending swap buffers
    * request at a time.
    */
   if ( !radeonWaitForFrameCompletion( rmesa ) ) {
      rmesa->hardwareWentIdle = 1;
   } else {
      rmesa->hardwareWentIdle = 0;
   }

   /* The kernel will have been initialized to perform page flipping
    * on a swapbuffers ioctl.
    */
   ret = drmRadeonSwapBuffers( rmesa->dri.fd );

   UNLOCK_HARDWARE( rmesa );

   if ( ret ) {
      fprintf( stderr, "drmRadeonSwapBuffers: return = %d\n", ret );
      exit( 1 );
   }

   if ( rmesa->currentPage == 0 ) {
	 rmesa->state.color.drawOffset = rmesa->radeonScreen->frontOffset;
	 rmesa->state.color.drawPitch  = rmesa->radeonScreen->frontPitch;
	 rmesa->currentPage = 1;
   } else {
	 rmesa->state.color.drawOffset = rmesa->radeonScreen->backOffset;
	 rmesa->state.color.drawPitch  = rmesa->radeonScreen->backPitch;
	 rmesa->currentPage = 0;
   }

   RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT );
   rmesa->state.hw.context.rb3d_coloroffset = rmesa->state.color.drawOffset;
   rmesa->state.hw.context.rb3d_colorpitch  = rmesa->state.color.drawPitch;

   /* Log the performance counters if necessary */
   radeonPerformanceCounters( rmesa );
}

void radeonPerformanceCounters( radeonContextPtr rmesa )
{
}

void radeonPerformanceBoxesLocked( radeonContextPtr rmesa )
{
}

/* ================================================================
 * Buffer clear
 */
#define RADEON_MAX_CLEARS	256

static void radeonClear( GLcontext *ctx, GLbitfield mask, GLboolean all,
			 GLint cx, GLint cy, GLint cw, GLint ch )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = rmesa->dri.drawable;
   RADEONSAREAPrivPtr sarea = rmesa->sarea;
   unsigned char *RADEONMMIO = rmesa->radeonScreen->mmio.map;
   CARD32 clear;
   GLuint flags = 0;
   GLuint color_mask = 0;
/*     GLuint depth_mask = 0; */
   GLint ret, i;

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s:  all=%d cx=%d cy=%d cw=%d ch=%d\n",
	       __FUNCTION__, all, cx, cy, cw, ch );
   }

   RADEON_FIREVERTICES( rmesa );

   if ( mask & DD_FRONT_LEFT_BIT ) {
      flags |= RADEON_FRONT;
      color_mask = rmesa->state.hw.mask.rb3d_planemask;
      mask &= ~DD_FRONT_LEFT_BIT;
   }

   if ( mask & DD_BACK_LEFT_BIT ) {
      flags |= RADEON_BACK;
      color_mask = rmesa->state.hw.mask.rb3d_planemask;
      mask &= ~DD_BACK_LEFT_BIT;
   }

   if ( mask & DD_DEPTH_BIT ) {
      if ( ctx->Depth.Mask ) flags |= RADEON_DEPTH; /* FIXME: ??? */
      mask &= ~DD_DEPTH_BIT;
   }

   if ( (mask & DD_STENCIL_BIT) && rmesa->state.stencil.hwBuffer ) {
      flags |= RADEON_STENCIL;
      mask &= ~DD_STENCIL_BIT;
   }

   if ( flags ) {
      /* Flip top to bottom */
      cx += dPriv->x;
      cy  = dPriv->y + dPriv->h - cy - ch;

      LOCK_HARDWARE( rmesa );

      /* Throttle the number of clear ioctls we do.
       */
      while ( 1 ) {
	 clear = INREG( RADEON_LAST_CLEAR_REG );
	 if ( sarea->last_clear - clear <= RADEON_MAX_CLEARS ) {
	    break;
	 }
	 /* Spin in place a bit so we aren't hammering the bus */
	 for ( i = 0 ; i < 1024 ; i++ ) {
	    delay();
	 }
      }

      /* Emit any new MASKS state.  This ioctl uses the old
       * sarea-based state mechanism, which is why I'm not using
       * emit_state() above.  Time for a new ioctl?  
       */
      if ( rmesa->state.hw.dirty ) {
	 memcpy( &sarea->ContextState, &rmesa->state.hw, 
		 sizeof(sarea->ContextState));
	 sarea->dirty |= RADEON_UPLOAD_CONTEXT_ALL;
      }


      for ( i = 0 ; i < dPriv->numClipRects ; ) {
	 GLint nr = MIN2( i + RADEON_NR_SAREA_CLIPRECTS, dPriv->numClipRects );
	 XF86DRIClipRectPtr box = dPriv->pClipRects;
	 XF86DRIClipRectPtr b = rmesa->sarea->boxes;
	 GLint n = 0;

	 if ( !all ) {
	    for ( ; i < nr ; i++ ) {
	       GLint x = box[i].x1;
	       GLint y = box[i].y1;
	       GLint w = box[i].x2 - x;
	       GLint h = box[i].y2 - y;

	       if ( x < cx ) w -= cx - x, x = cx;
	       if ( y < cy ) h -= cy - y, y = cy;
	       if ( x + w > cx + cw ) w = cx + cw - x;
	       if ( y + h > cy + ch ) h = cy + ch - y;
	       if ( w <= 0 ) continue;
	       if ( h <= 0 ) continue;

	       b->x1 = x;
	       b->y1 = y;
	       b->x2 = x + w;
	       b->y2 = y + h;
	       b++;
	       n++;
	    }
	 } else {
	    for ( ; i < nr ; i++ ) {
	       *b++ = box[i];
	       n++;
	    }
	 }

	 rmesa->sarea->nbox = n;

/*  	    fprintf( stderr, */
/*  		     "drmRadeonClear: flag 0x%x color %x depth %x sten %x nbox %d\n", */
/*  		     flags, */
/*  		     rmesa->state.color.clear, */
/*  		     rmesa->state.depth.clear, */
/*  		     rmesa->state.stencil.clear, */
/*  		     rmesa->sarea->nbox ); */

	 ret = drmRadeonClear( rmesa->dri.fd, flags,
			       rmesa->state.color.clear,
			       rmesa->state.depth.clear,
			       rmesa->state.hw.mask.rb3d_planemask,
			       rmesa->state.stencil.clear,
			       rmesa->sarea->boxes, rmesa->sarea->nbox );

	 if ( ret ) {
	    UNLOCK_HARDWARE( rmesa );
	    fprintf( stderr, "drmRadeonClear: return = %d\n", ret );
	    exit( 1 );
	 }
      }

      UNLOCK_HARDWARE( rmesa );

      RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_CONTEXT_ALL );
      if ( rmesa->state.texture.unit[0].texobj )
	 RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_TEX0 );
      if ( rmesa->state.texture.unit[1].texobj )
	 RADEON_STATECHANGE( rmesa, RADEON_UPLOAD_TEX1 );
      rmesa->upload_cliprects = 1;
   }

   if ( mask )
      _swrast_Clear( ctx, mask, all, cx, cy, cw, ch );
}


void radeonWaitForIdleLocked( radeonContextPtr rmesa )
{
    int fd = rmesa->dri.fd;
    int to = 0;
    int ret;

    do {
	ret = drmRadeonWaitForIdleCP( fd );
    } while ( ( ret == -EBUSY ) && ( to++ < RADEON_TIMEOUT ) );

    if ( ret < 0 ) {
	UNLOCK_HARDWARE( rmesa );
	fprintf( stderr, "Error: Radeon timed out... exiting\n" );
	exit( -1 );
    }
}


void radeonInitIoctlFuncs( GLcontext *ctx )
{
    ctx->Driver.Clear = radeonClear;
}



void radeonReleaseRetainedBuffer( radeonContextPtr rmesa )
{
   ASSERT(rmesa->dma.retained);

   if (rmesa->dma.retained &&
       rmesa->dma.retained != rmesa->dma.buffer) {
      RADEON_FIREVERTICES(rmesa); /* FIX ME: dependency tracking for retained */

/*        fprintf(stderr, "releaseRetained: retained %p current %p\n", */
/*  	      rmesa->dma.retained, rmesa->dma.buffer); */
      
      LOCK_HARDWARE(rmesa);
      drmRadeonFlushPrims( rmesa->dri.fd,
			   rmesa->dma.retained->idx, 
			   1,
			   0, rmesa->store.state,
			   0, rmesa->store.prim);
      UNLOCK_HARDWARE(rmesa);
   }

   rmesa->dma.retained = 0;
}
