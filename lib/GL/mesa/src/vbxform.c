/* Id: vbxform.c,v 1.3 1999/02/26 08:52:39 martin Exp $ */

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
 * Log: vbxform.c,v $
 * Revision 1.3  1999/02/26 08:52:39  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.19  1999/02/24 22:48:08  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.18  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.17  1999/01/30 04:33:01  brianp
 * gl_texgen() now takes a stride parameter
 *
 * Revision 3.16  1998/11/17 01:52:47  brianp
 * implemented GL_NV_texgen_reflection extension (MJK)
 *
 * Revision 3.15  1998/11/08 22:36:27  brianp
 * renamed texture sets to texture units
 *
 * Revision 3.14  1998/11/03 02:40:40  brianp
 * new ctx->RenderVB function pointer
 *
 * Revision 3.13  1998/10/31 17:08:00  brianp
 * variety of multi-texture changes
 *
 * Revision 3.12  1998/10/29 03:57:11  brianp
 * misc clean-up of new vertex transformation code
 *
 * Revision 3.11  1998/10/29 02:28:13  brianp
 * incorporated Keith Whitwell's transformation optimizations
 *
 * Revision 3.10  1998/08/23 22:19:30  brianp
 * added DoViewportMapping to context struct
 *
 * Revision 3.9  1998/07/09 03:15:41  brianp
 * include asm_386.h instead of asm-386.h
 *
 * Revision 3.8  1998/06/22 03:16:28  brianp
 * added MITS code
 *
 * Revision 3.7  1998/06/07 22:18:52  brianp
 * implemented GL_EXT_multitexture extension
 *
 * Revision 3.6  1998/04/18 05:01:18  brianp
 * renamed USE_ASM to USE_X86_ASM
 *
 * Revision 3.5  1998/03/28 04:01:53  brianp
 * added CONST macro to fix IRIX compilation problems
 *
 * Revision 3.4  1998/03/27 04:26:44  brianp
 * fixed G++ warnings
 *
 * Revision 3.3  1998/02/20 04:53:07  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.2  1998/02/02 03:09:34  brianp
 * added GL_LIGHT_MODEL_COLOR_CONTROL (separate specular color interpolation)
 *
 * Revision 3.1  1998/02/01 16:37:19  brianp
 * added GL_EXT_rescale_normal extension
 *
 * Revision 3.0  1998/01/31 21:06:45  brianp
 * initial rev
 *
 */


/*
 * This file implements transformation, clip testing and projection of
 * vertices in the vertex buffer.
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <stdlib.h>
#include "context.h"
#include "fog.h"
#include "light.h"
#include "macros.h"
#include "matrix.h"
#include "mmath.h"
#include "texture.h"
#include "types.h"
#include "vb.h"
#include "vbrender.h"
#include "vbxform.h"
#include "xform.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



/* This value matches the one in clip.c, used to cope with numeric error. */
#define MAGIC_NUMBER -0.8e-03F

/*
 * Test an array of vertices against the user-defined clipping planes.
 * Input:  ctx - the context
 *         n - number of vertices
 *         vEye - array [n] of vertices, in eye coordinate system
 * Output:  clipMask - array [n] of clip values: 0=not clipped, !0=clipped
 * Return:  CLIP_ALL - if all vertices are clipped by one of the planes
 *          CLIP_NONE - if no vertices were clipped
 *          CLIP_SOME - if some vertices were clipped
 */
