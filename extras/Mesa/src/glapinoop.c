/* $Id: glapinoop.c,v 1.3 2000/02/10 18:57:19 dawes Exp $ */

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


#ifdef XFree86LOADER
#include "xf86_ansic.h"
#else
#include <stdio.h>
#endif
#include "glapi.h"
#include "glapinoop.h"
#include "glapitable.h"


static GLboolean WarnFlag = GL_FALSE;


void
_glapi_noop_enable_warnings(GLboolean enable)
{
   WarnFlag = enable;
}


static void
warning(const char *funcName)
{
   if (WarnFlag) {
      fprintf(stderr, "GL User Error: calling %s without a current context\n",
              funcName);
   }
}



static void NoOpAccum(GLenum op, GLfloat value)
{
   (void) op;
   (void) value;
   warning("glAccum");
}

static void NoOpAlphaFunc(GLenum func, GLclampf ref)
{
   (void) func;
   (void) ref;
   warning("glAlphaFunc");
}

static void NoOpBegin(GLenum mode)
{
   (void) mode;
   warning("glBegin");
}

static void NoOpBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap)
{
   (void) width;
   (void) height;
   (void) xorig;
   (void) yorig;
   (void) xmove;
   (void) ymove;
   (void) bitmap;
   warning("glBitmap");
}

static void NoOpBlendFunc(GLenum sfactor, GLenum dfactor)
{
   (void) sfactor;
   (void) dfactor;
   warning("glBlendFunc");
}

static void NoOpCallList(GLuint list)
{
   (void) list;
   warning("glCallList");
}

static void NoOpCallLists(GLsizei n, GLenum type, const GLvoid *lists)
{
   (void) n;
   (void) type;
   (void) lists;
   warning("glCallLists");
}

static void NoOpClear(GLbitfield mask)
{
   (void) mask;
   warning("glClear");
}

static void NoOpClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   (void) red;
   (void) green;
   (void) blue;
   (void) alpha;
   warning("glClearAccum");
}

static void NoOpClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
   (void) red;
   (void) green;
   (void) blue;
   (void) alpha;
   warning("glClearColor");
}

static void NoOpClearDepth(GLclampd depth)
{
   (void) depth;
   warning("gl");
}

static void NoOpClearIndex(GLfloat c)
{
   (void) c;
   warning("glClearIndex");
}

static void NoOpClearStencil(GLint s)
{
   (void) s;
   warning("glStencil");
}

static void NoOpClipPlane(GLenum plane, const GLdouble *equation)
{
   (void) plane;
   (void) equation;
   warning("glClipPlane");
}

static void NoOpColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
   (void) red;
   (void) green;
   (void) blue;
   warning("glColor3b");
}

static void NoOpColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
   (void) red;
   (void) green;
   (void) blue;
   warning("glColor3d");
}

static void NoOpColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
   (void) red;
   (void) green;
   (void) blue;
   warning("glColor3f");
}

static void NoOpColor3i(GLint red, GLint green, GLint blue)
{
   (void) red;
   (void) green;
   (void) blue;
   warning("glColor3i");
}

static void NoOpColor3s(GLshort red, GLshort green, GLshort blue)
{
   (void) red;
   (void) green;
   (void) blue;
   warning("glColor3s");
}

static void NoOpColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
   (void) red;
   (void) green;
   (void) blue;
   warning("glColor3ub");
}

static void NoOpColor3ui(GLuint red, GLuint green, GLuint blue)
{
   (void) red;
   (void) green;
   (void) blue;
   warning("glColor3ui");
}

static void NoOpColor3us(GLushort red, GLushort green, GLushort blue)
{
   (void) red;
   (void) green;
   (void) blue;
   warning("glColor3us");
}

static void NoOpColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
   (void) red;
   (void) green;
   (void) blue;
   (void) alpha;
   warning("glColor4b");
}

static void NoOpColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
   (void) red;
   (void) green;
   (void) blue;
   (void) alpha;
   warning("glColor4d");
}

static void NoOpColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
   (void) red;
   (void) green;
   (void) blue;
   (void) alpha;
   warning("glColor4f");
}

static void NoOpColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
   (void) red;
   (void) green;
   (void) blue;
   (void) alpha;
   warning("glColor4i");
}

static void NoOpColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
   (void) red;
   (void) green;
   (void) blue;
   (void) alpha;
   warning("glColor4s");
}

static void NoOpColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
   (void) red;
   (void) green;
   (void) blue;
   (void) alpha;
   warning("glColor4ub");
}

static void NoOpColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
   (void) red;
   (void) green;
   (void) blue;
   (void) alpha;
   warning("glColor4ui");
}

static void NoOpColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
   (void) red;
   (void) green;
   (void) blue;
   (void) alpha;
   warning("glColor4us");
}

static void NoOpColor3bv(const GLbyte *v)
{
   (void) v;
   warning("glColor3bv");
}

static void NoOpColor3dv(const GLdouble *v)
{
   (void) v;
   warning("glColor3dv");
}

static void NoOpColor3fv(const GLfloat *v)
{
   (void) v;
   warning("glColor3fv");
}

static void NoOpColor3iv(const GLint *v)
{
   (void) v;
   warning("glColor3iv");
}

static void NoOpColor3sv(const GLshort *v)
{
   (void) v;
   warning("glColor3sv");
}

static void NoOpColor3ubv(const GLubyte *v)
{
   (void) v;
   warning("glColor3ubv");
}

static void NoOpColor3uiv(const GLuint *v)
{
   (void) v;
   warning("glColor3uiv");
}

static void NoOpColor3usv(const GLushort *v)
{
   (void) v;
   warning("glColor3usv");
}

static void NoOpColor4bv(const GLbyte *v)
{
   (void) v;
   warning("glColor4bv");
}

static void NoOpColor4dv(const GLdouble *v)
{
   (void) v;
   warning("glColor4dv");
}

static void NoOpColor4fv(const GLfloat *v)
{
   (void) v;
   warning("glColor4fv");
}

static void NoOpColor4iv(const GLint *v)
{
   (void) v;
   warning("glColor4iv");
}

static void NoOpColor4sv(const GLshort *v)
{
   (void) v;
   warning("glColor4sv");
}

static void NoOpColor4ubv(const GLubyte *v)
{
   (void) v;
   warning("glColor4ubv");
}

static void NoOpColor4uiv(const GLuint *v)
{
   (void) v;
   warning("glColor4uiv");
}

static void NoOpColor4usv(const GLushort *v)
{
   (void) v;
   warning("glColor4usv");
}

static void NoOpColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
   (void) red;
   (void) green;
   (void) blue;
   (void) alpha;
   warning("glColorMask");
}

static void NoOpColorMaterial(GLenum face, GLenum mode)
{
   (void) face;
   (void) mode;
   warning("glColorMaterial");
}

static void NoOpCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   (void) type;
   warning("glCopyPixels");
}

static void NoOpCullFace(GLenum mode)
{
   (void) mode;
   warning("glCullFace");
}

static void NoOpDepthFunc(GLenum func)
{
   (void) func;
   warning("glDepthFunc");
}

static void NoOpDepthMask(GLboolean flag)
{
   (void) flag;
   warning("glDepthMask");
}

static void NoOpDepthRange(GLclampd nearVal, GLclampd farVal)
{
   (void) nearVal;
   (void) farVal;
   warning("glDepthRange");
}

static void NoOpDeleteLists(GLuint list, GLsizei range)
{
   (void) list;
   (void) range;
   warning("glDeleteLists");
}

static void NoOpDisable(GLenum cap)
{
   (void) cap;
   warning("glDisable");
}

static void NoOpDisableClientState(GLenum cap)
{
   (void) cap;
   warning("glDisableClientState");
}

static void NoOpDrawBuffer(GLenum mode)
{
   (void) mode;
   warning("glDrawBuffer");
}

static void NoOpDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
   (void) width;
   (void) height;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glDrawPixels");
}

static void NoOpEnable(GLenum mode)
{
   (void) mode;
   warning("glEnable");
}

static void NoOpEnd(void)
{
   warning("glEnd");
}

static void NoOpEndList(void)
{
   warning("glEndList");
}

static void NoOpEvalCoord1d(GLdouble u)
{
   (void) u;
   warning("glEvalCoord1d");
}

static void NoOpEvalCoord1f(GLfloat u)
{
   (void) u;
   warning("glEvalCoord1f");
}

static void NoOpEvalCoord1dv(const GLdouble *u)
{
   (void) u;
   warning("glEvalCoord1dv");
}

static void NoOpEvalCoord1fv(const GLfloat *u)
{
   (void) u;
   warning("glEvalCoord1fv");
}

static void NoOpEvalCoord2d(GLdouble u, GLdouble v)
{
   (void) u;
   (void) v;
   warning("glEvalCoord2d");
}

static void NoOpEvalCoord2f(GLfloat u, GLfloat v)
{
   (void) u;
   (void) v;
   warning("glEvalCoord2f");
}

static void NoOpEvalCoord2dv(const GLdouble *u)
{
   (void) u;
   warning("glEvalCoord2dv");
}

static void NoOpEvalCoord2fv(const GLfloat *u)
{
   (void) u;
   warning("glEvalCoord2fv");
}

static void NoOpEvalPoint1(GLint i)
{
   (void) i;
   warning("glEvalPoint1");
}

static void NoOpEvalPoint2(GLint i, GLint j)
{
   (void) i;
   (void) j;
   warning("glEvalPoint2");
}

static void NoOpEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
   (void) mode;
   (void) i1;
   (void) i2;
   warning("glEvalMesh1");
}

static void NoOpEdgeFlag(GLboolean flag)
{
   (void) flag;
   warning("glEdgeFlag");
}

