/* $XFree86: xc/lib/GL/mesa/src/drv/radeon/radeon_tex.c,v 1.1 2001/01/08 01:07:28 martin Exp $ */
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
#include "radeon_vb.h"
#include "radeon_tex.h"

#include "mmath.h"
#include "simple_list.h"
#include "enums.h"
#include "mem.h"


static void radeonSetTexWrap( radeonTexObjPtr t, GLenum swrap, GLenum twrap )
{
   t->setup.pp_txfilter &= ~(RADEON_CLAMP_S_MASK | RADEON_CLAMP_T_MASK);

   switch ( swrap ) {
   case GL_REPEAT:
      t->setup.pp_txfilter |= RADEON_CLAMP_S_WRAP;
      break;
   case GL_CLAMP:
      t->setup.pp_txfilter |= RADEON_CLAMP_S_CLAMP_LAST;
      break;
   case GL_CLAMP_TO_EDGE:
      t->setup.pp_txfilter |= RADEON_CLAMP_S_CLAMP_LAST;
      break;
   }

   switch ( twrap ) {
   case GL_REPEAT:
      t->setup.pp_txfilter |= RADEON_CLAMP_T_WRAP;
      break;
   case GL_CLAMP:
      t->setup.pp_txfilter |= RADEON_CLAMP_T_CLAMP_LAST;
      break;
   case GL_CLAMP_TO_EDGE:
      t->setup.pp_txfilter |= RADEON_CLAMP_T_CLAMP_LAST;
      break;
   }
}

static void radeonSetTexFilter( radeonTexObjPtr t, GLenum minf, GLenum magf )
{
   t->setup.pp_txfilter &= ~(RADEON_MIN_FILTER_MASK | RADEON_MAG_FILTER_MASK);

   switch ( minf ) {
   case GL_NEAREST:
      t->setup.pp_txfilter |= RADEON_MIN_FILTER_NEAREST;
      break;
   case GL_LINEAR:
      t->setup.pp_txfilter |= RADEON_MIN_FILTER_LINEAR;
      break;
   case GL_NEAREST_MIPMAP_NEAREST:
      t->setup.pp_txfilter |= RADEON_MIN_FILTER_NEAREST_MIP_NEAREST;
      break;
   case GL_LINEAR_MIPMAP_NEAREST:
      t->setup.pp_txfilter |= RADEON_MIN_FILTER_LINEAR_MIP_NEAREST;
      break;
   case GL_NEAREST_MIPMAP_LINEAR:
      t->setup.pp_txfilter |= RADEON_MIN_FILTER_NEAREST_MIP_LINEAR;
      break;
   case GL_LINEAR_MIPMAP_LINEAR:
      t->setup.pp_txfilter |= RADEON_MIN_FILTER_LINEAR_MIP_LINEAR;
      break;
   }

   switch ( magf ) {
   case GL_NEAREST:
      t->setup.pp_txfilter |= RADEON_MAG_FILTER_NEAREST;
      break;
   case GL_LINEAR:
      t->setup.pp_txfilter |= RADEON_MAG_FILTER_LINEAR;
      break;
   }
}

static void radeonSetTexBorderColor( radeonTexObjPtr t, GLubyte c[4] )
{
   t->setup.pp_border_color = radeonPackColor( 4, c[0], c[1], c[2], c[3] );
}


/* Allocate and initialize hardware state associated with texture `t'.
 */
static radeonTexObjPtr radeonCreateTexObj( radeonContextPtr rmesa,
					   struct gl_texture_object *tObj )
{
   radeonTexObjPtr t;
   struct gl_texture_image *image;
   GLint log2Width, log2Height, log2Size, log2MinSize;
   GLint totalSize;
   GLint texelsPerDword = 0, blitWidth = 0, blitPitch = 0;
   GLint x, y, width, height;
   GLint i;
   GLuint txformat, txalpha;

   image = tObj->Image[0];
   if ( !image )
      return NULL;

   t = (radeonTexObjPtr) CALLOC( sizeof(*t) );
   if ( !t )
      return NULL;

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API )
      fprintf( stderr, "%s( %p )\n", __FUNCTION__, tObj );

   switch ( image->Format ) {
   case GL_RGBA:
      if ( image->IntFormat != GL_RGBA4 &&
	   ( image->IntFormat == GL_RGBA8 ||
	     rmesa->radeonScreen->cpp == 4 ) ) {
	 t->texelBytes = 4;
	 txformat = RADEON_TXF_32BPP_ARGB8888;
      } else {
	 t->texelBytes = 2;
	 txformat = RADEON_TXF_16BPP_ARGB4444;
      }
      txalpha = RADEON_TXF_ALPHA_IN_MAP;
      break;

   case GL_RGB:
      if ( image->IntFormat != GL_RGB5 &&
	   ( image->IntFormat == GL_RGB8 ||
	     rmesa->radeonScreen->cpp == 4 ) ) {
	 t->texelBytes = 4;
	 txformat = RADEON_TXF_32BPP_ARGB8888;
      } else {
	 t->texelBytes = 2;
	 txformat = RADEON_TXF_16BPP_RGB565;
      }
      txalpha = 0;
      break;

   case GL_ALPHA:
   case GL_LUMINANCE_ALPHA:
      t->texelBytes = 2;
      txformat = RADEON_TXF_16BPP_AI88;
      txalpha = RADEON_TXF_ALPHA_IN_MAP;
      break;

   case GL_LUMINANCE:
      t->texelBytes = 2;
      txformat = RADEON_TXF_16BPP_AI88;
      txalpha = 0;
      break;

   case GL_INTENSITY:
      t->texelBytes = 1;
      txformat = RADEON_TXF_8BPP_I;
      txalpha = 0;
      break;

   case GL_COLOR_INDEX:
   default:
      fprintf( stderr, "%s: bad image->Format\n", __FUNCTION__ );
      FREE( t );
      return NULL;
   }

   /* Calculate dimensions in log domain.
    */
   for ( i = 1, log2Height = 0 ; i < image->Height ; i *= 2 ) {
      log2Height++;
   }
   for ( i = 1, log2Width = 0 ;  i < image->Width ;  i *= 2 ) {
      log2Width++;
   }
   if ( image->Width > image->Height ) {
      log2Size = log2Width;
   } else {
      log2Size = log2Height;
   }

   t->dirty_images = 0;

   /* The Radeon has a 64-byte minimum pitch for all blits.  We
    * calculate the equivalent number of texels to simplify the
    * calculation of the texture image area.
    */
   switch ( t->texelBytes ) {
   case 1:
      texelsPerDword = 4;
      blitPitch = 64;
      break;
   case 2:
      texelsPerDword = 2;
      blitPitch = 32;
      break;
   case 4:
      texelsPerDword = 1;
      blitPitch = 16;
      break;
   }

   /* Select the larger of the two widths for our global texture image
    * coordinate space.  As the Radeon has very strict offset rules, we
    * can't upload mipmaps directly and have to reference their location
    * from the aligned start of the whole image.
    */
   blitWidth = MAX2( image->Width, blitPitch );

   /* Calculate mipmap offsets and dimensions.
    */
   totalSize = 0;
   x = 0;
   y = 0;

   for ( i = 0 ; i <= log2Size ; i++ ) {
      GLuint size;

      image = tObj->Image[i];
      if ( !image )
	 break;

      width = image->Width;
      height = image->Height;

      /* Texture images have a minimum pitch of 32 bytes (half of the
       * 64-byte minimum pitch for blits).  For images that have a
       * width smaller than this, we must pad each texture image
       * scanline out to this amount.
       */
      if ( width < blitPitch / 2 ) {
	 width = blitPitch / 2;
      }

      size = width * height * t->texelBytes;
      totalSize += size;

      t->dirty_images |= (1 << i);

      while ( width < blitWidth && height > 1 ) {
	 width *= 2;
	 height /= 2;
      }

      t->image[i].x = x;
      t->image[i].y = y;

      t->image[i].width  = width;
      t->image[i].height = height;

      t->image[i].dwords = size / sizeof(CARD32);

      /* While blits must have a pitch of at least 64 bytes, mipmaps
       * must be aligned on a 32-byte boundary (just like each texture
       * image scanline).
       */
      if ( width >= blitWidth ) {
	 y += height;
      } else {
	 x += width;
	 if ( x >= blitWidth ) {
	    x = 0;
	    y++;
	 }
      }

      if ( 0 )
	 fprintf( stderr, "level=%d p=%d   %dx%d -> %dx%d at (%d,%d)  %d dwords\n",
		  i, blitWidth, image->Width, image->Height,
		  t->image[i].width, t->image[i].height,
		  t->image[i].x, t->image[i].y,
		  t->image[i].dwords );
   }

   log2MinSize = log2Size - i + 1;

   /* Align the total size of texture memory block.
    */
   totalSize = (totalSize + RADEON_OFFSET_MASK) & ~RADEON_OFFSET_MASK;

   t->totalSize = totalSize;

   t->bound = 0;
   t->heap = 0;
   t->tObj = tObj;

   t->memBlock = NULL;
   t->bufAddr = 0;

   /* Hardware state:
    */
   t->setup.pp_txfilter = ((log2Size << RADEON_MAX_MIP_LEVEL_SHIFT) |
			   RADEON_BORDER_MODE_OGL);

   t->setup.pp_txformat = (txformat | txalpha |
			   (log2Width << RADEON_TXF_WIDTH_SHIFT) |
			   (log2Height << RADEON_TXF_HEIGHT_SHIFT) |
			   RADEON_TXF_ENDIAN_NO_SWAP |
			   RADEON_TXF_PERSPECTIVE_ENABLE);

   t->setup.pp_txoffset		= 0x00000000;
   t->setup.pp_txcblend		= 0x00000000;
   t->setup.pp_txablend		= 0x00000000;
   t->setup.pp_tfactor		= 0x00000000;
   t->setup.pp_border_color	= 0x00000000;

   radeonSetTexWrap( t, tObj->WrapS, tObj->WrapT );
   radeonSetTexFilter( t, tObj->MinFilter, tObj->MagFilter );
   radeonSetTexBorderColor( t, tObj->BorderColor );

   tObj->DriverData = t;

   make_empty_list( t );

   return t;
}