static GLuint userclip_test_vertices( GLcontext *ctx, GLuint n,
                                      CONST GLfloat vEye[][4],
                                      GLubyte clipMask[] )
{
   GLboolean anyClipped = GL_FALSE;
   GLuint p;

   ASSERT(ctx->Transform.AnyClip);

   START_FAST_MATH;

   for (p=0;p<MAX_CLIP_PLANES;p++) {
      if (ctx->Transform.ClipEnabled[p]) {
         GLfloat a = ctx->Transform.ClipEquation[p][0];
         GLfloat b = ctx->Transform.ClipEquation[p][1];
         GLfloat c = ctx->Transform.ClipEquation[p][2];
         GLfloat d = ctx->Transform.ClipEquation[p][3];
         GLboolean allClipped = GL_TRUE;
         GLuint i;
         for (i=0;i<n;i++) {
            GLfloat dot = vEye[i][0] * a + vEye[i][1] * b
                        + vEye[i][2] * c + vEye[i][3] * d;
            if (dot < MAGIC_NUMBER) {
               /* this vertex is clipped */
               clipMask[i] = CLIP_USER_BIT;
               anyClipped = GL_TRUE;
            }
            else {
               /* vertex not clipped */
               allClipped = GL_FALSE;
            }
         }
         if (allClipped) {
            return CLIP_ALL;
         }
      }
   }

   END_FAST_MATH;

   return anyClipped ? CLIP_SOME : CLIP_NONE;
}


/*
 * Transform an array of vertices from clip coordinate space to window
 * coordinates.
 * Input:  ctx - the context
 *         n - number of vertices to transform
 *         vClip - array [n] of input vertices
 *         clipMask - array [n] of vertex clip masks.  NULL = no clipped verts
 * Output:  vWin - array [n] of vertices in window coordinate system
 */
static void viewport_map_vertices( GLcontext *ctx,
                                   GLuint n, 
				   CONST GLfloat vClip[][4],
                                   const GLubyte clipMask[], 
				   GLfloat vWin[][3],
				   GLuint vertex_size)
{
   GLfloat sx = ctx->Viewport.Sx;
   GLfloat tx = ctx->Viewport.Tx;
   GLfloat sy = ctx->Viewport.Sy;
   GLfloat ty = ctx->Viewport.Ty;
   GLfloat sz = ctx->Viewport.Sz;
   GLfloat tz = ctx->Viewport.Tz;

   START_FAST_MATH;

   if (vertex_size < 4) {
      /* don't need to divide by W */
      if (clipMask) {
         /* one or more vertices are clipped */
         GLuint i;
         for (i=0;i<n;i++) {
            if (clipMask[i]==0) {
               vWin[i][0] = vClip[i][0] * sx + tx;
               vWin[i][1] = vClip[i][1] * sy + ty;
               vWin[i][2] = vClip[i][2] * sz + tz;
            }
         }
      }
      else {
         /* no vertices are clipped */
         GLuint i;
         for (i=0;i<n;i++) {
            vWin[i][0] = vClip[i][0] * sx + tx;
            vWin[i][1] = vClip[i][1] * sy + ty;
            vWin[i][2] = vClip[i][2] * sz + tz;
         }
      }
   }
   else {
      /* need to divide by W */
      if (clipMask) {
         /* one or more vertices are clipped */
         GLuint i;
         for (i=0;i<n;i++) {
            if (clipMask[i] == 0) {
               if (vClip[i][3] != 0.0F) {
                  GLfloat wInv = 1.0F / vClip[i][3];
                  vWin[i][0] = vClip[i][0] * wInv * sx + tx;
                  vWin[i][1] = vClip[i][1] * wInv * sy + ty;
                  vWin[i][2] = vClip[i][2] * wInv * sz + tz;
               }
               else {
                  /* Div by zero!  Can't set window coords to infinity, so...*/
                  vWin[i][0] = 0.0F;
                  vWin[i][1] = 0.0F;
                  vWin[i][2] = 0.0F;
               }
            }
         }
      }
      else {
         /* no vertices are clipped */
         GLuint i;
         for (i=0;i<n;i++) {
            if (vClip[i][3] != 0.0F) {
               GLfloat wInv = 1.0F / vClip[i][3];
               vWin[i][0] = vClip[i][0] * wInv * sx + tx;
               vWin[i][1] = vClip[i][1] * wInv * sy + ty;
               vWin[i][2] = vClip[i][2] * wInv * sz + tz;
            }
            else {
               /* Divide by zero!  Can't set window coords to infinity, so...*/
               vWin[i][0] = 0.0F;
               vWin[i][1] = 0.0F;
               vWin[i][2] = 0.0F;
            }
         }
      }
   }

   END_FAST_MATH;
}



