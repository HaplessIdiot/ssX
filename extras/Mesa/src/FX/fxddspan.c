/* -*- mode: C; tab-width:8; c-basic-offset:2 -*- */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
 *
 *
 * Original Mesa / 3Dfx device driver (C) 1999 David Bucciarelli, by the
 * terms stated above.
 *
 * Thank you for your contribution, David!
 *
 * Please make note of the above copyright/license statement.  If you
 * contributed code or bug fixes to this code under the previous (GNU
 * Library) license and object to the new license, your code will be
 * removed at your request.  Please see the Mesa docs/COPYRIGHT file
 * for more information.
 *
 * Additional Mesa/3Dfx driver developers:
 *   Daryll Strauss <daryll@precisioninsight.com>
 *   Keith Whitwell <keith@precisioninsight.com>
 *
 * See fxapi.h for more revision/author details.
 */


/* fxdd.c - 3Dfx VooDoo Mesa span and pixel functions */


#include "fxdrv.h"



/*
 * Examine the cliprects to generate an array of flags to indicate
 * which pixels in a span are visible.  Note: (x,y) is a screen
 * coordinate.
 */
static void
generate_vismask(const fxMesaContext fxMesa, GLint x, GLint y, GLint n,
                 GLubyte vismask[])
{
   GLboolean initialized = GL_FALSE;
   GLint i, j;

   /* turn on flags for all visible pixels */
   for (i = 0; i < fxMesa->numClipRects; i++) {
      const XF86DRIClipRectPtr rect = &fxMesa->pClipRects[i];

      if (y >= rect->y1 && y < rect->y2) {
         if (x >= rect->x1 && x + n <= rect->x2) {
            /* common case, whole span inside cliprect */
            MEMSET(vismask, 1, n);
            return;
         }
         if (x < rect->x2 && x + n >= rect->x1) {
            /* some of the span is inside the rect */
            GLint start, end;
            if (!initialized) {
               MEMSET(vismask, 0, n);
               initialized = GL_TRUE;
            }
            if (x < rect->x1)
               start = rect->x1 - x;
            else
               start = 0;
            if (x + n > rect->x2)
               end = rect->x2 - x;
            else
               end = n;
            assert(start >= 0);
            assert(end <= n);
            for (j = start; j < end; j++)
               vismask[j] = 1;
         }
      }
   }
}


/*
 * Examine cliprects and determine if the given screen pixel is visible.
 */
static GLboolean
visible_pixel(const fxMesaContext fxMesa, int scrX, int scrY)
{
   int i;
   for (i = 0; i < fxMesa->numClipRects; i++) {
      const XF86DRIClipRectPtr rect = &fxMesa->pClipRects[i];
      if (scrX >= rect->x1 &&
          scrX <  rect->x2 &&
          scrY >= rect->y1 &&
          scrY <  rect->y2)
        return GL_TRUE;
   }
   return GL_FALSE;
}



typedef enum { FBS_READ, FBS_WRITE } FBS_DIRECTION;
/*
 *     Read or write a single span from the frame buffer.
 *     Input Parameters:
 *         fxMesa:                The context
 *         target_buffer:         Which buffer to read from.
 *         xpos, ypos:            Starting Coordinates.
 *         xlength:               Length of the span.
 *         data_size:             Size of data elements: 1, 2, or 4.
 *         buffer:                Pointer to the buffer to read
 *                                to or write from.  This needs
 *                                to be turned into a pointer.
 *         direction:             Read or Write.
 */
static void
rw_fb_span(fxMesaContext fxMesa,
           GrBuffer_t    target_buffer,
           FxU32         xpos,
           FxU32         ypos,
           FxU32         xlength,
           FxU32         data_size,
           void         *buffer,
           FBS_DIRECTION direction)
{
  GrLfbInfo_t info;
  BEGIN_BOARD_LOCK();
  info.size=sizeof(info);

  if (grLfbLock(direction == FBS_READ ? GR_LFB_READ_ONLY : GR_LFB_WRITE_ONLY,
                target_buffer,
                GR_LFBWRITEMODE_ANY,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    const GLint winX = fxMesa->x_offset;
    const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
#ifdef XF86DRI
    /* stride in data elements */
    const GLint srcStride =
      (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT)
        ? (fxMesa->screen_width)
        : (info.strideInBytes / data_size);
#else
    /* stride in data elements */
    const GLint srcStride = info.strideInBytes / data_size;
#endif
    GLushort *data16 = (GLushort *) info.lfbPtr
      + (winY - ypos) * srcStride
      + (winX + xpos);
    GLubyte *target16 = (GLubyte *)  buffer;
    GLubyte *data8    = (GLubyte *)  data16;
    GLubyte *target8  = (GLubyte *)  buffer;
    GLuint  *data32   = (GLuint *)   data16;
    GLuint  *target32 = (GLuint *)  buffer;
    GLuint i, j;

    for (i = j = 0; i < xlength; i += 1, j += 1) {
      switch (data_size) {
      case 1:
        switch (direction) {
        case FBS_READ:
          *target8++ = *data8++;
          break;
        case FBS_WRITE:
          *data8++ = *target8++;
          break;
        }
        break;
      case 2:
        switch (direction) {
        case FBS_READ:
          *target16++ = *data16++;
          break;
        case FBS_WRITE:
          *data16++ = *target16++;
          break;
        }
        break;
      case 4:
        switch (direction) {
        case FBS_READ:
          *target32++ = *data32++;
          break;
        case FBS_WRITE:
          *data32++ = *target32++;
          break;
        }
        break;
      }
    }
    grLfbUnlock(direction == FBS_READ ? GR_LFB_READ_ONLY : GR_LFB_WRITE_ONLY,
                target_buffer);
  }
  END_BOARD_LOCK();
}
           
static FxBool
fb_point_is_clipped(fxMesaContext fxMesa,
                    FxU32 dst_x, FxU32 dst_y)
{
  int i;
  for (i=0; i<fxMesa->numClipRects; i++) {
    if ((dst_x>=fxMesa->pClipRects[i].x1) && 
        (dst_x<fxMesa->pClipRects[i].x2) &&
        (dst_y>=fxMesa->pClipRects[i].y1) && 
        (dst_y<fxMesa->pClipRects[i].y2)) {
      return GL_FALSE;
    }
  }
  return GL_TRUE;
}

static FxBool writeRegionClipped(fxMesaContext fxMesa, GrBuffer_t dst_buffer,
				 FxU32 dst_x, FxU32 dst_y, GrLfbSrcFmt_t src_format,
				 FxU32 src_width, FxU32 src_height, FxI32 src_stride,
				 void *src_data)
{
  int i, x, w, srcElt;
  void *data;

  if (src_width==1 && src_height==1) { /* Easy case writing a point */
    for (i=0; i<fxMesa->numClipRects; i++) {
      if ((dst_x>=fxMesa->pClipRects[i].x1) && 
          (dst_x<fxMesa->pClipRects[i].x2) &&
          (dst_y>=fxMesa->pClipRects[i].y1) && 
          (dst_y<fxMesa->pClipRects[i].y2)) {
        FX_grLfbWriteRegion(dst_buffer, dst_x, dst_y, src_format,
                            1, 1, src_stride, src_data);
        return GL_TRUE;
      }
    }
  } else if (src_height==1) { /* Writing a span */
    if (src_format==GR_LFB_SRC_FMT_8888) srcElt=4;
    else if (src_format==GR_LFB_SRC_FMT_ZA16) srcElt=2;
    else {
      fprintf(stderr, "Unknown src_format passed to writeRegionClipped\n");
      return GL_FALSE;
    }
    for (i=0; i<fxMesa->numClipRects; i++) {
      if (dst_y>=fxMesa->pClipRects[i].y1 && dst_y<fxMesa->pClipRects[i].y2) {
        if (dst_x<fxMesa->pClipRects[i].x1) {
          x=fxMesa->pClipRects[i].x1;
          data=((char*)src_data)+srcElt*(x - dst_x);
          w=src_width-(x-dst_x);
        } else {
          x=dst_x;
          data=src_data;
          w=src_width;
        }
        if (x+w>fxMesa->pClipRects[i].x2) {
          w=fxMesa->pClipRects[i].x2-x;
        }
        FX_grLfbWriteRegion(dst_buffer, x, dst_y, src_format, w, 1,
                            src_stride, data);
      }
    }
  } else { /* Punt on the case of arbitrary rectangles */
    return GL_FALSE;
  }
  return GL_TRUE;
}



/* KW: Rearranged the args in the call to grLfbWriteRegion().
 */
#define LFB_WRITE_SPAN_MESA(dst_buffer,         \
                            dst_x,              \
                            dst_y,              \
                            src_width,          \
                            src_stride,         \
                            src_data)           \
  writeRegionClipped(fxMesa, dst_buffer,        \
                   dst_x,                       \
                   dst_y,                       \
                   GR_LFB_SRC_FMT_8888,         \
                   src_width,                   \
                   1,                           \
                   src_stride,                  \
                   src_data)                    \




