/* Id: alphabuf.c,v 1.3 1999/02/26 08:52:30 martin Exp $ */

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
 * Log: alphabuf.c,v $
 * Revision 1.3  1999/02/26 08:52:30  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.5  1999/02/24 22:48:04  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.4  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.3  1998/11/19 03:08:09  brianp
 * clean-up of alpha buffer clearing code
 *
 * Revision 3.2  1998/03/28 03:57:13  brianp
 * added CONST macro to fix IRIX compilation problems
 *
 * Revision 3.1  1998/03/27 04:40:28  brianp
 * added a few const keywords
 *
 * Revision 3.0  1998/01/31 20:45:34  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/alphabuf.c,v 1.0tsi Exp $ */


/*
 * Software alpha planes.  Many frame buffers don't have alpha bits so
 * we simulate them in software.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <stdlib.h>
#include <string.h>
#endif
#include "alphabuf.h"
#include "context.h"
#include "macros.h"
#include "types.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



#define ALPHA_ADDR(X,Y)  (ctx->Buffer->Alpha + (Y) * ctx->Buffer->Width + (X))



/*
 * Allocate a new front and back alpha buffer.
 */
void gl_alloc_alpha_buffers( GLcontext* ctx )
{
   GLint bytes = ctx->Buffer->Width * ctx->Buffer->Height * sizeof(GLubyte);

   if (ctx->Visual->FrontAlphaEnabled) {
      if (ctx->Buffer->FrontAlpha) {
         free( ctx->Buffer->FrontAlpha );
      }
      ctx->Buffer->FrontAlpha = (GLubyte *) malloc( bytes );
      if (!ctx->Buffer->FrontAlpha) {
         /* out of memory */
         gl_error( ctx, GL_OUT_OF_MEMORY, "Couldn't allocate front alpha buffer" );
      }
   }
   if (ctx->Visual->BackAlphaEnabled) {
      if (ctx->Buffer->BackAlpha) {
         free( ctx->Buffer->BackAlpha );
      }
      ctx->Buffer->BackAlpha = (GLubyte *) malloc( bytes );
      if (!ctx->Buffer->BackAlpha) {
         /* out of memory */
         gl_error( ctx, GL_OUT_OF_MEMORY, "Couldn't allocate back alpha buffer" );
      }
   }
   if (ctx->Color.DrawBuffer==GL_FRONT) {
      ctx->Buffer->Alpha = ctx->Buffer->FrontAlpha;
   }
   if (ctx->Color.DrawBuffer==GL_BACK) {
      ctx->Buffer->Alpha = ctx->Buffer->BackAlpha;
   }
}



static void clear_alpha_buffer( const GLcontext *ctx, GLubyte *abuffer )
{
    GLubyte aclear = (GLint) (ctx->Color.ClearColor[3] * 255.0F);

    if (ctx->Scissor.Enabled) {
	/* clear scissor region */
	GLint j;
	GLint rowLen = ctx->Buffer->Xmax - ctx->Buffer->Xmin + 1;
	GLint rows = ctx->Buffer->Ymax - ctx->Buffer->Ymin + 1;
	GLubyte *aptr = abuffer + ctx->Buffer->Ymin * ctx->Buffer->Width + ctx->Buffer->Xmin;
	for (j = 0; j <= rows; j++) {
	    MEMSET( aptr, aclear, rowLen );
	    aptr += rowLen;
	}
    }
    else {
	/* clear whole buffer */
	MEMSET( abuffer, aclear, ctx->Buffer->Width * ctx->Buffer->Height );
    }
}


/*
 * Clear the front and/or back alpha planes.
 */
void gl_clear_alpha_buffers( GLcontext* ctx )
{
   if ((ctx->Color.DrawBuffer==GL_FRONT || ctx->Color.DrawBuffer==GL_FRONT_AND_BACK)
       && ctx->Visual->FrontAlphaEnabled && ctx->Buffer->FrontAlpha) {
       clear_alpha_buffer(ctx, ctx->Buffer->FrontAlpha);
   }
   if ((ctx->Color.DrawBuffer==GL_BACK || ctx->Color.DrawBuffer==GL_FRONT_AND_BACK)
       && ctx->Visual->BackAlphaEnabled && ctx->Buffer->BackAlpha) {
       clear_alpha_buffer( ctx, ctx->Buffer->BackAlpha );
   }
}



void gl_write_alpha_span( GLcontext* ctx, GLuint n, GLint x, GLint y,
                          CONST GLubyte rgba[][4], const GLubyte mask[] )
{
   GLubyte *aptr = ALPHA_ADDR( x, y );
   GLuint i;

   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            *aptr = rgba[i][ACOMP];
         }
         aptr++;
      }
   }
   else {
      for (i=0;i<n;i++) {
         *aptr++ = rgba[i][ACOMP];
      }
   }
}


void gl_write_mono_alpha_span( GLcontext* ctx, GLuint n, GLint x, GLint y,
                               GLubyte alpha, const GLubyte mask[] )
{
   GLubyte *aptr = ALPHA_ADDR( x, y );
   GLuint i;

   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            *aptr = alpha;
         }
         aptr++;
      }
   }
   else {
      for (i=0;i<n;i++) {
         *aptr++ = alpha;
      }
   }
}


void gl_write_alpha_pixels( GLcontext* ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            CONST GLubyte rgba[][4], const GLubyte mask[] )
{
   GLuint i;

   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            GLubyte *aptr = ALPHA_ADDR( x[i], y[i] );
            *aptr = rgba[i][ACOMP];
         }
      }
   }
   else {
      for (i=0;i<n;i++) {
         GLubyte *aptr = ALPHA_ADDR( x[i], y[i] );
         *aptr = rgba[i][ACOMP];
      }
   }
}


void gl_write_mono_alpha_pixels( GLcontext* ctx,
                                 GLuint n, const GLint x[], const GLint y[],
                                 GLubyte alpha, const GLubyte mask[] )
{
   GLuint i;

   if (mask) {
      for (i=0;i<n;i++) {
         if (mask[i]) {
            GLubyte *aptr = ALPHA_ADDR( x[i], y[i] );
            *aptr = alpha;
         }
      }
   }
   else {
      for (i=0;i<n;i++) {
         GLubyte *aptr = ALPHA_ADDR( x[i], y[i] );
         *aptr = alpha;
      }
   }
}



void gl_read_alpha_span( GLcontext *ctx,
                         GLuint n, GLint x, GLint y, GLubyte rgba[][4] )
{
   GLubyte *aptr = ALPHA_ADDR( x, y );
   GLuint i;
   for (i=0;i<n;i++) {
      rgba[i][ACOMP] = *aptr++;
   }
}


void gl_read_alpha_pixels( GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLubyte rgba[][4], const GLubyte mask[] )
{
   GLuint i;
   for (i=0;i<n;i++) {
      if (mask[i]) {
         GLubyte *aptr = ALPHA_ADDR( x[i], y[i] );
         rgba[i][ACOMP] = *aptr;
      }
   }
}



