
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
 */


/*
 * This is part of the reusable GL dispather, see glapi.c for details.
 */


#include "glheader.h"
#include "glapi.h"
#include "glapinoop.h"
#include "glapitable.h"


static GLboolean WarnFlag = GL_FALSE;


void
_glapi_noop_enable_warnings(GLboolean enable)
{
   WarnFlag = enable;
}


static GLboolean
warn(void)
{
   if (WarnFlag || getenv("MESA_DEBUG") || getenv("LIBGL_DEBUG"))
      return GL_TRUE;
   else
      return GL_FALSE;
}


#define KEYWORD1 static
#define KEYWORD2
#define NAME(func)  NoOp##func

#define F stderr

#define DISPATCH(func, args, msg)			\
   if (warn()) {					\
      fprintf(stderr, "GL User Error: calling ");	\
      fprintf msg;					\
      fprintf(stderr, " without a current context\n");	\
   }

#define RETURN_DISPATCH(func, args, msg)		\
   if (warn()) {					\
      fprintf(stderr, "GL User Error: calling ");	\
      fprintf msg;					\
      fprintf(stderr, " without a current context\n");	\
   }							\
   return 0

#include "glapitemp.h"



static void Dummy(void)
{
}


struct _glapi_table __glapi_noop_table = {
   Dummy,
   NoOpAccum,
   NoOpAlphaFunc,
   NoOpBegin,
   NoOpBitmap,
   NoOpBlendFunc,
   NoOpCallList,
   NoOpCallLists,
   NoOpClear,
   NoOpClearAccum,
   NoOpClearColor,
   NoOpClearDepth,
   NoOpClearIndex,
   NoOpClearStencil,
   NoOpClipPlane,
   NoOpColor3b,
   NoOpColor3bv,
   NoOpColor3d,
   NoOpColor3dv,
   NoOpColor3f,
   NoOpColor3fv,
   NoOpColor3i,
   NoOpColor3iv,
   NoOpColor3s,
   NoOpColor3sv,
   NoOpColor3ub,
   NoOpColor3ubv,
   NoOpColor3ui,
   NoOpColor3uiv,
   NoOpColor3us,
   NoOpColor3usv,
   NoOpColor4b,
   NoOpColor4bv,
   NoOpColor4d,
   NoOpColor4dv,
   NoOpColor4f,
   NoOpColor4fv,
   NoOpColor4i,
   NoOpColor4iv,
   NoOpColor4s,
   NoOpColor4sv,
   NoOpColor4ub,
   NoOpColor4ubv,
   NoOpColor4ui,
   NoOpColor4uiv,
   NoOpColor4us,
   NoOpColor4usv,
   NoOpColorMask,
   NoOpColorMaterial,
   NoOpCopyPixels,
   NoOpCullFace,
   NoOpDeleteLists,
   NoOpDepthFunc,
   NoOpDepthMask,
   NoOpDepthRange,
   NoOpDisable,
   NoOpDrawBuffer,
   NoOpDrawPixels,
   NoOpEdgeFlag,
   NoOpEdgeFlagv,
   NoOpEnable,
   NoOpEnd,
   NoOpEndList,
   NoOpEvalCoord1d,
   NoOpEvalCoord1dv,
   NoOpEvalCoord1f,
   NoOpEvalCoord1fv,
   NoOpEvalCoord2d,
   NoOpEvalCoord2dv,
   NoOpEvalCoord2f,
   NoOpEvalCoord2fv,
   NoOpEvalMesh1,
   NoOpEvalMesh2,
   NoOpEvalPoint1,
   NoOpEvalPoint2,
   NoOpFeedbackBuffer,
   NoOpFinish,
   NoOpFlush,
   NoOpFogf,
   NoOpFogfv,
   NoOpFogi,
   NoOpFogiv,
   NoOpFrontFace,
   NoOpFrustum,
   NoOpGenLists,
   NoOpGetBooleanv,
   NoOpGetClipPlane,
   NoOpGetDoublev,
   NoOpGetError,
   NoOpGetFloatv,
   NoOpGetIntegerv,
   NoOpGetLightfv,
   NoOpGetLightiv,
   NoOpGetMapdv,
   NoOpGetMapfv,
   NoOpGetMapiv,
   NoOpGetMaterialfv,
   NoOpGetMaterialiv,
   NoOpGetPixelMapfv,
   NoOpGetPixelMapuiv,
   NoOpGetPixelMapusv,
   NoOpGetPolygonStipple,
   NoOpGetString,
   NoOpGetTexEnvfv,
   NoOpGetTexEnviv,
   NoOpGetTexGendv,
   NoOpGetTexGenfv,
   NoOpGetTexGeniv,
   NoOpGetTexImage,
   NoOpGetTexLevelParameterfv,
   NoOpGetTexLevelParameteriv,
   NoOpGetTexParameterfv,
   NoOpGetTexParameteriv,
   NoOpHint,
   NoOpIndexMask,
   NoOpIndexd,
   NoOpIndexdv,
   NoOpIndexf,
   NoOpIndexfv,
   NoOpIndexi,
   NoOpIndexiv,
   NoOpIndexs,
   NoOpIndexsv,
   NoOpInitNames,
   NoOpIsEnabled,
   NoOpIsList,
   NoOpLightModelf,
   NoOpLightModelfv,
   NoOpLightModeli,
   NoOpLightModeliv,
   NoOpLightf,
   NoOpLightfv,
   NoOpLighti,
   NoOpLightiv,
   NoOpLineStipple,
   NoOpLineWidth,
   NoOpListBase,
   NoOpLoadIdentity,
   NoOpLoadMatrixd,
   NoOpLoadMatrixf,
   NoOpLoadName,
   NoOpLogicOp,
   NoOpMap1d,
   NoOpMap1f,
   NoOpMap2d,
   NoOpMap2f,
   NoOpMapGrid1d,
   NoOpMapGrid1f,
   NoOpMapGrid2d,
   NoOpMapGrid2f,
   NoOpMaterialf,
   NoOpMaterialfv,
   NoOpMateriali,
   NoOpMaterialiv,
   NoOpMatrixMode,
   NoOpMultMatrixd,
   NoOpMultMatrixf,
   NoOpNewList,
   NoOpNormal3b,
   NoOpNormal3bv,
   NoOpNormal3d,
   NoOpNormal3dv,
   NoOpNormal3f,
   NoOpNormal3fv,
   NoOpNormal3i,
   NoOpNormal3iv,
   NoOpNormal3s,
   NoOpNormal3sv,
   NoOpOrtho,
   NoOpPassThrough,
   NoOpPixelMapfv,
   NoOpPixelMapuiv,
   NoOpPixelMapusv,
   NoOpPixelStoref,
   NoOpPixelStorei,
   NoOpPixelTransferf,
   NoOpPixelTransferi,
   NoOpPixelZoom,
   NoOpPointSize,
   NoOpPolygonMode,
   NoOpPolygonOffset,
   NoOpPolygonStipple,
   NoOpPopAttrib,
   NoOpPopMatrix,
   NoOpPopName,
   NoOpPushAttrib,
   NoOpPushMatrix,
   NoOpPushName,
   NoOpRasterPos2d,
   NoOpRasterPos2dv,
   NoOpRasterPos2f,
   NoOpRasterPos2fv,
   NoOpRasterPos2i,
   NoOpRasterPos2iv,
   NoOpRasterPos2s,
   NoOpRasterPos2sv,
   NoOpRasterPos3d,
   NoOpRasterPos3dv,
   NoOpRasterPos3f,
   NoOpRasterPos3fv,
   NoOpRasterPos3i,
   NoOpRasterPos3iv,
   NoOpRasterPos3s,
   NoOpRasterPos3sv,
   NoOpRasterPos4d,
   NoOpRasterPos4dv,
   NoOpRasterPos4f,
   NoOpRasterPos4fv,
   NoOpRasterPos4i,
   NoOpRasterPos4iv,
   NoOpRasterPos4s,
   NoOpRasterPos4sv,
   NoOpReadBuffer,
   NoOpReadPixels,
   NoOpRectd,
   NoOpRectdv,
   NoOpRectf,
   NoOpRectfv,
   NoOpRecti,
   NoOpRectiv,
   NoOpRects,
   NoOpRectsv,
   NoOpRenderMode,
   NoOpRotated,
   NoOpRotatef,
   NoOpScaled,
   NoOpScalef,
   NoOpScissor,
   NoOpSelectBuffer,
   NoOpShadeModel,
   NoOpStencilFunc,
   NoOpStencilMask,
   NoOpStencilOp,
   NoOpTexCoord1d,
   NoOpTexCoord1dv,
   NoOpTexCoord1f,
   NoOpTexCoord1fv,
   NoOpTexCoord1i,
   NoOpTexCoord1iv,
   NoOpTexCoord1s,
   NoOpTexCoord1sv,
   NoOpTexCoord2d,
   NoOpTexCoord2dv,
   NoOpTexCoord2f,
   NoOpTexCoord2fv,
   NoOpTexCoord2i,
   NoOpTexCoord2iv,
   NoOpTexCoord2s,
   NoOpTexCoord2sv,
   NoOpTexCoord3d,
   NoOpTexCoord3dv,
   NoOpTexCoord3f,
   NoOpTexCoord3fv,
   NoOpTexCoord3i,
   NoOpTexCoord3iv,
   NoOpTexCoord3s,
   NoOpTexCoord3sv,
   NoOpTexCoord4d,
   NoOpTexCoord4dv,
   NoOpTexCoord4f,
   NoOpTexCoord4fv,
   NoOpTexCoord4i,
   NoOpTexCoord4iv,
   NoOpTexCoord4s,
   NoOpTexCoord4sv,
   NoOpTexEnvf,
   NoOpTexEnvfv,
   NoOpTexEnvi,
   NoOpTexEnviv,
   NoOpTexGend,
   NoOpTexGendv,
   NoOpTexGenf,
   NoOpTexGenfv,
   NoOpTexGeni,
   NoOpTexGeniv,
   NoOpTexImage1D,
   NoOpTexImage2D,
   NoOpTexParameterf,
   NoOpTexParameterfv,
   NoOpTexParameteri,
   NoOpTexParameteriv,
   NoOpTranslated,
   NoOpTranslatef,
   NoOpVertex2d,
   NoOpVertex2dv,
   NoOpVertex2f,
   NoOpVertex2fv,
   NoOpVertex2i,
   NoOpVertex2iv,
   NoOpVertex2s,
   NoOpVertex2sv,
   NoOpVertex3d,
   NoOpVertex3dv,
   NoOpVertex3f,
   NoOpVertex3fv,
   NoOpVertex3i,
   NoOpVertex3iv,
   NoOpVertex3s,
   NoOpVertex3sv,
   NoOpVertex4d,
   NoOpVertex4dv,
   NoOpVertex4f,
   NoOpVertex4fv,
   NoOpVertex4i,
   NoOpVertex4iv,
   NoOpVertex4s,
   NoOpVertex4sv,
   NoOpViewport,