/*
 * 16bpp span/pixel functions
 */

static void
write_R5G6B5_rgba_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                       const GLubyte rgba[][4], const GLubyte mask[])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint i;
  GLint bottom=fxMesa->height+fxMesa->y_offset-1; 

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDWriteRGBASpan(...)\n");
  }

  x+=fxMesa->x_offset;
  if (mask) {
    int span=0;

    for (i=0;i<n;i++) {
      if (mask[i]) {
        ++span; 
      } else {
        if (span > 0) {
          LFB_WRITE_SPAN_MESA( fxMesa->currentFB, x+i-span, bottom-y,
			       /* GR_LFB_SRC_FMT_8888,*/ span, /*1,*/ 0, (void *) rgba[i-span] );
          span = 0;
        }
      }
    }

    if (span > 0)
      LFB_WRITE_SPAN_MESA( fxMesa->currentFB, x+n-span, bottom-y,
			   /* GR_LFB_SRC_FMT_8888, */ span, /*1,*/ 0, (void *) rgba[n-span] );
  } else
    LFB_WRITE_SPAN_MESA( fxMesa->currentFB, x, bottom-y,/* GR_LFB_SRC_FMT_8888,*/
			 n,/* 1,*/ 0, (void *) rgba );
}


static void
write_R5G6B5_rgb_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                      const GLubyte rgb[][3], const GLubyte mask[])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint i;
  GLint bottom=fxMesa->height+fxMesa->y_offset-1;
  GLubyte rgba[MAX_WIDTH][4];

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDWriteRGBSpan()\n");
  }

  x+=fxMesa->x_offset;
  if (mask) {
    int span=0;

    for (i=0;i<n;i++) {
      if (mask[i]) {
        rgba[span][RCOMP] = rgb[i][0];
        rgba[span][GCOMP] = rgb[i][1];
        rgba[span][BCOMP] = rgb[i][2];
        rgba[span][ACOMP] = 255;
        ++span;
      } else {
        if (span > 0) {
          LFB_WRITE_SPAN_MESA( fxMesa->currentFB, x+i-span, bottom-y,
			       /*GR_LFB_SRC_FMT_8888,*/ span,/* 1,*/ 0, (void *) rgba );
          span = 0;
        }
      }
    }

    if (span > 0)
      LFB_WRITE_SPAN_MESA( fxMesa->currentFB, x+n-span, bottom-y,
			   /*GR_LFB_SRC_FMT_8888,*/ span,/* 1,*/ 0, (void *) rgba );
  } else {
    for (i=0;i<n;i++) {
      rgba[i][RCOMP]=rgb[i][0];
      rgba[i][GCOMP]=rgb[i][1];
      rgba[i][BCOMP]=rgb[i][2];
      rgba[i][ACOMP]=255;
    }

    LFB_WRITE_SPAN_MESA( fxMesa->currentFB, x, bottom-y,/* GR_LFB_SRC_FMT_8888,*/
			 n,/* 1,*/ 0, (void *) rgba );
  }
}


static void
write_R5G6B5_mono_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                       const GLubyte mask[])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint i;
  GLint bottom=fxMesa->height+fxMesa->y_offset-1;
  GLuint data[MAX_WIDTH];

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDWriteMonoRGBASpan(...)\n");
  }

  x+=fxMesa->x_offset;
  if (mask) {
    int span=0;

    for (i=0;i<n;i++) {
      if (mask[i]) {
        data[span] = (GLuint) fxMesa->color;
        ++span;
      } else {
        if (span > 0) {
          writeRegionClipped(fxMesa,  fxMesa->currentFB, x+i-span, bottom-y,
			     GR_LFB_SRC_FMT_8888, span, 1, 0,
			     (void *) data );
          span = 0;
        }
      }
    }

    if (span > 0)
      writeRegionClipped(fxMesa,  fxMesa->currentFB, x+n-span, bottom-y,
			 GR_LFB_SRC_FMT_8888, span, 1, 0,
			 (void *) data );
  } else {
    for (i=0;i<n;i++) {
      data[i]=(GLuint) fxMesa->color;
    }

    writeRegionClipped(fxMesa,  fxMesa->currentFB, x, bottom-y, GR_LFB_SRC_FMT_8888,
		       n, 1, 0, (void *) data );
  }
}


/*
 * Read a span of 16-bit RGB pixels.  Note, we don't worry about cliprects
 * since OpenGL says obscured pixels have undefined values.
 */
static void
read_R5G6B5_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                 GLubyte rgba[][4])
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GrLfbInfo_t info;
  BEGIN_BOARD_LOCK();
  info.size=sizeof(info);
  if (grLfbLock(GR_LFB_READ_ONLY,
                fxMesa->currentFB,
                GR_LFBWRITEMODE_ANY,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    const GLint winX = fxMesa->x_offset;
    const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
#ifdef XF86DRI
    const GLint srcStride = (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT)
      ? (fxMesa->screen_width) : (info.strideInBytes / 2);
#else
    const GLint srcStride = info.strideInBytes / 2; /* stride in GLushorts */
#endif
    const GLushort *data16 = (const GLushort *) info.lfbPtr
      + (winY - y) * srcStride
      + (winX + x);
    const GLuint *data32 = (const GLuint *) data16;
    GLuint i, j;
    GLuint extraPixel = (n & 1);
    n -= extraPixel;
    for (i = j = 0; i < n; i += 2, j++) {
      GLuint pixel = data32[j];
      GLuint pixel0 = pixel & 0xffff;
      GLuint pixel1 = pixel >> 16;
      rgba[i][RCOMP] = FX_PixelToR[pixel0];
      rgba[i][GCOMP] = FX_PixelToG[pixel0];
      rgba[i][BCOMP] = FX_PixelToB[pixel0];
      rgba[i][ACOMP] = 255;
      rgba[i+1][RCOMP] = FX_PixelToR[pixel1];
      rgba[i+1][GCOMP] = FX_PixelToG[pixel1];
      rgba[i+1][BCOMP] = FX_PixelToB[pixel1];
      rgba[i+1][ACOMP] = 255;
    }
    if (extraPixel) {
      GLushort pixel = data16[n];
      rgba[n][RCOMP] = FX_PixelToR[pixel];
      rgba[n][GCOMP] = FX_PixelToG[pixel];
      rgba[n][BCOMP] = FX_PixelToB[pixel];
      rgba[n][ACOMP] = 255;
    }

    grLfbUnlock(GR_LFB_READ_ONLY, fxMesa->currentFB);
  }
  END_BOARD_LOCK();
}


