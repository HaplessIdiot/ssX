/* Id: vbfill.h,v 1.2 1999/02/26 08:52:39 martin Exp $ */

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
 * Log: vbfill.h,v $
 * Revision 1.2  1999/02/26 08:52:39  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.2  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.1  1998/02/20 04:53:07  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.0  1998/01/31 21:06:45  brianp
 * initial rev
 *
 */


#ifndef VBFILL_H
#define VBFILL_H


#include "types.h"


extern void gl_Normal3f( GLcontext *ctx, GLfloat nx, GLfloat ny, GLfloat nz );

extern void gl_Normal3fv( GLcontext *ctx, const GLfloat *normal );


extern void gl_Indexf( GLcontext *ctx, GLfloat c );

extern void gl_Indexi( GLcontext *ctx, GLint c );


extern void gl_Color3f( GLcontext *ctx,
                        GLfloat red, GLfloat green, GLfloat blue );

extern void gl_Color3fv( GLcontext *ctx, const GLfloat *c );

extern void gl_Color4f( GLcontext *ctx,
                        GLfloat red, GLfloat green,
                        GLfloat blue, GLfloat alpha );

extern void gl_Color4fv( GLcontext *ctx, const GLfloat *c );

extern void gl_Color4ub( GLcontext *ctx,
                         GLubyte red, GLubyte green,
                         GLubyte blue, GLubyte alpha );

extern void gl_Color4ubv( GLcontext *ctx, const GLubyte *c );


extern void gl_ColorMat3f( GLcontext *ctx,
                           GLfloat red, GLfloat green, GLfloat blue );

extern void gl_ColorMat3fv( GLcontext *ctx, const GLfloat *c );

extern void gl_ColorMat3ub( GLcontext *ctx,
                            GLubyte red, GLubyte green, GLubyte blue );

extern void gl_ColorMat4f( GLcontext *ctx,
                           GLfloat red, GLfloat green,
                           GLfloat blue, GLfloat alpha );

extern void gl_ColorMat4fv( GLcontext *ctx, const GLfloat *c );

extern void gl_ColorMat4ub( GLcontext *ctx,
                            GLubyte red, GLubyte green,
                            GLubyte blue, GLubyte alpha );


extern void gl_TexCoord2f( GLcontext *ctx, GLfloat s, GLfloat t );

extern void gl_TexCoord4f( GLcontext *ctx,
                           GLfloat s, GLfloat t, GLfloat r, GLfloat q );

/* GL_SGIS_multitexture */
extern void gl_MultiTexCoord4f( GLcontext *ctx, GLenum target,
                                GLfloat s, GLfloat t, GLfloat r, GLfloat q );


extern void gl_EdgeFlag( GLcontext *ctx, GLboolean flag );



extern void gl_vertex2f_nop( GLcontext *ctx, GLfloat x, GLfloat y );

extern void gl_vertex3f_nop( GLcontext *ctx, GLfloat x, GLfloat y, GLfloat z );

extern void gl_vertex4f_nop( GLcontext *ctx,
                             GLfloat x, GLfloat y, GLfloat z, GLfloat w );

extern void gl_vertex3fv_nop( GLcontext *ctx, const GLfloat *v );



extern void gl_set_vertex_function( GLcontext *ctx );

extern void gl_set_color_function( GLcontext *ctx );



extern void gl_eval_vertex( GLcontext *ctx,
                            const GLfloat vertex[4], const GLfloat normal[3],
			    const GLubyte color[4], GLuint index,
                            const GLfloat texcoord[4] );


extern void gl_Begin( GLcontext *ctx, GLenum p );

extern void gl_End( GLcontext *ctx );


#endif

