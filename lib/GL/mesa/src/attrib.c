/* Id: attrib.c,v 1.3 1999/02/26 08:52:30 martin Exp $ */

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
 * Log: attrib.c,v $
 * Revision 1.3  1999/02/26 08:52:30  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.11  1999/02/24 22:48:04  jens
 * Added header file to get XMesa to compile standalone and inside XFree86
 *
 * Revision 3.10  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.9  1999/02/05 03:23:56  brianp
 * fixed a few BeOS compiler warnings
 *
 * Revision 3.8  1999/01/22 04:29:47  brianp
 * lot's of changes: call device driver functions in gl_PopAttrib()
 *
 * Revision 3.7  1998/11/08 22:34:55  brianp
 * fixed texture object reference count bug in glPush/PopAttrib()
 *
 * Revision 3.6  1998/10/31 17:06:15  brianp
 * variety of multi-texture changes
 *
 * Revision 3.5  1998/08/23 22:18:49  brianp
 * added Driver.Viewport and Driver.DepthRange function pointers
 *
 * Revision 3.4  1998/07/29 04:09:52  brianp
 * save/restore of texture state was incomplete
 *
 * Revision 3.3  1998/02/20 04:50:09  brianp
 * implemented GL_SGIS_multitexture
 *
 * Revision 3.2  1998/02/08 20:16:50  brianp
 * removed unneeded headers
 *
 * Revision 3.1  1998/02/01 16:37:19  brianp
 * added GL_EXT_rescale_normal extension
 *
 * Revision 3.0  1998/01/31 20:44:48  brianp
 * initial rev
 *
 */
/* $XFree86: xc/lib/GL/mesa/src/attrib.c,v 1.0tsi Exp $ */

#ifdef PC_HEADER
#include "all.h"
#else
#ifndef XFree86Server
#include <stdlib.h>
#include <string.h>
#endif
#include "attrib.h"
#include "context.h"
#include "macros.h"
#include "glmisc.h"
#include "types.h"
#ifdef XFree86Server
#include "GL/xf86glx.h"
#endif
#endif


#define MALLOC_STRUCT(T)  (struct T *) malloc( sizeof(struct T) )



/*
 * Allocate a new attribute state node.  These nodes have a
 * "kind" value and a pointer to a struct of state data.
 */
static struct gl_attrib_node *new_attrib_node( GLbitfield kind )
{
   struct gl_attrib_node *an;

   an = (struct gl_attrib_node *) malloc( sizeof(struct gl_attrib_node) );
   if (an) {
      an->kind = kind;
   }
   return an;
}



