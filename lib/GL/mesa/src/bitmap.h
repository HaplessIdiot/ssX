/* Id: bitmap.h,v 1.2 1999/02/26 08:52:31 martin Exp $ */

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
 * Log: bitmap.h,v $
 * Revision 1.2  1999/02/26 08:52:31  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.3  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.2  1998/02/15 01:32:59  brianp
 * updated driver bitmap function
 *
 * Revision 3.1  1998/02/08 20:23:49  brianp
 * lots of bitmap rendering changes
 *
 * Revision 3.0  1998/01/31 20:47:29  brianp
 * initial rev
 *
 */


#ifndef BITMAP_H
#define BITMAP_H


#include "types.h"


extern GLboolean gl_direct_bitmap( GLcontext *ctx, 
                                   GLsizei width, GLsizei height,
                                   GLfloat xorig, GLfloat yorig,
                                   GLfloat xmove, GLfloat ymove,
                                   const GLubyte *bitmap );


extern void gl_Bitmap( GLcontext* ctx,
                       GLsizei width, GLsizei height,
                       GLfloat xorig, GLfloat yorig,
                       GLfloat xmove, GLfloat ymove,
                       struct gl_image *bitmap );


#endif
