/* Id: winpos.c,v 1.3 1999/02/26 08:52:40 martin Exp $ */

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
/* $XFree86: xc/lib/GL/mesa/src/winpos.c,v 1.0tsi Exp $ */

/*
 * Log: winpos.c,v $
 * Revision 1.3  1999/02/26 08:52:40  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.2  1999/02/24 22:48:08  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.1  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.0  1998/01/31 21:07:51  brianp
 * initial rev
 *
 */


/*
 * GL_MESA_window_pos extension
 *
 * This extension offers a set of functions named glWindowPos*MESA() which
 * directly set the current raster position to the given window coordinate.
 * glWindowPos*MESA() are similar to glRasterPos*() but bypass the
 * modelview, projection and viewport transformations.
 *
 * These functions should be very handy in conjunction with glDrawPixels()
 * and glCopyPixels().
 *
 * If your application uses glWindowPos*MESA() and may be compiled with
 * a real OpenGL instead of Mesa you can simply copy the winpos.[ch] files
 * into your source tree and compile them with the rest of your code
 * since glWindowPos*MESA() can, and is, implemented in terms of standard
 * OpenGL commands when not using Mesa.  In your source files which use
 * glWindowPos*MESA() just #include "winpos.h".
 */



#ifdef PC_HEADER
#include "all.h"
#else
#include "GL/gl.h"
#endif

#ifdef GL_MESA_window_pos


#ifndef PC_HEADER
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#include "rastpos.h"
#include "winpos.h"
#endif



/*
 * Mesa implementation of glWindowPos*MESA()
 */
void gl_WindowPos4fMESA( GLcontext *ctx,
                         GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   gl_windowpos( ctx, x, y, z, w );
}


#else


/*
 * OpenGL implementation of glWindowPos*MESA()
 */
void glWindowPos4fMESA( GLfloat x, GLfloat y, GLfloat z, GLfloat w )
{
   GLfloat fx, fy;

   /* Push current matrix mode and viewport attributes */
   glPushAttrib( GL_TRANSFORM_BIT | GL_VIEWPORT_BIT );

   /* Setup projection parameters */
   glMatrixMode( GL_PROJECTION );
   glPushMatrix();
   glLoadIdentity();
   glMatrixMode( GL_MODELVIEW );
   glPushMatrix();
   glLoadIdentity();

   glDepthRange( z, z );
   glViewport( (int) x - 1, (int) y - 1, 2, 2 );

   /* set the raster (window) position */
   fx = x - (int) x;
   fy = y - (int) y;
   glRasterPos4f( fx, fy, 0.0, w );

   /* restore matrices, viewport and matrix mode */
   glPopMatrix();
   glMatrixMode( GL_PROJECTION );
   glPopMatrix();

   glPopAttrib();
}


#endif