static void NoOpEdgeFlagv(const GLboolean *flag)
{
   (void) flag;
   warning("glEdgeFlagv");
}

static void NoOpEdgeFlagPointer(GLsizei stride, const GLvoid *ptr)
{
   (void) stride;
   (void) ptr;
   warning("glEdgeFlagPointer");
}

static void NoOpEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
   (void) mode;
   (void) i1;
   (void) i2;
   (void) j1;
   (void) j2;
   warning("glEvalMesh2");
}

static void NoOpFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer)
{
   (void) size;
   (void) type;
   (void) buffer;
   warning("glFeedbackBuffer");
}

static void NoOpFinish(void)
{
   warning("glFinish");
}

static void NoOpFlush(void)
{
   warning("glFlush");
}

static void NoOpFogf(GLenum pname, GLfloat param)
{
   (void) pname;
   (void) param;
   warning("glFogf");
}

static void NoOpFogi(GLenum pname, GLint param)
{
   (void) pname;
   (void) param;
   warning("glFogi");
}

static void NoOpFogfv(GLenum pname, const GLfloat *params)
{
   (void) pname;
   (void) params;
   warning("glFogfv");
}

static void NoOpFogiv(GLenum pname, const GLint *params)
{
   (void) pname;
   (void) params;
   warning("glFogiv");
}

static void NoOpFrontFace(GLenum mode)
{
   (void) mode;
   warning("glFrontFace");
}

static void NoOpFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearval, GLdouble farval)
{
   (void) left;
   (void) right;
   (void) bottom;
   (void) top;
   (void) nearval;
   (void) farval;
   warning("glFrustum");
}

static GLuint NoOpGenLists(GLsizei range)
{
   (void) range;
   warning("glGenLists");
   return 0;
}

static void NoOpGetBooleanv(GLenum pname, GLboolean *params)
{
   (void) pname;
   (void) params;
   warning("glGetBooleanv");
}

static void NoOpGetClipPlane(GLenum plane, GLdouble *equation)
{
   (void) plane;
   (void) equation;
   warning("glGetClipPlane");
}

static void NoOpGetDoublev(GLenum pname, GLdouble *params)
{
   (void) pname;
   (void) params;
   warning("glGetDoublev");
}

static GLenum NoOpGetError(void)
{
   warning("glGetError");
   return (GLenum) 0;
}

static void NoOpGetFloatv(GLenum pname, GLfloat *params)
{
   (void) pname;
   (void) params;
   warning("glGetFloatv");
}

static void NoOpGetIntegerv(GLenum pname, GLint *params)
{
   (void) pname;
   (void) params;
   warning("glGetIntegerv");
}

static void NoOpGetLightfv(GLenum light, GLenum pname, GLfloat *params)
{
   (void) light;
   (void) pname;
   (void) params;
   warning("glGetLightfv");
}

static void NoOpGetLightiv(GLenum light, GLenum pname, GLint *params)
{
   (void) light;
   (void) pname;
   (void) params;
   warning("glGetLightiv");
}

static void NoOpGetMapdv(GLenum target, GLenum query, GLdouble *v)
{
   (void) target;
   (void) query;
   (void) v;
   warning("glGetMapdv");
}

static void NoOpGetMapfv(GLenum target, GLenum query, GLfloat *v)
{
   (void) target;
   (void) query;
   (void) v;
   warning("glGetMapfv");
}

static void NoOpGetMapiv(GLenum target, GLenum query, GLint *v)
{
   (void) target;
   (void) query;
   (void) v;
   warning("glGetMapiv");
}

static void NoOpGetMaterialfv(GLenum face, GLenum pname, GLfloat *params)
{
   (void) face;
   (void) pname;
   (void) params;
   warning("glGetMaterialfv");
}

static void NoOpGetMaterialiv(GLenum face, GLenum pname, GLint *params)
{
   (void) face;
   (void) pname;
   (void) params;
   warning("glGetMaterialiv");
}

static void NoOpGetPixelMapfv(GLenum map, GLfloat *values)
{
   (void) map;
   (void) values;
   warning("glGetPixelMapfv");
}

static void NoOpGetPixelMapuiv(GLenum map, GLuint *values)
{
   (void) map;
   (void) values;
   warning("glGetPixelMapuiv");
}

static void NoOpGetPixelMapusv(GLenum map, GLushort *values)
{
   (void) map;
   (void) values;
   warning("glGetPixelMapusv");
}

static void NoOpGetPolygonStipple(GLubyte *mask)
{
   (void) mask;
   warning("glGetPolygonStipple");
}

static const GLubyte * NoOpGetString(GLenum name)
{
   (void) name;
   warning("glGetString");
   return NULL;
}

static void NoOpGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetTexEnvfv");
}

static void NoOpGetTexEnviv(GLenum target, GLenum pname, GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetTexEnviv");
}

static void NoOpGetTexGeniv(GLenum coord, GLenum pname, GLint *params)
{
   (void) coord;
   (void) pname;
   (void) params;
   warning("glGetTexGeniv");
}

static void NoOpGetTexGendv(GLenum coord, GLenum pname, GLdouble *params)
{
   (void) coord;
   (void) pname;
   (void) params;
   warning("glGetTexGendv");
}

static void NoOpGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params)
{
   (void) coord;
   (void) pname;
   (void) params;
   warning("glGetTexGenfv");
}

static void NoOpGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *pixels)
{
   (void) target;
   (void) level;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glGetTexImage");
}

static void NoOpGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params)
{
   (void) target;
   (void) level;
   (void) pname;
   (void) params;
   warning("glGetTexLevelParameterfv");
}

static void NoOpGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params)
{
   (void) target;
   (void) level;
   (void) pname;
   (void) params;
   warning("glGetTexLevelParameteriv");
}

static void NoOpGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetTexParameterfv");
}

static void NoOpGetTexParameteriv(GLenum target, GLenum pname, GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetTexParameteriv");
}

static void NoOpHint(GLenum target, GLenum mode)
{
   (void) target;
   (void) mode;
   warning("glHint");
}

static void NoOpIndexd(GLdouble c)
{
   (void) c;
   warning("glIndexd");
}

static void NoOpIndexdv(const GLdouble *c)
{
   (void) c;
   warning("glIndexdv");
}

static void NoOpIndexf(GLfloat c)
{
   (void) c;
   warning("glIndexf");
}

static void NoOpIndexfv(const GLfloat *c)
{
   (void) c;
   warning("glIndexfv");
}

static void NoOpIndexi(GLint c)
{
   (void) c;
   warning("glIndexi");
}

static void NoOpIndexiv(const GLint *c)
{
   (void) c;
   warning("glIndexiv");
}

static void NoOpIndexs(GLshort c)
{
   (void) c;
   warning("glIndexs");
}

static void NoOpIndexsv(const GLshort *c)
{
   (void) c;
   warning("glIndexsv");
}

static void NoOpIndexMask(GLuint mask)
{
   (void) mask;
   warning("glIndexMask");
}

static void NoOpInitNames(void)
{
   warning("glInitNames");
}

static GLboolean NoOpIsList(GLuint list)
{
   (void) list;
   warning("glIsList");
   return GL_FALSE;
}

static void NoOpLightf(GLenum light, GLenum pname, GLfloat param)
{
   (void) light;
   (void) pname;
   (void) param;
   warning("glLightf");
}

static void NoOpLighti(GLenum light, GLenum pname, GLint param)
{
   (void) light;
   (void) pname;
   (void) param;
   warning("glLighti");
}

static void NoOpLightfv(GLenum light, GLenum pname, const GLfloat *params)
{
   (void) light;
   (void) pname;
   (void) params;
   warning("glLightfv");
}

static void NoOpLightiv(GLenum light, GLenum pname, const GLint *params)
{
   (void) light;
   (void) pname;
   (void) params;
   warning("glLightiv");
}

static void NoOpLightModelf(GLenum pname, GLfloat param)
{
   (void) pname;
   (void) param;
   warning("glLightModelf");
}

static void NoOpLightModeli(GLenum pname, GLint param)
{
   (void) pname;
   (void) param;
   warning("glLightModeli");
}

static void NoOpLightModelfv(GLenum pname, const GLfloat *params)
{
   (void) pname;
   (void) params;
   warning("glLightModelfv");
}

static void NoOpLightModeliv(GLenum pname, const GLint *params)
{
   (void) pname;
   (void) params;
   warning("glLightModeliv");
}

static void NoOpLineWidth(GLfloat width)
{
   (void) width;
   warning("glLineWidth");
}

static void NoOpLineStipple(GLint factor, GLushort pattern)
{
   (void) factor;
   (void) pattern;
   warning("glLineStipple");
}

static void NoOpListBase(GLuint base)
{
   (void) base;
   warning("glListBase");
}

static void NoOpLoadIdentity(void)
{
   warning("glLoadIdentity");
}

static void NoOpLoadMatrixd(const GLdouble *m)
{
   (void) m;
   warning("glLoadMatrixd");
}

static void NoOpLoadMatrixf(const GLfloat *m)
{
   (void) m;
   warning("glLoadMatrixf");
}

static void NoOpLoadName(GLuint name)
{
   (void) name;
   warning("glLoadName");
}

static void NoOpLogicOp(GLenum opcode)
{
   (void) opcode;
   warning("glLogicOp");
}

static void NoOpMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *points)
{
   (void) target;
   (void) u1;
   (void) u2;
   (void) stride;
   (void) order;
   (void) points;
   warning("glMap1d");
}

static void NoOpMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *points)
{
   (void) target;
   (void) u1;
   (void) u2;
   (void) stride;
   (void) order;
   (void) points;
   warning("glMap1f");
}