   /* GL 1.1 */
   NoOpAreTexturesResident,
   NoOpArrayElement,
   NoOpBindTexture,
   NoOpColorPointer,
   NoOpCopyTexImage1D,
   NoOpCopyTexImage2D,
   NoOpCopyTexSubImage1D,
   NoOpCopyTexSubImage2D,
   NoOpDeleteTextures,
   NoOpDisableClientState,
   NoOpDrawArrays,
   NoOpDrawElements,
   NoOpEdgeFlagPointer,
   NoOpEnableClientState,
   NoOpGenTextures,
   NoOpGetPointerv,
   NoOpIndexPointer,
   NoOpIndexub,
   NoOpIndexubv,
   NoOpInterleavedArrays,
   NoOpIsTexture,
   NoOpNormalPointer,
   NoOpPopClientAttrib,
   NoOpPrioritizeTextures,
   NoOpPushClientAttrib,
   NoOpTexCoordPointer,
   NoOpTexSubImage1D,
   NoOpTexSubImage2D,
   NoOpVertexPointer,

   /* GL 1.2 */
   NoOpCopyTexSubImage3D,
   NoOpDrawRangeElements,
   NoOpTexImage3D,
   NoOpTexSubImage3D,

   /* GL_ARB_imaging */
   NoOpBlendColor,
   NoOpBlendEquation,
   NoOpColorSubTable,
   NoOpColorTable,
   NoOpColorTableParameterfv,
   NoOpColorTableParameteriv,
   NoOpConvolutionFilter1D,
   NoOpConvolutionFilter2D,
   NoOpConvolutionParameterf,
   NoOpConvolutionParameterfv,
   NoOpConvolutionParameteri,
   NoOpConvolutionParameteriv,
   NoOpCopyColorSubTable,
   NoOpCopyColorTable,
   NoOpCopyConvolutionFilter1D,
   NoOpCopyConvolutionFilter2D,
   NoOpGetColorTable,
   NoOpGetColorTableParameterfv,
   NoOpGetColorTableParameteriv,
   NoOpGetConvolutionFilter,
   NoOpGetConvolutionParameterfv,
   NoOpGetConvolutionParameteriv,
   NoOpGetHistogram,
   NoOpGetHistogramParameterfv,
   NoOpGetHistogramParameteriv,
   NoOpGetMinmax,
   NoOpGetMinmaxParameterfv,
   NoOpGetMinmaxParameteriv,
   NoOpGetSeparableFilter,
   NoOpHistogram,
   NoOpMinmax,
   NoOpResetHistogram,
   NoOpResetMinmax,
   NoOpSeparableFilter2D,

