/* $XFree86: xc/lib/GL/mesa/src/drv/gamma/gamma_gl.c,v 1.2 1999/06/27 14:07:29 dawes Exp $ */
/**************************************************************************

Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sub license, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial portions
of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
IN NO EVENT SHALL PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR
ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <kevin@precisioninsight.com>
 *
 */

#ifdef GLX_DIRECT_RENDERING

#include <math.h>
#include "gamma_gl.h"
#include "gamma_init.h"
#ifdef RANDOMIZE_COLORS
#include <stdlib.h>
#endif

void glAccum(GLenum op, GLfloat value)
{
    DEBUG_GLCMDS(("Accum: %d %f\n", (int)op, value));
}

void glAlphaFunc(GLenum func, GLclampf ref)
{
    unsigned char r = ref * 255.0;

    DEBUG_GLCMDS(("AlphaFunc: %d %f\n", (int)func, (float)ref));

    gCCPriv->AlphaTestMode &= ~(AT_CompareMask | AT_RefValueMask);

    switch (func) {
    case GL_NEVER:
	gCCPriv->AlphaTestMode |= AT_Never;
	break;
    case GL_LESS:
	gCCPriv->AlphaTestMode |= AT_Less;
	break;
    case GL_EQUAL:
	gCCPriv->AlphaTestMode |= AT_Equal;
	break;
    case GL_LEQUAL:
	gCCPriv->AlphaTestMode |= AT_LessEqual;
	break;
    case GL_GREATER:
	gCCPriv->AlphaTestMode |= AT_Greater;
	break;
    case GL_NOTEQUAL:
	gCCPriv->AlphaTestMode |= AT_NotEqual;
	break;
    case GL_GEQUAL:
	gCCPriv->AlphaTestMode |= AT_GreaterEqual;
	break;
    case GL_ALWAYS:
	gCCPriv->AlphaTestMode |= AT_Always;
	break;
    default:
	/* ERROR!! */
	break;
    }

    gCCPriv->AlphaTestMode |= r << 4;

    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
    WRITE(gCCPriv->buf, AlphaTestMode, gCCPriv->AlphaTestMode);
}

GLboolean glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
    DEBUG_GLCMDS(("AreTexturesResident: %d\n", (int)n));
#ifdef DEBUG_VERBOSE_EXTRA
    {
	int t;
	for (t = 0; t < n; t++)
	    printf("\t%d\n", (int)textures[t]);
    }
#endif

    return GL_TRUE;
}

void glArrayElement(GLint i)
{
    DEBUG_GLCMDS(("ArrayElement: %d\n", (int)i));
}

void glBegin(GLenum mode)
{
    DEBUG_GLCMDS(("Begin: %04x\n", (int)mode));

    if ((gCCPriv->Begin & B_PrimType_Mask) != B_PrimType_Null) {
	/* ERROR!!! */
	return;
    }

    gCCPriv->Begin &= ~B_PrimType_Mask;
    switch (mode) {
    case GL_POINTS:
	gCCPriv->Begin |= B_PrimType_Points;
	break;
    case GL_LINES:
	gCCPriv->Begin |= B_PrimType_Lines;
	break;
    case GL_LINE_LOOP:
	gCCPriv->Begin |= B_PrimType_LineLoop;
	break;
    case GL_LINE_STRIP:
	gCCPriv->Begin |= B_PrimType_LineStrip;
	break;
    case GL_TRIANGLES:
	gCCPriv->Begin |= B_PrimType_Triangles;
	break;
    case GL_TRIANGLE_STRIP:
	gCCPriv->Begin |= B_PrimType_TriangleStrip;
	break;
    case GL_TRIANGLE_FAN:
	gCCPriv->Begin |= B_PrimType_TriangleFan;
	break;
    case GL_QUADS:
	gCCPriv->Begin |= B_PrimType_Quads;
	break;
    case GL_QUAD_STRIP:
	gCCPriv->Begin |= B_PrimType_QuadStrip;
	break;
    case GL_POLYGON:
	gCCPriv->Begin |= B_PrimType_Polygon;
	break;
    default:
	/* ERROR!! */
	break;
    }

    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
    WRITE(gCCPriv->buf, Begin, gCCPriv->Begin);
}

void glBindTexture(GLenum target, GLuint texture)
{
    unsigned long addrs[MIPMAP_LEVELS];
    int i;

    DEBUG_GLCMDS(("BindTexture: %04x %d\n",
		  (int)target, (unsigned int)texture));

    /* Disable all of the units in the previous bind */
    gCCPriv->curTexObj->TextureAddressMode &= ~TextureAddressModeEnable;
    gCCPriv->curTexObj->TextureReadMode    &= ~TextureReadModeEnable;
    gCCPriv->curTexObj->TextureColorMode   &= ~TextureColorModeEnable;
    gCCPriv->curTexObj->TextureFilterMode  &= ~TextureFilterModeEnable;

    /* Find the texture (create it, if necessary) */
    gCCPriv->curTexObj = gammaTOFind(texture);

    /* Make the new texture images resident */
    if (!driTMMMakeImagesResident(gCCPriv->tmm, MIPMAP_LEVELS,
				  gCCPriv->curTexObj->image, addrs)) {
	/* NOT_DONE: Handle error */
    }

    for (i = 0; i < MIPMAP_LEVELS; i++)
	gCCPriv->curTexObj->TextureBaseAddr[i] = addrs[i] << 5;

    /* Set the target */
    gCCPriv->curTexObj->TextureAddressMode &= ~TAM_TexMapType_Mask;
    gCCPriv->curTexObj->TextureReadMode    &= ~TRM_TexMapType_Mask;
    switch (target) {
    case GL_TEXTURE_1D:
	gCCPriv->curTexObj1D = gCCPriv->curTexObj;
	gCCPriv->curTexObj->TextureAddressMode |= TAM_TexMapType_1D;
	gCCPriv->curTexObj->TextureReadMode    |= TRM_TexMapType_1D;
	break;
    case GL_TEXTURE_2D:
	gCCPriv->curTexObj2D = gCCPriv->curTexObj;
	gCCPriv->curTexObj->TextureAddressMode |= TAM_TexMapType_2D;
	gCCPriv->curTexObj->TextureReadMode    |= TRM_TexMapType_2D;
	break;
    default:
	break;
    }

    /* Enable the units if texturing is enabled */
    if (target == GL_TEXTURE_1D && gCCPriv->Texture1DEnabled) {
	gCCPriv->curTexObj->TextureAddressMode |= TextureAddressModeEnable;
	gCCPriv->curTexObj->TextureReadMode    |= TextureReadModeEnable;
	gCCPriv->curTexObj->TextureColorMode   |= TextureColorModeEnable;
	gCCPriv->curTexObj->TextureFilterMode  |= TextureFilterModeEnable;
    } else if (target == GL_TEXTURE_2D && gCCPriv->Texture2DEnabled) {
	gCCPriv->curTexObj->TextureAddressMode |= TextureAddressModeEnable;
	gCCPriv->curTexObj->TextureReadMode    |= TextureReadModeEnable;
	gCCPriv->curTexObj->TextureColorMode   |= TextureColorModeEnable;
	gCCPriv->curTexObj->TextureFilterMode  |= TextureFilterModeEnable;
    }

    /* Restore the units */
    CHECK_DMA_BUFFER(gCC, gCCPriv, 18);
    WRITE(gCCPriv->buf, TextureAddressMode,
	  gCCPriv->curTexObj->TextureAddressMode);
    WRITE(gCCPriv->buf, TextureReadMode,
	  gCCPriv->curTexObj->TextureReadMode);
    WRITE(gCCPriv->buf, TextureColorMode,
	  gCCPriv->curTexObj->TextureColorMode);
    WRITE(gCCPriv->buf, TextureFilterMode,
	  gCCPriv->curTexObj->TextureFilterMode);
    WRITE(gCCPriv->buf, TextureFormat,
	  gCCPriv->curTexObj->TextureFormat);
    WRITE(gCCPriv->buf, TxBaseAddr0,  gCCPriv->curTexObj->TextureBaseAddr[ 0]);
    WRITE(gCCPriv->buf, TxBaseAddr1,  gCCPriv->curTexObj->TextureBaseAddr[ 1]);
    WRITE(gCCPriv->buf, TxBaseAddr2,  gCCPriv->curTexObj->TextureBaseAddr[ 2]);
    WRITE(gCCPriv->buf, TxBaseAddr3,  gCCPriv->curTexObj->TextureBaseAddr[ 3]);
    WRITE(gCCPriv->buf, TxBaseAddr4,  gCCPriv->curTexObj->TextureBaseAddr[ 4]);
    WRITE(gCCPriv->buf, TxBaseAddr5,  gCCPriv->curTexObj->TextureBaseAddr[ 5]);
    WRITE(gCCPriv->buf, TxBaseAddr6,  gCCPriv->curTexObj->TextureBaseAddr[ 6]);
    WRITE(gCCPriv->buf, TxBaseAddr7,  gCCPriv->curTexObj->TextureBaseAddr[ 7]);
    WRITE(gCCPriv->buf, TxBaseAddr8,  gCCPriv->curTexObj->TextureBaseAddr[ 8]);
    WRITE(gCCPriv->buf, TxBaseAddr9,  gCCPriv->curTexObj->TextureBaseAddr[ 9]);
    WRITE(gCCPriv->buf, TxBaseAddr10, gCCPriv->curTexObj->TextureBaseAddr[10]);
    WRITE(gCCPriv->buf, TxBaseAddr11, gCCPriv->curTexObj->TextureBaseAddr[11]);
    WRITE(gCCPriv->buf, TextureCacheControl, (TCC_Enable | TCC_Invalidate));
}

void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    DEBUG_GLCMDS(("Bitmap: \n"));
}

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    DEBUG_GLCMDS(("BlendFunc: %04x %04x\n", (int)sfactor, (int)dfactor));

    gCCPriv->AB_FBReadMode_Save = 0;
    gCCPriv->AlphaBlendMode &= ~(AB_SrcBlendMask | AB_DstBlendMask);

    switch (sfactor) {
    case GL_ZERO:
	gCCPriv->AlphaBlendMode |= AB_Src_Zero;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    case GL_ONE:
	gCCPriv->AlphaBlendMode |= AB_Src_One;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    case GL_DST_COLOR:
	gCCPriv->AlphaBlendMode |= AB_Src_DstColor;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    case GL_ONE_MINUS_DST_COLOR:
	gCCPriv->AlphaBlendMode |= AB_Src_OneMinusDstColor;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    case GL_SRC_ALPHA:
	gCCPriv->AlphaBlendMode |= AB_Src_SrcAlpha;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    case GL_ONE_MINUS_SRC_ALPHA:
	gCCPriv->AlphaBlendMode |= AB_Src_OneMinusSrcAlpha;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    case GL_DST_ALPHA:
	gCCPriv->AlphaBlendMode |= AB_Src_DstAlpha;
	gCCPriv->AB_FBReadMode_Save |= (FBReadSrcEnable | FBReadDstEnable);
	break;
    case GL_ONE_MINUS_DST_ALPHA:
	gCCPriv->AlphaBlendMode |= AB_Src_OneMinusDstAlpha;
	gCCPriv->AB_FBReadMode_Save |= (FBReadSrcEnable | FBReadDstEnable);
	break;
    case GL_SRC_ALPHA_SATURATE:
	gCCPriv->AlphaBlendMode |= AB_Src_SrcAlphaSaturate;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    default:
	/* ERROR!! */
	break;
    }

    switch (dfactor) {
    case GL_ZERO:
	gCCPriv->AlphaBlendMode |= AB_Dst_Zero;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    case GL_ONE:
	gCCPriv->AlphaBlendMode |= AB_Dst_One;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    case GL_SRC_COLOR:
	gCCPriv->AlphaBlendMode |= AB_Dst_SrcColor;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    case GL_ONE_MINUS_SRC_COLOR:
	gCCPriv->AlphaBlendMode |= AB_Dst_OneMinusSrcColor;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    case GL_SRC_ALPHA:
	gCCPriv->AlphaBlendMode |= AB_Dst_SrcAlpha;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    case GL_ONE_MINUS_SRC_ALPHA:
	gCCPriv->AlphaBlendMode |= AB_Dst_OneMinusSrcAlpha;
	gCCPriv->AB_FBReadMode_Save |= (FBReadDstEnable);
	break;
    case GL_DST_ALPHA:
	gCCPriv->AlphaBlendMode |= AB_Dst_DstAlpha;
	gCCPriv->AB_FBReadMode_Save |= (FBReadSrcEnable | FBReadDstEnable);
	break;
    case GL_ONE_MINUS_DST_ALPHA:
	gCCPriv->AlphaBlendMode |= AB_Dst_OneMinusDstAlpha;
	gCCPriv->AB_FBReadMode_Save |= (FBReadSrcEnable | FBReadDstEnable);
	break;
    default:
	/* ERROR!! */
	break;
    }

    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
    WRITE(gCCPriv->buf, AlphaBlendMode, gCCPriv->AlphaBlendMode);

    if (gCCPriv->AlphaBlendMode & AlphaBlendModeEnable) {
	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	gCCPriv->AB_FBReadMode = gCCPriv->AB_FBReadMode_Save;
	WRITE(gCCPriv->buf, FBReadMode, (gCCPriv->FBReadMode |
					 gCCPriv->AB_FBReadMode));
    }
}

