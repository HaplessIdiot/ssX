/* Id: apiext.c,v 1.3 1999/02/26 08:52:30 martin Exp $ */

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
 * Log: apiext.c,v $
 * Revision 1.3  1999/02/26 08:52:30  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.9  1999/02/24 22:48:04  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.8  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.7  1999/01/03 03:28:39  brianp
 * now using GLAPIENTRY keyword (Ted Jump)
 *
 * Revision 3.6  1998/11/03 01:40:37  brianp
 * implemented GL_ARB_multitexture
 *
 * Revision 3.5  1998/10/23 01:03:47  brianp
 * added GL_ARB_multitexture extension
 *
 * Revision 3.4  1998/10/23 00:44:11  brianp
 * removed GL_EXT_multitexture
 *
 * Revision 3.3  1998/10/05 00:40:15  brianp
 * added GL_INGR_blend_func_separate extension
 *
 * Revision 3.2  1998/06/07 22:18:52  brianp
 * implemented GL_EXT_multitexture extension
 *
 * Revision 3.1  1998/03/27 03:30:36  brianp
 * fixed G++ warnings
 *
 * Revision 3.0  1998/02/20 04:45:50  brianp
 * implemented GL_SGIS_multitexture
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/apiext.c,v 1.2 1999/03/14 03:20:39 dawes Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <stdio.h>
#include <stdlib.h>
#else
#include "GL/xf86glx.h"
#endif
#include "api.h"
#include "context.h"
#include "types.h"
#endif



/*
 * Extension API functions
 */



/*
 * GL_EXT_blend_minmax
 */

void GLAPIENTRY glBlendEquationEXT( GLenum mode )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.BlendEquation)(CC, mode);
}




/*
 * GL_EXT_blend_color
 */

void GLAPIENTRY glBlendColorEXT( GLclampf red, GLclampf green,
                               GLclampf blue, GLclampf alpha )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.BlendColor)(CC, red, green, blue, alpha);
}




/*
 * GL_EXT_vertex_array
 */

void GLAPIENTRY glVertexPointerEXT( GLint size, GLenum type, GLsizei stride,
                                  GLsizei count, const GLvoid *ptr )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.VertexPointer)(CC, size, type, stride, ptr);
   (void) count;
}


void GLAPIENTRY glNormalPointerEXT( GLenum type, GLsizei stride, GLsizei count,
                                  const GLvoid *ptr )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.NormalPointer)(CC, type, stride, ptr);
   (void) count;
}


void GLAPIENTRY glColorPointerEXT( GLint size, GLenum type, GLsizei stride,
                                 GLsizei count, const GLvoid *ptr )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.ColorPointer)(CC, size, type, stride, ptr);
   (void) count;
}


void GLAPIENTRY glIndexPointerEXT( GLenum type, GLsizei stride,
                                 GLsizei count, const GLvoid *ptr )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.IndexPointer)(CC, type, stride, ptr);
   (void) count;
}


void GLAPIENTRY glTexCoordPointerEXT( GLint size, GLenum type, GLsizei stride,
                                    GLsizei count, const GLvoid *ptr )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.TexCoordPointer)(CC, size, type, stride, ptr);
   (void) count;
}


void GLAPIENTRY glEdgeFlagPointerEXT( GLsizei stride, GLsizei count,
                                    const GLboolean *ptr )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.EdgeFlagPointer)(CC, stride, ptr);
   (void) count;
}


void GLAPIENTRY glGetPointervEXT( GLenum pname, GLvoid **params )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.GetPointerv)(CC, pname, params);
}


void GLAPIENTRY glArrayElementEXT( GLint i )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.ArrayElement)(CC, i);
}


void GLAPIENTRY glDrawArraysEXT( GLenum mode, GLint first, GLsizei count )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.DrawArrays)(CC, mode, first, count);
}




/*
 * GL_EXT_texture_object
 */

GLboolean GLAPIENTRY glAreTexturesResidentEXT( GLsizei n, const GLuint *textures,
                                             GLboolean *residences )
{
   return glAreTexturesResident( n, textures, residences );
}


void GLAPIENTRY glBindTextureEXT( GLenum target, GLuint texture )
{
   glBindTexture( target, texture );
}


void GLAPIENTRY glDeleteTexturesEXT( GLsizei n, const GLuint *textures)
{
   glDeleteTextures( n, textures );
}


void GLAPIENTRY glGenTexturesEXT( GLsizei n, GLuint *textures )
{
   glGenTextures( n, textures );
}


