/* Id: texstate.c,v 1.3 1999/02/26 08:52:38 martin Exp $ */

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
 * Log: texstate.c,v $
 * Revision 1.3  1999/02/26 08:52:38  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.21  1999/02/24 22:48:07  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.20  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.19  1999/01/22 04:34:13  brianp
 * call new ActiveTexture() device driver function
 *
 * Revision 3.18  1998/11/27 14:13:15  brianp
 * removed some unneeded casts
 *
 * Revision 3.17  1998/11/17 01:52:47  brianp
 * implemented GL_NV_texgen_reflection extension (MJK)
 *
 * Revision 3.16  1998/11/08 22:34:07  brianp
 * renamed texture sets to texture units
 *
 * Revision 3.15  1998/11/03 01:40:37  brianp
 * implemented GL_ARB_multitexture
 *
 * Revision 3.14  1998/11/03 01:16:57  brianp
 * added Texture.ReallyEnabled to simply testing for enabled textures
 *
 * Revision 3.13  1998/10/31 17:07:46  brianp
 * variety of multi-texture changes
 *
 * Revision 3.12  1998/10/20 02:27:56  brianp
 * removed GL_EXT_multitexture extension
 *
 * Revision 3.11  1998/08/21 01:50:14  brianp
 * made max_texture_coord_sets() non-static
 *
 * Revision 3.10  1998/07/18 03:35:28  brianp
 * get of GL_TEXTURE_INTERNAL_FORMAT wasn't handled
 *
 * Revision 3.9  1998/06/17 01:21:06  brianp
 * include assert.h
 *
 * Revision 3.8  1998/06/07 22:18:52  brianp
 * implemented GL_EXT_multitexture extension
 *
 * Revision 3.7  1998/04/20 23:55:13  brianp
 * applied DavidB's changes for v0.25 of 3Dfx driver
 *
 * Revision 3.6  1998/03/27 04:17:31  brianp
 * fixed G++ warnings
 *
 * Revision 3.5  1998/03/15 17:55:03  brianp
 * fixed a typo in gl_update_texture_state()
 *
 * Revision 3.4  1998/02/20 04:53:37  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.3  1998/02/04 05:00:07  brianp
 * added a few casts for Amiga StormC compiler
 *
 * Revision 3.2  1998/02/03 04:27:54  brianp
 * added texture lod clamping
 *
 * Revision 3.1  1998/02/01 19:39:09  brianp
 * added GL_CLAMP_TO_EDGE texture wrap mode
 *
 * Revision 3.0  1998/01/31 21:04:38  brianp
 * initial rev
 *
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include <assert.h>
#include "context.h"
#include "macros.h"
#include "matrix.h"
#include "texobj.h"
#include "texstate.h"
#include "texture.h"
#include "types.h"
#include "xform.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif



#ifdef SPECIALCAST
/* Needed for an Amiga compiler */
#define ENUM_TO_FLOAT(X) ((GLfloat)(GLint)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(GLint)(X))
#else
/* all other compilers */
#define ENUM_TO_FLOAT(X) ((GLfloat)(X))
#define ENUM_TO_DOUBLE(X) ((GLdouble)(X))
#endif




/**********************************************************************/
/*                       Texture Environment                          */
/**********************************************************************/


void gl_TexEnvfv( GLcontext *ctx,
                  GLenum target, GLenum pname, const GLfloat *param )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glTexEnv" );
      return;
   }

   if (target!=GL_TEXTURE_ENV) {
      gl_error( ctx, GL_INVALID_ENUM, "glTexEnv(target)" );
      return;
   }

   if (pname==GL_TEXTURE_ENV_MODE) {
      GLenum mode = (GLenum) (GLint) *param;
      switch (mode) {
	 case GL_MODULATE:
	 case GL_BLEND:
	 case GL_DECAL:
	 case GL_REPLACE:
	    /* A small optimization for drivers */ 
	    if (texUnit->EnvMode == mode)
               return;
	    texUnit->EnvMode = mode;
	    break;
	 default:
	    gl_error( ctx, GL_INVALID_VALUE, "glTexEnv(param)" );
	    return;
      }
   }
   else if (pname==GL_TEXTURE_ENV_COLOR) {
      texUnit->EnvColor[0] = CLAMP( param[0], 0.0, 1.0 );
      texUnit->EnvColor[1] = CLAMP( param[1], 0.0, 1.0 );
      texUnit->EnvColor[2] = CLAMP( param[2], 0.0, 1.0 );
      texUnit->EnvColor[3] = CLAMP( param[3], 0.0, 1.0 );
   }
   else {
      gl_error( ctx, GL_INVALID_ENUM, "glTexEnv(pname)" );
      return;
   }

   /* Tell device driver about the new texture environment */
   if (ctx->Driver.TexEnv) {
      (*ctx->Driver.TexEnv)( ctx, pname, param );
   }
}





