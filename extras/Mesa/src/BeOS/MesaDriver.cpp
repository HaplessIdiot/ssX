/* $Id: MesaDriver.cpp,v 1.1 2002/12/11 20:33:29 dawes Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.1
 * 
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
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

#include <GLView.h>

#include "MesaDriver.h"

MesaDriver::MesaDriver()
{
   m_glcontext 		= NULL;
   m_glvisual		= NULL;
   m_glframebuffer 	= NULL;
   m_bglview 		= NULL;
   m_bitmap 		= NULL;

   m_clear_color[BE_RCOMP] = 0;
   m_clear_color[BE_GCOMP] = 0;
   m_clear_color[BE_BCOMP] = 0;
   m_clear_color[BE_ACOMP] = 0;

   m_clear_index = 0;
}


MesaDriver::~MesaDriver()
{
   _mesa_destroy_visual(m_glvisual);
   _mesa_destroy_framebuffer(m_glframebuffer);
   _mesa_destroy_context(m_glcontext);

}


void MesaDriver::Init(BGLView * bglview, GLcontext * ctx, GLvisual * visual, GLframebuffer * framebuffer)
{
	m_bglview 		= bglview;
	m_glcontext 	= ctx;
	m_glvisual 		= visual;
	m_glframebuffer = framebuffer;

	MesaDriver * md = (MesaDriver *) ctx->DriverCtx;
	struct swrast_device_driver * swdd = _swrast_GetDeviceDriverReference( ctx );
	TNLcontext * tnl = TNL_CONTEXT(ctx);

	assert(md->m_glcontext == ctx );

	ctx->Driver.GetString = MesaDriver::GetString;
	ctx->Driver.UpdateState = MesaDriver::UpdateState;
	ctx->Driver.SetDrawBuffer = MesaDriver::SetDrawBuffer;
	ctx->Driver.GetBufferSize = MesaDriver::GetBufferSize;
	ctx->Driver.ResizeBuffers = _swrast_alloc_buffers;

	ctx->Driver.Accum = _swrast_Accum;
	ctx->Driver.Bitmap = _swrast_Bitmap;
	ctx->Driver.ClearIndex = MesaDriver::ClearIndex;
	ctx->Driver.ClearColor = MesaDriver::ClearColor;
	ctx->Driver.Clear = MesaDriver::Clear;
	ctx->Driver.CopyPixels = _swrast_CopyPixels;
   	ctx->Driver.DrawPixels = _swrast_DrawPixels;
   	ctx->Driver.ReadPixels = _swrast_ReadPixels;

   	ctx->Driver.ChooseTextureFormat = _mesa_choose_tex_format;
   	ctx->Driver.TexImage1D = _mesa_store_teximage1d;
   	ctx->Driver.TexImage2D = _mesa_store_teximage2d;
   	ctx->Driver.TexImage3D = _mesa_store_teximage3d;
   	ctx->Driver.TexSubImage1D = _mesa_store_texsubimage1d;
   	ctx->Driver.TexSubImage2D = _mesa_store_texsubimage2d;
   	ctx->Driver.TexSubImage3D = _mesa_store_texsubimage3d;
   	ctx->Driver.TestProxyTexImage = _mesa_test_proxy_teximage;

   	ctx->Driver.CopyTexImage1D = _swrast_copy_teximage1d;
   	ctx->Driver.CopyTexImage2D = _swrast_copy_teximage2d;
   	ctx->Driver.CopyTexSubImage1D = _swrast_copy_texsubimage1d;
   	ctx->Driver.CopyTexSubImage2D = _swrast_copy_texsubimage2d;
   	ctx->Driver.CopyTexSubImage3D = _swrast_copy_texsubimage3d;
   	ctx->Driver.CopyColorTable = _swrast_CopyColorTable;
   	ctx->Driver.CopyColorSubTable = _swrast_CopyColorSubTable;
   	ctx->Driver.CopyConvolutionFilter1D = _swrast_CopyConvolutionFilter1D;
   	ctx->Driver.CopyConvolutionFilter2D = _swrast_CopyConvolutionFilter2D;

   	ctx->Driver.BaseCompressedTexFormat = _mesa_base_compressed_texformat;
   	ctx->Driver.CompressedTextureSize = _mesa_compressed_texture_size;
   	ctx->Driver.GetCompressedTexImage = _mesa_get_compressed_teximage;

	swdd->SetReadBuffer = MesaDriver::SetReadBuffer;

	tnl->Driver.RunPipeline = _tnl_run_pipeline;

	_swsetup_Wakeup(ctx);
}


void MesaDriver::LockGL()
{
	m_bglview->LockLooper();

   UpdateState(m_glcontext, 0);
   _mesa_make_current(m_glcontext, m_glframebuffer);
}


void MesaDriver::UnlockGL()
{
	m_bglview->UnlockLooper();
   // Could call _mesa_make_current(NULL, NULL) but it would just
   // hinder performance
}


void MesaDriver::SwapBuffers() const
{
	// _mesa_swap_buffers();

	if (m_bitmap) {
		m_bglview->LockLooper();
		m_bglview->DrawBitmap(m_bitmap, BPoint(0, 0));
		m_bglview->UnlockLooper();
	};
}


void MesaDriver::CopySubBuffer(GLint x, GLint y, GLuint width, GLuint height) const
{
   if (m_bitmap) {
      // Source bitmap and view's bitmap are same size.
      // Source and dest rectangle are the same.
      // Note (x,y) = (0,0) is the lower-left corner, have to flip Y
      BRect srcAndDest;
      srcAndDest.left = x;
      srcAndDest.right = x + width - 1;
      srcAndDest.bottom = m_bottom - y;
      srcAndDest.top = srcAndDest.bottom - height + 1;
      m_bglview->DrawBitmap(m_bitmap, srcAndDest, srcAndDest);
   }
}


void MesaDriver::Draw(BRect updateRect) const
{
   if (m_bitmap)
      m_bglview->DrawBitmap(m_bitmap, updateRect, updateRect);
}


void MesaDriver::UpdateState( GLcontext *ctx, GLuint new_state )
{
	struct swrast_device_driver *	swdd = _swrast_GetDeviceDriverReference( ctx );

   _swrast_InvalidateState( ctx, new_state );
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );


	if (ctx->Color.DrawBuffer == GL_FRONT) {
      /* read/write front buffer */
      swdd->WriteRGBASpan = MesaDriver::WriteRGBASpanFront;
      swdd->WriteRGBSpan = MesaDriver::WriteRGBSpanFront;
      swdd->WriteRGBAPixels = MesaDriver::WriteRGBAPixelsFront;
      swdd->WriteMonoRGBASpan = MesaDriver::WriteMonoRGBASpanFront;
      swdd->WriteMonoRGBAPixels = MesaDriver::WriteMonoRGBAPixelsFront;
      swdd->WriteCI32Span = MesaDriver::WriteCI32SpanFront;
      swdd->WriteCI8Span = MesaDriver::WriteCI8SpanFront;
      swdd->WriteMonoCISpan = MesaDriver::WriteMonoCISpanFront;
      swdd->WriteCI32Pixels = MesaDriver::WriteCI32PixelsFront;
      swdd->WriteMonoCIPixels = MesaDriver::WriteMonoCIPixelsFront;
      swdd->ReadRGBASpan = MesaDriver::ReadRGBASpanFront;
      swdd->ReadRGBAPixels = MesaDriver::ReadRGBAPixelsFront;
      swdd->ReadCI32Span = MesaDriver::ReadCI32SpanFront;
      swdd->ReadCI32Pixels = MesaDriver::ReadCI32PixelsFront;
   }
   else {
      /* read/write back buffer */
      swdd->WriteRGBASpan = MesaDriver::WriteRGBASpanBack;
      swdd->WriteRGBSpan = MesaDriver::WriteRGBSpanBack;
      swdd->WriteRGBAPixels = MesaDriver::WriteRGBAPixelsBack;
      swdd->WriteMonoRGBASpan = MesaDriver::WriteMonoRGBASpanBack;
      swdd->WriteMonoRGBAPixels = MesaDriver::WriteMonoRGBAPixelsBack;
      swdd->WriteCI32Span = MesaDriver::WriteCI32SpanBack;
      swdd->WriteCI8Span = MesaDriver::WriteCI8SpanBack;
      swdd->WriteMonoCISpan = MesaDriver::WriteMonoCISpanBack;
      swdd->WriteCI32Pixels = MesaDriver::WriteCI32PixelsBack;
      swdd->WriteMonoCIPixels = MesaDriver::WriteMonoCIPixelsBack;
      swdd->ReadRGBASpan = MesaDriver::ReadRGBASpanBack;
      swdd->ReadRGBAPixels = MesaDriver::ReadRGBAPixelsBack;
      swdd->ReadCI32Span = MesaDriver::ReadCI32SpanBack;
      swdd->ReadCI32Pixels = MesaDriver::ReadCI32PixelsBack;
    }
}