static void
write_R5G6B5_pixels(const GLcontext *ctx,
                    GLuint n, const GLint x[], const GLint y[],
                    CONST GLubyte rgba[][4], const GLubyte mask[])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint i;
  GLint bottom=fxMesa->height+fxMesa->y_offset-1;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDWriteRGBAPixels(...)\n");
  }

  for(i=0;i<n;i++)
    if(mask[i])
      LFB_WRITE_SPAN_MESA(fxMesa->currentFB, x[i]+fxMesa->x_offset, bottom-y[i],
			  1, 1, (void *)rgba[i]);
}


static void
write_R5G6B5_mono_pixels(const GLcontext *ctx,
                         GLuint n, const GLint x[], const GLint y[],
                         const GLubyte mask[])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint i;
  GLint bottom=fxMesa->height+fxMesa->y_offset-1;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDWriteMonoRGBAPixels(...)\n");
  }

  for(i=0;i<n;i++)
    if(mask[i])
      writeRegionClipped(fxMesa, fxMesa->currentFB,x[i]+fxMesa->x_offset,bottom-y[i],
			 GR_LFB_SRC_FMT_8888,1,1,0,(void *) &fxMesa->color);
}

static void
read_R5G6B5_pixels(const GLcontext *ctx,
                   GLuint n, const GLint x[], const GLint y[],
                   GLubyte rgba[][4], const GLubyte mask[])
{
  fxMesaContext fxMesa=(fxMesaContext)ctx->DriverCtx;
  GLuint i;
  GLint bottom=fxMesa->height+fxMesa->y_offset-1;

  if (MESA_VERBOSE&VERBOSE_DRIVER) {
    fprintf(stderr,"fxmesa: fxDDReadRGBAPixels(...)\n");
  }

  for(i=0;i<n;i++) {
    if(mask[i]) {
      GLushort pixel;
      FX_grLfbReadRegion(fxMesa->currentFB,x[i],bottom-y[i],1,1,0,&pixel);
      rgba[i][RCOMP] = FX_PixelToR[pixel];
      rgba[i][GCOMP] = FX_PixelToG[pixel];
      rgba[i][BCOMP] = FX_PixelToB[pixel];
      rgba[i][ACOMP] = 255;
    }
  }
}


/*
 * 24bpp span/pixel functions
 */

static void
write_R8G8B8_rgb_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                      const GLubyte rgb[][3], const GLubyte mask[])
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GrLfbWriteMode_t mode;
  GrLfbInfo_t info;

  if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT)
    mode = GR_LFBWRITEMODE_888;
  else
    mode = GR_LFBWRITEMODE_888;

  BEGIN_BOARD_LOCK();
  info.size = sizeof(info);
  if (grLfbLock(GR_LFB_WRITE_ONLY,
                fxMesa->currentFB,
                mode,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
    const GLint winX = fxMesa->x_offset;
    const GLint scrX = winX + x;
    const GLint scrY = winY - y;

    if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT) {
      /*GLint dstStride = fxMesa->screen_width * 3;*/
      GLint dstStride = info.strideInBytes / 1;
      GLubyte *dst = (GLubyte *) info.lfbPtr
                   + (winY - y) * dstStride + (winX + x) * 1;
      GLuint *dst32 = (GLuint *) dst;
      GLubyte visMask[MAX_WIDTH];
      GLuint i;
      generate_vismask(fxMesa, scrX, scrY, n, visMask);
      for (i = 0; i < n; i++) {
        if (visMask[i] && (!mask || mask[i])) {
          dst32[i] = PACK_BGRA32(rgb[i][0], rgb[i][1], rgb[i][2], 255);
        }
      }
    }
    else {
      /* back buffer */
      GLint dstStride = info.strideInBytes;
      GLubyte *dst = (GLubyte *) info.lfbPtr
                   + (winY - y) * dstStride + (winX + x) * 4;
      GLuint *dst32 = (GLuint *) dst;
      if (mask) {
        GLuint i;
        for (i = 0; i < n; i++) {
          if (mask[i]) {
            dst32[i] = PACK_RGBA32(rgb[i][0], rgb[i][1], rgb[i][2], 255);
          }
        }
      }
      else {
        GLuint i;
        for (i = 0; i < n; i++) {
          dst32[i] = PACK_RGBA32(rgb[i][2], rgb[i][1], rgb[i][0], 255);
        }
      }
    }
    grLfbUnlock(GR_LFB_WRITE_ONLY, fxMesa->currentFB);
  }
  END_BOARD_LOCK();
}



static void
write_R8G8B8_rgba_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                       const GLubyte rgba[][4], const GLubyte mask[])
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GrLfbWriteMode_t mode;
  GrLfbInfo_t info;

  if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT)
    mode = GR_LFBWRITEMODE_8888;
  else
    mode = GR_LFBWRITEMODE_888 /*565*/;

  BEGIN_BOARD_LOCK();
  info.size = sizeof(info);
  if (grLfbLock(GR_LFB_WRITE_ONLY,
                fxMesa->currentFB,
                mode,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
    const GLint winX = fxMesa->x_offset;
    const GLint scrX = winX + x;
    const GLint scrY = winY - y;

    if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT) {
      /* XXX have to do cliprect clipping! */
      GLint dstStride = fxMesa->screen_width * 4;
      GLubyte *dst = (GLubyte *) info.lfbPtr
                   + (winY - y) * dstStride + (winX + x) * 4;
      GLuint *dst32 = (GLuint *) dst;
      GLubyte visMask[MAX_WIDTH];
      GLuint i;
      generate_vismask(fxMesa, scrX, scrY, n, visMask);
      for (i = 0; i < n; i++) {
        if (visMask[i] && (!mask || mask[i])) {
          dst32[i] = PACK_BGRA32(rgba[i][0], rgba[i][1], rgba[i][2], rgba[i][3]);
        }
      }
    }
    else {
      /* back buffer */
      GLint dstStride = info.strideInBytes;
      GLubyte *dst = (GLubyte *) info.lfbPtr
                   + (winY - y) * dstStride + (winX + x) * 4;
      if (mask) {
        const GLuint *src32 = (const GLuint *) rgba;
        GLuint *dst32 = (GLuint *) dst;
        GLuint i;
        for (i = 0; i < n; i++) {
          if (mask[i]) {
            dst32[i] = src32[i];
          }
        }
      }
      else {
        /* no mask, write all pixels */
        MEMCPY(dst, rgba, 4 * n);
      }
    }
    grLfbUnlock(GR_LFB_WRITE_ONLY, fxMesa->currentFB);
  }
  END_BOARD_LOCK();
}