void glCallList(GLuint list)
{
    DEBUG_GLCMDS(("CallList: %d\n", (unsigned int)list));
}

void glCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    DEBUG_GLCMDS(("CallLists: %d %04x\n", (int)n, (int)type));
}

void glClear(GLbitfield mask)
{
    unsigned int depth = 0;
    int do_clear = 0;
#ifdef DO_VALIDATE
    __DRIscreenPrivate *driScrnPriv = gCC->driContextPriv->driScreenPriv;
#endif

    DEBUG_GLCMDS(("Clear: %04x\n", (int)mask));

#ifdef TURN_OFF_CLEARS
    {
	static int done_first_clear = 0;
	if (done_first_clear)
	    return;
	done_first_clear = 1;
    }
#endif

#ifdef DO_VALIDATE
    /* Flush any partially filled buffers */
    FLUSH_DMA_BUFFER(gCC,gCCPriv);

    DRM_SPINLOCK(&driScrnPriv->pSAREA->drawable_lock,
		 driScrnPriv->drawLockID);
    VALIDATE_DRAWABLE_INFO_NO_LOCK(gCC,gCCPriv);
#endif

    if ((mask & GL_DEPTH_BUFFER_BIT) &&
	(gCCPriv->Flags & GAMMA_DEPTH_BUFFER)) {
	double d = (((double)gCCPriv->ClearDepth-gCCPriv->zNear)/
		    (gCCPriv->zFar-gCCPriv->zNear));

	if (d > 1.0) d = 1.0;
	else if (d < 0.0) d = 0.0;

	switch (gCCPriv->DepthSize) {
	case 16:
	    depth = d * 65535.0; /* 2^16-1 */
	    break;
	case 24:
	    depth = d * 16777215.0; /* 2^24-1 */
	    break;
	case 32:
	    depth = d * 4294967295.0; /* 2^32-1 */
	    break;
	}

#ifdef TURN_OFF_CLEARS
	depth = 0;
#endif

	/* Turn off writes the FB */
	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, FBWriteMode, FBWriteModeDisable);

	/*
	 * Turn Rectangle2DControl off when the window is not clipped
	 * (i.e., the GID tests are not necessary).  This dramatically
	 * increases the performance of the depth clears.
	 */
	if (!gCCPriv->NotClipped) {
	    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	    WRITE(gCCPriv->buf, Rectangle2DControl, 1);
	}

	CHECK_DMA_BUFFER(gCC, gCCPriv, 2);
	WRITE(gCCPriv->buf, DepthMode, (DepthModeEnable |
					DM_Always |
					DM_SourceDepthRegister |
					DM_WriteMask));
	WRITE(gCCPriv->buf, GLINTDepth, depth);

	/* Increment the frame count */
	gCCPriv->FrameCount++;
#ifdef FAST_CLEAR_4
	gCCPriv->FrameCount &= 0x0f;
#else
	gCCPriv->FrameCount &= 0xff;
#endif

	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, LBReadMode, 
	      ((gCCPriv->LBReadMode & LBPartialProdMask) |
	       LBScanLineInt2 |
	       LBWindowOriginBot));

	/* Force FCP to be written */
	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, GLINTWindow, (WindowEnable |
					  W_PassIfEqual |
					  (gCCPriv->Window & W_GIDMask) |
					  W_DepthFCP |
					  W_LBUpdateFromRegisters |
					  W_OverrideWriteFiltering |
					  (gCCPriv->FrameCount << 9)));

	/* Clear part of the depth and FCP buffers */
	{
	    int y = gCCPriv->y;
	    int h = gCCPriv->h;
#ifndef TURN_OFF_FCP
	    float hsub = h;

	    if (gCCPriv->WindowChanged) {
		gCCPriv->WindowChanged = GL_FALSE;
	    } else {
#ifdef FAST_CLEAR_4
		hsub /= 16;
#else
		hsub /= 256;
#endif

		/* Handle the case where the height < # of FCPs */
		if (hsub < 1.0) {
		    if (gCCPriv->FrameCount > h)
			gCCPriv->FrameCount = 0;
		    h = 1;
		    y += gCCPriv->FrameCount;
		} else {
		    h = (gCCPriv->FrameCount+1)*hsub;
		    h -= (int)(gCCPriv->FrameCount*hsub);
		    y += gCCPriv->FrameCount*hsub;
		}
	    }
#endif

	    if (h) {
		CHECK_DMA_BUFFER(gCC, gCCPriv, 8);
		WRITE(gCCPriv->buf, StartXDom,   gCCPriv->x<<16);
		WRITE(gCCPriv->buf, StartY,      y<<16);
		WRITE(gCCPriv->buf, StartXSub,  (gCCPriv->x+gCCPriv->w)<<16);
		WRITE(gCCPriv->buf, GLINTCount,  h);
		WRITE(gCCPriv->buf, dY,          1<<16);
		WRITE(gCCPriv->buf, dXDom,       0<<16);
		WRITE(gCCPriv->buf, dXSub,       0<<16);
		WRITE(gCCPriv->buf, Render,      0x00000040); /* NOT_DONE */
	    }
	}

	/* Restore modes */
	CHECK_DMA_BUFFER(gCC, gCCPriv, 5);
	WRITE(gCCPriv->buf, FBWriteMode, FBWriteModeEnable);
	WRITE(gCCPriv->buf, DepthMode, gCCPriv->DepthMode);
	WRITE(gCCPriv->buf, LBReadMode, gCCPriv->LBReadMode);
	WRITE(gCCPriv->buf, GLINTWindow, gCCPriv->Window);
	WRITE(gCCPriv->buf, FastClearDepth, depth);

	/* Turn on Depth FCP */
	if (gCCPriv->Window & W_DepthFCP) {
	    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	    WRITE(gCCPriv->buf, WindowOr, (gCCPriv->FrameCount << 9));
	}

	/* Turn off GID clipping if window is not clipped */
	if (gCCPriv->NotClipped) {
	    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	    WRITE(gCCPriv->buf, Rectangle2DControl, 0);
	}
    }

    if (mask & GL_COLOR_BUFFER_BIT) {
	CHECK_DMA_BUFFER(gCC, gCCPriv, 3);
	WRITE(gCCPriv->buf, ConstantColor,
	      (((GLuint)(gCCPriv->ClearColor[3]*255.0) << 24) |
	       ((GLuint)(gCCPriv->ClearColor[2]*255.0) << 16) |
	       ((GLuint)(gCCPriv->ClearColor[1]*255.0) << 8)  |
	       ((GLuint)(gCCPriv->ClearColor[0]*255.0))));
	WRITE(gCCPriv->buf, FBBlockColor,
	      (((GLuint)(gCCPriv->ClearColor[3]*255.0) << 24) |
	       ((GLuint)(gCCPriv->ClearColor[0]*255.0) << 16) |
	       ((GLuint)(gCCPriv->ClearColor[1]*255.0) << 8)  |
	       ((GLuint)(gCCPriv->ClearColor[2]*255.0))));
	WRITE(gCCPriv->buf, ColorDDAMode, (ColorDDAEnable |
					   ColorDDAFlat));
	do_clear = 1;
    } else {
	/* Turn off writes the FB */
	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, FBWriteMode, FBWriteModeDisable);
    }

    if (do_clear) {
	/* Turn on GID clipping if window is not clipped */
	if (!gCCPriv->NotClipped) {
	    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	    WRITE(gCCPriv->buf, Rectangle2DControl, 1);
	}

	CHECK_DMA_BUFFER(gCC, gCCPriv, 6);
	WRITE(gCCPriv->buf, DepthMode, 0);
	WRITE(gCCPriv->buf, AlphaBlendMode, 0);
	WRITE(gCCPriv->buf, Rectangle2DMode, (((gCCPriv->h & 0xfff)<<12) |
					      (gCCPriv->w & 0xfff)));
	WRITE(gCCPriv->buf, DrawRectangle2D, (((gCCPriv->y & 0xffff)<<16) |
					      (gCCPriv->x & 0xffff)));
	WRITE(gCCPriv->buf, DepthMode, gCCPriv->DepthMode);
	WRITE(gCCPriv->buf, AlphaBlendMode, gCCPriv->AlphaBlendMode);

	/* Turn off GID clipping if window is not clipped */
	if (gCCPriv->NotClipped) {
	    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	    WRITE(gCCPriv->buf, Rectangle2DControl, 0);
	}
    }

    if (mask & GL_COLOR_BUFFER_BIT) {
	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, ColorDDAMode, gCCPriv->ColorDDAMode);
    } else {
	/* Turn on writes the FB */
	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, FBWriteMode, FBWriteModeEnable);
    }

#ifdef DO_VALIDATE
    PROCESS_DMA_BUFFER_TOP_HALF(gCCPriv);

    DRM_SPINUNLOCK(&driScrnPriv->pSAREA->drawable_lock,
		   driScrnPriv->drawLockID);
    VALIDATE_DRAWABLE_INFO_NO_LOCK_POST(gCC,gCCPriv);

    PROCESS_DMA_BUFFER_BOTTOM_HALF(gCCPriv);
#endif

#if 0
    FLUSH_DMA_BUFFER(gCC,gCCPriv);
#endif
}

void glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    DEBUG_GLCMDS(("ClearAccum: %f %f %f %f\n", red, green, blue, alpha));
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    DEBUG_GLCMDS(("ClearColor: %f %f %f %f\n",
		  (float)red, (float)green, (float)blue, (float)alpha));

    gCCPriv->ClearColor[0] = red;
    gCCPriv->ClearColor[1] = green;
    gCCPriv->ClearColor[2] = blue;
    gCCPriv->ClearColor[3] = alpha;
}

void glClearDepth(GLclampd depth)
{
    DEBUG_GLCMDS(("ClearDepth: %f\n", (float)depth));

    gCCPriv->ClearDepth = depth;
}

void glClearIndex(GLfloat c)
{
    DEBUG_GLCMDS(("ClearIndex: %f\n", c));
}

void glClearStencil(GLint s)
{
    DEBUG_GLCMDS(("ClearStencil: %d\n", (int)s));
}

void glClipPlane(GLenum plane, const GLdouble *equation)
{
    DEBUG_GLCMDS(("ClipPlane: %04x %f %f %f %f\n", (int)plane,
		  equation[0], equation[1], equation[2], equation[3]));
}

void glColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
    DEBUG_GLCMDS(("Color3b: %d %d %d\n", red, green, blue));
}

void glColor3bv(const GLbyte *v)
{
    DEBUG_GLCMDS(("Color3bv: %d %d %d\n", v[0], v[1], v[2]));
}

void glColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
    DEBUG_GLCMDS(("Color3d: %f %f %f\n", red, green, blue));
}

void glColor3dv(const GLdouble *v)
{
    DEBUG_GLCMDS(("Color3dv: %f %f %f\n", v[0], v[1], v[2]));
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
    DEBUG_GLCMDS(("Color3f: %f %f %f\n", red, green, blue));

#ifdef RANDOMIZE_COLORS
    red = (random() / (double)RAND_MAX);
    green = (random() / (double)RAND_MAX);
    blue = (random() / (double)RAND_MAX);
#endif

    CHECK_DMA_BUFFER(gCC, gCCPriv, 3);
    WRITEF(gCCPriv->buf, Cb,  blue);
    WRITEF(gCCPriv->buf, Cg,  green);
    WRITEF(gCCPriv->buf, Cr3, red);
}

void glColor3fv(const GLfloat *v)
{
    DEBUG_GLCMDS(("Color3fv: %f %f %f\n", v[0], v[1], v[2]));

#ifdef RANDOMIZE_COLORS
    {
	float r, g, b;
	r = (random() / (double)RAND_MAX);
	g = (random() / (double)RAND_MAX);
	b = (random() / (double)RAND_MAX);
	CHECK_DMA_BUFFER(gCC, gCCPriv, 3);
	WRITEF(gCCPriv->buf, Cb,  b);
	WRITEF(gCCPriv->buf, Cg,  g);
	WRITEF(gCCPriv->buf, Cr3, r);
    }
#else
    CHECK_DMA_BUFFER(gCC, gCCPriv, 3);
    WRITEF(gCCPriv->buf, Cb,  v[2]);
    WRITEF(gCCPriv->buf, Cg,  v[1]);
    WRITEF(gCCPriv->buf, Cr3, v[0]);
#endif
}

void glColor3i(GLint red, GLint green, GLint blue)
{
    DEBUG_GLCMDS(("Color3i: %d %d %d\n", (int)red, (int)green, (int)blue));
}

void glColor3iv(const GLint *v)
{
    DEBUG_GLCMDS(("Color3iv: %d %d %d\n", (int)v[0], (int)v[1], (int)v[2]));
}

void glColor3s(GLshort red, GLshort green, GLshort blue)
{
    DEBUG_GLCMDS(("Color3s: %d %d %d\n", red, green, blue));
}

void glColor3sv(const GLshort *v)
{
    DEBUG_GLCMDS(("Color3sv: %d %d %d\n", v[0], v[1], v[2]));
}

void glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    DEBUG_GLCMDS(("Color3ub: %d %d %d\n", red, green, blue));
}

void glColor3ubv(const GLubyte *v)
{
    DEBUG_GLCMDS(("Color3ubv: %d %d %d\n", v[0], v[1], v[2]));
}