static void NoOpMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, const GLdouble *points)
{
   (void) target;
   (void) u1;
   (void) u2;
   (void) ustride;
   (void) uorder;
   (void) v1;
   (void) v2;
   (void) vstride;
   (void) vorder;
   (void) points;
   warning("glMap2d");
}

static void NoOpMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, const GLfloat *points)
{
   (void) target;
   (void) u1;
   (void) u2;
   (void) ustride;
   (void) uorder;
   (void) v1;
   (void) v2;
   (void) vstride;
   (void) vorder;
   (void) points;
   warning("glMap2f");
}

static void NoOpMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
   (void) un;
   (void) u1;
   (void) u2;
   warning("glMapGrid1d");
}

static void NoOpMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
   (void) un;
   (void) u1;
   (void) u2;
   warning("glMapGrid1f");
}

static void NoOpMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
   (void) un;
   (void) u1;
   (void) u2;
   (void) vn;
   (void) v1;
   (void) v2;
   warning("glMapGrid2d");
}

static void NoOpMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
   (void) un;
   (void) u1;
   (void) u2;
   (void) vn;
   (void) v1;
   (void) v2;
   warning("glMapGrid2f");
}

static void NoOpMaterialf(GLenum face, GLenum pname, GLfloat param)
{
   (void) face;
   (void) pname;
   (void) param;
   warning("glMaterialf");
}

static void NoOpMateriali(GLenum face, GLenum pname, GLint param)
{
   (void) face;
   (void) pname;
   (void) param;
   warning("glMateriali");
}

static void NoOpMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
{
   (void) face;
   (void) pname;
   (void) params;
   warning("glMaterialfv");
}

static void NoOpMaterialiv(GLenum face, GLenum pname, const GLint *params)
{
   (void) face;
   (void) pname;
   (void) params;
   warning("glMaterialiv");
}

static void NoOpMatrixMode(GLenum mode)
{
   (void) mode;
   warning("glMatrixMode");
}

static void NoOpMultMatrixd(const GLdouble *m)
{
   (void) m;
   warning("glMultMatrixd");
}

static void NoOpMultMatrixf(const GLfloat *m)
{
   (void) m;
   warning("glMultMatrixf");
}

static void NoOpNewList(GLuint list, GLenum mode)
{
   (void) list;
   (void) mode;
   warning("glNewList");
}

static void NoOpNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
   (void) nx;
   (void) ny;
   (void) nz;
   warning("glNormal3b");
}

static void NoOpNormal3bv(const GLbyte *v)
{
   (void) v;
   warning("glNormal3bv");
}

static void NoOpNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
   (void) nx;
   (void) ny;
   (void) nz;
   warning("glNormal3d");
}

static void NoOpNormal3dv(const GLdouble *v)
{
   (void) v;
   warning("glNormal3dv");
}

static void NoOpNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
   (void) nx;
   (void) ny;
   (void) nz;
   warning("glNormal3f");
}

static void NoOpNormal3fv(const GLfloat *v)
{
   (void) v;
   warning("glNormal3fv");
}

static void NoOpNormal3i(GLint nx, GLint ny, GLint nz)
{
   (void) nx;
   (void) ny;
   (void) nz;
   warning("glNormal3i");
}

static void NoOpNormal3iv(const GLint *v)
{
   (void) v;
   warning("glNormal3iv");
}

static void NoOpNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
   (void) nx;
   (void) ny;
   (void) nz;
   warning("glNormal3s");
}

static void NoOpNormal3sv(const GLshort *v)
{
   (void) v;
   warning("glNormal3sv");
}

static void NoOpOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble nearval, GLdouble farval)
{
   (void) left;
   (void) right;
   (void) bottom;
   (void) top;
   (void) nearval;
   (void) farval;
   warning("glOrtho");
}

static void NoOpPassThrough(GLfloat token)
{
   (void) token;
   warning("glPassThrough");
}

static void NoOpPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values)
{
   (void) map;
   (void) mapsize;
   (void) values;
   warning("glPixelMapfv");
}

static void NoOpPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values)
{
   (void) map;
   (void) mapsize;
   (void) values;
   warning("glPixelMapuiv");
}

static void NoOpPixelMapusv(GLenum map, GLint mapsize, const GLushort *values)
{
   (void) map;
   (void) mapsize;
   (void) values;
   warning("glPixelMapusv");
}

static void NoOpPixelStoref(GLenum pname, GLfloat param)
{
   (void) pname;
   (void) param;
   warning("glPixelStoref");
}

static void NoOpPixelStorei(GLenum pname, GLint param)
{
   (void) pname;
   (void) param;
   warning("glPixelStorei");
}

static void NoOpPixelTransferf(GLenum pname, GLfloat param)
{
   (void) pname;
   (void) param;
   warning("glPixelTransferf");
}

static void NoOpPixelTransferi(GLenum pname, GLint param)
{
   (void) pname;
   (void) param;
   warning("glPixelTransferi");
}

static void NoOpPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
   (void) xfactor;
   (void) yfactor;
   warning("glPixelZoom");
}

static void NoOpPointSize(GLfloat size)
{
   (void) size;
   warning("glPointSize");
}

static void NoOpPolygonMode(GLenum face, GLenum mode)
{
   (void) face;
   (void) mode;
   warning("glPolygonMode");
}

static void NoOpPolygonStipple(const GLubyte *pattern)
{
   (void) pattern;
   warning("glPolygonStipple");
}

static void NoOpPopAttrib(void)
{
   warning("glPopAttrib");
}

static void NoOpPopMatrix(void)
{
   warning("glPopMatrix");
}

static void NoOpPopName(void)
{
   warning("glPopName");
}

static void NoOpPushMatrix(void)
{
   warning("glPushMatrix");
}

static void NoOpRasterPos2d(GLdouble x, GLdouble y)
{
   (void) x;
   (void) y;
   warning("glRasterPos2d");
}

static void NoOpRasterPos2f(GLfloat x, GLfloat y)
{
   (void) x;
   (void) y;
   warning("glRasterPos2f");
}

static void NoOpRasterPos2i(GLint x, GLint y)
{
   (void) x;
   (void) y;
   warning("glRasterPos2i");
}

static void NoOpRasterPos2s(GLshort x, GLshort y)
{
   (void) x;
   (void) y;
   warning("glRasterPos2s");
}

static void NoOpRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
   (void) x;
   (void) y;
   (void) z;
   warning("glRasterPos3d");
}

static void NoOpRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
   (void) x;
   (void) y;
   (void) z;
   warning("glRasterPos3f");
}

static void NoOpRasterPos3i(GLint x, GLint y, GLint z)
{
   (void) x;
   (void) y;
   (void) z;
   warning("glRasterPos3i");
}

static void NoOpRasterPos3s(GLshort x, GLshort y, GLshort z)
{
   (void) x;
   (void) y;
   (void) z;
   warning("glRasterPos3s");
}

static void NoOpRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   (void) x;
   (void) y;
   (void) z;
   (void) w;
   warning("glRasterPos4d");
}

static void NoOpRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   (void) x;
   (void) y;
   (void) z;
   (void) w;
   warning("glRasterPos4f");
}

static void NoOpRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
   (void) x;
   (void) y;
   (void) z;
   (void) w;
   warning("glRasterPos4i");
}

static void NoOpRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   (void) x;
   (void) y;
   (void) z;
   (void) w;
   warning("glRasterPos4s");
}

static void NoOpRasterPos2dv(const GLdouble *v)
{
   (void) v;
   warning("glRasterPos2dv");
}

static void NoOpRasterPos2fv(const GLfloat *v)
{
   (void) v;
   warning("glRasterPos2fv");
}

static void NoOpRasterPos2iv(const GLint *v)
{
   (void) v;
   warning("glRasterPos2iv");
}

static void NoOpRasterPos2sv(const GLshort *v)
{
   (void) v;
   warning("glRasterPos2sv");
}

static void NoOpRasterPos3dv(const GLdouble *v)
{
   (void) v;
   warning("glRasterPos3dv");
}

static void NoOpRasterPos3fv(const GLfloat *v)
{
   (void) v;
   warning("glRasterPos3fv");
}

static void NoOpRasterPos3iv(const GLint *v)
{
   (void) v;
   warning("glRasterPos3iv");
}

static void NoOpRasterPos3sv(const GLshort *v)
{
   (void) v;
   warning("glRasterPos3sv");
}

static void NoOpRasterPos4dv(const GLdouble *v)
{
   (void) v;
   warning("glRasterPos4dv");
}

static void NoOpRasterPos4fv(const GLfloat *v)
{
   (void) v;
   warning("glRasterPos4fv");
}

static void NoOpRasterPos4iv(const GLint *v)
{
   (void) v;
   warning("glRasterPos4iv");
}

static void NoOpRasterPos4sv(const GLshort *v)
{
   (void) v;
   warning("glRasterPos4sv");
}

static void NoOpReadBuffer(GLenum mode)
{ 
   (void) mode;
   warning("glReadBuffer");
}

static void NoOpReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels)
{
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glReadPixels");
}

static void NoOpRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
   (void) x1;
   (void) y1;
   (void) x2;
   (void) y2;
   warning("glRectd");
}

static void NoOpRectdv(const GLdouble *v1, const GLdouble *v2)
{
   (void) v1;
   (void) v2;
   warning("glRectdv");
}

static void NoOpRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
   (void) x1;
   (void) y1;
   (void) x2;
   (void) y2;
   warning("glRectf");
}

static void NoOpRectfv(const GLfloat *v1, const GLfloat *v2)
{
   (void) v1;
   (void) v2;
   warning("glRectfv");
}

static void NoOpRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
   (void) x1;
   (void) y1;
   (void) x2;
   (void) y2;
   warning("glRecti");
}

static void NoOpRectiv(const GLint *v1, const GLint *v2)
{
   (void) v1;
   (void) v2;
   warning("glRectiv");
}

