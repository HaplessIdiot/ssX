/* Id: masking.c,v 1.3 1999/02/26 08:52:35 martin Exp $ */

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
 * Log: masking.c,v $
 * Revision 1.3  1999/02/26 08:52:35  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.3  1999/02/24 22:48:06  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.2  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.1  1998/02/08 20:19:41  brianp
 * ColorMask is now GLubyte[4] instead of GLuint
 *
 * Revision 3.0  1998/01/31 20:58:46  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/masking.c,v 1.0tsi Exp $ */

/*
 * Implement the effect of glColorMask and glIndexMask in software.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <string.h>
#endif
#include "alphabuf.h"
#include "context.h"
#include "macros.h"
#include "masking.h"
#include "pb.h"
#include "span.h"
#include "types.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



void gl_IndexMask( GLcontext *ctx, GLuint mask )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glIndexMask" );
      return;
   }
   ctx->Color.IndexMask = mask;
   ctx->NewState |= NEW_RASTER_OPS;
}



void gl_ColorMask( GLcontext *ctx, GLboolean red, GLboolean green,
                   GLboolean blue, GLboolean alpha )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glColorMask" );
      return;
   }
   ctx->Color.ColorMask[RCOMP] = red    ? 0xff : 0x0;
   ctx->Color.ColorMask[GCOMP] = green  ? 0xff : 0x0;
   ctx->Color.ColorMask[BCOMP] = blue   ? 0xff : 0x0;
   ctx->Color.ColorMask[ACOMP] = alpha  ? 0xff : 0x0;

   ctx->NewState |= NEW_RASTER_OPS;
}




/*
 * Apply glColorMask to a span of RGBA pixels.
 */
void gl_mask_rgba_span( GLcontext *ctx,
                        GLuint n, GLint x, GLint y, GLubyte rgba[][4] )
{
   GLubyte dest[MAX_WIDTH][4];
   GLuint srcMask = *((GLuint*)ctx->Color.ColorMask);
   GLuint dstMask = ~srcMask;
   GLuint *rgba32 = (GLuint *) rgba;
   GLuint *dest32 = (GLuint *) dest;
   GLuint i;

   gl_read_rgba_span( ctx, n, x, y, dest );

   for (i=0; i<n; i++) {
      rgba32[i] = (rgba32[i] & srcMask) | (dest32[i] & dstMask);
   }
}



/*
 * Apply glColorMask to an array of RGBA pixels.
 */
void gl_mask_rgba_pixels( GLcontext *ctx,
                          GLuint n, const GLint x[], const GLint y[],
                          GLubyte rgba[][4], const GLubyte mask[] )
{
   GLubyte dest[PB_SIZE][4];
   GLuint srcMask = *((GLuint*)ctx->Color.ColorMask);
   GLuint dstMask = ~srcMask;
   GLuint *rgba32 = (GLuint *) rgba;
   GLuint *dest32 = (GLuint *) dest;
   GLuint i;

   (*ctx->Driver.ReadRGBAPixels)( ctx, n, x, y, dest, mask );

   for (i=0; i<n; i++) {
      rgba32[i] = (rgba32[i] & srcMask) | (dest32[i] & dstMask);
   }
}



/*
 * Apply glIndexMask to a span of CI pixels.
 */
void gl_mask_index_span( GLcontext *ctx,
                         GLuint n, GLint x, GLint y, GLuint index[] )
{
   GLuint i;
   GLuint fbindexes[MAX_WIDTH];
   GLuint msrc, mdest;

   gl_read_index_span( ctx, n, x, y, fbindexes );

   msrc = ctx->Color.IndexMask;
   mdest = ~msrc;

   for (i=0;i<n;i++) {
      index[i] = (index[i] & msrc) | (fbindexes[i] & mdest);
   }
}



/*
 * Apply glIndexMask to an array of CI pixels.
 */
void gl_mask_index_pixels( GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLuint index[], const GLubyte mask[] )
{
   GLuint i;
   GLuint fbindexes[PB_SIZE];
   GLuint msrc, mdest;

   (*ctx->Driver.ReadCI32Pixels)( ctx, n, x, y, fbindexes, mask );

   msrc = ctx->Color.IndexMask;
   mdest = ~msrc;

   for (i=0;i<n;i++) {
      index[i] = (index[i] & msrc) | (fbindexes[i] & mdest);
   }
}

