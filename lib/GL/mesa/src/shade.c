/* Id: shade.c,v 1.3 1999/02/26 08:52:36 martin Exp $ */

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
 * Log: shade.c,v $
 * Revision 1.3  1999/02/26 08:52:36  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.9  1999/02/24 22:48:07  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.8  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.7  1998/10/30 04:41:24  brianp
 * inserted a missing vector length computation in shade_ci_any_sided()
 *
 * Revision 3.6  1998/10/29 03:57:11  brianp
 * misc clean-up of new vertex transformation code
 *
 * Revision 3.5  1998/10/29 02:28:13  brianp
 * incorporated Keith Whitwell's transformation optimizations
 *
 * Revision 3.4  1998/07/01 02:39:14  brianp
 * added a hack to work around suspected gcc bug
 *
 * Revision 3.3  1998/04/18 05:00:28  brianp
 * now using FLOAT_COLOR_TO_UBYTE_COLOR macro
 *
 * Revision 3.2  1998/03/27 04:17:31  brianp
 * fixed G++ warnings
 *
 * Revision 3.1  1998/02/02 03:09:34  brianp
 * added GL_LIGHT_MODEL_COLOR_CONTROL (separate specular color interpolation)
 *
 * Revision 3.0  1998/01/31 21:03:42  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/shade.c,v 1.0tsi Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <math.h>
#endif
#include "macros.h"
#include "mmath.h"
#include "shade.h"
#include "types.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif




/* Ugh, I think there's a bug in gcc 2.7.2.3.  If the following
 * no-op code isn't here then the results of the above
 * FLOAT_COLOR_TO_UBYTE_COLOR() macro are unpredictable!
 */
#define DODGY_NOOP(x)					\
do {							\
   GLubyte r0 = FloatToInt(CLAMP(x, 0, 1) * 255.0);	\
   (void) r0;						\
} while( 0 )

#define SH_TAB_MAX 1.0

#define GET_SHINE_TAB_ENTRY( tab, dp, shininess, result )	\
do {								\
   int k = (int) (dp * ((SHINE_TABLE_SIZE-1)/SH_TAB_MAX));		\
   if (tab[k] < 0.0F) {						\
      GLdouble z = pow(dp, shininess);				\
      if (z<1.0e-10)						\
	 result = tab[k] = 0.0F;				\
      else							\
	 result = tab[k] = (GLfloat) z;				\
   }								\
   else								\
      result = tab[k];						\
} while(0)



