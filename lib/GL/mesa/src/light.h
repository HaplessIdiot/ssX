/* Id: light.h,v 1.2 1999/02/26 08:52:34 martin Exp $ */

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
 * Log: light.h,v $
 * Revision 1.2  1999/02/26 08:52:34  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.2  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.1  1998/10/29 02:28:13  brianp
 * incorporated Keith Whitwell's transformation optimizations
 *
 * Revision 3.0  1998/01/31 20:54:56  brianp
 * initial rev
 *
 */


#ifndef LIGHT_H
#define LIGHT_H


#include "types.h"


extern void gl_ShadeModel( GLcontext *ctx, GLenum mode );

extern void gl_ColorMaterial( GLcontext *ctx, GLenum face, GLenum mode );

extern void gl_Lightfv( GLcontext *ctx,
                        GLenum light, GLenum pname, const GLfloat *params,
                        GLint nparams );

extern void gl_LightModelfv( GLcontext *ctx,
                             GLenum pname, const GLfloat *params );


extern GLuint gl_material_bitmask( GLenum face, GLenum pname );

extern void gl_set_material( GLcontext *ctx, GLuint bitmask,
                             const GLfloat *params);

extern void gl_Materialfv( GLcontext *ctx,
                           GLenum face, GLenum pname, const GLfloat *params );



extern void gl_GetLightfv( GLcontext *ctx,
                           GLenum light, GLenum pname, GLfloat *params );

extern void gl_GetLightiv( GLcontext *ctx,
                           GLenum light, GLenum pname, GLint *params );


extern void gl_GetMaterialfv( GLcontext *ctx,
                              GLenum face, GLenum pname, GLfloat *params );

extern void gl_GetMaterialiv( GLcontext *ctx,
                              GLenum face, GLenum pname, GLint *params );


extern void gl_compute_spot_exp_table( struct gl_light *l );

extern void gl_compute_material_shine_table( struct gl_material *m );

extern void gl_update_lighting( GLcontext *ctx );

extern void gl_compute_light_positions( GLcontext *ctx );

extern void gl_update_shade_context( GLcontext *ctx, 
				     struct gl_shade_context *sc );

#endif

