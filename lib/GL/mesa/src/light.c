/* Id: light.c,v 1.3 1999/02/26 08:52:34 martin Exp $ */

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
 * Log: light.c,v $
 * Revision 1.3  1999/02/26 08:52:34  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.9  1999/02/24 22:48:06  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.8  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.7  1999/01/30 03:28:15  brianp
 * fixed bug in lighting code (Keith Whitwell)
 *
 * Revision 3.6  1998/10/29 03:57:11  brianp
 * misc clean-up of new vertex transformation code
 *
 * Revision 3.5  1998/10/29 02:28:13  brianp
 * incorporated Keith Whitwell's transformation optimizations
 *
 * Revision 1.4  1998/10/01 20:50:22  keithw
 * new tests in gl_update_state, fix for copy_vertex perf bug
 *
 * Revision 1.3  1998/10/01 13:01:41  keithw
 * object space lighting works with vertex arrays
 *
 * Revision 1.2  1998/09/28 23:00:19  keithw
 * object space lighting
 *
 * Revision 1.1.1.1  1998/09/22 11:58:22  keithw
 * new
 *
 * Revision 3.4  1998/03/27 03:37:40  brianp
 * fixed G++ warnings
 *
 * Revision 3.3  1998/02/27 00:44:52  brianp
 * fixed an incorrect error message string
 *
 * Revision 3.2  1998/02/08 20:18:20  brianp
 * removed unneeded headers
 *
 * Revision 3.1  1998/02/02 03:09:34  brianp
 * added GL_LIGHT_MODEL_COLOR_CONTROL (separate specular color interpolation)
 *
 * Revision 3.0  1998/01/31 20:54:56  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/light.c,v 1.2 1999/03/14 03:20:46 dawes Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <assert.h>
#include <stdlib.h>
#endif
#include <float.h>
#include <math.h>
#include "context.h"
#include "light.h"
#include "matrix.h"
#include "macros.h"
#include "mmath.h"
#include "types.h"
#include "vb.h"
#include "xform.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



void gl_ShadeModel( GLcontext *ctx, GLenum mode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glShadeModel" );
      return;
   }

   switch (mode) {
      case GL_FLAT:
      case GL_SMOOTH:
         if (ctx->Light.ShadeModel!=mode) {
            ctx->Light.ShadeModel = mode;
            ctx->NewState |= NEW_RASTER_OPS;
         }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glShadeModel" );
   }
}



