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
 * $PI: xc/lib/GL/mesa/src/mesa_api.h,v 1.1 1999/03/23 06:20:15 martin Exp $
 */

#ifndef _MESA_API_H_
#define _MESA_API_H_

#ifdef GLX_DIRECT_RENDERING

#include <GL/gl.h>
#include "mesa_api_wrap.h"

/* NOTE: This file could be automatically generated */

void __MESA_SW_glAccum(GLenum op, GLfloat value);
void __MESA_SW_glAlphaFunc(GLenum func, GLclampf ref);
GLboolean __MESA_SW_glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences);
void __MESA_SW_glArrayElement(GLint i);
void __MESA_SW_glBegin(GLenum mode);
void __MESA_SW_glBindTexture(GLenum target, GLuint texture);
void __MESA_SW_glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void __MESA_SW_glBlendFunc(GLenum sfactor, GLenum dfactor);
void __MESA_SW_glCallList(GLuint list);
void __MESA_SW_glCallLists(GLsizei n, GLenum type, const GLvoid *lists);
void __MESA_SW_glClear(GLbitfield mask);
void __MESA_SW_glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void __MESA_SW_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void __MESA_SW_glClearDepth(GLclampd depth);
void __MESA_SW_glClearIndex(GLfloat c);
void __MESA_SW_glClearStencil(GLint s);
void __MESA_SW_glClipPlane(GLenum plane, const GLdouble *equation);
void __MESA_SW_glColor3b(GLbyte red, GLbyte green, GLbyte blue);
void __MESA_SW_glColor3bv(const GLbyte *v);
void __MESA_SW_glColor3d(GLdouble red, GLdouble green, GLdouble blue);
void __MESA_SW_glColor3dv(const GLdouble *v);
void __MESA_SW_glColor3f(GLfloat red, GLfloat green, GLfloat blue);
void __MESA_SW_glColor3fv(const GLfloat *v);
void __MESA_SW_glColor3i(GLint red, GLint green, GLint blue);
void __MESA_SW_glColor3iv(const GLint *v);
void __MESA_SW_glColor3s(GLshort red, GLshort green, GLshort blue);
void __MESA_SW_glColor3sv(const GLshort *v);
void __MESA_SW_glColor3ub(GLubyte red, GLubyte green, GLubyte blue);
void __MESA_SW_glColor3ubv(const GLubyte *v);
void __MESA_SW_glColor3ui(GLuint red, GLuint green, GLuint blue);
void __MESA_SW_glColor3uiv(const GLuint *v);
void __MESA_SW_glColor3us(GLushort red, GLushort green, GLushort blue);
void __MESA_SW_glColor3usv(const GLushort *v);
void __MESA_SW_glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
void __MESA_SW_glColor4bv(const GLbyte *v);
void __MESA_SW_glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
void __MESA_SW_glColor4dv(const GLdouble *v);
void __MESA_SW_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void __MESA_SW_glColor4fv(const GLfloat *v);
void __MESA_SW_glColor4i(GLint red, GLint green, GLint blue, GLint alpha);
void __MESA_SW_glColor4iv(const GLint *v);
void __MESA_SW_glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha);
void __MESA_SW_glColor4sv(const GLshort *v);
void __MESA_SW_glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void __MESA_SW_glColor4ubv(const GLubyte *v);
void __MESA_SW_glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha);
void __MESA_SW_glColor4uiv(const GLuint *v);
void __MESA_SW_glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha);
void __MESA_SW_glColor4usv(const GLushort *v);
void __MESA_SW_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void __MESA_SW_glColorMaterial(GLenum face, GLenum mode);
void __MESA_SW_glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void __MESA_SW_glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void __MESA_SW_glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
void __MESA_SW_glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void __MESA_SW_glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void __MESA_SW_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void __MESA_SW_glCullFace(GLenum mode);
void __MESA_SW_glDeleteLists(GLuint list, GLsizei range);
void __MESA_SW_glDeleteTextures(GLsizei n, const GLuint *textures);
void __MESA_SW_glDepthFunc(GLenum func);
void __MESA_SW_glDepthMask(GLboolean flag);
void __MESA_SW_glDepthRange(GLclampd zNear, GLclampd zFar);
void __MESA_SW_glDisable(GLenum cap);
void __MESA_SW_glDisableClientState(GLenum array);
void __MESA_SW_glDrawArrays(GLenum mode, GLint first, GLsizei count);
void __MESA_SW_glDrawBuffer(GLenum mode);
void __MESA_SW_glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void __MESA_SW_glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
void __MESA_SW_glEdgeFlag(GLboolean flag);
void __MESA_SW_glEdgeFlagPointer(GLsizei stride, const GLvoid *pointer);
void __MESA_SW_glEdgeFlagv(const GLboolean *flag);
void __MESA_SW_glEnable(GLenum cap);
void __MESA_SW_glEnableClientState(GLenum array);
void __MESA_SW_glEnd(void);
void __MESA_SW_glEndList(void);
void __MESA_SW_glEvalCoord1d(GLdouble u);
void __MESA_SW_glEvalCoord1dv(const GLdouble *u);
void __MESA_SW_glEvalCoord1f(GLfloat u);
void __MESA_SW_glEvalCoord1fv(const GLfloat *u);
void __MESA_SW_glEvalCoord2d(GLdouble u, GLdouble v);
void __MESA_SW_glEvalCoord2dv(const GLdouble *u);
void __MESA_SW_glEvalCoord2f(GLfloat u, GLfloat v);
void __MESA_SW_glEvalCoord2fv(const GLfloat *u);
void __MESA_SW_glEvalMesh1(GLenum mode, GLint i1, GLint i2);
void __MESA_SW_glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
void __MESA_SW_glEvalPoint1(GLint i);
void __MESA_SW_glEvalPoint2(GLint i, GLint j);
void __MESA_SW_glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer);
void __MESA_SW_glFinish(void);
void __MESA_SW_glFlush(void);
void __MESA_SW_glFogf(GLenum pname, GLfloat param);
void __MESA_SW_glFogfv(GLenum pname, const GLfloat *params);
void __MESA_SW_glFogi(GLenum pname, GLint param);
void __MESA_SW_glFogiv(GLenum pname, const GLint *params);
void __MESA_SW_glFrontFace(GLenum mode);
void __MESA_SW_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLuint __MESA_SW_glGenLists(GLsizei range);
void __MESA_SW_glGenTextures(GLsizei n, GLuint *textures);
void __MESA_SW_glGetBooleanv(GLenum val, GLboolean *b);
void __MESA_SW_glGetClipPlane(GLenum plane, GLdouble *equation);
void __MESA_SW_glGetDoublev(GLenum val, GLdouble *d);
GLenum __MESA_SW_glGetError(void);
void __MESA_SW_glGetFloatv(GLenum val, GLfloat *f);
void __MESA_SW_glGetIntegerv(GLenum val, GLint *i);
void __MESA_SW_glGetLightfv(GLenum light, GLenum pname, GLfloat *params);
void __MESA_SW_glGetLightiv(GLenum light, GLenum pname, GLint *params);
void __MESA_SW_glGetMapdv(GLenum target, GLenum query, GLdouble *v);
void __MESA_SW_glGetMapfv(GLenum target, GLenum query, GLfloat *v);
void __MESA_SW_glGetMapiv(GLenum target, GLenum query, GLint *v);
void __MESA_SW_glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params);
void __MESA_SW_glGetMaterialiv(GLenum face, GLenum pname, GLint *params);
void __MESA_SW_glGetPixelMapfv(GLenum map, GLfloat *values);
void __MESA_SW_glGetPixelMapuiv(GLenum map, GLuint *values);
void __MESA_SW_glGetPixelMapusv(GLenum map, GLushort *values);
void __MESA_SW_glGetPointerv(GLenum pname, void **params);
void __MESA_SW_glGetPolygonStipple(GLubyte *mask);
const GLubyte *__MESA_SW_glGetString(GLenum name);
void __MESA_SW_glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params);
void __MESA_SW_glGetTexEnviv(GLenum target, GLenum pname, GLint *params);
void __MESA_SW_glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params);
void __MESA_SW_glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params);
void __MESA_SW_glGetTexGeniv(GLenum coord, GLenum pname, GLint *params);
void __MESA_SW_glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *texels);
void __MESA_SW_glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params);
void __MESA_SW_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params);
void __MESA_SW_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params);
void __MESA_SW_glGetTexParameteriv(GLenum target, GLenum pname, GLint *params);
void __MESA_SW_glHint(GLenum target, GLenum mode);
void __MESA_SW_glIndexMask(GLuint mask);
void __MESA_SW_glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void __MESA_SW_glIndexd(GLdouble c);
void __MESA_SW_glIndexdv(const GLdouble *c);
void __MESA_SW_glIndexf(GLfloat c);
void __MESA_SW_glIndexfv(const GLfloat *c);
void __MESA_SW_glIndexi(GLint c);
void __MESA_SW_glIndexiv(const GLint *c);
void __MESA_SW_glIndexs(GLshort c);
void __MESA_SW_glIndexsv(const GLshort *c);
void __MESA_SW_glIndexub(GLubyte c);
void __MESA_SW_glIndexubv(const GLubyte *c);
void __MESA_SW_glInitNames(void);
void __MESA_SW_glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer);
GLboolean __MESA_SW_glIsEnabled(GLenum cap);
GLboolean __MESA_SW_glIsList(GLuint list);
GLboolean __MESA_SW_glIsTexture(GLuint texture);
void __MESA_SW_glLightModelf(GLenum pname, GLfloat param);
void __MESA_SW_glLightModelfv(GLenum pname, const GLfloat *params);
void __MESA_SW_glLightModeli(GLenum pname, GLint param);
void __MESA_SW_glLightModeliv(GLenum pname, const GLint *params);
void __MESA_SW_glLightf(GLenum light, GLenum pname, GLfloat param);
void __MESA_SW_glLightfv(GLenum light, GLenum pname, const GLfloat *params);
void __MESA_SW_glLighti(GLenum light, GLenum pname, GLint param);
void __MESA_SW_glLightiv(GLenum light, GLenum pname, const GLint *params);
void __MESA_SW_glLineStipple(GLint factor, GLushort pattern);
void __MESA_SW_glLineWidth(GLfloat width);
void __MESA_SW_glListBase(GLuint base);
void __MESA_SW_glLoadIdentity(void);
void __MESA_SW_glLoadMatrixd(const GLdouble *m);
void __MESA_SW_glLoadMatrixf(const GLfloat *m);
void __MESA_SW_glLoadName(GLuint name);
void __MESA_SW_glLogicOp(GLenum opcode);
void __MESA_SW_glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *pnts);
void __MESA_SW_glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *pnts);
void __MESA_SW_glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustr, GLint uord, GLdouble v1, GLdouble v2, GLint vstr, GLint vord, const GLdouble *pnts);
void __MESA_SW_glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustr, GLint uord, GLfloat v1, GLfloat v2, GLint vstr, GLint vord, const GLfloat *pnts);
void __MESA_SW_glMapGrid1d(GLint un, GLdouble u1, GLdouble u2);
void __MESA_SW_glMapGrid1f(GLint un, GLfloat u1, GLfloat u2);
void __MESA_SW_glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
void __MESA_SW_glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
void __MESA_SW_glMaterialf(GLenum face, GLenum pname, GLfloat param);
void __MESA_SW_glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);
void __MESA_SW_glMateriali(GLenum face, GLenum pname, GLint param);
void __MESA_SW_glMaterialiv(GLenum face, GLenum pname, const GLint *params);
void __MESA_SW_glMatrixMode(GLenum mode);
void __MESA_SW_glMultMatrixd(const GLdouble *m);
void __MESA_SW_glMultMatrixf(const GLfloat *m);
void __MESA_SW_glNewList(GLuint list, GLenum mode);
void __MESA_SW_glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz);
void __MESA_SW_glNormal3bv(const GLbyte *v);
void __MESA_SW_glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz);
void __MESA_SW_glNormal3dv(const GLdouble *v);
void __MESA_SW_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void __MESA_SW_glNormal3fv(const GLfloat *v);
void __MESA_SW_glNormal3i(GLint nx, GLint ny, GLint nz);
void __MESA_SW_glNormal3iv(const GLint *v);
void __MESA_SW_glNormal3s(GLshort nx, GLshort ny, GLshort nz);
void __MESA_SW_glNormal3sv(const GLshort *v);
void __MESA_SW_glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void __MESA_SW_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void __MESA_SW_glPassThrough(GLfloat token);
void __MESA_SW_glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values);
void __MESA_SW_glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values);
void __MESA_SW_glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values);
void __MESA_SW_glPixelStoref(GLenum pname, GLfloat param);
void __MESA_SW_glPixelStorei(GLenum pname, GLint param);
void __MESA_SW_glPixelTransferf(GLenum pname, GLfloat param);
void __MESA_SW_glPixelTransferi(GLenum pname, GLint param);
void __MESA_SW_glPixelZoom(GLfloat xfactor, GLfloat yfactor);
void __MESA_SW_glPointSize(GLfloat size);
void __MESA_SW_glPolygonMode(GLenum face, GLenum mode);
void __MESA_SW_glPolygonOffset(GLfloat factor, GLfloat units);
void __MESA_SW_glPolygonStipple(const GLubyte *mask);
void __MESA_SW_glPopAttrib(void);
void __MESA_SW_glPopClientAttrib(void);
void __MESA_SW_glPopMatrix(void);
void __MESA_SW_glPopName(void);
void __MESA_SW_glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities);
void __MESA_SW_glPushAttrib(GLbitfield mask);
void __MESA_SW_glPushClientAttrib(GLuint mask);
void __MESA_SW_glPushMatrix(void);
void __MESA_SW_glPushName(GLuint name);
void __MESA_SW_glRasterPos2d(GLdouble x, GLdouble y);
void __MESA_SW_glRasterPos2dv(const GLdouble *v);
void __MESA_SW_glRasterPos2f(GLfloat x, GLfloat y);
void __MESA_SW_glRasterPos2fv(const GLfloat *v);
void __MESA_SW_glRasterPos2i(GLint x, GLint y);
void __MESA_SW_glRasterPos2iv(const GLint *v);
void __MESA_SW_glRasterPos2s(GLshort x, GLshort y);
void __MESA_SW_glRasterPos2sv(const GLshort *v);
void __MESA_SW_glRasterPos3d(GLdouble x, GLdouble y, GLdouble z);
void __MESA_SW_glRasterPos3dv(const GLdouble *v);
void __MESA_SW_glRasterPos3f(GLfloat x, GLfloat y, GLfloat z);
void __MESA_SW_glRasterPos3fv(const GLfloat *v);
void __MESA_SW_glRasterPos3i(GLint x, GLint y, GLint z);
void __MESA_SW_glRasterPos3iv(const GLint *v);
void __MESA_SW_glRasterPos3s(GLshort x, GLshort y, GLshort z);
void __MESA_SW_glRasterPos3sv(const GLshort *v);
void __MESA_SW_glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void __MESA_SW_glRasterPos4dv(const GLdouble *v);
void __MESA_SW_glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void __MESA_SW_glRasterPos4fv(const GLfloat *v);
void __MESA_SW_glRasterPos4i(GLint x, GLint y, GLint z, GLint w);
void __MESA_SW_glRasterPos4iv(const GLint *v);
void __MESA_SW_glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w);
void __MESA_SW_glRasterPos4sv(const GLshort *v);
void __MESA_SW_glReadBuffer(GLenum mode);
void __MESA_SW_glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void __MESA_SW_glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void __MESA_SW_glRectdv(const GLdouble *v1, const GLdouble *v2);
void __MESA_SW_glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void __MESA_SW_glRectfv(const GLfloat *v1, const GLfloat *v2);
void __MESA_SW_glRecti(GLint x1, GLint y1, GLint x2, GLint y2);
void __MESA_SW_glRectiv(const GLint *v1, const GLint *v2);
void __MESA_SW_glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void __MESA_SW_glRectsv(const GLshort *v1, const GLshort *v2);
GLint __MESA_SW_glRenderMode(GLenum mode);
void __MESA_SW_glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void __MESA_SW_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void __MESA_SW_glScaled(GLdouble x, GLdouble y, GLdouble z);
void __MESA_SW_glScalef(GLfloat x, GLfloat y, GLfloat z);
void __MESA_SW_glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void __MESA_SW_glSelectBuffer(GLsizei numnames, GLuint *buffer);
void __MESA_SW_glShadeModel(GLenum mode);
void __MESA_SW_glStencilFunc(GLenum func, GLint ref, GLuint mask);
void __MESA_SW_glStencilMask(GLuint mask);
void __MESA_SW_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
void __MESA_SW_glTexCoord1d(GLdouble s);
void __MESA_SW_glTexCoord1dv(const GLdouble *v);
void __MESA_SW_glTexCoord1f(GLfloat s);
void __MESA_SW_glTexCoord1fv(const GLfloat *v);
void __MESA_SW_glTexCoord1i(GLint s);
void __MESA_SW_glTexCoord1iv(const GLint *v);
void __MESA_SW_glTexCoord1s(GLshort s);
void __MESA_SW_glTexCoord1sv(const GLshort *v);
void __MESA_SW_glTexCoord2d(GLdouble s, GLdouble t);
void __MESA_SW_glTexCoord2dv(const GLdouble *v);
void __MESA_SW_glTexCoord2f(GLfloat s, GLfloat t);
void __MESA_SW_glTexCoord2fv(const GLfloat *v);
void __MESA_SW_glTexCoord2i(GLint s, GLint t);
void __MESA_SW_glTexCoord2iv(const GLint *v);
void __MESA_SW_glTexCoord2s(GLshort s, GLshort t);
void __MESA_SW_glTexCoord2sv(const GLshort *v);
void __MESA_SW_glTexCoord3d(GLdouble s, GLdouble t, GLdouble r);
void __MESA_SW_glTexCoord3dv(const GLdouble *v);
void __MESA_SW_glTexCoord3f(GLfloat s, GLfloat t, GLfloat r);
void __MESA_SW_glTexCoord3fv(const GLfloat *v);
void __MESA_SW_glTexCoord3i(GLint s, GLint t, GLint r);
void __MESA_SW_glTexCoord3iv(const GLint *v);
void __MESA_SW_glTexCoord3s(GLshort s, GLshort t, GLshort r);
void __MESA_SW_glTexCoord3sv(const GLshort *v);
void __MESA_SW_glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void __MESA_SW_glTexCoord4dv(const GLdouble *v);
void __MESA_SW_glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void __MESA_SW_glTexCoord4fv(const GLfloat *v);
void __MESA_SW_glTexCoord4i(GLint s, GLint t, GLint r, GLint q);
void __MESA_SW_glTexCoord4iv(const GLint *v);
void __MESA_SW_glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q);
void __MESA_SW_glTexCoord4sv(const GLshort *v);
void __MESA_SW_glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void __MESA_SW_glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void __MESA_SW_glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
void __MESA_SW_glTexEnvi(GLenum target, GLenum pname, GLint param);
void __MESA_SW_glTexEnviv(GLenum target, GLenum pname, const GLint *params);
void __MESA_SW_glTexGend(GLenum coord, GLenum pname, GLdouble param);
void __MESA_SW_glTexGendv(GLenum coord, GLenum pname, const GLdouble *params);
void __MESA_SW_glTexGenf(GLenum coord, GLenum pname, GLfloat param);
void __MESA_SW_glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params);
void __MESA_SW_glTexGeni(GLenum coord, GLenum pname, GLint param);
void __MESA_SW_glTexGeniv(GLenum coord, GLenum pname, const GLint *params);
void __MESA_SW_glTexImage1D(GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *image);
void __MESA_SW_glTexImage2D(GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *image);
void __MESA_SW_glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void __MESA_SW_glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params);
void __MESA_SW_glTexParameteri(GLenum target, GLenum pname, GLint param);
void __MESA_SW_glTexParameteriv(GLenum target, GLenum pname, const GLint *params);
void __MESA_SW_glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *image);
void __MESA_SW_glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
void __MESA_SW_glTranslated(GLdouble x, GLdouble y, GLdouble z);
void __MESA_SW_glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void __MESA_SW_glVertex2d(GLdouble x, GLdouble y);
void __MESA_SW_glVertex2dv(const GLdouble *v);
void __MESA_SW_glVertex2f(GLfloat x, GLfloat y);
void __MESA_SW_glVertex2fv(const GLfloat *v);
void __MESA_SW_glVertex2i(GLint x, GLint y);
void __MESA_SW_glVertex2iv(const GLint *v);
void __MESA_SW_glVertex2s(GLshort x, GLshort y);
void __MESA_SW_glVertex2sv(const GLshort *v);
void __MESA_SW_glVertex3d(GLdouble x, GLdouble y, GLdouble z);
void __MESA_SW_glVertex3dv(const GLdouble *v);
void __MESA_SW_glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void __MESA_SW_glVertex3fv(const GLfloat *v);
void __MESA_SW_glVertex3i(GLint x, GLint y, GLint z);
void __MESA_SW_glVertex3iv(const GLint *v);
void __MESA_SW_glVertex3s(GLshort x, GLshort y, GLshort z);
void __MESA_SW_glVertex3sv(const GLshort *v);
void __MESA_SW_glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void __MESA_SW_glVertex4dv(const GLdouble *v);
void __MESA_SW_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void __MESA_SW_glVertex4fv(const GLfloat *v);
void __MESA_SW_glVertex4i(GLint x, GLint y, GLint z, GLint w);
void __MESA_SW_glVertex4iv(const GLint *v);
void __MESA_SW_glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w);
void __MESA_SW_glVertex4sv(const GLshort *v);
void __MESA_SW_glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void __MESA_SW_glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
#endif

#endif /* _MESA_API_H_ */