/*
 * Check if the global material has to be updated with info that was
 * associated with a vertex via glMaterial.
 * This function is used when any material values get changed between
 * glBegin/glEnd either by calling glMaterial() or by calling glColor()
 * when GL_COLOR_MATERIAL is enabled.
 *
 * KW: Added code here to keep the precomputed variables uptodate.
 *     This means we can use the faster shade functions when using
 *     GL_COLOR_MATERIAL, and we can also now use the precomputed 
 *     values in the slower shading functions, which further offsets
 *     the cost of doing this here.
 */
static void update_material( GLcontext *ctx, GLuint i )
{
   struct vertex_buffer *VB = ctx->VB;
   struct gl_light *light, *first = ctx->Light.FirstEnabled;
   GLfloat tmp[4];
   
   if (VB->MaterialMask[i] & FRONT_AMBIENT_BIT) {
      struct gl_material *mat = &ctx->Light.Material[0];
      SUB_3V( tmp, VB->Material[i][0].Ambient, mat->Ambient );
      ACC_SCALE_3V( ctx->Light.BaseColor[0], ctx->Light.Model.Ambient, tmp);
      for (light = first; light; light = light->NextEnabled) {
	 ACC_SCALE_3V( ctx->Light.BaseColor[0], light->Ambient, tmp );
      }
      COPY_4V( mat->Ambient, VB->Material[i][0].Ambient );
   }

   if (VB->MaterialMask[i] & BACK_AMBIENT_BIT) {
      struct gl_material *mat = &ctx->Light.Material[1];
      SUB_3V( tmp, VB->Material[i][1].Ambient, mat->Ambient );
      ACC_SCALE_3V( ctx->Light.BaseColor[1], ctx->Light.Model.Ambient, tmp);
      for (light = first; light; light = light->NextEnabled) {
	 ACC_SCALE_3V( ctx->Light.BaseColor[0], light->Ambient, tmp );
      }
      COPY_4V( mat->Ambient, VB->Material[i][1].Ambient );
   }

   if (VB->MaterialMask[i] & FRONT_DIFFUSE_BIT) {
      struct gl_material *mat = &ctx->Light.Material[0];
      SUB_3V( tmp, VB->Material[i][0].Diffuse, mat->Diffuse );
      for (light = first; light; light = light->NextEnabled) {
	 ACC_SCALE_3V( light->MatDiffuse[0], light->Diffuse, tmp );
      }
      ctx->Light.BaseColor[0][3] = MIN2( mat->Diffuse[3], 1.0F );
      COPY_4V( mat->Diffuse, VB->Material[i][0].Diffuse );
   }

   if (VB->MaterialMask[i] & BACK_DIFFUSE_BIT) {
      struct gl_material *mat = &ctx->Light.Material[1];
      SUB_3V( tmp, VB->Material[i][1].Diffuse, mat->Diffuse );
      for (light = first; light; light = light->NextEnabled) {
	 ACC_SCALE_3V( light->MatDiffuse[1], light->Diffuse, tmp );
      }
      ctx->Light.BaseColor[1][3] = MIN2( mat->Diffuse[3], 1.0F );
      COPY_4V( mat->Diffuse, VB->Material[i][1].Diffuse );
   }

   if (VB->MaterialMask[i] & FRONT_SPECULAR_BIT) {
      struct gl_material *mat = &ctx->Light.Material[0];
      SUB_3V( tmp, VB->Material[i][0].Specular, mat->Specular );
      for (light = first; light; light = light->NextEnabled) {
	 if (light->IsSpecular) {
	    ACC_SCALE_3V( light->MatSpecular[0], light->Specular, tmp );
	    light->IsMatSpecular[0] = 
	       (LEN_SQUARED_3FV(light->MatSpecular[0]) > 1e-16);
	 }
      }
      COPY_4V( mat->Specular, VB->Material[i][0].Specular );
   }
   if (VB->MaterialMask[i] & BACK_SPECULAR_BIT) {
      struct gl_material *mat = &ctx->Light.Material[1];
      SUB_3V( tmp, VB->Material[i][1].Specular, mat->Specular );
      for (light = first; light; light = light->NextEnabled) {
	 if (light->IsSpecular) {
	    ACC_SCALE_3V( light->MatSpecular[1], light->Specular, tmp );
	    light->IsMatSpecular[1] = 
	       (LEN_SQUARED_3FV(light->MatSpecular[1]) > 1e-16);
	 }
      }
      COPY_4V( mat->Specular, VB->Material[i][1].Specular );
   }
   if (VB->MaterialMask[i] & FRONT_EMISSION_BIT) {
      struct gl_material *mat = &ctx->Light.Material[0];
      SUB_3V( tmp, VB->Material[i][0].Emission, mat->Emission );
      ACC_3V( ctx->Light.BaseColor[0], tmp );
      COPY_4V( mat->Emission, VB->Material[i][0].Emission );
   }
   if (VB->MaterialMask[i] & BACK_EMISSION_BIT) {
      struct gl_material *mat = &ctx->Light.Material[1];
      SUB_3V( tmp, VB->Material[i][1].Emission, mat->Emission );
      ACC_3V( ctx->Light.BaseColor[1], tmp );
      COPY_4V( mat->Emission, VB->Material[i][1].Emission );
   }
   if (VB->MaterialMask[i] & FRONT_SHININESS_BIT) {
      ctx->Light.Material[0].Shininess = VB->Material[i][0].Shininess;
      gl_compute_material_shine_table( &ctx->Light.Material[0] );
   }
   if (VB->MaterialMask[i] & BACK_SHININESS_BIT) {
      ctx->Light.Material[1].Shininess = VB->Material[i][1].Shininess;
      gl_compute_material_shine_table( &ctx->Light.Material[1] );
   }
   if (VB->MaterialMask[i] & FRONT_INDEXES_BIT) {
      ctx->Light.Material[0].AmbientIndex = VB->Material[i][0].AmbientIndex;
      ctx->Light.Material[0].DiffuseIndex = VB->Material[i][0].DiffuseIndex;
      ctx->Light.Material[0].SpecularIndex = VB->Material[i][0].SpecularIndex;
   }
   if (VB->MaterialMask[i] & BACK_INDEXES_BIT) {
      ctx->Light.Material[1].AmbientIndex = VB->Material[i][1].AmbientIndex;
      ctx->Light.Material[1].DiffuseIndex = VB->Material[i][1].DiffuseIndex;
      ctx->Light.Material[1].SpecularIndex = VB->Material[i][1].SpecularIndex;
   }

   VB->MaterialMask[i] = 0;  /* reset now */
}