void MesaDriver::ClearIndex(GLcontext *ctx, GLuint index)
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   md->m_clear_index = index;
}


void MesaDriver::ClearColor(GLcontext *ctx, const GLchan color[4])
						// GLubyte r, GLubyte g,
                        // GLubyte b, GLubyte a)
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   md->m_clear_color[BE_RCOMP] = color[0];
   md->m_clear_color[BE_GCOMP] = color[1];
   md->m_clear_color[BE_BCOMP] = color[2];
   md->m_clear_color[BE_ACOMP] = color[3];
   assert(md->m_bglview);
}


void MesaDriver::Clear(GLcontext *ctx, GLbitfield mask,
                               GLboolean all, GLint x, GLint y,
                               GLint width, GLint height)
{
   if (mask & DD_FRONT_LEFT_BIT)
		ClearFront(ctx, all, x, y, width, height);
   if (mask & DD_BACK_LEFT_BIT)
		ClearBack(ctx, all, x, y, width, height);

	mask &= ~(DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT);
	if (mask)
		_swrast_Clear( ctx, mask, all, x, y, width, height );

   return;
}


void MesaDriver::ClearFront(GLcontext *ctx,
                         GLboolean all, GLint x, GLint y,
                         GLint width, GLint height)
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);

   bglview->SetHighColor(md->m_clear_color[BE_RCOMP],
                         md->m_clear_color[BE_GCOMP],
                         md->m_clear_color[BE_BCOMP],
                         md->m_clear_color[BE_ACOMP]);
   bglview->SetLowColor(md->m_clear_color[BE_RCOMP],
                        md->m_clear_color[BE_GCOMP],
                        md->m_clear_color[BE_BCOMP],
                        md->m_clear_color[BE_ACOMP]);
   if (all) {
      BRect b = bglview->Bounds();
      bglview->FillRect(b);
   }
   else {
      // XXX untested
      BRect b;
      b.left = x;
      b.right = x + width;
      b.bottom = md->m_height - y - 1;
      b.top = b.bottom - height;
      bglview->FillRect(b);
   }

   // restore drawing color