void gl_Lightfv( GLcontext *ctx,
                 GLenum light, GLenum pname, const GLfloat *params,
                 GLint nparams )
{
   GLint l;

   (void) nparams;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glLight" );
      return;
   }

   l = (GLint) (light - GL_LIGHT0);

   if (l<0 || l>=MAX_LIGHTS) {
      gl_error( ctx, GL_INVALID_ENUM, "glLight" );
      return;
   }

   switch (pname) {
      case GL_AMBIENT:
         COPY_4V( ctx->Light.Light[l].Ambient, params );
         break;
      case GL_DIFFUSE:
         COPY_4V( ctx->Light.Light[l].Diffuse, params );
         break;
      case GL_SPECULAR:
         COPY_4V( ctx->Light.Light[l].Specular, params );
         break;
      case GL_POSITION:
	 /* transform position by ModelView matrix */
	 TRANSFORM_POINT( ctx->Light.Light[l].EyePosition, 
			  ctx->ModelView.m,
                          params );
         break;
      case GL_SPOT_DIRECTION:
	 /* transform direction by inverse modelview */
	 if (ctx->ModelView.flags & MAT_DIRTY_INVERSE) {
	    gl_matrix_analyze( &ctx->ModelView );
	 }
	 TRANSFORM_NORMAL( ctx->Light.Light[l].EyeDirection,
			   params,
			   ctx->ModelView.inv );
         break;
      case GL_SPOT_EXPONENT:
         if (params[0]<0.0 || params[0]>128.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         if (ctx->Light.Light[l].SpotExponent != params[0]) {
            ctx->Light.Light[l].SpotExponent = params[0];
            gl_compute_spot_exp_table( &ctx->Light.Light[l] );
         }
         break;
      case GL_SPOT_CUTOFF:
         if ((params[0]<0.0 || params[0]>90.0) && params[0]!=180.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         ctx->Light.Light[l].SpotCutoff = params[0];
         ctx->Light.Light[l].CosCutoff = cos(params[0]*DEG2RAD);
         break;
      case GL_CONSTANT_ATTENUATION:
         if (params[0]<0.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         ctx->Light.Light[l].ConstantAttenuation = params[0];
         break;
      case GL_LINEAR_ATTENUATION:
         if (params[0]<0.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         ctx->Light.Light[l].LinearAttenuation = params[0];
         break;
      case GL_QUADRATIC_ATTENUATION:
         if (params[0]<0.0) {
            gl_error( ctx, GL_INVALID_VALUE, "glLight" );
            return;
         }
         ctx->Light.Light[l].QuadraticAttenuation = params[0];
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glLight" );
         break;
   }

   ctx->NewState |= NEW_LIGHTING;
}



void gl_GetLightfv( GLcontext *ctx,
                    GLenum light, GLenum pname, GLfloat *params )
{
   GLint l = (GLint) (light - GL_LIGHT0);

   if (l<0 || l>=MAX_LIGHTS) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetLightfv" );
      return;
   }

   switch (pname) {
      case GL_AMBIENT:
         COPY_4V( params, ctx->Light.Light[l].Ambient );
         break;
      case GL_DIFFUSE:
         COPY_4V( params, ctx->Light.Light[l].Diffuse );
         break;
      case GL_SPECULAR:
         COPY_4V( params, ctx->Light.Light[l].Specular );
         break;
      case GL_POSITION:
         COPY_4V( params, ctx->Light.Light[l].EyePosition );
         break;
      case GL_SPOT_DIRECTION:
         COPY_3V( params, ctx->Light.Light[l].EyeDirection );
         break;
      case GL_SPOT_EXPONENT:
         params[0] = ctx->Light.Light[l].SpotExponent;
         break;
      case GL_SPOT_CUTOFF:
         params[0] = ctx->Light.Light[l].SpotCutoff;
         break;
      case GL_CONSTANT_ATTENUATION:
         params[0] = ctx->Light.Light[l].ConstantAttenuation;
         break;
      case GL_LINEAR_ATTENUATION:
         params[0] = ctx->Light.Light[l].LinearAttenuation;
         break;
      case GL_QUADRATIC_ATTENUATION:
         params[0] = ctx->Light.Light[l].QuadraticAttenuation;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetLightfv" );
         break;
   }
}



void gl_GetLightiv( GLcontext *ctx, GLenum light, GLenum pname, GLint *params )
{
   GLint l = (GLint) (light - GL_LIGHT0);

   if (l<0 || l>=MAX_LIGHTS) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetLightiv" );
      return;
   }

   switch (pname) {
      case GL_AMBIENT:
         params[0] = FLOAT_TO_INT(ctx->Light.Light[l].Ambient[0]);
         params[1] = FLOAT_TO_INT(ctx->Light.Light[l].Ambient[1]);
         params[2] = FLOAT_TO_INT(ctx->Light.Light[l].Ambient[2]);
         params[3] = FLOAT_TO_INT(ctx->Light.Light[l].Ambient[3]);
         break;
      case GL_DIFFUSE:
         params[0] = FLOAT_TO_INT(ctx->Light.Light[l].Diffuse[0]);
         params[1] = FLOAT_TO_INT(ctx->Light.Light[l].Diffuse[1]);
         params[2] = FLOAT_TO_INT(ctx->Light.Light[l].Diffuse[2]);
         params[3] = FLOAT_TO_INT(ctx->Light.Light[l].Diffuse[3]);
         break;
      case GL_SPECULAR:
         params[0] = FLOAT_TO_INT(ctx->Light.Light[l].Specular[0]);
         params[1] = FLOAT_TO_INT(ctx->Light.Light[l].Specular[1]);
         params[2] = FLOAT_TO_INT(ctx->Light.Light[l].Specular[2]);
         params[3] = FLOAT_TO_INT(ctx->Light.Light[l].Specular[3]);
         break;
      case GL_POSITION:
         params[0] = (GLint) ctx->Light.Light[l].EyePosition[0];
         params[1] = (GLint) ctx->Light.Light[l].EyePosition[1];
         params[2] = (GLint) ctx->Light.Light[l].EyePosition[2];
         params[3] = (GLint) ctx->Light.Light[l].EyePosition[3];
         break;
      case GL_SPOT_DIRECTION:
         params[0] = (GLint) ctx->Light.Light[l].EyeDirection[0];
         params[1] = (GLint) ctx->Light.Light[l].EyeDirection[1];
         params[2] = (GLint) ctx->Light.Light[l].EyeDirection[2];
         break;
      case GL_SPOT_EXPONENT:
         params[0] = (GLint) ctx->Light.Light[l].SpotExponent;
         break;
      case GL_SPOT_CUTOFF:
         params[0] = (GLint) ctx->Light.Light[l].SpotCutoff;
         break;
      case GL_CONSTANT_ATTENUATION:
         params[0] = (GLint) ctx->Light.Light[l].ConstantAttenuation;
         break;
      case GL_LINEAR_ATTENUATION:
         params[0] = (GLint) ctx->Light.Light[l].LinearAttenuation;
         break;
      case GL_QUADRATIC_ATTENUATION:
         params[0] = (GLint) ctx->Light.Light[l].QuadraticAttenuation;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetLightiv" );
         break;
   }
}



/**********************************************************************/
/***                        Light Model                             ***/
/**********************************************************************/


void gl_LightModelfv( GLcontext *ctx, GLenum pname, const GLfloat *params )
{
   switch (pname) {
      case GL_LIGHT_MODEL_AMBIENT:
         COPY_4V( ctx->Light.Model.Ambient, params );
         break;
      case GL_LIGHT_MODEL_LOCAL_VIEWER:
         if (params[0]==0.0)
            ctx->Light.Model.LocalViewer = GL_FALSE;
         else
            ctx->Light.Model.LocalViewer = GL_TRUE;
         break;
      case GL_LIGHT_MODEL_TWO_SIDE:
         if (params[0]==0.0)
            ctx->Light.Model.TwoSide = GL_FALSE;
         else
            ctx->Light.Model.TwoSide = GL_TRUE;
         break;
      case GL_LIGHT_MODEL_COLOR_CONTROL:
         if (params[0] == (GLfloat) GL_SINGLE_COLOR)
            ctx->Light.Model.ColorControl = GL_SINGLE_COLOR;
         else if (params[0] == (GLfloat) GL_SEPARATE_SPECULAR_COLOR)
            ctx->Light.Model.ColorControl = GL_SEPARATE_SPECULAR_COLOR;
         else
            gl_error( ctx, GL_INVALID_ENUM, "glLightModel(param)" );
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glLightModel" );
         break;
   }
   ctx->NewState |= NEW_LIGHTING;
}




/********** MATERIAL **********/


/*
 * Given a face and pname value (ala glColorMaterial), compute a bitmask
 * of the targeted material values.
 */
GLuint gl_material_bitmask( GLenum face, GLenum pname )
{
   GLuint bitmask = 0;

   /* Make a bitmask indicating what material attribute(s) we're updating */
   switch (pname) {
      case GL_EMISSION:
         bitmask |= FRONT_EMISSION_BIT | BACK_EMISSION_BIT;
         break;
      case GL_AMBIENT:
         bitmask |= FRONT_AMBIENT_BIT | BACK_AMBIENT_BIT;
         break;
      case GL_DIFFUSE:
         bitmask |= FRONT_DIFFUSE_BIT | BACK_DIFFUSE_BIT;
         break;
      case GL_SPECULAR:
         bitmask |= FRONT_SPECULAR_BIT | BACK_SPECULAR_BIT;
         break;
      case GL_SHININESS:
         bitmask |= FRONT_SHININESS_BIT | BACK_SHININESS_BIT;
         break;
      case GL_AMBIENT_AND_DIFFUSE:
         bitmask |= FRONT_AMBIENT_BIT | BACK_AMBIENT_BIT;
         bitmask |= FRONT_DIFFUSE_BIT | BACK_DIFFUSE_BIT;
         break;
      case GL_COLOR_INDEXES:
         bitmask |= FRONT_INDEXES_BIT  | BACK_INDEXES_BIT;
         break;
      default:
         gl_problem(NULL, "Bad param in gl_material_bitmask");
         return 0;
   }

   ASSERT( face==GL_FRONT || face==GL_BACK || face==GL_FRONT_AND_BACK );

   if (face==GL_FRONT) {
      bitmask &= FRONT_MATERIAL_BITS;
   }
   else if (face==GL_BACK) {
      bitmask &= BACK_MATERIAL_BITS;
   }

   return bitmask;
}



/*
 * This is called by glColor() when GL_COLOR_MATERIAL is enabled and
 * called by glMaterial() when GL_COLOR_MATERIAL is disabled.
 */
void gl_set_material( GLcontext *ctx, GLuint bitmask, const GLfloat *params )
{
   struct gl_material *mat;

   if (INSIDE_BEGIN_END(ctx)) {
      struct vertex_buffer *VB = ctx->VB;
      /* Save per-vertex material changes in the Vertex Buffer.
       * The update_material function will eventually update the global
       * ctx->Light.Material values.
       */
      mat = VB->Material[VB->Count];
      VB->MaterialMask[VB->Count] |= bitmask;
      if (VB->LastMaterial != VB->Count) {
	 VB->NextMaterial[VB->LastMaterial] = VB->Count;
	 VB->LastMaterial = VB->Count;
      }
   }
   else {
      /* just update the global material property */
      mat = ctx->Light.Material;
      ctx->NewState |= NEW_LIGHTING;
   }

   if (bitmask & FRONT_AMBIENT_BIT) {
      COPY_4V( mat[0].Ambient, params );
   }
   if (bitmask & BACK_AMBIENT_BIT) {
      COPY_4V( mat[1].Ambient, params );
   }
   if (bitmask & FRONT_DIFFUSE_BIT) {
      COPY_4V( mat[0].Diffuse, params );
   }
   if (bitmask & BACK_DIFFUSE_BIT) {
      COPY_4V( mat[1].Diffuse, params );
   }
   if (bitmask & FRONT_SPECULAR_BIT) {
      COPY_4V( mat[0].Specular, params );
   }
   if (bitmask & BACK_SPECULAR_BIT) {
      COPY_4V( mat[1].Specular, params );
   }
   if (bitmask & FRONT_EMISSION_BIT) {
      COPY_4V( mat[0].Emission, params );
   }
   if (bitmask & BACK_EMISSION_BIT) {
      COPY_4V( mat[1].Emission, params );
   }
   if (bitmask & FRONT_SHININESS_BIT) {
      GLfloat shininess = CLAMP( params[0], 0.0F, 128.0F );
      if (mat[0].Shininess != shininess) {
         mat[0].Shininess = shininess;
	 /* duplicated in update_material, 
	  * shouldn't be necessary betweeen begin/end:
	  */
         gl_compute_material_shine_table( &mat[0] ); 
      }
   }
   if (bitmask & BACK_SHININESS_BIT) {
      GLfloat shininess = CLAMP( params[0], 0.0F, 128.0F );
      if (mat[1].Shininess != shininess) {
         mat[1].Shininess = shininess;
	 /* duplicated in update_material: */
         gl_compute_material_shine_table( &mat[1] );
      }
   }
   if (bitmask & FRONT_INDEXES_BIT) {
      mat[0].AmbientIndex = params[0];
      mat[0].DiffuseIndex = params[1];
      mat[0].SpecularIndex = params[2];
   }
   if (bitmask & BACK_INDEXES_BIT) {
      mat[1].AmbientIndex = params[0];
      mat[1].DiffuseIndex = params[1];
      mat[1].SpecularIndex = params[2];
   }
}



void gl_ColorMaterial( GLcontext *ctx, GLenum face, GLenum mode )
{
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glColorMaterial" );
      return;
   }
   switch (face) {
      case GL_FRONT:
      case GL_BACK:
      case GL_FRONT_AND_BACK:
         ctx->Light.ColorMaterialFace = face;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glColorMaterial(face)" );
         return;
   }
   switch (mode) {
      case GL_EMISSION:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
      case GL_AMBIENT_AND_DIFFUSE:
         ctx->Light.ColorMaterialMode = mode;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glColorMaterial(mode)" );
         return;
   }

   ctx->Light.ColorMaterialBitmask = gl_material_bitmask( face, mode );
}