static void shade_rgba_spec_any_sided( GLcontext *ctx,
				       struct gl_shade_context *sc,
				       GLuint start,
				       GLuint n )
{
   GLuint j;
   GLfloat Fbase[3], FbaseA;
   GLfloat Bbase[3], BbaseA;
   GLint FsumA;
   GLint BsumA;
   struct gl_light *light;
   struct gl_material *mat;

   GLuint side_flags = sc->side_flags;
   GLuint  vstride = sc->vertex_stride;
   const GLfloat *vertex = sc->vertices + (start * vstride);
   GLuint  nstride = sc->normal_stride;
   const GLfloat *normal = sc->normals + (start * nstride);
   GLubyte (*Fcolor)[4] = sc->Fcolor + start;
   GLubyte (*Bcolor)[4] = sc->Bcolor + start; 
   GLubyte (*Fspec)[4] = sc->Fspec + start;
   GLubyte (*Bspec)[4] = sc->Bspec + start; 


   mat = &ctx->Light.Material[0];
   SCALE_3V( Fbase, ctx->Light.Model.Ambient, mat->Ambient);
   ACC_3V( Fbase, mat->Emission );
   FbaseA = mat->Diffuse[3];  /* Alpha is simple, same for all vertices */
   FLOAT_COLOR_TO_UBYTE_COLOR( FsumA, FbaseA );

   if (side_flags & 2)
   {
      mat = &ctx->Light.Material[1];
      SCALE_3V( Bbase, ctx->Light.Model.Ambient, mat->Ambient);
      ACC_3V( Bbase, mat->Emission );
      BbaseA = mat->Diffuse[3];  /* Alpha is simple, same for all vertices */
      FLOAT_COLOR_TO_UBYTE_COLOR( BsumA, BbaseA );
   }

   for ( j = 0 ; j < n ; j++, vertex += vstride, normal += nstride ) {
      GLfloat sum[2][3], spec[2][3];
      COPY_3V(sum[0], Fbase);
      ZERO_3V(spec[0]);
      
      if (side_flags & 2) {
	 COPY_3V(sum[1], Bbase);
	 ZERO_3V(spec[1]);
      }

      /* Add contribution from each enabled light source */
      for (light=ctx->Light.FirstEnabled; light; light=light->NextEnabled) {
	 struct gl_material *mat;
	 GLfloat n_dot_h;
	 GLfloat correction = 1;
	 GLint side = 0; 
	 GLfloat contrib[3];
         GLfloat attenuation = 1.0;
         GLfloat VP[3];  /* unit vector from vertex to light */
         GLfloat n_dot_VP;       /* n dot VP */
	 GLfloat *h;
	 GLboolean normalized;

         /* compute VP and attenuation */
         if (!light->IsPositional) {
            /* directional light */
            COPY_3V(VP, light->VP_inf_norm);
         }
         else {
            GLfloat d;     /* distance from vertex to light */
            SUB_3V(VP, light->Position, vertex);
            d = LEN_3FV( VP );
            if ( d > 1e-6) {
               GLfloat invd = 1.0F / d;
               SELF_SCALE_SCALAR_3V(VP, invd);
            }
	    /* if (light->Attenuated) */
	    attenuation = 1.0F / (light->ConstantAttenuation + d * 
				  (light->LinearAttenuation + d * 
				   light->QuadraticAttenuation));
         }

         /* spotlight attenuation */
         if (light->IsSpot) 
	 {
            GLfloat PV_dot_dir = - DOT3(VP, light->NormDirection);
            if (PV_dot_dir<=0.0F || PV_dot_dir<light->CosCutoff) {
	       continue; /* this light makes no contribution */
            }
            else {
               double x = PV_dot_dir * (EXP_TABLE_SIZE-1);
               int k = (int) x;
               GLfloat spot = (light->SpotExpTable[k][0]
			       + (x-k)*light->SpotExpTable[k][1]);
	       attenuation *= spot;
            }
         }

	 if (attenuation < 1e-5) 
	    continue;		/* this light makes no contribution */

         /* Compute dot product or normal and vector from V to light pos */
         n_dot_VP = DOT3( normal, VP );

         /* which side are we lighting? */
         if (n_dot_VP < 0.0F) {
	    ACC_SCALE_SCALAR_3V(sum[0], attenuation, light->MatAmbient[0]);
	    if (!(side_flags & 2)) {
	       continue;
	    }
	    side = 1;
	    correction = -1;
	    n_dot_VP = -n_dot_VP;
	 } else if (side_flags & 2) {
	    ACC_SCALE_SCALAR_3V( sum[1], attenuation, light->MatAmbient[1] );
	 } 

	 /* diffuse term */
	 COPY_3V(contrib, light->MatAmbient[side]);
	 ACC_SCALE_SCALAR_3V(contrib, n_dot_VP, light->MatDiffuse[side]);
	 ACC_SCALE_SCALAR_3V(sum[side], attenuation, contrib );

	 if (!light->IsMatSpecular[side])
	    continue;

	 /* specular term - cannibalize VP... */
	 if (ctx->Light.Model.LocalViewer) {
	    GLfloat v[3];
	    COPY_3V(v, vertex);
	    NORMALIZE_3FV(v);
	    SUB_3V(VP, VP, v);                /* h = VP + VPe */
	    h = VP;
	    normalized = 0;
	 }
	 else if (light->IsPositional) {
	    h = VP;
	    /*h[2] += 1.0;                     h = VP + eye<0,0,1> */
	    ACC_3V(h, ctx->EyeZDir);
	    normalized = 0;
	 } else {
	    h = light->h_inf_norm;
	    normalized = 1;
	 }
	 
	 n_dot_h = correction * DOT3(normal, h);

	 if (n_dot_h > 0.0F) 
	 {
	    GLfloat spec_coef;
	    mat = &ctx->Light.Material[side];

	    if (!normalized)
	       n_dot_h /= LEN_3FV( h );
	    
	    if (n_dot_h>SH_TAB_MAX) 
	       spec_coef = pow( n_dot_h, mat->Shininess );
	    else {
	       GET_SHINE_TAB_ENTRY( mat->ShineTable, n_dot_h, 
				    mat->Shininess, spec_coef );
	    }

	    if (spec_coef > 1.0e-10) {
	       spec_coef *= attenuation;
	       ACC_SCALE_SCALAR_3V( spec[side], spec_coef,
				    light->MatSpecular[side]);
	    }
         }
      } /*loop over lights*/

      /* clamp and convert to integer or fixed point */
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][0], sum[0][0] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][1], sum[0][1] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][2], sum[0][2] );
      Fcolor[j][3] = FsumA;


      FLOAT_COLOR_TO_UBYTE_COLOR( Fspec[j][0], spec[0][0] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fspec[j][1], spec[0][1] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fspec[j][2], spec[0][2] );
      Fspec[j][3] = 255;  /* but never used */

      if (side_flags & 2) {
	 FLOAT_COLOR_TO_UBYTE_COLOR( Bcolor[j][0], sum[1][0] );
	 FLOAT_COLOR_TO_UBYTE_COLOR( Bcolor[j][1], sum[1][1] );
	 FLOAT_COLOR_TO_UBYTE_COLOR( Bcolor[j][2], sum[1][2] );
	 Bcolor[j][3] = BsumA;
	 
	 FLOAT_COLOR_TO_UBYTE_COLOR( Bspec[j][0], spec[1][0] );
	 FLOAT_COLOR_TO_UBYTE_COLOR( Bspec[j][1], spec[1][1] );
	 FLOAT_COLOR_TO_UBYTE_COLOR( Bspec[j][2], spec[1][2] );
	 Bspec[j][3] = 255;  /* but never used */
      }
   } 
}