   /* GL_ARB_multitexture */
   NoOpActiveTextureARB,
   NoOpClientActiveTextureARB,
   NoOpMultiTexCoord1dARB,
   NoOpMultiTexCoord1dvARB,
   NoOpMultiTexCoord1fARB,
   NoOpMultiTexCoord1fvARB,
   NoOpMultiTexCoord1iARB,
   NoOpMultiTexCoord1ivARB,
   NoOpMultiTexCoord1sARB,
   NoOpMultiTexCoord1svARB,
   NoOpMultiTexCoord2dARB,
   NoOpMultiTexCoord2dvARB,
   NoOpMultiTexCoord2fARB,
   NoOpMultiTexCoord2fvARB,
   NoOpMultiTexCoord2iARB,
   NoOpMultiTexCoord2ivARB,
   NoOpMultiTexCoord2sARB,
   NoOpMultiTexCoord2svARB,
   NoOpMultiTexCoord3dARB,
   NoOpMultiTexCoord3dvARB,
   NoOpMultiTexCoord3fARB,
   NoOpMultiTexCoord3fvARB,
   NoOpMultiTexCoord3iARB,
   NoOpMultiTexCoord3ivARB,
   NoOpMultiTexCoord3sARB,
   NoOpMultiTexCoord3svARB,
   NoOpMultiTexCoord4dARB,
   NoOpMultiTexCoord4dvARB,
   NoOpMultiTexCoord4fARB,
   NoOpMultiTexCoord4fvARB,
   NoOpMultiTexCoord4iARB,
   NoOpMultiTexCoord4ivARB,
   NoOpMultiTexCoord4sARB,
   NoOpMultiTexCoord4svARB,