/*
 * This is only called via the api_function_table struct or by the
 * display list executor.
 */
void gl_Materialfv( GLcontext *ctx,
                    GLenum face, GLenum pname, const GLfloat *params )
{
   GLuint bitmask;

   /* error checking */
   if (face!=GL_FRONT && face!=GL_BACK && face!=GL_FRONT_AND_BACK) {
      gl_error( ctx, GL_INVALID_ENUM, "glMaterial(face)" );
      return;
   }
   switch (pname) {
      case GL_EMISSION:
      case GL_AMBIENT:
      case GL_DIFFUSE:
      case GL_SPECULAR:
      case GL_SHININESS:
      case GL_AMBIENT_AND_DIFFUSE:
      case GL_COLOR_INDEXES:
         /* OK */
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glMaterial(pname)" );
         return;
   }

   /* convert face and pname to a bitmask */
   bitmask = gl_material_bitmask( face, pname );

   if (ctx->Light.ColorMaterialEnabled) {
      /* The material values specified by glColorMaterial() can't be */
      /* updated by glMaterial() while GL_COLOR_MATERIAL is enabled! */
      bitmask &= ~ctx->Light.ColorMaterialBitmask;
   }

   gl_set_material( ctx, bitmask, params );
}




void gl_GetMaterialfv( GLcontext *ctx,
                       GLenum face, GLenum pname, GLfloat *params )
{
   GLuint f;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetMaterialfv" );
      return;
   }
   if (face==GL_FRONT) {
      f = 0;
   }
   else if (face==GL_BACK) {
      f = 1;
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glGetMaterialfv(face)" );
      return;
   }
   switch (pname) {
      case GL_AMBIENT:
         COPY_4V( params, ctx->Light.Material[f].Ambient );
         break;
      case GL_DIFFUSE:
         COPY_4V( params, ctx->Light.Material[f].Diffuse );
	 break;
      case GL_SPECULAR:
         COPY_4V( params, ctx->Light.Material[f].Specular );
	 break;
      case GL_EMISSION:
	 COPY_4V( params, ctx->Light.Material[f].Emission );
	 break;
      case GL_SHININESS:
	 *params = ctx->Light.Material[f].Shininess;
	 break;
      case GL_COLOR_INDEXES:
	 params[0] = ctx->Light.Material[f].AmbientIndex;
	 params[1] = ctx->Light.Material[f].DiffuseIndex;
	 params[2] = ctx->Light.Material[f].SpecularIndex;
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetMaterialfv(pname)" );
   }
}



