/* Id: api2.c,v 1.3 1999/02/26 08:52:30 martin Exp $ */

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
 * Log: api2.c,v $
 * Revision 1.3  1999/02/26 08:52:30  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.12  1999/02/24 22:48:04  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.11  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.10  1999/01/03 03:28:39  brianp
 * now using GLAPIENTRY keyword (Ted Jump)
 *
 * Revision 3.9  1998/11/27 14:12:05  brianp
 * changed glTexImage3D() internalFormat from GLenum to GLint
 *
 * Revision 3.8  1998/11/27 13:59:26  brianp
 * fixed multi-texture typo in glTexImage2D
 *
 * Revision 3.7  1998/11/19 03:07:14  brianp
 * clean-up in glTexImage2D()
 *
 * Revision 3.6  1998/08/21 02:43:52  brianp
 * implemented true packing/unpacking of polygon stipples
 *
 * Revision 3.5  1998/07/26 02:32:43  brianp
 * removed unused variable
 *
 * Revision 3.4  1998/03/27 03:30:36  brianp
 * fixed G++ warnings
 *
 * Revision 3.3  1998/03/10 01:26:57  brianp
 * updated for David's v0.23 fxmesa driver
 *
 * Revision 3.2  1998/02/20 04:49:19  brianp
 * move extension functions into apiext.c
 *
 * Revision 3.1  1998/02/06 01:57:42  brianp
 * added GL 1.2 3-D texture enums and functions
 *
 * Revision 3.0  1998/01/31 20:43:12  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/api2.c,v 1.0tsi Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <stdio.h>
#include <stdlib.h>
#endif
#include "api.h"
#include "context.h"
#include "image.h"
#include "macros.h"
#include "matrix.h"
#include "teximage.h"
#include "types.h"
#include "vb.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif


/*
 * Part 2 of API functions
 */


void GLAPIENTRY glOrtho( GLdouble left, GLdouble right,
                       GLdouble bottom, GLdouble top,
                       GLdouble nearval, GLdouble farval )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Ortho)(CC, left, right, bottom, top, nearval, farval);
}


void GLAPIENTRY glPassThrough( GLfloat token )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PassThrough)(CC, token);
}


void GLAPIENTRY glPixelMapfv( GLenum map, GLint mapsize, const GLfloat *values )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PixelMapfv)( CC, map, mapsize, values );
}


void GLAPIENTRY glPixelMapuiv( GLenum map, GLint mapsize, const GLuint *values )
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLint i;
   GET_CONTEXT;
   CHECK_CONTEXT;

   if (map==GL_PIXEL_MAP_I_TO_I || map==GL_PIXEL_MAP_S_TO_S) {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = UINT_TO_FLOAT( values[i] );
      }
   }
   (*CC->API.PixelMapfv)( CC, map, mapsize, fvalues );
}



void GLAPIENTRY glPixelMapusv( GLenum map, GLint mapsize, const GLushort *values )
{
   GLfloat fvalues[MAX_PIXEL_MAP_TABLE];
   GLint i;
   GET_CONTEXT;
   CHECK_CONTEXT;

   if (map==GL_PIXEL_MAP_I_TO_I || map==GL_PIXEL_MAP_S_TO_S) {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = (GLfloat) values[i];
      }
   }
   else {
      for (i=0;i<mapsize;i++) {
         fvalues[i] = USHORT_TO_FLOAT( values[i] );
      }
   }
   (*CC->API.PixelMapfv)( CC, map, mapsize, fvalues );
}


void GLAPIENTRY glPixelStoref( GLenum pname, GLfloat param )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PixelStorei)( CC, pname, (GLint) param );
}


void GLAPIENTRY glPixelStorei( GLenum pname, GLint param )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PixelStorei)( CC, pname, param );
}


void GLAPIENTRY glPixelTransferf( GLenum pname, GLfloat param )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PixelTransferf)(CC, pname, param);
}


void GLAPIENTRY glPixelTransferi( GLenum pname, GLint param )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PixelTransferf)(CC, pname, (GLfloat) param);
}


void GLAPIENTRY glPixelZoom( GLfloat xfactor, GLfloat yfactor )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PixelZoom)(CC, xfactor, yfactor);
}


void GLAPIENTRY glPointSize( GLfloat size )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PointSize)(CC, size);
}


void GLAPIENTRY glPolygonMode( GLenum face, GLenum mode )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PolygonMode)(CC, face, mode);
}


