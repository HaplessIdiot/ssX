/* Id: bitmap.c,v 1.3 1999/02/26 08:52:31 martin Exp $ */

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
 * Log: bitmap.c,v $
 * Revision 1.3  1999/02/26 08:52:31  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.12  1999/02/24 22:48:04  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.11  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.10  1998/07/30 02:39:18  brianp
 * feedback/selection modes didn't work
 *
 * Revision 3.9  1998/07/29 04:07:53  brianp
 * feedback was returning wrong values
 *
 * Revision 3.8  1998/05/04 23:54:35  brianp
 * include stdio.h since assert.h needs it on some systems
 *
 * Revision 3.7  1998/03/27 03:30:36  brianp
 * fixed G++ warnings
 *
 * Revision 3.6  1998/03/25 01:24:11  brianp
 * glBitmap in feedback mode returned X,Y offset by bitmap origin (wrong)
 *
 * Revision 3.5  1998/03/05 03:07:56  brianp
 * check for invalid raster pos in gl_direct_bitamp()
 *
 * Revision 3.4  1998/02/27 00:54:29  brianp
 * NULL bitmap image caused assertion- fixed
 *
 * Revision 3.3  1998/02/15 01:32:59  brianp
 * updated driver bitmap function
 *
 * Revision 3.2  1998/02/08 20:23:49  brianp
 * lots of bitmap rendering changes
 *
 * Revision 3.1  1998/02/04 00:33:45  brianp
 * fixed a few cast problems for Amiga StormC compiler
 *
 * Revision 3.0  1998/01/31 20:47:29  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/bitmap.c,v 1.0tsi Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#endif 
#include "bitmap.h"
#include "context.h"
#include "feedback.h"
#include "image.h"
#include "macros.h"
#include "pb.h"
#include "pixel.h"
#include "types.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



/*
 * Render bitmap data.
 */
static void render_bitmap( GLcontext *ctx, GLint px, GLint py,
                           GLsizei width, GLsizei height,
                           const struct gl_pixelstore_attrib *unpack,
                           const GLubyte *bitmap )
{
   struct pixel_buffer *PB = ctx->PB;
   GLint row, col;
   GLint pz;

   if (!bitmap)
      return;

   if (ctx->NewState) {
      gl_update_state(ctx);
      PB_INIT( PB, GL_BITMAP );
   }

   if (ctx->Visual->RGBAflag) {
      GLint r, g, b, a;
      r = (GLint) (ctx->Current.RasterColor[0] * 255.0F);
      g = (GLint) (ctx->Current.RasterColor[1] * 255.0F);
      b = (GLint) (ctx->Current.RasterColor[2] * 255.0F);
      a = (GLint) (ctx->Current.RasterColor[3] * 255.0F);
      PB_SET_COLOR( ctx, PB, r, g, b, a );
   }
   else {
      PB_SET_INDEX( ctx, PB, ctx->Current.RasterIndex );
   }

   pz = (GLint) ( ctx->Current.RasterPos[2] * DEPTH_SCALE );

   for (row=0; row<height; row++) {
      GLubyte *src = (GLubyte *) gl_pixel_addr_in_image( unpack, bitmap,
                      width, height, GL_COLOR_INDEX, GL_BITMAP, 0, row, 0 );

      if (unpack->LsbFirst) {
         /* Lsb first */
         GLubyte bitmask = 1;
         for (col=0; col<width; col++) {
            if (*src & bitmask) {
               PB_WRITE_PIXEL( PB, px+col, py+row, pz );
            }
            bitmask = bitmask << 1;
            if (bitmask==0) {
               src++;
               bitmask = 1;
            }
         }

         PB_CHECK_FLUSH( ctx, PB )

         /* get ready for next row */
         if (bitmask!=1)
            src++;
      }
      else {
         /* Msb first */
         GLubyte bitmask = 128;
         for (col=0; col<width; col++) {
            if (*src & bitmask) {
               PB_WRITE_PIXEL( PB, px+col, py+row, pz );
            }
            bitmask = bitmask >> 1;
            if (bitmask==0) {
               src++;
               bitmask = 128;
            }
         }

         PB_CHECK_FLUSH( ctx, PB )

         /* get ready for next row */
         if (bitmask!=128)
            src++;
      }
   }

   gl_flush_pb(ctx);
}




/*
 * Optimized glBitmap
 */