static void NoOpRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
   (void) x1;
   (void) y1;
   (void) x2;
   (void) y2;
   warning("glRects");
}

static void NoOpRectsv(const GLshort *v1, const GLshort *v2)
{
   (void) v1;
   (void) v2;
   warning("glRectsv");
}

static void NoOpScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   warning("glScissor");
}

static GLboolean NoOpIsEnabled(GLenum cap)
{
   (void) cap;
   warning("glIsEnabled");
   return GL_FALSE;
}

static void NoOpPushAttrib(GLbitfield mask)
{
   (void) mask;
   warning("glPushAttrib");
}

static void NoOpPushName(GLuint name)
{
   (void) name;
   warning("glPushName");
}

static GLint NoOpRenderMode(GLenum mode)
{
   (void) mode;
   warning("glRenderMode");
   return 0;
}

static void NoOpRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
   (void) angle;
   (void) x;
   (void) y;
   (void) z;
   warning("glRotated");
}

static void NoOpRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   (void) angle;
   (void) x;
   (void) y;
   (void) z;
   warning("glRotatef");
}

static void NoOpSelectBuffer(GLsizei size, GLuint *buffer)
{
   (void) size;
   (void) buffer;
   warning("glSelectBuffer");
}

static void NoOpScaled(GLdouble x, GLdouble y, GLdouble z)
{
   (void) x;
   (void) y;
   (void) z;
   warning("glScaled");
}

static void NoOpScalef(GLfloat x, GLfloat y, GLfloat z)
{
   (void) x;
   (void) y;
   (void) z;
   warning("glScalef");
}

static void NoOpShadeModel(GLenum mode)
{
   (void) mode;
   warning("glShadeModel");
}

static void NoOpStencilFunc(GLenum func, GLint ref, GLuint mask)
{
   (void) func;
   (void) ref;
   (void) mask;
   warning("glStencilFunc");
}

static void NoOpStencilMask(GLuint mask)
{
   (void) mask;
   warning("glStencilMask");
}

static void NoOpStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
   (void) fail;
   (void) zfail;
   (void) zpass;
   warning("glStencilOp");
}

static void NoOpTexCoord1d(GLdouble s)
{
   (void) s;
   warning("glTexCoord1d");
}

static void NoOpTexCoord1dv(const GLdouble *v)
{
   (void) v;
   warning("glTexCoord1dv");
}

static void NoOpTexCoord1f(GLfloat s)
{
   (void) s;
   warning("glTexCoord1f");
}

static void NoOpTexCoord1fv(const GLfloat *v)
{
   (void) v;
   warning("glTexCoord1fv");
}

static void NoOpTexCoord1i(GLint s)
{
   (void) s;
   warning("glTexCoord1i");
}

static void NoOpTexCoord1iv(const GLint *v)
{
   (void) v;
   warning("glTexCoord1iv");
}

static void NoOpTexCoord1s(GLshort s)
{
   (void) s;
   warning("glTexCoord1s");
}

static void NoOpTexCoord1sv(const GLshort *v)
{
   (void) v;
   warning("glTexCoord1sv");
}

static void NoOpTexCoord2d(GLdouble s, GLdouble t)
{
   (void) s;
   (void) t;
   warning("glTexCoord2d");
}

static void NoOpTexCoord2dv(const GLdouble *v)
{
   (void) v;
   warning("glTexCoord2dv");
}

static void NoOpTexCoord2f(GLfloat s, GLfloat t)
{
   (void) s;
   (void) t;
   warning("glTexCoord2f");
}

static void NoOpTexCoord2fv(const GLfloat *v)
{
   (void) v;
   warning("glTexCoord2fv");
}

static void NoOpTexCoord2s(GLshort s, GLshort t)
{
   (void) s;
   (void) t;
   warning("glTexCoord2s");
}

static void NoOpTexCoord2sv(const GLshort *v)
{
   (void) v;
   warning("glTexCoord2sv");
}

static void NoOpTexCoord2i(GLint s, GLint t)
{
   (void) s;
   (void) t;
   warning("glTexCoord2i");
}

static void NoOpTexCoord2iv(const GLint *v)
{
   (void) v;
   warning("glTexCoord2iv");
}

static void NoOpTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
   (void) s;
   (void) t;
   (void) r;
   warning("glTexCoord3d");
}

static void NoOpTexCoord3dv(const GLdouble *v)
{
   (void) v;
   warning("glTexCoord3dv");
}

static void NoOpTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
   (void) s;
   (void) t;
   (void) r;
   warning("glTexCoord3f");
}

static void NoOpTexCoord3fv(const GLfloat *v)
{
   (void) v;
   warning("glTexCoord3fv");
}

static void NoOpTexCoord3i(GLint s, GLint t, GLint r)
{
   (void) s;
   (void) t;
   (void) r;
   warning("glTexCoord3i");
}

static void NoOpTexCoord3iv(const GLint *v)
{
   (void) v;
   warning("glTexCoord3iv");
}

static void NoOpTexCoord3s(GLshort s, GLshort t, GLshort r)
{
   (void) s;
   (void) t;
   (void) r;
   warning("glTexCoord3s");
}

static void NoOpTexCoord3sv(const GLshort *v)
{
   (void) v;
   warning("glTexCoord3sv");
}

static void NoOpTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   (void) s;
   (void) t;
   (void) r;
   warning("glTexCoord4d");
}

static void NoOpTexCoord4dv(const GLdouble *v)
{
   (void) v;
   warning("glTexCoord4dv");
}

static void NoOpTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   (void) s;
   (void) t;
   (void) r;
   (void) q;
   warning("glTexCoord4f");
}

static void NoOpTexCoord4fv(const GLfloat *v)
{
   (void) v;
   warning("glTexCoord4fv");
}

static void NoOpTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
   (void) s;
   (void) t;
   (void) r;
   (void) q;
   warning("glTexCoord4i");
}

static void NoOpTexCoord4iv(const GLint *v)
{
   (void) v;
   warning("glTexCoord4iv");
}

static void NoOpTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
   (void) s;
   (void) t;
   (void) r;
   (void) q;
   warning("glTexCoord4s");
}

static void NoOpTexCoord4sv(const GLshort *v)
{
   (void) v;
   warning("glTexCoord4sv");
}

static void NoOpTexGend(GLenum coord, GLenum pname, GLdouble param)
{
   (void) coord;
   (void) pname;
   (void) param;
   warning("glTexGend");
}

static void NoOpTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
   (void) coord;
   (void) pname;
   (void) param;
   warning("glTexGenf");
}

static void NoOpTexGeni(GLenum coord, GLenum pname, GLint param)
{
   (void) coord;
   (void) pname;
   (void) param;
   warning("glTexGeni");
}

static void NoOpTexGendv(GLenum coord, GLenum pname, const GLdouble *params)
{
   (void) coord;
   (void) pname;
   (void) params;
   warning("glTexGendv");
}

static void NoOpTexGeniv(GLenum coord, GLenum pname, const GLint *params)
{
   (void) coord;
   (void) pname;
   (void) params;
   warning("glTexGeniv");
}

static void NoOpTexGenfv(GLenum coord, GLenum pname, const GLfloat *params)
{
   (void) coord;
   (void) pname;
   (void) params;
   warning("glTexGenfv");
}

static void NoOpTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
   (void) target;
   (void) pname;
   (void) param;
   warning("glTexEnvf");
}

static void NoOpTexEnvi(GLenum target, GLenum pname, GLint param)
{
   (void) target;
   (void) pname;
   (void) param;
   warning("glTexEnvi");
}

static void NoOpTexEnvfv(GLenum target, GLenum pname, const GLfloat *param)
{
   (void) target;
   (void) pname;
   (void) param;
   warning("glTexEnvfv");
}

static void NoOpTexEnviv(GLenum target, GLenum pname, const GLint *param)
{
   (void) target;
   (void) pname;
   (void) param;
   warning("glTexEnviv");
}

static void NoOpTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   (void) target;
   (void) level;
   (void) internalformat;
   (void) width;
   (void) border;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glTexImage1D");
}

static void NoOpTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   (void) target;
   (void) level;
   (void) internalformat;
   (void) width;
   (void) height;
   (void) border;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glTexImage2D");
}

static void NoOpTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
   (void) target;
   (void) pname;
   (void) param;
   warning("glTexParameterf");
}

static void NoOpTexParameteri(GLenum target, GLenum pname, GLint param)
{
   (void) target;
   (void) pname;
   (void) param;
   warning("glTexParameteri");
}

static void NoOpTexParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glTexParameterfv");
}

static void NoOpTexParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glTexParameteriv");
}

static void NoOpTranslated(GLdouble x, GLdouble y, GLdouble z)
{
   (void) x;
   (void) y;
   (void) z;
   warning("glTranslated");
}

static void NoOpTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
   (void) x;
   (void) y;
   (void) z;
   warning("glTranslatef");
}

static void NoOpVertex2d(GLdouble x, GLdouble y)
{
   (void) x;
   (void) y;
   warning("glVertex2d");
}

static void NoOpVertex2dv(const GLdouble *v)
{
   (void) v;
   warning("glVertex2dv");
}

static void NoOpVertex2f(GLfloat x, GLfloat y)
{
   (void) x;
   (void) y;
   warning("glVertex2f");
}

static void NoOpVertex2fv(const GLfloat *v)
{
   (void) v;
   warning("glVertex2fv");
}

static void NoOpVertex2i(GLint x, GLint y)
{
   (void) x;
   (void) y;
   warning("glVertex2i");
}

static void NoOpVertex2iv(const GLint *v)
{
   (void) v;
   warning("glVertex2iv");
}

