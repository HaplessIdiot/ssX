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
 * $PI: xc/lib/GL/mesa/src/drv/gamma/gamma_gl_wrap.h,v 1.1 1999/04/05 05:24:35 martin Exp $
 */

#ifndef _GAMMA_GL_WRAP_H_
#define _GAMMA_GL_WRAP_H_

/*
** This entire file should be removed after we implement dynamic loading
** in the DRI.
*/

#if defined(GLX_DIRECT_RENDERING) && defined(NEED_GAMMA_FUNCS_WRAPPED)

/* NOTE: This file could be automatically generated */

#define glAccum __GAMMA_glAccum
#define glAlphaFunc __GAMMA_glAlphaFunc
#define glAreTexturesResident __GAMMA_glAreTexturesResident
#define glArrayElement __GAMMA_glArrayElement
#define glBegin __GAMMA_glBegin
#define glBindTexture __GAMMA_glBindTexture
#define glBitmap __GAMMA_glBitmap
#define glBlendFunc __GAMMA_glBlendFunc
#define glCallList __GAMMA_glCallList
#define glCallLists __GAMMA_glCallLists
#define glClear __GAMMA_glClear
#define glClearAccum __GAMMA_glClearAccum
#define glClearColor __GAMMA_glClearColor
#define glClearDepth __GAMMA_glClearDepth
#define glClearIndex __GAMMA_glClearIndex
#define glClearStencil __GAMMA_glClearStencil
#define glClipPlane __GAMMA_glClipPlane
#define glColor3b __GAMMA_glColor3b
#define glColor3bv __GAMMA_glColor3bv
#define glColor3d __GAMMA_glColor3d
#define glColor3dv __GAMMA_glColor3dv
#define glColor3f __GAMMA_glColor3f
#define glColor3fv __GAMMA_glColor3fv
#define glColor3i __GAMMA_glColor3i
#define glColor3iv __GAMMA_glColor3iv
#define glColor3s __GAMMA_glColor3s
#define glColor3sv __GAMMA_glColor3sv
#define glColor3ub __GAMMA_glColor3ub
#define glColor3ubv __GAMMA_glColor3ubv
#define glColor3ui __GAMMA_glColor3ui
#define glColor3uiv __GAMMA_glColor3uiv
#define glColor3us __GAMMA_glColor3us
#define glColor3usv __GAMMA_glColor3usv
#define glColor4b __GAMMA_glColor4b
#define glColor4bv __GAMMA_glColor4bv
#define glColor4d __GAMMA_glColor4d
#define glColor4dv __GAMMA_glColor4dv
#define glColor4f __GAMMA_glColor4f
#define glColor4fv __GAMMA_glColor4fv
#define glColor4i __GAMMA_glColor4i
#define glColor4iv __GAMMA_glColor4iv
#define glColor4s __GAMMA_glColor4s
#define glColor4sv __GAMMA_glColor4sv
#define glColor4ub __GAMMA_glColor4ub
#define glColor4ubv __GAMMA_glColor4ubv
#define glColor4ui __GAMMA_glColor4ui
#define glColor4uiv __GAMMA_glColor4uiv
#define glColor4us __GAMMA_glColor4us
#define glColor4usv __GAMMA_glColor4usv
#define glColorMask __GAMMA_glColorMask
#define glColorMaterial __GAMMA_glColorMaterial
#define glColorPointer __GAMMA_glColorPointer
#define glCopyPixels __GAMMA_glCopyPixels
#define glCopyTexImage1D __GAMMA_glCopyTexImage1D
#define glCopyTexImage2D __GAMMA_glCopyTexImage2D
#define glCopyTexSubImage1D __GAMMA_glCopyTexSubImage1D
#define glCopyTexSubImage2D __GAMMA_glCopyTexSubImage2D
#define glCullFace __GAMMA_glCullFace
#define glDeleteLists __GAMMA_glDeleteLists
#define glDeleteTextures __GAMMA_glDeleteTextures
#define glDepthFunc __GAMMA_glDepthFunc
#define glDepthMask __GAMMA_glDepthMask
#define glDepthRange __GAMMA_glDepthRange
#define glDisable __GAMMA_glDisable
#define glDisableClientState __GAMMA_glDisableClientState
#define glDrawArrays __GAMMA_glDrawArrays
#define glDrawBuffer __GAMMA_glDrawBuffer
#define glDrawElements __GAMMA_glDrawElements
#define glDrawPixels __GAMMA_glDrawPixels
#define glEdgeFlag __GAMMA_glEdgeFlag
#define glEdgeFlagPointer __GAMMA_glEdgeFlagPointer
#define glEdgeFlagv __GAMMA_glEdgeFlagv
#define glEnable __GAMMA_glEnable
#define glEnableClientState __GAMMA_glEnableClientState
#define glEnd __GAMMA_glEnd
#define glEndList __GAMMA_glEndList
#define glEvalCoord1d __GAMMA_glEvalCoord1d
#define glEvalCoord1dv __GAMMA_glEvalCoord1dv
#define glEvalCoord1f __GAMMA_glEvalCoord1f
#define glEvalCoord1fv __GAMMA_glEvalCoord1fv
#define glEvalCoord2d __GAMMA_glEvalCoord2d
#define glEvalCoord2dv __GAMMA_glEvalCoord2dv
#define glEvalCoord2f __GAMMA_glEvalCoord2f
#define glEvalCoord2fv __GAMMA_glEvalCoord2fv
#define glEvalMesh1 __GAMMA_glEvalMesh1
#define glEvalMesh2 __GAMMA_glEvalMesh2
#define glEvalPoint1 __GAMMA_glEvalPoint1
#define glEvalPoint2 __GAMMA_glEvalPoint2
#define glFeedbackBuffer __GAMMA_glFeedbackBuffer
#define glFinish __GAMMA_glFinish
#define glFlush __GAMMA_glFlush
#define glFogf __GAMMA_glFogf
#define glFogfv __GAMMA_glFogfv
#define glFogi __GAMMA_glFogi
#define glFogiv __GAMMA_glFogiv
#define glFrontFace __GAMMA_glFrontFace
#define glFrustum __GAMMA_glFrustum
#define glGenLists __GAMMA_glGenLists
#define glGenTextures __GAMMA_glGenTextures
#define glGetBooleanv __GAMMA_glGetBooleanv
#define glGetClipPlane __GAMMA_glGetClipPlane
#define glGetDoublev __GAMMA_glGetDoublev
#define glGetError __GAMMA_glGetError
#define glGetFloatv __GAMMA_glGetFloatv
#define glGetIntegerv __GAMMA_glGetIntegerv
#define glGetLightfv __GAMMA_glGetLightfv
#define glGetLightiv __GAMMA_glGetLightiv
#define glGetMapdv __GAMMA_glGetMapdv
#define glGetMapfv __GAMMA_glGetMapfv
#define glGetMapiv __GAMMA_glGetMapiv
#define glGetMaterialfv __GAMMA_glGetMaterialfv
#define glGetMaterialiv __GAMMA_glGetMaterialiv
#define glGetPixelMapfv __GAMMA_glGetPixelMapfv
#define glGetPixelMapuiv __GAMMA_glGetPixelMapuiv
#define glGetPixelMapusv __GAMMA_glGetPixelMapusv
#define glGetPointerv __GAMMA_glGetPointerv
#define glGetPolygonStipple __GAMMA_glGetPolygonStipple
#define glGetString __GAMMA_glGetString
#define glGetTexEnvfv __GAMMA_glGetTexEnvfv
#define glGetTexEnviv __GAMMA_glGetTexEnviv
#define glGetTexGendv __GAMMA_glGetTexGendv
#define glGetTexGenfv __GAMMA_glGetTexGenfv
#define glGetTexGeniv __GAMMA_glGetTexGeniv
#define glGetTexImage __GAMMA_glGetTexImage
#define glGetTexLevelParameterfv __GAMMA_glGetTexLevelParameterfv
#define glGetTexLevelParameteriv __GAMMA_glGetTexLevelParameteriv
#define glGetTexParameterfv __GAMMA_glGetTexParameterfv
#define glGetTexParameteriv __GAMMA_glGetTexParameteriv
#define glHint __GAMMA_glHint
#define glIndexMask __GAMMA_glIndexMask
#define glIndexPointer __GAMMA_glIndexPointer
#define glIndexd __GAMMA_glIndexd
#define glIndexdv __GAMMA_glIndexdv
#define glIndexf __GAMMA_glIndexf
#define glIndexfv __GAMMA_glIndexfv
#define glIndexi __GAMMA_glIndexi
#define glIndexiv __GAMMA_glIndexiv
#define glIndexs __GAMMA_glIndexs
#define glIndexsv __GAMMA_glIndexsv
#define glIndexub __GAMMA_glIndexub
#define glIndexubv __GAMMA_glIndexubv
#define glInitNames __GAMMA_glInitNames
#define glInterleavedArrays __GAMMA_glInterleavedArrays
#define glIsEnabled __GAMMA_glIsEnabled
#define glIsList __GAMMA_glIsList
#define glIsTexture __GAMMA_glIsTexture
#define glLightModelf __GAMMA_glLightModelf
#define glLightModelfv __GAMMA_glLightModelfv
#define glLightModeli __GAMMA_glLightModeli
#define glLightModeliv __GAMMA_glLightModeliv
#define glLightf __GAMMA_glLightf
#define glLightfv __GAMMA_glLightfv
#define glLighti __GAMMA_glLighti
#define glLightiv __GAMMA_glLightiv
#define glLineStipple __GAMMA_glLineStipple
#define glLineWidth __GAMMA_glLineWidth
#define glListBase __GAMMA_glListBase
#define glLoadIdentity __GAMMA_glLoadIdentity
#define glLoadMatrixd __GAMMA_glLoadMatrixd
#define glLoadMatrixf __GAMMA_glLoadMatrixf
#define glLoadName __GAMMA_glLoadName
#define glLogicOp __GAMMA_glLogicOp
#define glMap1d __GAMMA_glMap1d
#define glMap1f __GAMMA_glMap1f
#define glMap2d __GAMMA_glMap2d
#define glMap2f __GAMMA_glMap2f
#define glMapGrid1d __GAMMA_glMapGrid1d
#define glMapGrid1f __GAMMA_glMapGrid1f
#define glMapGrid2d __GAMMA_glMapGrid2d
#define glMapGrid2f __GAMMA_glMapGrid2f
#define glMaterialf __GAMMA_glMaterialf
#define glMaterialfv __GAMMA_glMaterialfv
#define glMateriali __GAMMA_glMateriali
#define glMaterialiv __GAMMA_glMaterialiv
#define glMatrixMode __GAMMA_glMatrixMode
#define glMultMatrixd __GAMMA_glMultMatrixd
#define glMultMatrixf __GAMMA_glMultMatrixf
#define glNewList __GAMMA_glNewList
#define glNormal3b __GAMMA_glNormal3b
#define glNormal3bv __GAMMA_glNormal3bv
#define glNormal3d __GAMMA_glNormal3d
#define glNormal3dv __GAMMA_glNormal3dv
#define glNormal3f __GAMMA_glNormal3f
#define glNormal3fv __GAMMA_glNormal3fv
#define glNormal3i __GAMMA_glNormal3i
#define glNormal3iv __GAMMA_glNormal3iv
#define glNormal3s __GAMMA_glNormal3s
#define glNormal3sv __GAMMA_glNormal3sv
#define glNormalPointer __GAMMA_glNormalPointer
#define glOrtho __GAMMA_glOrtho
#define glPassThrough __GAMMA_glPassThrough
#define glPixelMapfv __GAMMA_glPixelMapfv
#define glPixelMapuiv __GAMMA_glPixelMapuiv
#define glPixelMapusv __GAMMA_glPixelMapusv
#define glPixelStoref __GAMMA_glPixelStoref
#define glPixelStorei __GAMMA_glPixelStorei
#define glPixelTransferf __GAMMA_glPixelTransferf
#define glPixelTransferi __GAMMA_glPixelTransferi
#define glPixelZoom __GAMMA_glPixelZoom
#define glPointSize __GAMMA_glPointSize
#define glPolygonMode __GAMMA_glPolygonMode
#define glPolygonOffset __GAMMA_glPolygonOffset
#define glPolygonStipple __GAMMA_glPolygonStipple
#define glPopAttrib __GAMMA_glPopAttrib
#define glPopClientAttrib __GAMMA_glPopClientAttrib
#define glPopMatrix __GAMMA_glPopMatrix
#define glPopName __GAMMA_glPopName
#define glPrioritizeTextures __GAMMA_glPrioritizeTextures
#define glPushAttrib __GAMMA_glPushAttrib
#define glPushClientAttrib __GAMMA_glPushClientAttrib
#define glPushMatrix __GAMMA_glPushMatrix
#define glPushName __GAMMA_glPushName
#define glRasterPos2d __GAMMA_glRasterPos2d
#define glRasterPos2dv __GAMMA_glRasterPos2dv
#define glRasterPos2f __GAMMA_glRasterPos2f
#define glRasterPos2fv __GAMMA_glRasterPos2fv
#define glRasterPos2i __GAMMA_glRasterPos2i
#define glRasterPos2iv __GAMMA_glRasterPos2iv
#define glRasterPos2s __GAMMA_glRasterPos2s
#define glRasterPos2sv __GAMMA_glRasterPos2sv
#define glRasterPos3d __GAMMA_glRasterPos3d
#define glRasterPos3dv __GAMMA_glRasterPos3dv
#define glRasterPos3f __GAMMA_glRasterPos3f
#define glRasterPos3fv __GAMMA_glRasterPos3fv
#define glRasterPos3i __GAMMA_glRasterPos3i
#define glRasterPos3iv __GAMMA_glRasterPos3iv
#define glRasterPos3s __GAMMA_glRasterPos3s
#define glRasterPos3sv __GAMMA_glRasterPos3sv
#define glRasterPos4d __GAMMA_glRasterPos4d
#define glRasterPos4dv __GAMMA_glRasterPos4dv
#define glRasterPos4f __GAMMA_glRasterPos4f
#define glRasterPos4fv __GAMMA_glRasterPos4fv
#define glRasterPos4i __GAMMA_glRasterPos4i
#define glRasterPos4iv __GAMMA_glRasterPos4iv
#define glRasterPos4s __GAMMA_glRasterPos4s
#define glRasterPos4sv __GAMMA_glRasterPos4sv
#define glReadBuffer __GAMMA_glReadBuffer
#define glReadPixels __GAMMA_glReadPixels
#define glRectd __GAMMA_glRectd
#define glRectdv __GAMMA_glRectdv
#define glRectf __GAMMA_glRectf
#define glRectfv __GAMMA_glRectfv
#define glRecti __GAMMA_glRecti
#define glRectiv __GAMMA_glRectiv
#define glRects __GAMMA_glRects
#define glRectsv __GAMMA_glRectsv
#define glRenderMode __GAMMA_glRenderMode
#define glRotated __GAMMA_glRotated
#define glRotatef __GAMMA_glRotatef
#define glScaled __GAMMA_glScaled
#define glScalef __GAMMA_glScalef
#define glScissor __GAMMA_glScissor
#define glSelectBuffer __GAMMA_glSelectBuffer
#define glShadeModel __GAMMA_glShadeModel
#define glStencilFunc __GAMMA_glStencilFunc
#define glStencilMask __GAMMA_glStencilMask
#define glStencilOp __GAMMA_glStencilOp
#define glTexCoord1d __GAMMA_glTexCoord1d
#define glTexCoord1dv __GAMMA_glTexCoord1dv
#define glTexCoord1f __GAMMA_glTexCoord1f
#define glTexCoord1fv __GAMMA_glTexCoord1fv
#define glTexCoord1i __GAMMA_glTexCoord1i
#define glTexCoord1iv __GAMMA_glTexCoord1iv
#define glTexCoord1s __GAMMA_glTexCoord1s
#define glTexCoord1sv __GAMMA_glTexCoord1sv
#define glTexCoord2d __GAMMA_glTexCoord2d
#define glTexCoord2dv __GAMMA_glTexCoord2dv
#define glTexCoord2f __GAMMA_glTexCoord2f
#define glTexCoord2fv __GAMMA_glTexCoord2fv
#define glTexCoord2i __GAMMA_glTexCoord2i
#define glTexCoord2iv __GAMMA_glTexCoord2iv
#define glTexCoord2s __GAMMA_glTexCoord2s
#define glTexCoord2sv __GAMMA_glTexCoord2sv
#define glTexCoord3d __GAMMA_glTexCoord3d
#define glTexCoord3dv __GAMMA_glTexCoord3dv
#define glTexCoord3f __GAMMA_glTexCoord3f
#define glTexCoord3fv __GAMMA_glTexCoord3fv
#define glTexCoord3i __GAMMA_glTexCoord3i
#define glTexCoord3iv __GAMMA_glTexCoord3iv
#define glTexCoord3s __GAMMA_glTexCoord3s
#define glTexCoord3sv __GAMMA_glTexCoord3sv
#define glTexCoord4d __GAMMA_glTexCoord4d
#define glTexCoord4dv __GAMMA_glTexCoord4dv
#define glTexCoord4f __GAMMA_glTexCoord4f
#define glTexCoord4fv __GAMMA_glTexCoord4fv
#define glTexCoord4i __GAMMA_glTexCoord4i
#define glTexCoord4iv __GAMMA_glTexCoord4iv
#define glTexCoord4s __GAMMA_glTexCoord4s
#define glTexCoord4sv __GAMMA_glTexCoord4sv
#define glTexCoordPointer __GAMMA_glTexCoordPointer
#define glTexEnvf __GAMMA_glTexEnvf
#define glTexEnvfv __GAMMA_glTexEnvfv
#define glTexEnvi __GAMMA_glTexEnvi
#define glTexEnviv __GAMMA_glTexEnviv
#define glTexGend __GAMMA_glTexGend
#define glTexGendv __GAMMA_glTexGendv
#define glTexGenf __GAMMA_glTexGenf
#define glTexGenfv __GAMMA_glTexGenfv
#define glTexGeni __GAMMA_glTexGeni
#define glTexGeniv __GAMMA_glTexGeniv
#define glTexImage1D __GAMMA_glTexImage1D
#define glTexImage2D __GAMMA_glTexImage2D
#define glTexParameterf __GAMMA_glTexParameterf
#define glTexParameterfv __GAMMA_glTexParameterfv
#define glTexParameteri __GAMMA_glTexParameteri
#define glTexParameteriv __GAMMA_glTexParameteriv
#define glTexSubImage1D __GAMMA_glTexSubImage1D
#define glTexSubImage2D __GAMMA_glTexSubImage2D
#define glTranslated __GAMMA_glTranslated
#define glTranslatef __GAMMA_glTranslatef
#define glVertex2d __GAMMA_glVertex2d
#define glVertex2dv __GAMMA_glVertex2dv
#define glVertex2f __GAMMA_glVertex2f
#define glVertex2fv __GAMMA_glVertex2fv
#define glVertex2i __GAMMA_glVertex2i
#define glVertex2iv __GAMMA_glVertex2iv
#define glVertex2s __GAMMA_glVertex2s
#define glVertex2sv __GAMMA_glVertex2sv
#define glVertex3d __GAMMA_glVertex3d
#define glVertex3dv __GAMMA_glVertex3dv
#define glVertex3f __GAMMA_glVertex3f
#define glVertex3fv __GAMMA_glVertex3fv
#define glVertex3i __GAMMA_glVertex3i
#define glVertex3iv __GAMMA_glVertex3iv
#define glVertex3s __GAMMA_glVertex3s
#define glVertex3sv __GAMMA_glVertex3sv
#define glVertex4d __GAMMA_glVertex4d
#define glVertex4dv __GAMMA_glVertex4dv
#define glVertex4f __GAMMA_glVertex4f
#define glVertex4fv __GAMMA_glVertex4fv
#define glVertex4i __GAMMA_glVertex4i
#define glVertex4iv __GAMMA_glVertex4iv
#define glVertex4s __GAMMA_glVertex4s
#define glVertex4sv __GAMMA_glVertex4sv
#define glVertexPointer __GAMMA_glVertexPointer
#define glViewport __GAMMA_glViewport
#endif

#endif /* _GAMMA_GL_WRAP_H_ */