static void
write_R8G8B8_mono_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                       const GLubyte mask[])
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GLubyte rgba[MAX_WIDTH][4];
  GLuint *data = (GLuint *) rgba;
  GLuint i;

  /* XXX this is a simple-minded implementation but good enough for now */
  for (i = 0; i < n; i++) {
    data[i] = (GLuint) fxMesa->color;
  }
  write_R8G8B8_rgba_span(ctx, n, x, y, (const GLubyte (*)[4]) rgba, mask);
}


static void
read_R8G8B8_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                 GLubyte rgba[][4])
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GrLfbWriteMode_t mode;
  GrLfbInfo_t info;

  if (1 || fxMesa->glCtx->Color.DrawBuffer == GL_FRONT) {
    mode = GR_LFBWRITEMODE_8888;
  }
  else {
    mode = GR_LFBWRITEMODE_565; /*888*/ /*565*/;
  }

  BEGIN_BOARD_LOCK();
  info.size = sizeof(info);
  if (grLfbLock(GR_LFB_READ_ONLY,
                fxMesa->currentFB,
                mode, /*GR_LFBWRITEMODE_ANY,*/
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
    const GLint winX = fxMesa->x_offset;

    if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT) {
      GLint srcStride = fxMesa->screen_width * 4;
      const GLubyte *src = (const GLubyte *) info.lfbPtr
                         + (winY - y) * srcStride + (winX + x) * 4;
      GLuint i;
      for (i = 0; i < n; i++) {
        rgba[i][0] = src[i * 4 + 2];
        rgba[i][1] = src[i * 4 + 1];
        rgba[i][2] = src[i * 4 + 0];
        rgba[i][3] = src[i * 4 + 3];
      }
    }
    else {
      /* back buffer */
      GLint srcStride = /*info.strideInBytes;*/ 8192 / 2;  /* XXX a hack! */
      const GLubyte *src = (const GLubyte *) info.lfbPtr
                         + (winY - y) * srcStride + (winX + x) * 4;
      GLuint i;
      for (i = 0; i < n; i++) {
        rgba[i][0] = src[i * 4 + 2];
        rgba[i][1] = src[i * 4 + 1];
        rgba[i][2] = src[i * 4 + 0];
        rgba[i][3] = src[i * 4 + 3];
      }
    }
    grLfbUnlock(GR_LFB_READ_ONLY, fxMesa->currentFB);
  }
  END_BOARD_LOCK();
}


static void
write_R8G8B8_pixels(const GLcontext *ctx,
                    GLuint n, const GLint x[], const GLint y[],
                    CONST GLubyte rgba[][4], const GLubyte mask[])
{
#if 00
  GLuint i;
  for (i = 0; i < n; i++) {
    write_R8G8B8_rgba_span(ctx, 1, x[i], y[i], rgba + i, mask + i);
  }

#else
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GrLfbWriteMode_t mode;
  GrLfbInfo_t info;

  if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT)
    mode = GR_LFBWRITEMODE_8888;
  else
    mode = GR_LFBWRITEMODE_888 /*565*/;

  BEGIN_BOARD_LOCK();
  info.size = sizeof(info);
  if (grLfbLock(GR_LFB_WRITE_ONLY,
                fxMesa->currentFB,
                mode,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT) {
      GLuint i;
      for (i = 0; i < n; i++) {
        const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
        const GLint winX = fxMesa->x_offset;
        const GLint scrX = winX + x[i];
        const GLint scrY = winY - y[i];
        if (mask[i] && visible_pixel(fxMesa, scrX, scrY)) {
          GLint dstStride = fxMesa->screen_width * 4;
          GLubyte *dst = (GLubyte *) info.lfbPtr + scrY * dstStride + scrX * 4;
          GLuint *dst32 = (GLuint *) dst;
          *dst32 = PACK_BGRA32(rgba[i][0], rgba[i][1],
                               rgba[i][2], rgba[i][3]);
        }
      }
    }
    else {
      /* back buffer */
      GLuint i;
      for (i = 0; i < n; i++) {
        const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
        const GLint winX = fxMesa->x_offset;
        const GLint scrX = winX + x[i];
        const GLint scrY = winY - y[i];
        if (mask[i] && visible_pixel(fxMesa, scrX, scrY)) {
          GLint dstStride = info.strideInBytes;
          GLubyte *dst = (GLubyte *) info.lfbPtr + scrY * dstStride + scrX * 4;
          GLuint *dst32 = (GLuint *) dst;
          const GLuint *src32 = (const GLuint *) rgba;
          *dst32 = src32[i];
        }
      }
    }
    grLfbUnlock(GR_LFB_WRITE_ONLY, fxMesa->currentFB);
  }
  END_BOARD_LOCK();
#endif
}


static void
write_R8G8B8_mono_pixels(const GLcontext *ctx,
                         GLuint n, const GLint x[], const GLint y[],
                         const GLubyte mask[])
{
}


static void
read_R8G8B8_pixels(const GLcontext *ctx,
                   GLuint n, const GLint x[], const GLint y[],
                   GLubyte rgba[][4], const GLubyte mask[])
{
  printf("read_R8G8B8_pixels %d\n", n);
}



/*
 * 32bpp span/pixel functions
 */

static void
write_R8G8B8A8_rgb_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                        const GLubyte rgb[][3], const GLubyte mask[])
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GrLfbWriteMode_t mode;
  GrLfbInfo_t info;

  if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT)
    mode = GR_LFBWRITEMODE_8888;
  else
    mode = GR_LFBWRITEMODE_888 /*565*/;

  BEGIN_BOARD_LOCK();
  info.size = sizeof(info);
  if (grLfbLock(GR_LFB_WRITE_ONLY,
                fxMesa->currentFB,
                mode,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
    const GLint winX = fxMesa->x_offset;
    const GLint scrX = winX + x;
    const GLint scrY = winY - y;

    if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT) {
      GLint dstStride = fxMesa->screen_width * 4;
      GLubyte * dst = (GLubyte *) info.lfbPtr
                    + (winY - y) * dstStride + (winX + x) * 4;
      GLuint *dst32 = (GLuint *) dst;
      GLubyte visMask[MAX_WIDTH];
      GLuint i;
      generate_vismask(fxMesa, scrX, scrY, n, visMask);
      for (i = 0; i < n; i++) {
        if (visMask[i] && (!mask || mask[i])) {
          dst32[i] = PACK_BGRA32(rgb[i][0], rgb[i][1], rgb[i][2], 255);
        }
      }
    }
    else {
      /* back buffer */
      GLint dstStride = info.strideInBytes;
      GLubyte *dst = (GLubyte *) info.lfbPtr
                   + (winY - y) * dstStride + (winX + x) * 4;
      if (mask) {
        GLuint *dst32 = (GLuint *) dst;
        GLuint i;
        for (i = 0; i < n; i++) {
          if (mask[i]) {
            dst32[i] = PACK_RGBA32(rgb[i][0], rgb[i][1], rgb[i][2], 255);
          }
        }
      }
      else {
        GLuint *dst32 = (GLuint *) dst;
        GLuint i;
        for (i = 0; i < n; i++) {
          dst32[i] = PACK_RGBA32(rgb[i][0], rgb[i][1], rgb[i][2], 255);
        }
      }
    }
    grLfbUnlock(GR_LFB_WRITE_ONLY, fxMesa->currentFB);
  }
  END_BOARD_LOCK();
}