static void NoOpVertex2s(GLshort x, GLshort y)
{
   (void) x;
   (void) y;
   warning("glVertex2s");
}

static void NoOpVertex2sv(const GLshort *v)
{
   (void) v;
   warning("glVertex2sv");
}

static void NoOpVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
   (void) x;
   (void) y;
   (void) z;
   warning("glVertex3d");
}

static void NoOpVertex3dv(const GLdouble *v)
{
   (void) v;
   warning("glVertex3dv");
}

static void NoOpVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
   (void) x;
   (void) y;
   (void) z;
   warning("glVertex3f");
}

static void NoOpVertex3fv(const GLfloat *v)
{
   (void) v;
   warning("glVertex3fv");
}

static void NoOpVertex3i(GLint x, GLint y, GLint z)
{
   (void) x;
   (void) y;
   (void) z;
   warning("glVertex3i");
}

static void NoOpVertex3iv(const GLint *v)
{
   (void) v;
   warning("glVertex3iv");
}

static void NoOpVertex3s(GLshort x, GLshort y, GLshort z)
{
   (void) x;
   (void) y;
   (void) z;
   warning("glVertex3s");
}

static void NoOpVertex3sv(const GLshort *v)
{
   (void) v;
   warning("glVertex3sv");
}

static void NoOpVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
   (void) x;
   (void) y;
   (void) z;
   (void) w;
   warning("glVertex4d");
}

static void NoOpVertex4dv(const GLdouble *v)
{
   (void) v;
   warning("glVertex4dv");
}

static void NoOpVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   (void) x;
   (void) y;
   (void) z;
   (void) w;
   warning("glVertex4f");
}

static void NoOpVertex4fv(const GLfloat *v)
{
   (void) v;
   warning("glVertex4fv");
}

static void NoOpVertex4i(GLint x, GLint y, GLint z, GLint w)
{
   (void) x;
   (void) y;
   (void) z;
   (void) w;
   warning("glVertex4i");
}

static void NoOpVertex4iv(const GLint *v)
{
   (void) v;
   warning("glVertex4iv");
}

static void NoOpVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
   (void) x;
   (void) y;
   (void) z;
   (void) w;
   warning("glVertex4s");
}

static void NoOpVertex4sv(const GLshort *v)
{
   (void) v;
   warning("glVertex4sv");
}

static void NoOpViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   warning("glViewport");
}


/* GL 1.1 */

static GLboolean NoOpAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences)
{
   (void) n;
   (void) textures;
   (void) residences;
   warning("glAreTexturesResident");
   return GL_FALSE;
}

static void NoOpArrayElement(GLint i)
{
   (void) i;
   warning("glArrayElement");
}

static void NoOpBindTexture(GLenum target, GLuint texture)
{
   (void) target;
   (void) texture;
   warning("glBindTexture");
}

static void NoOpColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   (void) size;
   (void) type;
   (void) stride;
   (void) ptr;
   warning("glColorPointer");
}

static void NoOpCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
   (void) target;
   (void) level;
   (void) internalformat;
   (void) x;
   (void) y;
   (void) width;
   (void) border;
   warning("glCopyTexImage1D");
}

static void NoOpCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
   (void) target;
   (void) level;
   (void) internalformat;
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   (void) border;
   warning("glCopyTexImage2D");
}

static void NoOpCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) x;
   (void) y;
   (void) width;
   warning("glCopyTexSubImage1D");
}

static void NoOpCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) yoffset;
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   warning("glCopyTexSubImage2D");
}

static void NoOpDeleteTextures(GLsizei n, const GLuint *textures)
{
   (void) n;
   (void) textures;
   warning("glDeleteTextures");
}

static void NoOpDrawArrays(GLenum mode, GLint first, GLsizei count)
{
   (void) mode;
   (void) first;
   (void) count;
   warning("glDrawArrays");
}

static void NoOpDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices)
{
   (void) mode;
   (void) type;
   (void) indices;
   warning("glDrawElements");
}

static void NoOpEnableClientState(GLenum cap)
{
   (void) cap;
   warning("glEnableClientState");
}

static void NoOpGenTextures(GLsizei n, GLuint *textures)
{
   (void) n;
   (void) textures;
   warning("glGenTextures");
}

static void NoOpGetPointerv(GLenum pname, GLvoid **params)
{
   (void) pname;
   (void) params;
   warning("glGetPointerv");
}

static void NoOpIndexub(GLubyte c)
{
   (void) c;
   warning("glIndexub");
}

static void NoOpIndexubv(const GLubyte *c)
{
   (void) c;
   warning("glIndexubv");
}

static void NoOpIndexPointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   (void) type;
   (void) stride;
   (void) ptr;
   warning("glIndexPointer");
}

static void NoOpInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer)
{
   (void) format;
   (void) stride;
   (void) pointer;
   warning("glInterleavedArrays");
}

static GLboolean NoOpIsTexture(GLuint texture)
{
   (void) texture;
   warning("glIsTexture");
   return GL_FALSE;
}

static void NoOpNormalPointer(GLenum type, GLsizei stride, const GLvoid *ptr)
{
   (void) type;
   (void) stride;
   (void) ptr;
   warning("glNormalPointer");
}

static void NoOpPolygonOffset(GLfloat factor, GLfloat units)
{
   (void) factor;
   (void) units;
   warning("glPolygonOffset");
}

static void NoOpPopClientAttrib(void)
{
   warning("glPopClientAttrib");
}

static void NoOpPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
   (void) n;
   (void) textures;
   (void) priorities;
   warning("glPrioritizeTextures");
}

static void NoOpPushClientAttrib(GLbitfield mask)
{
   (void) mask;
   warning("glPushClientAttrib");
}

static void NoOpTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   (void) size;
   (void) type;
   (void) stride;
   (void) ptr;
   warning("glTexCoordPointer");
}

static void NoOpTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) width;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glTexSubImage1D");
}

static void NoOpTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) yoffset;
   (void) width;
   (void) height;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glTexSubImage2D");
}

static void NoOpVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *ptr)
{
   (void) size;
   (void) type;
   (void) stride;
   (void) ptr;
   warning("glVertexPointer");
}



/* GL 1.2 */

static void NoOpCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) yoffset;
   (void) zoffset;
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   warning("glCopyTexSubImage3D");
}

static void NoOpDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices)
{
   (void) mode;
   (void) start;
   (void) end;
   (void) count;
   (void) type;
   (void) indices;
   warning("glDrawRangeElements");
}

static void NoOpTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   (void) target;
   (void) level;
   (void) internalformat;
   (void) width;
   (void) height;
   (void) depth;
   (void) border;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glTexImage3D");
}

static void NoOpTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) yoffset;
   (void) zoffset;
   (void) width;
   (void) height;
   (void) depth;
   (void) depth;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glTexSubImage3D");
}



/* GL_ARB_imaging */

static void NoOpBlendColor(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
   (void) r;
   (void) g;
   (void) b;
   (void) a;
   warning("glBlendColor");
}

static void NoOpBlendEquation(GLenum eq)
{
   (void) eq;
   warning("glBlendEquation");
}

static void NoOpColorSubTable(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data)
{
   (void) target;
   (void) start;
   (void) count;
   (void) format;
   (void) type;
   (void) data;
   warning("glColorSubTable");
}

static void NoOpColorTable(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
   (void) target;
   (void) internalformat;
   (void) width;
   (void) format;
   (void) type;
   (void) table;
   warning("glColorTable");
}

static void NoOpColorTableParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glColorTableParameterfv");
}

static void NoOpColorTableParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glColorTableParameteriv");
}

static void NoOpConvolutionFilter1D(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
   (void) target;
   (void) internalformat;
   (void) width;
   (void) format;
   (void) type;
   (void) image;
   warning("glConvolutionFilter1D");
}

static void NoOpConvolutionFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
   (void) target;
   (void) internalformat;
   (void) width;
   (void) height;
   (void) format;
   (void) type;
   (void) image;
   warning("glConvolutionFilter2D");
}

static void NoOpConvolutionParameterf(GLenum target, GLenum pname, GLfloat params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glConvolutionParameterf");
}

static void NoOpConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glConvolutionParameterfv");
}

static void NoOpConvolutionParameteri(GLenum target, GLenum pname, GLint params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glConvolutionParameteri");
}

static void NoOpConvolutionParameteriv(GLenum target, GLenum pname, const GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glConvolutionParameteriv");
}

static void NoOpCopyColorSubTable(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
   (void) target;
   (void) start;
   (void) x;
   (void) y;
   (void) width;
   warning("glCopyColorSubTable");
}

static void NoOpCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   (void) target;
   (void) internalformat;
   (void) x;
   (void) y;
   (void) width;
   warning("glCopyColorTable");
}

static void NoOpCopyConvolutionFilter1D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   (void) target;
   (void) internalformat;
   (void) x;
   (void) y;
   (void) width;
   warning("glCopyConvolutionFilter1D");
}

static void NoOpCopyConvolutionFilter2D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
   (void) target;
   (void) internalformat;
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   warning("glCopyConvolutionFilter2D");
}

static void NoOpGetColorTable(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
   (void) target;
   (void) format;
   (void) type;
   (void) table;
   warning("glGetColorTable");
}

static void NoOpGetColorTableParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetColorTableParameterfv");
}

static void NoOpGetColorTableParameteriv(GLenum target, GLenum pname, GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetColorTableParameteriv");
}

static void NoOpGetConvolutionFilter(GLenum target, GLenum format, GLenum type, GLvoid *image)
{
   (void) target;
   (void) format;
   (void) type;
   (void) image;
   warning("glGetConvolutionFilter");
}

static void NoOpGetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetConvolutionParameterfv");
}

static void NoOpGetConvolutionParameteriv(GLenum target, GLenum pname, GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetConvolutionParameteriv");
}