#if 0
   bglview->SetHighColor(md->mColor[BE_RCOMP],
                         md->mColor[BE_GCOMP],
                         md->mColor[BE_BCOMP],
                         md->mColor[BE_ACOMP]);
   bglview->SetLowColor(md->mColor[BE_RCOMP],
                        md->mColor[BE_GCOMP],
                        md->mColor[BE_BCOMP],
                        md->mColor[BE_ACOMP]);
#endif
}


void MesaDriver::ClearBack(GLcontext *ctx,
                        GLboolean all, GLint x, GLint y,
                        GLint width, GLint height)
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);
   BBitmap *bitmap = md->m_bitmap;
   assert(bitmap);
   GLuint *start = (GLuint *) bitmap->Bits();
   const GLuint *clearPixelPtr = (const GLuint *) md->m_clear_color;
   const GLuint clearPixel = *clearPixelPtr;

   if (all) {
      const int numPixels = md->m_width * md->m_height;
      if (clearPixel == 0) {
         memset(start, 0, numPixels * 4);
      }
      else {
         for (int i = 0; i < numPixels; i++) {
             start[i] = clearPixel;
         }
      }
   }
   else {
      // XXX untested
      start += y * md->m_width + x;
      for (int i = 0; i < height; i++) {
         for (int j = 0; j < width; j++) {
            start[j] = clearPixel;
         }
         start += md->m_width;
      }
   }
}