/*
 *XXX test of grLfbWriteRegion in 32bpp mode.  Doesn't seem to work!
 */
#if 0
static void
write_R8G8B8A8_rgb_span2(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                        const GLubyte rgb[][3], const GLubyte mask[])
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1; 
  GLint x2 = fxMesa->x_offset +x;
  GLint y2 = bottom - y;

  FX_grLfbWriteRegion(fxMesa->currentFB, x2, y2, GR_LFB_SRC_FMT_888,
                      n, 1, 0, rgb);
}
#endif


static void
write_R8G8B8A8_rgba_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                         const GLubyte rgba[][4], const GLubyte mask[])
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GrLfbWriteMode_t mode;
  GrLfbInfo_t info;

  if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT)
    mode = GR_LFBWRITEMODE_8888;
  else
    mode = GR_LFBWRITEMODE_888 /*565*/;

  BEGIN_BOARD_LOCK();
  info.size = sizeof(info);
  if (grLfbLock(GR_LFB_WRITE_ONLY,
                fxMesa->currentFB,
                mode,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
    const GLint winX = fxMesa->x_offset;
    const GLint scrX = winX + x;
    const GLint scrY = winY - y;

    if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT) {
      GLint dstStride = fxMesa->screen_width * 4;
      GLubyte *dst = (GLubyte *) info.lfbPtr
                   + (winY - y) * dstStride + (winX + x) * 4;
      GLuint *dst32 = (GLuint *) dst;
      GLubyte visMask[MAX_WIDTH];
      GLuint i;
      generate_vismask(fxMesa, scrX, scrY, n, visMask);
      for (i = 0; i < n; i++) {
        if (visMask[i] && (!mask || mask[i])) {
          dst32[i] = PACK_BGRA32(rgba[i][0], rgba[i][1], rgba[i][2], rgba[i][3]);
        }
      }
    }
    else {
      /* back buffer */
      GLint dstStride = 8192; /* XXX a hack  info.strideInBytes; */
      GLubyte *dst = (GLubyte *) info.lfbPtr
                   + (winY - y) * dstStride + (winX + x) * 4;
      if (mask) {
        const GLuint *src32 = (const GLuint *) rgba;
        GLuint *dst32 = (GLuint *) dst;
        GLuint i;
        for (i = 0; i < n; i++) {
          if (mask[i]) {
            dst32[i] = src32[i];
          }
        }
      }
      else {
        /* no mask, write all pixels */
        MEMCPY(dst, rgba, 4 * n);
      }
    }
    grLfbUnlock(GR_LFB_WRITE_ONLY, fxMesa->currentFB);
  }
  else {
    info.strideInBytes = -1;
  }
  END_BOARD_LOCK();
}


static void
write_R8G8B8A8_mono_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                         const GLubyte mask[])
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GLubyte rgba[MAX_WIDTH][4];
  GLuint *data = (GLuint *) rgba;
  GLuint i;

  /* XXX this is a simple-minded implementation but good enough for now */
  for (i = 0; i < n; i++) {
    data[i] = (GLuint) fxMesa->color;
  }
  write_R8G8B8A8_rgba_span(ctx, n, x, y, (const GLubyte (*)[4]) rgba, mask);
}


static void
read_R8G8B8A8_span(const GLcontext *ctx, GLuint n, GLint x, GLint y,
                   GLubyte rgba[][4])
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GrLfbWriteMode_t mode;
  GrLfbInfo_t info;

  if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT) {
    mode = GR_LFBWRITEMODE_8888;
  }
  else {
    mode = GR_LFBWRITEMODE_8888; /*565;*/
  }

  BEGIN_BOARD_LOCK();
  info.size = sizeof(info);
  if (grLfbLock(GR_LFB_READ_ONLY,
                fxMesa->currentFB,
                mode,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
    const GLint winX = fxMesa->x_offset;

    if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT) {
      GLint srcStride = fxMesa->screen_width;
      const GLuint *src32 = (const GLuint *) info.lfbPtr
                          + (winY - y) * srcStride + (winX + x);
      GLuint i;
      for (i = 0; i < n; i++) {
        const GLuint p = src32[i];
        rgba[i][0] = (p >> 16) & 0xff;
        rgba[i][1] = (p >>  8) & 0xff;
        rgba[i][2] = (p >>  0) & 0xff;
        rgba[i][3] = (p >> 24) & 0xff;
      }
    }
    else {
      /* back buffer */
      GLint srcStride = 1024;  /* XXX a hack */
      const GLuint *src32 = (const GLuint *) info.lfbPtr
                          + (winY - y) * srcStride + (winX + x);
      GLuint i;
      for (i = 0; i < n; i++) {
        GLuint p = src32[i];
        rgba[i][0] = (p >> 16) & 0xff;
        rgba[i][1] = (p >>  8) & 0xff;
        rgba[i][2] = (p >>  0) & 0xff;
        rgba[i][3] = (p >> 24) & 0xff;
      }
    }
    grLfbUnlock(GR_LFB_READ_ONLY, fxMesa->currentFB);
  }
  else
    info.strideInBytes = -1;
  END_BOARD_LOCK();
}


static void
write_R8G8B8A8_pixels(const GLcontext *ctx,
                      GLuint n, const GLint x[], const GLint y[],
                      CONST GLubyte rgba[][4], const GLubyte mask[])
{
#if 00
  GLuint i;
  for (i = 0; i < n; i++) {
    write_R8G8B8A8_rgba_span(ctx, 1, x[i], y[i], rgba + i, mask + i);
  }

#else
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GrLfbWriteMode_t mode;
  GrLfbInfo_t info;

  if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT)
    mode = GR_LFBWRITEMODE_8888;
  else
    mode = GR_LFBWRITEMODE_888 /*565*/;

  BEGIN_BOARD_LOCK();
  info.size = sizeof(info);
  if (grLfbLock(GR_LFB_WRITE_ONLY,
                fxMesa->currentFB,
                mode,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    if (fxMesa->glCtx->Color.DrawBuffer == GL_FRONT) {
      GLuint i;
      for (i = 0; i < n; i++) {
        const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
        const GLint winX = fxMesa->x_offset;
        const GLint scrX = winX + x[i];
        const GLint scrY = winY - y[i];
        if (mask[i] && visible_pixel(fxMesa, scrX, scrY)) {
          GLint dstStride = fxMesa->screen_width * 4;
          GLubyte *dst = (GLubyte *) info.lfbPtr + scrY * dstStride + scrX * 4;
          GLuint *dst32 = (GLuint *) dst;
          *dst32 = PACK_BGRA32(rgba[i][0], rgba[i][1],
                               rgba[i][2], rgba[i][3]);
        }
      }
    }
    else {
      /* back buffer */
      GLuint i;
      for (i = 0; i < n; i++) {
        const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
        const GLint winX = fxMesa->x_offset;
        const GLint scrX = winX + x[i];
        const GLint scrY = winY - y[i];
        if (mask[i] && visible_pixel(fxMesa, scrX, scrY)) {
          GLint dstStride = info.strideInBytes;
          GLubyte *dst = (GLubyte *) info.lfbPtr + scrY * dstStride + scrX * 4;
          GLuint *dst32 = (GLuint *) dst;
          const GLuint *src32 = (const GLuint *) rgba;
          *dst32 = src32[i];
        }
      }
    }
    grLfbUnlock(GR_LFB_WRITE_ONLY, fxMesa->currentFB);
  }
  END_BOARD_LOCK();
#endif
}