/* Destroy hardware state associated with texture `t'.
 */
void radeonDestroyTexObj( radeonContextPtr rmesa, radeonTexObjPtr t )
{
#if ENABLE_PERF_BOXES
   /* Bump the performace counter */
   rmesa->c_textureSwaps++;
#endif
   if ( !t ) return;

   if ( t->memBlock ) {
      mmFreeMem( t->memBlock );
      t->memBlock = NULL;
   }

   if ( t->tObj )
      t->tObj->DriverData = NULL;
   if ( t->bound )
      rmesa->CurrentTexObj[t->bound-1] = NULL;

   remove_from_list( t );
   FREE( t );
}

/* Keep track of swapped out texture objects.
 */
static void radeonSwapOutTexObj( radeonContextPtr rmesa, radeonTexObjPtr t )
{
#if ENABLE_PERF_BOXES
   /* Bump the performace counter */
   rmesa->c_textureSwaps++;
#endif
   if ( t->memBlock ) {
      mmFreeMem( t->memBlock );
      t->memBlock = NULL;
   }

   t->dirty_images = ~0;
   move_to_tail( &rmesa->SwappedOut, t );
}

/* Print out debugging information about texture LRU.
 */
void radeonPrintLocalLRU( radeonContextPtr rmesa, int heap )
{
   radeonTexObjPtr t;
   int sz = 1 << (rmesa->radeonScreen->logTexGranularity[heap]);

   fprintf( stderr, "\nLocal LRU, heap %d:\n", heap );

   foreach ( t, &rmesa->TexObjList[heap] ) {
      if (!t->tObj) {
	 fprintf( stderr, "Placeholder %d at 0x%x sz 0x%x\n",
		  t->memBlock->ofs / sz,
		  t->memBlock->ofs,
		  t->memBlock->size );
      } else {
	 fprintf( stderr, "Texture (bound %d) at 0x%x sz 0x%x\n",
		  t->bound,
		  t->memBlock->ofs,
		  t->memBlock->size );
      }
   }

   fprintf( stderr, "\n" );
}

void radeonPrintGlobalLRU( radeonContextPtr rmesa, int heap )
{
   radeon_tex_region_t *list = rmesa->sarea->texList[heap];
   int i, j;

   fprintf( stderr, "\nGlobal LRU, heap %d list %p:\n", heap, list );

   for ( i = 0, j = RADEON_NR_TEX_REGIONS ; i < RADEON_NR_TEX_REGIONS ; i++ ) {
      fprintf( stderr, "list[%d] age %d next %d prev %d\n",
	       j, list[j].age, list[j].next, list[j].prev );
      j = list[j].next;
      if ( j == RADEON_NR_TEX_REGIONS ) break;
   }

   if ( j != RADEON_NR_TEX_REGIONS ) {
      fprintf( stderr, "Loop detected in global LRU\n" );
      for ( i = 0 ; i < RADEON_NR_TEX_REGIONS ; i++ ) {
	 fprintf( stderr, "list[%d] age %d next %d prev %d\n",
		  i, list[i].age, list[i].next, list[i].prev );
      }
   }

   fprintf( stderr, "\n" );
}

/* Reset the global texture LRU.
 */
static void radeonResetGlobalLRU( radeonContextPtr rmesa, int heap )
{
   radeon_tex_region_t *list = rmesa->sarea->texList[heap];
   int sz = 1 << rmesa->radeonScreen->logTexGranularity[heap];
   int i;

   /*
    * (Re)initialize the global circular LRU list.  The last element in
    * the array (RADEON_NR_TEX_REGIONS) is the sentinal.  Keeping it at
    * the end of the array allows it to be addressed rationally when
    * looking up objects at a particular location in texture memory.
    */
   for ( i = 0 ; (i+1) * sz <= rmesa->radeonScreen->texSize[heap] ; i++ ) {
      list[i].prev = i-1;
      list[i].next = i+1;
      list[i].age = 0;
   }

   i--;
   list[0].prev = RADEON_NR_TEX_REGIONS;
   list[i].prev = i-1;
   list[i].next = RADEON_NR_TEX_REGIONS;
   list[RADEON_NR_TEX_REGIONS].prev = i;
   list[RADEON_NR_TEX_REGIONS].next = 0;
   rmesa->sarea->texAge[heap] = 0;
}

/* Update the local and glock texture LRUs.
 */
static void radeonUpdateTexLRU(radeonContextPtr rmesa, radeonTexObjPtr t )
{
   int heap = t->heap;
   radeon_tex_region_t *list = rmesa->sarea->texList[heap];
   int sz = rmesa->radeonScreen->logTexGranularity[heap];
   int start = t->memBlock->ofs >> sz;
   int end = (t->memBlock->ofs + t->memBlock->size-1) >> sz;
   int i;

   rmesa->lastTexAge[heap] = ++rmesa->sarea->texAge[heap];

   if ( !t->memBlock ) {
      fprintf( stderr, "no memblock\n\n" );
      return;
   }

   /* Update our local LRU */
   move_to_head( &rmesa->TexObjList[heap], t );

   /* Update the global LRU */
   for ( i = start ; i <= end ; i++ ) {
      list[i].in_use = 1;
      list[i].age = rmesa->lastTexAge[heap];

      /* remove_from_list(i) */
      list[(CARD32)list[i].next].prev = list[i].prev;
      list[(CARD32)list[i].prev].next = list[i].next;

      /* insert_at_head(list, i) */
      list[i].prev = RADEON_NR_TEX_REGIONS;
      list[i].next = list[RADEON_NR_TEX_REGIONS].next;
      list[(CARD32)list[RADEON_NR_TEX_REGIONS].next].prev = i;
      list[RADEON_NR_TEX_REGIONS].next = i;
   }

   if ( 0 ) {
      radeonPrintGlobalLRU( rmesa, t->heap );
      radeonPrintLocalLRU( rmesa, t->heap );
   }
}

/* Update our notion of what textures have been changed since we last
 * held the lock.  This pertains to both our local textures and the
 * textures belonging to other clients.  Keep track of other client's
 * textures by pushing a placeholder texture onto the LRU list -- these
 * are denoted by (tObj == NULL).
 */
static void radeonTexturesGone( radeonContextPtr rmesa, int heap,
				int offset, int size, int in_use )
{
   radeonTexObjPtr t, tmp;

   foreach_s ( t, tmp, &rmesa->TexObjList[heap] ) {
      if ( t->memBlock->ofs >= offset + size ||
	   t->memBlock->ofs + t->memBlock->size <= offset )
	 continue;

      /* It overlaps - kick it out.  Need to hold onto the currently
       * bound objects, however.
       */
      if ( t->bound ) {
	 radeonSwapOutTexObj( rmesa, t );
      } else {
	 radeonDestroyTexObj( rmesa, t );
      }
   }

   if ( in_use ) {
      t = (radeonTexObjPtr) CALLOC( sizeof(*t) );
      if ( !t ) return;

      t->memBlock = mmAllocMem( rmesa->texHeap[heap], size, 0, offset );
      if ( !t->memBlock ) {
	 fprintf( stderr, "Couldn't alloc placeholder sz %x ofs %x\n",
		  (int)size, (int)offset );
	 mmDumpMemInfo( rmesa->texHeap[heap] );
	 return;
      }
      insert_at_head( &rmesa->TexObjList[heap], t );
   }
}

