/* Id: drawpix.h,v 1.2 1999/02/26 08:52:32 martin Exp $ */

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
 * Log: drawpix.h,v $
 * Revision 1.2  1999/02/26 08:52:32  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.1  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.0  1998/01/31 20:51:03  brianp
 * initial rev
 *
 */


#ifndef DRAWPIXELS_H
#define DRAWPIXELS_H


#include "types.h"


extern GLboolean
gl_direct_DrawPixels( GLcontext *ctx, 
                      const struct gl_pixelstore_attrib *unpack,
                      GLsizei width, GLsizei height,
                      GLenum format, GLenum type, const GLvoid *pixels );


#if 000
extern void gl_DrawPixels( GLcontext *ctx, GLsizei width, GLsizei height,
                           GLenum format, GLenum type, const GLvoid *pixels );
#endif


extern void gl_DrawPixels( GLcontext *ctx, struct gl_image *image );


#endif