static void NoOpGetMinmax(GLenum target, GLboolean reset, GLenum format, GLenum types, GLvoid *values)
{
   (void) target;
   (void) reset;
   (void) format;
   (void) types;
   (void) values;
   warning("glGetMinmax");
}

static void NoOpGetHistogram(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
   (void) target;
   (void) reset;
   (void) format;
   (void) type;
   (void) values;
   warning("glGetHistogram");
}

static void NoOpGetHistogramParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetHistogramParameterfv");
}

static void NoOpGetHistogramParameteriv(GLenum target, GLenum pname, GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetHistogramParameteriv");
}

static void NoOpGetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetMinmaxParameterfv");
}

static void NoOpGetMinmaxParameteriv(GLenum target, GLenum pname, GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetMinmaxParameteriv");
}

static void NoOpGetSeparableFilter(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span)
{
   (void) target;
   (void) format;
   (void) type;
   (void) row;
   (void) column;
   (void) span;
   warning("glGetSeperableFilter");
}

static void NoOpHistogram(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
   (void) target;
   (void) width;
   (void) internalformat;
   (void) sink;
   warning("glHistogram");
}

static void NoOpMinmax(GLenum target, GLenum internalformat, GLboolean sink)
{
   (void) target;
   (void) internalformat;
   (void) sink;
   warning("glMinmax");
}

static void NoOpResetHistogram(GLenum target)
{
   (void) target;
   warning("glResetHistogram");
}

static void NoOpResetMinmax(GLenum target)
{
   (void) target;
   warning("glResetMinmax");
}

static void NoOpSeparableFilter2D(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)
{
   (void) target;
   (void) internalformat;
   (void) width;
   (void) height;
   (void) format;
   (void) type;
   (void) row;
   (void) column;
   warning("glSeparableFilter2D");
}




/* GL_ARB_multitexture */

static void NoOpActiveTextureARB(GLenum texture)
{
   (void) texture;
   warning("glActiveTextureARB");
}

static void NoOpClientActiveTextureARB(GLenum texture)
{
   (void) texture;
   warning("glClientActiveTextureARB");
}

static void NoOpMultiTexCoord1dARB(GLenum target, GLdouble s)
{
   (void) target;
   (void) s;
   warning("glMultiTexCoord1dARB");
}

static void NoOpMultiTexCoord1dvARB(GLenum target, const GLdouble *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord1dvARB");
}

static void NoOpMultiTexCoord1fARB(GLenum target, GLfloat s)
{
   (void) target;
   (void) s;
   warning("glMultiTexCoord1fARB");
}

static void NoOpMultiTexCoord1fvARB(GLenum target, const GLfloat *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord1fvARB");
}

static void NoOpMultiTexCoord1iARB(GLenum target, GLint s)
{
   (void) target;
   (void) s;
   warning("glMultiTexCoord1iARB");
}

static void NoOpMultiTexCoord1ivARB(GLenum target, const GLint *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord1ivARB");
}

static void NoOpMultiTexCoord1sARB(GLenum target, GLshort s)
{
   (void) target;
   (void) s;
   warning("glMultiTexCoord1sARB");
}

static void NoOpMultiTexCoord1svARB(GLenum target, const GLshort *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord1svARB");
}

static void NoOpMultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t)
{
   (void) target;
   (void) s;
   (void) t;
   warning("glMultiTexCoord2dARB");
}

static void NoOpMultiTexCoord2dvARB(GLenum target, const GLdouble *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord2dvARB");
}

static void NoOpMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
{
   (void) target;
   (void) s;
   (void) t;
   warning("glMultiTexCoord2fARB");
}

static void NoOpMultiTexCoord2fvARB(GLenum target, const GLfloat *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord2fvARB");
}

static void NoOpMultiTexCoord2iARB(GLenum target, GLint s, GLint t)
{
   (void) target;
   (void) s;
   (void) t;
   warning("glMultiTexCoord2iARB");
}

static void NoOpMultiTexCoord2ivARB(GLenum target, const GLint *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord2ivARB");
}

static void NoOpMultiTexCoord2sARB(GLenum target, GLshort s, GLshort t)
{
   (void) target;
   (void) s;
   (void) t;
   warning("glMultiTexCoord2sARB");
}

static void NoOpMultiTexCoord2svARB(GLenum target, const GLshort *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord2svARB");
}

static void NoOpMultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
   (void) target;
   (void) s;
   (void) t;
   (void) r;
   warning("glMultiTexCoord3dARB");
}

static void NoOpMultiTexCoord3dvARB(GLenum target, const GLdouble *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord3dvARB");
}

static void NoOpMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
   (void) target;
   (void) s;
   (void) t;
   (void) r;
   warning("glMultiTexCoord3fARB");
}

static void NoOpMultiTexCoord3fvARB(GLenum target, const GLfloat *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord3fvARB");
}

static void NoOpMultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r)
{
   (void) target;
   (void) s;
   (void) t;
   (void) r;
   warning("glMultiTexCoord3iARB");
}

static void NoOpMultiTexCoord3ivARB(GLenum target, const GLint *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord3ivARB");
}

static void NoOpMultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r)
{
   (void) target;
   (void) s;
   (void) t;
   (void) r;
   warning("glMultiTexCoord3sARB");
}

static void NoOpMultiTexCoord3svARB(GLenum target, const GLshort *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord3svARB");
}

static void NoOpMultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
   (void) target;
   (void) s;
   (void) t;
   (void) r;
   (void) q;
   warning("glMultiTexCoord4dARB");
}

static void NoOpMultiTexCoord4dvARB(GLenum target, const GLdouble *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord4dvARB");
}

static void NoOpMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   (void) target;
   (void) s;
   (void) t;
   (void) r;
   (void) q;
   warning("glMultiTexCoord4fARB");
}

static void NoOpMultiTexCoord4fvARB(GLenum target, const GLfloat *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord4fvARB");
}

static void NoOpMultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
   (void) target;
   (void) s;
   (void) t;
   (void) r;
   (void) q;
   warning("glMultiTexCoord4iARB");
}

static void NoOpMultiTexCoord4ivARB(GLenum target, const GLint *v)
{
   (void) target;
   (void) v;
   warning("glMultiTexCoord4ivARB");
}

static void NoOpMultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
   (void) target;
   (void) s;
   (void) t;
   (void) r;
   (void) q;
   warning("glMultiTexCoord4sARB");
}

static void NoOpMultiTexCoord4svARB(GLenum target, const GLshort *v)
{
   (void) v;
   warning("glMultiTexCoord4svARB");
}




/***
 *** Extension functions
 ***/


/* 2. GL_EXT_blend_color */
static void NoOpBlendColorEXT(GLclampf r, GLclampf g, GLclampf b, GLclampf a)
{
   (void) r;
   (void) g;
   (void) b;
   (void) a;
   warning("glBlendColor");
}


/* 3. GL_EXT_polygon_offset */
static void NoOpPolygonOffsetEXT(GLfloat factor, GLfloat bias)
{
   (void) factor;
   (void) bias;
   warning("glPolygonOffsetEXT");
}



/* 6. GL_EXT_texture3D */

static void NoOpTexImage3DEXT(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels)
{
   (void) target;
   (void) level;
   (void) internalformat;
   (void) width;
   (void) height;
   (void) depth;
   (void) border;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glTexImage3DEXT");
}

static void NoOpTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) yoffset;
   (void) zoffset;
   (void) width;
   (void) height;
   (void) depth;
   (void) depth;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glTexSubImage3DEXT");
}

static void NoOpCopyTexSubImage3DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) yoffset;
   (void) zoffset;
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   warning("glCopyTexSubImage3DEXT");
}



/* 7. GL_SGI_texture_filter4 */

static void NoOpGetTexFilterFuncSGIS(GLenum target, GLenum filter, GLsizei n, const GLfloat *weights)
{
   (void) target;
   (void) filter;
   (void) n;
   (void) weights;
   warning("glGetTexFilterFuncSGIS");
}

static void NoOpTexFilterFuncSGIS(GLenum target, GLenum filter, GLfloat *weights)
{
   (void) target;
   (void) filter;
   (void) weights;
   warning("glTexFilterFuncSGIS");
}



/* 9. GL_EXT_subtexture */

static void NoOpCopyTexSubImage1DEXT(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) x;
   (void) y;
   (void) width;
   warning("glCopyTexSubImage1DEXT");
}

static void NoOpTexSubImage1DEXT(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) width;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glTexSubImage2DEXT");
}
static void NoOpTexSubImage2DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) yoffset;
   (void) width;
   (void) height;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glTexSubImage2DEXT");
}


/* 10. GL_EXT_copy_texture */

static void NoOpCopyTexImage1DEXT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
   (void) target;
   (void) level;
   (void) internalformat;
   (void) x;
   (void) y;
   (void) width;
   (void) border;
   warning("glCopyTexImage1DEXT");
}

static void NoOpCopyTexImage2DEXT(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
   (void) target;
   (void) level;
   (void) internalformat;
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   (void) border;
   warning("glCopyTexImage2DEXT");
}


static void NoOpCopyTexSubImage2DEXT(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) yoffset;
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   warning("glCopyTexSubImage2DEXT");
}



/* 11. GL_EXT_histogram */
static void NoOpGetHistogramEXT(GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values)
{
   (void) target;
   (void) reset;
   (void) format;
   (void) type;
   (void) values;
   warning("glGetHistogramEXT");
}

static void NoOpGetHistogramParameterfvEXT(GLenum target, GLenum pname, GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetHistogramParameterfvEXT");
}

static void NoOpGetHistogramParameterivEXT(GLenum target, GLenum pname, GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetHistogramParameterivEXT");
}