   /*
    * Extensions
    */

   /* 2. GL_EXT_blend_color */
   NoOpBlendColorEXT,

   /* 3. GL_EXT_polygon_offset */
   NoOpPolygonOffsetEXT,

   /* 6. GL_EXT_texture3d */
   NoOpCopyTexSubImage3DEXT,
   NoOpTexImage3DEXT,
   NoOpTexSubImage3DEXT,

   /* 7. GL_SGI_texture_filter4 */
   NoOpGetTexFilterFuncSGIS,
   NoOpTexFilterFuncSGIS,

   /* 9. GL_EXT_subtexture */
   NoOpTexSubImage1DEXT,
   NoOpTexSubImage2DEXT,

   /* 10. GL_EXT_copy_texture */
   NoOpCopyTexImage1DEXT,
   NoOpCopyTexImage2DEXT,
   NoOpCopyTexSubImage1DEXT,
   NoOpCopyTexSubImage2DEXT,

   /* 11. GL_EXT_histogram */
   NoOpGetHistogramEXT,
   NoOpGetHistogramParameterfvEXT,
   NoOpGetHistogramParameterivEXT,
   NoOpGetMinmaxEXT,
   NoOpGetMinmaxParameterfvEXT,
   NoOpGetMinmaxParameterivEXT,
   NoOpHistogramEXT,
   NoOpMinmaxEXT,
   NoOpResetHistogramEXT,
   NoOpResetMinmaxEXT,