void gl_GetTexEnvfv( GLcontext *ctx,
                     GLenum target, GLenum pname, GLfloat *params )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   if (target!=GL_TEXTURE_ENV) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(target)" );
      return;
   }
   switch (pname) {
      case GL_TEXTURE_ENV_MODE:
         *params = ENUM_TO_FLOAT(texUnit->EnvMode);
	 break;
      case GL_TEXTURE_ENV_COLOR:
	 COPY_4V( params, texUnit->EnvColor );
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexEnvfv(pname)" );
   }
}


void gl_GetTexEnviv( GLcontext *ctx,
                     GLenum target, GLenum pname, GLint *params )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   if (target!=GL_TEXTURE_ENV) {
      gl_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(target)" );
      return;
   }
   switch (pname) {
      case GL_TEXTURE_ENV_MODE:
         *params = (GLint) texUnit->EnvMode;
	 break;
      case GL_TEXTURE_ENV_COLOR:
	 params[0] = FLOAT_TO_INT( texUnit->EnvColor[0] );
	 params[1] = FLOAT_TO_INT( texUnit->EnvColor[1] );
	 params[2] = FLOAT_TO_INT( texUnit->EnvColor[2] );
	 params[3] = FLOAT_TO_INT( texUnit->EnvColor[3] );
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexEnviv(pname)" );
   }
}




/**********************************************************************/
/*                       Texture Parameters                           */
/**********************************************************************/