static void
write_R8G8B8A8_mono_pixels(const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           const GLubyte mask[])
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx; 
  GLuint i;
  GLuint color = fxMesa->color;
  for (i = 0; i < n; i++) {
    if (mask[i]) {
      write_R8G8B8A8_rgba_span(ctx, 1, x[i], y[i],
                               (const GLubyte (*)[4]) &color, mask + i);
    }
  }
}



static void
read_R8G8B8A8_pixels(const GLcontext *ctx,
                     GLuint n, const GLint x[], const GLint y[],
                     GLubyte rgba[][4], const GLubyte mask[])
{
  GLuint i;
  for (i = 0; i < n; i++) {
    if (mask[i]) {
      read_R8G8B8A8_span(ctx, 1, x[i], y[i], rgba + i);
    }
  }
  printf("read_R8G8B8A8_pixels %d\n", n);
}



/*
 * Depth buffer read/write functions.
 */

void fxDDWriteDepthSpan(GLcontext *ctx,
                        GLuint n, GLint x, GLint y, const GLdepth depth[],
                        const GLubyte mask[])
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
  GLuint depth_size = fxMesa->glVis->DepthBits;
  GLuint stencil_size = fxMesa->glVis->StencilBits;

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
    fprintf(stderr, "fxmesa: fxDDWriteDepthSpan(...)\n");
  }

  x += fxMesa->x_offset;

  if (mask) {
    GLint i;
    for (i = 0; i < n; i++) {
      if (mask[i]) {
	GLshort d16;
	GLuint d32;
	switch (depth_size) {
	case 16:
	  d16 = depth[i];
	  writeRegionClipped(fxMesa, GR_BUFFER_AUXBUFFER, x + i, bottom - y,
			     GR_LFB_SRC_FMT_ZA16, 1, 1, 0, (void *) &d16);
	  break;
	case 32:
	  if (!fb_point_is_clipped(fxMesa, x+i, bottom-y)) {
	    if (stencil_size > 0) {
	      rw_fb_span(fxMesa,
			 GR_BUFFER_AUXBUFFER,
			 x + i, bottom - y,
			 1,
			 sizeof(GLdepth),
			 &d32,
			 FBS_READ);
	      d32 = depth[i] & 0xFF000000;
	      break;
	    } else {
	      d32 = depth[i];
	    }
	    rw_fb_span(fxMesa,
		       GR_BUFFER_AUXBUFFER,
		       x + i, bottom - y,
		       1,
		       sizeof(GLdepth),
		       &d32,
		       FBS_WRITE);
	  }
	}
      }
    }
  } else {
    GLushort depth16[MAX_WIDTH];
    GLint i;
    GLuint d32;
    switch (depth_size) {
    case 16:
      for (i = 0; i < n; i++) {
	depth16[i] = depth[i];
      }
      writeRegionClipped(fxMesa, GR_BUFFER_AUXBUFFER, x, bottom - y,
			 GR_LFB_SRC_FMT_ZA16, n, 1, 0, (void *) depth16);
      break;
    case 32:
      for (i = 0; i < n; i++) {
	if (fb_point_is_clipped(fxMesa, x+i, bottom-y)) {
	  if (stencil_size > 0) {
	    rw_fb_span(fxMesa,
		       GR_BUFFER_AUXBUFFER,
		       x + i, bottom - y,
		       1,
		       sizeof(GLdepth),
		       &d32,
		       FBS_READ);
	    d32 = (d32 & 0xFF0000) | (depth[i] & 0x00FFFFFF);
	  } else {
	    d32 = depth[i];
	  }
	  rw_fb_span(fxMesa,
		     GR_BUFFER_AUXBUFFER,
		     x + i, bottom - y,
		     1,
		     sizeof(GLdepth),
		     &d32,
		     FBS_WRITE);
	}
      }	   
    }
  }
}


void fxDDReadDepthSpan(GLcontext *ctx,
                       GLuint n, GLint x, GLint y, GLdepth depth[])
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
  GLushort depth16[MAX_WIDTH];
  GLuint i;
  GLuint depth_size = fxMesa->glVis->DepthBits;
  GLuint stencil_size = fxMesa->glVis->StencilBits;

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
    fprintf(stderr, "fxmesa: fxDDReadDepthSpan(...)\n");
  }

  x += fxMesa->x_offset;
  switch (depth_size) {
  case 16:
    FX_grLfbReadRegion(GR_BUFFER_AUXBUFFER, x, bottom - y, n, 1, 0, depth16);
    for (i = 0; i < n; i++) {
      depth[i] = depth16[i];
    }
    break;
  case 32:
    rw_fb_span(fxMesa,
	       GR_BUFFER_AUXBUFFER,
	       x, bottom - y,
	       n,
	       sizeof(GLdepth),
	       depth,
	       FBS_READ);
    if (stencil_size > 0) {
      for (i = 0; i < n; i++) {
	depth[i] &= 0xFFFFFF;
      }
    }
  }
}



void fxDDWriteDepthPixels(GLcontext *ctx,
                          GLuint n, const GLint x[], const GLint y[],
                          const GLdepth depth[], const GLubyte mask[])
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
  GLuint i;
  GLuint d32;
  GLuint depth_size = fxMesa->glVis->DepthBits;
  GLuint stencil_size = fxMesa->glVis->StencilBits;

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
    fprintf(stderr, "fxmesa: fxDDWriteDepthPixels(...)\n");
  }

  for (i = 0; i < n; i++) {
    if (mask[i]) {
      int xpos = x[i] + fxMesa->x_offset;
      int ypos = bottom - y[i];
      GLushort d16 = depth[i];
      switch (depth_size) {
      case 16:
	writeRegionClipped(fxMesa, GR_BUFFER_AUXBUFFER, xpos, ypos,
			   GR_LFB_SRC_FMT_ZA16, 1, 1, 0,
			   (void *) &d16);
	break;
      case 32:
        if (!fb_point_is_clipped(fxMesa, xpos, ypos)) {
          if (stencil_size > 0) {
	    /*
	     * Should I sign extend this?  I don't
	     * believe so, since GLdepth is unsigned.
	     */
            rw_fb_span(fxMesa,
                       GR_BUFFER_AUXBUFFER,
                       xpos, ypos,
                       1,
                       sizeof(GLdepth),
                       &d32,
                       FBS_READ);
            d32 = (d32 &~ 0xFF000000) | (depth[i] & 0x00FFFFFF);
          } else {
            d32 = depth[i];
          }
          rw_fb_span(fxMesa,
                     GR_BUFFER_AUXBUFFER,
                     xpos, ypos,
                     1,
                     sizeof(GLdepth),
                     &d32,
                     FBS_WRITE);
        }
        break;
      }
    }
  }
}