static void NoOpGetMinmaxEXT(GLenum target, GLboolean reset, GLenum format, GLenum types, GLvoid *values)
{
   (void) target;
   (void) reset;
   (void) format;
   (void) types;
   (void) values;
   warning("glGetMinmaxEXT");
}

static void NoOpGetMinmaxParameterfvEXT(GLenum target, GLenum pname, GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetMinmaxParameterfvEXT");
}

static void NoOpGetMinmaxParameterivEXT(GLenum target, GLenum pname, GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetMinmaxParameterivEXT");
}

static void NoOpHistogramEXT(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
   (void) target;
   (void) width;
   (void) internalformat;
   (void) sink;
   warning("glHistogramEXT");
}

static void NoOpMinmaxEXT(GLenum target, GLenum internalformat, GLboolean sink)
{
   (void) target;
   (void) internalformat;
   (void) sink;
   warning("glMinmaxEXT");
}

static void NoOpResetHistogramEXT(GLenum target)
{
   (void) target;
   warning("glResetHistogramEXT");
}

static void NoOpResetMinmaxEXT(GLenum target)
{
   (void) target;
   warning("glResetMinmaxEXT");
}



/* 12. GL_EXT_convolution */

static void NoOpConvolutionFilter1DEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image)
{
   (void) target;
   (void) internalformat;
   (void) width;
   (void) format;
   (void) type;
   (void) image;
   warning("glConvolutionFilter1DEXT");
}

static void NoOpConvolutionFilter2DEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image)
{
   (void) target;
   (void) internalformat;
   (void) width;
   (void) height;
   (void) format;
   (void) type;
   (void) image;
   warning("glConvolutionFilter2DEXT");
}

static void NoOpConvolutionParameterfEXT(GLenum target, GLenum pname, GLfloat params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glConvolutionParameterfEXT");
}

static void NoOpConvolutionParameterfvEXT(GLenum target, GLenum pname, const GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glConvolutionParameterfvEXT");
}

static void NoOpConvolutionParameteriEXT(GLenum target, GLenum pname, GLint params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glConvolutionParameteriEXT");
}

static void NoOpConvolutionParameterivEXT(GLenum target, GLenum pname, const GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glConvolutionParameterivEXT");
}

static void NoOpCopyConvolutionFilter1DEXT(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   (void) target;
   (void) internalformat;
   (void) x;
   (void) y;
   (void) width;
   warning("glCopyConvolutionFilter1DEXT");
}

static void NoOpCopyConvolutionFilter2DEXT(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
   (void) target;
   (void) internalformat;
   (void) x;
   (void) y;
   (void) width;
   (void) height;
   warning("glCopyConvolutionFilter2DEXT");
}

static void NoOpGetConvolutionFilterEXT(GLenum target, GLenum format, GLenum type, GLvoid *image)
{
   (void) target;
   (void) format;
   (void) type;
   (void) image;
   warning("glGetConvolutionFilterEXT");
}

static void NoOpGetConvolutionParameterfvEXT(GLenum target, GLenum pname, GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetConvolutionParameterfvEXT");
}

static void NoOpGetConvolutionParameterivEXT(GLenum target, GLenum pname, GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetConvolutionParameterivEXT");
}

static void NoOpGetSeparableFilterEXT(GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span)
{
   (void) target;
   (void) format;
   (void) type;
   (void) row;
   (void) column;
   (void) span;
   warning("glGetSeperableFilterEXT");
}

static void NoOpSeparableFilter2DEXT(GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column)
{
   (void) target;
   (void) internalformat;
   (void) width;
   (void) height;
   (void) format;
   (void) type;
   (void) row;
   (void) column;
   warning("glSeparableFilter2DEXT");
}


/* 14. GL_SGI_color_table */

static void NoOpColorTableSGI(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
   (void) target;
   (void) internalformat;
   (void) width;
   (void) format;
   (void) type;
   (void) table;
   warning("glColorTableSGI");
}

static void NoOpColorTableParameterfvSGI(GLenum target, GLenum pname, const GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glColorTableParameterfvSGI");
}

static void NoOpColorTableParameterivSGI(GLenum target, GLenum pname, const GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glColorTableParameterivSGI");
}

static void NoOpCopyColorTableSGI(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
   (void) target;
   (void) internalformat;
   (void) x;
   (void) y;
   (void) width;
   warning("glCopyColorTableSGI");
}

static void NoOpGetColorTableSGI(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
   (void) target;
   (void) format;
   (void) type;
   (void) table;
   warning("glGetColorTableSGI");
}

static void NoOpGetColorTableParameterfvSGI(GLenum target, GLenum pname, GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetColorTableParameterfvSGI");
}

static void NoOpGetColorTableParameterivSGI(GLenum target, GLenum pname, GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetColorTableParameterivSGI");
}


/* 15. GL_SGIS_pixel_texture */
static void NoOpPixelTexGenParameterfSGIS(GLenum pname, GLfloat value)
{
   (void) pname;
   (void) value;
   warning("glPixelTexGenParameterfSGIS");
}

static void NoOpGetPixelTexGenParameterfvSGIS(GLenum pname, GLfloat *value)
{
   (void) pname;
   (void) value;
   warning("glGetPixelTexGenParameterfvSGIS");
}

static void NoOpPixelTexGenParameteriSGIS(GLenum pname, GLint value)
{
   (void) pname;
   (void) value;
   warning("glPixelTexGenParameteriSGIS");
}

static void NoOpGetPixelTexGenParameterivSGIS(GLenum pname, GLint *value)
{
   (void) pname;
   (void) value;
   warning("glGetPixelTexGenParameterivSGIS");
}



/* 16. GL_SGIS_texture4D */

static void NoOpTexImage4DSGIS(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLint border, GLenum format, GLenum type, const void *pixels)
{
   (void) target;
   (void) level;
   (void) internalformat;
   (void) width;
   (void) height;
   (void) depth;
   (void) extent;
   (void) border;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glTexImage4DSGIS");
}

static void NoOpTexSubImage4DSGIS(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei extent, GLenum format, GLenum type, const void *pixels)
{
   (void) target;
   (void) level;
   (void) xoffset;
   (void) yoffset;
   (void) zoffset;
   (void) woffset;
   (void) width;
   (void) height;
   (void) depth;
   (void) extent;
   (void) format;
   (void) type;
   (void) pixels;
   warning("glTexSubImage4DSGIS");
}


/* 20. GL_EXT_texture_object */

static GLboolean NoOpAreTexturesResidentEXT(GLsizei n, const GLuint *textures, GLboolean *residences)
{
   (void) n;
   (void) textures;
   (void) residences;
   warning("glAreTexturesResidentEXT");
   return GL_FALSE;
}

static void NoOpBindTextureEXT(GLenum target, GLuint texture)
{
   (void) target;
   (void) texture;
   warning("glBindTextureEXT");
}

static void NoOpDeleteTexturesEXT(GLsizei n, const GLuint *textures)
{
   (void) n;
   (void) textures;
   warning("glDeleteTexturesEXT");
}

static void NoOpGenTexturesEXT(GLsizei n, GLuint *textures)
{
   (void) n;
   (void) textures;
   warning("glGenTexturesEXT");
}

static GLboolean NoOpIsTextureEXT(GLuint texture)
{
   (void) texture;
   warning("glIsTextureEXT");
   return GL_FALSE;
}

static void NoOpPrioritizeTexturesEXT(GLsizei n, const GLuint *textures, const GLclampf *priorities)
{
   (void) n;
   (void) textures;
   (void) priorities;
   warning("glPrioritizeTexturesEXT");
}



/* 21. GL_SGIS_detail_texture */

static void NoOpDetailTexFuncSGIS(GLenum target, GLsizei n, const GLfloat *value)
{
   (void) target;
   (void) n;
   (void) value;
   warning("glDetailTexFuncSGIS");
}

static void NoOpGetDetailTexFuncSGIS(GLenum target, GLfloat *value)
{
   (void) target;
   (void) value;
   warning("glGetDetailTexFuncSGIS");
}



/* 22. GL_SGIS_sharpen_texture */

static void NoOpGetSharpenTexFuncSGIS(GLenum target, GLfloat *value)
{
   (void) target;
   (void) value;
   warning("glGetSharpenTexFuncSGIS");
}

static void NoOpSharpenTexFuncSGIS(GLenum target, GLsizei n, const GLfloat *value)
{
   (void) target;
   (void) n;
   (void) value;
   warning("glSharpenTexFuncSGIS");
}



/* 25. GL_SGIS_multisample */

static void NoOpSampleMaskSGIS(GLclampf range, GLboolean invert)
{
   (void) range;
   (void) invert;
   warning("glSampleMaskSGIS");
}

static void NoOpSamplePatternSGIS(GLenum pattern)
{
   (void) pattern;
   warning("glSamplePatternSGIS");
}



/* 30. GL_EXT_vertex_array */

static void NoOpArrayElementEXT(GLint i)
{
   (void) i;
   warning("glArrayElementEXT");
}

static void NoOpColorPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer)
{
   (void) size;
   (void) type;
   (void) stride;
   (void) count;
   (void) pointer;
   warning("glColorPointerEXT");
}

static void NoOpDrawArraysEXT(GLenum mode, GLint first, GLsizei count)
{
   (void) mode;
   (void) first;
   (void) count;
   warning("glDrawArraysEXT");
}

static void NoOpEdgeFlagPointerEXT(GLsizei stride, GLsizei count, const GLboolean *pointer)
{
   (void) stride;
   (void) count;
   (void) pointer;
   warning("glEdgeFlagPointerEXT");
}

static void NoOpGetPointervEXT(GLenum pname, void **params)
{
   (void) pname;
   (void) params;
   warning("glGetPointervEXT");
}

