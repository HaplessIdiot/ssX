/* Id: lnaatemp.h,v 1.1 1999/02/26 08:52:34 martin Exp $ */

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
 * Log: lnaatemp.h,v $
 * Revision 1.1  1999/02/26 08:52:34  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 1.2  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 1.1  1998/12/01 02:41:26  brianp
 * Initial revision
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/lnaatemp.h,v 1.0tsi Exp $ */

/*
 * Antialiased Line Rasterizer Template
 *
 * This file is #include'd to generate custom line rasterizers.
 *
 * The following macros may be defined to indicate what auxillary information
 * must be interplated along the line:
 *    INTERP_RGB    - if defined, interpolate RGB values
 *    INTERP_ALPHA  - if defined, interpolate Alpha values
 *    INTERP_INDEX  - if defined, interpolate color index values
 *    INTERP_STUV    - if defined, interpolate float STU texcoords with
 *                         perspective correction
 *                         NOTE:  OpenGL STRQ = Mesa STUV (R was taken for red)
 *    INTERP_STUV1   - if defined, interpolate set 1 float STRQ texcoords
 *
 * Optionally, one may provide one-time setup code
 *    SETUP_CODE    - code which is to be executed once per line
 *
 * To actually "plot" each pixel the PLOT macro must be defined.
 *
 * This code was designed for the origin to be in the lower-left corner.
 */

/* void aa_line( GLcontext *ctx, GLuint vert0, GLuint vert1, GLuint pvert ) */
{
   struct vertex_buffer *VB = ctx->VB;
   struct pixel_buffer *pb = ctx->PB;
   GLfloat halfWidth = 0.5F * ctx->Line.Width;  /* 0.5 is a bit of a hack */
   GLboolean solid = !ctx->Line.StippleFlag;
   GLint x0 = (GLint) VB->Win[vert0][0], x1 = (GLint) VB->Win[vert1][0];
   GLint y0 = (GLint) VB->Win[vert0][1], y1 = (GLint) VB->Win[vert1][1];
   GLint dx = x1 - x0;
   GLint dy = y1 - y0;
   GLint xStep, yStep;
   GLint z0, z1;
#ifdef INTERP_RGBA
   GLfixed fr, fg, fb, fa;      /* fixed-pt RGBA */
   GLfixed dfr, dfg, dfb, dfa;  /* fixed-pt RGBA deltas */
#endif
#ifdef INTERP_STUV
   GLfloat hs0, dhs;     /* h denotes hyperbolic */
   GLfloat ht0, dht;
   GLfloat hu0, dhu;
   GLfloat hv0, dhv;
#endif
#ifdef INTERP_STUV1
   GLfloat hs01, dhs1;     /* h denotes hyperbolic */
   GLfloat ht01, dht1;
   GLfloat hu01, dhu1;
   GLfloat hv01, dhv1;
#endif
#ifdef INTERP_INDEX
   GLfixed fi, dfi;
#endif

   if (dx == 0 && dy == 0)
      return;

#if DEPTH_BITS==16
   z0 = FloatToFixed(VB->Win[vert0][2]);
   z1 = FloatToFixed(VB->Win[vert1][2]);
#else
   z0 = (int) VB->Win[vert0][2];
   z1 = (int) VB->Win[vert1][2];
#endif

#ifdef INTERP_STUV
   {
      GLfloat invw0 = 1.0F / VB->Clip[vert0][3];
      GLfloat invw1 = 1.0F / VB->Clip[vert1][3];
      hs0 = VB->MultiTexCoord[0][vert0][0] * invw0;
      dhs = VB->MultiTexCoord[0][vert1][0] * invw1 - hs0;
      ht0 = VB->MultiTexCoord[0][vert0][1] * invw0;
      dht = VB->MultiTexCoord[0][vert1][1] * invw1 - ht0;
      hu0 = VB->MultiTexCoord[0][vert0][2] * invw0;
      dhu = VB->MultiTexCoord[0][vert1][2] * invw1 - hu0;
      hv0 = VB->MultiTexCoord[0][vert0][3] * invw0;
      dhv = VB->MultiTexCoord[0][vert1][3] * invw1 - hv0;
   }
#endif
#ifdef INTERP_STUV1
   {
      GLfloat invw0 = 1.0F / VB->Clip[vert0][3];
      GLfloat invw1 = 1.0F / VB->Clip[vert1][3];
      hs01 = VB->MultiTexCoord[1][vert0][0] * invw0;
      dhs1 = VB->MultiTexCoord[1][vert1][0] * invw1 - hs01;
      ht01 = VB->MultiTexCoord[1][vert0][1] * invw0;
      dht1 = VB->MultiTexCoord[1][vert1][1] * invw1 - ht01;
      hu01 = VB->MultiTexCoord[1][vert0][2] * invw0;
      dhu1 = VB->MultiTexCoord[1][vert1][2] * invw1 - hu01;
      hv01 = VB->MultiTexCoord[1][vert0][3] * invw0;
      dhv1 = VB->MultiTexCoord[1][vert1][3] * invw1 - hv01;
   }
#endif

#ifdef INTERP_RGBA
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      fr = IntToFixed(VB->Color[vert0][0]);
      fg = IntToFixed(VB->Color[vert0][1]);
      fb = IntToFixed(VB->Color[vert0][2]);
      fa = IntToFixed(VB->Color[vert0][3]);
   }
   else {
      fr = IntToFixed(VB->Color[pvert][0]);
      fg = IntToFixed(VB->Color[pvert][1]);
      fb = IntToFixed(VB->Color[pvert][2]);
      fa = IntToFixed(VB->Color[pvert][3]);
      dfr = dfg = dfb = dfa = 0;
   }