GLboolean GLAPIENTRY glIsTextureEXT( GLuint texture )
{
   return glIsTexture( texture );
}


void GLAPIENTRY glPrioritizeTexturesEXT( GLsizei n, const GLuint *textures,
                                       const GLclampf *priorities )
{
   glPrioritizeTextures( n, textures, priorities );
}




/*
 * GL_EXT_texture3D
 */

void GLAPIENTRY glCopyTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset,
                                      GLint yoffset, GLint zoffset,
                                      GLint x, GLint y, GLsizei width,
                                      GLsizei height )
{
   glCopyTexSubImage3D(target, level, xoffset, yoffset, zoffset,
                       x, y, width, height);
}



void GLAPIENTRY glTexImage3DEXT( GLenum target, GLint level, GLenum internalformat,
                               GLsizei width, GLsizei height, GLsizei depth,
                               GLint border, GLenum format, GLenum type,
                               const GLvoid *pixels )
{
   glTexImage3D(target, level, internalformat, width, height, depth,
                border, format, type, pixels);
}


void GLAPIENTRY glTexSubImage3DEXT( GLenum target, GLint level, GLint xoffset,
                                  GLint yoffset, GLint zoffset, GLsizei width,
                                  GLsizei height, GLsizei depth, GLenum format,
                                  GLenum type, const GLvoid *pixels )
{
   glTexSubImage3D(target, level, xoffset, yoffset, zoffset,
                   width, height, depth, format, type, pixels);
}




/*
 * GL_EXT_point_parameters
 */

void GLAPIENTRY glPointParameterfEXT( GLenum pname, GLfloat param )
{
   GLfloat params[3];
   GET_CONTEXT;
   CHECK_CONTEXT;
   params[0] = param;
   params[1] = 0.0;
   params[2] = 0.0;
   (*CC->API.PointParameterfvEXT)(CC, pname, params);
}


void GLAPIENTRY glPointParameterfvEXT( GLenum pname, const GLfloat *params )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PointParameterfvEXT)(CC, pname, params);
}




#ifdef GL_MESA_window_pos
/*
 * Mesa implementation of glWindowPos*MESA()
 */
void GLAPIENTRY glWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.WindowPos4fMESA)( CC, x, y, z, w );
}
#else
/* Implementation in winpos.c is used */
#endif


void GLAPIENTRY glWindowPos2iMESA( GLint x, GLint y )
{
   glWindowPos4fMESA( (GLfloat) x, (GLfloat) y, 0.0F, 1.0F );
}

void GLAPIENTRY glWindowPos2sMESA( GLshort x, GLshort y )
{
   glWindowPos4fMESA( (GLfloat) x, (GLfloat) y, 0.0F, 1.0F );
}

void GLAPIENTRY glWindowPos2fMESA( GLfloat x, GLfloat y )
{
   glWindowPos4fMESA( x, y, 0.0F, 1.0F );
}

void GLAPIENTRY glWindowPos2dMESA( GLdouble x, GLdouble y )
{
   glWindowPos4fMESA( (GLfloat) x, (GLfloat) y, 0.0F, 1.0F );
}

void GLAPIENTRY glWindowPos2ivMESA( const GLint *p )
{
   glWindowPos4fMESA( (GLfloat) p[0], (GLfloat) p[1], 0.0F, 1.0F );
}

void GLAPIENTRY glWindowPos2svMESA( const GLshort *p )
{
   glWindowPos4fMESA( (GLfloat) p[0], (GLfloat) p[1], 0.0F, 1.0F );
}

void GLAPIENTRY glWindowPos2fvMESA( const GLfloat *p )
{
   glWindowPos4fMESA( p[0], p[1], 0.0F, 1.0F );
}

void GLAPIENTRY glWindowPos2dvMESA( const GLdouble *p )
{
   glWindowPos4fMESA( (GLfloat) p[0], (GLfloat) p[1], 0.0F, 1.0F );
}

