/* $XFree86$ */
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
 * $PI: xc/lib/GL/glx/dri_glapi.c,v 1.2 1999/04/05 05:24:31 martin Exp $
 */

#ifdef GLX_DIRECT_RENDERING

#include <GL/gl.h>
#include "dri_glapi.h"
#include "glxclient.h"

#define __DRI_GET_CONTEXT(_gc) __GLXcontext *_gc = __glXGetCurrentContext()

void glAccum(GLenum op, GLfloat value)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Accum)(op, value);
}

void glAlphaFunc(GLenum func, GLclampf ref)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.AlphaFunc)(func, ref);
}

GLboolean glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
    __DRI_GET_CONTEXT(gc);
    return (*gc->glAPI.AreTexturesResident)(n, textures, residences);
}

void glArrayElement(GLint i)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ArrayElement)(i);
}

void glBegin(GLenum mode)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Begin)(mode);
}

void glBindTexture(GLenum target, GLuint texture)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.BindTexture)(target, texture);
}

void glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Bitmap)(width, height, xorig, yorig, xmove, ymove, bitmap);
}

void glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.BlendFunc)(sfactor, dfactor);
}

void glCallList(GLuint list)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.CallList)(list);
}

void glCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.CallLists)(n, type, lists);
}

void glClear(GLbitfield mask)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Clear)(mask);
}

void glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ClearAccum)(red, green, blue, alpha);
}

void glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ClearColor)(red, green, blue, alpha);
}

void glClearDepth(GLclampd depth)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ClearDepth)(depth);
}

void glClearIndex(GLfloat c)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ClearIndex)(c);
}

void glClearStencil(GLint s)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ClearStencil)(s);
}

void glClipPlane(GLenum plane, const GLdouble *equation)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ClipPlane)(plane, equation);
}

void glColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3b)(red, green, blue);
}

void glColor3bv(const GLbyte *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3bv)(v);
}

void glColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3d)(red, green, blue);
}

void glColor3dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3dv)(v);
}

void glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3f)(red, green, blue);
}

void glColor3fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3fv)(v);
}

void glColor3i(GLint red, GLint green, GLint blue)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3i)(red, green, blue);
}

void glColor3iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3iv)(v);
}

void glColor3s(GLshort red, GLshort green, GLshort blue)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3s)(red, green, blue);
}

void glColor3sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3sv)(v);
}

void glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3ub)(red, green, blue);
}

void glColor3ubv(const GLubyte *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3ubv)(v);
}

void glColor3ui(GLuint red, GLuint green, GLuint blue)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3ui)(red, green, blue);
}

void glColor3uiv(const GLuint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3uiv)(v);
}

void glColor3us(GLushort red, GLushort green, GLushort blue)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3us)(red, green, blue);
}

void glColor3usv(const GLushort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color3usv)(v);
}

void glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4b)(red, green, blue, alpha);
}

void glColor4bv(const GLbyte *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4bv)(v);
}

void glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4d)(red, green, blue, alpha);
}

void glColor4dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4dv)(v);
}

void glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4f)(red, green, blue, alpha);
}

void glColor4fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4fv)(v);
}

void glColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4i)(red, green, blue, alpha);
}

void glColor4iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4iv)(v);
}

void glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4s)(red, green, blue, alpha);
}

void glColor4sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4sv)(v);
}

void glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4ub)(red, green, blue, alpha);
}

void glColor4ubv(const GLubyte *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4ubv)(v);
}

void glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4ui)(red, green, blue, alpha);
}

void glColor4uiv(const GLuint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4uiv)(v);
}

void glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4us)(red, green, blue, alpha);
}

void glColor4usv(const GLushort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Color4usv)(v);
}

void glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ColorMask)(red, green, blue, alpha);
}

void glColorMaterial(GLenum face, GLenum mode)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ColorMaterial)(face, mode);
}

void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ColorPointer)(size, type, stride, pointer);
}

void glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.CopyPixels)(x, y, width, height, type);
}

void glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.CopyTexImage1D)(target, level, internalformat, x, y, width, border);
}

void glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.CopyTexImage2D)(target, level, internalformat, x, y, width, height, border);
}

void glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.CopyTexSubImage1D)(target, level, xoffset, x, y, width);
}

void glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.CopyTexSubImage2D)(target, level, xoffset, yoffset, x, y, width, height);
}

void glCullFace(GLenum mode)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.CullFace)(mode);
}

void glDeleteLists(GLuint list, GLsizei range)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.DeleteLists)(list, range);
}

void glDeleteTextures(GLsizei n, const GLuint *textures)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.DeleteTextures)(n, textures);
}

void glDepthFunc(GLenum func)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.DepthFunc)(func);
}

void glDepthMask(GLboolean flag)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.DepthMask)(flag);
}

void glDepthRange(GLclampd zNear, GLclampd zFar)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.DepthRange)(zNear, zFar);
}

void glDisable(GLenum cap)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Disable)(cap);
}

void glDisableClientState(GLenum array)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.DisableClientState)(array);
}

void glDrawArrays(GLenum mode, GLint first, GLsizei count)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.DrawArrays)(mode, first, count);
}

void glDrawBuffer(GLenum mode)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.DrawBuffer)(mode);
}

void glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.DrawElements)(mode, count, type, indices);
}

void glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.DrawPixels)(width, height, format, type, image);
}

void glEdgeFlag(GLboolean flag)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EdgeFlag)(flag);
}

void glEdgeFlagPointer(GLsizei stride, const GLvoid *pointer)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EdgeFlagPointer)(stride, pointer);
}

void glEdgeFlagv(const GLboolean *flag)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EdgeFlagv)(flag);
}

void glEnable(GLenum cap)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Enable)(cap);
}

void glEnableClientState(GLenum array)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EnableClientState)(array);
}

void glEnd(void)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.End)();
}

void glEndList(void)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EndList)();
}

void glEvalCoord1d(GLdouble u)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EvalCoord1d)(u);
}

void glEvalCoord1dv(const GLdouble *u)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EvalCoord1dv)(u);
}

void glEvalCoord1f(GLfloat u)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EvalCoord1f)(u);
}

void glEvalCoord1fv(const GLfloat *u)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EvalCoord1fv)(u);
}

void glEvalCoord2d(GLdouble u, GLdouble v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EvalCoord2d)(u, v);
}

void glEvalCoord2dv(const GLdouble *u)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EvalCoord2dv)(u);
}

void glEvalCoord2f(GLfloat u, GLfloat v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EvalCoord2f)(u, v);
}

void glEvalCoord2fv(const GLfloat *u)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EvalCoord2fv)(u);
}

void glEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EvalMesh1)(mode, i1, i2);
}

void glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EvalMesh2)(mode, i1, i2, j1, j2);
}

void glEvalPoint1(GLint i)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EvalPoint1)(i);
}

void glEvalPoint2(GLint i, GLint j)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.EvalPoint2)(i, j);
}

void glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.FeedbackBuffer)(size, type, buffer);
}

void glFinish(void)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Finish)();
}

void glFlush(void)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Flush)();
}

void glFogf(GLenum pname, GLfloat param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Fogf)(pname, param);
}

void glFogfv(GLenum pname, const GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Fogfv)(pname, params);
}

void glFogi(GLenum pname, GLint param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Fogi)(pname, param);
}

void glFogiv(GLenum pname, const GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Fogiv)(pname, params);
}

void glFrontFace(GLenum mode)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.FrontFace)(mode);
}

void glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Frustum)(left, right, bottom, top, zNear, zFar);
}

GLuint glGenLists(GLsizei range)
{
    __DRI_GET_CONTEXT(gc);
    return (*gc->glAPI.GenLists)(range);
}

void glGenTextures(GLsizei n, GLuint *textures)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GenTextures)(n, textures);
}

void glGetBooleanv(GLenum val, GLboolean *b)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetBooleanv)(val, b);
}

void glGetClipPlane(GLenum plane, GLdouble *equation)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetClipPlane)(plane, equation);
}

void glGetDoublev(GLenum val, GLdouble *d)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetDoublev)(val, d);
}

GLenum glGetError(void)
{
    __DRI_GET_CONTEXT(gc);
    return (*gc->glAPI.GetError)();
}

void glGetFloatv(GLenum val, GLfloat *f)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetFloatv)(val, f);
}

void glGetIntegerv(GLenum val, GLint *i)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetIntegerv)(val, i);
}

void glGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetLightfv)(light, pname, params);
}

