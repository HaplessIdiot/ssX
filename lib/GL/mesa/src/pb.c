/* Id: pb.c,v 1.3 1999/02/26 08:52:36 martin Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Log: pb.c,v $
 * Revision 1.3  1999/02/26 08:52:36  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.7  1999/02/24 22:48:07  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.6  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.5  1998/12/01 02:42:43  brianp
 * patched per MJK for multi-texturing
 *
 * Revision 3.4  1998/04/01 02:58:52  brianp
 * applied Miklos Fazekas's 3-31-98 Macintosh changes
 *
 * Revision 3.3  1998/03/05 02:59:05  brianp
 * [RGBA]COMP wasn't being used in a few places
 *
 * Revision 3.2  1998/02/20 04:50:44  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.1  1998/02/15 01:32:37  brianp
 * fixed a comment
 *
 * Revision 3.0  1998/01/31 21:00:28  brianp
 * initial rev
 *
 */


/*
 * Pixel buffer:
 *
 * As fragments are produced (by point, line, and bitmap drawing) they
 * are accumlated in a buffer.  When the buffer is full or has to be
 * flushed (glEnd), we apply all enabled rasterization functions to the
 * pixels and write the results to the display buffer.  The goal is to
 * maximize the number of pixels processed inside loops and to minimize
 * the number of function calls.
 */
/* $XFree86: xc/lib/GL/mesa/src/pb.c,v 1.0tsi Exp $ */


#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <stdlib.h>
#include <string.h>
#endif
#include "alpha.h"
#include "alphabuf.h"
#include "blend.h"
#include "depth.h"
#include "fog.h"
#include "logic.h"
#include "macros.h"
#include "masking.h"
#include "pb.h"
#include "scissor.h"
#include "stencil.h"
#include "texture.h"
#include "types.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



/*
 * Allocate and initialize a new pixel buffer structure.
 */
struct pixel_buffer *gl_alloc_pb(void)
{
   struct pixel_buffer *pb;
   pb = (struct pixel_buffer *) calloc(sizeof(struct pixel_buffer), 1);
   if (pb) {
      int i, j;
      /* set non-zero fields */
      pb->primitive = GL_BITMAP;
      /* Set all lambda values to 0.0 since we don't do mipmapping for
       * points or lines and want to use the level 0 texture image.
       */

      for (j=0;j<MAX_TEXTURE_UNITS;j++) {
         for (i=0; i<PB_SIZE; i++) {
            pb->lambda[j][i] = 0.0;
         }
      }
   }
   return pb;
}




/*
 * When the pixel buffer is full, or needs to be flushed, call this
 * function.  All the pixels in the pixel buffer will be subjected
 * to texturing, scissoring, stippling, alpha testing, stenciling,
 * depth testing, blending, and finally written to the frame buffer.
 */
