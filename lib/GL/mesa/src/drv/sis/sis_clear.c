/**************************************************************************

Copyright 2000 Silicon Integrated Systems Corp, Inc., HsinChu, Taiwan.
Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, PRECISION INSIGHT AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/
/* $XFree86: xc/lib/GL/mesa/src/drv/sis/sis_clear.c,v 1.6tsi Exp $ */

/*
 * Authors:
 *   Sung-Ching Lin <sclin@sis.com.tw>
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#include "sis_context.h"
#include "sis_state.h"
#include "sis_lock.h"

#include "swrast/swrast.h"
#include "mmath.h"

#if 0
static GLbitfield sis_3D_Clear( GLcontext * ctx, GLbitfield mask,
				GLint x, GLint y, GLint width,
				GLint height );
#endif
static void sis_clear_color_buffer( GLcontext *ctx, GLenum mask, GLint x,
				    GLint y, GLint width, GLint height );
static void sis_clear_z_stencil_buffer( GLcontext * ctx,
					GLbitfield mask, GLint x,
					GLint y, GLint width,
					GLint height );

static void
set_color_pattern( sisContextPtr smesa, GLubyte red, GLubyte green,
		   GLubyte blue, GLubyte alpha )
{
   /* XXX only RGB565 and ARGB8888 */
   switch (GET_ColorFormat(smesa))
   {
   case DST_FORMAT_ARGB_8888:
      smesa->clearColorPattern = (alpha << 24) +
	 (red << 16) + (green << 8) + (blue);
      break;
   case DST_FORMAT_RGB_565:
      smesa->clearColorPattern = ((red >> 3) << 11) +
	 ((green >> 2) << 5) + (blue >> 3);
      smesa->clearColorPattern |= smesa->clearColorPattern << 16;
      break;
   default:
      assert(0);
   }
}

void
sisUpdateZStencilPattern( sisContextPtr smesa, GLclampd z, GLint stencil )
{
   GLuint zPattern;

   if (z <= 0.0f)
      zPattern = 0x0;
   else if (z >= 1.0f)
      zPattern = 0xFFFFFFFF;
   else
      zPattern = doFPtoFixedNoRound( z, 32 );

   switch (smesa->zFormat)
   {
   case SiS_ZFORMAT_Z16:
      zPattern = zPattern >> 16;
      zPattern |= zPattern << 16;
      break;
   case SiS_ZFORMAT_S8Z24:
      zPattern = zPattern >> 8;
      zPattern |= stencil << 24;
      break;
   case SiS_ZFORMAT_Z32:
      break;
   default:
      assert(0);
   }
   smesa->clearZStencilPattern = zPattern;
}

void
sisDDClear( GLcontext * ctx, GLbitfield mask, GLboolean all,
	    GLint x, GLint y, GLint width, GLint height )
{
  sisContextPtr smesa = SIS_CONTEXT(ctx);

  GLint x1, y1, width1, height1;

   if (all) {
      GLframebuffer *buffer = ctx->DrawBuffer;

      x1 = 0;
      y1 = 0;
      width1 = buffer->Width;
      height1 = buffer->Height;
   } else {
      x1 = x;
      y1 = Y_FLIP(y+height-1);
      width1 = width;            
      height1 = height;
   }

   LOCK_HARDWARE();

   /* The 3D_Clear code is broken and I've failed to fix it so far. */
#if 0
   if (( ctx->Visual.stencilBits &&
       ((mask | GL_DEPTH_BUFFER_BIT) ^ (mask | GL_STENCIL_BUFFER_BIT)) ) ||
       (*(GLint *)(ctx->Color.ColorMask) != 0xffffffff) )
   {
      /* only Clear either depth or stencil buffer */ 
      mask = sis_3D_Clear( ctx, mask, x1, y1, width1, height1 );
   }
#endif

   if ( mask & DD_FRONT_LEFT_BIT || mask & DD_BACK_LEFT_BIT) {
      sis_clear_color_buffer( ctx, mask, x1, y1, width1, height1 );
      mask &= ~(DD_FRONT_LEFT_BIT | DD_BACK_LEFT_BIT);
   }

   if (mask & (DD_DEPTH_BIT | DD_STENCIL_BIT)) {
      if (smesa->depthbuffer != NULL)
         sis_clear_z_stencil_buffer( ctx, mask, x1, y1, width1, height1 );
      mask &= ~(DD_DEPTH_BIT | DD_STENCIL_BIT);
   }

   UNLOCK_HARDWARE();

   if (mask != 0)
      _swrast_Clear( ctx, mask, all, x1, y1, width, height );
}


