/* $XFree86: xc/lib/GL/mesa/src/mesa_api.h,v 1.1 1999/06/14 07:23:44 dawes Exp $ */
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
 * $PI: xc/lib/GL/mesa/src/mesa_api.h,v 1.3 1999/06/14 21:10:40 faith Exp $
 */

#ifndef _MESA_API_H_
#define _MESA_API_H_

#ifdef GLX_DIRECT_RENDERING

#include <GL/gl.h>
#include "mesa_api_wrap.h"

/* NOTE: This file could be automatically generated */

void __glAccum(GLenum op, GLfloat value);
void __glAlphaFunc(GLenum func, GLclampf ref);
GLboolean __glAreTexturesResident(GLsizei n, const GLuint *textures, GLboolean *residences);
void __glArrayElement(GLint i);
void __glBegin(GLenum mode);
void __glBindTexture(GLenum target, GLuint texture);
void __glBitmap(GLsizei width, GLsizei height, GLfloat xorig, GLfloat yorig, GLfloat xmove, GLfloat ymove, const GLubyte *bitmap);
void __glBlendFunc(GLenum sfactor, GLenum dfactor);
void __glCallList(GLuint list);
void __glCallLists(GLsizei n, GLenum type, const GLvoid *lists);
void __glClear(GLbitfield mask);
void __glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void __glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void __glClearDepth(GLclampd depth);
void __glClearIndex(GLfloat c);
void __glClearStencil(GLint s);
void __glClipPlane(GLenum plane, const GLdouble *equation);
void __glColor3b(GLbyte red, GLbyte green, GLbyte blue);
void __glColor3bv(const GLbyte *v);
void __glColor3d(GLdouble red, GLdouble green, GLdouble blue);
void __glColor3dv(const GLdouble *v);
void __glColor3f(GLfloat red, GLfloat green, GLfloat blue);
void __glColor3fv(const GLfloat *v);
void __glColor3i(GLint red, GLint green, GLint blue);
void __glColor3iv(const GLint *v);
void __glColor3s(GLshort red, GLshort green, GLshort blue);
void __glColor3sv(const GLshort *v);
void __glColor3ub(GLubyte red, GLubyte green, GLubyte blue);
void __glColor3ubv(const GLubyte *v);
void __glColor3ui(GLuint red, GLuint green, GLuint blue);
void __glColor3uiv(const GLuint *v);
void __glColor3us(GLushort red, GLushort green, GLushort blue);
void __glColor3usv(const GLushort *v);
void __glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha);
void __glColor4bv(const GLbyte *v);
void __glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha);
void __glColor4dv(const GLdouble *v);
void __glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
void __glColor4fv(const GLfloat *v);
void __glColor4i(GLint red, GLint green, GLint blue, GLint alpha);
void __glColor4iv(const GLint *v);
void __glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha);
void __glColor4sv(const GLshort *v);
void __glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha);
void __glColor4ubv(const GLubyte *v);
void __glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha);
void __glColor4uiv(const GLuint *v);
void __glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha);
void __glColor4usv(const GLushort *v);
void __glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
void __glColorMaterial(GLenum face, GLenum mode);
void __glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void __glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type);
void __glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
void __glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
void __glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
void __glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
void __glCullFace(GLenum mode);
void __glDeleteLists(GLuint list, GLsizei range);
void __glDeleteTextures(GLsizei n, const GLuint *textures);
void __glDepthFunc(GLenum func);
void __glDepthMask(GLboolean flag);
void __glDepthRange(GLclampd zNear, GLclampd zFar);
void __glDisable(GLenum cap);
void __glDisableClientState(GLenum array);
void __glDrawArrays(GLenum mode, GLint first, GLsizei count);
void __glDrawBuffer(GLenum mode);
void __glDrawElements(GLenum mode, GLsizei count, GLenum type, const GLvoid *indices);
void __glDrawPixels(GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
void __glEdgeFlag(GLboolean flag);
void __glEdgeFlagPointer(GLsizei stride, const GLvoid *pointer);
void __glEdgeFlagv(const GLboolean *flag);
void __glEnable(GLenum cap);
void __glEnableClientState(GLenum array);
void __glEnd(void);
void __glEndList(void);
void __glEvalCoord1d(GLdouble u);
void __glEvalCoord1dv(const GLdouble *u);
void __glEvalCoord1f(GLfloat u);
void __glEvalCoord1fv(const GLfloat *u);
void __glEvalCoord2d(GLdouble u, GLdouble v);
void __glEvalCoord2dv(const GLdouble *u);
void __glEvalCoord2f(GLfloat u, GLfloat v);
void __glEvalCoord2fv(const GLfloat *u);
void __glEvalMesh1(GLenum mode, GLint i1, GLint i2);
void __glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2);
void __glEvalPoint1(GLint i);
void __glEvalPoint2(GLint i, GLint j);
void __glFeedbackBuffer(GLsizei size, GLenum type, GLfloat *buffer);
void __glFinish(void);
void __glFlush(void);
void __glFogf(GLenum pname, GLfloat param);
void __glFogfv(GLenum pname, const GLfloat *params);
void __glFogi(GLenum pname, GLint param);
void __glFogiv(GLenum pname, const GLint *params);
void __glFrontFace(GLenum mode);
void __glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
GLuint __glGenLists(GLsizei range);
void __glGenTextures(GLsizei n, GLuint *textures);
void __glGetBooleanv(GLenum val, GLboolean *b);
void __glGetClipPlane(GLenum plane, GLdouble *equation);
void __glGetDoublev(GLenum val, GLdouble *d);
GLenum __glGetError(void);
void __glGetFloatv(GLenum val, GLfloat *f);
void __glGetIntegerv(GLenum val, GLint *i);
void __glGetLightfv(GLenum light, GLenum pname, GLfloat *params);
void __glGetLightiv(GLenum light, GLenum pname, GLint *params);
void __glGetMapdv(GLenum target, GLenum query, GLdouble *v);
void __glGetMapfv(GLenum target, GLenum query, GLfloat *v);
void __glGetMapiv(GLenum target, GLenum query, GLint *v);
void __glGetMaterialfv(GLenum face, GLenum pname, GLfloat *params);
void __glGetMaterialiv(GLenum face, GLenum pname, GLint *params);
void __glGetPixelMapfv(GLenum map, GLfloat *values);
void __glGetPixelMapuiv(GLenum map, GLuint *values);
void __glGetPixelMapusv(GLenum map, GLushort *values);
void __glGetPointerv(GLenum pname, void **params);
void __glGetPolygonStipple(GLubyte *mask);
const GLubyte *__glGetString(GLenum name);
void __glGetTexEnvfv(GLenum target, GLenum pname, GLfloat *params);
void __glGetTexEnviv(GLenum target, GLenum pname, GLint *params);
void __glGetTexGendv(GLenum coord, GLenum pname, GLdouble *params);
void __glGetTexGenfv(GLenum coord, GLenum pname, GLfloat *params);
void __glGetTexGeniv(GLenum coord, GLenum pname, GLint *params);
void __glGetTexImage(GLenum target, GLint level, GLenum format, GLenum type, GLvoid *texels);
void __glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat *params);
void __glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint *params);
void __glGetTexParameterfv(GLenum target, GLenum pname, GLfloat *params);
void __glGetTexParameteriv(GLenum target, GLenum pname, GLint *params);
void __glHint(GLenum target, GLenum mode);
void __glIndexMask(GLuint mask);
void __glIndexPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void __glIndexd(GLdouble c);
void __glIndexdv(const GLdouble *c);
void __glIndexf(GLfloat c);
void __glIndexfv(const GLfloat *c);
void __glIndexi(GLint c);
void __glIndexiv(const GLint *c);
void __glIndexs(GLshort c);
void __glIndexsv(const GLshort *c);
void __glIndexub(GLubyte c);
void __glIndexubv(const GLubyte *c);
void __glInitNames(void);
void __glInterleavedArrays(GLenum format, GLsizei stride, const GLvoid *pointer);
GLboolean __glIsEnabled(GLenum cap);
GLboolean __glIsList(GLuint list);
GLboolean __glIsTexture(GLuint texture);
void __glLightModelf(GLenum pname, GLfloat param);
void __glLightModelfv(GLenum pname, const GLfloat *params);
void __glLightModeli(GLenum pname, GLint param);
void __glLightModeliv(GLenum pname, const GLint *params);
void __glLightf(GLenum light, GLenum pname, GLfloat param);
void __glLightfv(GLenum light, GLenum pname, const GLfloat *params);
void __glLighti(GLenum light, GLenum pname, GLint param);
void __glLightiv(GLenum light, GLenum pname, const GLint *params);
void __glLineStipple(GLint factor, GLushort pattern);
void __glLineWidth(GLfloat width);
void __glListBase(GLuint base);
void __glLoadIdentity(void);
void __glLoadMatrixd(const GLdouble *m);
void __glLoadMatrixf(const GLfloat *m);
void __glLoadName(GLuint name);
void __glLogicOp(GLenum opcode);
void __glMap1d(GLenum target, GLdouble u1, GLdouble u2, GLint stride, GLint order, const GLdouble *pnts);
void __glMap1f(GLenum target, GLfloat u1, GLfloat u2, GLint stride, GLint order, const GLfloat *pnts);
void __glMap2d(GLenum target, GLdouble u1, GLdouble u2, GLint ustr, GLint uord, GLdouble v1, GLdouble v2, GLint vstr, GLint vord, const GLdouble *pnts);
void __glMap2f(GLenum target, GLfloat u1, GLfloat u2, GLint ustr, GLint uord, GLfloat v1, GLfloat v2, GLint vstr, GLint vord, const GLfloat *pnts);
void __glMapGrid1d(GLint un, GLdouble u1, GLdouble u2);
void __glMapGrid1f(GLint un, GLfloat u1, GLfloat u2);
void __glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2);
void __glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2);
void __glMaterialf(GLenum face, GLenum pname, GLfloat param);
void __glMaterialfv(GLenum face, GLenum pname, const GLfloat *params);
void __glMateriali(GLenum face, GLenum pname, GLint param);
void __glMaterialiv(GLenum face, GLenum pname, const GLint *params);
void __glMatrixMode(GLenum mode);
void __glMultMatrixd(const GLdouble *m);
void __glMultMatrixf(const GLfloat *m);
void __glNewList(GLuint list, GLenum mode);
void __glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz);
void __glNormal3bv(const GLbyte *v);
void __glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz);
void __glNormal3dv(const GLdouble *v);
void __glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz);
void __glNormal3fv(const GLfloat *v);
void __glNormal3i(GLint nx, GLint ny, GLint nz);
void __glNormal3iv(const GLint *v);
void __glNormal3s(GLshort nx, GLshort ny, GLshort nz);
void __glNormal3sv(const GLshort *v);
void __glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer);
void __glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar);
void __glPassThrough(GLfloat token);
void __glPixelMapfv(GLenum map, GLint mapsize, const GLfloat *values);
void __glPixelMapuiv(GLenum map, GLint mapsize, const GLuint *values);
void __glPixelMapusv(GLenum map, GLint mapsize, const GLushort *values);
void __glPixelStoref(GLenum pname, GLfloat param);
void __glPixelStorei(GLenum pname, GLint param);
void __glPixelTransferf(GLenum pname, GLfloat param);
void __glPixelTransferi(GLenum pname, GLint param);
void __glPixelZoom(GLfloat xfactor, GLfloat yfactor);
void __glPointSize(GLfloat size);
void __glPolygonMode(GLenum face, GLenum mode);
void __glPolygonOffset(GLfloat factor, GLfloat units);
void __glPolygonStipple(const GLubyte *mask);
void __glPopAttrib(void);
void __glPopClientAttrib(void);
void __glPopMatrix(void);
void __glPopName(void);
void __glPrioritizeTextures(GLsizei n, const GLuint *textures, const GLclampf *priorities);
void __glPushAttrib(GLbitfield mask);
void __glPushClientAttrib(GLuint mask);
void __glPushMatrix(void);
void __glPushName(GLuint name);
void __glRasterPos2d(GLdouble x, GLdouble y);
void __glRasterPos2dv(const GLdouble *v);
void __glRasterPos2f(GLfloat x, GLfloat y);
void __glRasterPos2fv(const GLfloat *v);
void __glRasterPos2i(GLint x, GLint y);
void __glRasterPos2iv(const GLint *v);
void __glRasterPos2s(GLshort x, GLshort y);
void __glRasterPos2sv(const GLshort *v);
void __glRasterPos3d(GLdouble x, GLdouble y, GLdouble z);
void __glRasterPos3dv(const GLdouble *v);
void __glRasterPos3f(GLfloat x, GLfloat y, GLfloat z);
void __glRasterPos3fv(const GLfloat *v);
void __glRasterPos3i(GLint x, GLint y, GLint z);
void __glRasterPos3iv(const GLint *v);
void __glRasterPos3s(GLshort x, GLshort y, GLshort z);
void __glRasterPos3sv(const GLshort *v);
void __glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void __glRasterPos4dv(const GLdouble *v);
void __glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void __glRasterPos4fv(const GLfloat *v);
void __glRasterPos4i(GLint x, GLint y, GLint z, GLint w);
void __glRasterPos4iv(const GLint *v);
void __glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w);
void __glRasterPos4sv(const GLshort *v);
void __glReadBuffer(GLenum mode);
void __glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid *pixels);
void __glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2);
void __glRectdv(const GLdouble *v1, const GLdouble *v2);
void __glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2);
void __glRectfv(const GLfloat *v1, const GLfloat *v2);
void __glRecti(GLint x1, GLint y1, GLint x2, GLint y2);
void __glRectiv(const GLint *v1, const GLint *v2);
void __glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2);
void __glRectsv(const GLshort *v1, const GLshort *v2);
GLint __glRenderMode(GLenum mode);
void __glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z);
void __glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z);
void __glScaled(GLdouble x, GLdouble y, GLdouble z);
void __glScalef(GLfloat x, GLfloat y, GLfloat z);
void __glScissor(GLint x, GLint y, GLsizei width, GLsizei height);
void __glSelectBuffer(GLsizei numnames, GLuint *buffer);
void __glShadeModel(GLenum mode);
void __glStencilFunc(GLenum func, GLint ref, GLuint mask);
void __glStencilMask(GLuint mask);
void __glStencilOp(GLenum fail, GLenum zfail, GLenum zpass);
void __glTexCoord1d(GLdouble s);
void __glTexCoord1dv(const GLdouble *v);
void __glTexCoord1f(GLfloat s);
void __glTexCoord1fv(const GLfloat *v);
void __glTexCoord1i(GLint s);
void __glTexCoord1iv(const GLint *v);
void __glTexCoord1s(GLshort s);
void __glTexCoord1sv(const GLshort *v);
void __glTexCoord2d(GLdouble s, GLdouble t);
void __glTexCoord2dv(const GLdouble *v);
void __glTexCoord2f(GLfloat s, GLfloat t);
void __glTexCoord2fv(const GLfloat *v);
void __glTexCoord2i(GLint s, GLint t);
void __glTexCoord2iv(const GLint *v);
void __glTexCoord2s(GLshort s, GLshort t);
void __glTexCoord2sv(const GLshort *v);
void __glTexCoord3d(GLdouble s, GLdouble t, GLdouble r);
void __glTexCoord3dv(const GLdouble *v);
void __glTexCoord3f(GLfloat s, GLfloat t, GLfloat r);
void __glTexCoord3fv(const GLfloat *v);
void __glTexCoord3i(GLint s, GLint t, GLint r);
void __glTexCoord3iv(const GLint *v);
void __glTexCoord3s(GLshort s, GLshort t, GLshort r);
void __glTexCoord3sv(const GLshort *v);
void __glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q);
void __glTexCoord4dv(const GLdouble *v);
void __glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q);
void __glTexCoord4fv(const GLfloat *v);
void __glTexCoord4i(GLint s, GLint t, GLint r, GLint q);
void __glTexCoord4iv(const GLint *v);
void __glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q);
void __glTexCoord4sv(const GLshort *v);
void __glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void __glTexEnvf(GLenum target, GLenum pname, GLfloat param);
void __glTexEnvfv(GLenum target, GLenum pname, const GLfloat *params);
void __glTexEnvi(GLenum target, GLenum pname, GLint param);
void __glTexEnviv(GLenum target, GLenum pname, const GLint *params);
void __glTexGend(GLenum coord, GLenum pname, GLdouble param);
void __glTexGendv(GLenum coord, GLenum pname, const GLdouble *params);
void __glTexGenf(GLenum coord, GLenum pname, GLfloat param);
void __glTexGenfv(GLenum coord, GLenum pname, const GLfloat *params);
void __glTexGeni(GLenum coord, GLenum pname, GLint param);
void __glTexGeniv(GLenum coord, GLenum pname, const GLint *params);
void __glTexImage1D(GLenum target, GLint level, GLint components, GLsizei width, GLint border, GLenum format, GLenum type, const GLvoid *image);
void __glTexImage2D(GLenum target, GLint level, GLint components, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const GLvoid *image);
void __glTexParameterf(GLenum target, GLenum pname, GLfloat param);
void __glTexParameterfv(GLenum target, GLenum pname, const GLfloat *params);
void __glTexParameteri(GLenum target, GLenum pname, GLint param);
void __glTexParameteriv(GLenum target, GLenum pname, const GLint *params);
void __glTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *image);
void __glTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
void __glTranslated(GLdouble x, GLdouble y, GLdouble z);
void __glTranslatef(GLfloat x, GLfloat y, GLfloat z);
void __glVertex2d(GLdouble x, GLdouble y);
void __glVertex2dv(const GLdouble *v);
void __glVertex2f(GLfloat x, GLfloat y);
void __glVertex2fv(const GLfloat *v);
void __glVertex2i(GLint x, GLint y);
void __glVertex2iv(const GLint *v);
void __glVertex2s(GLshort x, GLshort y);
void __glVertex2sv(const GLshort *v);
void __glVertex3d(GLdouble x, GLdouble y, GLdouble z);
void __glVertex3dv(const GLdouble *v);
void __glVertex3f(GLfloat x, GLfloat y, GLfloat z);
void __glVertex3fv(const GLfloat *v);
void __glVertex3i(GLint x, GLint y, GLint z);
void __glVertex3iv(const GLint *v);
void __glVertex3s(GLshort x, GLshort y, GLshort z);
void __glVertex3sv(const GLshort *v);
void __glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w);
void __glVertex4dv(const GLdouble *v);
void __glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w);
void __glVertex4fv(const GLfloat *v);
void __glVertex4i(GLint x, GLint y, GLint z, GLint w);
void __glVertex4iv(const GLint *v);
void __glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w);
void __glVertex4sv(const GLshort *v);
void __glVertexPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
void __glViewport(GLint x, GLint y, GLsizei width, GLsizei height);
#endif

#endif /* _MESA_API_H_ */