void glColor3ui(GLuint red, GLuint green, GLuint blue)
{
    DEBUG_GLCMDS(("Color3ui: %d %d %d\n",
		  (unsigned int)red, (unsigned int)green, (unsigned int)blue));
}

void glColor3uiv(const GLuint *v)
{
    DEBUG_GLCMDS(("Color3uiv: %d %d %d\n",
		  (unsigned int)v[0], (unsigned int)v[1], (unsigned int)v[2]));
}

void glColor3us(GLushort red, GLushort green, GLushort blue)
{
    DEBUG_GLCMDS(("Color3us: %d %d %d\n", red, green, blue));
}

void glColor3usv(const GLushort *v)
{
    DEBUG_GLCMDS(("Color3usv: %d %d %d\n", v[0], v[1], v[2]));
}

void glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    DEBUG_GLCMDS(("Color4b: %d %d %d %d\n", red, green, blue, alpha));
}

void glColor4bv(const GLbyte *v)
{
    DEBUG_GLCMDS(("Color4bv: %d %d %d %d\n", v[0], v[1], v[2], v[3]));
}

void glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    DEBUG_GLCMDS(("Color4d: %f %f %f %f\n", red, green, blue, alpha));
}

void glColor4dv(const GLdouble *v)
{
    DEBUG_GLCMDS(("Color4dv: %f %f %f %f\n", v[0], v[1], v[2], v[3]));
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    DEBUG_GLCMDS(("Color4f: %f %f %f %f\n", red, green, blue, alpha));

#ifdef RANDOMIZE_COLORS
    red = (random() / (double)RAND_MAX);
    green = (random() / (double)RAND_MAX);
    blue = (random() / (double)RAND_MAX);
#endif

    CHECK_DMA_BUFFER(gCC, gCCPriv, 4);
    WRITEF(gCCPriv->buf, Ca,  alpha);
    WRITEF(gCCPriv->buf, Cb,  blue);
    WRITEF(gCCPriv->buf, Cg,  green);
    WRITEF(gCCPriv->buf, Cr4, red);
}

void glColor4fv(const GLfloat *v)
{
    DEBUG_GLCMDS(("Color4fv: %f %f %f %f\n", v[0], v[1], v[2], v[3]));

#ifdef RANDOMIZE_COLORS
    {
	float r, g, b;
	r = (random() / (double)RAND_MAX);
	g = (random() / (double)RAND_MAX);
	b = (random() / (double)RAND_MAX);
	CHECK_DMA_BUFFER(gCC, gCCPriv, 4);
	WRITEF(gCCPriv->buf, Ca,  v[3]);
	WRITEF(gCCPriv->buf, Cb,  b);
	WRITEF(gCCPriv->buf, Cg,  g);
	WRITEF(gCCPriv->buf, Cr3, r);
    }
#else
    CHECK_DMA_BUFFER(gCC, gCCPriv, 4);
    WRITEF(gCCPriv->buf, Ca,  v[3]);
    WRITEF(gCCPriv->buf, Cb,  v[2]);
    WRITEF(gCCPriv->buf, Cg,  v[1]);
    WRITEF(gCCPriv->buf, Cr4, v[0]);
#endif
}

void glColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
    DEBUG_GLCMDS(("Color4i: %d %d %d %d\n", (int)red, (int)green, (int)blue,
		  (int)alpha));
}

void glColor4iv(const GLint *v)
{
    DEBUG_GLCMDS(("Color4iv: %d %d %d %d\n", (int)v[0], (int)v[1], (int)v[2],
		  (int)v[3]));
}

void glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    DEBUG_GLCMDS(("Color4s: %d %d %d %d\n", red, green, blue, alpha));
}

void glColor4sv(const GLshort *v)
{
    DEBUG_GLCMDS(("Color4sv: %d %d %d %d\n", v[0], v[1], v[2], v[3]));
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    GLuint c;

    DEBUG_GLCMDS(("Color4ub: %d %d %d %d\n", red, green, blue, alpha));

#ifdef RANDOMIZE_COLORS
    c = (random() / (double)RAND_MAX) * 16777216;
#else
    c = (alpha << 24) | (blue << 16) | (green << 8) | red;
#endif

    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
    WRITE(gCCPriv->buf, PackedColor4,  c);
}

void glColor4ubv(const GLubyte *v)
{
    GLuint c;

    DEBUG_GLCMDS(("Color4ubv: %d %d %d %d\n", v[0], v[1], v[2], v[3]));

#ifdef RANDOMIZE_COLORS
    c = (random() / (double)RAND_MAX) * 16777216;
#else
/* NOT_DONE: Is there a standard define for endianness? */
#define IS_LITTLE_ENDIAN 1
#if IS_LITTLE_ENDIAN
    c = *((GLuint *)v);
#else
    c = (v[3] << 24) | (v[2] << 16) | (v[1] << 8) | v[0];
#endif
#endif

    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
    WRITE(gCCPriv->buf, PackedColor4,  c);
}

void glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    DEBUG_GLCMDS(("Color4ui: %d %d %d %d\n",
		  (unsigned int)red, (unsigned int)green,
		  (unsigned int)blue, (unsigned int)alpha));
}

void glColor4uiv(const GLuint *v)
{
    DEBUG_GLCMDS(("Color4uiv: %d %d %d %d\n",
		  (unsigned int)v[0], (unsigned int)v[1],
		  (unsigned int)v[2], (unsigned int)v[3]));
}

void glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    DEBUG_GLCMDS(("Color4us: %d %d %d %d\n", red, green, blue, alpha));
}

void glColor4usv(const GLushort *v)
{
    DEBUG_GLCMDS(("Color4usv: %d %d %d %d\n", v[0], v[1], v[2], v[3]));
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    DEBUG_GLCMDS(("ColorMask: %d %d %d %d\n", red, green, blue, alpha));
}

void glColorMaterial(GLenum face, GLenum mode)
{
    DEBUG_GLCMDS(("ColorMaterial: %04x %04x\n", (int)face, (int)mode));
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    DEBUG_GLCMDS(("ColorPointer: %d %04x %d\n",
		  (int)size, (int)type, (int)stride));
}

void glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    DEBUG_GLCMDS(("CopyPixels: %d %d %d %d %04x\n", (int)x, (int)y,
		  (int)width, (int)height, (int)type));
}

void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    DEBUG_GLCMDS(("CopyTexImage1D: %04x %d %04x %d %d %d %d\n",
		  (int)target, (int)level, (int)internalformat,
		  (int)x, (int)y, (int)width, (int)border));
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    DEBUG_GLCMDS(("CopyTexImage2D: %04x %d %04x %d %d %d %d %d\n",
		  (int)target, (int)level, (int)internalformat,
		  (int)x, (int)y, (int)width, (int)height, (int)border));
}

void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    DEBUG_GLCMDS(("CopyTexSubImage1D: %04x %d %d %d %d %d\n",
		  (int)target, (int)level,
		  (int)xoffset, (int)x, (int)y, (int)width));
}

void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    DEBUG_GLCMDS(("CopyTexSubImage2D: %04x %d %d %d %d %d %d %d\n",
		  (int)target, (int)level, (int)xoffset, (int)yoffset,
		  (int)x, (int)y, (int)width, (int)height));
}

void glCullFace(GLenum mode)
{
    DEBUG_GLCMDS(("CullFace: %04x\n", (int)mode));

    gCCPriv->GeometryMode &= ~GM_PolyCullMask;

#ifdef CULL_ALL_PRIMS
    gCCPriv->GeometryMode |= GM_PolyCullBoth;
#else
    switch (mode) {
    case GL_FRONT:
	gCCPriv->GeometryMode |= GM_PolyCullFront;
	break;
    case GL_BACK:
	gCCPriv->GeometryMode |= GM_PolyCullBack;
	break;
    case GL_FRONT_AND_BACK:
	gCCPriv->GeometryMode |= GM_PolyCullBoth;
	break;
    default:
	/* ERROR!! */
	break;
    }
#endif
    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
    WRITE(gCCPriv->buf, GeometryMode, gCCPriv->GeometryMode);
}

void glDeleteLists(GLuint list, GLsizei range)
{
    DEBUG_GLCMDS(("DeleteLists: %d %d\n", (unsigned int)list, (int)range));
}

void glDeleteTextures(GLsizei n, const GLuint *textures)
{
    int i;

    DEBUG_GLCMDS(("DeleteTextures: %d\n", (int)n));
#ifdef DEBUG_VERBOSE_EXTRA
    {
	int t;
	for (t = 0; t < n; t++)
	    printf("\t%d\n", (unsigned int)textures[t]);
    }
#endif

    for (i = 0; i < n; i++) {
	gammaTexObj *t = gammaTOFind(textures[i]);
	if (!driTMMDeleteImages(gCCPriv->tmm, MIPMAP_LEVELS, t->image)) {
	    /* NOT_DONE: Handle error */
	}
	gammaTODelete(textures[i]);
    }

    gCCPriv->curTexObj = gammaTOFind(0);
    gCCPriv->curTexObj1D = gCCPriv->curTexObj;
    gCCPriv->curTexObj2D = gCCPriv->curTexObj;
}

void glDepthFunc(GLenum func)
{
    DEBUG_GLCMDS(("DepthFunc: %04x\n", (int)func));

    gCCPriv->DepthMode &= ~DM_CompareMask;

    switch (func) {
    case GL_NEVER:
	gCCPriv->DepthMode |= DM_Never;
	break;
    case GL_LESS:
	gCCPriv->DepthMode |= DM_Less;
	break;
    case GL_EQUAL:
	gCCPriv->DepthMode |= DM_Equal;
	break;
    case GL_LEQUAL:
	gCCPriv->DepthMode |= DM_LessEqual;
	break;
    case GL_GREATER:
	gCCPriv->DepthMode |= DM_Greater;
	break;
    case GL_NOTEQUAL:
	gCCPriv->DepthMode |= DM_NotEqual;
	break;
    case GL_GEQUAL:
	gCCPriv->DepthMode |= DM_GreaterEqual;
	break;
    case GL_ALWAYS:
	gCCPriv->DepthMode |= DM_Always;
	break;
    default:
	/* ERROR!! */
	break;
    }

    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
    WRITE(gCCPriv->buf, DepthMode, gCCPriv->DepthMode);
}

void glDepthMask(GLboolean flag)
{
    DEBUG_GLCMDS(("DepthMask: %d\n", flag));

    if (flag) {
	gCCPriv->DepthMode |=  DM_WriteMask;
    } else {
	gCCPriv->DepthMode &= ~DM_WriteMask;
    }

    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
    WRITE(gCCPriv->buf, DepthMode, gCCPriv->DepthMode);
}

void glDepthRange(GLclampd zNear, GLclampd zFar)
{
    GLfloat sz, oz;

    DEBUG_GLCMDS(("DepthRange: %f %f\n", (float)zNear, (float)zFar));

    gCCPriv->zNear = zNear;
    gCCPriv->zFar = zFar;

    oz = (zFar+zNear)/2.0;
    sz = (zFar-zNear)/2.0;

    CHECK_DMA_BUFFER(gCC, gCCPriv, 2);
    WRITEF(gCCPriv->buf, ViewPortScaleZ,  sz);
    WRITEF(gCCPriv->buf, ViewPortOffsetZ, oz);
}

void glDisable(GLenum cap)
{
    DEBUG_GLCMDS(("Disable %04x\n", (int)cap));

    switch (cap) {
    case GL_CULL_FACE:
#ifdef CULL_ALL_PRIMS
	gCCPriv->GeometryMode &= ~GM_PolyCullEnable;
	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, GeometryMode, gCCPriv->GeometryMode);
#endif
	break;
    case GL_DEPTH_TEST:
	if (gCCPriv->Flags & GAMMA_DEPTH_BUFFER) {
	    gCCPriv->EnabledFlags &= ~GAMMA_DEPTH_BUFFER;
	    gCCPriv->DepthMode &= ~DepthModeEnable;
	    gCCPriv->LBReadMode &= ~LBReadDstEnable;
	    gCCPriv->DeltaMode &= ~DM_DepthEnable;
	    gCCPriv->Window &= ~W_DepthFCP;

	    /* Turn depth mode off */
	    CHECK_DMA_BUFFER(gCC, gCCPriv, 4);
	    WRITE(gCCPriv->buf, DepthMode,      gCCPriv->DepthMode);
	    WRITE(gCCPriv->buf, DeltaMode,      gCCPriv->DeltaMode);
	    WRITE(gCCPriv->buf, LBReadModeAnd, ~LBReadDstEnable);
	    WRITE(gCCPriv->buf, WindowAnd,     ~W_DepthFCP);
	}
	break;
    case GL_ALPHA_TEST:
	/* Do I need to verify that alpha is enabled? */
	gCCPriv->AlphaTestMode &= ~AlphaTestModeEnable;
	CHECK_DMA_BUFFER(gCC, gCCPriv, 2);
	WRITE(gCCPriv->buf, AlphaTestMode, gCCPriv->AlphaTestMode);
	WRITE(gCCPriv->buf, RouterMode, R_Order_DepthTexture);
	break;
    case GL_BLEND:
	/* Do I need to verify that alpha is enabled? */
	gCCPriv->AlphaBlendMode &= ~AlphaBlendModeEnable;
	gCCPriv->AB_FBReadMode   =  0;
	CHECK_DMA_BUFFER(gCC, gCCPriv, 2);
	WRITE(gCCPriv->buf, AlphaBlendMode, gCCPriv->AlphaBlendMode);
	WRITE(gCCPriv->buf, FBReadMode, (gCCPriv->FBReadMode |
					 gCCPriv->AB_FBReadMode));
	break;
    case GL_TEXTURE_2D:
	gCCPriv->Texture2DEnabled = GL_FALSE;
	gCCPriv->Begin                         &= ~B_TextureEnable;
	gCCPriv->GeometryMode                  &= ~GM_TextureEnable;
	gCCPriv->DeltaMode                     &= ~DM_TextureEnable;
	gCCPriv->curTexObj->TextureAddressMode &= ~TextureAddressModeEnable;
	gCCPriv->curTexObj->TextureReadMode    &= ~TextureReadModeEnable;
	gCCPriv->curTexObj->TextureColorMode   &= ~TextureColorModeEnable;
	gCCPriv->curTexObj->TextureFilterMode  &= ~TextureFilterModeEnable;

	CHECK_DMA_BUFFER(gCC, gCCPriv, 6);
	WRITE(gCCPriv->buf, GeometryModeAnd, ~GM_TextureEnable);
	WRITE(gCCPriv->buf, DeltaModeAnd, ~DM_TextureEnable);
	WRITE(gCCPriv->buf, TextureAddressMode,
	      gCCPriv->curTexObj->TextureAddressMode);
	WRITE(gCCPriv->buf, TextureReadMode,
	      gCCPriv->curTexObj->TextureReadMode);
	WRITE(gCCPriv->buf, TextureColorMode,
	      gCCPriv->curTexObj->TextureColorMode);
	WRITE(gCCPriv->buf, TextureFilterMode,
	      gCCPriv->curTexObj->TextureFilterMode);
	break;
    default:
	break;
    }
}