   /* 12. GL_EXT_convolution */
   NoOpConvolutionFilter1DEXT,
   NoOpConvolutionFilter2DEXT,
   NoOpConvolutionParameterfEXT,
   NoOpConvolutionParameterfvEXT,
   NoOpConvolutionParameteriEXT,
   NoOpConvolutionParameterivEXT,
   NoOpCopyConvolutionFilter1DEXT,
   NoOpCopyConvolutionFilter2DEXT,
   NoOpGetConvolutionFilterEXT,
   NoOpGetConvolutionParameterfvEXT,
   NoOpGetConvolutionParameterivEXT,
   NoOpGetSeparableFilterEXT,
   NoOpSeparableFilter2DEXT,

   /* 14. GL_SGI_color_table */
   NoOpColorTableSGI,
   NoOpColorTableParameterfvSGI,
   NoOpColorTableParameterivSGI,
   NoOpCopyColorTableSGI,
   NoOpGetColorTableSGI,
   NoOpGetColorTableParameterfvSGI,
   NoOpGetColorTableParameterivSGI,

   /* 15. GL_SGIS_pixel_texture */
   NoOpPixelTexGenParameterfSGIS,
   NoOpPixelTexGenParameteriSGIS,
   NoOpGetPixelTexGenParameterfvSGIS,
   NoOpGetPixelTexGenParameterivSGIS,

   /* 16. GL_SGIS_texture4D */
   NoOpTexImage4DSGIS,
   NoOpTexSubImage4DSGIS,

   /* 20. GL_EXT_texture_object */
   NoOpAreTexturesResidentEXT,
   NoOpBindTextureEXT,
   NoOpDeleteTexturesEXT,
   NoOpGenTexturesEXT,
   NoOpIsTextureEXT,
   NoOpPrioritizeTexturesEXT,

   /* 21. GL_SGIS_detail_texture */
   NoOpDetailTexFuncSGIS,
   NoOpGetDetailTexFuncSGIS,

   /* 22. GL_SGIS_sharpen_texture */
   NoOpGetSharpenTexFuncSGIS,
   NoOpSharpenTexFuncSGIS,

   /* 25. GL_SGIS_multisample */
   NoOpSampleMaskSGIS,
   NoOpSamplePatternSGIS,

   /* 30. GL_EXT_vertex_array */
   NoOpArrayElementEXT,
   NoOpColorPointerEXT,
   NoOpDrawArraysEXT,
   NoOpEdgeFlagPointerEXT,
   NoOpGetPointervEXT,
   NoOpIndexPointerEXT,
   NoOpNormalPointerEXT,
   NoOpTexCoordPointerEXT,
   NoOpVertexPointerEXT,

   /* 37. GL_EXT_blend_minmax */
   NoOpBlendEquationEXT,

   /* 52. GL_SGIX_sprite */
   NoOpSpriteParameterfSGIX,
   NoOpSpriteParameterfvSGIX,
   NoOpSpriteParameteriSGIX,
   NoOpSpriteParameterivSGIX,