void gl_GetMaterialiv( GLcontext *ctx,
                       GLenum face, GLenum pname, GLint *params )
{
   GLuint f;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetMaterialiv" );
      return;
   }
   if (face==GL_FRONT) {
      f = 0;
   }
   else if (face==GL_BACK) {
      f = 1;
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glGetMaterialiv(face)" );
      return;
   }
   switch (pname) {
      case GL_AMBIENT:
         params[0] = FLOAT_TO_INT( ctx->Light.Material[f].Ambient[0] );
         params[1] = FLOAT_TO_INT( ctx->Light.Material[f].Ambient[1] );
         params[2] = FLOAT_TO_INT( ctx->Light.Material[f].Ambient[2] );
         params[3] = FLOAT_TO_INT( ctx->Light.Material[f].Ambient[3] );
         break;
      case GL_DIFFUSE:
         params[0] = FLOAT_TO_INT( ctx->Light.Material[f].Diffuse[0] );
         params[1] = FLOAT_TO_INT( ctx->Light.Material[f].Diffuse[1] );
         params[2] = FLOAT_TO_INT( ctx->Light.Material[f].Diffuse[2] );
         params[3] = FLOAT_TO_INT( ctx->Light.Material[f].Diffuse[3] );
	 break;
      case GL_SPECULAR:
         params[0] = FLOAT_TO_INT( ctx->Light.Material[f].Specular[0] );
         params[1] = FLOAT_TO_INT( ctx->Light.Material[f].Specular[1] );
         params[2] = FLOAT_TO_INT( ctx->Light.Material[f].Specular[2] );
         params[3] = FLOAT_TO_INT( ctx->Light.Material[f].Specular[3] );
	 break;
      case GL_EMISSION:
         params[0] = FLOAT_TO_INT( ctx->Light.Material[f].Emission[0] );
         params[1] = FLOAT_TO_INT( ctx->Light.Material[f].Emission[1] );
         params[2] = FLOAT_TO_INT( ctx->Light.Material[f].Emission[2] );
         params[3] = FLOAT_TO_INT( ctx->Light.Material[f].Emission[3] );
	 break;
      case GL_SHININESS:
         *params = ROUNDF( ctx->Light.Material[f].Shininess );
	 break;
      case GL_COLOR_INDEXES:
	 params[0] = ROUNDF( ctx->Light.Material[f].AmbientIndex );
	 params[1] = ROUNDF( ctx->Light.Material[f].DiffuseIndex );
	 params[2] = ROUNDF( ctx->Light.Material[f].SpecularIndex );
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetMaterialfv(pname)" );
   }
}