void gl_TexParameterfv( GLcontext *ctx,
                        GLenum target, GLenum pname, const GLfloat *params )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   GLenum eparam = (GLenum) (GLint) params[0];
   struct gl_texture_object *texObj;

   switch (target) {
      case GL_TEXTURE_1D:
         texObj = texUnit->Current1D;
         break;
      case GL_TEXTURE_2D:
         texObj = texUnit->Current2D;
         break;
      case GL_TEXTURE_3D_EXT:
         texObj = texUnit->Current3D;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexParameter(target)" );
         return;
   }

   switch (pname) {
      case GL_TEXTURE_MIN_FILTER:
         /* A small optimization */
         if (texObj->MinFilter == eparam)
            return;

         if (eparam==GL_NEAREST || eparam==GL_LINEAR
             || eparam==GL_NEAREST_MIPMAP_NEAREST
             || eparam==GL_LINEAR_MIPMAP_NEAREST
             || eparam==GL_NEAREST_MIPMAP_LINEAR
             || eparam==GL_LINEAR_MIPMAP_LINEAR) {
            texObj->MinFilter = eparam;
            ctx->NewState |= NEW_TEXTURING;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_MAG_FILTER:
         /* A small optimization */
         if (texObj->MagFilter == eparam)
            return;

         if (eparam==GL_NEAREST || eparam==GL_LINEAR) {
            texObj->MagFilter = eparam;
            ctx->NewState |= NEW_TEXTURING;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_S:
         if (eparam==GL_CLAMP || eparam==GL_REPEAT || eparam==GL_CLAMP_TO_EDGE) {
            texObj->WrapS = eparam;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_T:
         if (eparam==GL_CLAMP || eparam==GL_REPEAT || eparam==GL_CLAMP_TO_EDGE) {
            texObj->WrapT = eparam;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         break;
      case GL_TEXTURE_WRAP_R_EXT:
         if (eparam==GL_CLAMP || eparam==GL_REPEAT || eparam==GL_CLAMP_TO_EDGE) {
            texObj->WrapR = eparam;
         }
         else {
            gl_error( ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
         }
         break;
      case GL_TEXTURE_BORDER_COLOR:
         texObj->BorderColor[0] = CLAMP((GLint)(params[0]*255.0), 0, 255);
         texObj->BorderColor[1] = CLAMP((GLint)(params[1]*255.0), 0, 255);
         texObj->BorderColor[2] = CLAMP((GLint)(params[2]*255.0), 0, 255);
         texObj->BorderColor[3] = CLAMP((GLint)(params[3]*255.0), 0, 255);
         break;
      case GL_TEXTURE_MIN_LOD:
         texObj->MinLod = params[0];
         break;
      case GL_TEXTURE_MAX_LOD:
         texObj->MaxLod = params[0];
         break;
      case GL_TEXTURE_BASE_LEVEL:
         if (params[0] < 0.0) {
            gl_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         texObj->BaseLevel = (GLint) params[0];
         break;
      case GL_TEXTURE_MAX_LEVEL:
         if (params[0] < 0.0) {
            gl_error(ctx, GL_INVALID_VALUE, "glTexParameter(param)" );
            return;
         }
         texObj->MaxLevel = (GLint) params[0];
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexParameter(pname)" );
         return;
   }

   texObj->Dirty = GL_TRUE;
   ctx->Texture.AnyDirty = GL_TRUE;
   
   if (ctx->Driver.TexParameter) {
      (*ctx->Driver.TexParameter)( ctx, target, texObj, pname, params );
   }
}



void gl_GetTexLevelParameterfv( GLcontext *ctx, GLenum target, GLint level,
                                GLenum pname, GLfloat *params )
{
   GLint iparam;
   gl_GetTexLevelParameteriv( ctx, target, level, pname, &iparam );
   *params = (GLfloat) iparam;
}



void gl_GetTexLevelParameteriv( GLcontext *ctx, GLenum target, GLint level,
                                GLenum pname, GLint *params )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_image *tex;

   if (level < 0 || level >= ctx->Const.MaxTextureLevels) {
      gl_error( ctx, GL_INVALID_VALUE, "glGetTexLevelParameter[if]v" );
      return;
   }

   switch (target) {
      case GL_TEXTURE_1D:
         tex = texUnit->Current1D->Image[level];
         if (!tex)
            return;
         switch (pname) {
	    case GL_TEXTURE_WIDTH:
	       *params = tex->Width;
	       break;
	    case GL_TEXTURE_COMPONENTS:
               /* alias for case GL_TEXTURE_INTERNAL_FORMAT */
	       *params = tex->IntFormat;
	       break;
	    case GL_TEXTURE_BORDER:
	       *params = tex->Border;
	       break;
            case GL_TEXTURE_RED_SIZE:
            case GL_TEXTURE_GREEN_SIZE:
            case GL_TEXTURE_BLUE_SIZE:
            case GL_TEXTURE_ALPHA_SIZE:
            case GL_TEXTURE_INTENSITY_SIZE:
            case GL_TEXTURE_LUMINANCE_SIZE:
               *params = 8;  /* 8-bits */
               break;
            case GL_TEXTURE_INDEX_SIZE_EXT:
               *params = 8;
               break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM,
                         "glGetTexLevelParameter[if]v(pname)" );
	 }
	 break;
      case GL_TEXTURE_2D:
         tex = texUnit->Current2D->Image[level];
         if (!tex)
            return;
	 switch (pname) {
	    case GL_TEXTURE_WIDTH:
	       *params = tex->Width;
	       break;
	    case GL_TEXTURE_HEIGHT:
	       *params = tex->Height;
	       break;
	    case GL_TEXTURE_COMPONENTS:
               /* alias for case GL_TEXTURE_INTERNAL_FORMAT */
	       *params = tex->IntFormat;
	       break;
	    case GL_TEXTURE_BORDER:
	       *params = tex->Border;
	       break;
            case GL_TEXTURE_RED_SIZE:
            case GL_TEXTURE_GREEN_SIZE:
            case GL_TEXTURE_BLUE_SIZE:
            case GL_TEXTURE_ALPHA_SIZE:
            case GL_TEXTURE_INTENSITY_SIZE:
            case GL_TEXTURE_LUMINANCE_SIZE:
               *params = 8;  /* 8-bits */
               break;
            case GL_TEXTURE_INDEX_SIZE_EXT:
               *params = 8;
               break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM,
                         "glGetTexLevelParameter[if]v(pname)" );
	 }
	 break;
      case GL_TEXTURE_3D_EXT:
         tex = texUnit->Current3D->Image[level];
         if (!tex)
            return;
         switch (pname) {
            case GL_TEXTURE_WIDTH:
               *params = tex->Width;
               break;
            case GL_TEXTURE_HEIGHT:
               *params = tex->Height;
               break;
            case GL_TEXTURE_DEPTH_EXT:
               *params = tex->Depth;
               break;
            case GL_TEXTURE_COMPONENTS:
               /* alias for case GL_TEXTURE_INTERNAL_FORMAT */
	       *params = tex->IntFormat;
               break;
            case GL_TEXTURE_BORDER:
               *params = tex->Border;
               break;
            case GL_TEXTURE_INDEX_SIZE_EXT:
               *params = 8;
               break;
            default:
               gl_error( ctx, GL_INVALID_ENUM,
                         "glGetTexLevelParameter[if]v(pname)" );
         }
         break;
      case GL_PROXY_TEXTURE_1D:
         tex = ctx->Texture.Proxy1D->Image[level];
         if (!tex)
            return;
         switch (pname) {
	    case GL_TEXTURE_WIDTH:
	       *params = tex->Width;
	       break;
	    case GL_TEXTURE_COMPONENTS:
               /* alias for case GL_TEXTURE_INTERNAL_FORMAT */
	       *params = tex->IntFormat;
	       break;
	    case GL_TEXTURE_BORDER:
	       *params = tex->Border;
	       break;
            case GL_TEXTURE_RED_SIZE:
            case GL_TEXTURE_GREEN_SIZE:
            case GL_TEXTURE_BLUE_SIZE:
            case GL_TEXTURE_ALPHA_SIZE:
            case GL_TEXTURE_INTENSITY_SIZE:
            case GL_TEXTURE_LUMINANCE_SIZE:
               *params = 8;  /* 8-bits */
               break;
            case GL_TEXTURE_INDEX_SIZE_EXT:
               *params = 8;
               break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM,
                         "glGetTexLevelParameter[if]v(pname)" );
	 }
	 break;
      case GL_PROXY_TEXTURE_2D:
         tex = ctx->Texture.Proxy2D->Image[level];
         if (!tex)
            return;
	 switch (pname) {
	    case GL_TEXTURE_WIDTH:
	       *params = tex->Width;
	       break;
	    case GL_TEXTURE_HEIGHT:
	       *params = tex->Height;
	       break;
	    case GL_TEXTURE_COMPONENTS:
               /* alias for case GL_TEXTURE_INTERNAL_FORMAT */
	       *params = tex->IntFormat;
	       break;
	    case GL_TEXTURE_BORDER:
	       *params = tex->Border;
	       break;
            case GL_TEXTURE_RED_SIZE:
            case GL_TEXTURE_GREEN_SIZE:
            case GL_TEXTURE_BLUE_SIZE:
            case GL_TEXTURE_ALPHA_SIZE:
            case GL_TEXTURE_INTENSITY_SIZE:
            case GL_TEXTURE_LUMINANCE_SIZE:
               *params = 8;  /* 8-bits */
               break;
            case GL_TEXTURE_INDEX_SIZE_EXT:
               *params = 8;
               break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM,
                         "glGetTexLevelParameter[if]v(pname)" );
	 }
	 break;
      case GL_PROXY_TEXTURE_3D_EXT:
         tex = ctx->Texture.Proxy3D->Image[level];
         if (!tex)
            return;
	 switch (pname) {
	    case GL_TEXTURE_WIDTH:
	       *params = tex->Width;
	       break;
	    case GL_TEXTURE_HEIGHT:
	       *params = tex->Height;
	       break;
	    case GL_TEXTURE_DEPTH_EXT:
	       *params = tex->Depth;
	       break;
	    case GL_TEXTURE_COMPONENTS:
               /* alias for case GL_TEXTURE_INTERNAL_FORMAT */
	       *params = tex->IntFormat;
	       break;
	    case GL_TEXTURE_BORDER:
	       *params = tex->Border;
	       break;
            case GL_TEXTURE_RED_SIZE:
            case GL_TEXTURE_GREEN_SIZE:
            case GL_TEXTURE_BLUE_SIZE:
            case GL_TEXTURE_ALPHA_SIZE:
            case GL_TEXTURE_INTENSITY_SIZE:
            case GL_TEXTURE_LUMINANCE_SIZE:
               *params = 8;  /* 8-bits */
               break;
            case GL_TEXTURE_INDEX_SIZE_EXT:
               *params = 8;
               break;
	    default:
	       gl_error( ctx, GL_INVALID_ENUM,
                         "glGetTexLevelParameter[if]v(pname)" );
	 }
	 break;
     default:
	 gl_error(ctx, GL_INVALID_ENUM, "glGetTexLevelParameter[if]v(target)");
   }	 
}