void
sisDDClearColor( GLcontext * ctx, const GLfloat color[4] )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);
   GLubyte c[4];

   CLAMPED_FLOAT_TO_UBYTE(c[0], color[0]);
   CLAMPED_FLOAT_TO_UBYTE(c[1], color[1]);
   CLAMPED_FLOAT_TO_UBYTE(c[2], color[2]);
   CLAMPED_FLOAT_TO_UBYTE(c[3], color[3]);

   set_color_pattern( smesa, c[0], c[1], c[2], c[3] );
}

void
sisDDClearDepth( GLcontext * ctx, GLclampd d )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   sisUpdateZStencilPattern( smesa, d, ctx->Stencil.Clear );
}

void
sisDDClearStencil( GLcontext * ctx, GLint s )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   sisUpdateZStencilPattern( smesa, ctx->Depth.Clear, s );
}

#define MAKE_CLEAR_COLOR_8888(cc)   \
            ( (((GLint)(((GLubyte *)(cc))[3] * 255.0 + 0.5))<<24) | \
              (((GLint)(((GLubyte *)(cc))[0] * 255.0 + 0.5))<<16) | \
              (((GLint)(((GLubyte *)(cc))[1] * 255.0 + 0.5))<<8) | \
               ((GLint)(((GLubyte *)(cc))[2] * 255.0 + 0.5)) )

#if 0
static GLbitfield
sis_3D_Clear( GLcontext * ctx, GLbitfield mask,
	      GLint x, GLint y, GLint width, GLint height )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   __GLSiSHardware *current = &smesa->current;

#define RESETSTATE (GFLAG_ENABLESETTING | GFLAG_ENABLESETTING2 | \
                    GFLAG_ZSETTING | GFLAG_DESTSETTING | \
                    GFLAG_STENCILSETTING | GFLAG_CLIPPING)