void glGetLightiv(GLenum light, GLenum pname, GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetLightiv)(light, pname, params);
}

void glGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetMapdv)(target, query, v);
}

void glGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetMapfv)(target, query, v);
}

void glGetMapiv(GLenum target, GLenum query, GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetMapiv)(target, query, v);
}

void glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetMaterialfv)(face, pname, params);
}

void glGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetMaterialiv)(face, pname, params);
}

void glGetPixelMapfv(GLenum map, GLfloat *values)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetPixelMapfv)(map, values);
}

void glGetPixelMapuiv(GLenum map, GLuint *values)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetPixelMapuiv)(map, values);
}

void glGetPixelMapusv(GLenum map, GLushort *values)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetPixelMapusv)(map, values);
}

void glGetPointerv(GLenum pname, void **params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetPointerv)(pname, params);
}

void glGetPolygonStipple(GLubyte *mask)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetPolygonStipple)(mask);
}

const GLubyte *glGetString(GLenum name)
{
    __DRI_GET_CONTEXT(gc);
    return (*gc->glAPI.GetString)(name);
}

void glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetTexEnvfv)(target, pname, params);
}

void glGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetTexEnviv)(target, pname, params);
}

void glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetTexGendv)(coord, pname, params);
}

void glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetTexGenfv)(coord, pname, params);
}

void glGetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetTexGeniv)(coord, pname, params);
}

void glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *texels)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetTexImage)(target, level, format, type, texels);
}

void glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetTexLevelParameterfv)(target, level, pname, params);
}

void glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetTexLevelParameteriv)(target, level, pname, params);
}

void glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetTexParameterfv)(target, pname, params);
}

void glGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.GetTexParameteriv)(target, pname, params);
}

void glHint(GLenum target, GLenum mode)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Hint)(target, mode);
}

void glIndexMask(GLuint mask)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.IndexMask)(mask);
}

void glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.IndexPointer)(type, stride, pointer);
}

void glIndexd(GLdouble c)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Indexd)(c);
}

void glIndexdv(const GLdouble *c)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Indexdv)(c);
}

void glIndexf(GLfloat c)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Indexf)(c);
}

void glIndexfv(const GLfloat *c)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Indexfv)(c);
}

void glIndexi(GLint c)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Indexi)(c);
}

void glIndexiv(const GLint *c)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Indexiv)(c);
}

void glIndexs(GLshort c)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Indexs)(c);
}

void glIndexsv(const GLshort *c)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Indexsv)(c);
}

void glIndexub(GLubyte c)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Indexub)(c);
}

void glIndexubv(const GLubyte *c)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Indexubv)(c);
}

void glInitNames(void)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.InitNames)();
}

void glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.InterleavedArrays)(format, stride, pointer);
}

GLboolean glIsEnabled(GLenum cap)
{
    __DRI_GET_CONTEXT(gc);
    return (*gc->glAPI.IsEnabled)(cap);
}

GLboolean glIsList(GLuint list)
{
    __DRI_GET_CONTEXT(gc);
    return (*gc->glAPI.IsList)(list);
}

GLboolean glIsTexture(GLuint texture)
{
    __DRI_GET_CONTEXT(gc);
    return (*gc->glAPI.IsTexture)(texture);
}

void glLightModelf(GLenum pname, GLfloat param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.LightModelf)(pname, param);
}

void glLightModelfv(GLenum pname, const GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.LightModelfv)(pname, params);
}

void glLightModeli(GLenum pname, GLint param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.LightModeli)(pname, param);
}

void glLightModeliv(GLenum pname, const GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.LightModeliv)(pname, params);
}

void glLightf(GLenum light, GLenum pname, GLfloat param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Lightf)(light, pname, param);
}

void glLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Lightfv)(light, pname, params);
}

void glLighti(GLenum light, GLenum pname, GLint param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Lighti)(light, pname, param);
}

void glLightiv(GLenum light, GLenum pname, const GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Lightiv)(light, pname, params);
}

void glLineStipple(GLint factor, GLushort pattern)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.LineStipple)(factor, pattern);
}

void glLineWidth(GLfloat width)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.LineWidth)(width);
}

void glListBase(GLuint base)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ListBase)(base);
}

void glLoadIdentity(void)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.LoadIdentity)();
}