static void shade_rgba_twosided( GLcontext *ctx,
				 struct gl_shade_context *sc,
				 GLuint start,
				 GLuint n )
{
   GLuint j;
   GLfloat Fbase[3], FbaseA;
   GLfloat Bbase[3], BbaseA;
   GLint FsumA;
   GLint BsumA;
   struct gl_light *light;
   struct gl_material *mat;

   GLuint  vstride = sc->vertex_stride;
   const GLfloat *vertex = sc->vertices + (start * vstride);
   GLuint  nstride = sc->normal_stride;
   const GLfloat *normal = sc->normals + (start * nstride);
   GLubyte (*Fcolor)[4] = sc->Fcolor + start;
   GLubyte (*Bcolor)[4] = sc->Bcolor + start; 


   mat = &ctx->Light.Material[0];
   Fbase[0] = mat->Emission[0] + ctx->Light.Model.Ambient[0] * mat->Ambient[0];
   Fbase[1] = mat->Emission[1] + ctx->Light.Model.Ambient[1] * mat->Ambient[1];
   Fbase[2] = mat->Emission[2] + ctx->Light.Model.Ambient[2] * mat->Ambient[2];
   FbaseA = mat->Diffuse[3];  /* Alpha is simple, same for all vertices */
   FLOAT_COLOR_TO_UBYTE_COLOR( FsumA, FbaseA );

   mat = &ctx->Light.Material[0];
   Bbase[0] = mat->Emission[0] + ctx->Light.Model.Ambient[0] * mat->Ambient[0];
   Bbase[1] = mat->Emission[1] + ctx->Light.Model.Ambient[1] * mat->Ambient[1];
   Bbase[2] = mat->Emission[2] + ctx->Light.Model.Ambient[2] * mat->Ambient[2];
   BbaseA = mat->Diffuse[3];  /* Alpha is simple, same for all vertices */
   FLOAT_COLOR_TO_UBYTE_COLOR( BsumA, BbaseA );

   for (j = 0; j < n ; j++, vertex += vstride, normal += nstride) {
      GLfloat sum[2][3];

      COPY_3V(sum[0], Fbase);
      COPY_3V(sum[1], Bbase);

      /* Add contribution from each enabled light source */
      for (light=ctx->Light.FirstEnabled; light; light=light->NextEnabled) {
	 GLfloat n_dot_h;
	 GLfloat correction = 1;
	 GLint side = 0; 
	 GLfloat contrib[2][3];
         GLfloat attenuation = 1.0;
         GLfloat VP[3];          /* unit vector from vertex to light */
         GLfloat n_dot_VP;       /* n dot VP */
	 GLfloat *h;
	 GLboolean normalized;

         /* compute VP and attenuation */
         if (!light->IsPositional) {
            /* directional light */
            COPY_3V(VP, light->VP_inf_norm);
         }
         else {
            GLfloat d;     /* distance from vertex to light */
            SUB_3V(VP, light->Position, vertex);
            d = LEN_3FV( VP );
            if ( d > 1e-6) {
               GLfloat invd = 1.0F / d;
               SELF_SCALE_SCALAR_3V(VP, invd);
            }
	    /* if (light->Attenuated) */
	    attenuation = 1.0F / (light->ConstantAttenuation + d * 
				  (light->LinearAttenuation + d * 
				   light->QuadraticAttenuation));
         }

         /* spotlight attenuation */
         if (light->IsSpot) 
	 {
            GLfloat PV_dot_dir = - DOT3(VP, light->NormDirection);
            if (PV_dot_dir<=0.0F || PV_dot_dir<light->CosCutoff) {
	       continue; /* this light makes no contribution */
            }
            else {
               double x = PV_dot_dir * (EXP_TABLE_SIZE-1);
               int k = (int) x;
               GLfloat spot = (light->SpotExpTable[k][0]
			       + (x-k)*light->SpotExpTable[k][1]);
	       attenuation *= spot;
            }
         }

	 if (attenuation < 1e-5) 
	    continue;		/* this light makes no contribution */

	 COPY_3V(contrib[0], light->MatAmbient[0]);
	 COPY_3V(contrib[1], light->MatAmbient[1]);

         /* Compute dot product or normal and vector from V to light pos */
         n_dot_VP = DOT3( normal, VP );

         /* which side are we lighting? */
         if (n_dot_VP < 0.0F) {
	    side = 1;
	    correction = -1;
	    n_dot_VP = -n_dot_VP;
	 }
      
	 /* diffuse term */
	 ACC_SCALE_SCALAR_3V(contrib[side], n_dot_VP, light->MatDiffuse[side]);

	 /* specular term - cannibalize VP... */
	 if (light->IsMatSpecular[side])
	 {
	    if (ctx->Light.Model.LocalViewer) {
	       GLfloat v[3];
	       COPY_3V(v, vertex);
	       NORMALIZE_3FV(v);
	       SUB_3V(VP, VP, v);                /* h = VP + VPe */
	       h = VP;
	       normalized = 0;
	    }
	    else if (light->IsPositional) {
	       h = VP;
	       /* h[2] += 1.0;                     h = VP + eye<0,0,1> */
	       ACC_3V(h, ctx->EyeZDir);
	       normalized = 0;
	    } else {
	       h = light->h_inf_norm;
	       normalized = 1;
	    }
	 
	    n_dot_h = correction * DOT3(normal, h);

	    if (n_dot_h > 0.0F) 
	    {
	       GLfloat spec_coef;
	       
	       if (!normalized)
		  n_dot_h /= LEN_3FV( h );
	       
	       if (n_dot_h>SH_TAB_MAX) 
		  spec_coef = pow( n_dot_h, mat->Shininess );
	       else {
		  GET_SHINE_TAB_ENTRY( mat->ShineTable, n_dot_h, 
				       mat->Shininess, spec_coef );
	       }

	       if (spec_coef > 1.0e-10) {
		  ACC_SCALE_SCALAR_3V( contrib[side], spec_coef,
				       light->MatSpecular[side]);
	       }
	    }
         }

	 ACC_SCALE_SCALAR_3V( sum[0], attenuation, contrib[0] );
	 ACC_SCALE_SCALAR_3V( sum[1], attenuation, contrib[1] );
      } /*loop over lights*/

      /* clamp and convert to integer or fixed point */
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][0], sum[0][0] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][1], sum[0][1] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][2], sum[0][2] );
      Fcolor[j][3] = FsumA;

      FLOAT_COLOR_TO_UBYTE_COLOR( Bcolor[j][0], sum[1][0] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Bcolor[j][1], sum[1][1] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Bcolor[j][2], sum[1][2] );
      Bcolor[j][3] = BsumA;

   } /*loop over vertices*/
}