#define STEN_OP (SiS_SFAIL_REPLACE | SiS_SPASS_ZFAIL_REPLACE | \
                 SiS_SPASS_ZPASS_REPLACE)

   float left, top, right, bottom, zClearVal;
   GLint dwColor=0;
   GLint bClrColor, bClrDepth, bClrStencil;
   GLint dwPrimitiveSet;
   GLint dwEnable1, dwEnable2 = 0, dwDepthMask=0, dwSten1=0, dwSten2=0;

   int count;
   XF86DRIClipRectPtr pExtents;

   bClrColor = 0;
   bClrDepth = (mask & DD_DEPTH_BIT) && 
               (ctx->Visual.depthBits);
   bClrStencil = (mask & DD_STENCIL_BIT) && 
                 (ctx->Visual.stencilBits);

  /* update HW state */
  /* TODO: if enclosing sis_Clear by sis_RenderStart and sis_RenderEnd is
   * uniform, but it seems needless to do so
   */
   if (smesa->GlobalFlag)
      sis_update_render_state( smesa );

   if (bClrColor)
      dwColor = MAKE_CLEAR_COLOR_8888( ctx->Color.ClearColor );
   else
      dwEnable2 |= MASK_ColorMaskWriteEnable;

   if (bClrDepth && bClrStencil) {
      GLint wmask, smask;
      GLstencil sten;

      zClearVal = ctx->Depth.Clear;
      sten = ctx->Stencil.Clear;
      wmask = ctx->Stencil.WriteMask[0];
      smask = 0xff;

      dwEnable1 = MASK_ZWriteEnable | MASK_StencilTestEnable;
      dwEnable2 |= MASK_ZMaskWriteEnable;
      dwDepthMask = ((wmask << 24) | 0x00ffffff);
      dwSten1 = STENCIL_FORMAT_8 | (((GLint) sten << 8) | smask) |
         SiS_STENCIL_ALWAYS;
      dwSten2 = STEN_OP;
   } else if (bClrDepth) {
      zClearVal = ctx->Depth.Clear;
      dwEnable1 = MASK_ZWriteEnable;
      dwEnable2 |= MASK_ZMaskWriteEnable;
      dwDepthMask = 0xffffff;
   } else if (bClrStencil) {
      GLint wmask, smask;
      GLstencil sten;

      sten = (GLstencil)ctx->Stencil.Clear;
      wmask = (GLint)ctx->Stencil.WriteMask[0];
      smask = 0xff;

      zClearVal = 0;
      dwEnable1 = MASK_ZWriteEnable | MASK_StencilTestEnable;
      dwEnable2 |= MASK_ZMaskWriteEnable;
      dwDepthMask = (wmask << 24) & 0xff000000;
      dwSten1 = STENCIL_FORMAT_8 | (((GLint) sten << 8) | smask) |
         SiS_STENCIL_ALWAYS;
      dwSten2 = STEN_OP;
   } else {
      dwEnable2 &= ~MASK_ZMaskWriteEnable;
      dwEnable1 = 0L;
      zClearVal = 1;
   }

   mWait3DCmdQueue (35);
   MMIO(REG_3D_TEnable, dwEnable1);
   MMIO(REG_3D_TEnable2, dwEnable2);
   if (bClrDepth | bClrStencil) {
      MMIO(REG_3D_ZSet, current->hwZ);
      MMIO(REG_3D_ZStWriteMask, dwDepthMask);
      MMIO(REG_3D_ZAddress, current->hwOffsetZ);
   }
   if (bClrColor) {
      MMIO(REG_3D_DstSet, (current->hwDstSet & 0x00ffffff) | 0xc000000);
      MMIO(REG_3D_DstAddress, current->hwOffsetDest);
   } else {
      MMIO(REG_3D_DstAlphaWriteMask, 0L);
   }
   if (bClrStencil) {
      MMIO(REG_3D_StencilSet, dwSten1);
      MMIO(REG_3D_StencilSet2, dwSten2);
   }

   if (mask & DD_FRONT_LEFT_BIT) {
      pExtents = smesa->driDrawable->pClipRects;
      count = smesa->driDrawable->numClipRects;
   } else {
      pExtents = NULL;
      count = 1;
   }

   while(count--) {
      left = x;
      right = x + width - 1;
      top = y;
      bottom = y + height - 1;

      if (pExtents != NULL) {
         GLuint x1, y1, x2, y2;

         x1 = pExtents->x1 - smesa->driDrawable->x;
         y1 = pExtents->y1 - smesa->driDrawable->y;
         x2 = pExtents->x2 - smesa->driDrawable->x - 1;
         y2 = pExtents->y2 - smesa->driDrawable->y - 1;

         left = (left > x1) ? left : x1;
         right = (right > x2) ? x2 : right;
         top = (top > y1) ? top : y1;
         bottom = (bottom > y2) ? y2 : bottom;
         if (left > right || top > bottom)
            continue;
         pExtents++;
      }

      MMIO(REG_3D_ClipTopBottom, ((GLint) top << 13) | (GLint) bottom);
      MMIO(REG_3D_ClipLeftRight, ((GLint) left << 13) | (GLint) right);

      /* the first triangle */
      dwPrimitiveSet = (OP_3D_TRIANGLE_DRAW | OP_3D_FIRE_TSARGBc | 
                        SHADE_FLAT_VertexC);
      MMIO(REG_3D_PrimitiveSet, dwPrimitiveSet);

      MMIO(REG_3D_TSZa, *(GLint *) & zClearVal);
      MMIO(REG_3D_TSXa, *(GLint *) & right);
      MMIO(REG_3D_TSYa, *(GLint *) & top);
      MMIO(REG_3D_TSARGBa, dwColor);

      MMIO(REG_3D_TSZb, *(GLint *) & zClearVal);
      MMIO(REG_3D_TSXb, *(GLint *) & left);
      MMIO(REG_3D_TSYb, *(GLint *) & top);
      MMIO(REG_3D_TSARGBb, dwColor);

      MMIO(REG_3D_TSZc, *(GLint *) & zClearVal);
      MMIO(REG_3D_TSXc, *(GLint *) & left);
      MMIO(REG_3D_TSYc, *(GLint *) & bottom);
      MMIO(REG_3D_TSARGBc, dwColor);

      /* second triangle */
      dwPrimitiveSet = (OP_3D_TRIANGLE_DRAW | OP_3D_FIRE_TSARGBb |
                        SHADE_FLAT_VertexB);
      MMIO(REG_3D_PrimitiveSet, dwPrimitiveSet);

      MMIO(REG_3D_TSZb, *(GLint *) & zClearVal);
      MMIO(REG_3D_TSXb, *(GLint *) & right);
      MMIO(REG_3D_TSYb, *(GLint *) & bottom);
      MMIO(REG_3D_TSARGBb, dwColor);
    }

   mEndPrimitive ();

   smesa->GlobalFlag |= RESETSTATE;

#undef RESETSTATE
#undef STEN_OP
  
   /* 
    * TODO: will mesa call several times if multiple draw buffer
    */
   return (mask & ~(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));
}
#endif

