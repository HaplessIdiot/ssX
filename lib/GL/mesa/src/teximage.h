/* Id: teximage.h,v 1.2 1999/02/26 08:52:37 martin Exp $ */

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
 * Log: teximage.h,v $
 * Revision 1.2  1999/02/26 08:52:37  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.1  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.0  1998/01/31 21:04:38  brianp
 * initial rev
 *
 */


#ifndef TEXIMAGE_H
#define TEXIMAGE_H


#include "types.h"


/*** Internal functions ***/


extern struct gl_texture_image *gl_alloc_texture_image( void );


extern void gl_free_texture_image( struct gl_texture_image *teximage );


extern struct gl_image *
gl_unpack_texsubimage( GLcontext *ctx, GLint width, GLint height,
                       GLenum format, GLenum type, const GLvoid *pixels );


extern struct gl_image *
gl_unpack_texsubimage3D( GLcontext *ctx, GLint width, GLint height,GLint depth,
                         GLenum format, GLenum type, const GLvoid *pixels );


extern struct gl_texture_image *
gl_unpack_texture( GLcontext *ctx,
                   GLint dimensions,
                   GLenum target,
                   GLint level,
                   GLint internalformat,
                   GLsizei width, GLsizei height,
                   GLint border,
                   GLenum format, GLenum type,
                   const GLvoid *pixels );

extern struct gl_texture_image *
gl_unpack_texture3D( GLcontext *ctx,
                     GLint dimensions,
                     GLenum target,
                     GLint level,
                     GLint internalformat,
                     GLsizei width, GLsizei height, GLsizei depth,
                     GLint border,
                     GLenum format, GLenum type,
                     const GLvoid *pixels );


extern void gl_tex_image_1D( GLcontext *ctx,
                             GLenum target, GLint level, GLint internalformat,
                             GLsizei width, GLint border, GLenum format,
                             GLenum type, const GLvoid *pixels );


extern void gl_tex_image_2D( GLcontext *ctx,
                             GLenum target, GLint level, GLint internalformat,
                             GLsizei width, GLint height, GLint border,
                             GLenum format, GLenum type,
                             const GLvoid *pixels );

extern void gl_tex_image_3D( GLcontext *ctx,
                             GLenum target, GLint level, GLint internalformat,
                             GLsizei width, GLint height, GLint depth,
                             GLint border,
                             GLenum format, GLenum type,
                             const GLvoid *pixels );


/*** API entry points ***/


extern void gl_TexImage1D( GLcontext *ctx,
                           GLenum target, GLint level, GLint internalformat,
                           GLsizei width, GLint border, GLenum format,
                           GLenum type, struct gl_image *teximage );


extern void gl_TexImage2D( GLcontext *ctx,
                           GLenum target, GLint level, GLint internalformat,
                           GLsizei width, GLsizei height, GLint border,
                           GLenum format, GLenum type,
                           struct gl_image *teximage );


extern void gl_TexImage3DEXT( GLcontext *ctx,
                              GLenum target, GLint level, GLint internalformat,
                              GLsizei width, GLsizei height, GLsizei depth,
                              GLint border,
                              GLenum format, GLenum type,
                              struct gl_image *teximage );


extern void gl_GetTexImage( GLcontext *ctx, GLenum target, GLint level,
                            GLenum format, GLenum type, GLvoid *pixels );



extern void gl_TexSubImage1D( GLcontext *ctx,
                              GLenum target, GLint level, GLint xoffset,
                              GLsizei width, GLenum format, GLenum type,
                              struct gl_image *image );


extern void gl_TexSubImage2D( GLcontext *ctx,
                              GLenum target, GLint level,
                              GLint xoffset, GLint yoffset,
                              GLsizei width, GLsizei height,
                              GLenum format, GLenum type,
                              struct gl_image *image );


extern void gl_TexSubImage3DEXT( GLcontext *ctx,
                                 GLenum target, GLint level,
                                 GLint xoffset, GLint yoffset, GLint zoffset,
                                 GLsizei width, GLsizei height, GLsizei depth,
                                 GLenum format, GLenum type,
                                 struct gl_image *image );


extern void gl_CopyTexImage1D( GLcontext *ctx,
                               GLenum target, GLint level,
                               GLenum internalformat,
                               GLint x, GLint y,
                               GLsizei width, GLint border );


extern void gl_CopyTexImage2D( GLcontext *ctx,
                               GLenum target, GLint level,
                               GLenum internalformat, GLint x, GLint y,
                               GLsizei width, GLsizei height,
                               GLint border );


extern void gl_CopyTexSubImage1D( GLcontext *ctx,
                                  GLenum target, GLint level,
                                  GLint xoffset, GLint x, GLint y,
                                  GLsizei width );


extern void gl_CopyTexSubImage2D( GLcontext *ctx,
                                  GLenum target, GLint level,
                                  GLint xoffset, GLint yoffset,
                                  GLint x, GLint y,
                                  GLsizei width, GLsizei height );


extern void gl_CopyTexSubImage3DEXT( GLcontext *ctx,
                                     GLenum target, GLint level,
                                     GLint xoffset, GLint yoffset,
                                     GLint zoffset,
                                     GLint x, GLint y,
                                     GLsizei width, GLsizei height );

#endif