void glDisableClientState(GLenum array)
{
    DEBUG_GLCMDS(("DisableClientState: %04x\n", (int)array));
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    DEBUG_GLCMDS(("DrawArrays: %04x %d %d\n", (int)mode, (int)first,
		  (int)count));
}

void glDrawBuffer(GLenum mode)
{
    DEBUG_GLCMDS(("DrawBuffer: %04x\n", (int)mode));
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    DEBUG_GLCMDS(("DrawElements: %04x %d %04x\n", (int)mode, (int)count,
		  (int)type));
}

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
    DEBUG_GLCMDS(("DrawPixels: %d %d %04x %04x\n", (int)width, (int)height,
		  (int)format, (int)type));
}

void glEdgeFlag(GLboolean flag)
{
    DEBUG_GLCMDS(("EdgeFlag: %d\n", flag));
}

void glEdgeFlagPointer(GLsizei stride, const GLvoid *pointer)
{
    DEBUG_GLCMDS(("EdgeFlagPointer: %d\n", (int)stride));
}

void glEdgeFlagv(const GLboolean *flag)
{
    DEBUG_GLCMDS(("EdgeFlagv: \n"));
}

void glEnable(GLenum cap)
{
    DEBUG_GLCMDS(("Enable %04x\n", (int)cap));

    switch (cap) {
    case GL_CULL_FACE:
	gCCPriv->GeometryMode |= GM_PolyCullEnable;
	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, GeometryMode, gCCPriv->GeometryMode);
	break;
    case GL_DEPTH_TEST:
	if (gCCPriv->Flags & GAMMA_DEPTH_BUFFER) {
	    gCCPriv->EnabledFlags |= GAMMA_DEPTH_BUFFER;
#ifndef TURN_OFF_DEPTH
	    gCCPriv->DepthMode |= DepthModeEnable;
	    gCCPriv->LBReadMode |= LBReadDstEnable;
	    gCCPriv->DeltaMode |= DM_DepthEnable;
#ifndef TURN_OFF_FCP
	    gCCPriv->Window |= W_DepthFCP;
#endif

	    /* Turn depth mode on */
	    CHECK_DMA_BUFFER(gCC, gCCPriv, 4);
	    WRITE(gCCPriv->buf, DepthMode,     gCCPriv->DepthMode);
	    WRITE(gCCPriv->buf, DeltaMode,     gCCPriv->DeltaMode);
	    WRITE(gCCPriv->buf, LBReadModeOr,  LBReadDstEnable);
#ifndef TURN_OFF_FCP
	    WRITE(gCCPriv->buf, WindowOr,     (W_DepthFCP |
					       (gCCPriv->FrameCount << 9)));
#else
	    WRITE(gCCPriv->buf, WindowOr,     (gCCPriv->FrameCount << 9));
#endif
#endif
	}
	break;
    case GL_ALPHA_TEST:
	gCCPriv->AlphaTestMode |= AlphaTestModeEnable;
	CHECK_DMA_BUFFER(gCC, gCCPriv, 2);
	WRITE(gCCPriv->buf, AlphaTestMode, gCCPriv->AlphaTestMode);
	WRITE(gCCPriv->buf, RouterMode, R_Order_TextureDepth);
	break;
    case GL_BLEND:
#ifndef TURN_OFF_BLEND
	gCCPriv->AlphaBlendMode |= AlphaBlendModeEnable;
	gCCPriv->AB_FBReadMode   = gCCPriv->AB_FBReadMode_Save;
#endif
	CHECK_DMA_BUFFER(gCC, gCCPriv, 2);
	WRITE(gCCPriv->buf, AlphaBlendMode, gCCPriv->AlphaBlendMode);
	WRITE(gCCPriv->buf, FBReadMode, (gCCPriv->FBReadMode |
					 gCCPriv->AB_FBReadMode));
	break;
    case GL_TEXTURE_2D:
	gCCPriv->Texture2DEnabled = GL_TRUE;
#ifndef TURN_OFF_TEXTURES
	gCCPriv->Begin                         |= B_TextureEnable;
#endif
	gCCPriv->GeometryMode                  |= GM_TextureEnable;
	gCCPriv->DeltaMode                     |= DM_TextureEnable;
	gCCPriv->curTexObj->TextureAddressMode |= TextureAddressModeEnable;
	gCCPriv->curTexObj->TextureReadMode    |= TextureReadModeEnable;
	gCCPriv->curTexObj->TextureColorMode   |= TextureColorModeEnable;
	gCCPriv->curTexObj->TextureFilterMode  |= TextureFilterModeEnable;

	CHECK_DMA_BUFFER(gCC, gCCPriv, 6);
	WRITE(gCCPriv->buf, GeometryModeOr, GM_TextureEnable);
	WRITE(gCCPriv->buf, DeltaModeOr, DM_TextureEnable);
	WRITE(gCCPriv->buf, TextureAddressMode,
	      gCCPriv->curTexObj->TextureAddressMode);
	WRITE(gCCPriv->buf, TextureReadMode,
	      gCCPriv->curTexObj->TextureReadMode);
	WRITE(gCCPriv->buf, TextureColorMode,
	      gCCPriv->curTexObj->TextureColorMode);
	WRITE(gCCPriv->buf, TextureFilterMode,
	      gCCPriv->curTexObj->TextureFilterMode);
	break;
    default:
	break;
    }
}

void glEnableClientState(GLenum array)
{
    DEBUG_GLCMDS(("EnableClientState: %04x\n", (int)array));
}

void glEnd(void)
{
    DEBUG_GLCMDS(("End\n"));

    if ((gCCPriv->Begin & B_PrimType_Mask) == B_PrimType_Null) {
	/* ERROR!!! */
	return;
    }

    /* No longer inside of Begin/End */
    gCCPriv->Begin &= ~B_PrimType_Mask;
    gCCPriv->Begin |=  B_PrimType_Null;

    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
    WRITE(gCCPriv->buf, End, 0);
#if 0
    /* To force creation of smaller buffers */
    FLUSH_DMA_BUFFER(gCC,gCCPriv);
#endif
}

void glEndList(void)
{
    DEBUG_GLCMDS(("EndList\n"));
}

void glEvalCoord1d(GLdouble u)
{
    DEBUG_GLCMDS(("EvalCoord1d: %f\n", u));
}

void glEvalCoord1dv(const GLdouble *u)
{
}

void glEvalCoord1f(GLfloat u)
{
}

void glEvalCoord1fv(const GLfloat *u)
{
}

void glEvalCoord2d(GLdouble u, GLdouble v)
{
}

void glEvalCoord2dv(const GLdouble *u)
{
}

void glEvalCoord2f(GLfloat u, GLfloat v)
{
}

void glEvalCoord2fv(const GLfloat *u)
{
}

void glEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
}

void glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
}

void glEvalPoint1(GLint i)
{
}

void glEvalPoint2(GLint i, GLint j)
{
}

void glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
    DEBUG_GLCMDS(("FeedbackBuffer: %d %04x\n", (int)size, (int)type));
}

void glFinish(void)
{
    DEBUG_GLCMDS(("Finish\n"));

    FLUSH_DMA_BUFFER(gCC,gCCPriv);
}

void glFlush(void)
{
    DEBUG_GLCMDS(("Flush\n"));

    FLUSH_DMA_BUFFER(gCC,gCCPriv);
}

void glFogf(GLenum pname, GLfloat param)
{
    DEBUG_GLCMDS(("Fogf: %04x %f\n", (int)pname, param));
}

void glFogfv(GLenum pname, const GLfloat *params)
{
    DEBUG_GLCMDS(("Fogfv: %04x %f\n", (int)pname, *params));
}

void glFogi(GLenum pname, GLint param)
{
    DEBUG_GLCMDS(("Fogi: %04x %d\n", (int)pname, (int)param));
}

void glFogiv(GLenum pname, const GLint *params)
{
    DEBUG_GLCMDS(("Fogiv: %04x %d\n", (int)pname, (int)*params));
}

void glFrontFace(GLenum mode)
{
    DEBUG_GLCMDS(("FrontFace: %04x\n", (int)mode));
}

void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
#define FRUSTUM_X() ((GLfloat)((2.0*zNear)/(right-left)))
#define FRUSTUM_Y() ((GLfloat)((2.0*zNear)/(top-bottom)))

#define FRUSTUM_A() ((GLfloat)(      (right+left)/(right-left)))
#define FRUSTUM_B() ((GLfloat)(      (top+bottom)/(top-bottom)))
#define FRUSTUM_C() ((GLfloat)(    -((zFar+zNear)/(zFar-zNear))))
#define FRUSTUM_D() ((GLfloat)(-((2.0*zFar*zNear)/(zFar-zNear))))

    GLfloat m[16];
    int i;

    DEBUG_GLCMDS(("Frustum: %f %f %f %f %f %f\n",
		  left, right, bottom, top, zNear, zFar));

    for (i = 0; i < 16; i++) m[i] = 0.0;

    m[0]  = FRUSTUM_X();
    m[5]  = FRUSTUM_Y();
    m[8]  = FRUSTUM_A();
    m[9]  = FRUSTUM_B();
    m[10] = FRUSTUM_C();
    m[11] = -1.0;
    m[14] = FRUSTUM_D();

    gammaMultMatrix(m);
    gammaLoadHWMatrix();
}

GLuint glGenLists(GLsizei range)
{
    DEBUG_GLCMDS(("GenLists: %d\n", (int)range));

    return GL_TRUE;
}

void glGenTextures(GLsizei n, GLuint *textures)
{
    DEBUG_GLCMDS(("GenTextures: %d\n", (int)n));
}

void glGetBooleanv(GLenum val, GLboolean *b)
{
    DEBUG_GLCMDS(("GetBooleanv: %04x\n", (int)val));
}

void glGetClipPlane(GLenum plane, GLdouble *equation)
{
    DEBUG_GLCMDS(("GetClipPlane: %04x %f %f %f %f\n", (int)plane,
		  equation[0], equation[1], equation[2], equation[3]));
}

void glGetDoublev(GLenum val, GLdouble *d)
{
    DEBUG_GLCMDS(("GetDoublev: %04x\n", (int)val));
}

GLenum glGetError(void)
{
    DEBUG_GLCMDS(("GetError\n"));
    return 0;
}

void glGetFloatv(GLenum val, GLfloat *f)
{
    int i;

    DEBUG_GLCMDS(("GetFloatv: %04x\n", (int)val));

    switch (val) {
    case GL_MODELVIEW_MATRIX:
	for (i = 0; i < 16; i++)
	    f[i] = gCCPriv->ModelView[i];
	break;
    default:
	break;
    }
}

void glGetIntegerv(GLenum val, GLint *i)
{
    DEBUG_GLCMDS(("GetIntegerv: %04x\n", (int)val));
}

void glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
}

void glGetLightiv(GLenum light, GLenum pname, GLint *params)
{
}

void glGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
}

void glGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
}

void glGetMapiv(GLenum target, GLenum query, GLint *v)
{
}

void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
}

void glGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
}

void glGetPixelMapfv(GLenum map, GLfloat *values)
{
}

void glGetPixelMapuiv(GLenum map, GLuint *values)
{
}

void glGetPixelMapusv(GLenum map, GLushort *values)
{
}

void glGetPointerv(GLenum pname, void **params)
{
    DEBUG_GLCMDS(("GetPointerv: %04x\n", (int)pname));
}

void glGetPolygonStipple(GLubyte *mask)
{
}