/**********************************************************************/
/*****                  Lighting computation                      *****/
/**********************************************************************/


/*
 * Notes:
 *   When two-sided lighting is enabled we compute the color (or index)
 *   for both the front and back side of the primitive.  Then, when the
 *   orientation of the facet is later learned, we can determine which
 *   color (or index) to use for rendering.
 *
 * Variables:
 *   n = normal vector
 *   V = vertex position
 *   P = light source position
 *   Pe = (0,0,0,1)
 *
 * Precomputed:
 *   IF P[3]==0 THEN
 *       // light at infinity
 *       IF local_viewer THEN
 *           VP_inf_norm = unit vector from V to P      // Precompute
 *       ELSE 
 *           // eye at infinity
 *           h_inf_norm = Normalize( VP + <0,0,1> )     // Precompute
 *       ENDIF
 *   ENDIF
 *
 * Functions:
 *   Normalize( v ) = normalized vector v
 *   Magnitude( v ) = length of vector v
 */



/*
 * Whenever the spotlight exponent for a light changes we must call
 * this function to recompute the exponent lookup table.
 */
void gl_compute_spot_exp_table( struct gl_light *l )
{
   int i;
   double exponent = l->SpotExponent;
   double tmp;
   int clamp = 0;

   l->SpotExpTable[0][0] = 0.0;

   for (i=EXP_TABLE_SIZE-1;i>0;i--) {
      if (clamp == 0) {
         tmp = pow(i/(double)(EXP_TABLE_SIZE-1), exponent);
         if (tmp < FLT_MIN*100.0) {
            tmp = 0.0;
            clamp = 1;
         }
      }
      l->SpotExpTable[i][0] = tmp;
   }
   for (i=0;i<EXP_TABLE_SIZE-1;i++) {
      l->SpotExpTable[i][1] = l->SpotExpTable[i+1][0] - l->SpotExpTable[i][0];
   }
   l->SpotExpTable[EXP_TABLE_SIZE-1][1] = 0.0;
}