/* Update our client's shared texture state.  If another client has
 * modified a region in which we have textures, then we need to figure
 * out which of our textures has been removed, and update our global
 * LRU.
 */
void radeonAgeTextures( radeonContextPtr rmesa, int heap )
{
   RADEONSAREAPrivPtr sarea = rmesa->sarea;

   if ( sarea->texAge[heap] != rmesa->lastTexAge[heap] ) {
      int sz = 1 << rmesa->radeonScreen->logTexGranularity[heap];
      int nr = 0;
      int idx;

      for ( idx = sarea->texList[heap][RADEON_NR_TEX_REGIONS].prev ;
	    idx != RADEON_NR_TEX_REGIONS && nr < RADEON_NR_TEX_REGIONS ;
	    idx = sarea->texList[heap][idx].prev, nr++ )
      {
	 /* If switching texturing schemes, then the SAREA might not
	  * have been properly cleared, so we need to reset the
	  * global texture LRU.
	  */
	 if ( idx * sz > rmesa->radeonScreen->texSize[heap] ) {
	    nr = RADEON_NR_TEX_REGIONS;
	    break;
	 }

	 if ( sarea->texList[heap][idx].age > rmesa->lastTexAge[heap] ) {
	    radeonTexturesGone( rmesa, heap, idx * sz, sz,
				sarea->texList[heap][idx].in_use );
	 }
      }

      if ( nr == RADEON_NR_TEX_REGIONS ) {
	 radeonTexturesGone( rmesa, heap, 0,
			     rmesa->radeonScreen->texSize[heap], 0 );
	 radeonResetGlobalLRU( rmesa, heap );
      }

      rmesa->dirty |= (RADEON_UPLOAD_CONTEXT |
		       RADEON_UPLOAD_TEX0IMAGES |
		       RADEON_UPLOAD_TEX1IMAGES);
      rmesa->lastTexAge[heap] = sarea->texAge[heap];
   }
}


/* ================================================================
 * Texture image conversions
 */

/* Convert a block of Mesa-formatted texture to an 8bpp hardware format.
 */
static void radeonConvertTexture8bpp( CARD32 *dst,
				      struct gl_texture_image *image,
				      int x, int y, int width, int height,
				      int pitch )
{
   CARD8 *src;
   int i, j;

   if ( width < 4 ) {
      width = 4;
   }

#define ALIGN_DST							\
   do {									\
      if ( width < 32 ) {						\
	 dst += ((32 - width) / 4);					\
      }									\
   } while (0)

   switch ( image->Format ) {
   case GL_INTENSITY:
      for ( i = 0 ; i < height ; i++ ) {
	 src = (CARD8 *)image->Data + ((y + i) * pitch + x);
	 for ( j = width >> 2 ; j ; j-- ) {
	    *dst++ = src[0] | (src[1] << 8) | (src[2] << 16) | (src[3] << 24);
	    src += 4;
	 }
	 ALIGN_DST;
      }
      break;

   case GL_COLOR_INDEX:
   default:
      fprintf( stderr, "%s: unsupported format 0x%x\n",
	       __FUNCTION__, image->Format );
      break;
   }
}
#undef ALIGN_DST

/* Convert a block of Mesa-formatted texture to a 16bpp hardware format.
 */
static void radeonConvertTexture16bpp( CARD32 *dst,
				       struct gl_texture_image *image,
				       int x, int y, int width, int height,
				       int pitch )
{
   CARD8 *src;
   int i, j;

   if ( width < 2 ) {
      width = 2;
   }

#define ALIGN_DST							\
   do {									\
      if ( width < 16 ) {						\
	 dst += ((16 - width) / 2);					\
      }									\
   } while (0)

   switch ( image->Format ) {
   case GL_RGBA:
      for ( i = 0 ; i < height ; i++ ) {
	 src = (CARD8 *)image->Data + ((y + i) * pitch + x) * 4;
	 for ( j = width >> 1 ; j ; j-- ) {
	    *dst++ = ((RADEONPACKCOLOR4444( src[0], src[1], src[2], src[3] )) |
		      (RADEONPACKCOLOR4444( src[4], src[5], src[6], src[7] ) << 16));
	    src += 8;
	 }
	 ALIGN_DST;
      }
      break;

   case GL_RGB:
   {
      for ( i = 0 ; i < height ; i++ ) {
	 src = (CARD8 *)image->Data + ((y + i) * pitch + x) * 3;
	 for ( j = width >> 1 ; j ; j-- ) {
	    *dst++ = ((RADEONPACKCOLOR565( src[0], src[1], src[2] )) |
		      (RADEONPACKCOLOR565( src[3], src[4], src[5] ) << 16));
	    src += 6;
	 }
	 ALIGN_DST;
      }
   }
      break;

   case GL_ALPHA:
      for ( i = 0 ; i < height ; i++ ) {
	 src = (CARD8 *)image->Data + ((y + i) * pitch + x);
	 for ( j = width >> 1 ; j ; j-- ) {
	    *dst++ = ((RADEONPACKCOLOR88( 0x00, src[0] )) |
		      (RADEONPACKCOLOR88( 0x00, src[1] ) << 16));
	    src += 2;
	 }
	 ALIGN_DST;
      }
      break;

   case GL_LUMINANCE:
      for ( i = 0 ; i < height ; i++ ) {
	 src = (CARD8 *)image->Data + ((y + i) * pitch + x);
	 for ( j = width >> 1 ; j ; j-- ) {
	    *dst++ = ((RADEONPACKCOLOR88( src[0], 0xff )) |
		      (RADEONPACKCOLOR88( src[1], 0xff ) << 16));
	    src += 2;
	 }
	 ALIGN_DST;
      }
      break;

   case GL_LUMINANCE_ALPHA:
      for ( i = 0 ; i < height ; i++ ) {
	 src = (CARD8 *)image->Data + ((y + i) * pitch + x) * 2;
	 for ( j = width >> 1 ; j ; j-- ) {
	    *dst++ = ((RADEONPACKCOLOR88( src[0], src[1] )) |
		      (RADEONPACKCOLOR88( src[2], src[3] ) << 16));
	    src += 4;
	 }
	 ALIGN_DST;
      }
      break;

   default:
      fprintf( stderr, "%s: unsupported format 0x%x\n",
	       __FUNCTION__, image->Format );
      break;
   }
}
#undef ALIGN_DST

/* Convert a block of Mesa-formatted texture to a 32bpp hardware format.
 */
static void radeonConvertTexture32bpp( CARD32 *dst,
				       struct gl_texture_image *image,
				       int x, int y, int width, int height,
				       int pitch )
{
   CARD8 *src;
   int i, j;

#define ALIGN_DST							\
   do {									\
      if ( width < 8 ) {						\
	 dst += (8 - width);						\
      }									\
   } while (0)

   switch ( image->Format ) {
   case GL_RGBA:
      for ( i = 0 ; i < height ; i++ ) {
	 src = (CARD8 *)image->Data + ((y + i) * pitch + x) * 4;
	 for ( j = width ; j ; j-- ) {
	    *dst++ = RADEONPACKCOLOR8888( src[0], src[1], src[2], src[3] );
	    src += 4;
	 }
	 ALIGN_DST;
      }
      break;

   case GL_RGB:
      for ( i = 0 ; i < height ; i++ ) {
	 src = (CARD8 *)image->Data + ((y + i) * pitch + x) * 3;
	 for ( j = width ; j ; j-- ) {
	    *dst++ = RADEONPACKCOLOR8888( src[0], src[1], src[2], 0xff );
	    src += 3;
	 }
	 ALIGN_DST;
      }
      break;

   default:
      fprintf( stderr, "%s: unsupported format 0x%x\n",
	       __FUNCTION__, image->Format );
      break;
   }
}

/* Upload the texture image associated with texture `t' at level `level'
 * at the address relative to `start'.
 */