/*
 * Use current lighting/material settings to compute the RGBA colors of
 * an array of vertexes.
 * Input:  side - 0=use front material, 1=use back material
 *         n - number of vertexes to process
 *         vertex - array of vertex positions in eye coordinates
 *         normal - array of surface normal vectors
 * Output:  color - array of resulting colors
 */
static void shade_rgba( GLcontext *ctx,
			struct gl_shade_context *sc,
			GLuint start,
			GLuint n )
{
   GLuint j;
   GLfloat base[3], baseA;
   GLint sumA;
   struct gl_light *light;
   struct gl_material *mat;

   GLuint  vstride = sc->vertex_stride;
   const GLfloat *vertex = sc->vertices + (start * vstride);
   GLuint  nstride = sc->normal_stride;
   const GLfloat *normal = sc->normals + (start * nstride);
   GLubyte (*Fcolor)[4] = sc->Fcolor + start;

   mat = &ctx->Light.Material[0];
   base[0] = mat->Emission[0] + ctx->Light.Model.Ambient[0] * mat->Ambient[0];
   base[1] = mat->Emission[1] + ctx->Light.Model.Ambient[1] * mat->Ambient[1];
   base[2] = mat->Emission[2] + ctx->Light.Model.Ambient[2] * mat->Ambient[2];
   baseA = mat->Diffuse[3];  /* Alpha is simple, same for all vertices */

   FLOAT_COLOR_TO_UBYTE_COLOR( sumA, baseA );

   for (j = 0; j < n ; j++, vertex += vstride, normal += nstride) {
      GLfloat sum[3];
      COPY_3V(sum, base);

      /* Add contribution from each enabled light source */
      for (light=ctx->Light.FirstEnabled; light; light=light->NextEnabled) {
         GLfloat contrib[3];
         GLfloat attenuation = 1.0;
         GLfloat VP[3];  /* unit vector from vertex to light */
         GLfloat n_dot_VP;       /* n dot VP */

         /* compute VP and attenuation */
         if (!light->IsPositional) {
            /* directional light */
            COPY_3V(VP, light->VP_inf_norm);
         }
         else {
            GLfloat d;     /* distance from vertex to light */
            SUB_3V(VP, light->Position, vertex);
            d = LEN_3FV( VP );
            if ( d > 1e-6) {
               GLfloat invd = 1.0F / d;
               SELF_SCALE_SCALAR_3V(VP, invd);
            }
	    /* if (light->Attenuated) */
	    attenuation = 1.0F / (light->ConstantAttenuation + d * 
				  (light->LinearAttenuation + d * 
				   light->QuadraticAttenuation));
         }

         /* spotlight attenuation */
         if (light->IsSpot) 
	 {
            GLfloat PV_dot_dir = - DOT3(VP, light->NormDirection);
            if (PV_dot_dir<=0.0F || PV_dot_dir<light->CosCutoff) {
	       continue; /* this light makes no contribution */
            }
            else {
               double x = PV_dot_dir * (EXP_TABLE_SIZE-1);
               int k = (int) x;
               GLfloat spot = (light->SpotExpTable[k][0]
			       + (x-k)*light->SpotExpTable[k][1]);
	       attenuation *= spot;
            }
         }

	 if (attenuation < 1e-5) 
	    continue;		/* this light makes no contribution */

	 COPY_3V(contrib, light->MatAmbient[0]);

         /* Compute dot product or normal and vector from V to light pos */
         n_dot_VP = DOT3( normal, VP );

         /* diffuse and specular terms */
         if (n_dot_VP > 0.0F) {
            GLfloat *h, n_dot_h;
            GLboolean normalized = 0;
      

            /* diffuse term */
	    ACC_SCALE_SCALAR_3V( contrib, n_dot_VP, light->MatDiffuse[0] );

            /* specular term - cannibalize VP... */
	    if (light->IsMatSpecular[0]) 
	    {
	       if (ctx->Light.Model.LocalViewer) {
		  GLfloat v[3];
		  COPY_3V(v, vertex);
		  NORMALIZE_3FV(v);
		  SUB_3V(VP, VP, v);           /* h = VP + VPe */
		  h = VP;
	       }
	       else if (light->IsPositional) {
		  h = VP;
		  /*h[2] += 1.0;*/           /* h = VP + eye-space<0,0,1> */
		  ACC_3V(h, ctx->EyeZDir);
	       } else {
		  h = light->h_inf_norm;
		  normalized = 1;
	       }

	       /* attention: h may not be normalized, done later if needed */
	       n_dot_h = DOT3(normal, h);

	       if (n_dot_h > 0.0F) 
	       {
		  GLfloat spec_coef;

		  /* now `correct' the dot product */
		  if (!normalized)
		     n_dot_h /= LEN_3FV( h );

		  if (n_dot_h>SH_TAB_MAX) {
		     /* only happens if normal vector length > 1.0 */
		     spec_coef = pow( n_dot_h, mat->Shininess );
		  }
		  else {
		     /* use table lookup approximation */
		     GET_SHINE_TAB_ENTRY( mat->ShineTable, n_dot_h, 
					  mat->Shininess, spec_coef );
		  }
		  if (spec_coef > 1.0e-10) {
		     ACC_SCALE_SCALAR_3V( contrib, spec_coef,
					  light->MatSpecular[0]);
		  }
	       }
	    }
         }
	 ACC_SCALE_SCALAR_3V( sum, attenuation, contrib );
      } /*loop over lights*/

      /* clamp and convert to integer or fixed point */
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][0], sum[0] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][1], sum[1] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][2], sum[2] );
      Fcolor[j][3] = sumA;

   } /*loop over vertices*/
}