static void
sis_bitblt_clear_cmd( sisContextPtr smesa, ENGPACKET * pkt )
{
   GLint *lpdwDest, *lpdwSrc;
   int i;

   lpdwSrc = (GLint *) pkt + 1;
   lpdwDest = (GLint *) (GET_IOBase (smesa) + REG_SRC_ADDR) + 1;

   mWait3DCmdQueue (10);

   *lpdwDest++ = *lpdwSrc++;
   lpdwSrc++;
   lpdwDest++;
   for (i = 3; i < 8; i++) {
      *lpdwDest++ = *lpdwSrc++;
   }

   MMIO(REG_CMD0, *(GLint *) & pkt->stdwCmd);
   MMIO(REG_QueueLen, -1);
}

static void
sis_clear_color_buffer( GLcontext *ctx, GLenum mask, GLint x, GLint y,
			GLint width, GLint height )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   int count;
   GLuint depth = GET_DEPTH (smesa);
   XF86DRIClipRectPtr pExtents = NULL;
   GLint xx, yy;
   GLint x0, y0, width0, height0;

   ENGPACKET stEngPacket;

   /* Clear back buffer */
   if (mask & DD_BACK_LEFT_BIT) {
      smesa->cbClearPacket.stdwDestPos.wY = y;
      smesa->cbClearPacket.stdwDestPos.wX = x;
      smesa->cbClearPacket.stdwDim.wWidth = (GLshort) width;
      smesa->cbClearPacket.stdwDim.wHeight = (GLshort) height;
      smesa->cbClearPacket.dwFgRopColor = smesa->clearColorPattern;

      sis_bitblt_clear_cmd( smesa, &smesa->cbClearPacket );
   }
  
   if ((mask & DD_FRONT_LEFT_BIT) == 0)
      return;

   /* Clear front buffer */
   x0 = x;
   y0 = y;
   width0 = width;
   height0 = height;

   pExtents = smesa->driDrawable->pClipRects;
   count = smesa->driDrawable->numClipRects;

   memset( &stEngPacket, 0, sizeof (ENGPACKET) );

   stEngPacket.dwSrcPitch = (depth == 2) ? 0x80000000 : 0xc0000000;
   stEngPacket.dwDestBaseAddr = smesa->frontOffset;
   stEngPacket.wDestPitch = smesa->frontPitch;
   /* TODO: set maximum value? */
   stEngPacket.wDestHeight = smesa->virtualY;
   stEngPacket.stdwCmd.cRop = 0xf0;
   stEngPacket.dwFgRopColor = smesa->clearColorPattern;

   /* for SGRAM Block Write Enable */
   if (smesa->blockWrite)
      stEngPacket.stdwCmd.cCmd0 = CMD0_PAT_FG_COLOR;
   else
      stEngPacket.stdwCmd.cCmd0 = 0;
   stEngPacket.stdwCmd.cCmd1 = CMD1_DIR_X_INC | CMD1_DIR_Y_INC;

   while (count--) {
      GLint x2 = pExtents->x1 - smesa->driDrawable->x;
      GLint y2 = pExtents->y1 - smesa->driDrawable->y;
      GLint xx2 = pExtents->x2 - smesa->driDrawable->x;
      GLint yy2 = pExtents->y2 - smesa->driDrawable->y;

      x = (x0 > x2) ? x0 : x2;
      y = (y0 > y2) ? y0 : y2;
      xx = ((x0 + width0) > (xx2)) ? xx2 : x0 + width0;
      yy = ((y0 + height0) > (yy2)) ? yy2 : y0 + height0;
      width = xx - x;
      height = yy - y;
      pExtents++;

      if (width <= 0 || height <= 0)
	continue;

      stEngPacket.stdwDestPos.wY = y;
      stEngPacket.stdwDestPos.wX = x;
      stEngPacket.stdwDim.wWidth = (GLshort)width;
      stEngPacket.stdwDim.wHeight = (GLshort)height;

      sis_bitblt_clear_cmd( smesa, &stEngPacket );
   }
}

static void
sis_clear_z_stencil_buffer( GLcontext * ctx, GLbitfield mask,
			    GLint x, GLint y, GLint width, GLint height )
{
   sisContextPtr smesa = SIS_CONTEXT(ctx);

   /* TODO: check write mask */

   if ( smesa->depthbuffer == NULL )
      return;

   /* TODO: consider alignment of width, height? */
   smesa->zClearPacket.stdwDestPos.wY = y;
   smesa->zClearPacket.stdwDestPos.wX = x;
   smesa->zClearPacket.stdwDim.wWidth = (GLshort) width;
   smesa->zClearPacket.stdwDim.wHeight = (GLshort) height;
   smesa->zClearPacket.dwFgRopColor = smesa->clearZStencilPattern;

   sis_bitblt_clear_cmd( smesa, &smesa->zClearPacket );
}