static void radeonUploadSubImage( radeonContextPtr rmesa,
				  radeonTexObjPtr t, GLint level,
				  GLint x, GLint y, GLint width, GLint height )
{
   struct gl_texture_image *image;
   GLint texelsPerDword = 0;
   GLint imageX, imageY, imageWidth, imageHeight;
   GLint blitX, blitY, blitWidth, blitHeight;
   GLint imageRows, blitRows;
   GLint remaining ;
   GLint format, dwords;
   CARD32 pitch, offset;
   drmBufPtr buffer;
   CARD32 *dst;

   /* Ensure we have a valid texture to upload */
   if ( ( level < 0 ) || ( level >= RADEON_MAX_TEXTURE_LEVELS ) )
      return;

   image = t->tObj->Image[level];
   if ( !image )
      return;

   switch ( t->texelBytes ) {
   case 1:
      texelsPerDword = 4;
      break;
   case 2:
      texelsPerDword = 2;
      break;
   case 4:
      texelsPerDword = 1;
      break;
   }

   format = t->setup.pp_txformat & RADEON_TXF_FORMAT_MASK;

   imageX = 0;
   imageY = 0;
   imageWidth = image->Width;
   imageHeight = image->Height;

   blitX = t->image[level].x;
   blitY = t->image[level].y;
   blitWidth = t->image[level].width;
   blitHeight = t->image[level].height;

   dwords = t->image[level].dwords;
   offset = t->bufAddr;
   pitch = (t->image[0].width * t->texelBytes) / 64;

#if ENABLE_PERF_BOXES
   /* Bump the performace counter */
   rmesa->c_textureBytes += (dwords << 2);
#endif

   if ( RADEON_DEBUG & DEBUG_VERBOSE_MSG ) {
      fprintf( stderr, "   upload image: %d,%d at %d,%d\n",
	       imageWidth, imageHeight, imageX, imageY );
      fprintf( stderr, "   upload  blit: %d,%d at %d,%d\n",
	       blitWidth, blitHeight, blitX, blitY );
      fprintf( stderr, "       blit ofs: 0x%07x pitch: 0x%x dwords: %d "
	       "level: %d format: %x\n",
	       (GLuint)offset, (GLuint)pitch, dwords, level, format );
   }

   /* Subdivide the texture if required */
   if ( dwords <= RADEON_BUFFER_MAX_DWORDS / 2 ) {
      imageRows = imageHeight;
      blitRows = blitHeight;
   } else {
      imageRows = (RADEON_BUFFER_MAX_DWORDS * texelsPerDword)/(2 * imageWidth);
      blitRows = (RADEON_BUFFER_MAX_DWORDS * texelsPerDword) / (2 * blitWidth);
   }

   for ( remaining = imageHeight ;
	 remaining > 0 ;
	 remaining -= imageRows, imageY += imageRows, blitY += blitRows )
   {
      if ( remaining >= imageRows ) {
	 imageHeight = imageRows;
	 blitHeight = blitRows;
      } else {
	 imageHeight = remaining;
	 blitHeight = blitRows;
      }
      dwords = blitWidth * blitHeight / texelsPerDword;

      if ( RADEON_DEBUG & DEBUG_VERBOSE_MSG ) {
	 fprintf( stderr, "       blitting: %d,%d at %d,%d\n",
		  imageWidth, imageHeight, imageX, imageY );
	 fprintf( stderr, "                 %d,%d at %d,%d - %d dwords\n",
		  blitWidth, blitHeight, blitX, blitY, dwords );
      }

      /* Grab the indirect buffer for the texture blit */
      buffer = radeonGetBufferLocked( rmesa );

      dst = (CARD32 *)((char *)buffer->address + RADEON_HOSTDATA_BLIT_OFFSET);

      /* Actually do the texture conversion */
      switch ( t->texelBytes ) {
      case 1:
	 radeonConvertTexture8bpp( dst, image,
				   imageX, imageY, imageWidth, imageHeight,
				   imageWidth );
	 break;
      case 2:
	 radeonConvertTexture16bpp( dst, image,
				    imageX, imageY, imageWidth, imageHeight,
				    imageWidth );
	 break;
      case 4:
	 radeonConvertTexture32bpp( dst, image,
				    imageX, imageY, imageWidth, imageHeight,
				    imageWidth );
	 break;
      }

      radeonFireBlitLocked( rmesa, buffer,
			    offset, pitch, format,
			    blitX, blitY, blitWidth, blitHeight );
   }

   rmesa->new_state |= RADEON_NEW_CONTEXT;
   rmesa->dirty |= RADEON_UPLOAD_CONTEXT | RADEON_UPLOAD_MASKS;
}

/* Upload the texture images associated with texture `t'.  This might
 * require removing our own and/or other client's texture objects to
 * make room for these images.
 */
/* NOTE: This function is only called while holding the hardware lock */
int radeonUploadTexImages( radeonContextPtr rmesa, radeonTexObjPtr t )
{
   int i;
   int heap;

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %p, %p )\n",
	       __FUNCTION__, rmesa->glCtx, t );
   }

   if ( !t )
      return 0;

   /* Choose the heap appropriately */
   heap = t->heap = RADEON_CARD_HEAP;
#if 0
   if ( !rmesa->radeonScreen->IsPCI &&
	t->totalSize > rmesa->radeonScreen->texSize[heap] ) {
      heap = t->heap = RADEON_AGP_HEAP;
   }
#endif

   /* Do we need to eject LRU texture objects? */
   if ( !t->memBlock ) {
      /* Allocate a memory block on a 4k boundary (1<<12 == 4096) */
      t->memBlock = mmAllocMem( rmesa->texHeap[heap],
				t->totalSize, 12, 0 );

#if 0
      /* Try AGP before kicking anything out of local mem */
      if ( !t->memBlock && heap == RADEON_CARD_HEAP ) {
	 t->memBlock = mmAllocMem( rmesa->texHeap[RADEON_AGP_HEAP],
				   t->totalSize, 12, 0 );

	 if ( t->memBlock )
	    heap = t->heap = RADEON_AGP_HEAP;
      }
#endif

      /* Kick out textures until the requested texture fits */
      while ( !t->memBlock ) {
	 if ( rmesa->TexObjList[heap].prev->bound ) {
	    fprintf( stderr,
		     "radeonUploadTexImages: ran into bound texture\n" );
	    return -1;
	 }
	 if ( rmesa->TexObjList[heap].prev ==
	      &rmesa->TexObjList[heap] ) {
	    if ( rmesa->radeonScreen->IsPCI ) {
	       fprintf( stderr, "radeonUploadTexImages: upload texture "
			"failure on local texture heaps, sz=%d\n",
			t->totalSize );
	       return -1;
#if 0
	    } else if ( heap == RADEON_CARD_HEAP ) {
	       heap = t->heap = RADEON_AGP_HEAP;
	       continue;
#endif
	    } else {
	       fprintf( stderr, "radeonUploadTexImages: upload texture "
			"failure on both local and AGP texture heaps, "
			"sz=%d\n",
			t->totalSize );
	       return -1;
	    }
	 }

	 radeonDestroyTexObj( rmesa, rmesa->TexObjList[heap].prev );

	 t->memBlock = mmAllocMem( rmesa->texHeap[heap],
				   t->totalSize, 12, 0 );
      }

      /* Set the base offset of the texture image */
      t->bufAddr = rmesa->radeonScreen->texOffset[heap] + t->memBlock->ofs;

      t->setup.pp_txoffset = t->bufAddr;
#if 0
      /* Fix AGP texture offsets */
      if ( heap == RADEON_AGP_HEAP ) {
	  t->setup.pp_tx_offset += RADEON_AGP_TEX_OFFSET +
	      rmesa->radeonScreen->agpTexOffset;
      }
#endif

      /* Force loading the new state into the hardware */
      switch ( t->bound ) {
      case 1:
	 rmesa->dirty |= RADEON_UPLOAD_CONTEXT | RADEON_UPLOAD_TEX0;
	 break;

      case 2:
	 rmesa->dirty |= RADEON_UPLOAD_CONTEXT | RADEON_UPLOAD_TEX1;
	 break;

      default:
	 return -1;
      }
   }

   /* Let the world know we've used this memory recently */
   radeonUpdateTexLRU( rmesa, t );

   /* Upload any images that are new */
   if ( t->dirty_images ) {
      int num_levels = ((t->setup.pp_txfilter & RADEON_MAX_MIP_LEVEL_MASK) >>
			RADEON_MAX_MIP_LEVEL_SHIFT);

      for ( i = 0 ; i <= num_levels ; i++ ) {
	 if ( t->dirty_images & (1 << i) ) {
	    radeonUploadSubImage( rmesa, t, i, 0, 0,
				  t->image[i].width, t->image[i].height );
	 }
      }

      rmesa->dirty |= RADEON_UPLOAD_CONTEXT;
   }

   t->dirty_images = 0;
   return 0;
}


/* ================================================================
 * Texture combine functions
 */

#define RADEON_DISABLE		0
#define RADEON_REPLACE		1
#define RADEON_MODULATE		2
#define RADEON_DECAL		3
#define RADEON_BLEND		4
#define RADEON_ADD		5
#define RADEON_MAX_COMBFUNC	6