/*
 * Compute shading for vertices in vertex buffer.
 */
static void shade( GLcontext *ctx, GLuint vbStart, GLuint vbCount )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint i,j;

   for ( i = vbStart ; i < vbCount ; i = j ) {
      j = VB->NextMaterial[i];
	    
      if (VB->MaterialMask[i])
	 update_material( ctx, i );

      (ctx->shade_func)( ctx, &ctx->shade_context, i, j - i );
   }
      
   if (VB->MaterialMask[vbCount])
      update_material( ctx, i );
}



/*
 * Compute shading for vertices in vertex buffer when they all share
 * the same normal vector.
 */
static void shade_mononormal( GLcontext *ctx, GLuint vbStart, 
			      GLuint vbCount )
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint i,j,k;
   GLuint side_flags = ctx->shade_context.side_flags;

   for ( i = vbStart ; i < vbCount ; i = j ) {
      j = VB->NextMaterial[i];
	    
      if (VB->MaterialMask[i])
	 update_material( ctx, i );

      (ctx->shade_func)( ctx, &ctx->shade_context, i, 1 );

      if (side_flags & 2) {
	 for ( k = i+1 ; k < j ; k++ ) {
	    COPY_4V( VB->Fcolor[k], VB->Fcolor[i] );
	    COPY_4V( VB->Bcolor[k], VB->Bcolor[i] );
	 }
      } else {
	 for ( k = i+1 ; k < j ; k++ ) {
	    COPY_4V( VB->Fcolor[k], VB->Fcolor[i] );
	 }
      }
   }
      
   if (VB->MaterialMask[vbCount])
      update_material( ctx, i );
}