void gl_flush_pb( GLcontext *ctx )
{
   struct pixel_buffer* PB = ctx->PB;

   GLubyte mask[PB_SIZE];
   GLubyte saved[PB_SIZE][4];

   if (PB->count==0)  goto CleanUp;

   /* initialize mask array and clip pixels simultaneously */
   {
      GLint xmin = ctx->Buffer->Xmin;
      GLint xmax = ctx->Buffer->Xmax;
      GLint ymin = ctx->Buffer->Ymin;
      GLint ymax = ctx->Buffer->Ymax;
      GLint *x = PB->x;
      GLint *y = PB->y;
      GLuint i, n = PB->count;
      for (i=0;i<n;i++) {
         mask[i] = (x[i]>=xmin) & (x[i]<=xmax) & (y[i]>=ymin) & (y[i]<=ymax);
      }
   }

   if (ctx->Visual->RGBAflag) {
      /* RGBA COLOR PIXELS */
      if (PB->mono && ctx->MutablePixels) {
	 /* Copy flat color to all pixels */
         GLuint i;
         for (i=0; i<PB->count; i++) {
            PB->rgba[i][RCOMP] = PB->color[RCOMP];
            PB->rgba[i][GCOMP] = PB->color[GCOMP];
            PB->rgba[i][BCOMP] = PB->color[BCOMP];
            PB->rgba[i][ACOMP] = PB->color[ACOMP];
         }
      }

      /* If each pixel can be of a different color... */
      if (ctx->MutablePixels || !PB->mono) {

	 if (ctx->Texture.Enabled) {
            int texUnit;

            for (texUnit=0;texUnit<MAX_TEXTURE_UNITS;texUnit++) {
	        gl_texture_pixels( ctx, texUnit,
                                   PB->count, PB->s[texUnit], PB->t[texUnit], PB->u[texUnit], PB->lambda[texUnit],
                                   PB->rgba);
            }
	 }

	 if (ctx->Fog.Enabled
             && (ctx->Hint.Fog==GL_NICEST || PB->primitive==GL_BITMAP
                 || ctx->Texture.Enabled)) {
	    gl_fog_rgba_pixels( ctx, PB->count, PB->z, PB->rgba );
	 }

         /* Scissoring already done above */

	 if (ctx->Color.AlphaEnabled) {
	    if (gl_alpha_test( ctx, PB->count, PB->rgba, mask )==0) {
	       goto CleanUp;
	    }
	 }

	 if (ctx->Stencil.Enabled) {
	    /* first stencil test */
	    if (gl_stencil_pixels( ctx, PB->count, PB->x, PB->y, mask )==0) {
	       goto CleanUp;
	    }
	    /* depth buffering w/ stencil */
	    gl_depth_stencil_pixels( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }
	 else if (ctx->Depth.Test) {
	    /* regular depth testing */
	    (*ctx->Driver.DepthTestPixels)( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }

         if (ctx->RasterMask & NO_DRAW_BIT) {
            goto CleanUp;
         }

         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /* make a copy of the colors */
            MEMCPY( saved, PB->rgba, PB->count * 4 * sizeof(GLubyte) );
         }

         if (ctx->Color.SWLogicOpEnabled) {
            gl_logicop_rgba_pixels( ctx, PB->count, PB->x, PB->y,
                                    PB->rgba, mask);
         }
         else if (ctx->Color.BlendEnabled) {
            gl_blend_pixels( ctx, PB->count, PB->x, PB->y, PB->rgba, mask);
         }

         if (ctx->Color.SWmasking) {
            gl_mask_rgba_pixels(ctx, PB->count, PB->x, PB->y, PB->rgba, mask);
         }

         /* write pixels */
         (*ctx->Driver.WriteRGBAPixels)( ctx, PB->count, PB->x, PB->y,
                                         PB->rgba, mask );
         if (ctx->RasterMask & ALPHABUF_BIT) {
            gl_write_alpha_pixels( ctx, PB->count, PB->x, PB->y, PB->rgba, mask );
         }

         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /*** Also draw to back buffer ***/
            (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
            if (ctx->Color.SWLogicOpEnabled) {
               gl_logicop_rgba_pixels( ctx, PB->count, PB->x, PB->y,
                                       PB->rgba, mask);
            }
            else if (ctx->Color.BlendEnabled) {
               gl_blend_pixels( ctx, PB->count, PB->x, PB->y, saved, mask );
            }
            if (ctx->Color.SWmasking) {
               gl_mask_rgba_pixels(ctx, PB->count, PB->x, PB->y, saved, mask);
            }
            (*ctx->Driver.WriteRGBAPixels)( ctx, PB->count, PB->x, PB->y,
                                            saved, mask);
            if (ctx->RasterMask & ALPHABUF_BIT) {
               ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
               gl_write_alpha_pixels( ctx, PB->count, PB->x, PB->y,
                                      saved, mask );
               ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
            }
            (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
            /*** ALL DONE ***/
         }
      }
      else {
	 /* Same color for all pixels */

         /* Scissoring already done above */

	 if (ctx->Color.AlphaEnabled) {
	    if (gl_alpha_test( ctx, PB->count, PB->rgba, mask )==0) {
	       goto CleanUp;
	    }
	 }

	 if (ctx->Stencil.Enabled) {
	    /* first stencil test */
	    if (gl_stencil_pixels( ctx, PB->count, PB->x, PB->y, mask )==0) {
	       goto CleanUp;
	    }
	    /* depth buffering w/ stencil */
	    gl_depth_stencil_pixels( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }
	 else if (ctx->Depth.Test) {
	    /* regular depth testing */
	    (*ctx->Driver.DepthTestPixels)( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }

         if (ctx->RasterMask & NO_DRAW_BIT) {
            goto CleanUp;
         }

         /* write pixels */
         {
            GLubyte red, green, blue, alpha;
            red   = PB->color[RCOMP];
            green = PB->color[GCOMP];
            blue  = PB->color[BCOMP];
            alpha = PB->color[ACOMP];
	    (*ctx->Driver.Color)( ctx, red, green, blue, alpha );
         }
         (*ctx->Driver.WriteMonoRGBAPixels)( ctx, PB->count, PB->x, PB->y, mask );
         if (ctx->RasterMask & ALPHABUF_BIT) {
            gl_write_mono_alpha_pixels( ctx, PB->count, PB->x, PB->y,
                                        PB->color[ACOMP], mask );
         }

         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /*** Also render to back buffer ***/
            (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
            (*ctx->Driver.WriteMonoRGBAPixels)( ctx, PB->count, PB->x, PB->y, mask );
            if (ctx->RasterMask & ALPHABUF_BIT) {
               ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
               gl_write_mono_alpha_pixels( ctx, PB->count, PB->x, PB->y,
                                           PB->color[ACOMP], mask );
               ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
            }
            (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
	 }
         /*** ALL DONE ***/
      }
   }
   else {
      /* COLOR INDEX PIXELS */

      /* If we may be writting pixels with different indexes... */
      if (PB->mono && ctx->MutablePixels) {
	 /* copy index to all pixels */
         GLuint n = PB->count, indx = PB->index;
         GLuint *pbindex = PB->i;
         do {
	    *pbindex++ = indx;
            n--;
	 } while (n);
      }

      if (ctx->MutablePixels || !PB->mono) {
	 /* Pixel color index may be modified */
         GLuint *isave = (GLuint*)saved;

	 if (ctx->Fog.Enabled
             && (ctx->Hint.Fog==GL_NICEST || PB->primitive==GL_BITMAP)) {
	    gl_fog_ci_pixels( ctx, PB->count, PB->z, PB->i );
	 }

         /* Scissoring already done above */

	 if (ctx->Stencil.Enabled) {
	    /* first stencil test */
	    if (gl_stencil_pixels( ctx, PB->count, PB->x, PB->y, mask )==0) {
	       goto CleanUp;
	    }
	    /* depth buffering w/ stencil */
	    gl_depth_stencil_pixels( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }
	 else if (ctx->Depth.Test) {
	    /* regular depth testing */
	    (*ctx->Driver.DepthTestPixels)( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }

         if (ctx->RasterMask & NO_DRAW_BIT) {
            goto CleanUp;
         }

         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /* make a copy of the indexes */
            MEMCPY( isave, PB->i, PB->count * sizeof(GLuint) );
         }

         if (ctx->Color.SWLogicOpEnabled) {
            gl_logicop_ci_pixels( ctx, PB->count, PB->x, PB->y, PB->i, mask );
         }

         if (ctx->Color.SWmasking) {
            gl_mask_index_pixels( ctx, PB->count, PB->x, PB->y, PB->i, mask );
         }

         /* write pixels */
         (*ctx->Driver.WriteCI32Pixels)( ctx, PB->count, PB->x, PB->y,
                                          PB->i, mask );

         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /*** Also write to back buffer ***/
            (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
            MEMCPY( PB->i, isave, PB->count * sizeof(GLuint) );
            if (ctx->Color.SWLogicOpEnabled) {
               gl_logicop_ci_pixels( ctx, PB->count, PB->x, PB->y, PB->i, mask );
            }
            if (ctx->Color.SWmasking) {
               gl_mask_index_pixels( ctx, PB->count, PB->x, PB->y,
                                     PB->i, mask );
            }
            (*ctx->Driver.WriteCI32Pixels)( ctx, PB->count, PB->x, PB->y,
                                             PB->i, mask );
            (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
         }

         /*** ALL DONE ***/
      }
      else {
	 /* Same color index for all pixels */

         /* Scissoring already done above */

	 if (ctx->Stencil.Enabled) {
	    /* first stencil test */
	    if (gl_stencil_pixels( ctx, PB->count, PB->x, PB->y, mask )==0) {
	       goto CleanUp;
	    }
	    /* depth buffering w/ stencil */
	    gl_depth_stencil_pixels( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }
	 else if (ctx->Depth.Test) {
	    /* regular depth testing */
	    (*ctx->Driver.DepthTestPixels)( ctx, PB->count, PB->x, PB->y, PB->z, mask );
	 }
         
         if (ctx->RasterMask & NO_DRAW_BIT) {
            goto CleanUp;
         }

         /* write pixels */
         (*ctx->Driver.Index)( ctx, PB->index );
         (*ctx->Driver.WriteMonoCIPixels)( ctx, PB->count, PB->x, PB->y, mask );

         if (ctx->RasterMask & FRONT_AND_BACK_BIT) {
            /*** Also write to back buffer ***/
            (*ctx->Driver.SetBuffer)( ctx, GL_BACK );
            (*ctx->Driver.WriteMonoCIPixels)( ctx, PB->count, PB->x, PB->y, mask );
            (*ctx->Driver.SetBuffer)( ctx, GL_FRONT );
         }
         /*** ALL DONE ***/
      }
   }

CleanUp:
   PB->count = 0;
}


