/* Id: quads.c,v 1.3 1999/02/26 08:52:36 martin Exp $ */

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
 * Log: quads.c,v $
 * Revision 1.3  1999/02/26 08:52:36  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.4  1999/02/24 22:48:07  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.3  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.2  1998/05/31 23:50:36  brianp
 * cleaned up a few Solaris compiler warnings
 *
 * Revision 3.1  1998/03/27 04:17:31  brianp
 * fixed G++ warnings
 *
 * Revision 3.0  1998/01/31 21:02:06  brianp
 * initial rev
 *
 */


/*
 * Quadrilateral rendering functions.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "types.h"
#include "quads.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



/*
 * At this time there is no quadrilateral optimization.  Just call the
 * triangle function twice.
 * v0, v1, v2, v3 in CCW order = front facing.
 */
static void basic_quad( GLcontext *ctx,
                        GLuint v0, GLuint v1, GLuint v2, GLuint v3, GLuint pv )
{
   (*ctx->Driver.TriangleFunc)( ctx, v0, v1, v3, pv );
   (*ctx->Driver.TriangleFunc)( ctx, v1, v2, v3, pv );
}



/*
 * Draw nothing (NULL raster mode)
 */
static void null_quad( GLcontext *ctx,
                       GLuint v0, GLuint v1, GLuint v2, GLuint v3, GLuint pv )
{
   (void) ctx;
   (void) v0;
   (void) v1;
   (void) v2;
   (void) v3;
   (void) pv;
}



void gl_set_quad_function( GLcontext *ctx )
{
   if (ctx->RenderMode==GL_RENDER) {
      if (ctx->NoRaster) {
         ctx->Driver.QuadFunc = null_quad;
      }
      else if (ctx->Driver.QuadFunc) {
         /* Device driver will draw quads. */
      }
      else {
         ctx->Driver.QuadFunc = basic_quad;
      }
   }
   else {
      /* if in feedback or selection mode we can fall back to triangle code */
      ctx->Driver.QuadFunc = basic_quad;
   }      
}