static GLuint radeon_color_combine[][RADEON_MAX_COMBFUNC] =
{
   /* Unit 0:
    */
   {
      /* Disable combiner stage
       */
      (RADEON_COLOR_ARG_A_ZERO |
       RADEON_COLOR_ARG_B_ZERO |
       RADEON_COLOR_ARG_C_CURRENT_COLOR |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_REPLACE = 0x00802800
       */
      (RADEON_COLOR_ARG_A_ZERO |
       RADEON_COLOR_ARG_B_ZERO |
       RADEON_COLOR_ARG_C_T0_COLOR |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_MODULATE = 0x00800142
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_T0_COLOR |
       RADEON_COLOR_ARG_C_ZERO |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_DECAL = 0x008c2d42
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_T0_COLOR |
       RADEON_COLOR_ARG_C_T0_ALPHA |
       RADEON_BLEND_CTL_BLEND |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_BLEND = 0x008c2902
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_TFACTOR_COLOR |
       RADEON_COLOR_ARG_C_T0_COLOR |
       RADEON_BLEND_CTL_BLEND |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_ADD = 0x00812802
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_ZERO |
       RADEON_COLOR_ARG_C_T0_COLOR |
       RADEON_COMP_ARG_B |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),
   },

   /* Unit 1:
    */
   {
      /* Disable combiner stage
       */
      (RADEON_COLOR_ARG_A_ZERO |
       RADEON_COLOR_ARG_B_ZERO |
       RADEON_COLOR_ARG_C_CURRENT_COLOR |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_REPLACE = 0x00803000
       */
      (RADEON_COLOR_ARG_A_ZERO |
       RADEON_COLOR_ARG_B_ZERO |
       RADEON_COLOR_ARG_C_T1_COLOR |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_MODULATE = 0x00800182
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_T1_COLOR |
       RADEON_COLOR_ARG_C_ZERO |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_DECAL = 0x008c3582
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_T1_COLOR |
       RADEON_COLOR_ARG_C_T1_ALPHA |
       RADEON_BLEND_CTL_BLEND |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_BLEND = 0x008c3102
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_TFACTOR_COLOR |
       RADEON_COLOR_ARG_C_T1_COLOR |
       RADEON_BLEND_CTL_BLEND |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_ADD = 0x00813002
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_ZERO |
       RADEON_COLOR_ARG_C_T1_COLOR |
       RADEON_COMP_ARG_B |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),
   },

   /* Unit 2:
    */
   {
      /* Disable combiner stage
       */
      (RADEON_COLOR_ARG_A_ZERO |
       RADEON_COLOR_ARG_B_ZERO |
       RADEON_COLOR_ARG_C_CURRENT_COLOR |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_REPLACE = 0x00803800
       */
      (RADEON_COLOR_ARG_A_ZERO |
       RADEON_COLOR_ARG_B_ZERO |
       RADEON_COLOR_ARG_C_T2_COLOR |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_MODULATE = 0x008001c2
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_T2_COLOR |
       RADEON_COLOR_ARG_C_ZERO |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_DECAL = 0x008c3dc2
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_T2_COLOR |
       RADEON_COLOR_ARG_C_T2_ALPHA |
       RADEON_BLEND_CTL_BLEND |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_BLEND = 0x008c3902
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_TFACTOR_COLOR |
       RADEON_COLOR_ARG_C_T2_COLOR |
       RADEON_BLEND_CTL_BLEND |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_ADD = 0x00813802
       */
      (RADEON_COLOR_ARG_A_CURRENT_COLOR |
       RADEON_COLOR_ARG_B_ZERO |
       RADEON_COLOR_ARG_C_T2_COLOR |
       RADEON_COMP_ARG_B |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),
   }
};

static GLuint radeon_alpha_combine[][RADEON_MAX_COMBFUNC] =
{
   /* Unit 0:
    */
   {
      /* Disable combiner stage
       */
      (RADEON_ALPHA_ARG_A_ZERO |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_CURRENT_ALPHA |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_REPLACE = 0x00800500
       */
      (RADEON_ALPHA_ARG_A_ZERO |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_T0_ALPHA |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_MODULATE = 0x00800051
       */
      (RADEON_ALPHA_ARG_A_CURRENT_ALPHA |
       RADEON_ALPHA_ARG_B_T0_ALPHA |
       RADEON_ALPHA_ARG_C_ZERO |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_DECAL = 0x00800100
       */
      (RADEON_ALPHA_ARG_A_ZERO |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_CURRENT_ALPHA |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_BLEND = 0x00800051
       */
      (RADEON_ALPHA_ARG_A_CURRENT_ALPHA |
       RADEON_ALPHA_ARG_B_TFACTOR_ALPHA |
       RADEON_ALPHA_ARG_C_T0_ALPHA |
       RADEON_BLEND_CTL_BLEND |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_ADD = 0x00800051
       */
      (RADEON_ALPHA_ARG_A_CURRENT_ALPHA |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_T0_ALPHA |
       RADEON_COMP_ARG_B |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),
   },

   /* Unit 1:
    */
   {
      /* Disable combiner stage
       */
      (RADEON_ALPHA_ARG_A_ZERO |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_CURRENT_ALPHA |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_REPLACE = 0x00800600
       */
      (RADEON_ALPHA_ARG_A_ZERO |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_T1_ALPHA |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_MODULATE = 0x00800061
       */
      (RADEON_ALPHA_ARG_A_CURRENT_ALPHA |
       RADEON_ALPHA_ARG_B_T1_ALPHA |
       RADEON_ALPHA_ARG_C_ZERO |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_DECAL = 0x00800100
       */
      (RADEON_ALPHA_ARG_A_ZERO |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_CURRENT_ALPHA |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_BLEND = 0x00800061
       */
      (RADEON_ALPHA_ARG_A_CURRENT_ALPHA |
       RADEON_ALPHA_ARG_B_TFACTOR_ALPHA |
       RADEON_ALPHA_ARG_C_T1_ALPHA |
       RADEON_BLEND_CTL_BLEND |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_ADD = 0x00800061
       */
      (RADEON_ALPHA_ARG_A_CURRENT_ALPHA |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_T1_ALPHA |
       RADEON_COMP_ARG_B |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),
   },

   /* Unit 2:
    */
   {
      /* Disable combiner stage
       */
      (RADEON_ALPHA_ARG_A_ZERO |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_CURRENT_ALPHA |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_REPLACE = 0x00800700
       */
      (RADEON_ALPHA_ARG_A_ZERO |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_T2_ALPHA |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_MODULATE = 0x00800071
       */
      (RADEON_ALPHA_ARG_A_CURRENT_ALPHA |
       RADEON_ALPHA_ARG_B_T2_ALPHA |
       RADEON_ALPHA_ARG_C_ZERO |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_DECAL = 0x00800100
       */
      (RADEON_ALPHA_ARG_A_ZERO |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_CURRENT_ALPHA |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_BLEND = 0x00800071
       */
      (RADEON_ALPHA_ARG_A_CURRENT_ALPHA |
       RADEON_ALPHA_ARG_B_TFACTOR_ALPHA |
       RADEON_ALPHA_ARG_C_T2_ALPHA |
       RADEON_BLEND_CTL_BLEND |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),

      /* GL_ADD = 0x00800021
       */
      (RADEON_ALPHA_ARG_A_CURRENT_ALPHA |
       RADEON_ALPHA_ARG_B_ZERO |
       RADEON_ALPHA_ARG_C_T2_ALPHA |
       RADEON_COMP_ARG_B |
       RADEON_BLEND_CTL_ADD |
       RADEON_SCALE_1X |
       RADEON_CLAMP_TX),
   }
};


/* GL_EXT_texture_env_combine support
 */

/* The color tables have combine functions for GL_SRC_COLOR,
 * GL_ONE_MINUS_SRC_COLOR, GL_SRC_ALPHA and GL_ONE_MINUS_SRC_ALPHA.
 */
static GLuint radeon_texture_color[][RADEON_MAX_TEXTURE_UNITS] =
{
   {
      RADEON_COLOR_ARG_A_T0_COLOR,
      RADEON_COLOR_ARG_A_T1_COLOR,
      RADEON_COLOR_ARG_A_T2_COLOR
   },
   {
      RADEON_COLOR_ARG_A_T0_COLOR | RADEON_COMP_ARG_A,
      RADEON_COLOR_ARG_A_T1_COLOR | RADEON_COMP_ARG_A,
      RADEON_COLOR_ARG_A_T2_COLOR | RADEON_COMP_ARG_A
   },
   {
      RADEON_COLOR_ARG_A_T0_ALPHA,
      RADEON_COLOR_ARG_A_T1_ALPHA,
      RADEON_COLOR_ARG_A_T2_ALPHA
   },
   {
      RADEON_COLOR_ARG_A_T0_ALPHA | RADEON_COMP_ARG_A,
      RADEON_COLOR_ARG_A_T1_ALPHA | RADEON_COMP_ARG_A,
      RADEON_COLOR_ARG_A_T2_ALPHA | RADEON_COMP_ARG_A
   },
};

static GLuint radeon_tfactor_color[] =
{
   RADEON_COLOR_ARG_A_TFACTOR_COLOR,
   RADEON_COLOR_ARG_A_TFACTOR_COLOR | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_TFACTOR_ALPHA,
   RADEON_COLOR_ARG_A_TFACTOR_ALPHA | RADEON_COMP_ARG_A
};

static GLuint radeon_primary_color[] =
{
   RADEON_COLOR_ARG_A_DIFFUSE_COLOR,
   RADEON_COLOR_ARG_A_DIFFUSE_COLOR | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_DIFFUSE_ALPHA,
   RADEON_COLOR_ARG_A_DIFFUSE_ALPHA | RADEON_COMP_ARG_A
};

static GLuint radeon_previous_color[] =
{
   RADEON_COLOR_ARG_A_CURRENT_COLOR,
   RADEON_COLOR_ARG_A_CURRENT_COLOR | RADEON_COMP_ARG_A,
   RADEON_COLOR_ARG_A_CURRENT_ALPHA,
   RADEON_COLOR_ARG_A_CURRENT_ALPHA | RADEON_COMP_ARG_A
};

/* The alpha tables only have GL_SRC_ALPHA and GL_ONE_MINUS_SRC_ALPHA.
 */
static GLuint radeon_texture_alpha[][RADEON_MAX_TEXTURE_UNITS] =
{
   {
      RADEON_ALPHA_ARG_A_T0_ALPHA,
      RADEON_ALPHA_ARG_A_T1_ALPHA,
      RADEON_ALPHA_ARG_A_T2_ALPHA
   },
   {
      RADEON_ALPHA_ARG_A_T0_ALPHA | RADEON_COMP_ARG_A,
      RADEON_ALPHA_ARG_A_T1_ALPHA | RADEON_COMP_ARG_A,
      RADEON_ALPHA_ARG_A_T2_ALPHA | RADEON_COMP_ARG_A
   },
};

static GLuint radeon_tfactor_alpha[] =
{
   RADEON_ALPHA_ARG_A_TFACTOR_ALPHA,
   RADEON_ALPHA_ARG_A_TFACTOR_ALPHA | RADEON_COMP_ARG_A
};

static GLuint radeon_primary_alpha[] =
{
   RADEON_ALPHA_ARG_A_DIFFUSE_ALPHA,
   RADEON_ALPHA_ARG_A_DIFFUSE_ALPHA | RADEON_COMP_ARG_A
};

static GLuint radeon_previous_alpha[] =
{
   RADEON_ALPHA_ARG_A_CURRENT_ALPHA,
   RADEON_ALPHA_ARG_A_CURRENT_ALPHA | RADEON_COMP_ARG_A
};


/* Extract the arg from slot A, shift it into the correct argument slot
 * and set the corresponding complement bit.
 */
#define RADEON_COLOR_ARG( n, arg )					\
do {									\
   color_combine |=							\
      ((color_arg[n] & RADEON_COLOR_ARG_MASK)				\
       << RADEON_COLOR_ARG_##arg##_SHIFT);				\
   color_combine |=							\
      ((color_arg[n] >> RADEON_COMP_ARG_SHIFT)				\
       << RADEON_COMP_ARG_##arg##_SHIFT);				\
} while (0)

#define RADEON_ALPHA_ARG( n, arg )					\
do {									\
   alpha_combine |=							\
      ((alpha_arg[n] & RADEON_ALPHA_ARG_MASK)				\
       << RADEON_ALPHA_ARG_##arg##_SHIFT);				\
   alpha_combine |=							\
      ((alpha_arg[n] >> RADEON_COMP_ARG_SHIFT)				\
       << RADEON_COMP_ARG_##arg##_SHIFT);				\
} while (0)


/* ================================================================
 * Texture unit state management
 */

static void radeonUpdateTextureEnv( GLcontext *ctx, int unit )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   int source = rmesa->tmu_source[unit];
   struct gl_texture_object *tObj;
   struct gl_texture_unit *texUnit;
   GLuint enabled;
   GLuint color_combine, alpha_combine;
   GLuint color_arg[3], alpha_arg[3];
   GLuint i, numColorArgs = 0, numAlphaArgs = 0;
   GLuint op;

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %p, %d )\n",
	       __FUNCTION__, ctx, unit );
   }

   enabled = (ctx->Texture.ReallyEnabled >> (source * 4)) & TEXTURE0_ANY;
   if ( enabled != TEXTURE0_2D && enabled != TEXTURE0_1D )
      return;

   /* Only update the hardware texture state if the texture is current,
    * complete and enabled.
    */
   texUnit = &ctx->Texture.Unit[source];
   tObj = texUnit->Current;
   if ( !tObj || !tObj->Complete )
      return;

   if ( ( tObj != texUnit->CurrentD[2] ) &&
	( tObj != texUnit->CurrentD[1] ) )
      return;

   /* Set the texture environment state.  Isn't this nice and clean?
    * The Radeon will automagically set the texture alpha to 0xff when
    * the texture format does not include an alpha component.  This
    * reduces the amount of special-casing we have to do, alpha-only
    * textures being a notable exception.
    */
   switch ( texUnit->EnvMode ) {
   case GL_REPLACE:
      switch ( tObj->Image[0]->Format ) {
      case GL_RGBA:
      case GL_RGB:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_INTENSITY:
	 color_combine = radeon_color_combine[unit][RADEON_REPLACE];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_REPLACE];
	 break;
      case GL_ALPHA:
	 color_combine = radeon_color_combine[unit][RADEON_DISABLE];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_REPLACE];
	 break;
      case GL_COLOR_INDEX:
      default:
	 return;
      }
      break;

   case GL_MODULATE:
      switch ( tObj->Image[0]->Format ) {
      case GL_RGBA:
      case GL_RGB:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_INTENSITY:
	 color_combine = radeon_color_combine[unit][RADEON_MODULATE];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_MODULATE];
	 break;
      case GL_ALPHA:
	 color_combine = radeon_color_combine[unit][RADEON_DISABLE];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_MODULATE];
	 break;
      case GL_COLOR_INDEX:
      default:
	 return;
      }
      break;

   case GL_DECAL:
      switch ( tObj->Image[0]->Format ) {
      case GL_RGBA:
      case GL_RGB:
	 color_combine = radeon_color_combine[unit][RADEON_DECAL];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_DISABLE];
	 break;
      case GL_ALPHA:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
      case GL_INTENSITY:
	 color_combine = radeon_color_combine[unit][RADEON_DISABLE];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_DISABLE];
	 break;
      case GL_COLOR_INDEX:
      default:
	 return;
      }
      break;

   case GL_BLEND:
      switch ( tObj->Image[0]->Format ) {
      case GL_RGBA:
      case GL_RGB:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	 color_combine = radeon_color_combine[unit][RADEON_BLEND];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_MODULATE];
	 break;
      case GL_ALPHA:
	 color_combine = radeon_color_combine[unit][RADEON_DISABLE];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_MODULATE];
	 break;
      case GL_INTENSITY:
	 color_combine = radeon_color_combine[unit][RADEON_BLEND];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_BLEND];
	 break;
      case GL_COLOR_INDEX:
      default:
	 return;
      }
      break;

   case GL_ADD:
      switch ( tObj->Image[0]->Format ) {
      case GL_RGBA:
      case GL_RGB:
      case GL_LUMINANCE:
      case GL_LUMINANCE_ALPHA:
	 color_combine = radeon_color_combine[unit][RADEON_ADD];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_MODULATE];
	 break;
      case GL_ALPHA:
	 color_combine = radeon_color_combine[unit][RADEON_DISABLE];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_MODULATE];
	 break;
      case GL_INTENSITY:
	 color_combine = radeon_color_combine[unit][RADEON_ADD];
	 alpha_combine = radeon_alpha_combine[unit][RADEON_ADD];
	 break;
      case GL_COLOR_INDEX:
      default:
	 return;
      }
      break;

   case GL_COMBINE_EXT:
      /* Step 0:
       * Calculate how many arguments we need to process.
       */
      switch ( texUnit->CombineModeRGB ) {
      case GL_REPLACE:
	 numColorArgs = 1;
	 break;
      case GL_MODULATE:
      case GL_ADD:
      case GL_ADD_SIGNED_EXT:
      case GL_DOT3_RGB_EXT:
      case GL_DOT3_RGBA_EXT:
	 numColorArgs = 2;
	 break;
      case GL_INTERPOLATE_EXT:
	 numColorArgs = 3;
	 break;
      default:
	 return;
      }

      switch ( texUnit->CombineModeA ) {
      case GL_REPLACE:
	 numAlphaArgs = 1;
	 break;
      case GL_MODULATE:
      case GL_ADD:
      case GL_ADD_SIGNED_EXT:
	 numAlphaArgs = 2;
	 break;
      case GL_INTERPOLATE_EXT:
	 numAlphaArgs = 3;
	 break;
      default:
	 return;
      }

      /* Step 1:
       * Extract the color and alpha combine function arguments.
       */
      for ( i = 0 ; i < numColorArgs ; i++ ) {
	 op = texUnit->CombineOperandRGB[i] - GL_SRC_COLOR;
	 switch ( texUnit->CombineSourceRGB[i] ) {
	 case GL_TEXTURE:
	    color_arg[i] = radeon_texture_color[op][unit];
	    break;
	 case GL_CONSTANT_EXT:
	    color_arg[i] = radeon_tfactor_color[op];
	    break;
	 case GL_PRIMARY_COLOR_EXT:
	    color_arg[i] = radeon_primary_color[op];
	    break;
	 case GL_PREVIOUS_EXT:
	    color_arg[i] = radeon_previous_color[op];
	    break;
	 default:
	    return;
	 }
      }

      for ( i = 0 ; i < numAlphaArgs ; i++ ) {
	 op = texUnit->CombineOperandA[i] - GL_SRC_ALPHA;
	 switch ( texUnit->CombineSourceA[i] ) {
	 case GL_TEXTURE:
	    alpha_arg[i] = radeon_texture_alpha[op][unit];
	    break;
	 case GL_CONSTANT_EXT:
	    alpha_arg[i] = radeon_tfactor_alpha[op];
	    break;
	 case GL_PRIMARY_COLOR_EXT:
	    alpha_arg[i] = radeon_primary_alpha[op];
	    break;
	 case GL_PREVIOUS_EXT:
	    alpha_arg[i] = radeon_previous_alpha[op];
	    break;
	 default:
	    return;
	 }
      }

      /* Step 2:
       * Build up the color and alpha combine functions.
       */
      switch ( texUnit->CombineModeRGB ) {
      case GL_REPLACE:
	 color_combine = (RADEON_COLOR_ARG_A_ZERO |
			  RADEON_COLOR_ARG_B_ZERO |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, C );
	 break;
      case GL_MODULATE:
	 color_combine = (RADEON_COLOR_ARG_C_ZERO |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, B );
	 break;
      case GL_ADD:
	 color_combine = (RADEON_COLOR_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 break;
      case GL_ADD_SIGNED_EXT:
	 color_combine = (RADEON_COLOR_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_ADDSIGNED |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, C );
	 break;
      case GL_INTERPOLATE_EXT:
	 color_combine = (RADEON_BLEND_CTL_BLEND |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, B );
	 RADEON_COLOR_ARG( 1, A );
	 RADEON_COLOR_ARG( 2, C );
	 break;
      case GL_DOT3_RGB_EXT:
      case GL_DOT3_RGBA_EXT:
	 color_combine = (RADEON_COLOR_ARG_C_ZERO |
			  RADEON_BLEND_CTL_DOT3 |
			  RADEON_CLAMP_TX);
	 RADEON_COLOR_ARG( 0, A );
	 RADEON_COLOR_ARG( 1, B );
	 break;
      default:
	 return;
      }

      switch ( texUnit->CombineModeA ) {
      case GL_REPLACE:
	 alpha_combine = (RADEON_ALPHA_ARG_A_ZERO |
			  RADEON_ALPHA_ARG_B_ZERO |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, C );
	 break;
      case GL_MODULATE:
	 alpha_combine = (RADEON_ALPHA_ARG_C_ZERO |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, B );
	 break;
      case GL_ADD:
	 alpha_combine = (RADEON_ALPHA_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_ADD |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 break;
      case GL_ADD_SIGNED_EXT:
	 alpha_combine = (RADEON_ALPHA_ARG_B_ZERO |
			  RADEON_COMP_ARG_B |
			  RADEON_BLEND_CTL_ADDSIGNED |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, A );
	 RADEON_ALPHA_ARG( 1, C );
	 break;
      case GL_INTERPOLATE_EXT:
	 alpha_combine = (RADEON_BLEND_CTL_BLEND |
			  RADEON_CLAMP_TX);
	 RADEON_ALPHA_ARG( 0, B );
	 RADEON_ALPHA_ARG( 1, A );
	 RADEON_ALPHA_ARG( 2, C );
	 break;
      default:
	 return;
      }

      if ( texUnit->CombineModeRGB == GL_DOT3_RGB_EXT ) {
	 alpha_combine |= RADEON_DOT_ALPHA_DONT_REPLICATE;
      }

      /* Step 3:
       * Apply the scale factor.  The EXT extension has a somewhat
       * unnecessary restriction that the scale must be 4x.  The ARB
       * extension will likely drop this and we can just apply the
       * scale factors regardless.
       */
      if ( texUnit->CombineModeRGB != GL_DOT3_RGB_EXT &&
	   texUnit->CombineModeRGB != GL_DOT3_RGBA_EXT ) {
	 color_combine |= (texUnit->CombineScaleShiftRGB << 21);
	 alpha_combine |= (texUnit->CombineScaleShiftA << 21);
      } else {
	 color_combine |= RADEON_SCALE_4X;
	 alpha_combine |= RADEON_SCALE_4X;
      }

      /* All done!
       */
      break;

   default:
      return;
   }

   rmesa->color_combine[source] = color_combine;
   rmesa->alpha_combine[source] = alpha_combine;
}

static void radeonUpdateTextureObject( GLcontext *ctx, int unit )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   int source = rmesa->tmu_source[unit];
   struct gl_texture_object *tObj;
   radeonTexObjPtr t;
   GLuint enabled;

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %p, %d )\n",
	       __FUNCTION__, ctx, unit );
   }

   enabled = (ctx->Texture.ReallyEnabled >> (source * 4)) & TEXTURE0_ANY;
   if ( enabled != TEXTURE0_2D && enabled != TEXTURE0_1D ) {
      if ( enabled )
	 rmesa->Fallback |= RADEON_FALLBACK_TEXTURE;
      return;
   }

   /* Only update the hardware texture state if the texture is current,
    * complete and enabled.
    */
   tObj = ctx->Texture.Unit[source].Current;
   if ( !tObj || !tObj->Complete )
      return;

   if ( ( tObj != ctx->Texture.Unit[source].CurrentD[2] ) &&
	( tObj != ctx->Texture.Unit[source].CurrentD[1] ) )
      return;

   if ( !tObj->DriverData ) {
      /* If this is the first time the texture has been used, then create
       * a new texture object for it.
       */
      radeonCreateTexObj( rmesa, tObj );

      if ( !tObj->DriverData ) {
	 /* Can't create a texture object... */
	 fprintf( stderr, "%s: texture object creation failed!\n",
		  __FUNCTION__ );
	 rmesa->Fallback |= RADEON_FALLBACK_TEXTURE;
	 return;
      }
   }

   /* We definately have a valid texture now */
   t = tObj->DriverData;

   /* Force the texture unit state to be loaded into the hardware */
   rmesa->dirty |= RADEON_UPLOAD_CONTEXT | (RADEON_UPLOAD_TEX0 << unit);

   /* Force any texture images to be loaded into the hardware */
   if ( t->dirty_images )
      rmesa->dirty |= (RADEON_UPLOAD_TEX0IMAGES << unit);

   /* Bind to the given texture unit */
   rmesa->CurrentTexObj[unit] = t;
   t->bound = unit + 1;

   if ( t->memBlock )
      radeonUpdateTexLRU( rmesa, t );

   switch ( unit ) {
   case 0:
      rmesa->setup.pp_cntl |= (RADEON_TEX_0_ENABLE |
			       RADEON_TEX_BLEND_0_ENABLE);
      break;
   case 1:
      rmesa->setup.pp_cntl |= (RADEON_TEX_1_ENABLE |
			       RADEON_TEX_BLEND_1_ENABLE);
      break;
   }
}

void radeonUpdateTextureState( GLcontext *ctx )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %p ) en=0x%x\n",
	       __FUNCTION__, ctx, ctx->Texture.ReallyEnabled );
   }

   /* Clear any texturing fallbacks */
   rmesa->Fallback &= ~RADEON_FALLBACK_TEXTURE;

   /* Unbind any currently bound textures */
   if ( rmesa->CurrentTexObj[0] ) rmesa->CurrentTexObj[0]->bound = 0;
   if ( rmesa->CurrentTexObj[1] ) rmesa->CurrentTexObj[1]->bound = 0;
   rmesa->CurrentTexObj[0] = NULL;
   rmesa->CurrentTexObj[1] = NULL;

   if ( ctx->Enabled & (TEXTURE0_3D|TEXTURE1_3D) )
      rmesa->Fallback |= RADEON_FALLBACK_TEXTURE;

   /* Disable all texturing until it is known to be good */
   rmesa->setup.pp_cntl &= ~(RADEON_TEX_ENABLE_MASK |
			     RADEON_TEX_BLEND_ENABLE_MASK);

   radeonUpdateTextureObject( ctx, 0 );
   radeonUpdateTextureEnv( ctx, 0 );

   if ( rmesa->multitex ) {
      radeonUpdateTextureObject( ctx, 1 );
      radeonUpdateTextureEnv( ctx, 1 );
   }

   rmesa->dirty |= RADEON_UPLOAD_CONTEXT;
}


/* ================================================================
 * DD interface texturing functions
 *
 * FIXME: Many of these are deprecated -- we should move to the new
 * single-copy texture interface.
 */
#define SCALED_FLOAT_TO_BYTE( x, scale ) \
		((((GLint)((256.0F / scale) * (x))) - 1) / 2)

static void radeonDDTexEnv( GLcontext *ctx, GLenum target,
			    GLenum pname, const GLfloat *param )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   struct gl_texture_unit *texUnit;
   GLuint source;
   GLubyte c[4];
   GLuint col;
   GLfloat bias;
   GLubyte b;

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %s )\n",
	       __FUNCTION__, gl_lookup_enum_by_nr( pname ) );
   }

   switch ( pname ) {
   case GL_TEXTURE_ENV_MODE:
      FLUSH_BATCH( rmesa );
      rmesa->new_state |= RADEON_NEW_TEXTURE | RADEON_NEW_ALPHA;
      break;

   case GL_TEXTURE_ENV_COLOR:
      source = rmesa->tmu_source[ctx->Texture.CurrentUnit];
      texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
      FLOAT_RGBA_TO_UBYTE_RGBA( c, texUnit->EnvColor );
      col = radeonPackColor( 4, c[0], c[1], c[2], c[3] );
      if ( rmesa->env_color[source] != col ) {
	 FLUSH_BATCH( rmesa );
	 rmesa->env_color[source] = col;

	 rmesa->new_state |= RADEON_NEW_TEXTURE;
      }
      break;

   case GL_TEXTURE_LOD_BIAS_EXT:
      /* The Radeon's LOD bias is a signed 2's complement value with a
       * range of -1.0 <= bias < 4.0.  We break this into two linear
       * functions, one mapping [-1.0,0.0] to [-128,0] and one mapping
       * [0.0,4.0] to [0,127].
       */
      source = rmesa->tmu_source[ctx->Texture.CurrentUnit];
      bias = CLAMP( *param, -1.0, 4.0 );
      if ( bias == 0 ) {
	 b = 0;
      } else if ( bias > 0 ) {
	 b = (GLubyte) SCALED_FLOAT_TO_BYTE( bias, 4.0 );
      } else {
	 b = (GLubyte) SCALED_FLOAT_TO_BYTE( bias, 1.0 );
      }
      if ( rmesa->lod_bias[source] != (GLuint)b ) {
	 FLUSH_BATCH( rmesa );
	 rmesa->lod_bias[source] = (GLuint)b;

	 rmesa->new_state |= RADEON_NEW_TEXTURE;
      }
      break;

   default:
      return;
   }
}