void gl_GetTexParameterfv( GLcontext *ctx,
                           GLenum target, GLenum pname, GLfloat *params )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *obj;

   switch (target) {
      case GL_TEXTURE_1D:
         obj = texUnit->Current1D;
         break;
      case GL_TEXTURE_2D:
         obj = texUnit->Current2D;
         break;
      case GL_TEXTURE_3D_EXT:
         obj = texUnit->Current3D;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetTexParameterfv(target)");
         return;
   }

   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
	 *params = ENUM_TO_FLOAT(obj->MagFilter);
	 break;
      case GL_TEXTURE_MIN_FILTER:
         *params = ENUM_TO_FLOAT(obj->MinFilter);
         break;
      case GL_TEXTURE_WRAP_S:
         *params = ENUM_TO_FLOAT(obj->WrapS);
         break;
      case GL_TEXTURE_WRAP_T:
         *params = ENUM_TO_FLOAT(obj->WrapT);
         break;
      case GL_TEXTURE_WRAP_R_EXT:
         *params = ENUM_TO_FLOAT(obj->WrapR);
         break;
      case GL_TEXTURE_BORDER_COLOR:
         params[0] = obj->BorderColor[0] / 255.0F;
         params[1] = obj->BorderColor[1] / 255.0F;
         params[2] = obj->BorderColor[2] / 255.0F;
         params[3] = obj->BorderColor[3] / 255.0F;
         break;
      case GL_TEXTURE_RESIDENT:
         *params = ENUM_TO_FLOAT(GL_TRUE);
         break;
      case GL_TEXTURE_PRIORITY:
         *params = texUnit->Current1D->Priority;
         break;
      case GL_TEXTURE_MIN_LOD:
         *params = obj->MinLod;
         break;
      case GL_TEXTURE_MAX_LOD:
         *params = obj->MaxLod;
         break;
      case GL_TEXTURE_BASE_LEVEL:
         *params = obj->BaseLevel;
         break;
      case GL_TEXTURE_MAX_LEVEL:
         *params = obj->MaxLevel;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexParameterfv(pname)" );
   }
}