static void NoOpIndexPointerEXT(GLenum type, GLsizei stride, GLsizei count, const void *pointer)
{
   (void) type;
   (void) stride;
   (void) count;
   (void) pointer;
   warning("glIndexPointerEXT");
}

static void NoOpNormalPointerEXT(GLenum type, GLsizei stride, GLsizei count, const void *pointer)
{
   (void) type;
   (void) stride;
   (void) count;
   (void) pointer;
   warning("glNormalPointerEXT");
}

static void NoOpTexCoordPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer)
{
   (void) size;
   (void) type;
   (void) stride;
   (void) count;
   (void) pointer;
   warning("glTexCoordPointerEXT");
}

static void NoOpVertexPointerEXT(GLint size, GLenum type, GLsizei stride, GLsizei count, const void *pointer)
{
   (void) size;
   (void) type;
   (void) stride;
   (void) count;
   (void) pointer;
   warning("glVertexPointerEXT");
}


/* 52. GL_SGIX_sprite */
static void NoOpSpriteParameterfSGIX(GLenum target, GLfloat value)
{
   warning("glSpriteParameterfSGIX");
}

static void NoOpSpriteParameterfvSGIX(GLenum target, const GLfloat *value)
{
   warning("glSpriteParameterfvSGIX");
}

static void NoOpSpriteParameteriSGIX(GLenum target, GLint value)
{
   warning("glSpriteParameteriSGIX");
}

static void NoOpSpriteParameterivSGIX(GLenum target, const GLint *value)
{
   warning("glSpriteParameterivSGIX");
}



/* 54. GL_EXT_point_parameters */

static void NoOpPointParameterfEXT(GLenum target, GLfloat param)
{
   (void) target;
   (void) param;
   warning("glPointParameterfEXT");
}

static void NoOpPointParameterfvEXT(GLenum target, const GLfloat *param)
{
   (void) target;
   (void) param;
   warning("glPointParameterfvEXT");
}



/* 55. GL_SGIX_instruments */

static GLint NoOpGetInstrumentsSGIX(void)
{
   warning("glGetInstrumentsSGIX");
   return 0;
}

static void NoOpInstrumentsBufferSGIX(GLsizei n, GLint *values)
{
   (void) n;
   (void) values;
   warning("glInstrumentsBufferSGIX");
}

static GLint NoOpPollInstrumentsSGIX(GLint *values)
{
   (void) values;
   warning("glPollInstrumentsSGIX");
   return 0;
}

static void NoOpReadInstrumentsSGIX(GLint value)
{
   (void) value;
   warning("glReadInstrumentsSGIX");
}

static void NoOpStartInstrumentsSGIX(void)
{
   warning("glStartInstrumentsSGIX");
}

static void NoOpStopInstrumentsSGIX(GLint value)
{
   (void) value;
   warning("glStopInstrumentsSGIX");
}



/* 57. GL_SGIX_framezoom */
static void NoOpFrameZoomSGIX(GLint factor)
{
   (void) factor;
   warning("glFrameZoomSGIX");
}



/* 60. GL_SGIX_reference_plane */
static void NoOpReferencePlaneSGIX(const GLdouble *equation)
{
   (void) equation;
   warning("glReferencePlaneSGIX");
}



/* 61. GL_SGIX_flush_raster */
static void NoOpFlushRasterSGIX(void)
{
   warning("glFlushRasterSGIX");
}


/* 66. GL_HP_image_transform */
static void NoOpGetImageTransformParameterfvHP(GLenum target, GLenum pname, GLfloat *param)
{
   warning("glGetImageTransformParameterfvHP");
}

static void NoOpGetImageTransformParameterivHP(GLenum target, GLenum pname, GLint *param)
{
   warning("glGetImageTransformParameterivHP");
}

static void NoOpImageTransformParameterfHP(GLenum target, GLenum pname, GLfloat param)
{
   warning("glImageTransformParameterfHP");
}

static void NoOpImageTransformParameterfvHP(GLenum target, GLenum pname, const GLfloat *params)
{
   warning("glImageTransformParameterfvHP");
}

static void NoOpImageTransformParameteriHP(GLenum target, GLenum pname, GLint param)
{
   warning("glImageTransformParameteriHP");
}

static void NoOpImageTransformParameterivHP(GLenum target, GLenum pname, const GLint *params)
{
   warning("glImageTransformParameterivHP");
}



/* 74. GL_EXT_color_subtable */

static void NoOpColorSubTableEXT(GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data)
{
   (void) target;
   (void) start;
   (void) count;
   (void) format;
   (void) type;
   (void) data;
   warning("glColorSubTableEXT");
}

static void NoOpCopyColorSubTableEXT(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
   (void) target;
   (void) start;
   (void) x;
   (void) y;
   (void) width;
   warning("glCopyColorSubTableEXT");
}



/* 77. GL_PGI_misc_hints */
static void NoOpHintPGI(GLenum target, GLint mode)
{
   (void) target;
   (void) mode;
   warning("glHintPGI");
}



/* 78. GL_EXT_paletted_texture */

static void NoOpColorTableEXT(GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table)
{
   (void) target;
   (void) internalformat;
   (void) width;
   (void) format;
   (void) type;
   (void) table;
   warning("glColorTableEXT");
}

static void NoOpGetColorTableEXT(GLenum target, GLenum format, GLenum type, GLvoid *table)
{
   (void) target;
   (void) format;
   (void) type;
   (void) table;
   warning("glGetColorTableEXT");
}

static void NoOpGetColorTableParameterfvEXT(GLenum target, GLenum pname, GLfloat *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetColorTableParameterfvEXT");
}

static void NoOpGetColorTableParameterivEXT(GLenum target, GLenum pname, GLint *params)
{
   (void) target;
   (void) pname;
   (void) params;
   warning("glGetColorTableParameterivEXT");
}




/* 80. GL_SGIX_list_priority */
static void NoOpGetListParameterfvSGIX(GLuint list, GLenum pname, GLfloat *param)
{
   warning("glGetListParameterfvSGIX");
}

static void NoOpGetListParameterivSGIX(GLuint list, GLenum pname, GLint *param)
{
   warning("glGetListParameterivSGIX");
}

static void NoOpListParameterfSGIX(GLuint list, GLenum pname, GLfloat param)
{
   warning("glListParameterfSGIX");
}

static void NoOpListParameterfvSGIX(GLuint list, GLenum pname, const GLfloat *params)
{
   warning("glListParameterfvSGIX");
}

static void NoOpListParameteriSGIX(GLuint list, GLenum pname, GLint param)
{
   warning("glListParameteriSGIX");
}

static void NoOpListParameterivSGIX(GLuint list, GLenum pname, const GLint *params)
{
   warning("glListParameterivSGIX");
}


/* 94. GL_EXT_index_material */
static void NoOpIndexMaterialEXT(GLenum face, GLenum mode)
{
   (void) face;
   (void) mode;
   warning("glIndexMaterialEXT");
}


/* 95. GL_EXT_index_func */
static void NoOpIndexFuncEXT(GLenum pname, GLfloat value)
{
   (void) pname;
   (void) value;
   warning("glIndexFuncEXT");
}


/* 97. GL_EXT_compiled_vertex_array */

static void NoOpLockArraysEXT(GLint first, GLsizei count)
{
   (void) first;
   (void) count;
   warning("glLockArraysEXT");
}

static void NoOpUnlockArraysEXT(void)
{
   warning("glUnlockArraysEXT");
}



/* 98. GL_EXT_cull_vertex */

static void NoOpCullParameterfvEXT(GLenum pname, const GLfloat *values)
{
   (void) pname;
   (void) values;
   warning("glCullParameterfvEXT");
}

static void NoOpCullParameterdvEXT(GLenum pname, const GLdouble *values)
{
   (void) pname;
   (void) values;
   warning("glCullParameterdvEXT");
}



/* 37. GL_EXT_blend_minmax */

static void NoOpBlendEquationEXT(GLenum mode)
{
   (void) mode;
   warning("glBlendEuqationEXT");
}



/* GL_EXT/INGR_blend_func_separate */
static void NoOpBlendFuncSeparateINGR(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
   (void) sfactorRGB;
   (void) dfactorRGB;
   (void) sfactorAlpha;
   (void) dfactorAlpha;
   warning("glBlendFuncSeparateINGR");
}



/* GL_MESA_window_pos */
static void NoOpWindowPos4fMESA(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   (void) x;
   (void) y;
   (void) z;
   (void) w;
   warning("glWindowPos[234][ifdv][v]MESA");
}



/* GL_MESA_resize_buffers */
static void NoOpResizeBuffersMESA(void)
{
   warning("glResizeBuffersMESA");
}



/* GL_ARB_transpose_matrix */

static void NoOpLoadTransposeMatrixdARB(const GLdouble mat[16])
{
   (void) mat;
   warning("glLoadTransposeMatrixdARB");
}

static void NoOpLoadTransposeMatrixfARB(const GLfloat mat[16])
{
   (void) mat;
   warning("glLoadTransposeMatrixfARB");
}

static void NoOpMultTransposeMatrixdARB(const GLdouble mat[16])
{
   (void) mat;
   warning("glMultTransposeMatrixdARB");
}

static void NoOpMultTransposeMatrixfARB(const GLfloat mat[16])
{
   (void) mat;
   warning("glMultTransposeMatrixfARB");
}



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
   NoOpWindowPos4fMESA,

   /* GL_MESA_resize_buffers */
   NoOpResizeBuffersMESA,

   /* GL_ARB_transpose_matrix */
   NoOpLoadTransposeMatrixdARB,
   NoOpLoadTransposeMatrixfARB,
   NoOpMultTransposeMatrixdARB,
   NoOpMultTransposeMatrixfARB,

};