static void radeonDDTexImage( GLcontext *ctx, GLenum target,
			      struct gl_texture_object *tObj, GLint level,
			      GLint internalFormat,
			      const struct gl_texture_image *image )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   radeonTexObjPtr t;

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API )
      fprintf( stderr, "%s( %p, level %d )\n", __FUNCTION__, tObj, level );

   if ( ( target != GL_TEXTURE_2D ) &&
	( target != GL_TEXTURE_1D ) )
      return;

   if ( level >= RADEON_MAX_TEXTURE_LEVELS )
      return;

   t = (radeonTexObjPtr)tObj->DriverData;
   if ( t ) {
      if ( t->bound ) FLUSH_BATCH( rmesa );

      /* Destroy the old texture, and upload a new one.  The actual
       * uploading of the texture image occurs in the UploadSubImage
       * function.
       */
      radeonDestroyTexObj( rmesa, t );
      rmesa->new_state |= RADEON_NEW_TEXTURE;
   }
}

static void radeonDDTexSubImage( GLcontext *ctx, GLenum target,
				 struct gl_texture_object *tObj, GLint level,
				 GLint xoffset, GLint yoffset,
				 GLsizei width, GLsizei height,
				 GLint internalFormat,
				 const struct gl_texture_image *image )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   radeonTexObjPtr t;

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %p, level %d ) size: %d,%d of %d,%d\n",
	       __FUNCTION__, tObj, level, width, height,
	       image->Width, image->Height );
   }

   if ( ( target != GL_TEXTURE_2D ) &&
	( target != GL_TEXTURE_1D ) )
      return;

   if ( level >= RADEON_MAX_TEXTURE_LEVELS )
      return;

   t = (radeonTexObjPtr)tObj->DriverData;
   if ( t ) {
      if ( t->bound ) FLUSH_BATCH( rmesa );

#if 0
      /* FIXME: Only upload textures if we already have space in the heap.
       */
      LOCK_HARDWARE( rmesa );
      radeonUploadSubImage( rmesa, t, level,
			    xoffset, yoffset, width, height );
      UNLOCK_HARDWARE( rmesa );
#else
      radeonDestroyTexObj( rmesa, t );
#endif
      /* Update the context state */
      rmesa->new_state |= RADEON_NEW_TEXTURE;
   }
}