static void shade_rgba_fast_specular( GLcontext *ctx,
				      struct gl_shade_context *sc,
				      GLuint start,
				      GLuint n )
{
   GLuint j;

   GLuint  nstride = sc->normal_stride;
   const GLfloat *normal = sc->normals + (start * nstride);
   GLubyte (*Fcolor)[4] = sc->Fcolor + start;

   GLint FsumA = (GLint) (ctx->Light.BaseColor[0][3] * 255.0F);

   for (j = 0; j < n; j++, normal += nstride) {
      GLfloat sum[3];
      struct gl_light *light;

      COPY_3V(sum, ctx->Light.BaseColor[0]);

      for (light=ctx->Light.FirstEnabled; light; light=light->NextEnabled) 
      {
	 struct gl_material *m;
	 GLfloat n_dot_h;
         GLfloat n_dot_VP = DOT3(normal, light->VP_inf_norm);

         if (n_dot_VP < 0.0F)
	    continue;

	 ACC_SCALE_SCALAR_3V(sum, n_dot_VP, light->MatDiffuse[0]);
	    
	 if (!light->IsMatSpecular[0]) 
	    continue;

	 m = &ctx->Light.Material[0];

	 n_dot_h = DOT3(normal, light->h_inf_norm);	 

	 if (n_dot_h < 0.0F) 
	    continue;
	 
	 if (n_dot_h > SH_TAB_MAX) 
	 {
	    GLfloat spec = pow( n_dot_h, m->Shininess );
	    if (spec > 1.0e-10F) {
	       ACC_SCALE_SCALAR_3V(sum, spec, light->MatSpecular[0]);
	    }
	 }
	 else 
	 {
	    GLfloat spec;
	    GET_SHINE_TAB_ENTRY( m->ShineTable, n_dot_h, m->Shininess, spec );
	    ACC_SCALE_SCALAR_3V( sum, spec, light->MatSpecular[0] );
	 }
      } 

      /* clamp and convert to integer or fixed point */
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][0], sum[0] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][1], sum[1] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][2], sum[2] );
      Fcolor[j][3] = FsumA;

      DODGY_NOOP(sum[0]);
   } 
}