void GLAPIENTRY glWindowPos3iMESA( GLint x, GLint y, GLint z )
{
   glWindowPos4fMESA( (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F );
}

void GLAPIENTRY glWindowPos3sMESA( GLshort x, GLshort y, GLshort z )
{
   glWindowPos4fMESA( (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F );
}

void GLAPIENTRY glWindowPos3fMESA( GLfloat x, GLfloat y, GLfloat z )
{
   glWindowPos4fMESA( x, y, z, 1.0F );
}

void GLAPIENTRY glWindowPos3dMESA( GLdouble x, GLdouble y, GLdouble z )
{
   glWindowPos4fMESA( (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F );
}

void GLAPIENTRY glWindowPos3ivMESA( const GLint *p )
{
   glWindowPos4fMESA( (GLfloat) p[0], (GLfloat) p[1], (GLfloat) p[2], 1.0F );
}

void GLAPIENTRY glWindowPos3svMESA( const GLshort *p )
{
   glWindowPos4fMESA( (GLfloat) p[0], (GLfloat) p[1], (GLfloat) p[2], 1.0F );
}

void GLAPIENTRY glWindowPos3fvMESA( const GLfloat *p )
{
   glWindowPos4fMESA( p[0], p[1], p[2], 1.0F );
}

void GLAPIENTRY glWindowPos3dvMESA( const GLdouble *p )
{
   glWindowPos4fMESA( (GLfloat) p[0], (GLfloat) p[1], (GLfloat) p[2], 1.0F );
}

void GLAPIENTRY glWindowPos4iMESA( GLint x, GLint y, GLint z, GLint w )
{
   glWindowPos4fMESA( (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w );
}

void GLAPIENTRY glWindowPos4sMESA( GLshort x, GLshort y, GLshort z, GLshort w )
{
   glWindowPos4fMESA( (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w );
}

void GLAPIENTRY glWindowPos4dMESA( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
   glWindowPos4fMESA( (GLfloat) x, (GLfloat) y, (GLfloat) z, (GLfloat) w );
}


void GLAPIENTRY glWindowPos4ivMESA( const GLint *p )
{
   glWindowPos4fMESA( (GLfloat) p[0], (GLfloat) p[1],
                      (GLfloat) p[2], (GLfloat) p[3] );
}

void GLAPIENTRY glWindowPos4svMESA( const GLshort *p )
{
   glWindowPos4fMESA( (GLfloat) p[0], (GLfloat) p[1],
                      (GLfloat) p[2], (GLfloat) p[3] );
}

void GLAPIENTRY glWindowPos4fvMESA( const GLfloat *p )
{
   glWindowPos4fMESA( p[0], p[1], p[2], p[3] );
}

void GLAPIENTRY glWindowPos4dvMESA( const GLdouble *p )
{
   glWindowPos4fMESA( (GLfloat) p[0], (GLfloat) p[1],
                      (GLfloat) p[2], (GLfloat) p[3] );
}




/*
 * GL_MESA_resize_buffers
 */

/*
 * Called by user application when window has been resized.
 */
void GLAPIENTRY glResizeBuffersMESA( void )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.ResizeBuffersMESA)( CC );
}



/*
 * GL_SGIS_multitexture
 */

void GLAPIENTRY glMultiTexCoord1dSGIS(GLenum target, GLdouble s)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1dvSGIS(GLenum target, const GLdouble *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1fSGIS(GLenum target, GLfloat s)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1fvSGIS(GLenum target, const GLfloat *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1iSGIS(GLenum target, GLint s)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1ivSGIS(GLenum target, const GLint *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1sSGIS(GLenum target, GLshort s)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1svSGIS(GLenum target, const GLshort *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2dSGIS(GLenum target, GLdouble s, GLdouble t)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2dvSGIS(GLenum target, const GLdouble *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2fSGIS(GLenum target, GLfloat s, GLfloat t)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2fvSGIS(GLenum target, const GLfloat *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2iSGIS(GLenum target, GLint s, GLint t)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2ivSGIS(GLenum target, const GLint *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2sSGIS(GLenum target, GLshort s, GLshort t)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2svSGIS(GLenum target, const GLshort *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord3dSGIS(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, 1.0 );
}

void GLAPIENTRY glMultiTexCoord3dvSGIS(GLenum target, const GLdouble *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], 1.0 );
}

void GLAPIENTRY glMultiTexCoord3fSGIS(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, 1.0 );
}

void GLAPIENTRY glMultiTexCoord3fvSGIS(GLenum target, const GLfloat *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], 1.0 );
}

void GLAPIENTRY glMultiTexCoord3iSGIS(GLenum target, GLint s, GLint t, GLint r)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, 1.0 );
}

void GLAPIENTRY glMultiTexCoord3ivSGIS(GLenum target, const GLint *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], 1.0 );
}

void GLAPIENTRY glMultiTexCoord3sSGIS(GLenum target, GLshort s, GLshort t, GLshort r)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, 1.0 );
}