/*
 * Whenever the shininess of a material changes we must call this
 * function to recompute the exponential lookup table.
 */
void gl_compute_material_shine_table( struct gl_material *m )
{
   int i;
   m->ShineTable[0] = 0.0F;
   for (i=1;i<SHINE_TABLE_SIZE;i++) {
      /* just invalidate the table */
      m->ShineTable[i] = -1.0;
   }
}



/*
 * Examine current lighting parameters to determine if the optimized lighting
 * function can be used.
 * Also, precompute some lighting values such as the products of light
 * source and material ambient, diffuse and specular coefficients.
 */
void gl_update_lighting( GLcontext *ctx )
{
   GLint i, side;
   struct gl_light **prev_enabled, *light;
   static GLfloat ci[3] = { .30, .59, .11 };

   if (!ctx->Light.Enabled) {
      /* If lighting is not enabled, we can skip all this. */
      return;
   }

   /* Setup linked list of enabled light sources */
   prev_enabled = &(ctx->Light.FirstEnabled);
   light = ctx->Light.Light;
   for (i=0; i<MAX_LIGHTS; i++, light++) {
      if (light->Enabled) {
	 (*prev_enabled) = light;
         prev_enabled = &light->NextEnabled;
      }
   }
   *prev_enabled = NULL;


   /* Precompute some lighting flags */
   ctx->Light.AnySpecular = GL_FALSE;
   ctx->Light.NeedVertices = GL_FALSE;
      
   for (light = ctx->Light.FirstEnabled; light; light = light->NextEnabled) {
      light->IsPositional = (light->EyePosition[3] != 0.0F);
      light->IsSpecular = (LEN_SQUARED_3FV(light->Specular) > 1e-16);
      light->IsSpot = (light->SpotCutoff != 180.0F);
      ctx->Light.AnySpecular |= light->IsSpecular;
      ctx->Light.NeedVertices |= (light->IsPositional | light->IsSpot);
   }

   if (ctx->Light.Model.LocalViewer && ctx->Light.AnySpecular)
      ctx->Light.NeedVertices = GL_TRUE;


   /* Precompute some shading values */
   if (ctx->Visual->RGBAflag) 
   {
      GLuint sides = (ctx->LightTwoSide ? 2 : 1);
      for (side=0; side < sides; side++) {
	 struct gl_material *mat = &ctx->Light.Material[side];
	 
	 COPY_3V(ctx->Light.BaseColor[side], mat->Emission);
	 ACC_SCALE_3V(ctx->Light.BaseColor[side], 
		      ctx->Light.Model.Ambient,
		      mat->Ambient);

	 ctx->Light.BaseColor[side][3]
	    = MIN2( ctx->Light.Material[side].Diffuse[3], 1.0F );
      }
      
      for (light = ctx->Light.FirstEnabled; light; light = light->NextEnabled)
      {
	 for (side=0; side< sides; side++) {
	    struct gl_material *mat = &ctx->Light.Material[side];
	    SCALE_3V( light->MatDiffuse[side],  light->Diffuse, mat->Diffuse );
	    SCALE_3V( light->MatAmbient[side],  light->Ambient, mat->Ambient );
	    ACC_3V( ctx->Light.BaseColor[side], light->MatAmbient[side] ); 
	    if (light->IsSpecular)
	    {
	       SCALE_3V( light->MatSpecular[side], light->Specular,
			 mat->Specular);
	       light->IsMatSpecular[side] = 
		  (LEN_SQUARED_3FV(light->MatSpecular[side]) > 1e-16);
	    } 
	    else 
	       light->IsMatSpecular[side] = 0;
	 }
      }
   }
   else 
   {
      for (light = ctx->Light.FirstEnabled; light; light = light->NextEnabled)
      {
	 /* Compute color index diffuse and specular light intensities */
	 light->dli = DOT3(ci, light->Diffuse);
	 light->sli = DOT3(ci, light->Specular);
      }
   }


}