GLboolean gl_direct_bitmap( GLcontext *ctx, 
                            GLsizei width, GLsizei height,
                            GLfloat xorig, GLfloat yorig,
                            GLfloat xmove, GLfloat ymove,
                            const GLubyte *bitmap )
{
   GLint px = (GLint) ( (ctx->Current.RasterPos[0] - xorig) + 0.0F );
   GLint py = (GLint) ( (ctx->Current.RasterPos[1] - yorig) + 0.0F );
   GLboolean completed = GL_FALSE;

   if (ctx->Current.RasterPosValid==GL_FALSE) {
      return GL_TRUE;  /* command completed as a no-op */
   }

   if (ctx->RenderMode != GL_RENDER)
      return GL_FALSE;

   if (ctx->Driver.Bitmap) {
      /* let device driver try to render the bitmap */
      completed = (*ctx->Driver.Bitmap)( ctx, px, py, width, height,
                                         &ctx->Unpack, bitmap );
   }
   if (!completed) {
      /* We can handle an pixel unpacking combo! */
      render_bitmap( ctx, px, py, width, height, &ctx->Unpack, bitmap );
   }

   /* update raster position */
   ctx->Current.RasterPos[0] += xmove;
   ctx->Current.RasterPos[1] += ymove;

   return GL_TRUE;
}




/* Simple unpacking parameters: */
static struct gl_pixelstore_attrib NoUnpack = {
   1,            /* Alignment */
   0,            /* RowLength */
   0,            /* SkipPixels */
   0,            /* SkipRows */
   0,            /* ImageHeight */
   0,            /* SkipImages */
   GL_FALSE,     /* SwapBytes */
   GL_FALSE      /* LsbFirst */
};



/*
 * Execute a glBitmap command:
 *   1. check for errors
 *   2. feedback/render/select
 *   3. advance raster position
 */
void gl_Bitmap( GLcontext* ctx,
                GLsizei width, GLsizei height,
	        GLfloat xorig, GLfloat yorig,
	        GLfloat xmove, GLfloat ymove,
                struct gl_image *bitmap )
{
   if (width<0 || height<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glBitmap" );
      return;
   }
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glBitmap" );
      return;
   }
   if (ctx->Current.RasterPosValid==GL_FALSE) {
      /* do nothing */
      return;
   }

   assert(bitmap->Type == GL_BITMAP);
   assert(bitmap->Format == GL_COLOR_INDEX);
   
   if (ctx->RenderMode==GL_RENDER) {
      GLint px = (GLint) ( (ctx->Current.RasterPos[0] - xorig) + 0.0F );
      GLint py = (GLint) ( (ctx->Current.RasterPos[1] - yorig) + 0.0F );
      GLboolean completed = GL_FALSE;
      if (ctx->Driver.Bitmap) {
         /* let device driver try to render the bitmap */
         completed = (*ctx->Driver.Bitmap)( ctx, px, py, width, height,
                                            &NoUnpack,
                                            (const GLubyte *) bitmap->Data );
      }
      if (!completed) {
         /* use generic function */
         render_bitmap( ctx, px, py, width, height, &NoUnpack,
                        (const GLubyte *) bitmap->Data );
      }
   }
   else if (ctx->RenderMode==GL_FEEDBACK) {
      GLfloat color[4], texcoord[4], invq;
      color[0] = ctx->Current.RasterColor[0];
      color[1] = ctx->Current.RasterColor[1];
      color[2] = ctx->Current.RasterColor[2];
      color[3] = ctx->Current.RasterColor[3];
      if (ctx->Current.TexCoord[3] == 0.0)
         invq = 1.0F;
      else
         invq = 1.0F / ctx->Current.RasterTexCoord[3];
      texcoord[0] = ctx->Current.RasterTexCoord[0] * invq;
      texcoord[1] = ctx->Current.RasterTexCoord[1] * invq;
      texcoord[2] = ctx->Current.RasterTexCoord[2] * invq;
      texcoord[3] = ctx->Current.RasterTexCoord[3];
      FEEDBACK_TOKEN( ctx, (GLfloat) (GLint) GL_BITMAP_TOKEN );
      gl_feedback_vertex( ctx,
                          ctx->Current.RasterPos[0],
			  ctx->Current.RasterPos[1],
			  ctx->Current.RasterPos[2],
			  ctx->Current.RasterPos[3],
			  color, ctx->Current.RasterIndex, texcoord );
   }
   else if (ctx->RenderMode==GL_SELECT) {
      /* Bitmaps don't generate selection hits.  See appendix B of 1.1 spec. */
   }

   /* update raster position */
   ctx->Current.RasterPos[0] += xmove;
   ctx->Current.RasterPos[1] += ymove;
}