/*
 * Compute fog for the vertices in the vertex buffer.
 */
static void fog_vertices( GLcontext *ctx, GLuint vbStart, GLuint vbCount )
{
   struct vertex_buffer *VB = ctx->VB;

   if (ctx->Visual->RGBAflag) {
      /* Fog RGB colors */
      gl_fog_rgba_vertices( ctx, vbCount - vbStart,
                            VB->Eye + vbStart,
                            VB->Fcolor + vbStart );
      if (ctx->LightTwoSide) {
         gl_fog_rgba_vertices( ctx, vbCount - vbStart,
                               VB->Eye + vbStart,
                               VB->Bcolor + vbStart );
      }
   }
   else {
      /* Fog color indexes */
      gl_fog_ci_vertices( ctx, vbCount - vbStart,
                          VB->Eye + vbStart,
                          VB->Findex + vbStart );
      if (ctx->LightTwoSide) {
         gl_fog_ci_vertices( ctx, vbCount - vbStart,
                             VB->Eye + vbStart,
                             VB->Bindex + vbStart );
      }
   }
}


/*
 * In the glbegin code it is necessary to:
 * 1) determine whether we need eye coords:
 *       - fog
 *       - texture generation
 *       - positional lights & attenuation & ~length preserving
 *       - any lights & ~fast_light_mode & ~angle preserving
 *       - (temporarily) user clip planes
 *
 * 2) determine whether we need eye normals:
 *       - need eye coords && need normals
 *       - any lights & fast_light_mode & ~angle preserving
 * 3) if we don't need one or both of these, 
 *    determine whether we need:
 *       - object space light directions
 *       - object space light positions
 *       - (eventually) object space linear attenuation factors
 *       - (eventually) object space user clip planes
 *       - combined model/projection matrix
 *       - fast lighting function (also in update-material)
 *    and compute/repair those we do.
 *
 * Given this information, we are able under various circumstances to
 *       - skip the model transformation entirely, or
 *       - skip vertex transform and only transform the normals, or
 *       - skip vertex and normal transforms and just normalize the normals, or
 *       - perform the full three-stage tranformation in the worst case...
 * 
 * This function returns the clipOrMask and clipAndMask values 
 * for the given VB range.  
 *
 * ... Could actually calculate positional lights with a matrix
 * containing a uniform scale, as long as we adjusted the distance
 * calculations accordingly.
 *
 * Anyway, fix this later...
 *
 *
 */