void glLoadMatrixd(const GLdouble *m)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.LoadMatrixd)(m);
}

void glLoadMatrixf(const GLfloat *m)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.LoadMatrixf)(m);
}

void glLoadName(GLuint name)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.LoadName)(name);
}

void glLogicOp(GLenum opcode)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.LogicOp)(opcode);
}

void glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *pnts)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Map1d)(target, u1, u2, stride, order, pnts);
}

void glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *pnts)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Map1f)(target, u1, u2, stride, order, pnts);
}

void glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustr, GLint uord, GLdouble v1, GLdouble v2, GLint vstr, GLint vord, const GLdouble *pnts)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Map2d)(target, u1, u2, ustr, uord, v1, v2, vstr, vord, pnts);
}

void glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustr, GLint uord, GLfloat v1, GLfloat v2, GLint vstr, GLint vord, const GLfloat *pnts)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Map2f)(target, u1, u2, ustr, uord, v1, v2, vstr, vord, pnts);
}

void glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.MapGrid1d)(un, u1, u2);
}

void glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.MapGrid1f)(un, u1, u2);
}

void glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.MapGrid2d)(un, u1, u2, vn, v1, v2);
}

void glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.MapGrid2f)(un, u1, u2, vn, v1, v2);
}

void glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Materialf)(face, pname, param);
}

void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Materialfv)(face, pname, params);
}

void glMateriali(GLenum face, GLenum pname, GLint param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Materiali)(face, pname, param);
}

void glMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Materialiv)(face, pname, params);
}

void glMatrixMode(GLenum mode)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.MatrixMode)(mode);
}

void glMultMatrixd(const GLdouble *m)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.MultMatrixd)(m);
}

void glMultMatrixf(const GLfloat *m)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.MultMatrixf)(m);
}

void glNewList(GLuint list, GLenum mode)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.NewList)(list, mode);
}

void glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Normal3b)(nx, ny, nz);
}

void glNormal3bv(const GLbyte *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Normal3bv)(v);
}

void glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Normal3d)(nx, ny, nz);
}

void glNormal3dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Normal3dv)(v);
}

void glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Normal3f)(nx, ny, nz);
}

void glNormal3fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Normal3fv)(v);
}

void glNormal3i(GLint nx, GLint ny, GLint nz)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Normal3i)(nx, ny, nz);
}

void glNormal3iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Normal3iv)(v);
}

void glNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Normal3s)(nx, ny, nz);
}

void glNormal3sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Normal3sv)(v);
}

void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.NormalPointer)(type, stride, pointer);
}

void glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Ortho)(left, right, bottom, top, zNear, zFar);
}

void glPassThrough(GLfloat token)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PassThrough)(token);
}

void glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PixelMapfv)(map, mapsize, values);
}

void glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PixelMapuiv)(map, mapsize, values);
}

void glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PixelMapusv)(map, mapsize, values);
}

void glPixelStoref(GLenum pname, GLfloat param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PixelStoref)(pname, param);
}

void glPixelStorei(GLenum pname, GLint param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PixelStorei)(pname, param);
}

void glPixelTransferf(GLenum pname, GLfloat param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PixelTransferf)(pname, param);
}

void glPixelTransferi(GLenum pname, GLint param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PixelTransferi)(pname, param);
}

void glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PixelZoom)(xfactor, yfactor);
}

void glPointSize(GLfloat size)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PointSize)(size);
}

void glPolygonMode(GLenum face, GLenum mode)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PolygonMode)(face, mode);
}

void glPolygonOffset(GLfloat factor, GLfloat units)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PolygonOffset)(factor, units);
}

void glPolygonStipple(const GLubyte *mask)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PolygonStipple)(mask);
}

void glPopAttrib(void)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PopAttrib)();
}

void glPopClientAttrib(void)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PopClientAttrib)();
}

void glPopMatrix(void)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PopMatrix)();
}

void glPopName(void)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PopName)();
}

void glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PrioritizeTextures)(n, textures, priorities);
}

void glPushAttrib(GLbitfield mask)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PushAttrib)(mask);
}

void glPushClientAttrib(GLuint mask)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PushClientAttrib)(mask);
}

void glPushMatrix(void)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PushMatrix)();
}

void glPushName(GLuint name)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.PushName)(name);
}