static void radeonDDTexParameter( GLcontext *ctx, GLenum target,
				  struct gl_texture_object *tObj,
				  GLenum pname, const GLfloat *params )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   radeonTexObjPtr t = (radeonTexObjPtr)tObj->DriverData;

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %s )\n",
	       __FUNCTION__, gl_lookup_enum_by_nr( pname ) );
   }

   /* If we don't have a hardware texture, it will be automatically
    * created with current state before it is used, so we don't have
    * to do anything now.
    */
   if ( !t || !t->bound )
      return;

   if ( ( target != GL_TEXTURE_2D ) &&
	( target != GL_TEXTURE_1D ) )
      return;

   switch ( pname ) {
   case GL_TEXTURE_MIN_FILTER:
   case GL_TEXTURE_MAG_FILTER:
      if ( t->bound ) FLUSH_BATCH( rmesa );
      radeonSetTexFilter( t, tObj->MinFilter, tObj->MagFilter );
      break;

   case GL_TEXTURE_WRAP_S:
   case GL_TEXTURE_WRAP_T:
      if ( t->bound ) FLUSH_BATCH( rmesa );
      radeonSetTexWrap( t, tObj->WrapS, tObj->WrapT );
      break;

   case GL_TEXTURE_BORDER_COLOR:
      if ( t->bound ) FLUSH_BATCH( rmesa );
      radeonSetTexBorderColor( t, tObj->BorderColor );
      break;

   default:
      return;
   }

   rmesa->new_state |= RADEON_NEW_TEXTURE;
}