void gl_GetTexParameteriv( GLcontext *ctx,
                           GLenum target, GLenum pname, GLint *params )
{
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[ctx->Texture.CurrentUnit];
   struct gl_texture_object *obj;

   switch (target) {
      case GL_TEXTURE_1D:
         obj = texUnit->Current1D;
         break;
      case GL_TEXTURE_2D:
         obj = texUnit->Current2D;
         break;
      case GL_TEXTURE_3D_EXT:
         obj = texUnit->Current3D;
         break;
      default:
         gl_error(ctx, GL_INVALID_ENUM, "glGetTexParameterfv(target)");
         return;
   }

   switch (pname) {
      case GL_TEXTURE_MAG_FILTER:
         *params = (GLint) obj->MagFilter;
         break;
      case GL_TEXTURE_MIN_FILTER:
         *params = (GLint) obj->MinFilter;
         break;
      case GL_TEXTURE_WRAP_S:
         *params = (GLint) obj->WrapS;
         break;
      case GL_TEXTURE_WRAP_T:
         *params = (GLint) obj->WrapT;
         break;
      case GL_TEXTURE_WRAP_R_EXT:
         *params = (GLint) obj->WrapR;
         break;
      case GL_TEXTURE_BORDER_COLOR:
         {
            GLfloat color[4];
            color[0] = obj->BorderColor[0]/255.0;
            color[1] = obj->BorderColor[1]/255.0;
            color[2] = obj->BorderColor[2]/255.0;
            color[3] = obj->BorderColor[3]/255.0;
            params[0] = FLOAT_TO_INT( color[0] );
            params[1] = FLOAT_TO_INT( color[1] );
            params[2] = FLOAT_TO_INT( color[2] );
            params[3] = FLOAT_TO_INT( color[3] );
         }
         break;
      case GL_TEXTURE_RESIDENT:
         *params = (GLint) GL_TRUE;
         break;
      case GL_TEXTURE_PRIORITY:
         *params = (GLint) obj->Priority;
         break;
      case GL_TEXTURE_MIN_LOD:
         *params = (GLint) obj->MinLod;
         break;
      case GL_TEXTURE_MAX_LOD:
         *params = (GLint) obj->MaxLod;
         break;
      case GL_TEXTURE_BASE_LEVEL:
         *params = obj->BaseLevel;
         break;
      case GL_TEXTURE_MAX_LEVEL:
         *params = obj->MaxLevel;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexParameteriv(pname)" );
   }
}