void GLAPIENTRY glPolygonOffset( GLfloat factor, GLfloat units )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PolygonOffset)( CC, factor, units );
}


/* GL_EXT_polygon_offset */
void GLAPIENTRY glPolygonOffsetEXT( GLfloat factor, GLfloat bias )
{
   glPolygonOffset( factor, bias * DEPTH_SCALE );
}


void GLAPIENTRY glPolygonStipple( const GLubyte *pattern )
{
   GLuint unpackedPattern[32];
   GET_CONTEXT;
   CHECK_CONTEXT;
   gl_unpack_polygon_stipple( CC, pattern, unpackedPattern );
   (*CC->API.PolygonStipple)(CC, unpackedPattern);
}


void GLAPIENTRY glPopAttrib( void )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PopAttrib)(CC);
}


void GLAPIENTRY glPopClientAttrib( void )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PopClientAttrib)(CC);
}


void GLAPIENTRY glPopMatrix( void )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PopMatrix)( CC );
}


void GLAPIENTRY glPopName( void )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PopName)(CC);
}


void GLAPIENTRY glPrioritizeTextures( GLsizei n, const GLuint *textures,
                                    const GLclampf *priorities )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PrioritizeTextures)(CC, n, textures, priorities);
}


void GLAPIENTRY glPushMatrix( void )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PushMatrix)( CC );
}


void GLAPIENTRY glRasterPos2d( GLdouble x, GLdouble y )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, 0.0F, 1.0F );
}


void GLAPIENTRY glRasterPos2f( GLfloat x, GLfloat y )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, 0.0F, 1.0F );
}


void GLAPIENTRY glRasterPos2i( GLint x, GLint y )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, 0.0F, 1.0F );
}


void GLAPIENTRY glRasterPos2s( GLshort x, GLshort y )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, 0.0F, 1.0F );
}


void GLAPIENTRY glRasterPos3d( GLdouble x, GLdouble y, GLdouble z )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F );
}


void GLAPIENTRY glRasterPos3f( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F );
}


void GLAPIENTRY glRasterPos3i( GLint x, GLint y, GLint z )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F );
}


void GLAPIENTRY glRasterPos3s( GLshort x, GLshort y, GLshort z )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z, 1.0F );
}


void GLAPIENTRY glRasterPos4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y,
							   (GLfloat) z, (GLfloat) w );
}


void GLAPIENTRY glRasterPos4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, x, y, z, w );
}


void GLAPIENTRY glRasterPos4i( GLint x, GLint y, GLint z, GLint w )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y,
                           (GLfloat) z, (GLfloat) w );
}


void GLAPIENTRY glRasterPos4s( GLshort x, GLshort y, GLshort z, GLshort w )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) x, (GLfloat) y,
                           (GLfloat) z, (GLfloat) w );
}


void GLAPIENTRY glRasterPos2dv( const GLdouble *v )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F );
}


void GLAPIENTRY glRasterPos2fv( const GLfloat *v )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F );
}


void GLAPIENTRY glRasterPos2iv( const GLint *v )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F );
}


void GLAPIENTRY glRasterPos2sv( const GLshort *v )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1], 0.0F, 1.0F );
}


/*** 3 element vector ***/

void GLAPIENTRY glRasterPos3dv( const GLdouble *v )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                           (GLfloat) v[2], 1.0F );
}


void GLAPIENTRY glRasterPos3fv( const GLfloat *v )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                               (GLfloat) v[2], 1.0F );
}


void GLAPIENTRY glRasterPos3iv( const GLint *v )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                           (GLfloat) v[2], 1.0F );
}


void GLAPIENTRY glRasterPos3sv( const GLshort *v )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                           (GLfloat) v[2], 1.0F );
}


void GLAPIENTRY glRasterPos4dv( const GLdouble *v )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                           (GLfloat) v[2], (GLfloat) v[3] );
}


void GLAPIENTRY glRasterPos4fv( const GLfloat *v )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, v[0], v[1], v[2], v[3] );
}


void GLAPIENTRY glRasterPos4iv( const GLint *v )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                           (GLfloat) v[2], (GLfloat) v[3] );
}


void GLAPIENTRY glRasterPos4sv( const GLshort *v )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.RasterPos4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                           (GLfloat) v[2], (GLfloat) v[3] );
}


void GLAPIENTRY glReadBuffer( GLenum mode )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.ReadBuffer)( CC, mode );
}