static void radeonDDBindTexture( GLcontext *ctx, GLenum target,
				 struct gl_texture_object *tObj )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   GLint unit = ctx->Texture.CurrentUnit;

   if ( RADEON_DEBUG & DEBUG_VERBOSE_API ) {
      fprintf( stderr, "%s( %p ) unit=%d\n",
	       __FUNCTION__, tObj, ctx->Texture.CurrentUnit );
   }

   FLUSH_BATCH( rmesa );

   if ( rmesa->CurrentTexObj[unit] ) {
      rmesa->CurrentTexObj[unit]->bound = 0;
      rmesa->CurrentTexObj[unit] = NULL;
   }

   rmesa->new_state |= RADEON_NEW_TEXTURE;
}

static void radeonDDDeleteTexture( GLcontext *ctx,
				 struct gl_texture_object *tObj )
{
   radeonContextPtr rmesa = RADEON_CONTEXT(ctx);
   radeonTexObjPtr t = (radeonTexObjPtr)tObj->DriverData;

   if ( t ) {
      if ( t->bound ) {
	 FLUSH_BATCH( rmesa );

	 rmesa->CurrentTexObj[t->bound-1] = 0;
	 rmesa->new_state |= RADEON_NEW_TEXTURE;
      }

      radeonDestroyTexObj( rmesa, t );
      tObj->DriverData = NULL;
   }
}

static GLboolean radeonDDIsTextureResident( GLcontext *ctx,
					    struct gl_texture_object *tObj )
{
   radeonTexObjPtr t = (radeonTexObjPtr)tObj->DriverData;

   return ( t && t->memBlock );
}



void radeonDDInitTextureFuncs( GLcontext *ctx )
{
   ctx->Driver.TexEnv			= radeonDDTexEnv;
   ctx->Driver.TexImage			= radeonDDTexImage;
   ctx->Driver.TexSubImage		= radeonDDTexSubImage;
   ctx->Driver.TexParameter		= radeonDDTexParameter;
   ctx->Driver.BindTexture		= radeonDDBindTexture;
   ctx->Driver.DeleteTexture		= radeonDDDeleteTexture;
   ctx->Driver.UpdateTexturePalette	= NULL;
   ctx->Driver.ActiveTexture		= NULL;
   ctx->Driver.IsTextureResident	= radeonDDIsTextureResident;
   ctx->Driver.PrioritizeTexture	= NULL;
}