void glRasterPos2d(GLdouble x, GLdouble y)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos2d)(x, y);
}

void glRasterPos2dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos2dv)(v);
}

void glRasterPos2f(GLfloat x, GLfloat y)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos2f)(x, y);
}

void glRasterPos2fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos2fv)(v);
}

void glRasterPos2i(GLint x, GLint y)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos2i)(x, y);
}

void glRasterPos2iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos2iv)(v);
}

void glRasterPos2s(GLshort x, GLshort y)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos2s)(x, y);
}

void glRasterPos2sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos2sv)(v);
}

void glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos3d)(x, y, z);
}

void glRasterPos3dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos3dv)(v);
}

void glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos3f)(x, y, z);
}

void glRasterPos3fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos3fv)(v);
}

void glRasterPos3i(GLint x, GLint y, GLint z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos3i)(x, y, z);
}

void glRasterPos3iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos3iv)(v);
}

void glRasterPos3s(GLshort x, GLshort y, GLshort z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos3s)(x, y, z);
}

void glRasterPos3sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos3sv)(v);
}

void glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos4d)(x, y, z, w);
}

void glRasterPos4dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos4dv)(v);
}

void glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos4f)(x, y, z, w);
}

void glRasterPos4fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos4fv)(v);
}

void glRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos4i)(x, y, z, w);
}

void glRasterPos4iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos4iv)(v);
}

void glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos4s)(x, y, z, w);
}

void glRasterPos4sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.RasterPos4sv)(v);
}

void glReadBuffer(GLenum mode)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ReadBuffer)(mode);
}

void glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ReadPixels)(x, y, width, height, format, type, pixels);
}

void glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Rectd)(x1, y1, x2, y2);
}

void glRectdv(const GLdouble *v1, const GLdouble *v2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Rectdv)(v1, v2);
}

void glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Rectf)(x1, y1, x2, y2);
}

void glRectfv(const GLfloat *v1, const GLfloat *v2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Rectfv)(v1, v2);
}

void glRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Recti)(x1, y1, x2, y2);
}

void glRectiv(const GLint *v1, const GLint *v2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Rectiv)(v1, v2);
}

void glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Rects)(x1, y1, x2, y2);
}

void glRectsv(const GLshort *v1, const GLshort *v2)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Rectsv)(v1, v2);
}

GLint glRenderMode(GLenum mode)
{
    __DRI_GET_CONTEXT(gc);
    return (*gc->glAPI.RenderMode)(mode);
}

void glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Rotated)(angle, x, y, z);
}

void glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Rotatef)(angle, x, y, z);
}

void glScaled(GLdouble x, GLdouble y, GLdouble z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Scaled)(x, y, z);
}

void glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Scalef)(x, y, z);
}

void glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Scissor)(x, y, width, height);
}

void glSelectBuffer(GLsizei numnames, GLuint *buffer)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.SelectBuffer)(numnames, buffer);
}

void glShadeModel(GLenum mode)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.ShadeModel)(mode);
}

void glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.StencilFunc)(func, ref, mask);
}

void glStencilMask(GLuint mask)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.StencilMask)(mask);
}

void glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.StencilOp)(fail, zfail, zpass);
}

void glTexCoord1d(GLdouble s)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord1d)(s);
}

void glTexCoord1dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord1dv)(v);
}

void glTexCoord1f(GLfloat s)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord1f)(s);
}

void glTexCoord1fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord1fv)(v);
}

void glTexCoord1i(GLint s)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord1i)(s);
}

void glTexCoord1iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord1iv)(v);
}

void glTexCoord1s(GLshort s)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord1s)(s);
}

void glTexCoord1sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord1sv)(v);
}

void glTexCoord2d(GLdouble s, GLdouble t)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord2d)(s, t);
}

void glTexCoord2dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord2dv)(v);
}

void glTexCoord2f(GLfloat s, GLfloat t)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord2f)(s, t);
}

void glTexCoord2fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord2fv)(v);
}

void glTexCoord2i(GLint s, GLint t)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord2i)(s, t);
}

void glTexCoord2iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord2iv)(v);
}

void glTexCoord2s(GLshort s, GLshort t)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord2s)(s, t);
}