void GLAPIENTRY glReadPixels( GLint x, GLint y, GLsizei width, GLsizei height,
		   GLenum format, GLenum type, GLvoid *pixels )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.ReadPixels)( CC, x, y, width, height, format, type, pixels );
}


void GLAPIENTRY glRectd( GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2 )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Rectf)( CC, (GLfloat) x1, (GLfloat) y1,
                     (GLfloat) x2, (GLfloat) y2 );
}


void GLAPIENTRY glRectf( GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2 )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Rectf)( CC, x1, y1, x2, y2 );
}


void GLAPIENTRY glRecti( GLint x1, GLint y1, GLint x2, GLint y2 )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Rectf)( CC, (GLfloat) x1, (GLfloat) y1,
                         (GLfloat) x2, (GLfloat) y2 );
}


void GLAPIENTRY glRects( GLshort x1, GLshort y1, GLshort x2, GLshort y2 )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Rectf)( CC, (GLfloat) x1, (GLfloat) y1,
                     (GLfloat) x2, (GLfloat) y2 );
}


void GLAPIENTRY glRectdv( const GLdouble *v1, const GLdouble *v2 )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Rectf)(CC, (GLfloat) v1[0], (GLfloat) v1[1],
                    (GLfloat) v2[0], (GLfloat) v2[1]);
}


void GLAPIENTRY glRectfv( const GLfloat *v1, const GLfloat *v2 )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Rectf)(CC, v1[0], v1[1], v2[0], v2[1]);
}


void GLAPIENTRY glRectiv( const GLint *v1, const GLint *v2 )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Rectf)( CC, (GLfloat) v1[0], (GLfloat) v1[1],
                     (GLfloat) v2[0], (GLfloat) v2[1] );
}


void GLAPIENTRY glRectsv( const GLshort *v1, const GLshort *v2 )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Rectf)(CC, (GLfloat) v1[0], (GLfloat) v1[1],
        (GLfloat) v2[0], (GLfloat) v2[1]);
}


void GLAPIENTRY glScissor( GLint x, GLint y, GLsizei width, GLsizei height)
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Scissor)(CC, x, y, width, height);
}


GLboolean GLAPIENTRY glIsEnabled( GLenum cap )
{
   GET_CONTEXT;
   CHECK_CONTEXT_RETURN(GL_FALSE);
   return (*CC->API.IsEnabled)( CC, cap );
}



void GLAPIENTRY glPushAttrib( GLbitfield mask )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PushAttrib)(CC, mask);
}


void GLAPIENTRY glPushClientAttrib( GLbitfield mask )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PushClientAttrib)(CC, mask);
}


void GLAPIENTRY glPushName( GLuint name )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.PushName)(CC, name);
}


GLint GLAPIENTRY glRenderMode( GLenum mode )
{
   GET_CONTEXT;
   CHECK_CONTEXT_RETURN(0);
   return (*CC->API.RenderMode)(CC, mode);
}


void GLAPIENTRY glRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Rotatef)( CC, (GLfloat) angle,
                       (GLfloat) x, (GLfloat) y, (GLfloat) z );
}


void GLAPIENTRY glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Rotatef)( CC, angle, x, y, z );
}


void GLAPIENTRY glSelectBuffer( GLsizei size, GLuint *buffer )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.SelectBuffer)(CC, size, buffer);
}


void GLAPIENTRY glScaled( GLdouble x, GLdouble y, GLdouble z )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Scalef)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z );
}


void GLAPIENTRY glScalef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Scalef)( CC, x, y, z );
}


void GLAPIENTRY glShadeModel( GLenum mode )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.ShadeModel)(CC, mode);
}


void GLAPIENTRY glStencilFunc( GLenum func, GLint ref, GLuint mask )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.StencilFunc)(CC, func, ref, mask);
}


void GLAPIENTRY glStencilMask( GLuint mask )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.StencilMask)(CC, mask);
}


void GLAPIENTRY glStencilOp( GLenum fail, GLenum zfail, GLenum zpass )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.StencilOp)(CC, fail, zfail, zpass);
}


void GLAPIENTRY glTexCoord1d( GLdouble s )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, 0.0, 0.0, 1.0 );
}


void GLAPIENTRY glTexCoord1f( GLfloat s )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, s, 0.0, 0.0, 1.0 );
}


void GLAPIENTRY glTexCoord1i( GLint s )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, 0.0, 0.0, 1.0 );
}