const GLubyte *glGetString(GLenum name)
{
    static unsigned char vendor[] = "vendor";
    static unsigned char renderer[] = "renderer";
    static unsigned char version[] = "1.1";
    static unsigned char ext[] = "";

    switch (name) {
    case GL_VENDOR:
	return vendor;
    case GL_RENDERER:
	return renderer;
    case GL_VERSION:
	return version;
    case GL_EXTENSIONS:
	return ext;
    }

    return NULL;
}

void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
}

void glGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
}

void glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params)
{
}

void glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
}

void glGetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
}

void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *texels)
{
}

void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
}

void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
}

void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
}

void glHint(GLenum target, GLenum mode)
{
    DEBUG_GLCMDS(("Hint: %04x %04x\n", (int)target, (int)mode));
}

void glIndexMask(GLuint mask)
{
    DEBUG_GLCMDS(("Hint: %d\n", (unsigned int)mask));
}

void glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    DEBUG_GLCMDS(("IndexPointer: %04x %d\n", (int)type, (int)stride));
}

void glIndexd(GLdouble c)
{
    DEBUG_GLCMDS(("Indexd: %f\n", c));
}

void glIndexdv(const GLdouble *c)
{
    DEBUG_GLCMDS(("Indexdv: %f\n", *c));
}

void glIndexf(GLfloat c)
{
    DEBUG_GLCMDS(("Indexf: %f\n", c));
}

void glIndexfv(const GLfloat *c)
{
    DEBUG_GLCMDS(("Indexdv: %f\n", *c));
}

void glIndexi(GLint c)
{
    DEBUG_GLCMDS(("Indexi: %d\n", (int)c));
}

void glIndexiv(const GLint *c)
{
    DEBUG_GLCMDS(("Indexiv: %d\n", (int)*c));
}

void glIndexs(GLshort c)
{
    DEBUG_GLCMDS(("Indexs: %d\n", c));
}

void glIndexsv(const GLshort *c)
{
    DEBUG_GLCMDS(("Indexsv: %d\n", *c));
}

void glIndexub(GLubyte c)
{
    DEBUG_GLCMDS(("Indexub: %d\n", c));
}

void glIndexubv(const GLubyte *c)
{
    DEBUG_GLCMDS(("Indexubv: %d\n", *c));
}

void glInitNames(void)
{
    DEBUG_GLCMDS(("InitNames\n"));
}

void glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
    DEBUG_GLCMDS(("InterleavedArrays: %04x %d\n", (int)format, (int)stride));
}

GLboolean glIsEnabled(GLenum cap)
{
    DEBUG_GLCMDS(("IsEnabled: %04x\n", (int)cap));

    return GL_TRUE;
}

GLboolean glIsList(GLuint list)
{
    DEBUG_GLCMDS(("IsList: %04x\n", (unsigned int)list));

    return GL_TRUE;
}

GLboolean glIsTexture(GLuint texture)
{
    DEBUG_GLCMDS(("IsTexture: %04x\n", (unsigned int)texture));

    return GL_TRUE;
}

void glLightModelf(GLenum pname, GLfloat param)
{
}

void glLightModelfv(GLenum pname, const GLfloat *params)
{
}

void glLightModeli(GLenum pname, GLint param)
{
}

void glLightModeliv(GLenum pname, const GLint *params)
{
}

void glLightf(GLenum light, GLenum pname, GLfloat param)
{
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
}

void glLighti(GLenum light, GLenum pname, GLint param)
{
}

void glLightiv(GLenum light, GLenum pname, const GLint *params)
{
}

void glLineStipple(GLint factor, GLushort pattern)
{
    DEBUG_GLCMDS(("LineStipple: %d %d\n", (int)factor, pattern));
}

void glLineWidth(GLfloat width)
{
    DEBUG_GLCMDS(("LineWidth: %f\n", width));
}

void glListBase(GLuint base)
{
    DEBUG_GLCMDS(("ListBase: %d\n", (unsigned int)base));
}

void glLoadIdentity(void)
{
    DEBUG_GLCMDS(("LoadIdentity: %04x\n", gCCPriv->MatrixMode));

    gammaSetMatrix(IdentityMatrix);
    gammaLoadHWMatrix();
}

void glLoadMatrixd(const GLdouble *m)
{
    GLfloat f[16];
    int i;

    DEBUG_GLCMDS(("LoadMatrixd: %04x\n", gCCPriv->MatrixMode));

    for (i = 0; i < 16; i++) f[i] = m[i];
    gammaSetMatrix(f);
    gammaLoadHWMatrix();
}

void glLoadMatrixf(const GLfloat *m)
{
    DEBUG_GLCMDS(("LoadMatrixf: %04x\n", gCCPriv->MatrixMode));

    gammaSetMatrix((GLfloat *)m);
    gammaLoadHWMatrix();
}

void glLoadName(GLuint name)
{
    DEBUG_GLCMDS(("LoadName: %d\n", (unsigned int)name));
}

void glLogicOp(GLenum opcode)
{
    DEBUG_GLCMDS(("LogicOp: %04x\n", (int)opcode));
}

void glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *pnts)
{
    DEBUG_GLCMDS(("Map1d: %04x %f %f %d %d\n", (int)target, u1, u2,
		  (int)stride, (int)order));
}

void glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *pnts)
{
    DEBUG_GLCMDS(("Map1f: %04x %f %f %d %d\n", (int)target, u1, u2,
		  (int)stride, (int)order));
}

void glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustr, GLint uord, GLdouble v1, GLdouble v2, GLint vstr, GLint vord, const GLdouble *pnts)
{
    DEBUG_GLCMDS(("Map2d: %04x %f %f %d %d %f %f %d %d\n",
		   (int)target,
		   u1, u2, (int)ustr, (int)uord,
		   v1, v2, (int)vstr, (int)vord));
}

void glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustr, GLint uord, GLfloat v1, GLfloat v2, GLint vstr, GLint vord, const GLfloat *pnts)
{
    DEBUG_GLCMDS(("Map2f: %04x %f %f %d %d %f %f %d %d\n",
		   (int)target,
		   u1, u2, (int)ustr, (int)uord,
		   v1, v2, (int)vstr, (int)vord));
}

void glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
    DEBUG_GLCMDS(("MapGrid1d: %d %f %f\n", (int)un, u1, u2));
}

void glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
    DEBUG_GLCMDS(("MapGrid1f: %d %f %f\n", (int)un, u1, u2));
}

void glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    DEBUG_GLCMDS(("MapGrid2d: %d %f %f %d %f %f\n",
		  (int)un, u1, u2,
		  (int)vn, v1, v2));
}

void glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    DEBUG_GLCMDS(("MapGrid2f: %d %f %f %d %f %f\n",
		  (int)un, u1, u2,
		  (int)vn, v1, v2));
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
    DEBUG_GLCMDS(("Materialf: %04x %04x %f\n", (int)face, (int)pname, param));
}

void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    DEBUG_GLCMDS(("Materialfv: %04x %04x %f\n",
		  (int)face, (int)pname, *params));
}

void glMateriali(GLenum face, GLenum pname, GLint param)
{
    DEBUG_GLCMDS(("Materiali: %04x %04x %d\n",
		  (int)face, (int)pname, (int)param));
}

void glMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
    DEBUG_GLCMDS(("Materialiv: %04x %04x %d\n",
		  (int)face, (int)pname, (int)*params));
}

void glMatrixMode(GLenum mode)
{
    DEBUG_GLCMDS(("MatrixMode: %04x\n", (int)mode));

    switch (mode) {
    case GL_TEXTURE:
	/* Eanble the Texture transform */
	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, TransformModeOr, 0x00000010);
    case GL_MODELVIEW:
    case GL_PROJECTION:
        gCCPriv->MatrixMode = mode;
	break;
    default:
	/* ERROR!!! */
	break;
    }

}

void glMultMatrixd(const GLdouble *m)
{
    GLfloat f[16];
    int i;

    DEBUG_GLCMDS(("MatrixMultd\n"));

    for (i = 0; i < 16; i++) f[i] = m[i];
    gammaMultMatrix(f);
    gammaLoadHWMatrix();
}

void glMultMatrixf(const GLfloat *m)
{
    DEBUG_GLCMDS(("MatrixMultf\n"));

    gammaMultMatrix((GLfloat *)m);
    gammaLoadHWMatrix();
}

void glNewList(GLuint list, GLenum mode)
{
    DEBUG_GLCMDS(("NewList: %d %04x\n", (unsigned int)list, (int)mode));
}

void glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
    DEBUG_GLCMDS(("Normal3b: %d %d %d\n", nx, ny, nz));
}

void glNormal3bv(const GLbyte *v)
{
    DEBUG_GLCMDS(("Normal3bv: %d %d %d\n", v[0], v[1], v[2]));
}

void glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
    DEBUG_GLCMDS(("Normal3d: %f %f %f\n", nx, ny, nz));
}

void glNormal3dv(const GLdouble *v)
{
    DEBUG_GLCMDS(("Normal3dv: %f %f %f\n", v[0], v[1], v[2]));
}

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    DEBUG_GLCMDS(("Normal3f: %f %f %f\n", nx, ny, nz));
}

void glNormal3fv(const GLfloat *v)
{
    DEBUG_GLCMDS(("Normal3fv: %f %f %f\n", v[0], v[1], v[2]));
}

void glNormal3i(GLint nx, GLint ny, GLint nz)
{
    DEBUG_GLCMDS(("Normal3i: %d %d %d\n", (int)nx, (int)ny, (int)nz));
}

void glNormal3iv(const GLint *v)
{
    DEBUG_GLCMDS(("Normal3iv: %d %d %d\n", (int)v[0], (int)v[1], (int)v[2]));
}

void glNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
    DEBUG_GLCMDS(("Normal3s: %d %d %d\n", nx, ny, nz));
}

void glNormal3sv(const GLshort *v)
{
    DEBUG_GLCMDS(("Normal3sv: %d %d %d\n", v[0], v[1], v[2]));
}

void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    DEBUG_GLCMDS(("NormalPointer: %04x %d\n", (int)type, (int)stride));
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
#define ORTHO_X() ((GLfloat)( 2.0/(right-left)))
#define ORTHO_Y() ((GLfloat)( 2.0/(top-bottom)))
#define ORTHO_Z() ((GLfloat)(-2.0/(zFar-zNear)))

#define ORTHO_TX() ((GLfloat)(-((right+left)/(right-left))))
#define ORTHO_TY() ((GLfloat)(-((top+bottom)/(top-bottom))))
#define ORTHO_TZ() ((GLfloat)(-((zFar+zNear)/(zFar-zNear))))

    GLfloat m[16];
    int i;

    DEBUG_GLCMDS(("Ortho: %f %f %f %f %f %f\n",
		  left, right, bottom, top, zNear, zFar));

    for (i = 0; i < 16; i++) m[i] = 0.0;

    m[0]  = ORTHO_X();
    m[5]  = ORTHO_Y();
    m[10] = ORTHO_Z();
    m[12] = ORTHO_TX();
    m[13] = ORTHO_TY();
    m[14] = ORTHO_TZ();
    m[15] = 1.0;

    gammaMultMatrix(m);
    gammaLoadHWMatrix();
}

void glPassThrough(GLfloat token)
{
    DEBUG_GLCMDS(("PassThrough: %f\n", token));
}

void glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values)
{
    DEBUG_GLCMDS(("PixelMapfv: %04x %d\n", (int)map, (int)mapsize));
}

void glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values)
{
    DEBUG_GLCMDS(("PixelMapiv: %04x %d\n", (int)map, (int)mapsize));
}

void glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values)
{
    DEBUG_GLCMDS(("PixelMapusv: %04x %d\n", (int)map, (int)mapsize));
}

void glPixelStoref(GLenum pname, GLfloat param)
{
    DEBUG_GLCMDS(("PixelStoref: %04x %f\n", (int)pname, param));
}

void glPixelStorei(GLenum pname, GLint param)
{
    DEBUG_GLCMDS(("PixelStorei: %04x %d\n", (int)pname, (int)param));
}

void glPixelTransferf(GLenum pname, GLfloat param)
{
    DEBUG_GLCMDS(("PixelTransferf: %04x %f\n", (int)pname, param));
}

void glPixelTransferi(GLenum pname, GLint param)
{
    DEBUG_GLCMDS(("PixelTransferi: %04x %d\n", (int)pname, (int)param));
}

void glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    DEBUG_GLCMDS(("PixelZoom: %f %f\n", xfactor, yfactor));
}

void glPointSize(GLfloat size)
{
    unsigned char s = size;

    DEBUG_GLCMDS(("PointSize: %f\n", size));

    /* NOT_DONE: Needs to handle AAPoints also */
    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
    WRITE(gCCPriv->buf, PointSize, s);
}

void glPolygonMode(GLenum face, GLenum mode)
{
    DEBUG_GLCMDS(("PolygonMode: %04x %04x\n", (int)face, (int)mode));
}

void glPolygonOffset(GLfloat factor, GLfloat units)
{
    DEBUG_GLCMDS(("PolygonOffset: %f %f\n", factor, units));
}

void glPolygonStipple(const GLubyte *mask)
{
    DEBUG_GLCMDS(("PolygonStipple: \n"));
}

void glPopAttrib(void)
{
    DEBUG_GLCMDS(("PopAttrib\n"));
}

