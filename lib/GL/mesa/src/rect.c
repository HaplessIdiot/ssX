/* Id: rect.c,v 1.3 1999/02/26 08:52:36 martin Exp $ */

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
/* $XFree86: xc/lib/GL/mesa/src/rect.c,v 1.0tsi Exp $ */

/*
 * Log: rect.c,v $
 * Revision 1.3  1999/02/26 08:52:36  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.2  1999/02/24 22:48:07  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.1  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.0  1998/01/31 21:02:29  brianp
 * initial rev
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "macros.h"
#include "rect.h"
#include "vbfill.h"
#endif



/*
 * Execute a glRect*() function.
 */
void gl_Rectf( GLcontext *ctx, GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   /*
    * TODO: we could examine a bunch of state variables and ultimately
    * call the Driver->RectFunc() function to draw a screen-aligned
    * filled rectangle.  Someday...
    */

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glRect" );
      return;
   }
   gl_Begin( ctx, GL_QUADS );
   (*ctx->Exec.Vertex2f)( ctx, x1, y1 );
   (*ctx->Exec.Vertex2f)( ctx, x2, y1 );
   (*ctx->Exec.Vertex2f)( ctx, x2, y2 );
   (*ctx->Exec.Vertex2f)( ctx, x1, y2 );
   gl_End( ctx );
}