void GLAPIENTRY glTexCoord1s( GLshort s )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, 0.0, 0.0, 1.0 );
}


void GLAPIENTRY glTexCoord2d( GLdouble s, GLdouble t )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, (GLfloat) s, (GLfloat) t );
}


void GLAPIENTRY glTexCoord2f( GLfloat s, GLfloat t )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, s, t );
}


void GLAPIENTRY glTexCoord2i( GLint s, GLint t )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, (GLfloat) s, (GLfloat) t );
}


void GLAPIENTRY glTexCoord2s( GLshort s, GLshort t )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, (GLfloat) s, (GLfloat) t );
}


void GLAPIENTRY glTexCoord3d( GLdouble s, GLdouble t, GLdouble r )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, (GLfloat) t, (GLfloat) r, 1.0 );
}


void GLAPIENTRY glTexCoord3f( GLfloat s, GLfloat t, GLfloat r )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, s, t, r, 1.0 );
}


void GLAPIENTRY glTexCoord3i( GLint s, GLint t, GLint r )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, (GLfloat) t,
                               (GLfloat) r, 1.0 );
}


void GLAPIENTRY glTexCoord3s( GLshort s, GLshort t, GLshort r )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, (GLfloat) t,
                               (GLfloat) r, 1.0 );
}


void GLAPIENTRY glTexCoord4d( GLdouble s, GLdouble t, GLdouble r, GLdouble q )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, (GLfloat) t,
                               (GLfloat) r, (GLfloat) q );
}


void GLAPIENTRY glTexCoord4f( GLfloat s, GLfloat t, GLfloat r, GLfloat q )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, s, t, r, q );
}


void GLAPIENTRY glTexCoord4i( GLint s, GLint t, GLint r, GLint q )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, (GLfloat) t,
                               (GLfloat) r, (GLfloat) q );
}


void GLAPIENTRY glTexCoord4s( GLshort s, GLshort t, GLshort r, GLshort q )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) s, (GLfloat) t,
                               (GLfloat) r, (GLfloat) q );
}


void GLAPIENTRY glTexCoord1dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) *v, 0.0, 0.0, 1.0 );
}


void GLAPIENTRY glTexCoord1fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, *v, 0.0, 0.0, 1.0 );
}


void GLAPIENTRY glTexCoord1iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, *v, 0.0, 0.0, 1.0 );
}


void GLAPIENTRY glTexCoord1sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) *v, 0.0, 0.0, 1.0 );
}


void GLAPIENTRY glTexCoord2dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, (GLfloat) v[0], (GLfloat) v[1] );
}


void GLAPIENTRY glTexCoord2fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, v[0], v[1] );
}


void GLAPIENTRY glTexCoord2iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, (GLfloat) v[0], (GLfloat) v[1] );
}


void GLAPIENTRY glTexCoord2sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord2f)( CC, (GLfloat) v[0], (GLfloat) v[1] );
}


void GLAPIENTRY glTexCoord3dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                               (GLfloat) v[2], 1.0 );
}


void GLAPIENTRY glTexCoord3fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, v[0], v[1], v[2], 1.0 );
}


void GLAPIENTRY glTexCoord3iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                          (GLfloat) v[2], 1.0 );
}


void GLAPIENTRY glTexCoord3sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                               (GLfloat) v[2], 1.0 );
}


void GLAPIENTRY glTexCoord4dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                               (GLfloat) v[2], (GLfloat) v[3] );
}


void GLAPIENTRY glTexCoord4fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, v[0], v[1], v[2], v[3] );
}


void GLAPIENTRY glTexCoord4iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                               (GLfloat) v[2], (GLfloat) v[3] );
}


void GLAPIENTRY glTexCoord4sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.TexCoord4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                               (GLfloat) v[2], (GLfloat) v[3] );
}


void GLAPIENTRY glTexCoordPointer( GLint size, GLenum type, GLsizei stride,
                                 const GLvoid *ptr )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.TexCoordPointer)(CC, size, type, stride, ptr);
}


void GLAPIENTRY glTexGend( GLenum coord, GLenum pname, GLdouble param )
{
   GLfloat p = (GLfloat) param;
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.TexGenfv)( CC, coord, pname, &p );
}


void GLAPIENTRY glTexGenf( GLenum coord, GLenum pname, GLfloat param )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.TexGenfv)( CC, coord, pname, &param );
}


