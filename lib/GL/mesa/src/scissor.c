/* Id: scissor.c,v 1.3 1999/02/26 08:52:36 martin Exp $ */

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
 * Log: scissor.c,v $
 * Revision 1.3  1999/02/26 08:52:36  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.3  1999/02/24 22:48:07  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.2  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.1  1998/02/08 20:17:42  brianp
 * removed unneeded headers
 *
 * Revision 3.0  1998/01/31 21:03:42  brianp
 * initial rev
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "context.h"
#include "macros.h"
#include "scissor.h"
#include "types.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif


void gl_Scissor( GLcontext *ctx,
                 GLint x, GLint y, GLsizei width, GLsizei height )
{
   if (width<0 || height<0) {
      gl_error( ctx, GL_INVALID_VALUE, "glScissor" );
      return;
   }
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glBegin" );
      return;
   }

   if (x!=ctx->Scissor.X || y!=ctx->Scissor.Y || 
       width!=ctx->Scissor.Width || height!=ctx->Scissor.Height) {
      ctx->Scissor.X = x;
      ctx->Scissor.Y = y;
      ctx->Scissor.Width = width;
      ctx->Scissor.Height = height;
      ctx->NewState |= NEW_ALL;  /* TODO: this is overkill */
   }
}



/*
 * Apply the scissor test to a span of pixels.
 * Return:  0 = all pixels in the span are outside the scissor box.
 *          1 = one or more pixels passed the scissor test.
 */
GLint gl_scissor_span( GLcontext *ctx,
                       GLuint n, GLint x, GLint y, GLubyte mask[] )
{
   /* first check if whole span is outside the scissor box */
   if (y<ctx->Buffer->Ymin || y>ctx->Buffer->Ymax
       || x>ctx->Buffer->Xmax || x+(GLint)n-1<ctx->Buffer->Xmin) {
      return 0;
   }
   else {
      GLint i;
      GLint xMin = ctx->Buffer->Xmin;
      GLint xMax = ctx->Buffer->Xmax;
      for (i=0; x+i < xMin; i++) {
         mask[i] = 0;
      }
      for (i=(GLint)n-1; x+i > xMax; i--) {
         mask[i] = 0;
      }

      return 1;
   }
}




/*
 * Apply the scissor test to an array of pixels.
 */
GLuint gl_scissor_pixels( GLcontext *ctx,
                          GLuint n, const GLint x[], const GLint y[],
                          GLubyte mask[] )
{
   GLint xmin = ctx->Buffer->Xmin;
   GLint xmax = ctx->Buffer->Xmax;
   GLint ymin = ctx->Buffer->Ymin;
   GLint ymax = ctx->Buffer->Ymax;
   GLuint i;

   for (i=0;i<n;i++) {
      mask[i] &= (x[i]>=xmin) & (x[i]<=xmax) & (y[i]>=ymin) & (y[i]<=ymax);
   }

   return 1;
}