void MesaDriver::SetDrawBuffer(GLcontext *ctx, GLenum buffer)
{
	/* TODO */
}

void MesaDriver::SetReadBuffer(GLcontext *ctx, GLframebuffer *colorBuffer,
                            GLenum buffer)
{
   /* TODO */
}

void MesaDriver::GetBufferSize(GLframebuffer * framebuffer, GLuint *width,
                            GLuint *height)
{
   GET_CURRENT_CONTEXT(ctx);
   if (!ctx)
		return;

   MesaDriver * md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);

   BRect b = bglview->Bounds();
   *width = (GLuint) b.IntegerWidth() + 1; // (b.right - b.left + 1);
   *height = (GLuint) b.IntegerHeight() + 1; // (b.bottom - b.top + 1);
   md->m_bottom = (GLint) b.bottom;

   if (ctx->Visual.doubleBufferMode) {
      if (*width != md->m_width || *height != md->m_height) {
         // allocate new size of back buffer bitmap
         if (md->m_bitmap)
            delete md->m_bitmap;
         BRect rect(0.0, 0.0, *width - 1, *height - 1);
         md->m_bitmap = new BBitmap(rect, B_RGBA32);
      }
   }
   else
   {
      md->m_bitmap = NULL;
   }

   md->m_width = *width;
   md->m_height = *height;
}


const GLubyte *MesaDriver::GetString(GLcontext *ctx, GLenum name)
{
   switch (name) {
      case GL_RENDERER:
         return (const GLubyte *) "BGLView";
      default:
         // Let core library handle all other cases
         return NULL;
   }
}


// Plot a pixel.  (0,0) is upper-left corner
// This is only used when drawing to the front buffer.
static void Plot(BGLView *bglview, int x, int y)
{
   // XXX There's got to be a better way!
   BPoint p(x, y), q(x+1, y);
   bglview->StrokeLine(p, q);
}


void MesaDriver::WriteRGBASpanFront(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][4],
                                 const GLubyte mask[])
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);
   int flippedY = md->m_bottom - y;
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2], rgba[i][3]);
            Plot(bglview, x++, flippedY);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2], rgba[i][3]);
         Plot(bglview, x++, flippedY);
      }
   }
}

void MesaDriver::WriteRGBSpanFront(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                CONST GLubyte rgba[][3],
                                const GLubyte mask[])
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);
   int flippedY = md->m_bottom - y;
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
            Plot(bglview, x++, flippedY);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
         Plot(bglview, x++, flippedY);
      }
   }
}

void MesaDriver::WriteMonoRGBASpanFront(const GLcontext *ctx, GLuint n,
                                     GLint x, GLint y,
                                     const GLchan color[4],
                                     const GLubyte mask[])
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);
   int flippedY = md->m_bottom - y;
   bglview->SetHighColor(color[RCOMP], color[GCOMP], color[BCOMP]);
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            Plot(bglview, x++, flippedY);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         Plot(bglview, x++, flippedY);
      }
   }
}