void glPopClientAttrib(void)
{
    DEBUG_GLCMDS(("PopClientAttrib\n"));
}

void glPopMatrix(void)
{
    DEBUG_GLCMDS(("PopMatrix: %04x\n", gCCPriv->MatrixMode));

    switch (gCCPriv->MatrixMode) {
    case GL_TEXTURE:
	if (gCCPriv->TextureCount == 0) {
	    /* ERROR!!! */
	} else {
	    gCCPriv->TextureCount--;
	    memcpy(gCCPriv->Texture,
		   &gCCPriv->TextureStack[gCCPriv->TextureCount*16],
		   16*sizeof(*gCCPriv->Texture));
	    gammaLoadHWMatrix();
	}
	break;
    case GL_MODELVIEW:
	if (gCCPriv->ModelViewCount == 0) {
	    /* ERROR!!! */
	} else {
	    gCCPriv->ModelViewCount--;
	    memcpy(gCCPriv->ModelView,
		   &gCCPriv->ModelViewStack[gCCPriv->ModelViewCount*16],
		   16*sizeof(*gCCPriv->ModelView));
	    gammaLoadHWMatrix();
	}
	break;
    case GL_PROJECTION:
	if (gCCPriv->ProjCount == 0) {
	    /* ERROR!!! */
	} else {
	    gCCPriv->ProjCount--;
	    memcpy(gCCPriv->Proj,
		   &gCCPriv->ProjStack[gCCPriv->ProjCount*16],
		   16*sizeof(*gCCPriv->Proj));
	    gammaLoadHWMatrix();
	}
	break;
    default:
	/* ERROR!!! */
	break;
    }
}

void glPopName(void)
{
    DEBUG_GLCMDS(("PopName\n"));
}

void glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
    DEBUG_GLCMDS(("PrioritizeTextures: %d\n", (int)n));
}

void glPushAttrib(GLbitfield mask)
{
    DEBUG_GLCMDS(("PushAttrib: %04x\n", (int)mask));
}

void glPushClientAttrib(GLuint mask)
{
    DEBUG_GLCMDS(("PushClientAttrib: %04x\n", (unsigned int)mask));
}

void glPushMatrix(void)
{
    DEBUG_GLCMDS(("PushMatrix: %04x\n", gCCPriv->MatrixMode));

    switch (gCCPriv->MatrixMode) {
    case GL_TEXTURE:
	if (gCCPriv->TextureCount >= MAX_TEXTURE_STACK-1) {
	    /* ERROR!!! */
	} else {
	    memcpy(&gCCPriv->TextureStack[gCCPriv->TextureCount*16],
		   gCCPriv->Texture,
		   16*sizeof(*gCCPriv->Texture));
	    gCCPriv->TextureCount++;
	}
	break;
    case GL_MODELVIEW:
	if (gCCPriv->ModelViewCount >= MAX_MODELVIEW_STACK-1) {
	    /* ERROR!!! */
	} else {
	    memcpy(&gCCPriv->ModelViewStack[gCCPriv->ModelViewCount*16],
		   gCCPriv->ModelView,
		   16*sizeof(*gCCPriv->ModelView));
	    gCCPriv->ModelViewCount++;
	}
	break;
    case GL_PROJECTION:
	if (gCCPriv->ProjCount >= MAX_PROJECTION_STACK-1) {
	    /* ERROR!!! */
	} else {
	    memcpy(&gCCPriv->ProjStack[gCCPriv->ProjCount*16],
		   gCCPriv->Proj,
		   16*sizeof(*gCCPriv->Proj));
	    gCCPriv->ProjCount++;
	}
	break;
    default:
	/* ERROR!!! */
	break;
    }
}

void glPushName(GLuint name)
{
    DEBUG_GLCMDS(("PushName: %d\n", (int)name));
}

void glRasterPos2d(GLdouble x, GLdouble y)
{
}

void glRasterPos2dv(const GLdouble *v)
{
}

void glRasterPos2f(GLfloat x, GLfloat y)
{
}

void glRasterPos2fv(const GLfloat *v)
{
}

void glRasterPos2i(GLint x, GLint y)
{
}

void glRasterPos2iv(const GLint *v)
{
}

void glRasterPos2s(GLshort x, GLshort y)
{
}

void glRasterPos2sv(const GLshort *v)
{
}

void glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
}

void glRasterPos3dv(const GLdouble *v)
{
}

void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
}

void glRasterPos3fv(const GLfloat *v)
{
}

void glRasterPos3i(GLint x, GLint y, GLint z)
{
}

void glRasterPos3iv(const GLint *v)
{
}

void glRasterPos3s(GLshort x, GLshort y, GLshort z)
{
}

void glRasterPos3sv(const GLshort *v)
{
}

void glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
}

void glRasterPos4dv(const GLdouble *v)
{
}

void glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
}

void glRasterPos4fv(const GLfloat *v)
{
}

void glRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
}

void glRasterPos4iv(const GLint *v)
{
}

void glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
}

void glRasterPos4sv(const GLshort *v)
{
}

void glReadBuffer(GLenum mode)
{
    DEBUG_GLCMDS(("ReadBuffer: %04x\n", (int)mode));
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
    DEBUG_GLCMDS(("ReadPixels: %d %d %d %d %04x %04x\n", (int)x, (int)y,
		  (int)width, (int)height, (int)format, (int)type));
}

void glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    DEBUG_GLCMDS(("Rectd: %f %f %f %f\n", x1, y1, x2, y2));
}

void glRectdv(const GLdouble *v1, const GLdouble *v2)
{
    DEBUG_GLCMDS(("Rectdv: %f %f %f %f\n", v1[0], v1[1], v2[0], v2[1]));
}

void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    DEBUG_GLCMDS(("Rectf: %f %f %f %f\n", x1, y1, x2, y2));
}

void glRectfv(const GLfloat *v1, const GLfloat *v2)
{
    DEBUG_GLCMDS(("Rectfv: %f %f %f %f\n", v1[0], v1[1], v2[0], v2[1]));
}

void glRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
    DEBUG_GLCMDS(("Recti: %d %d %d %d\n", (int)x1, (int)y1, (int)x2, (int)y2));
}

void glRectiv(const GLint *v1, const GLint *v2)
{
    DEBUG_GLCMDS(("Rectiv: %d %d %d %d\n",
		  (int)v1[0], (int)v1[1], (int)v2[0], (int)v2[1]));
}

void glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    DEBUG_GLCMDS(("Rects: %d %d %d %d\n", x1, y1, x2, y2));
}

void glRectsv(const GLshort *v1, const GLshort *v2)
{
    DEBUG_GLCMDS(("Rectsv: %d %d %d %d\n", v1[0], v1[1], v2[0], v2[1]));
}

GLint glRenderMode(GLenum mode)
{
    DEBUG_GLCMDS(("RenderMode: %04x\n", (int)mode));

    return GL_TRUE;
}

void glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    GLfloat m[16];
    GLfloat l, c, s;

    DEBUG_GLCMDS(("Rotated: %f %f %f %f\n", angle, x, y, z));

    /* Normalize <x,y,z> */
    l = sqrt(x*x + y*y + z*z);
    x /= l;
    y /= l;
    z /= l;

    c = cos(angle * M_PI/180.0);
    s = sin(angle * M_PI/180.0);

    m[0]  = x * x * (1 - c) + c;
    m[1]  = y * x * (1 - c) + z * s;
    m[2]  = x * z * (1 - c) - y * s;
    m[3]  = 0.0;
    m[4]  = x * y * (1 - c) - z * s;
    m[5]  = y * y * (1 - c) + c;
    m[6]  = y * z * (1 - c) + x * s;
    m[7]  = 0.0;
    m[8]  = x * z * (1 - c) + y * s;
    m[9]  = y * z * (1 - c) - x * s;
    m[10] = z * z * (1 - c) + c;
    m[11] = 0.0;
    m[12] = 0.0;
    m[13] = 0.0;
    m[14] = 0.0;
    m[15] = 1.0;

    gammaMultMatrix(m);
    gammaLoadHWMatrix();
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    GLfloat m[16];
    GLfloat l, c, s;

    DEBUG_GLCMDS(("Rotatef: %f %f %f %f\n", angle, x, y, z));

    /* Normalize <x,y,z> */
    l = sqrt(x*x + y*y + z*z);
    x /= l;
    y /= l;
    z /= l;

    c = cos((double)angle * M_PI/180.0);
    s = sin((double)angle * M_PI/180.0);

    m[0]  = x * x * (1 - c) + c;
    m[1]  = y * x * (1 - c) + z * s;
    m[2]  = x * z * (1 - c) - y * s;
    m[3]  = 0.0;
    m[4]  = x * y * (1 - c) - z * s;
    m[5]  = y * y * (1 - c) + c;
    m[6]  = y * z * (1 - c) + x * s;
    m[7]  = 0.0;
    m[8]  = x * z * (1 - c) + y * s;
    m[9]  = y * z * (1 - c) - x * s;
    m[10] = z * z * (1 - c) + c;
    m[11] = 0.0;
    m[12] = 0.0;
    m[13] = 0.0;
    m[14] = 0.0;
    m[15] = 1.0;

    gammaMultMatrix(m);
    gammaLoadHWMatrix();
}

void glScaled(GLdouble x, GLdouble y, GLdouble z)
{
    GLfloat m[16];
    int i;

    DEBUG_GLCMDS(("scaled: %f %f %f\n", x, y, z));

    for (i = 0; i < 16; i++) m[i] = 0.0;

    m[0]  = x;
    m[5]  = y;
    m[10] = z;
    m[15] = 1.0;

    gammaMultMatrix(m);
    gammaLoadHWMatrix();
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    GLfloat m[16];
    int i;

    DEBUG_GLCMDS(("scalef: %f %f %f\n", x, y, z));

    for (i = 0; i < 16; i++) m[i] = 0.0;

    m[0]  = x;
    m[5]  = y;
    m[10] = z;
    m[15] = 1.0;

    gammaMultMatrix(m);
    gammaLoadHWMatrix();
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    DEBUG_GLCMDS(("Scissor: %d %d %d %d\n",
		  (int)x, (int)y, (int)width, (int)height));
}

void glSelectBuffer(GLsizei numnames, GLuint *buffer)
{
    DEBUG_GLCMDS(("SelectBuffer: %d\n", (int)numnames));
}

void glShadeModel(GLenum mode)
{
    DEBUG_GLCMDS(("ShadeModel: %04x\n", (int)mode));

    gCCPriv->GeometryMode &= ~GM_ShadingMask;
    gCCPriv->ColorDDAMode &= ~ColorDDAShadingMask;

    switch (mode) {
    case GL_FLAT:
	gCCPriv->ColorDDAMode |= ColorDDAFlat;
	gCCPriv->GeometryMode |= GM_FlatShading;
	break;
    case GL_SMOOTH:
	gCCPriv->ColorDDAMode |= ColorDDAGouraud;
	gCCPriv->GeometryMode |= GM_GouraudShading;
	break;
    default:
	/* ERROR!!! */
	break;
    }

    CHECK_DMA_BUFFER(gCC, gCCPriv, 2);
    WRITE(gCCPriv->buf, GeometryMode, gCCPriv->GeometryMode);
    WRITE(gCCPriv->buf, ColorDDAMode, gCCPriv->ColorDDAMode);
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    DEBUG_GLCMDS(("StencilFunc: %04x %d %d\n",
		  (int)func, (int)ref, (unsigned int)mask));
}

void glStencilMask(GLuint mask)
{
    DEBUG_GLCMDS(("StencilMask: %d\n", (unsigned int)mask));
}

void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    DEBUG_GLCMDS(("StencilOp: %04x %04x %04x\n",
		  (int)fail, (int)zfail, (int)zpass));
}

void glTexCoord1d(GLdouble s)
{
    DEBUG_GLCMDS(("TexCoord1d: %f\n", s));
}

void glTexCoord1dv(const GLdouble *v)
{
    DEBUG_GLCMDS(("TexCoord1dv: %f\n", *v));
}

void glTexCoord1f(GLfloat s)
{
    DEBUG_GLCMDS(("TexCoord1f: %f\n", s));
}

void glTexCoord1fv(const GLfloat *v)
{
    DEBUG_GLCMDS(("TexCoord1fv: %f\n", *v));
}

void glTexCoord1i(GLint s)
{
    DEBUG_GLCMDS(("TexCoord1i: %d\n", (int)s));
}

void glTexCoord1iv(const GLint *v)
{
    DEBUG_GLCMDS(("TexCoord1iv: %d\n", (int)*v));
}

void glTexCoord1s(GLshort s)
{
    DEBUG_GLCMDS(("TexCoord1s: %d\n", s));
}

void glTexCoord1sv(const GLshort *v)
{
    DEBUG_GLCMDS(("TexCoord1sv: %d\n", *v));
}

void glTexCoord2d(GLdouble s, GLdouble t)
{
    DEBUG_GLCMDS(("TexCoord2d: %f %f\n", s, t));
}

void glTexCoord2dv(const GLdouble *v)
{
    DEBUG_GLCMDS(("TexCoord2dv: %f %f\n", v[0], v[1]));
}

