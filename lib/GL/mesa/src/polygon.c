/* Id: polygon.c,v 1.3 1999/02/26 08:52:36 martin Exp $ */

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
 * Log: polygon.c,v $
 * Revision 1.3  1999/02/26 08:52:36  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.6  1999/02/24 22:48:07  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.5  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.4  1998/11/17 02:51:35  brianp
 * added a bunch of new device driver functions (Keith Harrison)
 *
 * Revision 3.3  1998/08/21 02:43:30  brianp
 * implemented true packing/unpacking of polygon stipples
 *
 * Revision 3.2  1998/07/29 04:08:31  brianp
 * implemented glGetPolygonStipple()
 *
 * Revision 3.1  1998/03/27 04:33:17  brianp
 * fixed G++ warnings
 *
 * Revision 3.0  1998/01/31 21:01:27  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/polygon.c,v 1.2 1999/03/14 03:20:50 dawes Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#else
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "image.h"
#include "macros.h"
#include "polygon.h"
#include "types.h"
#endif



void gl_CullFace( GLcontext *ctx, GLenum mode )
{
   if (mode!=GL_FRONT && mode!=GL_BACK && mode!=GL_FRONT_AND_BACK) {
      gl_error( ctx, GL_INVALID_ENUM, "glCullFace" );
      return;
   }
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glCullFace" );
      return;
   }
   ctx->Polygon.CullFaceMode = mode;
   ctx->NewState |= NEW_POLYGON;
}



void gl_FrontFace( GLcontext *ctx, GLenum mode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glFrontFace" );
      return;
   }
   if (mode!=GL_CW && mode!=GL_CCW) {
      gl_error( ctx, GL_INVALID_ENUM, "glFrontFace" );
      return;
   }
   ctx->Polygon.FrontFace = mode;
}



void gl_PolygonMode( GLcontext *ctx, GLenum face, GLenum mode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPolygonMode" );
      return;
   }
   if (face!=GL_FRONT && face!=GL_BACK && face!=GL_FRONT_AND_BACK) {
      gl_error( ctx, GL_INVALID_ENUM, "glPolygonMode(face)" );
      return;
   }
   else if (mode!=GL_POINT && mode!=GL_LINE && mode!=GL_FILL) {
      gl_error( ctx, GL_INVALID_ENUM, "glPolygonMode(mode)" );
      return;
   }

   if (face==GL_FRONT || face==GL_FRONT_AND_BACK) {
      ctx->Polygon.FrontMode = mode;
   }
   if (face==GL_BACK || face==GL_FRONT_AND_BACK) {
      ctx->Polygon.BackMode = mode;
   }

   /* Compute a handy "shortcut" value: */
   if (ctx->Polygon.FrontMode!=GL_FILL || ctx->Polygon.BackMode!=GL_FILL) {
      ctx->Polygon.Unfilled = GL_TRUE;
   }
   else {
      ctx->Polygon.Unfilled = GL_FALSE;
   }

   ctx->NewState |= NEW_POLYGON;

   if (ctx->Driver.PolygonMode) {
      (*ctx->Driver.PolygonMode)( ctx, face, mode );
   }
}



/*
 * NOTE:  stipple pattern has already been unpacked.
 */
void gl_PolygonStipple( GLcontext *ctx, const GLuint pattern[32] )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPolygonStipple" );
      return;
   }

   MEMCPY( ctx->PolygonStipple, pattern, 32 * 4 );

   if (ctx->Polygon.StippleFlag) {
      ctx->NewState |= NEW_RASTER_OPS;
   }
}



void gl_GetPolygonStipple( GLcontext *ctx, GLubyte *dest )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPolygonOffset" );
      return;
   }

   gl_pack_polygon_stipple( ctx, ctx->PolygonStipple, dest );
}



void gl_PolygonOffset( GLcontext *ctx,
                       GLfloat factor, GLfloat units )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPolygonOffset" );
      return;
   }
   ctx->Polygon.OffsetFactor = factor;
   ctx->Polygon.OffsetUnits = units;
}