#endif
#ifdef INTERP_INDEX
   if (ctx->Light.ShadeModel == GL_SMOOTH) {
      fi = IntToFixed(VB->Index[vert0]);
   }
   else {
      fi = IntToFixed(VB->Index[pvert]);
      dfi = 0;
   }
#endif

   /*
    * Setup
    */
#ifdef SETUP_CODE
   SETUP_CODE
#endif

   if (dx < 0) {
      xStep = -1;
      dx = -dx;
   }
   else {
      xStep = 1;
   }

   if (dy < 0) {
      yStep = -1;
      dy = -dy;
   }
   else {
      yStep = 1;
   }

   if (dx > dy) {
      /* X-major line */
      GLint i;
      GLint x = x0;
      GLfloat y = VB->Win[vert0][1];
      GLfloat yStep = (VB->Win[vert1][1] - y) / (GLfloat) dx;
      GLint dz = (z1 - z0) / dx;
#ifdef INTERP_RGBA
      GLfloat invDx = 1.0F / dx;
      if (ctx->Light.ShadeModel == GL_SMOOTH) {
         dfr = (IntToFixed(VB->Color[vert1][0]) - fr) / dx;
         dfg = (IntToFixed(VB->Color[vert1][1]) - fg) / dx;
         dfb = (IntToFixed(VB->Color[vert1][2]) - fb) / dx;
         dfa = (IntToFixed(VB->Color[vert1][3]) - fa) / dx;
      }
#endif
#ifdef INTERP_STUV
      dhs *= invDx;
      dht *= invDx;
      dhu *= invDx;
      dhv *= invDx;
#endif
#ifdef INTERP_STUV1
      dhs1 *= invDx;
      dht1 *= invDx;
      dhu1 *= invDx;
      dhv1 *= invDx;
#endif
      for (i = 0; i < dx; i++) {
         if (solid || (ctx->Line.StipplePattern & (1 << ((ctx->StippleCounter/ctx->Line.StippleFactor) & 0xf)))) {

            GLfloat yTop = y + halfWidth;
            GLfloat yBot = y - halfWidth;
            GLint yTopi = (GLint) yTop;
            GLint yBoti = (GLint) yBot;
            GLint iy;
#ifdef INTERP_RGBA
            GLubyte red   = FixedToInt(fr);
            GLubyte green = FixedToInt(fg);
            GLubyte blue  = FixedToInt(fb);
            GLubyte alpha = FixedToInt(fa);
            GLint coverage;
#endif
#ifdef INTERP_INDEX
            GLuint index = FixedToInt(fi) & 0xfffffff0;
            GLuint coverage;
#endif
#if DEPTH_BITS==16
            GLdepth z = FixedToInt(z0);
#else
            GLdepth z = z0;
#endif
            ASSERT(yBoti <= yTopi);

            {
#ifdef INTERP_STUV
               GLfloat invQ = 1.0F / hv0;
               GLfloat s = hs0 * invQ;
               GLfloat t = ht0 * invQ;
               GLfloat u = hu0 * invQ;
#endif
#ifdef INTERP_STUV1
               GLfloat invQ1 = 1.0F / hv01;
               GLfloat s1 = hs01 * invQ1;
               GLfloat t1 = ht01 * invQ1;
               GLfloat u1 = hu01 * invQ1;
#endif

               /* bottom pixel of swipe */
#ifdef INTERP_RGBA
               coverage = (GLint) (alpha * (1.0F - (yBot - yBoti)));
#endif
#ifdef INTERP_INDEX
               coverage = (GLuint) (15.0F * (1.0F - (yBot - yBoti)));
#endif
               PLOT(x, yBoti);
               yBoti++;

               /* top pixel of swipe */
#ifdef INTERP_RGBA
               coverage = (GLint) (alpha * (yTop - yTopi));
#endif
#ifdef INTERP_INDEX
               coverage = (GLuint) (15.0F * (yTop - yTopi));
#endif
               PLOT(x, yTopi);
               yTopi--;

               /* pixels between top and bottom with 100% coverage */
#ifdef INTERP_RGBA
               coverage = alpha;
#endif
#ifdef INTERP_INDEX
               coverage = 15;
#endif
               for (iy = yBoti; iy <= yTopi; iy++) {
                  PLOT(x, iy);
               }
            }
            PB_CHECK_FLUSH( ctx, pb );

         } /* if stippling */

         x += xStep;
         y += yStep;
         z0 += dz;
#ifdef INTERP_RGBA
         fr += dfr;
         fg += dfg;
         fb += dfb;
         fa += dfa;
#endif
#ifdef INTERP_STUV
         hs0 += dhs;
         ht0 += dht;
         hu0 += dhu;
         hv0 += dhv;
#endif
#ifdef INTERP_STUV1
         hs01 += dhs1;
         ht01 += dht1;
         hu01 += dhu1;
         hv01 += dhv1;
#endif
#ifdef INTERP_INDEX
         fi += dfi;
#endif

         if (!solid)
            ctx->StippleCounter++;
      }
   }
   else {
      /* Y-major line */
      GLint i;
      GLint y = y0;
      GLfloat x = VB->Win[vert0][0];
      GLfloat xStep = (VB->Win[vert1][0] - x) / (GLfloat) dy;
      GLint dz = (z1 - z0) / dy;
#ifdef INTERP_RGBA
      GLfloat invDy = 1.0F / dy;
      if (ctx->Light.ShadeModel == GL_SMOOTH) {
         dfr = (IntToFixed(VB->Color[vert1][0]) - fr) / dy;
         dfg = (IntToFixed(VB->Color[vert1][1]) - fg) / dy;
         dfb = (IntToFixed(VB->Color[vert1][2]) - fb) / dy;
         dfa = (IntToFixed(VB->Color[vert1][3]) - fa) / dy;
      }
#endif
#ifdef INTERP_STUV
      dhs *= invDy;
      dht *= invDy;
      dhu *= invDy;
      dhv *= invDy;
#endif
#ifdef INTERP_STUV1
      dhs1 *= invDy;
      dht1 *= invDy;
      dhu1 *= invDy;
      dhv1 *= invDy;
#endif
#ifdef INTERP_INDEX
      if (ctx->Light.ShadeModel == GL_SMOOTH) {
         dfi = (IntToFixed(VB->Index[vert1]) - fi) / dy;
      }
#endif
      for (i = 0; i < dy; i++) {
         if (solid || (ctx->Line.StipplePattern & (1 << ((ctx->StippleCounter/ctx->Line.StippleFactor) & 0xf)))) {
            GLfloat xRight = x + halfWidth;
            GLfloat xLeft = x - halfWidth;
            GLint xRighti = (GLint) xRight;
            GLint xLefti = (GLint) xLeft;
            GLint ix;
#ifdef INTERP_RGBA
            GLubyte red   = FixedToInt(fr);
            GLubyte green = FixedToInt(fg);
            GLubyte blue  = FixedToInt(fb);
            GLubyte alpha = FixedToInt(fa);
            GLint coverage;
#endif
#ifdef INTERP_INDEX
            GLuint index = FixedToInt(fi) & 0xfffffff0;
            GLuint coverage;
#endif
#if DEPTH_BITS==16
            GLdepth z = FixedToInt(z0);
#else
            GLdepth z = z0;
#endif

            ASSERT(xLefti < xRight);

            {
#ifdef INTERP_STUV
               GLfloat invQ = 1.0F / hv0;
               GLfloat s = hs0 * invQ;
               GLfloat t = ht0 * invQ;
               GLfloat u = hu0 * invQ;
#endif
#ifdef INTERP_STUV1
               GLfloat invQ1 = 1.0F / hv01;
               GLfloat s1 = hs01 * invQ1;
               GLfloat t1 = ht01 * invQ1;
               GLfloat u1 = hu01 * invQ1;
#endif

               /* left pixel of swipe */
#ifdef INTERP_RGBA
               coverage = (GLint) (alpha * (1.0F - (xLeft - xLefti)));
#endif
#ifdef INTERP_INDEX
               coverage = (GLuint) (15.0F * (1.0F - (xLeft - xLefti)));
#endif
               PLOT(xLefti, y);
               xLefti++;

               /* right pixel of swipe */
#ifdef INTERP_RGBA
               coverage = (GLint) (alpha * (xRight - xRighti));
#endif
#ifdef INTERP_INDEX
               coverage = (GLuint) (15.0F * (xRight - xRighti));
#endif
               PLOT(xRighti, y)
               xRighti--;

               /* pixels between top and bottom with 100% coverage */
#ifdef INTERP_RGBA
               coverage = alpha;
#endif
#ifdef INTERP_INDEX
               coverage = 15;
#endif
               for (ix = xLefti; ix <= xRighti; ix++) {
                  PLOT(ix, y);
               }
            }
            PB_CHECK_FLUSH( ctx, pb );
         }

         x += xStep;
         y += yStep;
         z0 += dz;
#ifdef INTERP_RGBA
         fr += dfr;
         fg += dfg;
         fb += dfb;
         fa += dfa;
#endif
#ifdef INTERP_STUV
         hs0 += dhs;
         ht0 += dht;
         hu0 += dhu;
         hv0 += dhv;
#endif
#ifdef INTERP_STUV1
         hs01 += dhs1;
         ht01 += dht1;
         hu01 += dhu1;
         hv01 += dhv1;
#endif
#ifdef INTERP_INDEX
         fi += dfi;
#endif
         if (!solid)
            ctx->StippleCounter++;
      }
   }
}

#undef INTERP_RGBA
#undef INTERP_STUV
#undef INTERP_STUV1
#undef INTERP_INDEX
#undef PLOT