static void shade_rgba_fast_specular_twosided( GLcontext *ctx,
					       struct gl_shade_context *sc,
					       GLuint start,
					       GLuint n )
{
   GLuint j;


   GLuint  nstride = sc->normal_stride;
   const GLfloat *normal = sc->normals + (start * nstride);
   GLubyte (*Fcolor)[4] = sc->Fcolor + start;
   GLubyte (*Bcolor)[4] = sc->Bcolor + start; 


   /* Alpha is easy to compute, same for all vertices */
   GLint FsumA = (GLint) (ctx->Light.BaseColor[0][3] * 255.0F);
   GLint BsumA = (GLint) (ctx->Light.BaseColor[1][3] * 255.0F);

   for (j = 0;j<n;j++, normal += nstride) {
      GLfloat sum[2][3];
      struct gl_light *light;

      COPY_3V(sum[0], ctx->Light.BaseColor[0]);
      COPY_3V(sum[1], ctx->Light.BaseColor[1]);

      for (light=ctx->Light.FirstEnabled; light; light=light->NextEnabled) 
      {
	 struct gl_material *m;
	 GLfloat n_dot_h;
	 GLfloat correction = 1;
	 GLint side = 0;
         GLfloat n_dot_VP = DOT3(normal, light->VP_inf_norm);

         if (n_dot_VP < 0.0F) {
	    side = 1;
	    n_dot_VP = -n_dot_VP;
	    correction = -1;
	 }

	 ACC_SCALE_SCALAR_3V(sum[side], n_dot_VP, light->MatDiffuse[side]);
	    
	 if (!light->IsMatSpecular[side]) 
	    continue;

	 m = &ctx->Light.Material[side];
	 n_dot_h = correction * DOT3(normal, light->h_inf_norm);
	 
	 if (n_dot_h < 0.0F) 
	    continue;
	 
	 if (n_dot_h > SH_TAB_MAX) 
	 {
	    GLfloat spec = pow( n_dot_h, m->Shininess );
	    if (spec > 1.0e-10F) {
	       ACC_SCALE_SCALAR_3V(sum[side], spec, light->MatSpecular[side]);
	    }
	 }
	 else 
	 {
	    GLfloat spec;
	    GET_SHINE_TAB_ENTRY( m->ShineTable, n_dot_h, m->Shininess, spec );
	    ACC_SCALE_SCALAR_3V( sum[side], spec, light->MatSpecular[side] );
	 }
      } 

      /* clamp and convert to integer or fixed point */
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][0], sum[0][0] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][1], sum[0][1] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][2], sum[0][2] );
      Fcolor[j][3] = FsumA;

      FLOAT_COLOR_TO_UBYTE_COLOR( Bcolor[j][0], sum[1][0] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Bcolor[j][1], sum[1][1] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Bcolor[j][2], sum[1][2] );
      Bcolor[j][3] = BsumA;

      DODGY_NOOP(sum[0][0]);
   } 
}



/*
 * It's hard to say whether there is any benefit to having these
 * functions hanging around:
 */
static void shade_rgba_diffuse( GLcontext *ctx,
				struct gl_shade_context *sc,
				GLuint start,
				GLuint n )
{
   GLuint j;

   GLuint  nstride = sc->normal_stride;
   const GLfloat *normal = sc->normals + (start * nstride);
   GLubyte (*Fcolor)[4] = sc->Fcolor + start;

   GLint sumA = (GLint) (ctx->Light.BaseColor[0][3] * 255.0F);

   for ( j = 0 ; j < n ; j++, normal += nstride ) 
   {
      GLfloat sum[3];
      struct gl_light *light;
      COPY_3V(sum, ctx->Light.BaseColor[0]);

      for (light=ctx->Light.FirstEnabled; light; light=light->NextEnabled) 
      {
         GLfloat n_dot_VP = DOT3(normal, light->VP_inf_norm);
         if (n_dot_VP > 0.0F) 
	    ACC_SCALE_SCALAR_3V( sum, n_dot_VP, light->MatDiffuse[0] );
      }

      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][0], sum[0] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][1], sum[1] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][2], sum[2] );
      Fcolor[j][3] = sumA;

      DODGY_NOOP(sum[0]);
   } 
}