void glTexCoord2sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord2sv)(v);
}

void glTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord3d)(s, t, r);
}

void glTexCoord3dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord3dv)(v);
}

void glTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord3f)(s, t, r);
}

void glTexCoord3fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord3fv)(v);
}

void glTexCoord3i(GLint s, GLint t, GLint r)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord3i)(s, t, r);
}

void glTexCoord3iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord3iv)(v);
}

void glTexCoord3s(GLshort s, GLshort t, GLshort r)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord3s)(s, t, r);
}

void glTexCoord3sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord3sv)(v);
}

void glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord4d)(s, t, r, q);
}

void glTexCoord4dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord4dv)(v);
}

void glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord4f)(s, t, r, q);
}

void glTexCoord4fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord4fv)(v);
}

void glTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord4i)(s, t, r, q);
}

void glTexCoord4iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord4iv)(v);
}

void glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord4s)(s, t, r, q);
}

void glTexCoord4sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoord4sv)(v);
}

void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexCoordPointer)(size, type, stride, pointer);
}

void glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexEnvf)(target, pname, param);
}

void glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexEnvfv)(target, pname, params);
}

void glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexEnvi)(target, pname, param);
}

void glTexEnviv(GLenum target, GLenum pname, const GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexEnviv)(target, pname, params);
}

void glTexGend(GLenum coord, GLenum pname, GLdouble param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexGend)(coord, pname, param);
}

void glTexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexGendv)(coord, pname, params);
}

void glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexGenf)(coord, pname, param);
}

void glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexGenfv)(coord, pname, params);
}

void glTexGeni(GLenum coord, GLenum pname, GLint param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexGeni)(coord, pname, param);
}

void glTexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexGeniv)(coord, pname, params);
}

void glTexImage1D(GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *image)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexImage1D)(target, level, components, width, border, format, type, image);
}

void glTexImage2D(GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *image)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexImage2D)(target, level, components, width, height, border, format, type, image);
}

void glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexParameterf)(target, pname, param);
}

void glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexParameterfv)(target, pname, params);
}

void glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexParameteri)(target, pname, param);
}

void glTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexParameteriv)(target, pname, params);
}

void glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexSubImage1D)(target, level, xoffset, width, format, type, image);
}

void glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.TexSubImage2D)(target, level, xoffset, yoffset, width, height, format, type, image);
}

void glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Translated)(x, y, z);
}

void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Translatef)(x, y, z);
}

void glVertex2d(GLdouble x, GLdouble y)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex2d)(x, y);
}

void glVertex2dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex2dv)(v);
}

void glVertex2f(GLfloat x, GLfloat y)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex2f)(x, y);
}

void glVertex2fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex2fv)(v);
}

void glVertex2i(GLint x, GLint y)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex2i)(x, y);
}

void glVertex2iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex2iv)(v);
}

void glVertex2s(GLshort x, GLshort y)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex2s)(x, y);
}

void glVertex2sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex2sv)(v);
}

void glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex3d)(x, y, z);
}

void glVertex3dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex3dv)(v);
}

void glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex3f)(x, y, z);
}

void glVertex3fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex3fv)(v);
}

void glVertex3i(GLint x, GLint y, GLint z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex3i)(x, y, z);
}

void glVertex3iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex3iv)(v);
}

void glVertex3s(GLshort x, GLshort y, GLshort z)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex3s)(x, y, z);
}

void glVertex3sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex3sv)(v);
}

void glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex4d)(x, y, z, w);
}

void glVertex4dv(const GLdouble *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex4dv)(v);
}

void glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex4f)(x, y, z, w);
}

void glVertex4fv(const GLfloat *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex4fv)(v);
}

void glVertex4i(GLint x, GLint y, GLint z, GLint w)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex4i)(x, y, z, w);
}

void glVertex4iv(const GLint *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex4iv)(v);
}

void glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex4s)(x, y, z, w);
}

void glVertex4sv(const GLshort *v)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Vertex4sv)(v);
}

void glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.VertexPointer)(size, type, stride, pointer);
}

void glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    __DRI_GET_CONTEXT(gc);
    (*gc->glAPI.Viewport)(x, y, width, height);
}

#endif