void MesaDriver::WriteRGBAPixelsFront(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] )
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
            Plot(bglview, x[i], md->m_bottom - y[i]);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         bglview->SetHighColor(rgba[i][0], rgba[i][1], rgba[i][2]);
         Plot(bglview, x[i], md->m_bottom - y[i]);
      }
   }
}


void MesaDriver::WriteMonoRGBAPixelsFront(const GLcontext *ctx, GLuint n,
                                       const GLint x[], const GLint y[],
                                       const GLchan color[4],
                                       const GLubyte mask[])
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BGLView *bglview = md->m_bglview;
   assert(bglview);
   // plot points using current color
   bglview->SetHighColor(color[RCOMP], color[GCOMP], color[BCOMP]);
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            Plot(bglview, x[i], md->m_bottom - y[i]);
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         Plot(bglview, x[i], md->m_bottom - y[i]);
      }
   }
}


void MesaDriver::WriteCI32SpanFront( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                             const GLuint index[], const GLubyte mask[] )
{
   // XXX to do
}

void MesaDriver::WriteCI8SpanFront( const GLcontext *ctx, GLuint n, GLint x, GLint y,
                            const GLubyte index[], const GLubyte mask[] )
{
   // XXX to do
}

void MesaDriver::WriteMonoCISpanFront( const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    GLuint colorIndex, const GLubyte mask[] )
{
   // XXX to do
}


void MesaDriver::WriteCI32PixelsFront( const GLcontext *ctx, GLuint n,
                                    const GLint x[], const GLint y[],
                                    const GLuint index[], const GLubyte mask[] )
{
   // XXX to do
}

void MesaDriver::WriteMonoCIPixelsFront( const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      GLuint colorIndex, const GLubyte mask[] )
{
   // XXX to do
}


void MesaDriver::ReadCI32SpanFront( const GLcontext *ctx,
                                 GLuint n, GLint x, GLint y, GLuint index[] )
{
 	printf("ReadCI32SpanFront() not implemented yet!\n");
  // XXX to do
}


void MesaDriver::ReadRGBASpanFront( const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y, GLubyte rgba[][4] )
{
 	printf("ReadRGBASpanFront() not implemented yet!\n");
   // XXX to do
}


void MesaDriver::ReadCI32PixelsFront( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[] )
{
 	printf("ReadCI32PixelsFront() not implemented yet!\n");
   // XXX to do
}


void MesaDriver::ReadRGBAPixelsFront( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLubyte rgba[][4], const GLubyte mask[] )
{
 	printf("ReadRGBAPixelsFront() not implemented yet!\n");
   // XXX to do
}




void MesaDriver::WriteRGBASpanBack(const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 CONST GLubyte rgba[][4],
                                 const GLubyte mask[])
{
	MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
	BBitmap *bitmap = md->m_bitmap;

	static bool already_called = false;
	if (! already_called) {
		printf("WriteRGBASpanBack() called.\n");
		already_called = true;
	}

	assert(bitmap);

	int row = md->m_bottom - y;
	uint8 * ptr = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + x * 4;
 	uint32 * pixel = (uint32 *) ptr;
	
	if (mask) {
		while(n--) {
			if (*mask++)
				*pixel = PACK_B_RGBA32(rgba[0]);
			pixel++;
			rgba++;
		};
	} else {
		while(n--) {
			*pixel++ = PACK_B_RGBA32(rgba[0]);
			rgba++;
		};
	};
 }


void MesaDriver::WriteRGBSpanBack(const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                CONST GLubyte rgb[][3],
                                const GLubyte mask[])
{
	MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
	BBitmap *bitmap = md->m_bitmap;

	static bool already_called = false;
	if (! already_called) {
		printf("WriteRGBSpanBack() called.\n");
		already_called = true;
	}

	assert(bitmap);

	int row = md->m_bottom - y;
	uint8 * ptr = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + x * 4;
 	uint32 * pixel = (uint32 *) ptr;
	
	if (mask) {
		while(n--) {
			if (*mask++)
				*pixel = PACK_B_RGB32(rgb[0]);
			pixel++;
			rgb++;
		};
	} else {
		while(n--) {
			*pixel++ = PACK_B_RGB32(rgb[0]);
			rgb++;
		};
	};
}