void fxDDReadDepthPixels(GLcontext *ctx, GLuint n,
                         const GLint x[], const GLint y[], GLdepth depth[])
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
  GLuint i;

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
    fprintf(stderr, "fxmesa: fxDDReadDepthPixels(...)\n");
  }

  for (i = 0; i < n; i++) {
    int xpos = x[i] + fxMesa->x_offset;
    int ypos = bottom - y[i];
    GLushort d16;
    GLuint d32;
    GLuint depth_size = fxMesa->glVis->DepthBits;
    GLuint stencil_size = fxMesa->glVis->StencilBits;
    
    assert((depth_size == 16) || (depth_size == 24) || (depth_size == 32));
    switch (depth_size) {
    case 16:
      FX_grLfbReadRegion(GR_BUFFER_AUXBUFFER, xpos, ypos, 1, 1, 0, &d16);
      depth[i] = d16;
      break;
    case 24:
    case 32:
      rw_fb_span(fxMesa,
		 GR_BUFFER_AUXBUFFER,
		 xpos, ypos,
		 1,
		 depth_size/8,
		 &d32,
		 FBS_READ);
      /*
       * Get rid of the stencil bits.  Should I sign
       * extend this?  I don't believe so, since GLdepth
       * is unsigned.
       */
      if (stencil_size > 0) {
	d32 &= 0xFFFFFF;
      }
      depth[i] = d32;
      break;
    default:
      assert(0);
    }
  }
}


/*
 * Stencil buffer read/write functions.
 */

#define DEPTH_VALUE_TO_STENCIL_VALUE(d)         (((d) >> 24) & 0xFF)
#define STENCIL_VALUE_TO_DEPTH_VALUE(s,d) \
        ((((s) & 0xFF) << 24) | ((d) & 0xFFFFFF))
#define ASSEMBLE_STENCIL_VALUE(os, ns, mask) \
        (((os) &~ (mask)) | ((ns) & (mask)))

#if 00
/*
 * Read a horizontal span of stencil values from the stencil buffer.
 */
void
fxDDReadStencilSpan( GLcontext *ctx,
                     GLuint n,
                     GLint x,
                     GLint y,
                     GLstencil stencil[] )
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
  GLuint i;
  int xpos = x + fxMesa->x_offset;
  int ypos = bottom - y;

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
    fprintf(stderr, "fxmesa: fxDDReadStencilSpan(...)\n");
  }

  for (i = 0; i < n; i++, ypos += 1) {
    GLuint d;
    rw_fb_span(fxMesa,
	       GR_BUFFER_AUXBUFFER,
	       xpos, ypos,
	       1,
	       sizeof(GLdepth),
	       &d,
	       FBS_READ);
    stencil[i] = DEPTH_VALUE_TO_STENCIL_VALUE(d);
  }
}

/*
 * Write a horizontal span of stencil values into the stencil buffer.
 * If mask is NULL, write all stencil values.
 * Else, only write stencil[i] if mask[i] is non-zero.
 */
void
fxDDWriteStencilSpan( GLcontext *ctx,
                      GLuint n,
                      GLint x,
                      GLint y,
                      const GLstencil stencil[],
                      const GLubyte mask[] )
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
  GLuint i;
  int xpos = x + fxMesa->x_offset;
  int ypos = bottom - y;

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
    fprintf(stderr, "fxmesa: fxDDWriteStencilSpan(...)\n");
  }

  for (i = 0; i < n; i++, ypos += 1) {
    GLdepth d;
    GLstencil ns;
    rw_fb_span(fxMesa,
	       GR_BUFFER_AUXBUFFER,
	       xpos, ypos,
	       1,
	       sizeof(GLdepth),
	       &d,
	       FBS_READ);
   /*
    * Find the old stencil value, and strip off the bits
    * we are going to write.
    */
    ns = ASSEMBLE_STENCIL_VALUE(DEPTH_VALUE_TO_STENCIL_VALUE(d),
                                stencil[i],
                                mask[i]);
   /*
    * Now, assemble the new StenDepth value, with the old
    * depth value, the old stencil value in the bits
    * where mask is 0, and the new stencil value in the bits
    * where mask is 1.
    */
    d = STENCIL_VALUE_TO_DEPTH_VALUE(ns, d);
    rw_fb_span(fxMesa,
	       GR_BUFFER_AUXBUFFER,
	       xpos, ypos,
	       1,
	       sizeof(GLdepth),
	       &d,
	       FBS_WRITE);
  }
}

/* Write an array of stencil values into the stencil buffer.
 * If mask is NULL, write all stencil values.
 * Else, only write stencil[i] if mask[i] is non-zero.
 */
void
fxDDReadStencilPixels( GLcontext *ctx,
                       GLuint n,
                       const GLint x[],
                       const GLint y[],
                       GLstencil stencil[])
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
  GLuint i;

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
    fprintf(stderr, "fxmesa: fxDDReadStencilPixels(...)\n");
  }

  for (i = 0; i < n; i++) {
    int xpos = x[i] + fxMesa->x_offset;
    int ypos = bottom - y[i];
    GLuint d;
    rw_fb_span(fxMesa,
               GR_BUFFER_AUXBUFFER,
	       xpos, ypos,
	       1,
	       sizeof(GLdepth),
	       &d,
	       FBS_READ);
    stencil[i] = DEPTH_VALUE_TO_STENCIL_VALUE(d);
  }
}

/*
 * Read an array of stencil values from the stencil buffer.
 */
void
fxDDWriteStencilPixels( GLcontext *ctx,
                        GLuint n,
                        const GLint x[],
                        const GLint y[],
                        const GLstencil stencil[],
			const GLstencil mask[])
{
  fxMesaContext fxMesa = (fxMesaContext)ctx->DriverCtx;
  GLint bottom = fxMesa->height + fxMesa->y_offset - 1;
  GLuint i;

  if (MESA_VERBOSE & VERBOSE_DRIVER) {
    fprintf(stderr, "fxmesa: fxDDWriteStencilPixels(...)\n");
  }

  for (i = 0; i < n; i++) {
    int xpos = x[i] + fxMesa->x_offset;
    int ypos = bottom - y[i];
    GLuint d;
    rw_fb_span(fxMesa,
               GR_BUFFER_AUXBUFFER,
	       xpos, ypos,
	       1,
	       sizeof(GLdepth),
	       &d,
	       FBS_READ);
    d = STENCIL_VALUE_TO_DEPTH_VALUE(stencil[i], d);
    rw_fb_span(fxMesa,
               GR_BUFFER_AUXBUFFER,
	       xpos, ypos,
	       1,
	       sizeof(GLdepth),
	       &d,
	       FBS_WRITE);
  }
}
#endif /* disable fxddStencil* funcs */


#define EXTRACT_S_FROM_ZS(zs) (((zs) >> 24) & 0xFF)
#define EXTRACT_Z_FROM_ZS(zs) ((zs) & 0xffffff)
#define BUILD_ZS(z, s)  (((s) << 24) | (z))