static void gl_transform_vb_range( GLcontext *ctx, 
				   GLuint vbStart, 
				   GLuint vbCount,
				   GLubyte *clipOrMask, 
				   GLubyte *clipAndMask,
				   GLboolean firstPartToDo)
{
   struct vertex_buffer *VB = ctx->VB;
   GLuint vertex_size = ((VB->VertexSizeMask&VERTEX4_BIT) ? 4 : 3);
   GLboolean have_eye_coords = ctx->NeedEyeCoords;
   const GLfloat *from;
   GLuint from_stride;
   GLmatrix *mat;


/*    printf("gl_t_v_r: %d-%d need: eyecoords: %d  eyenormals: %d sz: %d\n", */
/* 	   vbStart, vbCount, ctx->NeedEyeCoords, ctx->NeedEyeNormals, */
/* 	   vertex_size);  */


   /* If necessary, apply modelview matrix to transform vertexes
    * from Object to Eye coords.  
    */
   if (ctx->NeedEyeCoords) 
   {
      gl_transform_points( &ctx->ModelView, 
			   vbCount - vbStart,
			   VB->ObjPtr + vbStart * VB->ObjStride, 
			   VB->Eye + vbStart,
			   VB->ObjStride, 
			   vertex_size);

      if (!TEST_MAT_FLAGS( &ctx->ModelView, MAT_FLAGS_3D))
	 vertex_size = 4;
   }

   /* Similarly, transform normals to eye space or normalize
    * them inplace, as required. 
    */
   if (ctx->NeedEyeNormals) 
   {
      gl_transform_normals_3fv( vbCount - vbStart,
				VB->NormalPtr + vbStart * VB->NormalStride, 
				VB->NormalStride,
				ctx->ModelView.inv,
				VB->Normal + vbStart, 
				ctx->Transform.Normalize,
				ctx->Transform.RescaleNormals);
   } 
   else if (ctx->NeedNormals) 
   {
      if (ctx->Transform.Normalize) 
      {
	 gl_normalize_3fv( vbCount - vbStart,
			   VB->NormalPtr + vbStart * VB->NormalStride,
			   VB->NormalStride,
			   VB->Normal + vbStart);
      }
      else if (!ctx->Transform.RescaleNormals &&
	       ctx->rescale_factor != 1.0) 
      {
	 gl_scale_3fv( vbCount - vbStart,
		       VB->NormalPtr + vbStart * VB->NormalStride,
		       VB->NormalStride,
		       VB->Normal + vbStart,
		       ctx->rescale_factor );
      }
   }


   if (ctx->Transform.AnyClip) {
      GLuint result = userclip_test_vertices( ctx, 
                                              vbCount - vbStart,
                                              VB->Eye + vbStart,
                                              VB->ClipMask + vbStart );
      if (result==CLIP_ALL) {
	 *clipOrMask = CLIP_ALL_BITS; 
	 *clipAndMask = CLIP_ALL_BITS;
	 return;
      }
      else if (result==CLIP_SOME) {
	 *clipOrMask = CLIP_USER_BIT;
      }
      else {
	 *clipAndMask = 0;
      }
   }

   /* If we have skipped the obj->eye transform, we apply here the
    * combined obj->clip projection matrix.  Otherwise we apply the
    * bare projection matrix to vertices in eye space, ending up again
    * in clip coordinates.
    *
    * In both cases, we compute the ClipMask for each vertex.  This
    * could be skipped in the case of display lists if those lists
    * calculated a bounding box for their contents.  Further, if such
    * a bounding box were available and we knew that clipping was not
    * required, we could push the clip->window transform from
    * map_vertices into this transform, leaving only the perpective
    * division to do.
    */
   if (have_eye_coords) {
      from = (GLfloat *) (VB->Eye + vbStart);
      from_stride = 4;
      mat = &ctx->ProjectionMatrix;
   }
   else {
      from = VB->ObjPtr + vbStart * VB->ObjStride;
      from_stride = VB->ObjStride;
      mat = &ctx->ModelProjectMatrix;
   }

   gl_project_and_cliptest_points( mat, 
				   vbCount - vbStart, 
				   from,
				   VB->Clip + vbStart,
				   from_stride, 
				   VB->ClipMask + vbStart,
				   clipOrMask, 
				   clipAndMask,
				   vertex_size );
   
   if (!TEST_MAT_FLAGS(mat, MAT_FLAGS_3D))
      vertex_size = 4;

   if (*clipAndMask) {
      *clipOrMask = CLIP_ALL_BITS; 
      return;
   }

   /* Lighting */
   if (ctx->Light.Enabled) {
      VB->NextMaterial[VB->LastMaterial] = VB->Count;
      if (ctx->VB->MonoNormal)
	 shade_mononormal(ctx, vbStart, vbCount);
      else
	 shade(ctx, vbStart, vbCount);
   }

   /* Per-vertex fog */
   if (ctx->Fog.Enabled && ctx->FogMode==FOG_VERTEX) {
      /* KW: Requires eye coordinates */
      fog_vertices(ctx, vbStart, vbCount);
   }

   /* Generate/transform texture coords */
   if (ctx->Texture.Enabled || ctx->RenderMode==GL_FEEDBACK) {
      GLuint texUnit;
      for (texUnit = 0; texUnit < ctx->Const.MaxTextureUnits; texUnit++) {
	 if (ctx->Texture.Unit[texUnit].TexGenEnabled) {
	    /* KW: Requires eye coordinates and eye normals */
	    gl_texgen( ctx, 
		       vbCount - vbStart,
		       (const GLfloat *) VB->Obj + vbStart, 4,
		       VB->Eye + vbStart,
		       VB->Normal + vbStart,
		       VB->MultiTexCoord[texUnit] + vbStart,
		       texUnit );
            /* When R or Q is texgen'ed, R may be non-zero and Q may
             * be non-one. (MJK)
             */
            if (ctx->Texture.Unit[texUnit].TexGenEnabled & (R_BIT|Q_BIT)) {
               VB->TexCoordSize = 4;
            }
	 }

	 if (ctx->TextureMatrix[texUnit].type != MATRIX_IDENTITY) {
	    /* KW: Using texture coord size...
	     */
	    gl_transform_points( &ctx->TextureMatrix[texUnit], 
				 vbCount - vbStart,
				 (GLfloat *)(VB->MultiTexCoord[texUnit] + 
					     vbStart),
				 VB->MultiTexCoord[texUnit] + vbStart,
				 4,
				 ctx->VB->TexCoordSize);

            /* The texture matrix can make R non-zero and Q non-one so
             * switch to 4-component texture mode now. (MJK)
             */
            ctx->VB->TexCoordSize = 4;
	 }
      }
   }
  

   /* Use the viewport parameters to transform vertices from Clip
    * coordinates to Window coordinates.
    */
   if (ctx->DoViewportMapping) {
      viewport_map_vertices( ctx, 
			     vbCount - vbStart, 
			     VB->Clip + vbStart,
			     ( (*clipOrMask) 
			       ? VB->ClipMask + vbStart 
			       : (GLubyte*) NULL ),
			     VB->Win + vbStart,
			     vertex_size);
   }

   /* Device driver rasterization setup.  3Dfx driver, for example. */
   if (ctx->Driver.RasterSetup) {
      (*ctx->Driver.RasterSetup)( ctx, vbStart, vbCount );
   }
}