   /* 54. GL_EXT_point_parameters */
   NoOpPointParameterfEXT,
   NoOpPointParameterfvEXT,

   /* 55. GL_SGIX_instruments */
   NoOpGetInstrumentsSGIX,
   NoOpInstrumentsBufferSGIX,
   NoOpPollInstrumentsSGIX,
   NoOpReadInstrumentsSGIX,
   NoOpStartInstrumentsSGIX,
   NoOpStopInstrumentsSGIX,

   /* 57. GL_SGIX_framezoom */
   NoOpFrameZoomSGIX,

   /* 60. GL_SGIX_reference_plane */
   NoOpReferencePlaneSGIX,

   /* 61. GL_SGIX_flush_raster */
   NoOpFlushRasterSGIX,

   /* 66. GL_HP_image_transform */
   NoOpGetImageTransformParameterfvHP,
   NoOpGetImageTransformParameterivHP,
   NoOpImageTransformParameterfHP,
   NoOpImageTransformParameterfvHP,
   NoOpImageTransformParameteriHP,
   NoOpImageTransformParameterivHP,

   /* 74. GL_EXT_color_subtable */
   NoOpColorSubTableEXT,
   NoOpCopyColorSubTableEXT,

   /* 77. GL_PGI_misc_hints */
   NoOpHintPGI,

   /* 78. GL_EXT_paletted_texture */
   NoOpColorTableEXT,
   NoOpGetColorTableEXT,
   NoOpGetColorTableParameterfvEXT,
   NoOpGetColorTableParameterivEXT,

   /* 80. GL_SGIX_list_priority */
   NoOpGetListParameterfvSGIX,
   NoOpGetListParameterivSGIX,
   NoOpListParameterfSGIX,
   NoOpListParameterfvSGIX,
   NoOpListParameteriSGIX,
   NoOpListParameterivSGIX,

   /* 94. GL_EXT_index_material */
   NoOpIndexMaterialEXT,

   /* 95. GL_EXT_index_func */
   NoOpIndexFuncEXT,

   /* 96. GL_EXT_index_array_formats - no functions */

   /* 97. GL_EXT_compiled_vertex_array */
   NoOpLockArraysEXT,
   NoOpUnlockArraysEXT,

   /* 98. GL_EXT_cull_vertex */
   NoOpCullParameterfvEXT,
   NoOpCullParameterdvEXT,

   /* 173. GL_EXT/INGR_blend_func_separate */
   NoOpBlendFuncSeparateINGR,

   /* GL_MESA_window_pos */
   NoOpWindowPos2dMESA,
   NoOpWindowPos2dvMESA,
   NoOpWindowPos2fMESA,
   NoOpWindowPos2fvMESA,
   NoOpWindowPos2iMESA,
   NoOpWindowPos2ivMESA,
   NoOpWindowPos2sMESA,
   NoOpWindowPos2svMESA,
   NoOpWindowPos3dMESA,
   NoOpWindowPos3dvMESA,
   NoOpWindowPos3fMESA,
   NoOpWindowPos3fvMESA,
   NoOpWindowPos3iMESA,
   NoOpWindowPos3ivMESA,
   NoOpWindowPos3sMESA,
   NoOpWindowPos3svMESA,
   NoOpWindowPos4dMESA,
   NoOpWindowPos4dvMESA,
   NoOpWindowPos4fMESA,
   NoOpWindowPos4fvMESA,
   NoOpWindowPos4iMESA,
   NoOpWindowPos4ivMESA,
   NoOpWindowPos4sMESA,
   NoOpWindowPos4svMESA,

   /* GL_MESA_resize_buffers */
   NoOpResizeBuffersMESA,

   /* GL_ARB_transpose_matrix */
   NoOpLoadTransposeMatrixdARB,
   NoOpLoadTransposeMatrixfARB,
   NoOpMultTransposeMatrixdARB,
   NoOpMultTransposeMatrixfARB,

};