static void write_stencil_span(GLcontext *ctx, GLuint n, GLint x, GLint y,
                               const GLstencil stencil[],
                               const GLubyte mask[] )
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GrLfbInfo_t info;
  int s;
  void *d;

  BEGIN_BOARD_LOCK();
  info.size = sizeof(info);
  if (grLfbLock(GR_LFB_WRITE_ONLY,
                GR_BUFFER_AUXBUFFER,
                GR_LFBWRITEMODE_Z32,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
    const GLint winX = fxMesa->x_offset;
    const GLint scrX = winX + x;
    const GLint scrY = winY - y;

    GLint dstStride = info.strideInBytes;
    GLubyte *dst = (GLubyte *) info.lfbPtr
                   + (winY - y) * dstStride + (winX + x) * 4;
    GLuint *dst32 = (GLuint *) dst;
    GLubyte visMask[MAX_WIDTH];
    GLuint i;
    generate_vismask(fxMesa, scrX, scrY, n, visMask);
    s = dstStride;
    d = dst32;
    for (i = 0; i < n; i++) {
      if (visMask[i] && (!mask || mask[i])) {
        GLuint zs = dst32[i];
        GLuint z = EXTRACT_Z_FROM_ZS(zs);
        zs = BUILD_ZS(z, stencil[i]);
        dst32[i] = /*zs;*/ stencil[i];
      }
    }
    grLfbUnlock(GR_LFB_WRITE_ONLY, GR_BUFFER_AUXBUFFER);
  }
  else {
    s = -1;
    d = 0;
  }
  END_BOARD_LOCK();
  /*
  printf("write stencil span %d %p\n", s, d);
  printf("info: size=%d lfbPtr=%p stride=%x writeMode=%x origin=%x\n",
    info.size, info.lfbPtr, info.strideInBytes, info.writeMode, info.origin);
  */
}


static void
read_stencil_span(GLcontext *ctx, GLuint n, GLint x, GLint y,
                  GLstencil stencil[])
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);
  GrLfbInfo_t info;
  /*
  int s;
  void *d;
  */

  BEGIN_BOARD_LOCK();
  info.size = sizeof(info);
  if (grLfbLock(GR_LFB_READ_ONLY,
                GR_BUFFER_AUXBUFFER,
                GR_LFBWRITEMODE_Z32,
                GR_ORIGIN_UPPER_LEFT,
                FXFALSE,
                &info)) {
    const GLint winY = fxMesa->y_offset + fxMesa->height - 1;
    const GLint winX = fxMesa->x_offset;
    GLint srcStride = /*info.strideInBytes;*/ 8192 ;  /* XXX a hack! */
    const GLubyte *src = (const GLubyte *) info.lfbPtr
                       + (winY - y) * srcStride + (winX + x) * 4;
    const GLuint *src32 = (const GLuint *) src;
    GLuint i;
    /*
    s = srcStride;
    d = src32;
    */
    for (i = 0; i < n; i++) {
      stencil[i] = EXTRACT_S_FROM_ZS(src32[i]);
    }
    grLfbUnlock(GR_LFB_READ_ONLY, GR_BUFFER_AUXBUFFER);
  }
  END_BOARD_LOCK();
  /*
  printf("read stencil span %d %p\n", s, d);
  printf("info: size=%d lfbPtr=%p stride=%x writeMode=%x origin=%x\n",
    info.size, info.lfbPtr, info.strideInBytes, info.writeMode, info.origin);
  */
}


static void
write_stencil_pixels( GLcontext *ctx, GLuint n,
                      const GLint x[], const GLint y[],
                      const GLstencil stencil[], const GLubyte mask[])
{
  /* XXX optimize this */
  GLuint i;
  for (i = 0; i < n; i++) {
    if (mask[i]) {
      write_stencil_span(ctx, 1, x[i], y[i], stencil + i, mask + i);
    }
  }
}


static void
read_stencil_pixels(GLcontext *ctx, GLuint n, const GLint x[], const GLint y[],
                    GLstencil stencil[])
{
  /* XXX optimize this */
  GLuint i;
  for (i = 0; i < n; i++) {
    read_stencil_span(ctx, 1, x[i], y[i], stencil + i);
  }
}



void fxSetupDDSpanPointers(GLcontext *ctx)
{
  fxMesaContext fxMesa = FX_CONTEXT(ctx);

  if (ctx->Visual->RedBits   == 5 &&
      ctx->Visual->GreenBits == 6 &&
      ctx->Visual->BlueBits  == 5 &&
      ctx->Visual->AlphaBits == 0) {
    /* 16bpp mode */
    ctx->Driver.WriteRGBASpan       = write_R5G6B5_rgba_span;
    ctx->Driver.WriteRGBSpan        = write_R5G6B5_rgb_span;
    ctx->Driver.WriteMonoRGBASpan   = write_R5G6B5_mono_span;
    ctx->Driver.WriteRGBAPixels     = write_R5G6B5_pixels;
    ctx->Driver.WriteMonoRGBAPixels = write_R5G6B5_mono_pixels;
    ctx->Driver.ReadRGBASpan        = read_R5G6B5_span;
    ctx->Driver.ReadRGBAPixels      = read_R5G6B5_pixels;
  }
  else if (ctx->Visual->RedBits   == 8 &&
           ctx->Visual->GreenBits == 8 &&
           ctx->Visual->BlueBits  == 8 &&
           ctx->Visual->AlphaBits == 0) {
    /* 24bpp mode */
    ctx->Driver.WriteRGBASpan       = write_R8G8B8_rgba_span;
    ctx->Driver.WriteRGBSpan        = write_R8G8B8_rgb_span;
    ctx->Driver.WriteMonoRGBASpan   = write_R8G8B8_mono_span;
    ctx->Driver.WriteRGBAPixels     = write_R8G8B8_pixels;
    ctx->Driver.WriteMonoRGBAPixels = write_R8G8B8_mono_pixels;
    ctx->Driver.ReadRGBASpan        = read_R8G8B8_span;
    ctx->Driver.ReadRGBAPixels      = read_R8G8B8_pixels;
  }
  else if (ctx->Visual->RedBits   == 8 &&
           ctx->Visual->GreenBits == 8 &&
           ctx->Visual->BlueBits  == 8 &&
           ctx->Visual->AlphaBits == 8) {
    /* 32bpp mode */
    ctx->Driver.WriteRGBASpan       = write_R8G8B8A8_rgba_span;
    ctx->Driver.WriteRGBSpan        = write_R8G8B8A8_rgb_span;
    ctx->Driver.WriteMonoRGBASpan   = write_R8G8B8A8_mono_span;
    ctx->Driver.WriteRGBAPixels     = write_R8G8B8A8_pixels;
    ctx->Driver.WriteMonoRGBAPixels = write_R8G8B8A8_mono_pixels;
    ctx->Driver.ReadRGBASpan        = read_R8G8B8A8_span;
    ctx->Driver.ReadRGBAPixels      = read_R8G8B8A8_pixels;
  }
  else {
    abort();
  }

  if (fxMesa->haveHwStencil) {
    ctx->Driver.WriteStencilSpan    = write_stencil_span;
    ctx->Driver.ReadStencilSpan     = read_stencil_span;
    ctx->Driver.WriteStencilPixels  = write_stencil_pixels;
    ctx->Driver.ReadStencilPixels   = read_stencil_pixels;
  }

  ctx->Driver.WriteCI8Span        = NULL;
  ctx->Driver.WriteCI32Span       = NULL;
  ctx->Driver.WriteMonoCISpan     = NULL;
  ctx->Driver.WriteCI32Pixels     = NULL;
  ctx->Driver.WriteMonoCIPixels   = NULL;
  ctx->Driver.ReadCI32Span        = NULL;
  ctx->Driver.ReadCI32Pixels      = NULL;
}