static void shade_rgba_diffuse_twosided( GLcontext *ctx,
					 struct gl_shade_context *sc,
					 GLuint start,
					 GLuint n )
{
   GLuint j;

   GLuint  nstride = sc->normal_stride;
   const GLfloat *normal = sc->normals + (start * nstride);
   GLubyte (*Fcolor)[4] = sc->Fcolor + start;
   GLubyte (*Bcolor)[4] = sc->Bcolor + start; 

   GLint FsumA = (GLint) (ctx->Light.BaseColor[0][3] * 255.0F);
   GLint BsumA = (GLint) (ctx->Light.BaseColor[1][3] * 255.0F);

   /* Loop over vertices */
   for ( j = 0 ; j < n ; j++, normal += nstride ) 
   {
      GLfloat sum[2][3];
      struct gl_light *light;

      COPY_3V(sum[0], ctx->Light.BaseColor[0]);
      COPY_3V(sum[1], ctx->Light.BaseColor[1]);

      for (light=ctx->Light.FirstEnabled; light; light=light->NextEnabled) 
      {
         GLfloat n_dot_VP = DOT3(normal, light->VP_inf_norm);
         if (n_dot_VP > 0.0F) 
	 {
  	    ACC_SCALE_SCALAR_3V( sum[0], n_dot_VP, light->MatDiffuse[0] );
	 }
	 else
	 {
  	    ACC_SCALE_SCALAR_3V( sum[1], -n_dot_VP, light->MatDiffuse[1] );
	 }
      }

      /* clamp and convert to integer or fixed point */
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][0], sum[0][0] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][1], sum[0][1] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Fcolor[j][2], sum[0][2] );
      Fcolor[j][3] = FsumA;

      FLOAT_COLOR_TO_UBYTE_COLOR( Bcolor[j][0], sum[1][0] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Bcolor[j][1], sum[1][1] );
      FLOAT_COLOR_TO_UBYTE_COLOR( Bcolor[j][2], sum[1][2] );
      Bcolor[j][3] = BsumA;

      DODGY_NOOP(sum[0][0]);
   } 
}



/*
 * Use current lighting/material settings to compute the color indexes
 * for an array of vertices.
 * Input:  n - number of vertices to shade
 *         side - 0=use front material, 1=use back material
 *         vertex - array of [n] vertex position in eye coordinates
 *         normal - array of [n] surface normal vector
 * Output:  indexResult - resulting array of [n] color indexes
 */
