/* Id: image.h,v 1.2 1999/02/26 08:52:33 martin Exp $ */

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
 * Log: image.h,v $
 * Revision 1.2  1999/02/26 08:52:33  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.6  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.5  1999/01/17 21:36:50  brianp
 * added gl_bytes_per_pixel() and gl_is_legal_format_and_type()
 * fixed bugs related to packed pixel formats
 *
 * Revision 3.4  1998/08/21 02:41:39  brianp
 * added gl_pack/unpack_polygon_stipple()
 *
 * Revision 3.3  1998/07/26 17:24:18  brianp
 * replaced const with CONST because of IRIX cc warning
 *
 * Revision 3.2  1998/07/17 03:24:53  brianp
 * added gl_pack_rgba_span()
 *
 * Revision 3.1  1998/02/08 20:21:22  brianp
 * added gl_unpack_bitmap()
 *
 * Revision 3.0  1998/01/31 20:54:19  brianp
 * initial rev
 *
 */


#ifndef IMAGE_H
#define IMAGE_H


#include "types.h"


extern void gl_flip_bytes( GLubyte *p, GLuint n );


extern void gl_swap2( GLushort *p, GLuint n );

extern void gl_swap4( GLuint *p, GLuint n );


extern GLint gl_sizeof_type( GLenum type );

extern GLint gl_sizeof_packed_type( GLenum type );

extern GLint gl_components_in_format( GLenum format );

extern GLint gl_bytes_per_pixel( GLenum format, GLenum type );

extern GLboolean gl_is_legal_format_and_type( GLenum format, GLenum type );


extern GLvoid *gl_pixel_addr_in_image(
                                const struct gl_pixelstore_attrib *packing,
                                const GLvoid *image, GLsizei width,
                                GLsizei height, GLenum format, GLenum type,
                                GLint img, GLint row, GLint column );


extern struct gl_image *gl_unpack_bitmap( GLcontext *ctx,
                                          GLsizei width, GLsizei height,
                                          const GLubyte *bitmap );


extern void gl_unpack_polygon_stipple( const GLcontext *ctx,
                                       const GLubyte *pattern,
                                       GLuint dest[32] );


extern void gl_pack_polygon_stipple( const GLcontext *ctx,
                                     const GLuint pattern[32],
                                     GLubyte *dest );


extern struct gl_image *gl_unpack_image( GLcontext *ctx,
                                  GLint width, GLint height,
                                  GLenum srcFormat, GLenum srcType,
                                  const GLvoid *pixels );


struct gl_image *gl_unpack_image3D( GLcontext *ctx,
                                    GLint width, GLint height,GLint depth,
                                    GLenum srcFormat, GLenum srcType,
                                    const GLvoid *pixels );


extern void gl_pack_rgba_span( const GLcontext *ctx,
                               GLuint n, CONST GLubyte rgba[][4],
                               GLenum format, GLenum type, GLvoid *dest);


extern void gl_free_image( struct gl_image *image );


extern GLboolean gl_image_error_test( GLcontext *ctx,
                                      const struct gl_image *image,
                                      const char *msg );


#endif