/**********************************************************************/
/*                    Texture Coord Generation                        */
/**********************************************************************/


void gl_TexGenfv( GLcontext *ctx,
                  GLenum coord, GLenum pname, const GLfloat *params )
{
   GLuint tUnit = ctx->Texture.CurrentTransformUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glTexGenfv" );
      return;
   }

   switch( coord ) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    if (mode==GL_OBJECT_LINEAR ||
		mode==GL_EYE_LINEAR ||
                mode==GL_REFLECTION_MAP_NV ||
                mode==GL_NORMAL_MAP_NV ||
                mode==GL_SPHERE_MAP) {
	       texUnit->GenModeS = mode;
	    }
	    else {
	       gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    texUnit->ObjectPlaneS[0] = params[0];
	    texUnit->ObjectPlaneS[1] = params[1];
	    texUnit->ObjectPlaneS[2] = params[2];
	    texUnit->ObjectPlaneS[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelView.flags & MAT_DIRTY_INVERSE) {
               gl_matrix_analyze( &ctx->ModelView );
            }
            gl_transform_vector( texUnit->EyePlaneS, params,
                                 ctx->ModelView.inv );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    if (mode==GL_OBJECT_LINEAR ||
		mode==GL_EYE_LINEAR ||
                mode==GL_REFLECTION_MAP_NV ||
                mode==GL_NORMAL_MAP_NV ||
		mode==GL_SPHERE_MAP) {
	       texUnit->GenModeT = mode;
	    }
	    else {
	       gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    texUnit->ObjectPlaneT[0] = params[0];
	    texUnit->ObjectPlaneT[1] = params[1];
	    texUnit->ObjectPlaneT[2] = params[2];
	    texUnit->ObjectPlaneT[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelView.flags & MAT_DIRTY_INVERSE) {
               gl_matrix_analyze( &ctx->ModelView );
            }
            gl_transform_vector( texUnit->EyePlaneT, params,
                                 ctx->ModelView.inv );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    if (mode==GL_OBJECT_LINEAR ||
                mode==GL_REFLECTION_MAP_NV ||
                mode==GL_NORMAL_MAP_NV ||
		mode==GL_EYE_LINEAR) {
	       texUnit->GenModeR = mode;
	    }
	    else {
	       gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    texUnit->ObjectPlaneR[0] = params[0];
	    texUnit->ObjectPlaneR[1] = params[1];
	    texUnit->ObjectPlaneR[2] = params[2];
	    texUnit->ObjectPlaneR[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelView.flags & MAT_DIRTY_INVERSE) {
               gl_matrix_analyze( &ctx->ModelView );
            }
            gl_transform_vector( texUnit->EyePlaneR, params,
                                 ctx->ModelView.inv );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
	    GLenum mode = (GLenum) (GLint) *params;
	    if (mode==GL_OBJECT_LINEAR ||
		mode==GL_EYE_LINEAR) {
	       texUnit->GenModeQ = mode;
	    }
	    else {
	       gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(param)" );
	       return;
	    }
	 }
	 else if (pname==GL_OBJECT_PLANE) {
	    texUnit->ObjectPlaneQ[0] = params[0];
	    texUnit->ObjectPlaneQ[1] = params[1];
	    texUnit->ObjectPlaneQ[2] = params[2];
	    texUnit->ObjectPlaneQ[3] = params[3];
	 }
	 else if (pname==GL_EYE_PLANE) {
            /* Transform plane equation by the inverse modelview matrix */
            if (ctx->ModelView.flags & MAT_DIRTY_INVERSE) {
               gl_matrix_analyze( &ctx->ModelView );
            }
            gl_transform_vector( texUnit->EyePlaneQ, params,
                                 ctx->ModelView.inv );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(pname)" );
	    return;
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glTexGenfv(coord)" );
	 return;
   }

   ctx->NewState |= NEW_TEXTURING;
}



void gl_GetTexGendv( GLcontext *ctx,
                     GLenum coord, GLenum pname, GLdouble *params )
{
   GLuint tUnit = ctx->Texture.CurrentTransformUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetTexGendv" );
      return;
   }

   switch( coord ) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeS);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneS );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneS );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeT);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneT );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneT );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeR);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneR );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneR );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_DOUBLE(texUnit->GenModeQ);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneQ );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneQ );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(pname)" );
	    return;
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexGendv(coord)" );
	 return;
   }
}