#ifdef MITS

#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sched.h>
#include <unistd.h>


/* Flag to indicate first time through */
static int firsttime=1;

/* Semaphores for the vertex buffer processing threads */
static sem_t tDone1, tDone2;

pthread_attr_t attr1, attr2;
pthread_t thread1, thread2;


typedef struct {
   GLuint start;
   GLuint count;
   GLcontext *ctx;
   GLboolean firstPartToDo;
   sem_t *thread_sem;
   volatile int tsync;
} GLtsched;


static GLtsched set1, set2;


/* These are the processing threads for the vertex buffer */
void *gl_transform_vb_range_scheduler(void *s) 
{
   struct vertex_buffer *VB;
   GLtsched *set=(GLtsched *)s;

   int sval;

   while(1) {

      /* Sleep until main thread wakes us up again */
      sem_wait( set->thread_sem );

      VB = set->ctx->VB;

      gl_transform_vb_range( set->ctx, set->start, set->count,
                             &VB->ClipOrMask, &VB->ClipAndMask,
                             set->firstPartToDo );
      set->tsync = 1;

   } /* end while */

   return NULL;

} /* end gl_transform_vb_range_scheduler */
#endif



/*
 * This is the main entry point for the vertex transformation stage.
 * The vertices, normals, colors, texcoords, etc, which have been
 * accumulated in the vertex buffer (or vertex arrays) will now be
 * transformed, lit, fogged, clip tested, etc and finally handed
 * off to the rasterization stage.
 *
 * Input: ctx - the context
 *        allDone - TRUE if we're calling from glEnd
 *                  FALSE if we the vertex buffer is filled and more to come
 */