void GLAPIENTRY glTexGeni( GLenum coord, GLenum pname, GLint param )
{
   GLfloat p = (GLfloat) param;
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.TexGenfv)( CC, coord, pname, &p );
}


void GLAPIENTRY glTexGendv( GLenum coord, GLenum pname, const GLdouble *params )
{
   GLfloat p[4];
   GET_CONTEXT;
   CHECK_CONTEXT;
   p[0] = params[0];
   p[1] = params[1];
   p[2] = params[2];
   p[3] = params[3];
   (*CC->API.TexGenfv)( CC, coord, pname, p );
}


void GLAPIENTRY glTexGeniv( GLenum coord, GLenum pname, const GLint *params )
{
   GLfloat p[4];
   GET_CONTEXT;
   CHECK_CONTEXT;
   p[0] = params[0];
   p[1] = params[1];
   p[2] = params[2];
   p[3] = params[3];
   (*CC->API.TexGenfv)( CC, coord, pname, p );
}


void GLAPIENTRY glTexGenfv( GLenum coord, GLenum pname, const GLfloat *params )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.TexGenfv)( CC, coord, pname, params );
}




void GLAPIENTRY glTexEnvf( GLenum target, GLenum pname, GLfloat param )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.TexEnvfv)( CC, target, pname, &param );
}



void GLAPIENTRY glTexEnvi( GLenum target, GLenum pname, GLint param )
{
   GLfloat p[4];
   GET_CONTEXT;
   p[0] = (GLfloat) param;
   p[1] = p[2] = p[3] = 0.0;
   CHECK_CONTEXT;
   (*CC->API.TexEnvfv)( CC, target, pname, p );
}



void GLAPIENTRY glTexEnvfv( GLenum target, GLenum pname, const GLfloat *param )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.TexEnvfv)( CC, target, pname, param );
}



void GLAPIENTRY glTexEnviv( GLenum target, GLenum pname, const GLint *param )
{
   GLfloat p[4];
   GET_CONTEXT;
   p[0] = INT_TO_FLOAT( param[0] );
   p[1] = INT_TO_FLOAT( param[1] );
   p[2] = INT_TO_FLOAT( param[2] );
   p[3] = INT_TO_FLOAT( param[3] );
   CHECK_CONTEXT;
   (*CC->API.TexEnvfv)( CC, target, pname, p );
}


void GLAPIENTRY glTexImage1D( GLenum target, GLint level, GLint internalformat,
                            GLsizei width, GLint border,
                            GLenum format, GLenum type, const GLvoid *pixels )
{
   struct gl_image *teximage;
   GET_CONTEXT;
   CHECK_CONTEXT;
   teximage = gl_unpack_image( CC, width, 1, format, type, pixels );
   (*CC->API.TexImage1D)( CC, target, level, internalformat,
                          width, border, format, type, teximage );
}


void GLAPIENTRY glTexImage2D( GLenum target, GLint level, GLint internalformat,
                            GLsizei width, GLsizei height, GLint border,
                            GLenum format, GLenum type, const GLvoid *pixels )
{
#if defined(FX) && defined(__WIN32__)

   /*** Ugly hack for Windows GLQuake ***/
  
   struct gl_image *teximage;
   GLvoid *newpixels = NULL;
   GLsizei newwidth, newheight;
   GLint x, y;
   static GLint leveldif = 0;
   static GLuint lasttexobj = 0;
   GET_CONTEXT;
   CHECK_CONTEXT;

   if (CC->Texture.Unit[0].Current2D->Name!=lasttexobj) {
      lasttexobj = CC->Texture.Unit[0].Current2D->Name;
      leveldif = 0;
   }
  
   if ((format==GL_COLOR_INDEX) && (internalformat==1))
      internalformat = GL_COLOR_INDEX8_EXT;
  
   if (width>256 || height>256) {
      newpixels = malloc((width+4)*height*4);

      while (width>256 || height>256) {
         newwidth = width / 2;
         newheight = height / 2;
         leveldif++;
      
         fprintf(stderr,"Scaling: (%d) %dx%d -> %dx%d\n",internalformat,width,height,newwidth,newheight);
         fflush(stderr);
      
         for (y=0;y<newheight;y++) {
            for (x=0;x<newwidth;x++) {
               ((GLubyte *)newpixels)[(x+y*newwidth)*4+0]=((GLubyte *)pixels)[(x*2+y*width*2)*4+0];
               ((GLubyte *)newpixels)[(x+y*newwidth)*4+1]=((GLubyte *)pixels)[(x*2+y*width*2)*4+1];
               ((GLubyte *)newpixels)[(x+y*newwidth)*4+2]=((GLubyte *)pixels)[(x*2+y*width*2)*4+2];
               ((GLubyte *)newpixels)[(x+y*newwidth)*4+3]=((GLubyte *)pixels)[(x*2+y*width*2)*4+3];
            }
         }
         pixels = newpixels;
         width = newwidth;
         height = newheight;
      }
      level=0;
   }
   else {
     level -= leveldif;
   }

   teximage = gl_unpack_image( CC, width, height, format, type, pixels );
   (*CC->API.TexImage2D)( CC, target, level, internalformat,
                          width, height, border, format, type, teximage );

   if (newpixels)
      free(newpixels);

#else

   /*** all other systems ***/
   struct gl_image *teximage;
   GET_CONTEXT;
   CHECK_CONTEXT;
   teximage = gl_unpack_image( CC, width, height, format, type, pixels );
   (*CC->API.TexImage2D)( CC, target, level, internalformat,
			 width, height, border, format, type, teximage );

#endif
}