/* Need to seriously restrict the circumstances under which these
 * calc's are performed.
 */
void gl_compute_light_positions( GLcontext *ctx )
{
   struct gl_light *light;
   
   if (ctx->Light.NeedVertices && !ctx->Light.Model.LocalViewer) {
      GLfloat eye_z[3] = { 0, 0, 1 };
      if (!ctx->NeedEyeCoords) {
	 TRANSFORM_NORMAL( ctx->EyeZDir, eye_z, ctx->ModelView.m );
      } else {
	 COPY_3V( ctx->EyeZDir, eye_z );
      }
   }
	 
   for (light = ctx->Light.FirstEnabled; light; light = light->NextEnabled) 
   {          
      if (!ctx->NeedEyeCoords) {
	 TRANSFORM_POINT( light->Position, ctx->ModelView.inv, 
			  light->EyePosition );
/* 	 printf("light %d pos %f %f %f %f dirty: %d\n",  */
/* 		light - ctx->Light.Light, */
/* 		light->Position[0], light->Position[1], */
/* 		light->Position[2], light->Position[3], */
/* 		ctx->ModelView.flags & MAT_DIRTY_INVERSE); */
      } else {
	 COPY_4V( light->Position, light->EyePosition );
      }

      if (!light->IsPositional)
      {
	 /* VP (VP) = Normalize( Position ) */
	 COPY_3V( light->VP_inf_norm, light->Position );
	 NORMALIZE_3FV( light->VP_inf_norm );

	 /* if (light->IsSpecular &&  */
	 if (!ctx->Light.Model.LocalViewer)
	 {
	    /* h_inf_norm = Normalize( V_to_P + <0,0,1> ) */
	    ADD_3V( light->h_inf_norm, light->VP_inf_norm, ctx->EyeZDir);
	    NORMALIZE_3FV( light->h_inf_norm );
	 }
      }

      /* BUG!!! */
      if (light->IsSpot) {
	 if (ctx->NeedEyeNormals) {
	    COPY_3V( light->NormDirection, light->EyeDirection );
	 } else {
	    TRANSFORM_POINT3( light->NormDirection, 
			      ctx->ModelView.m, 
			      light->EyeDirection );
	 }

	 NORMALIZE_3FV( light->NormDirection );
      }
   }

   if (!ctx->NeedEyeNormals && 
       !ctx->Transform.RescaleNormals && 
       !ctx->Transform.Normalize)
   {
      if (ctx->ModelView.flags & (MAT_FLAG_UNIFORM_SCALE | 
				  MAT_FLAG_GENERAL_SCALE |
				  MAT_FLAG_GENERAL_3D |
	                          MAT_FLAG_GENERAL) ) 
      
      {
	 GLfloat *m = ctx->ModelView.inv;
	 GLfloat f = GL_SQRT( m[2]*m[2] + m[6]*m[6] + m[10]*m[10] );
	 if (f*f < 1e-30) f = 1.0F;
	 else if ((f-1)*(f-1) < 1e-16) f = 1.0F; 
/* 	 else f = 1.0 / f; */
	 ctx->rescale_factor = f; /* f; */
      }
      else
	 ctx->rescale_factor = 1.0F;	 
   }

   gl_update_shade_context( ctx, &ctx->shade_context );
}




void gl_update_shade_context( GLcontext *ctx, struct gl_shade_context *sc )
{
   if (ctx->NeedEyeCoords) {
      sc->vertices = (GLfloat *) ctx->VB->Eye;
      sc->vertex_stride = 4;
      sc->normals = (GLfloat *) ctx->VB->Normal;
      sc->normal_stride = 3;
   }
   else {
      sc->vertices = ctx->VB->ObjPtr;
      sc->vertex_stride = ctx->VB->ObjStride;
      
      if (ctx->Transform.Normalize || 
	  (!ctx->Transform.RescaleNormals && ctx->rescale_factor != 1.0)) {
	 sc->normals = (GLfloat *) ctx->VB->Normal;
	 sc->normal_stride = 3;
      }
      else
      {
	 sc->normals = ctx->VB->NormalPtr;
	 sc->normal_stride = ctx->VB->NormalStride;
      }
   }

   sc->side_flags = (ctx->LightTwoSide ? 0x3 : 0x1);
}