void MesaDriver::WriteMonoRGBASpanBack(const GLcontext *ctx, GLuint n,
                                    GLint x, GLint y,
                                    const GLchan color[4], const GLubyte mask[])
{
	MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
	BBitmap *bitmap = md->m_bitmap;

	static bool already_called = false;
	if (! already_called) {
		printf("WriteMonoRGBASpanBack() called.\n");
		already_called = true;
	}

	assert(bitmap);

	int row = md->m_bottom - y;
	uint8 * ptr = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + x * 4;
 	uint32 * pixel = (uint32 *) ptr;
	uint32 pixel_color = PACK_B_RGBA32(color);
	
	if (mask) {
		while(n--) {
			if (*mask++)
				*pixel = pixel_color;
			pixel++;
		};
	} else {
		while(n--) {
			*pixel++ = pixel_color;
		};
	};
}


void MesaDriver::WriteRGBAPixelsBack(const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   CONST GLubyte rgba[][4],
                                   const GLubyte mask[] )
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   BBitmap *bitmap = md->m_bitmap;

	static bool already_called = false;
	if (! already_called) {
		printf("WriteRGBAPixelsBack() called.\n");
		already_called = true;
	}

	assert(bitmap);
#if 0
	while(n--) {
		if (*mask++) {
			int row = md->m_bottom - *y;
			uint8 * pixel = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + *x * 4;
			*((uint32 *) pixel) = PACK_B_RGBA32(rgba[0]);
		};
		x++;
		y++;
		rgba++;
	};
#else
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            GLubyte *pixel = (GLubyte *) bitmap->Bits()
            + ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
            pixel[BE_RCOMP] = rgba[i][RCOMP];
            pixel[BE_GCOMP] = rgba[i][GCOMP];
            pixel[BE_BCOMP] = rgba[i][BCOMP];
            pixel[BE_ACOMP] = rgba[i][ACOMP];
         }
      }
   }
   else {
      for (GLuint i = 0; i < n; i++) {
         GLubyte *pixel = (GLubyte *) bitmap->Bits()
            + ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
         pixel[BE_RCOMP] = rgba[i][RCOMP];
         pixel[BE_GCOMP] = rgba[i][GCOMP];
         pixel[BE_BCOMP] = rgba[i][BCOMP];
         pixel[BE_ACOMP] = rgba[i][ACOMP];
      }
   }
#endif
}


void MesaDriver::WriteMonoRGBAPixelsBack(const GLcontext *ctx, GLuint n,
                                      const GLint x[], const GLint y[],
                                      const GLchan color[4],
                                      const GLubyte mask[])
{
	MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
	BBitmap *bitmap = md->m_bitmap;

	static bool already_called = false;
	if (! already_called) {
		printf("WriteMonoRGBAPixelsBack() called.\n");
		already_called = true;
	}

	assert(bitmap);

	uint32 pixel_color = PACK_B_RGBA32(color);
#if 0	
	while(n--) {
		if (*mask++) {
			int row = md->m_bottom - *y;
			uint8 * pixel = (uint8 *) bitmap->Bits() + (row * bitmap->BytesPerRow()) + *x * 4;

			*((uint32 *) pixel) = pixel_color;
		};
		x++;
		y++;
	};
#else
   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
         	GLubyte * ptr = (GLubyte *) bitmap->Bits()
            	+ ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
            *((uint32 *) ptr) = pixel_color;
         }
      }
   }
   else {
	  for (GLuint i = 0; i < n; i++) {
       	GLubyte * ptr = (GLubyte *) bitmap->Bits()
	           	+ ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
       *((uint32 *) ptr) = pixel_color;
      }
   }