void GLAPIENTRY glTexImage3D( GLenum target, GLint level, GLint internalformat,
                            GLsizei width, GLsizei height, GLsizei depth,
                            GLint border, GLenum format, GLenum type,
                            const GLvoid *pixels )
{
   struct gl_image *teximage;
   GET_CONTEXT;
   CHECK_CONTEXT;
   teximage = gl_unpack_image3D( CC, width, height, depth, format, type, pixels);
   (*CC->API.TexImage3DEXT)( CC, target, level, internalformat,
                             width, height, depth, border, format, type, 
                             teximage );
}


void GLAPIENTRY glTexParameterf( GLenum target, GLenum pname, GLfloat param )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.TexParameterfv)( CC, target, pname, &param );
}


void GLAPIENTRY glTexParameteri( GLenum target, GLenum pname, GLint param )
{
   GLfloat fparam[4];
   GET_CONTEXT;
   fparam[0] = (GLfloat) param;
   fparam[1] = fparam[2] = fparam[3] = 0.0;
   CHECK_CONTEXT;
   (*CC->API.TexParameterfv)( CC, target, pname, fparam );
}


void GLAPIENTRY glTexParameterfv( GLenum target, GLenum pname, const GLfloat *params )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.TexParameterfv)( CC, target, pname, params );
}


void GLAPIENTRY glTexParameteriv( GLenum target, GLenum pname, const GLint *params )
{
   GLfloat p[4];
   GET_CONTEXT;
   CHECK_CONTEXT;
   if (pname==GL_TEXTURE_BORDER_COLOR) {
      p[0] = INT_TO_FLOAT( params[0] );
      p[1] = INT_TO_FLOAT( params[1] );
      p[2] = INT_TO_FLOAT( params[2] );
      p[3] = INT_TO_FLOAT( params[3] );
   }
   else {
      p[0] = (GLfloat) params[0];
      p[1] = (GLfloat) params[1];
      p[2] = (GLfloat) params[2];
      p[3] = (GLfloat) params[3];
   }
   (*CC->API.TexParameterfv)( CC, target, pname, p );
}


void GLAPIENTRY glTexSubImage1D( GLenum target, GLint level, GLint xoffset,
                               GLsizei width, GLenum format,
                               GLenum type, const GLvoid *pixels )
{
   struct gl_image *image;
   GET_CONTEXT;
   CHECK_CONTEXT;
   image = gl_unpack_texsubimage( CC, width, 1, format, type, pixels );
   (*CC->API.TexSubImage1D)( CC, target, level, xoffset, width,
                             format, type, image );
}


void GLAPIENTRY glTexSubImage2D( GLenum target, GLint level,
                               GLint xoffset, GLint yoffset,
                               GLsizei width, GLsizei height,
                               GLenum format, GLenum type,
                               const GLvoid *pixels )
{
   struct gl_image *image;
   GET_CONTEXT;
   CHECK_CONTEXT;
   image = gl_unpack_texsubimage( CC, width, height, format, type, pixels );
   (*CC->API.TexSubImage2D)( CC, target, level, xoffset, yoffset,
                             width, height, format, type, image );
}


