/* Id: texture.h,v 1.2 1999/02/26 08:52:38 martin Exp $ */

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
 * Log: texture.h,v $
 * Revision 1.2  1999/02/26 08:52:38  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.4  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.3  1998/11/29 21:49:10  brianp
 * gl_texgen() now takes objStride
 *
 * Revision 3.2  1998/02/20 04:53:37  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.1  1998/02/03 04:27:54  brianp
 * added texture lod clamping
 *
 * Revision 3.0  1998/01/31 21:04:38  brianp
 * initial rev
 *
 */


#ifndef TEXTURE_H
#define TEXTURE_H


#include "types.h"


extern void gl_texgen( GLcontext *ctx, GLint n,
                       const GLfloat *obj, GLuint objStride,
		       CONST GLfloat eye[][4],
                       CONST GLfloat normal[][3], GLfloat texcoord[][4],
                       GLuint textureSet);



extern void gl_set_texture_sampler( struct gl_texture_object *t );



extern void gl_texture_pixels( GLcontext *ctx, GLuint texSet, GLuint n,
                               const GLfloat s[], const GLfloat t[],
                               const GLfloat r[], GLfloat lambda[],
                               GLubyte rgba[][4] );


#endif