void gl_GetTexGenfv( GLcontext *ctx,
                     GLenum coord, GLenum pname, GLfloat *params )
{
   GLuint tUnit = ctx->Texture.CurrentTransformUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetTexGenfv" );
      return;
   }

   switch( coord ) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeS);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneS );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneS );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeT);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneT );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneT );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeR);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneR );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneR );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = ENUM_TO_FLOAT(texUnit->GenModeQ);
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneQ );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneQ );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(pname)" );
	    return;
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexGenfv(coord)" );
	 return;
   }
}



void gl_GetTexGeniv( GLcontext *ctx,
                     GLenum coord, GLenum pname, GLint *params )
{
   GLuint tUnit = ctx->Texture.CurrentTransformUnit;
   struct gl_texture_unit *texUnit = &ctx->Texture.Unit[tUnit];
   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glGetTexGeniv" );
      return;
   }

   switch( coord ) {
      case GL_S:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeS;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneS );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneS );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_T:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeT;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneT );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneT );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_R:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeR;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneR );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneR );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      case GL_Q:
         if (pname==GL_TEXTURE_GEN_MODE) {
            params[0] = texUnit->GenModeQ;
	 }
	 else if (pname==GL_OBJECT_PLANE) {
            COPY_4V( params, texUnit->ObjectPlaneQ );
	 }
	 else if (pname==GL_EYE_PLANE) {
            COPY_4V( params, texUnit->EyePlaneQ );
	 }
	 else {
	    gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(pname)" );
	    return;
	 }
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetTexGeniv(coord)" );
	 return;
   }
}



/* GL_SGIS_multitexture */
void gl_SelectTextureSGIS( GLcontext *ctx, GLenum target )
{
   GLint maxUnits = ctx->Const.MaxTextureUnits;
   if (target >= GL_TEXTURE0_SGIS && target < GL_TEXTURE0_SGIS + maxUnits) {
      GLint texUnit = target - GL_TEXTURE0_SGIS;
      /* Select current texture env AND transformation unit! */
      ctx->Texture.CurrentUnit = texUnit;
      ctx->Texture.CurrentTransformUnit = texUnit;
      if (ctx->Driver.ActiveTexture) {
         (*ctx->Driver.ActiveTexture)( ctx, (GLuint) texUnit );
      }
   }
   else {
      gl_error(ctx, GL_INVALID_OPERATION, "glSelectTextureSGIS(target)");
   }
}


/* GL_SGIS_multitexture */
void gl_SelectTextureCoordSet( GLcontext *ctx, GLenum target )
{
   GLint maxUnits = ctx->Const.MaxTextureUnits;
   if (target >= GL_TEXTURE0_SGIS && target < GL_TEXTURE0_SGIS + maxUnits) {
      GLint texUnit = target - GL_TEXTURE0_SGIS;
      ctx->TexCoordUnit = texUnit;
      ctx->Current.TexCoord = ctx->Current.MultiTexCoord[texUnit];
   }
   else {
      gl_error(ctx, GL_INVALID_OPERATION, "glSelectTextureCoordUnit(target)");
   }
}


