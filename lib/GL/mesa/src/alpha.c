/* Id: alpha.c,v 1.3 1999/02/26 08:52:30 martin Exp $ */

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
 * Log: alpha.c,v $
 * Revision 1.3  1999/02/26 08:52:30  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.6  1999/02/24 22:48:04  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.5  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.4  1998/11/17 02:51:35  brianp
 * added a bunch of new device driver functions (Keith Harrison)
 *
 * Revision 3.3  1998/07/29 04:02:30  brianp
 * fixed rounding error in computing integer alpha reference value
 *
 * Revision 3.2  1998/03/28 03:57:13  brianp
 * added CONST macro to fix IRIX compilation problems
 *
 * Revision 3.1  1998/02/13 03:23:04  brianp
 * AlphaRef is now a GLubyte
 *
 * Revision 3.0  1998/01/31 20:44:19  brianp
 * initial rev
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "alpha.h"
#include "context.h"
#include "types.h"
#include "macros.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



void gl_AlphaFunc( GLcontext* ctx, GLenum func, GLclampf ref )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glAlphaFunc" );
      return;
   }
   switch (func) {
      case GL_NEVER:
      case GL_LESS:
      case GL_EQUAL:
      case GL_LEQUAL:
      case GL_GREATER:
      case GL_NOTEQUAL:
      case GL_GEQUAL:
      case GL_ALWAYS:
         ctx->Color.AlphaFunc = func;
         ctx->Color.AlphaRef = (GLubyte) (CLAMP(ref, 0.0F, 1.0F) * 255.0F + 0.5F);
         if (ctx->Driver.AlphaFunc) {
            (*ctx->Driver.AlphaFunc)(ctx, func, ctx->Color.AlphaRef);
         }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glAlphaFunc(func)" );
         break;
   }
}




/*
 * Apply the alpha test to a span of pixels.
 * In:  rgba - array of pixels
 * In/Out:  mask - current pixel mask.  Pixels which fail the alpha test
 *                 will set the corresponding mask flag to 0.
 * Return:  0 = all pixels in the span failed the alpha test.
 *          1 = one or more pixels passed the alpha test.
 */
GLint gl_alpha_test( const GLcontext* ctx,
                     GLuint n, CONST GLubyte rgba[][4], GLubyte mask[] )
{
   GLuint i;
   GLubyte ref = ctx->Color.AlphaRef;

   /* switch cases ordered from most frequent to less frequent */
   switch (ctx->Color.AlphaFunc) {
      case GL_LESS:
         for (i=0;i<n;i++) {
	    mask[i] &= (rgba[i][ACOMP] < ref);
	 }
	 return 1;
      case GL_LEQUAL:
         for (i=0;i<n;i++) {
	    mask[i] &= (rgba[i][ACOMP] <= ref);
	 }
	 return 1;
      case GL_GEQUAL:
         for (i=0;i<n;i++) {
	    mask[i] &= (rgba[i][ACOMP] >= ref);
	 }
	 return 1;
      case GL_GREATER:
         for (i=0;i<n;i++) {
	    mask[i] &= (rgba[i][ACOMP] > ref);
	 }
	 return 1;
      case GL_NOTEQUAL:
         for (i=0;i<n;i++) {
	    mask[i] &= (rgba[i][ACOMP] != ref);
	 }
	 return 1;
      case GL_EQUAL:
         for (i=0;i<n;i++) {
	    mask[i] &= (rgba[i][ACOMP] == ref);
	 }
	 return 1;
      case GL_ALWAYS:
	 /* do nothing */
	 return 1;
      case GL_NEVER:
         /* caller should check for zero! */
	 return 0;
      default:
	 gl_problem( ctx, "Invalid alpha test in gl_alpha_test" );
         return 0;
   }
   /* Never get here */
   /*return 1;*/
}