void glTexCoord2f(GLfloat s, GLfloat t)
{
    DEBUG_GLCMDS(("TexCoord2f: %f %f\n", s, t));

    CHECK_DMA_BUFFER(gCC, gCCPriv, 2);
    WRITEF(gCCPriv->buf, Tt2, t);
    WRITEF(gCCPriv->buf, Ts2, s);
}

void glTexCoord2fv(const GLfloat *v)
{
    DEBUG_GLCMDS(("TexCoord2fv: %f %f\n", v[0], v[1]));
}

void glTexCoord2i(GLint s, GLint t)
{
    DEBUG_GLCMDS(("TexCoord2i: %d %d\n", (int)s, (int)t));
}

void glTexCoord2iv(const GLint *v)
{
    DEBUG_GLCMDS(("TexCoord2iv: %d %d\n", (int)v[0], (int)v[1]));
}

void glTexCoord2s(GLshort s, GLshort t)
{
    DEBUG_GLCMDS(("TexCoord2s: %d %d\n", s, t));
}

void glTexCoord2sv(const GLshort *v)
{
    DEBUG_GLCMDS(("TexCoord2sv: %d %d\n", v[0], v[1]));
}

void glTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
    DEBUG_GLCMDS(("TexCoord3d: %f %f %f\n", s, t, r));
}

void glTexCoord3dv(const GLdouble *v)
{
    DEBUG_GLCMDS(("TexCoord3dv: %f %f %f\n", v[0], v[1], v[2]));
}

void glTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
    DEBUG_GLCMDS(("TexCoord3f: %f %f %f\n", s, t, r));
}

void glTexCoord3fv(const GLfloat *v)
{
    DEBUG_GLCMDS(("TexCoord3fv: %f %f %f\n", v[0], v[1], v[2]));
}

void glTexCoord3i(GLint s, GLint t, GLint r)
{
    DEBUG_GLCMDS(("TexCoord3i: %d %d %d\n", (int)s, (int)t, (int)r));
}

void glTexCoord3iv(const GLint *v)
{
    DEBUG_GLCMDS(("TexCoord3iv: %d %d %d\n", (int)v[0], (int)v[1], (int)v[2]));
}

void glTexCoord3s(GLshort s, GLshort t, GLshort r)
{
    DEBUG_GLCMDS(("TexCoord3s: %d %d %d\n", s, t, r));
}

void glTexCoord3sv(const GLshort *v)
{
    DEBUG_GLCMDS(("TexCoord3sv: %d %d %d\n", v[0], v[1], v[2]));
}

void glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    DEBUG_GLCMDS(("TexCoord4d: %f %f %f %f\n", s, t, r, q));
}

void glTexCoord4dv(const GLdouble *v)
{
    DEBUG_GLCMDS(("TexCoord4dv: %f %f %f %f\n", v[0], v[1], v[2], v[3]));
}

void glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    DEBUG_GLCMDS(("TexCoord4f: %f %f %f %f\n", s, t, r, q));
}

void glTexCoord4fv(const GLfloat *v)
{
    DEBUG_GLCMDS(("TexCoord4fv: %f %f %f %f\n", v[0], v[1], v[2], v[3]));
}

void glTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
    DEBUG_GLCMDS(("TexCoord4i: %d %d %d %d\n", (int)s, (int)t, (int)r, (int)q));
}

void glTexCoord4iv(const GLint *v)
{
    DEBUG_GLCMDS(("TexCoord4iv: %d %d %d %d\n",
		  (int)v[0], (int)v[1], (int)v[2], (int)v[3]));
}

void glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
    DEBUG_GLCMDS(("TexCoord4s: %d %d %d %d\n", s, t, r, q));
}

void glTexCoord4sv(const GLshort *v)
{
    DEBUG_GLCMDS(("TexCoord4sv: %d %d %d %d\n", v[0], v[1], v[2], v[3]));
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    DEBUG_GLCMDS(("TexCoordPointer: %d %04x %d\n",
		  (int)size, (int)type, (int)stride));
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    DEBUG_GLCMDS(("TexEnvf: %04x %04x %f\n", (int)target, (int)pname, param));

    if (target != GL_TEXTURE_ENV || pname != GL_TEXTURE_ENV_MODE) {
	/* ERROR !! */
    }

    gCCPriv->curTexObj->TextureColorMode &= ~TCM_ApplicationMask;

    switch ((int)param) {
    case GL_MODULATE:
	gCCPriv->curTexObj->TextureColorMode |= TCM_Modulate;
	break;
    case GL_DECAL:
	gCCPriv->curTexObj->TextureColorMode |= TCM_Decal;
	break;
    case GL_BLEND:
	gCCPriv->curTexObj->TextureColorMode |= TCM_Blend;
	break;
    case GL_REPLACE:
	gCCPriv->curTexObj->TextureColorMode |= TCM_Replace;
	break;
    default:
	break;
    }

    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
    WRITE(gCCPriv->buf, TextureColorMode,
	  gCCPriv->curTexObj->TextureColorMode);
}

void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
    DEBUG_GLCMDS(("TexEnvfv: %04x %04x %f\n",
		  (int)target, (int)pname, *params));
}

void glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    DEBUG_GLCMDS(("TexEnvi: %04x %04x %d\n",
		  (int)target, (int)pname, (int)param));
}

void glTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
    DEBUG_GLCMDS(("TexEnviv: %04x %04x %d\n",
		  (int)target, (int)pname, (int)*params));
}

void glTexGend(GLenum coord, GLenum pname, GLdouble param)
{
    DEBUG_GLCMDS(("TexGend: %04x %04x %f\n", (int)coord, (int)pname, param));
}

void glTexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
    DEBUG_GLCMDS(("TexGendv: %04x %04x %f\n", (int)coord, (int)pname, *params));
}

void glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    DEBUG_GLCMDS(("TexGenf: %04x %04x %f\n", (int)coord, (int)pname, param));
}

void glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
    DEBUG_GLCMDS(("TexGenfv: %04x %04x %f\n", (int)coord, (int)pname, *params));
}

void glTexGeni(GLenum coord, GLenum pname, GLint param)
{
    DEBUG_GLCMDS(("TexGeni: %04x %04x %d\n",
		  (int)coord, (int)pname, (int)param));
}

void glTexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
    DEBUG_GLCMDS(("TexGeniv: %04x %04x %d\n",
		  (int)coord, (int)pname, (int)*params));
}

void glTexImage1D(GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *image)
{
    DEBUG_GLCMDS(("TexImage1D: %04x %d %d %d %d %04x %04x\n",
		  (int)target, (int)level, (int)components,
		  (int)width, (int)border, (int)format, (int)type));
}

void glTexImage2D(GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *image)
{
    unsigned long addrs[MIPMAP_LEVELS];
    int l2w, l2h, l2d;
    gammaTexObj *t;
    int i;

    DEBUG_GLCMDS(("TexImage2D: %04x %d %d %d %d %d %04x %04x\n",
		  (int)target, (int)level, (int)components,
		  (int)width, (int)height, (int)border,
		  (int)format, (int)type));

    if (target == GL_TEXTURE_1D) {
	/* NOT_DONE */
    } else if (target == GL_TEXTURE_2D) {
	/* NOT_DONE: The follow are not currently supported... */
	if (border != 0 || format != GL_RGBA || type != GL_UNSIGNED_BYTE ||
	    (components != 3 && components != 4)) {
	    return;
	}

	if (width > 2048 || height > 2048) return; /* ERROR !! */
	if (level < 0 || level > 11) return; /* ERROR !! */

	/* Calculate the log2width, log2height and log2depth */
	CALC_LOG2(l2w, width);
	CALC_LOG2(l2h, height);
	l2d = 5;

	t = gCCPriv->curTexObj2D;

	/* Set the texture params for level 0 only */
	if (level == 0) {
	    t->TextureAddressMode &= ~(TAM_WidthMask |
				       TAM_HeightMask);
	    t->TextureAddressMode |= (l2w << 9);
	    t->TextureAddressMode |= (l2h << 13);

	    /* NOT_DONE: Support patch mode */
	    t->TextureReadMode &= ~(TRM_WidthMask |
				    TRM_HeightMask |
				    TRM_DepthMask |
				    TRM_Border |
				    TRM_Patch);
	    t->TextureReadMode |= (l2w << 1);
	    t->TextureReadMode |= (l2h << 5);
	    t->TextureReadMode |= (l2d << 9);

	    t->TextureColorMode &= ~(TCM_BaseFormatMask);

	    switch (components) {
	    case 3:
		t->TextureColorMode |= TCM_BaseFormat_RGB;
		break;
	    case 4:
		t->TextureColorMode |= TCM_BaseFormat_RGBA;
		break;
	    }

	    t->TextureFormat = (TF_LittleEndian |
				TF_ColorOrder_BGR |
				TF_Compnents_4 |
				TF_OutputFmt_Texel);
	}

	/* Remove the old image */
	if (t->image[level])
	    driTMMDeleteImage(gCCPriv->tmm, t->image[level]);

	/* Insert the new image */
	t->image[level] = driTMMInsertImage(gCCPriv->tmm,
					    width, height, 1<<l2d,
					    image, NULL);
	if (!t->image[level]) {
	    /* NOT_DONE: Handle error */
	}

	/* Make the new image resident (and all of the other mipmaps) */
	if (!driTMMMakeImagesResident(gCCPriv->tmm, MIPMAP_LEVELS,
				      t->image, addrs)) {
	    /* NOT_DONE: Handle error */
	}

	for (i = 0; i < MIPMAP_LEVELS; i++)
	    t->TextureBaseAddr[i] = addrs[i] << 5;

	CHECK_DMA_BUFFER(gCC, gCCPriv, 4);
	WRITE(gCCPriv->buf, TextureAddressMode,
	      gCCPriv->curTexObj2D->TextureAddressMode);
	WRITE(gCCPriv->buf, TextureReadMode,
	      gCCPriv->curTexObj2D->TextureReadMode);
	WRITE(gCCPriv->buf, TextureColorMode,
	      gCCPriv->curTexObj2D->TextureColorMode);
	WRITE(gCCPriv->buf, TextureFormat,
	      gCCPriv->curTexObj2D->TextureFormat);

	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	switch (level) {
	case 0:
	    WRITE(gCCPriv->buf, TxBaseAddr0,
		  gCCPriv->curTexObj2D->TextureBaseAddr[0]);
	    break;
	case 1:
	    WRITE(gCCPriv->buf, TxBaseAddr1,
		  gCCPriv->curTexObj2D->TextureBaseAddr[1]);
	    break;
	case 2:
	    WRITE(gCCPriv->buf, TxBaseAddr2,
		  gCCPriv->curTexObj2D->TextureBaseAddr[2]);
	    break;
	case 3:
	    WRITE(gCCPriv->buf, TxBaseAddr3,
		  gCCPriv->curTexObj2D->TextureBaseAddr[3]);
	    break;
	case 4:
	    WRITE(gCCPriv->buf, TxBaseAddr4,
		  gCCPriv->curTexObj2D->TextureBaseAddr[4]);
	    break;
	case 5:
	    WRITE(gCCPriv->buf, TxBaseAddr5,
		  gCCPriv->curTexObj2D->TextureBaseAddr[5]);
	    break;
	case 6:
	    WRITE(gCCPriv->buf, TxBaseAddr6,
		  gCCPriv->curTexObj2D->TextureBaseAddr[6]);
	    break;
	case 7:
	    WRITE(gCCPriv->buf, TxBaseAddr7,
		  gCCPriv->curTexObj2D->TextureBaseAddr[7]);
	    break;
	case 8:
	    WRITE(gCCPriv->buf, TxBaseAddr8,
		  gCCPriv->curTexObj2D->TextureBaseAddr[8]);
	    break;
	case 9:
	    WRITE(gCCPriv->buf, TxBaseAddr9,
		  gCCPriv->curTexObj2D->TextureBaseAddr[9]);
	    break;
	case 10:
	    WRITE(gCCPriv->buf, TxBaseAddr10,
		  gCCPriv->curTexObj2D->TextureBaseAddr[10]);
	    break;
	case 11:
	    WRITE(gCCPriv->buf, TxBaseAddr11,
		  gCCPriv->curTexObj2D->TextureBaseAddr[11]);
	    break;
	}
    } else {
	/* ERROR !! */
    }
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    DEBUG_GLCMDS(("TexParameterf: %04x %04x %f\n",
		  (int)target, (int)pname, param));

    if (target == GL_TEXTURE_1D) {
	/* NOT_DONE */
    } else if (target == GL_TEXTURE_2D) {
	switch (pname) {
	case GL_TEXTURE_MAG_FILTER:
	    gCCPriv->curTexObj2D->TextureReadMode &= ~TRM_Mag_Mask;
	    switch ((int)param) {
	    case GL_NEAREST:
		gCCPriv->curTexObj2D->TextureReadMode |=
		    TRM_Mag_Nearest;
		break;
	    case GL_LINEAR:
		gCCPriv->curTexObj2D->TextureReadMode |=
		    TRM_Mag_Linear;
		break;
	    default:
		break;
	    }

	    CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	    WRITE(gCCPriv->buf, TextureReadMode,
		  gCCPriv->curTexObj2D->TextureReadMode);
	    break;
	case GL_TEXTURE_MIN_FILTER:
	    gCCPriv->curTexObj2D->TextureReadMode    &= ~TRM_Min_Mask;
	    gCCPriv->curTexObj2D->TextureReadMode    |= TRM_MipMapEnable;
	    gCCPriv->curTexObj2D->TextureAddressMode |= TAM_LODEnable;
	    switch ((int)param) {
	    case GL_NEAREST:
		gCCPriv->curTexObj2D->TextureReadMode |=
		    TRM_Min_Nearest;
		gCCPriv->curTexObj2D->TextureReadMode    &= ~TRM_MipMapEnable;
		gCCPriv->curTexObj2D->TextureAddressMode &= ~TAM_LODEnable;
		break;
	    case GL_LINEAR:
		gCCPriv->curTexObj2D->TextureReadMode |=
		    TRM_Min_Linear;
		gCCPriv->curTexObj2D->TextureReadMode    &= ~TRM_MipMapEnable;
		gCCPriv->curTexObj2D->TextureAddressMode &= ~TAM_LODEnable;
		break;
	    case GL_NEAREST_MIPMAP_NEAREST:
		gCCPriv->curTexObj2D->TextureReadMode |=
		    TRM_Min_NearestMMNearest;
		break;
	    case GL_LINEAR_MIPMAP_NEAREST:
		gCCPriv->curTexObj2D->TextureReadMode |=
		    TRM_Min_NearestMMLinear;
		break;
	    case GL_NEAREST_MIPMAP_LINEAR:
		gCCPriv->curTexObj2D->TextureReadMode |=
		    TRM_Min_LinearMMNearest;
		break;
	    case GL_LINEAR_MIPMAP_LINEAR:
		gCCPriv->curTexObj2D->TextureReadMode |=
		    TRM_Min_LinearMMLinear;
		break;
	    default:
		break;
	    }

	    CHECK_DMA_BUFFER(gCC, gCCPriv, 2);
	    WRITE(gCCPriv->buf, TextureReadMode,
		  gCCPriv->curTexObj2D->TextureReadMode);
	    WRITE(gCCPriv->buf, TextureAddressMode,
		  gCCPriv->curTexObj2D->TextureAddressMode);
	    break;
	case GL_TEXTURE_WRAP_S:
	    gCCPriv->curTexObj2D->TextureAddressMode &= ~TAM_SWrap_Mask;
	    gCCPriv->curTexObj2D->TextureReadMode    &= ~TRM_UWrap_Mask;
	    switch ((int)param) {
	    case GL_CLAMP:
		gCCPriv->curTexObj2D->TextureAddressMode |= TAM_SWrap_Clamp;
		gCCPriv->curTexObj2D->TextureReadMode    |= TRM_UWrap_Clamp;
		break;
	    case GL_REPEAT:
		gCCPriv->curTexObj2D->TextureAddressMode |= TAM_SWrap_Repeat;
		gCCPriv->curTexObj2D->TextureReadMode    |= TRM_UWrap_Repeat;
		break;
	    default:
		break;
	    }

	    CHECK_DMA_BUFFER(gCC, gCCPriv, 2);
	    WRITE(gCCPriv->buf, TextureReadMode,
		  gCCPriv->curTexObj2D->TextureReadMode);
	    WRITE(gCCPriv->buf, TextureAddressMode,
		  gCCPriv->curTexObj2D->TextureAddressMode);
	    break;
	case GL_TEXTURE_WRAP_T:
	    gCCPriv->curTexObj2D->TextureAddressMode &= ~TAM_TWrap_Mask;
	    gCCPriv->curTexObj2D->TextureReadMode    &= ~TRM_VWrap_Mask;
	    switch ((int)param) {
	    case GL_CLAMP:
		gCCPriv->curTexObj2D->TextureAddressMode |= TAM_TWrap_Clamp;
		gCCPriv->curTexObj2D->TextureReadMode    |= TRM_VWrap_Clamp;
		break;
	    case GL_REPEAT:
		gCCPriv->curTexObj2D->TextureAddressMode |= TAM_TWrap_Repeat;
		gCCPriv->curTexObj2D->TextureReadMode    |= TRM_VWrap_Repeat;
		break;
	    default:
		break;
	    }

	    CHECK_DMA_BUFFER(gCC, gCCPriv, 2);
	    WRITE(gCCPriv->buf, TextureReadMode,
		  gCCPriv->curTexObj2D->TextureReadMode);
	    WRITE(gCCPriv->buf, TextureAddressMode,
		  gCCPriv->curTexObj2D->TextureAddressMode);
	    break;
	default:
	    break;
	}
    } else {
	/* ERROR !! */
    }
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
    DEBUG_GLCMDS(("TexParameterfv: %04x %04x %f\n",
		  (int)target, (int)pname, *params));
}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    DEBUG_GLCMDS(("TexParameteri: %04x %04x %d\n",
		  (int)target, (int)pname, (int)param));
}

void glTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
    DEBUG_GLCMDS(("TexParameteriv: %04x %04x %d\n",
		  (int)target, (int)pname, (int)*params));
}

void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
    DEBUG_GLCMDS(("TexSubImage1D: %04x %d %d %d %04x %04x\n",
		  (int)target, (int)level,
		  (int)xoffset, (int)width, (int)format, (int)type));
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
    DEBUG_GLCMDS(("TexSubImage2D: %04x %d %d %d %d %d %04x %04x\n",
		  (int)target, (int)level,
		  (int)xoffset, (int)yoffset, (int)width, (int)height,
		  (int)format, (int)type));

    if (target == GL_TEXTURE_1D) {
	/* NOT_DONE */
    } else if (target == GL_TEXTURE_2D) {
	/* NOT_DONE: The follow are not currently supported... */
	if (format != GL_RGBA || type != GL_UNSIGNED_BYTE) {
	    return;
	}

	if (width > 2048 || height > 2048) return; /* ERROR !! */
	if (level < 0 || level > 11) return; /* ERROR !! */

	/*
	** NOT_DONE: This should convert the image into the internal
	** format for the image that it is replacing.
	*/

	if (!driTMMSubImage(gCCPriv->tmm, gCCPriv->curTexObj2D->image[level],
			    xoffset, yoffset, width, height, image)) {
	    /* NOT_DONE: Handle error */
	}
    } else {
	/* ERROR !! */
    }
}

void glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
    GLfloat m[16];
    int i;

    DEBUG_GLCMDS(("Translated: %f %f %f\n", x, y, z));

    for (i = 0; i < 16; i++)
	if (i % 5 == 0)
	    m[i] = 1.0;
	else
	    m[i] = 0.0;

    m[12] = x;
    m[13] = y;
    m[14] = z;

    gammaMultMatrix(m);
    gammaLoadHWMatrix();
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    GLfloat m[16];
    int i;

    DEBUG_GLCMDS(("Tranlatef: %f %f %f\n", x, y, z));

    for (i = 0; i < 16; i++)
	if (i % 5 == 0)
	    m[i] = 1.0;
	else
	    m[i] = 0.0;

    m[12] = x;
    m[13] = y;
    m[14] = z;

    gammaMultMatrix(m);
    gammaLoadHWMatrix();
}

void glVertex2d(GLdouble x, GLdouble y)
{
    DEBUG_GLCMDS(("Vertex2d: %f %f\n", x, y));
}

void glVertex2dv(const GLdouble *v)
{
    DEBUG_GLCMDS(("Vertex2dv: %f %f\n", v[0], v[1]));
}

void glVertex2f(GLfloat x, GLfloat y)
{
    DEBUG_GLCMDS(("Vertex2f: %f %f\n", x, y));

#ifdef RANDOMIZE_COLORS
    {
	GLuint c;
	c = (random() / (double)RAND_MAX) * 16777216;
	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, PackedColor4,  c);
    }
#endif

    CHECK_DMA_BUFFER(gCC, gCCPriv, 2);
    WRITEF(gCCPriv->buf, Vy,  y);
    WRITEF(gCCPriv->buf, Vx2, x);
}

void glVertex2fv(const GLfloat *v)
{
    DEBUG_GLCMDS(("Vertex2fv: %f %f\n", v[0], v[1]));
}

void glVertex2i(GLint x, GLint y)
{
    DEBUG_GLCMDS(("Vertex2i: %d %d\n", (int)x, (int)y));
}

void glVertex2iv(const GLint *v)
{
    DEBUG_GLCMDS(("Vertex2iv: %d %d\n", (int)v[0], (int)v[1]));
}

void glVertex2s(GLshort x, GLshort y)
{
    DEBUG_GLCMDS(("Vertex2s: %d %d\n", x, y));
}

void glVertex2sv(const GLshort *v)
{
    DEBUG_GLCMDS(("Vertex2sv: %d %d\n", v[0], v[1]));
}

void glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    DEBUG_GLCMDS(("Vertex3d: %f %f %f\n", x, y, z));
}

void glVertex3dv(const GLdouble *v)
{
    DEBUG_GLCMDS(("Vertex2fv: %f %f %f\n", v[0], v[1], v[2]));
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    DEBUG_GLCMDS(("Vertex3f: %f %f %f\n", x, y, z));

#ifdef RANDOMIZE_COLORS
    {
	GLuint c;
	c = (random() / (double)RAND_MAX) * 16777216;
	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, PackedColor4,  c);
    }
#endif

    CHECK_DMA_BUFFER(gCC, gCCPriv, 3);
    WRITEF(gCCPriv->buf, Vz,  z);
    WRITEF(gCCPriv->buf, Vy,  y);
    WRITEF(gCCPriv->buf, Vx3, x);
}

void glVertex3fv(const GLfloat *v)
{
    DEBUG_GLCMDS(("Vertex3fv: %f %f %f\n", v[0], v[1], v[2]));

#ifdef RANDOMIZE_COLORS
    {
	GLuint c;
	c = (random() / (double)RAND_MAX) * 16777216;
	CHECK_DMA_BUFFER(gCC, gCCPriv, 1);
	WRITE(gCCPriv->buf, PackedColor4,  c);
    }
#endif

    CHECK_DMA_BUFFER(gCC, gCCPriv, 3);
    WRITEF(gCCPriv->buf, Vz,  v[2]);
    WRITEF(gCCPriv->buf, Vy,  v[1]);
    WRITEF(gCCPriv->buf, Vx3, v[0]);
}

void glVertex3i(GLint x, GLint y, GLint z)
{
    DEBUG_GLCMDS(("Vertex3i: %d %d %d\n", (int)x, (int)y, (int)z));
}

void glVertex3iv(const GLint *v)
{
    DEBUG_GLCMDS(("Vertex3iv: %d %d %d\n", (int)v[0], (int)v[1], (int)v[2]));
}

void glVertex3s(GLshort x, GLshort y, GLshort z)
{
    DEBUG_GLCMDS(("Vertex3s: %d %d %d\n", x, y, z));
}

void glVertex3sv(const GLshort *v)
{
    DEBUG_GLCMDS(("Vertex3sv: %d %d %d\n", v[0], v[1], v[2]));
}

void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    DEBUG_GLCMDS(("Vertex4d: %f %f %f %f\n", x, y, z, w));
}

void glVertex4dv(const GLdouble *v)
{
    DEBUG_GLCMDS(("Vertex4dv: %f %f %f %f\n", v[0], v[1], v[2], v[3]));
}

void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    DEBUG_GLCMDS(("Vertex4f: %f %f %f %f\n", x, y, z, w));
}

void glVertex4fv(const GLfloat *v)
{
    DEBUG_GLCMDS(("Vertex4fv: %f %f %f %f\n", v[0], v[1], v[2], v[3]));
}

void glVertex4i(GLint x, GLint y, GLint z, GLint w)
{
    DEBUG_GLCMDS(("Vertex4i: %d %d %d %d\n", (int)x, (int)y, (int)z, (int)w));
}

void glVertex4iv(const GLint *v)
{
    DEBUG_GLCMDS(("Vertex4iv: %d %d %d %d\n",
		  (int)v[0], (int)v[1], (int)v[2], (int)v[3]));
}

void glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    DEBUG_GLCMDS(("Vertex4s: %d %d %d %d\n", x, y, z, w));
}

void glVertex4sv(const GLshort *v)
{
    DEBUG_GLCMDS(("Vertex4sv: %d %d %d %d\n", v[0], v[1], v[2], v[3]));
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    DEBUG_GLCMDS(("VertexPointer: %d %04x %d\n",
		  (int)size, (int)type, (int)stride));
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    GLfloat sx, sy, ox, oy;

    DEBUG_GLCMDS(("Viewport: %d %d %d %d\n",
		  (int)x, (int)y, (int)width, (int)height));

    gCCPriv->x = gCC->driContextPriv->driDrawablePriv->x + x;
    gCCPriv->y = gCC->driContextPriv->driScreenPriv->fbHeight -
	(gCC->driContextPriv->driDrawablePriv->y +
	 gCC->driContextPriv->driDrawablePriv->h) + y;
    gCCPriv->w = width;
    gCCPriv->h = height;

    x = gCCPriv->x;
    y = gCCPriv->y;

    sx = width/2.0f;
    sy = height/2.0f;
    ox = x + sx;
    oy = y + sy;

    CHECK_DMA_BUFFER(gCC, gCCPriv, 4);
    WRITEF(gCCPriv->buf, ViewPortOffsetX, ox);
    WRITEF(gCCPriv->buf, ViewPortOffsetY, oy);
    WRITEF(gCCPriv->buf, ViewPortScaleX,  sx);
    WRITEF(gCCPriv->buf, ViewPortScaleY,  sy);
}

#endif