/* GL_ARB_multitexture */
void gl_ActiveTexture( GLcontext *ctx, GLenum target )
{
   GLint maxUnits = ctx->Const.MaxTextureUnits;
   if (target >= GL_TEXTURE0_ARB && target < GL_TEXTURE0_ARB + maxUnits) {
      GLint texUnit = target - GL_TEXTURE0_ARB;
      ctx->TexCoordUnit = texUnit;
      ctx->Texture.CurrentUnit = texUnit;
      ctx->Texture.CurrentTransformUnit = texUnit;
      ctx->Current.TexCoord = ctx->Current.MultiTexCoord[0];  /* always 0 */
      if (ctx->Driver.ActiveTexture) {
         (*ctx->Driver.ActiveTexture)( ctx, (GLuint) texUnit );
      }
   }
   else {
      gl_error(ctx, GL_INVALID_OPERATION, "glActiveTextureARB(target)");
   }
}


/* GL_ARB_multitexture */
void gl_ClientActiveTexture( GLcontext *ctx, GLenum target )
{
   GLint maxUnits = ctx->Const.MaxTextureUnits;
   if (target >= GL_TEXTURE0_ARB && target < GL_TEXTURE0_ARB + maxUnits) {
      GLint texUnit = target - GL_TEXTURE0_ARB;
      ctx->Array.ActiveTexture = texUnit;
   }
   else {
      gl_error(ctx, GL_INVALID_OPERATION, "glActiveTextureARB(target)");
   }
}



/*
 * This is called by gl_update_state() if the NEW_TEXTURING bit in
 * ctx->NewState is unit.
 */
void gl_update_texture_state( GLcontext *ctx )
{
   struct gl_texture_object *t;

   if (ctx->Texture.AnyDirty) {
      for (t = ctx->Shared->TexObjectList; t; t = t->Next) {
         if (t->Dirty) {
            gl_test_texture_object_completeness(ctx, t);
            gl_set_texture_sampler(t);
            t->Dirty = GL_FALSE;
         }
      }
      ctx->Texture.AnyDirty = GL_FALSE;
   }

   /* The following code will have to be modified if we increase the
    * number of texture units.
    */
   ASSERT(MAX_TEXTURE_UNITS <= 2);

   /*
    * Determine which texture units are really enabled after testing
    * for texture completeness, etc.
    */
   ctx->Texture.ReallyEnabled = 0;
   /* Texture unit 0 */
   if (ctx->Texture.Enabled & TEXTURE0_3D && ctx->Texture.Unit[0].Current3D->Complete) {
      ctx->Texture.ReallyEnabled |= TEXTURE0_3D;
      ctx->Texture.Unit[0].Current = ctx->Texture.Unit[0].Current3D;
   }
   else if (ctx->Texture.Enabled & TEXTURE0_2D && ctx->Texture.Unit[0].Current2D->Complete) {
      ctx->Texture.ReallyEnabled |= TEXTURE0_2D;
      ctx->Texture.Unit[0].Current = ctx->Texture.Unit[0].Current2D;
   }
   else if (ctx->Texture.Enabled & TEXTURE0_1D && ctx->Texture.Unit[0].Current1D->Complete) {
      ctx->Texture.ReallyEnabled |= TEXTURE0_1D;
      ctx->Texture.Unit[0].Current = ctx->Texture.Unit[0].Current1D;
   }
   else {
      ctx->Texture.Unit[0].Current = NULL;
   }
   /* Texture unit 1 */
   if (ctx->Const.MaxTextureUnits > 1) {
      if (ctx->Texture.Enabled & TEXTURE1_3D && ctx->Texture.Unit[1].Current3D->Complete) {
         ctx->Texture.ReallyEnabled |= TEXTURE1_3D;
         ctx->Texture.Unit[1].Current = ctx->Texture.Unit[1].Current3D;
      }
      else if (ctx->Texture.Enabled & TEXTURE1_2D && ctx->Texture.Unit[1].Current2D->Complete) {
         ctx->Texture.ReallyEnabled |= TEXTURE1_2D;
         ctx->Texture.Unit[1].Current = ctx->Texture.Unit[1].Current2D;
      }
      else if (ctx->Texture.Enabled & TEXTURE1_1D && ctx->Texture.Unit[1].Current1D->Complete) {
         ctx->Texture.ReallyEnabled |= TEXTURE1_1D;
         ctx->Texture.Unit[1].Current = ctx->Texture.Unit[1].Current1D;
      }
      else {
         ctx->Texture.Unit[1].Current = NULL;
      }
   }
}