#endif
}


void MesaDriver::WriteCI32SpanBack( const GLcontext *ctx, GLuint n,
                                 GLint x, GLint y,
                                 const GLuint index[], const GLubyte mask[] )
{
   // XXX to do
}

void MesaDriver::WriteCI8SpanBack( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y,
                                const GLubyte index[], const GLubyte mask[] )
{
   // XXX to do
}

void MesaDriver::WriteMonoCISpanBack( const GLcontext *ctx, GLuint n,
                                   GLint x, GLint y,
                                   GLuint colorIndex, const GLubyte mask[] )
{
   // XXX to do
}


void MesaDriver::WriteCI32PixelsBack( const GLcontext *ctx, GLuint n,
                                   const GLint x[], const GLint y[],
                                   const GLuint index[], const GLubyte mask[] )
{
   // XXX to do
}

void MesaDriver::WriteMonoCIPixelsBack( const GLcontext *ctx, GLuint n,
                                     const GLint x[], const GLint y[],
                                     GLuint colorIndex, const GLubyte mask[] )
{
   // XXX to do
}


void MesaDriver::ReadCI32SpanBack( const GLcontext *ctx,
                                GLuint n, GLint x, GLint y, GLuint index[] )
{
   // XXX to do
}


void MesaDriver::ReadRGBASpanBack( const GLcontext *ctx, GLuint n,
                                GLint x, GLint y, GLubyte rgba[][4] )
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   const BBitmap *bitmap = md->m_bitmap;
   assert(bitmap);
   int row = md->m_bottom - y;
   const GLubyte *pixel = (GLubyte *) bitmap->Bits()
                        + (row * bitmap->BytesPerRow()) + x * 4;

   for (GLuint i = 0; i < n; i++) {
      rgba[i][RCOMP] = pixel[BE_RCOMP];
      rgba[i][GCOMP] = pixel[BE_GCOMP];
      rgba[i][BCOMP] = pixel[BE_BCOMP];
      rgba[i][ACOMP] = pixel[BE_ACOMP];
      pixel += 4;
   }
}


void MesaDriver::ReadCI32PixelsBack( const GLcontext *ctx,
                                   GLuint n, const GLint x[], const GLint y[],
                                   GLuint indx[], const GLubyte mask[] )
{
   // XXX to do
}


void MesaDriver::ReadRGBAPixelsBack( const GLcontext *ctx,
                                  GLuint n, const GLint x[], const GLint y[],
                                  GLubyte rgba[][4], const GLubyte mask[] )
{
   MesaDriver *md = (MesaDriver *) ctx->DriverCtx;
   const BBitmap *bitmap = md->m_bitmap;
   assert(bitmap);

   if (mask) {
      for (GLuint i = 0; i < n; i++) {
         if (mask[i]) {
            GLubyte *pixel = (GLubyte *) bitmap->Bits()
            + ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
	         rgba[i][RCOMP] = pixel[BE_RCOMP];
    	     rgba[i][GCOMP] = pixel[BE_GCOMP];
        	 rgba[i][BCOMP] = pixel[BE_BCOMP];
        	 rgba[i][ACOMP] = pixel[BE_ACOMP];
         };
      };
   } else {
      for (GLuint i = 0; i < n; i++) {
         GLubyte *pixel = (GLubyte *) bitmap->Bits()
            + ((md->m_bottom - y[i]) * bitmap->BytesPerRow()) + x[i] * 4;
         rgba[i][RCOMP] = pixel[BE_RCOMP];
         rgba[i][GCOMP] = pixel[BE_GCOMP];
         rgba[i][BCOMP] = pixel[BE_BCOMP];
         rgba[i][ACOMP] = pixel[BE_ACOMP];
      };
   };
}