void gl_transform_vb( GLcontext *ctx, GLboolean allDone )
{
   GLboolean firstPartToDo = GL_TRUE;
   GLuint i;
   struct vertex_buffer *VB = ctx->VB;
#ifdef MITS
   struct sched_param sparam1, sparam2, mparams;
#endif

#ifdef PROFILE
   GLdouble t0 = gl_time();
#endif

   ASSERT( VB->Count>0 );

#ifdef PROFILE
   ctx->VertexTime += gl_time() - t0;
   ctx->VertexCount += VB->Count - VB->Start;
#endif
  
   if (ctx->Texture.Enabled || ctx->RenderMode==GL_FEEDBACK) {
      for (i = 0 ; i < ctx->Const.MaxTextureUnits; i++) {
         gl_matrix_analyze( &ctx->TextureMatrix[i] );
      }
   }

   if (ctx->Driver.RasterSetup && VB->Start) {
      (*ctx->Driver.RasterSetup)( ctx, 0, VB->Start );
   }

#ifdef MITS

   if (VB->Count > 72) {

      if( firsttime ) {
         int policy;
         if( !getuid() ) {
            policy = SCHED_FIFO;
         }
         else {
            policy = SCHED_OTHER;
         }

         mparams.sched_priority = sched_get_priority_max(policy) - 1;
         sched_setscheduler(0, policy, &mparams);

         sparam1.sched_priority = sched_get_priority_max(policy);
         sparam2.sched_priority = sched_get_priority_max(policy);

         sem_init( &tDone1, 0, 0);
         sem_init( &tDone2, 0, 0);	    

         /* Set up threads with OTHER policy and maximum priority */
         pthread_attr_init( &attr1 );
         pthread_attr_setscope( &attr1, PTHREAD_SCOPE_SYSTEM);
         pthread_attr_setschedpolicy( &attr1, policy );
         pthread_attr_setdetachstate( &attr1, PTHREAD_CREATE_DETACHED);
         pthread_attr_setschedparam( &attr1, &sparam1 );

         pthread_attr_init( &attr2 );
         pthread_attr_setscope( &attr2, PTHREAD_SCOPE_SYSTEM);
         pthread_attr_setschedpolicy( &attr2, policy );
         pthread_attr_setdetachstate( &attr2, PTHREAD_CREATE_DETACHED);
         pthread_attr_setschedparam( &attr2, &sparam2 );

         set1.thread_sem = &tDone1;
         set2.thread_sem = &tDone2;

         pthread_create( &thread1, &attr1, gl_transform_vb_range_scheduler, &set1);
         pthread_create( &thread2, &attr2, gl_transform_vb_range_scheduler, &set2); 
         firsttime = 0;

      } /* end if */

      set1.ctx = ctx;
      set1.start = VB->Start;
      set1.count = VB->Start + (VB->Count-VB->Start)/2;
      set1.firstPartToDo = firstPartToDo;

      set2.ctx = ctx;
      set2.start = VB->Start + (VB->Count - VB->Start)/2;
      set2.count = VB->Count;
      set2.firstPartToDo = firstPartToDo;

      set1.tsync = set2.tsync = 0;


      /* Wake the vertex buffer processing threads */
      sem_post( &tDone1 );
      sem_post( &tDone2 );
      sched_yield();

      /* Spin until both are done */
      while( !set1.tsync && !set2.tsync)
         ;
   }
   else {

      gl_transform_vb_range( ctx, VB->Start, VB->Count,
                             &VB->ClipOrMask, &VB->ClipAndMask,
                             firstPartToDo );

   } /* end if */

#else

   gl_transform_vb_range( ctx, VB->Start, VB->Count,
                          &VB->ClipOrMask, &VB->ClipAndMask,
                          firstPartToDo );

#endif

   if (VB->ClipAndMask) {
      /* all vertices are clipped by one plane- nothing to be rendered! */
      gl_reset_vb( ctx, allDone );
      return;
   }

#ifdef PROFILE
   ctx->VertexTime += gl_time() - t0;
   ctx->VertexCount += VB->Count - VB->Start;
#endif

   /*
    * Now we're ready to rasterize the Vertex Buffer!!!
    *
    * If the device driver can't rasterize the vertex buffer then we'll
    * do it ourselves.
    */
   if (!ctx->Driver.RenderVB || !(*ctx->Driver.RenderVB)(ctx,allDone)) {
      (*ctx->RenderVB)( ctx, allDone );
   }

   /*
    * Reset vertex buffer to default state or do setup for continuing
    * a very long primitive.
    */
   gl_reset_vb( ctx, allDone );
}