void gl_PushAttrib( GLcontext* ctx, GLbitfield mask )
{
   struct gl_attrib_node *newnode;
   struct gl_attrib_node *head;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPushAttrib" );
      return;
   }

   if (ctx->AttribStackDepth>=MAX_ATTRIB_STACK_DEPTH) {
      gl_error( ctx, GL_STACK_OVERFLOW, "glPushAttrib" );
      return;
   }

   /* Build linked list of attribute nodes which save all attribute */
   /* groups specified by the mask. */
   head = NULL;

   if (mask & GL_ACCUM_BUFFER_BIT) {
      struct gl_accum_attrib *attr;
      attr = MALLOC_STRUCT( gl_accum_attrib );
      MEMCPY( attr, &ctx->Accum, sizeof(struct gl_accum_attrib) );
      newnode = new_attrib_node( GL_ACCUM_BUFFER_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_COLOR_BUFFER_BIT) {
      struct gl_colorbuffer_attrib *attr;
      attr = MALLOC_STRUCT( gl_colorbuffer_attrib );
      MEMCPY( attr, &ctx->Color, sizeof(struct gl_colorbuffer_attrib) );
      newnode = new_attrib_node( GL_COLOR_BUFFER_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_CURRENT_BIT) {
      struct gl_current_attrib *attr;
      attr = MALLOC_STRUCT( gl_current_attrib );
      MEMCPY( attr, &ctx->Current, sizeof(struct gl_current_attrib) );
      newnode = new_attrib_node( GL_CURRENT_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_DEPTH_BUFFER_BIT) {
      struct gl_depthbuffer_attrib *attr;
      attr = MALLOC_STRUCT( gl_depthbuffer_attrib );
      MEMCPY( attr, &ctx->Depth, sizeof(struct gl_depthbuffer_attrib) );
      newnode = new_attrib_node( GL_DEPTH_BUFFER_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_ENABLE_BIT) {
      struct gl_enable_attrib *attr;
      GLuint i;
      attr = MALLOC_STRUCT( gl_enable_attrib );
      /* Copy enable flags from all other attributes into the enable struct. */
      attr->AlphaTest = ctx->Color.AlphaEnabled;
      attr->AutoNormal = ctx->Eval.AutoNormal;
      attr->Blend = ctx->Color.BlendEnabled;
      for (i=0;i<MAX_CLIP_PLANES;i++) {
         attr->ClipPlane[i] = ctx->Transform.ClipEnabled[i];
      }
      attr->ColorMaterial = ctx->Light.ColorMaterialEnabled;
      attr->CullFace = ctx->Polygon.CullFlag;
      attr->DepthTest = ctx->Depth.Test;
      attr->Dither = ctx->Color.DitherFlag;
      attr->Fog = ctx->Fog.Enabled;
      for (i=0;i<MAX_LIGHTS;i++) {
         attr->Light[i] = ctx->Light.Light[i].Enabled;
      }
      attr->Lighting = ctx->Light.Enabled;
      attr->LineSmooth = ctx->Line.SmoothFlag;
      attr->LineStipple = ctx->Line.StippleFlag;
      attr->IndexLogicOp = ctx->Color.IndexLogicOpEnabled;
      attr->ColorLogicOp = ctx->Color.ColorLogicOpEnabled;
      attr->Map1Color4 = ctx->Eval.Map1Color4;
      attr->Map1Index = ctx->Eval.Map1Index;
      attr->Map1Normal = ctx->Eval.Map1Normal;
      attr->Map1TextureCoord1 = ctx->Eval.Map1TextureCoord1;
      attr->Map1TextureCoord2 = ctx->Eval.Map1TextureCoord2;
      attr->Map1TextureCoord3 = ctx->Eval.Map1TextureCoord3;
      attr->Map1TextureCoord4 = ctx->Eval.Map1TextureCoord4;
      attr->Map1Vertex3 = ctx->Eval.Map1Vertex3;
      attr->Map1Vertex4 = ctx->Eval.Map1Vertex4;
      attr->Map2Color4 = ctx->Eval.Map2Color4;
      attr->Map2Index = ctx->Eval.Map2Index;
      attr->Map2Normal = ctx->Eval.Map2Normal;
      attr->Map2TextureCoord1 = ctx->Eval.Map2TextureCoord1;
      attr->Map2TextureCoord2 = ctx->Eval.Map2TextureCoord2;
      attr->Map2TextureCoord3 = ctx->Eval.Map2TextureCoord3;
      attr->Map2TextureCoord4 = ctx->Eval.Map2TextureCoord4;
      attr->Map2Vertex3 = ctx->Eval.Map2Vertex3;
      attr->Map2Vertex4 = ctx->Eval.Map2Vertex4;
      attr->Normalize = ctx->Transform.Normalize;
      attr->PointSmooth = ctx->Point.SmoothFlag;
      attr->PolygonOffsetPoint = ctx->Polygon.OffsetPoint;
      attr->PolygonOffsetLine = ctx->Polygon.OffsetLine;
      attr->PolygonOffsetFill = ctx->Polygon.OffsetFill;
      attr->PolygonSmooth = ctx->Polygon.SmoothFlag;
      attr->PolygonStipple = ctx->Polygon.StippleFlag;
      attr->RescaleNormals = ctx->Transform.RescaleNormals;
      attr->Scissor = ctx->Scissor.Enabled;
      attr->Stencil = ctx->Stencil.Enabled;
      attr->Texture = ctx->Texture.Enabled;
      for (i=0; i<MAX_TEXTURE_UNITS; i++) {
         attr->TexGen[i] = ctx->Texture.Unit[i].TexGenEnabled;
      }
      newnode = new_attrib_node( GL_ENABLE_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_EVAL_BIT) {
      struct gl_eval_attrib *attr;
      attr = MALLOC_STRUCT( gl_eval_attrib );
      MEMCPY( attr, &ctx->Eval, sizeof(struct gl_eval_attrib) );
      newnode = new_attrib_node( GL_EVAL_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_FOG_BIT) {
      struct gl_fog_attrib *attr;
      attr = MALLOC_STRUCT( gl_fog_attrib );
      MEMCPY( attr, &ctx->Fog, sizeof(struct gl_fog_attrib) );
      newnode = new_attrib_node( GL_FOG_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_HINT_BIT) {
      struct gl_hint_attrib *attr;
      attr = MALLOC_STRUCT( gl_hint_attrib );
      MEMCPY( attr, &ctx->Hint, sizeof(struct gl_hint_attrib) );
      newnode = new_attrib_node( GL_HINT_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_LIGHTING_BIT) {
      struct gl_light_attrib *attr;
      attr = MALLOC_STRUCT( gl_light_attrib );
      MEMCPY( attr, &ctx->Light, sizeof(struct gl_light_attrib) );
      newnode = new_attrib_node( GL_LIGHTING_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_LINE_BIT) {
      struct gl_line_attrib *attr;
      attr = MALLOC_STRUCT( gl_line_attrib );
      MEMCPY( attr, &ctx->Line, sizeof(struct gl_line_attrib) );
      newnode = new_attrib_node( GL_LINE_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_LIST_BIT) {
      struct gl_list_attrib *attr;
      attr = MALLOC_STRUCT( gl_list_attrib );
      MEMCPY( attr, &ctx->List, sizeof(struct gl_list_attrib) );
      newnode = new_attrib_node( GL_LIST_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_PIXEL_MODE_BIT) {
      struct gl_pixel_attrib *attr;
      attr = MALLOC_STRUCT( gl_pixel_attrib );
      MEMCPY( attr, &ctx->Pixel, sizeof(struct gl_pixel_attrib) );
      newnode = new_attrib_node( GL_PIXEL_MODE_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_POINT_BIT) {
      struct gl_point_attrib *attr;
      attr = MALLOC_STRUCT( gl_point_attrib );
      MEMCPY( attr, &ctx->Point, sizeof(struct gl_point_attrib) );
      newnode = new_attrib_node( GL_POINT_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_POLYGON_BIT) {
      struct gl_polygon_attrib *attr;
      attr = MALLOC_STRUCT( gl_polygon_attrib );
      MEMCPY( attr, &ctx->Polygon, sizeof(struct gl_polygon_attrib) );
      newnode = new_attrib_node( GL_POLYGON_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_POLYGON_STIPPLE_BIT) {
      GLuint *stipple;
      stipple = (GLuint *) malloc( 32*sizeof(GLuint) );
      MEMCPY( stipple, ctx->PolygonStipple, 32*sizeof(GLuint) );
      newnode = new_attrib_node( GL_POLYGON_STIPPLE_BIT );
      newnode->data = stipple;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_SCISSOR_BIT) {
      struct gl_scissor_attrib *attr;
      attr = MALLOC_STRUCT( gl_scissor_attrib );
      MEMCPY( attr, &ctx->Scissor, sizeof(struct gl_scissor_attrib) );
      newnode = new_attrib_node( GL_SCISSOR_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_STENCIL_BUFFER_BIT) {
      struct gl_stencil_attrib *attr;
      attr = MALLOC_STRUCT( gl_stencil_attrib );
      MEMCPY( attr, &ctx->Stencil, sizeof(struct gl_stencil_attrib) );
      newnode = new_attrib_node( GL_STENCIL_BUFFER_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_TEXTURE_BIT) {
      struct gl_texture_attrib *attr;
      GLuint u;
      /* Take care of texture object reference counters */
      for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
         if (ctx->Texture.Unit[u].Current1D->Name > 0)
            ctx->Texture.Unit[u].Current1D->RefCount++;
         if (ctx->Texture.Unit[u].Current2D->Name > 0)
            ctx->Texture.Unit[u].Current2D->RefCount++;
         if (ctx->Texture.Unit[u].Current3D->Name > 0)
            ctx->Texture.Unit[u].Current3D->RefCount++;
      }
      attr = MALLOC_STRUCT( gl_texture_attrib );
      MEMCPY( attr, &ctx->Texture, sizeof(struct gl_texture_attrib) );
      newnode = new_attrib_node( GL_TEXTURE_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_TRANSFORM_BIT) {
      struct gl_transform_attrib *attr;
      attr = MALLOC_STRUCT( gl_transform_attrib );
      MEMCPY( attr, &ctx->Transform, sizeof(struct gl_transform_attrib) );
      newnode = new_attrib_node( GL_TRANSFORM_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   if (mask & GL_VIEWPORT_BIT) {
      struct gl_viewport_attrib *attr;
      attr = MALLOC_STRUCT( gl_viewport_attrib );
      MEMCPY( attr, &ctx->Viewport, sizeof(struct gl_viewport_attrib) );
      newnode = new_attrib_node( GL_VIEWPORT_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   ctx->AttribStack[ctx->AttribStackDepth] = head;
   ctx->AttribStackDepth++;
}



/*
 * This function is kind of long just because we have to call a lot
 * of device driver functions to update device driver state.
 */
void gl_PopAttrib( GLcontext* ctx )
{
   struct gl_attrib_node *attr, *next;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPopAttrib" );
      return;
   }

   if (ctx->AttribStackDepth==0) {
      gl_error( ctx, GL_STACK_UNDERFLOW, "glPopAttrib" );
      return;
   }

   ctx->AttribStackDepth--;
   attr = ctx->AttribStack[ctx->AttribStackDepth];

   while (attr) {
      switch (attr->kind) {
         case GL_ACCUM_BUFFER_BIT:
            MEMCPY( &ctx->Accum, attr->data, sizeof(struct gl_accum_attrib) );
            break;
         case GL_COLOR_BUFFER_BIT:
            {
               GLenum oldDrawBuffer = ctx->Color.DrawBuffer;
               GLenum oldAlphaFunc = ctx->Color.AlphaFunc;
               GLubyte oldAlphaRef = ctx->Color.AlphaRef;
               GLenum oldBlendSrc = ctx->Color.BlendSrcRGB;
               GLenum oldBlendDst = ctx->Color.BlendDstRGB;
               MEMCPY( &ctx->Color, attr->data,
                       sizeof(struct gl_colorbuffer_attrib) );
               if (ctx->Color.DrawBuffer != oldDrawBuffer) {
                  gl_DrawBuffer(ctx, ctx->Color.DrawBuffer);
               }
               if ((ctx->Color.AlphaFunc != oldAlphaFunc ||
                    ctx->Color.AlphaRef != oldAlphaRef) &&
                   ctx->Driver.AlphaFunc)
                  (*ctx->Driver.AlphaFunc)( ctx, ctx->Color.AlphaFunc,
                                            ctx->Color.AlphaRef / 255.0F);
               if ((ctx->Color.BlendSrcRGB != oldBlendSrc ||
                    ctx->Color.BlendSrcRGB != oldBlendDst) &&
                   ctx->Driver.BlendFunc)
                  (*ctx->Driver.BlendFunc)( ctx, ctx->Color.BlendSrcRGB,
                                            ctx->Color.BlendDstRGB);
            }
            break;
         case GL_CURRENT_BIT:
            MEMCPY( &ctx->Current, attr->data,
		    sizeof(struct gl_current_attrib) );
            break;
         case GL_DEPTH_BUFFER_BIT:
            {
               GLenum oldDepthFunc = ctx->Depth.Func;
               GLboolean oldDepthMask = ctx->Depth.Mask;
               GLfloat oldDepthClear = ctx->Depth.Clear;
               MEMCPY( &ctx->Depth, attr->data,
                       sizeof(struct gl_depthbuffer_attrib) );
               if (ctx->Depth.Func != oldDepthFunc && ctx->Driver.DepthFunc)
                  (*ctx->Driver.DepthFunc)( ctx, ctx->Depth.Func );
               if (ctx->Depth.Mask != oldDepthMask && ctx->Driver.DepthMask)
                  (*ctx->Driver.DepthMask)( ctx, ctx->Depth.Mask );
               if (ctx->Depth.Clear != oldDepthClear && ctx->Driver.ClearDepth)
                  (*ctx->Driver.ClearDepth)( ctx, ctx->Depth.Clear );
            }
            break;
         case GL_ENABLE_BIT:
            {
               const struct gl_enable_attrib *enable;
               enable = (const struct gl_enable_attrib *) attr->data;

#define TEST_AND_UPDATE(VALUE, NEWVALUE, ENUM)				\
	if ((VALUE) != (NEWVALUE)) {					\
		VALUE = (NEWVALUE);					\
		if (ctx->Driver.Enable)					\
			(*ctx->Driver.Enable)( ctx, ENUM, (NEWVALUE) );	\
	}

               TEST_AND_UPDATE(ctx->Color.AlphaEnabled, enable->AlphaTest, GL_ALPHA_TEST);
               TEST_AND_UPDATE(ctx->Transform.Normalize, enable->AutoNormal, GL_NORMALIZE);
               TEST_AND_UPDATE(ctx->Color.BlendEnabled, enable->Blend, GL_BLEND);
               {
                  GLuint i;
                  for (i=0;i<MAX_CLIP_PLANES;i++) {
                     if (ctx->Transform.ClipEnabled[i] != enable->ClipPlane[i]) {
                        ctx->Transform.ClipEnabled[i] = enable->ClipPlane[i];
                        if (ctx->Driver.Enable) {
                           GLenum plane = (GLenum) (GL_CLIP_PLANE0 + i);
                           (*ctx->Driver.Enable)( ctx, plane,
                                                  enable->ClipPlane[i]);
                        }
                     }
                  }
               }
               TEST_AND_UPDATE(ctx->Light.ColorMaterialEnabled, enable->ColorMaterial, GL_COLOR_MATERIAL);
               TEST_AND_UPDATE(ctx->Polygon.CullFlag, enable->CullFace, GL_CULL_FACE);
               TEST_AND_UPDATE(ctx->Color.DitherFlag, enable->Dither, GL_DITHER);
               TEST_AND_UPDATE(ctx->Fog.Enabled, enable->Fog, GL_FOG);
               TEST_AND_UPDATE(ctx->Light.Enabled, enable->Lighting, GL_LIGHTING);
               TEST_AND_UPDATE(ctx->Line.SmoothFlag, enable->LineSmooth, GL_LINE_SMOOTH);
               TEST_AND_UPDATE(ctx->Line.StippleFlag, enable->LineStipple, GL_LINE_STIPPLE);
               TEST_AND_UPDATE(ctx->Color.IndexLogicOpEnabled, enable->IndexLogicOp, GL_INDEX_LOGIC_OP);
               TEST_AND_UPDATE(ctx->Color.ColorLogicOpEnabled, enable->ColorLogicOp, GL_COLOR_LOGIC_OP);
               TEST_AND_UPDATE(ctx->Eval.Map1Color4, enable->Map1Color4, GL_MAP1_COLOR_4);
               TEST_AND_UPDATE(ctx->Eval.Map1Index, enable->Map1Index, GL_MAP1_INDEX);
               TEST_AND_UPDATE(ctx->Eval.Map1Normal, enable->Map1Normal, GL_MAP1_NORMAL);
               TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord1, enable->Map1TextureCoord1, GL_MAP1_TEXTURE_COORD_1);
               TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord2, enable->Map1TextureCoord2, GL_MAP1_TEXTURE_COORD_2);
               TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord3, enable->Map1TextureCoord3, GL_MAP1_TEXTURE_COORD_3);
               TEST_AND_UPDATE(ctx->Eval.Map1TextureCoord4, enable->Map1TextureCoord4, GL_MAP1_TEXTURE_COORD_4);
               TEST_AND_UPDATE(ctx->Eval.Map1Vertex3, enable->Map1Vertex3, GL_MAP1_VERTEX_3);
               TEST_AND_UPDATE(ctx->Eval.Map1Vertex4, enable->Map1Vertex4, GL_MAP1_VERTEX_4);
               TEST_AND_UPDATE(ctx->Eval.Map2Color4, enable->Map2Color4, GL_MAP2_COLOR_4);
               TEST_AND_UPDATE(ctx->Eval.Map2Index, enable->Map2Index, GL_MAP2_INDEX);
               TEST_AND_UPDATE(ctx->Eval.Map2Normal, enable->Map2Normal, GL_MAP2_NORMAL);
               TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord1, enable->Map2TextureCoord1, GL_MAP2_TEXTURE_COORD_1);
               TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord2, enable->Map2TextureCoord2, GL_MAP2_TEXTURE_COORD_2);
               TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord3, enable->Map2TextureCoord3, GL_MAP2_TEXTURE_COORD_3);
               TEST_AND_UPDATE(ctx->Eval.Map2TextureCoord4, enable->Map2TextureCoord4, GL_MAP2_TEXTURE_COORD_4);
               TEST_AND_UPDATE(ctx->Eval.Map2Vertex3, enable->Map2Vertex3, GL_MAP2_VERTEX_3);
               TEST_AND_UPDATE(ctx->Eval.Map2Vertex4, enable->Map2Vertex4, GL_MAP2_VERTEX_4);
               TEST_AND_UPDATE(ctx->Transform.Normalize, enable->Normalize, GL_NORMALIZE);
               TEST_AND_UPDATE(ctx->Transform.RescaleNormals, enable->RescaleNormals, GL_RESCALE_NORMAL_EXT);
               TEST_AND_UPDATE(ctx->Point.SmoothFlag, enable->PointSmooth, GL_POINT_SMOOTH);
               TEST_AND_UPDATE(ctx->Polygon.OffsetPoint, enable->PolygonOffsetPoint, GL_POLYGON_OFFSET_POINT);
               TEST_AND_UPDATE(ctx->Polygon.OffsetLine, enable->PolygonOffsetLine, GL_POLYGON_OFFSET_LINE);
               TEST_AND_UPDATE(ctx->Polygon.OffsetFill, enable->PolygonOffsetFill, GL_POLYGON_OFFSET_FILL);
               ctx->Polygon.OffsetAny = ctx->Polygon.OffsetPoint ||
                                        ctx->Polygon.OffsetLine ||
                                        ctx->Polygon.OffsetFill;
               TEST_AND_UPDATE(ctx->Polygon.SmoothFlag, enable->PolygonSmooth, GL_POLYGON_SMOOTH);
               TEST_AND_UPDATE(ctx->Polygon.StippleFlag, enable->PolygonStipple, GL_POLYGON_STIPPLE);
               TEST_AND_UPDATE(ctx->Scissor.Enabled, enable->Scissor, GL_SCISSOR_TEST);
               TEST_AND_UPDATE(ctx->Stencil.Enabled, enable->Stencil, GL_STENCIL_TEST);
               if (ctx->Texture.Enabled != enable->Texture) {
                  ctx->Texture.Enabled = enable->Texture;
                  if (ctx->Driver.Enable) {
                     if (ctx->Driver.ActiveTexture)
                        (*ctx->Driver.ActiveTexture)( ctx, 0 );
                     (*ctx->Driver.Enable)( ctx, GL_TEXTURE_1D, (GLboolean) (enable->Texture & TEXTURE0_1D) );
                     (*ctx->Driver.Enable)( ctx, GL_TEXTURE_2D, (GLboolean) (enable->Texture & TEXTURE0_2D) );
                     (*ctx->Driver.Enable)( ctx, GL_TEXTURE_3D, (GLboolean) (enable->Texture & TEXTURE0_3D) );
                     if (ctx->Driver.ActiveTexture)
                        (*ctx->Driver.ActiveTexture)( ctx, 1 );
                     (*ctx->Driver.Enable)( ctx, GL_TEXTURE_1D, (GLboolean) (enable->Texture & TEXTURE1_1D) );
                     (*ctx->Driver.Enable)( ctx, GL_TEXTURE_2D, (GLboolean) (enable->Texture & TEXTURE1_2D) );
                     (*ctx->Driver.Enable)( ctx, GL_TEXTURE_3D, (GLboolean) (enable->Texture & TEXTURE1_3D) );
                     if (ctx->Driver.ActiveTexture)
                        (*ctx->Driver.ActiveTexture)( ctx, ctx->Texture.CurrentUnit );
                  }
               }
#undef TEST_AND_UPDATE
               {
                  GLuint i;
                  for (i=0; i<MAX_TEXTURE_UNITS; i++) {
                     if (ctx->Texture.Unit[i].TexGenEnabled != enable->TexGen[i]) {
                        ctx->Texture.Unit[i].TexGenEnabled = enable->TexGen[i];
                        if (ctx->Driver.Enable) {
                           if (ctx->Driver.ActiveTexture)
                              (*ctx->Driver.ActiveTexture)( ctx, i );
                           if (enable->TexGen[i] & S_BIT)
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_S, GL_TRUE);
                           else
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_S, GL_FALSE);
                           if (enable->TexGen[i] & T_BIT)
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_T, GL_TRUE);
                           else
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_T, GL_FALSE);
                           if (enable->TexGen[i] & R_BIT)
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_R, GL_TRUE);
                           else
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_R, GL_FALSE);
                           if (enable->TexGen[i] & Q_BIT)
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_Q, GL_TRUE);
                           else
                              (*ctx->Driver.Enable)( ctx, GL_TEXTURE_GEN_Q, GL_FALSE);
                        }
                     }
                  }
                  if (ctx->Driver.ActiveTexture)
                     (*ctx->Driver.ActiveTexture)( ctx, ctx->Texture.CurrentUnit );
               }
            }
            break;
         case GL_EVAL_BIT:
            MEMCPY( &ctx->Eval, attr->data, sizeof(struct gl_eval_attrib) );
            break;
         case GL_FOG_BIT:
            {
               GLboolean anyChange = (memcmp( &ctx->Fog, attr->data, sizeof(struct gl_fog_attrib) ) != 0);
               MEMCPY( &ctx->Fog, attr->data, sizeof(struct gl_fog_attrib) );
               if (anyChange && ctx->Driver.Fogfv) {
                  const GLfloat mode = ctx->Fog.Mode;
                  const GLfloat density = ctx->Fog.Density;
                  const GLfloat start = ctx->Fog.Start;
                  const GLfloat end = ctx->Fog.End;
                  const GLfloat index = ctx->Fog.Index;
                  (*ctx->Driver.Fogfv)( ctx, GL_FOG_MODE, &mode);
                  (*ctx->Driver.Fogfv)( ctx, GL_FOG_DENSITY, &density );
                  (*ctx->Driver.Fogfv)( ctx, GL_FOG_START, &start );
                  (*ctx->Driver.Fogfv)( ctx, GL_FOG_END, &end );
                  (*ctx->Driver.Fogfv)( ctx, GL_FOG_INDEX, &index );
                  (*ctx->Driver.Fogfv)( ctx, GL_FOG_COLOR, ctx->Fog.Color );
               }
            }
            break;
         case GL_HINT_BIT:
            MEMCPY( &ctx->Hint, attr->data, sizeof(struct gl_hint_attrib) );
            if (ctx->Driver.Hint) {
               (*ctx->Driver.Hint)( ctx, GL_PERSPECTIVE_CORRECTION_HINT,
                                    ctx->Hint.PerspectiveCorrection );
               (*ctx->Driver.Hint)( ctx, GL_POINT_SMOOTH_HINT,
                                    ctx->Hint.PointSmooth);
               (*ctx->Driver.Hint)( ctx, GL_LINE_SMOOTH_HINT,
                                    ctx->Hint.LineSmooth );
               (*ctx->Driver.Hint)( ctx, GL_POLYGON_SMOOTH_HINT,
                                    ctx->Hint.PolygonSmooth );
               (*ctx->Driver.Hint)( ctx, GL_FOG_HINT, ctx->Hint.Fog );
            }
            break;
         case GL_LIGHTING_BIT:
            MEMCPY( &ctx->Light, attr->data, sizeof(struct gl_light_attrib) );
            if (ctx->Driver.Enable) {
               GLuint i;
               for (i = 0; i < MAX_LIGHTS; i++) {
                  GLenum light = (GLenum) (GL_LIGHT0 + i);
                  (*ctx->Driver.Enable)( ctx, light, ctx->Light.Light[i].Enabled );
               }
               (*ctx->Driver.Enable)( ctx, GL_LIGHTING, ctx->Light.Enabled );
            }
            break;
         case GL_LINE_BIT:
            MEMCPY( &ctx->Line, attr->data, sizeof(struct gl_line_attrib) );
            if (ctx->Driver.Enable) {
               (*ctx->Driver.Enable)( ctx, GL_LINE_SMOOTH, ctx->Line.SmoothFlag );
               (*ctx->Driver.Enable)( ctx, GL_LINE_STIPPLE, ctx->Line.StippleFlag );
            }
            break;
         case GL_LIST_BIT:
            MEMCPY( &ctx->List, attr->data, sizeof(struct gl_list_attrib) );
            break;
         case GL_PIXEL_MODE_BIT:
            MEMCPY( &ctx->Pixel, attr->data, sizeof(struct gl_pixel_attrib) );
            break;
         case GL_POINT_BIT:
            MEMCPY( &ctx->Point, attr->data, sizeof(struct gl_point_attrib) );
            if (ctx->Driver.Enable)
               (*ctx->Driver.Enable)( ctx, GL_POINT_SMOOTH, ctx->Point.SmoothFlag );
            break;
         case GL_POLYGON_BIT:
            {
               GLenum oldFrontMode = ctx->Polygon.FrontMode;
               GLenum oldBackMode = ctx->Polygon.BackMode;
               MEMCPY( &ctx->Polygon, attr->data,
                       sizeof(struct gl_polygon_attrib) );
               if ((ctx->Polygon.FrontMode != oldFrontMode ||
                    ctx->Polygon.BackMode != oldBackMode) &&
                   ctx->Driver.PolygonMode) {
                  (*ctx->Driver.PolygonMode)( ctx, GL_FRONT, ctx->Polygon.FrontMode);
                  (*ctx->Driver.PolygonMode)( ctx, GL_BACK, ctx->Polygon.BackMode);
               }
               if (ctx->Driver.Enable)
                  (*ctx->Driver.Enable)( ctx, GL_POLYGON_SMOOTH, ctx->Polygon.SmoothFlag );
            }
            break;
	 case GL_POLYGON_STIPPLE_BIT:
	    MEMCPY( ctx->PolygonStipple, attr->data, 32*sizeof(GLuint) );
	    break;
         case GL_SCISSOR_BIT:
            MEMCPY( &ctx->Scissor, attr->data,
		    sizeof(struct gl_scissor_attrib) );
            if (ctx->Driver.Enable)
               (*ctx->Driver.Enable)( ctx, GL_SCISSOR_TEST, ctx->Scissor.Enabled );
            break;
         case GL_STENCIL_BUFFER_BIT:
            MEMCPY( &ctx->Stencil, attr->data,
		    sizeof(struct gl_stencil_attrib) );
            if (ctx->Driver.StencilFunc)
               (*ctx->Driver.StencilFunc)( ctx, ctx->Stencil.Function,
                                   ctx->Stencil.Ref, ctx->Stencil.ValueMask);
            if (ctx->Driver.StencilMask)
               (*ctx->Driver.StencilMask)( ctx, ctx->Stencil.WriteMask );
            if (ctx->Driver.StencilOp)
               (*ctx->Driver.StencilOp)( ctx, ctx->Stencil.FailFunc,
                              ctx->Stencil.ZFailFunc, ctx->Stencil.ZPassFunc);
            if (ctx->Driver.Enable)
               (*ctx->Driver.Enable)( ctx, GL_STENCIL_TEST, ctx->Stencil.Enabled );
            break;
         case GL_TRANSFORM_BIT:
            MEMCPY( &ctx->Transform, attr->data,
		    sizeof(struct gl_transform_attrib) );
            if (ctx->Driver.Enable) {
               (*ctx->Driver.Enable)( ctx, GL_NORMALIZE, ctx->Transform.Normalize );
               (*ctx->Driver.Enable)( ctx, GL_RESCALE_NORMAL_EXT, ctx->Transform.RescaleNormals );
            }
            break;
         case GL_TEXTURE_BIT:
            /* Take care of texture object reference counters */
            {
               GLuint u;
               for (u = 0; u < ctx->Const.MaxTextureUnits; u++) {
                  if (ctx->Texture.Unit[u].Current1D->Name > 0)
                     ctx->Texture.Unit[u].Current1D->RefCount--;
                  if (ctx->Texture.Unit[u].Current2D->Name > 0)
                     ctx->Texture.Unit[u].Current2D->RefCount--;
                  if (ctx->Texture.Unit[u].Current2D->Name > 0)
                     ctx->Texture.Unit[u].Current3D->RefCount--;
               }
            }
            MEMCPY( &ctx->Texture, attr->data,
		    sizeof(struct gl_texture_attrib) );
            break;
         case GL_VIEWPORT_BIT:
            MEMCPY( &ctx->Viewport, attr->data,
		    sizeof(struct gl_viewport_attrib) );
            if (ctx->Driver.Viewport) {
               (*ctx->Driver.Viewport)( ctx, ctx->Viewport.X, ctx->Viewport.Y,
                                  ctx->Viewport.Width, ctx->Viewport.Height );
            }
            if (ctx->Driver.DepthRange) {
               (*ctx->Driver.DepthRange)( ctx, ctx->Viewport.Near,
                                          ctx->Viewport.Far );
            }
            break;
         default:
            gl_problem( ctx, "Bad attrib flag in PopAttrib");
            break;
      }

      next = attr->next;
      free( (void *) attr->data );
      free( (void *) attr );
      attr = next;
   }

   ctx->NewState = NEW_ALL;
}


#define GL_CLIENT_PACK_BIT (1<<20)
#define GL_CLIENT_UNPACK_BIT (1<<21)


void gl_PushClientAttrib( GLcontext *ctx, GLbitfield mask )
{
   struct gl_attrib_node *newnode;
   struct gl_attrib_node *head;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPushClientAttrib" );
      return;
   }

   if (ctx->ClientAttribStackDepth>=MAX_CLIENT_ATTRIB_STACK_DEPTH) {
      gl_error( ctx, GL_STACK_OVERFLOW, "glPushClientAttrib" );
      return;
   }

   /* Build linked list of attribute nodes which save all attribute */
   /* groups specified by the mask. */
   head = NULL;

   if (mask & GL_CLIENT_PIXEL_STORE_BIT) {
      struct gl_pixelstore_attrib *attr;
      /* packing attribs */
      attr = MALLOC_STRUCT( gl_pixelstore_attrib );
      MEMCPY( attr, &ctx->Pack, sizeof(struct gl_pixelstore_attrib) );
      newnode = new_attrib_node( GL_CLIENT_PACK_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
      /* unpacking attribs */
      attr = MALLOC_STRUCT( gl_pixelstore_attrib );
      MEMCPY( attr, &ctx->Unpack, sizeof(struct gl_pixelstore_attrib) );
      newnode = new_attrib_node( GL_CLIENT_UNPACK_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }
   if (mask & GL_CLIENT_VERTEX_ARRAY_BIT) {
      struct gl_array_attrib *attr;
      attr = MALLOC_STRUCT( gl_array_attrib );
      MEMCPY( attr, &ctx->Array, sizeof(struct gl_array_attrib) );
      newnode = new_attrib_node( GL_CLIENT_VERTEX_ARRAY_BIT );
      newnode->data = attr;
      newnode->next = head;
      head = newnode;
   }

   ctx->ClientAttribStack[ctx->ClientAttribStackDepth] = head;
   ctx->ClientAttribStackDepth++;
}




void gl_PopClientAttrib( GLcontext *ctx )
{
   struct gl_attrib_node *attr, *next;

   if (INSIDE_BEGIN_END(ctx)) {
      gl_error( ctx, GL_INVALID_OPERATION, "glPopClientAttrib" );
      return;
   }

   if (ctx->ClientAttribStackDepth==0) {
      gl_error( ctx, GL_STACK_UNDERFLOW, "glPopClientAttrib" );
      return;
   }

   ctx->ClientAttribStackDepth--;
   attr = ctx->ClientAttribStack[ctx->ClientAttribStackDepth];

   while (attr) {
      switch (attr->kind) {
         case GL_CLIENT_PACK_BIT:
            MEMCPY( &ctx->Pack, attr->data,
                    sizeof(struct gl_pixelstore_attrib) );
            break;
         case GL_CLIENT_UNPACK_BIT:
            MEMCPY( &ctx->Unpack, attr->data,
                    sizeof(struct gl_pixelstore_attrib) );
            break;
         case GL_CLIENT_VERTEX_ARRAY_BIT:
            MEMCPY( &ctx->Array, attr->data,
		    sizeof(struct gl_array_attrib) );
            break;
         default:
            gl_problem( ctx, "Bad attrib flag in PopClientAttrib");
            break;
      }

      next = attr->next;
      free( (void *) attr->data );
      free( (void *) attr );
      attr = next;
   }

   ctx->NewState = NEW_ALL;
}