void GLAPIENTRY glMultiTexCoord3svSGIS(GLenum target, const GLshort *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], 1.0 );
}

void GLAPIENTRY glMultiTexCoord4dSGIS(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, q );
}

void GLAPIENTRY glMultiTexCoord4dvSGIS(GLenum target, const GLdouble *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], v[3] );
}

void GLAPIENTRY glMultiTexCoord4fSGIS(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, q );
}

void GLAPIENTRY glMultiTexCoord4fvSGIS(GLenum target, const GLfloat *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], v[3] );
}

void GLAPIENTRY glMultiTexCoord4iSGIS(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, q );
}

void GLAPIENTRY glMultiTexCoord4ivSGIS(GLenum target, const GLint *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], v[3] );
}

void GLAPIENTRY glMultiTexCoord4sSGIS(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, q );
}

void GLAPIENTRY glMultiTexCoord4svSGIS(GLenum target, const GLshort *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], v[3] );
}



void GLAPIENTRY glMultiTexCoordPointerSGIS(GLenum target, GLint size, GLenum type,
                                         GLsizei stride, const GLvoid *ptr)
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.MultiTexCoordPointer)(CC, target, size, type, stride, ptr);
}



void GLAPIENTRY glSelectTextureSGIS(GLenum target)
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.SelectTextureSGIS)(CC, target);
}



void GLAPIENTRY glSelectTextureCoordSetSGIS(GLenum target)
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.SelectTextureCoordSet)(CC, target);
}



/*
 * GL_ARB_multitexture
 */

void GLAPIENTRY glActiveTextureARB(GLenum texture)
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.ActiveTexture)(CC, texture);
}

void GLAPIENTRY glClientActiveTextureARB(GLenum texture)
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.ClientActiveTexture)(CC, texture);
}

void GLAPIENTRY glMultiTexCoord1dARB(GLenum target, GLdouble s)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1dvARB(GLenum target, const GLdouble *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1fARB(GLenum target, GLfloat s)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1fvARB(GLenum target, const GLfloat *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1iARB(GLenum target, GLint s)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1ivARB(GLenum target, const GLint *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1sARB(GLenum target, GLshort s)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord1svARB(GLenum target, const GLshort *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], 0.0, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2dvARB(GLenum target, const GLdouble *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2fvARB(GLenum target, const GLfloat *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2iARB(GLenum target, GLint s, GLint t)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2ivARB(GLenum target, const GLint *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2sARB(GLenum target, GLshort s, GLshort t)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord2svARB(GLenum target, const GLshort *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], 0.0, 1.0 );
}

void GLAPIENTRY glMultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, 1.0 );
}

void GLAPIENTRY glMultiTexCoord3dvARB(GLenum target, const GLdouble *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], 1.0 );
}

void GLAPIENTRY glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, 1.0 );
}

void GLAPIENTRY glMultiTexCoord3fvARB(GLenum target, const GLfloat *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], 1.0 );
}

void GLAPIENTRY glMultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, 1.0 );
}

void GLAPIENTRY glMultiTexCoord3ivARB(GLenum target, const GLint *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], 1.0 );
}

void GLAPIENTRY glMultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, 1.0 );
}

void GLAPIENTRY glMultiTexCoord3svARB(GLenum target, const GLshort *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], 1.0 );
}

void GLAPIENTRY glMultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, q );
}

void GLAPIENTRY glMultiTexCoord4dvARB(GLenum target, const GLdouble *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], v[3] );
}

void GLAPIENTRY glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, q );
}

void GLAPIENTRY glMultiTexCoord4fvARB(GLenum target, const GLfloat *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], v[3] );
}

void GLAPIENTRY glMultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, q );
}

void GLAPIENTRY glMultiTexCoord4ivARB(GLenum target, const GLint *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], v[3] );
}

void GLAPIENTRY glMultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, s, t, r, q );
}

void GLAPIENTRY glMultiTexCoord4svARB(GLenum target, const GLshort *v)
{
   GET_CONTEXT;
   (*CC->API.MultiTexCoord4f)( CC, target, v[0], v[1], v[2], v[3] );
}



/*
 * GL_INGR_blend_func_separate
 */

void GLAPIENTRY glBlendFuncSeparateINGR( GLenum sfactorRGB,
                                       GLenum dfactorRGB,
                                       GLenum sfactorAlpha, 
                                       GLenum dfactorAlpha )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.BlendFuncSeparate)( CC, sfactorRGB, dfactorRGB,
                                 sfactorAlpha, dfactorAlpha);
}