void GLAPIENTRY glTexSubImage3D( GLenum target, GLint level, GLint xoffset,
                               GLint yoffset, GLint zoffset, GLsizei width,
                               GLsizei height, GLsizei depth, GLenum format,
                               GLenum type, const GLvoid *pixels )
{
   struct gl_image *image;
   GET_CONTEXT;
   CHECK_CONTEXT;
   image = gl_unpack_texsubimage3D( CC, width, height, depth, format, type,
                                    pixels );
   (*CC->API.TexSubImage3DEXT)( CC, target, level, xoffset, yoffset, zoffset,
                                width, height, depth, format, type, image );
}


void GLAPIENTRY glTranslated( GLdouble x, GLdouble y, GLdouble z )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Translatef)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z );
}


void GLAPIENTRY glTranslatef( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CONTEXT;
   CHECK_CONTEXT;
   (*CC->API.Translatef)( CC, x, y, z );
}


void GLAPIENTRY glVertex2d( GLdouble x, GLdouble y )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, (GLfloat) x, (GLfloat) y );
}


void GLAPIENTRY glVertex2f( GLfloat x, GLfloat y )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, x, y );
}


void GLAPIENTRY glVertex2i( GLint x, GLint y )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, (GLfloat) x, (GLfloat) y );
}


void GLAPIENTRY glVertex2s( GLshort x, GLshort y )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, (GLfloat) x, (GLfloat) y );
}


void GLAPIENTRY glVertex3d( GLdouble x, GLdouble y, GLdouble z )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z );
}


void GLAPIENTRY glVertex3f( GLfloat x, GLfloat y, GLfloat z )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, x, y, z );
}


void GLAPIENTRY glVertex3i( GLint x, GLint y, GLint z )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z );
}


void GLAPIENTRY glVertex3s( GLshort x, GLshort y, GLshort z )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, (GLfloat) x, (GLfloat) y, (GLfloat) z );
}


void GLAPIENTRY glVertex4d( GLdouble x, GLdouble y, GLdouble z, GLdouble w )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, (GLfloat) x, (GLfloat) y,
                            (GLfloat) z, (GLfloat) w );
}


void GLAPIENTRY glVertex4f( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, x, y, z, w );
}


void GLAPIENTRY glVertex4i( GLint x, GLint y, GLint z, GLint w )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, (GLfloat) x, (GLfloat) y,
                            (GLfloat) z, (GLfloat) w );
}


void GLAPIENTRY glVertex4s( GLshort x, GLshort y, GLshort z, GLshort w )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, (GLfloat) x, (GLfloat) y,
                            (GLfloat) z, (GLfloat) w );
}


void GLAPIENTRY glVertex2dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, (GLfloat) v[0], (GLfloat) v[1] );
}


void GLAPIENTRY glVertex2fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, v[0], v[1] );
}


void GLAPIENTRY glVertex2iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, (GLfloat) v[0], (GLfloat) v[1] );
}


void GLAPIENTRY glVertex2sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex2f)( CC, (GLfloat) v[0], (GLfloat) v[1] );
}


void GLAPIENTRY glVertex3dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}


void GLAPIENTRY glVertex3fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex3fv)( CC, v );
}


void GLAPIENTRY glVertex3iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}


void GLAPIENTRY glVertex3sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex3f)( CC, (GLfloat) v[0], (GLfloat) v[1], (GLfloat) v[2] );
}


void GLAPIENTRY glVertex4dv( const GLdouble *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                            (GLfloat) v[2], (GLfloat) v[3] );
}


void GLAPIENTRY glVertex4fv( const GLfloat *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, v[0], v[1], v[2], v[3] );
}


void GLAPIENTRY glVertex4iv( const GLint *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                            (GLfloat) v[2], (GLfloat) v[3] );
}


void GLAPIENTRY glVertex4sv( const GLshort *v )
{
   GET_CONTEXT;
   (*CC->API.Vertex4f)( CC, (GLfloat) v[0], (GLfloat) v[1],
                            (GLfloat) v[2], (GLfloat) v[3] );
}


void GLAPIENTRY glVertexPointer( GLint size, GLenum type, GLsizei stride,
                               const GLvoid *ptr )
{
   GET_CONTEXT;
   (*CC->API.VertexPointer)(CC, size, type, stride, ptr);
}


void GLAPIENTRY glViewport( GLint x, GLint y, GLsizei width, GLsizei height )
{
   GET_CONTEXT;
   (*CC->API.Viewport)( CC, x, y, width, height );
}