static void shade_ci_any_sided( GLcontext *ctx,
				struct gl_shade_context *sc,
				GLuint start,
				GLuint n )
{
   struct gl_material *mat;
   GLuint j;

   GLuint  side_flags = sc->side_flags;
   GLuint  vstride = sc->vertex_stride;
   const GLfloat *vertex = sc->vertices + (start * vstride);
   GLuint  nstride = sc->normal_stride;
   const GLfloat *normal = sc->normals + (start * nstride);
   GLuint  *indexResult[2];

   indexResult[0] = sc->Findex + start;
   indexResult[1] = sc->Bindex + start;

   /* loop over vertices */
   for (j = 0 ; j < n ; j++, vertex += vstride, normal += nstride) {
      GLfloat diffuse[2], specular[2];
      GLuint side = 0;
      struct gl_light *light;

      diffuse[0] = specular[0] = 0.0F;
      diffuse[1] = specular[1] = 0.0F;

      /* Accumulate diffuse and specular from each light source */
      for (light=ctx->Light.FirstEnabled; light; light=light->NextEnabled) {
         GLfloat attenuation = 1.0F;
         GLfloat VP[3];  /* unit vector from vertex to light */
         GLfloat n_dot_VP;  /* dot product of l and n */
	 GLfloat *h, n_dot_h, correction = 1.0;
	 GLboolean normalized;

         /* compute l and attenuation */
         if (!light->IsPositional) {
            /* directional light */
	    COPY_3V(VP, light->VP_inf_norm);
         }
         else {
            GLfloat d;     /* distance from vertex to light */
	    SUB_3V(VP, light->Position, vertex);
            d = LEN_3FV( VP );
	    if ( d > 1e-6) {
               GLfloat invd = 1.0F / d;
               SELF_SCALE_SCALAR_3V(VP, invd);
            }
	    /* if (light->Attenuated) */
	    attenuation = 1.0F / (light->ConstantAttenuation + d * 
				  (light->LinearAttenuation + d * 
				   light->QuadraticAttenuation));
         }

         /* spotlight attenuation */
         if (light->IsSpot) 
	 {
            GLfloat PV_dot_dir = - DOT3(VP, light->NormDirection);
            if (PV_dot_dir<=0.0F || PV_dot_dir<light->CosCutoff) {
	       continue; /* this light makes no contribution */
            }
            else {
               double x = PV_dot_dir * (EXP_TABLE_SIZE-1);
               int k = (int) x;
               GLfloat spot = (light->SpotExpTable[k][0]
			       + (x-k)*light->SpotExpTable[k][1]);
	       attenuation *= spot;
            }
         }

	 if (attenuation < 1e-5) 
	    continue;		/* this light makes no contribution */

         n_dot_VP = DOT3( normal, VP );

	 /* which side are we lighting? */
         if (n_dot_VP < 0.0F) {
	    if (!(side_flags & 2)) {
	       continue;
	    }
	    side = 1;
	    correction = -1;
	    n_dot_VP = -n_dot_VP;
	 } 


	 /* accumulate diffuse term */
	 diffuse[side] += n_dot_VP * light->dli * attenuation;

	 /* specular term */
	 if (!light->IsSpecular)
	    continue;

	 if (ctx->Light.Model.LocalViewer) {
	    GLfloat v[3];
	    COPY_3V(v, vertex);
	    NORMALIZE_3FV(v);
	    SUB_3V(VP, VP, v);                /* h = VP + VPe */
	    h = VP;
	    normalized = 0;
	 }
	 else if (light->IsPositional) {
	    h = VP;
	    /*h[2] += 1.0;                     h = VP + eye<0,0,1> */
	    ACC_3V(h, ctx->EyeZDir);
	    normalized = 0;
	 } else {
	    h = light->h_inf_norm;
	    normalized = 1;
	 }

	 n_dot_h = correction * DOT3(normal, h);

	 if (n_dot_h > 0.0F) 
	 {
	    GLfloat spec_coef;
	    mat = &ctx->Light.Material[side];
	    
	    if (!normalized)
	       n_dot_h /= LEN_3FV( h );
	    
	    if (n_dot_h>SH_TAB_MAX) {
	       spec_coef = pow( n_dot_h, mat->Shininess );
	    } else {
	       GET_SHINE_TAB_ENTRY( mat->ShineTable, n_dot_h, 
				    mat->Shininess, spec_coef );
	    }
	    specular[side] += spec_coef * light->sli * attenuation;
         }
      } /*loop over lights*/

      /* Now compute final color index */
      for (side = 0 ; side < 2 ; side++)
      {
	 GLfloat index;

	 if (!(side_flags & (1<<side)))
	    continue;

	 mat = &ctx->Light.Material[side];
	 if (specular[side] > 1.0F) {
	    index = mat->SpecularIndex;
	 }
	 else {
	    GLfloat d_a, s_a;
	    d_a = mat->DiffuseIndex - mat->AmbientIndex;
	    s_a = mat->SpecularIndex - mat->AmbientIndex;
	       
	    index = mat->AmbientIndex
	       + diffuse[side] * (1.0F-specular[side]) * d_a
	       + specular[side] * s_a;
	    if (index > mat->SpecularIndex) {
	       index = mat->SpecularIndex;
	    }
	 }
	 indexResult[side][j] = (GLuint) (GLint) index;
      }
   } /*for vertex*/
}

void gl_update_lighting_function( GLcontext *ctx )
{
   if (ctx->Visual->RGBAflag)
   { 
      if (ctx->Texture.Enabled && 
	  ctx->Light.Model.ColorControl==GL_SEPARATE_SPECULAR_COLOR) 
      {
	 ctx->shade_func = shade_rgba_spec_any_sided;
      } 
      else if (ctx->Light.NeedVertices) 
      {
	 if (ctx->LightTwoSide) 
	    ctx->shade_func = shade_rgba_twosided;
	 else
	    ctx->shade_func = shade_rgba;
      } 
      else if (ctx->Light.AnySpecular) 
      {
	 if (ctx->LightTwoSide)
	    ctx->shade_func = shade_rgba_fast_specular_twosided;
	 else
	    ctx->shade_func = shade_rgba_fast_specular;
      }
      else if (ctx->LightTwoSide) 
	 ctx->shade_func = shade_rgba_diffuse_twosided;
      else
	 ctx->shade_func = shade_rgba_diffuse;
   } else {
      ctx->shade_func = shade_ci_any_sided;
   }
}








