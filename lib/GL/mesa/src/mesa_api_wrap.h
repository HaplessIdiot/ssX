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
 * $PI: xc/lib/GL/mesa/src/mesa_api_wrap.h,v 1.1 1999/03/23 06:20:15 martin Exp $
 */

#ifndef _MESA_API_WRAP_H_
#define _MESA_API_WRAP_H_

#if defined(GLX_DIRECT_RENDERING) && defined(NEED_MESA_FUNCS_WRAPPED)

/* NOTE: This file could be automatically generated */

#define glAccum __MESA_SW_glAccum
#define glAlphaFunc __MESA_SW_glAlphaFunc
#define glAreTexturesResident __MESA_SW_glAreTexturesResident
#define glArrayElement __MESA_SW_glArrayElement
#define glBegin __MESA_SW_glBegin
#define glBindTexture __MESA_SW_glBindTexture
#define glBitmap __MESA_SW_glBitmap
#define glBlendFunc __MESA_SW_glBlendFunc
#define glCallList __MESA_SW_glCallList
#define glCallLists __MESA_SW_glCallLists
#define glClear __MESA_SW_glClear
#define glClearAccum __MESA_SW_glClearAccum
#define glClearColor __MESA_SW_glClearColor
#define glClearDepth __MESA_SW_glClearDepth
#define glClearIndex __MESA_SW_glClearIndex
#define glClearStencil __MESA_SW_glClearStencil
#define glClipPlane __MESA_SW_glClipPlane
#define glColor3b __MESA_SW_glColor3b
#define glColor3bv __MESA_SW_glColor3bv
#define glColor3d __MESA_SW_glColor3d
#define glColor3dv __MESA_SW_glColor3dv
#define glColor3f __MESA_SW_glColor3f
#define glColor3fv __MESA_SW_glColor3fv
#define glColor3i __MESA_SW_glColor3i
#define glColor3iv __MESA_SW_glColor3iv
#define glColor3s __MESA_SW_glColor3s
#define glColor3sv __MESA_SW_glColor3sv
#define glColor3ub __MESA_SW_glColor3ub
#define glColor3ubv __MESA_SW_glColor3ubv
#define glColor3ui __MESA_SW_glColor3ui
#define glColor3uiv __MESA_SW_glColor3uiv
#define glColor3us __MESA_SW_glColor3us
#define glColor3usv __MESA_SW_glColor3usv
#define glColor4b __MESA_SW_glColor4b
#define glColor4bv __MESA_SW_glColor4bv
#define glColor4d __MESA_SW_glColor4d
#define glColor4dv __MESA_SW_glColor4dv
#define glColor4f __MESA_SW_glColor4f
#define glColor4fv __MESA_SW_glColor4fv
#define glColor4i __MESA_SW_glColor4i
#define glColor4iv __MESA_SW_glColor4iv
#define glColor4s __MESA_SW_glColor4s
#define glColor4sv __MESA_SW_glColor4sv
#define glColor4ub __MESA_SW_glColor4ub
#define glColor4ubv __MESA_SW_glColor4ubv
#define glColor4ui __MESA_SW_glColor4ui
#define glColor4uiv __MESA_SW_glColor4uiv
#define glColor4us __MESA_SW_glColor4us
#define glColor4usv __MESA_SW_glColor4usv
#define glColorMask __MESA_SW_glColorMask
#define glColorMaterial __MESA_SW_glColorMaterial
#define glColorPointer __MESA_SW_glColorPointer
#define glCopyPixels __MESA_SW_glCopyPixels
#define glCopyTexImage1D __MESA_SW_glCopyTexImage1D
#define glCopyTexImage2D __MESA_SW_glCopyTexImage2D
#define glCopyTexSubImage1D __MESA_SW_glCopyTexSubImage1D
#define glCopyTexSubImage2D __MESA_SW_glCopyTexSubImage2D
#define glCullFace __MESA_SW_glCullFace
#define glDeleteLists __MESA_SW_glDeleteLists
#define glDeleteTextures __MESA_SW_glDeleteTextures
#define glDepthFunc __MESA_SW_glDepthFunc
#define glDepthMask __MESA_SW_glDepthMask
#define glDepthRange __MESA_SW_glDepthRange
#define glDisable __MESA_SW_glDisable
#define glDisableClientState __MESA_SW_glDisableClientState
#define glDrawArrays __MESA_SW_glDrawArrays
#define glDrawBuffer __MESA_SW_glDrawBuffer
#define glDrawElements __MESA_SW_glDrawElements
#define glDrawPixels __MESA_SW_glDrawPixels
#define glEdgeFlag __MESA_SW_glEdgeFlag
#define glEdgeFlagPointer __MESA_SW_glEdgeFlagPointer
#define glEdgeFlagv __MESA_SW_glEdgeFlagv
#define glEnable __MESA_SW_glEnable
#define glEnableClientState __MESA_SW_glEnableClientState
#define glEnd __MESA_SW_glEnd
#define glEndList __MESA_SW_glEndList
#define glEvalCoord1d __MESA_SW_glEvalCoord1d
#define glEvalCoord1dv __MESA_SW_glEvalCoord1dv
#define glEvalCoord1f __MESA_SW_glEvalCoord1f
#define glEvalCoord1fv __MESA_SW_glEvalCoord1fv
#define glEvalCoord2d __MESA_SW_glEvalCoord2d
#define glEvalCoord2dv __MESA_SW_glEvalCoord2dv
#define glEvalCoord2f __MESA_SW_glEvalCoord2f
#define glEvalCoord2fv __MESA_SW_glEvalCoord2fv
#define glEvalMesh1 __MESA_SW_glEvalMesh1
#define glEvalMesh2 __MESA_SW_glEvalMesh2
#define glEvalPoint1 __MESA_SW_glEvalPoint1
#define glEvalPoint2 __MESA_SW_glEvalPoint2
#define glFeedbackBuffer __MESA_SW_glFeedbackBuffer
#define glFinish __MESA_SW_glFinish
#define glFlush __MESA_SW_glFlush
#define glFogf __MESA_SW_glFogf
#define glFogfv __MESA_SW_glFogfv
#define glFogi __MESA_SW_glFogi
#define glFogiv __MESA_SW_glFogiv
#define glFrontFace __MESA_SW_glFrontFace
#define glFrustum __MESA_SW_glFrustum
#define glGenLists __MESA_SW_glGenLists
#define glGenTextures __MESA_SW_glGenTextures
#define glGetBooleanv __MESA_SW_glGetBooleanv
#define glGetClipPlane __MESA_SW_glGetClipPlane
#define glGetDoublev __MESA_SW_glGetDoublev
#define glGetError __MESA_SW_glGetError
#define glGetFloatv __MESA_SW_glGetFloatv
#define glGetIntegerv __MESA_SW_glGetIntegerv
#define glGetLightfv __MESA_SW_glGetLightfv
#define glGetLightiv __MESA_SW_glGetLightiv
#define glGetMapdv __MESA_SW_glGetMapdv
#define glGetMapfv __MESA_SW_glGetMapfv
#define glGetMapiv __MESA_SW_glGetMapiv
#define glGetMaterialfv __MESA_SW_glGetMaterialfv
#define glGetMaterialiv __MESA_SW_glGetMaterialiv
#define glGetPixelMapfv __MESA_SW_glGetPixelMapfv
#define glGetPixelMapuiv __MESA_SW_glGetPixelMapuiv
#define glGetPixelMapusv __MESA_SW_glGetPixelMapusv
#define glGetPointerv __MESA_SW_glGetPointerv
#define glGetPolygonStipple __MESA_SW_glGetPolygonStipple
#define glGetString __MESA_SW_glGetString
#define glGetTexEnvfv __MESA_SW_glGetTexEnvfv
#define glGetTexEnviv __MESA_SW_glGetTexEnviv
#define glGetTexGendv __MESA_SW_glGetTexGendv
#define glGetTexGenfv __MESA_SW_glGetTexGenfv
#define glGetTexGeniv __MESA_SW_glGetTexGeniv
#define glGetTexImage __MESA_SW_glGetTexImage
#define glGetTexLevelParameterfv __MESA_SW_glGetTexLevelParameterfv
#define glGetTexLevelParameteriv __MESA_SW_glGetTexLevelParameteriv
#define glGetTexParameterfv __MESA_SW_glGetTexParameterfv
#define glGetTexParameteriv __MESA_SW_glGetTexParameteriv
#define glHint __MESA_SW_glHint
#define glIndexMask __MESA_SW_glIndexMask
#define glIndexPointer __MESA_SW_glIndexPointer
#define glIndexd __MESA_SW_glIndexd
#define glIndexdv __MESA_SW_glIndexdv
#define glIndexf __MESA_SW_glIndexf
#define glIndexfv __MESA_SW_glIndexfv
#define glIndexi __MESA_SW_glIndexi
#define glIndexiv __MESA_SW_glIndexiv
#define glIndexs __MESA_SW_glIndexs
#define glIndexsv __MESA_SW_glIndexsv
#define glIndexub __MESA_SW_glIndexub
#define glIndexubv __MESA_SW_glIndexubv
#define glInitNames __MESA_SW_glInitNames
#define glInterleavedArrays __MESA_SW_glInterleavedArrays
#define glIsEnabled __MESA_SW_glIsEnabled
#define glIsList __MESA_SW_glIsList
#define glIsTexture __MESA_SW_glIsTexture
#define glLightModelf __MESA_SW_glLightModelf
#define glLightModelfv __MESA_SW_glLightModelfv
#define glLightModeli __MESA_SW_glLightModeli
#define glLightModeliv __MESA_SW_glLightModeliv
#define glLightf __MESA_SW_glLightf
#define glLightfv __MESA_SW_glLightfv
#define glLighti __MESA_SW_glLighti
#define glLightiv __MESA_SW_glLightiv
#define glLineStipple __MESA_SW_glLineStipple
#define glLineWidth __MESA_SW_glLineWidth
#define glListBase __MESA_SW_glListBase
#define glLoadIdentity __MESA_SW_glLoadIdentity
#define glLoadMatrixd __MESA_SW_glLoadMatrixd
#define glLoadMatrixf __MESA_SW_glLoadMatrixf
#define glLoadName __MESA_SW_glLoadName
#define glLogicOp __MESA_SW_glLogicOp
#define glMap1d __MESA_SW_glMap1d
#define glMap1f __MESA_SW_glMap1f
#define glMap2d __MESA_SW_glMap2d
#define glMap2f __MESA_SW_glMap2f
#define glMapGrid1d __MESA_SW_glMapGrid1d
#define glMapGrid1f __MESA_SW_glMapGrid1f
#define glMapGrid2d __MESA_SW_glMapGrid2d
#define glMapGrid2f __MESA_SW_glMapGrid2f
#define glMaterialf __MESA_SW_glMaterialf
#define glMaterialfv __MESA_SW_glMaterialfv
#define glMateriali __MESA_SW_glMateriali
#define glMaterialiv __MESA_SW_glMaterialiv
#define glMatrixMode __MESA_SW_glMatrixMode
#define glMultMatrixd __MESA_SW_glMultMatrixd
#define glMultMatrixf __MESA_SW_glMultMatrixf
#define glNewList __MESA_SW_glNewList
#define glNormal3b __MESA_SW_glNormal3b
#define glNormal3bv __MESA_SW_glNormal3bv
#define glNormal3d __MESA_SW_glNormal3d
#define glNormal3dv __MESA_SW_glNormal3dv
#define glNormal3f __MESA_SW_glNormal3f
#define glNormal3fv __MESA_SW_glNormal3fv
#define glNormal3i __MESA_SW_glNormal3i
#define glNormal3iv __MESA_SW_glNormal3iv
#define glNormal3s __MESA_SW_glNormal3s
#define glNormal3sv __MESA_SW_glNormal3sv
#define glNormalPointer __MESA_SW_glNormalPointer
#define glOrtho __MESA_SW_glOrtho
#define glPassThrough __MESA_SW_glPassThrough
#define glPixelMapfv __MESA_SW_glPixelMapfv
#define glPixelMapuiv __MESA_SW_glPixelMapuiv
#define glPixelMapusv __MESA_SW_glPixelMapusv
#define glPixelStoref __MESA_SW_glPixelStoref
#define glPixelStorei __MESA_SW_glPixelStorei
#define glPixelTransferf __MESA_SW_glPixelTransferf
#define glPixelTransferi __MESA_SW_glPixelTransferi
#define glPixelZoom __MESA_SW_glPixelZoom
#define glPointSize __MESA_SW_glPointSize
#define glPolygonMode __MESA_SW_glPolygonMode
#define glPolygonOffset __MESA_SW_glPolygonOffset
#define glPolygonStipple __MESA_SW_glPolygonStipple
#define glPopAttrib __MESA_SW_glPopAttrib
#define glPopClientAttrib __MESA_SW_glPopClientAttrib
#define glPopMatrix __MESA_SW_glPopMatrix
#define glPopName __MESA_SW_glPopName
#define glPrioritizeTextures __MESA_SW_glPrioritizeTextures
#define glPushAttrib __MESA_SW_glPushAttrib
#define glPushClientAttrib __MESA_SW_glPushClientAttrib
#define glPushMatrix __MESA_SW_glPushMatrix
#define glPushName __MESA_SW_glPushName
#define glRasterPos2d __MESA_SW_glRasterPos2d
#define glRasterPos2dv __MESA_SW_glRasterPos2dv
#define glRasterPos2f __MESA_SW_glRasterPos2f
#define glRasterPos2fv __MESA_SW_glRasterPos2fv
#define glRasterPos2i __MESA_SW_glRasterPos2i
#define glRasterPos2iv __MESA_SW_glRasterPos2iv
#define glRasterPos2s __MESA_SW_glRasterPos2s
#define glRasterPos2sv __MESA_SW_glRasterPos2sv
#define glRasterPos3d __MESA_SW_glRasterPos3d
#define glRasterPos3dv __MESA_SW_glRasterPos3dv
#define glRasterPos3f __MESA_SW_glRasterPos3f
#define glRasterPos3fv __MESA_SW_glRasterPos3fv
#define glRasterPos3i __MESA_SW_glRasterPos3i
#define glRasterPos3iv __MESA_SW_glRasterPos3iv
#define glRasterPos3s __MESA_SW_glRasterPos3s
#define glRasterPos3sv __MESA_SW_glRasterPos3sv
#define glRasterPos4d __MESA_SW_glRasterPos4d
#define glRasterPos4dv __MESA_SW_glRasterPos4dv
#define glRasterPos4f __MESA_SW_glRasterPos4f
#define glRasterPos4fv __MESA_SW_glRasterPos4fv
#define glRasterPos4i __MESA_SW_glRasterPos4i
#define glRasterPos4iv __MESA_SW_glRasterPos4iv
#define glRasterPos4s __MESA_SW_glRasterPos4s
#define glRasterPos4sv __MESA_SW_glRasterPos4sv
#define glReadBuffer __MESA_SW_glReadBuffer
#define glReadPixels __MESA_SW_glReadPixels
#define glRectd __MESA_SW_glRectd
#define glRectdv __MESA_SW_glRectdv
#define glRectf __MESA_SW_glRectf
#define glRectfv __MESA_SW_glRectfv
#define glRecti __MESA_SW_glRecti
#define glRectiv __MESA_SW_glRectiv
#define glRects __MESA_SW_glRects
#define glRectsv __MESA_SW_glRectsv
#define glRenderMode __MESA_SW_glRenderMode
#define glRotated __MESA_SW_glRotated
#define glRotatef __MESA_SW_glRotatef
#define glScaled __MESA_SW_glScaled
#define glScalef __MESA_SW_glScalef
#define glScissor __MESA_SW_glScissor
#define glSelectBuffer __MESA_SW_glSelectBuffer
#define glShadeModel __MESA_SW_glShadeModel
#define glStencilFunc __MESA_SW_glStencilFunc
#define glStencilMask __MESA_SW_glStencilMask
#define glStencilOp __MESA_SW_glStencilOp
#define glTexCoord1d __MESA_SW_glTexCoord1d
#define glTexCoord1dv __MESA_SW_glTexCoord1dv
#define glTexCoord1f __MESA_SW_glTexCoord1f
#define glTexCoord1fv __MESA_SW_glTexCoord1fv
#define glTexCoord1i __MESA_SW_glTexCoord1i
#define glTexCoord1iv __MESA_SW_glTexCoord1iv
#define glTexCoord1s __MESA_SW_glTexCoord1s
#define glTexCoord1sv __MESA_SW_glTexCoord1sv
#define glTexCoord2d __MESA_SW_glTexCoord2d
#define glTexCoord2dv __MESA_SW_glTexCoord2dv
#define glTexCoord2f __MESA_SW_glTexCoord2f
#define glTexCoord2fv __MESA_SW_glTexCoord2fv
#define glTexCoord2i __MESA_SW_glTexCoord2i
#define glTexCoord2iv __MESA_SW_glTexCoord2iv
#define glTexCoord2s __MESA_SW_glTexCoord2s
#define glTexCoord2sv __MESA_SW_glTexCoord2sv
#define glTexCoord3d __MESA_SW_glTexCoord3d
#define glTexCoord3dv __MESA_SW_glTexCoord3dv
#define glTexCoord3f __MESA_SW_glTexCoord3f
#define glTexCoord3fv __MESA_SW_glTexCoord3fv
#define glTexCoord3i __MESA_SW_glTexCoord3i
#define glTexCoord3iv __MESA_SW_glTexCoord3iv
#define glTexCoord3s __MESA_SW_glTexCoord3s
#define glTexCoord3sv __MESA_SW_glTexCoord3sv
#define glTexCoord4d __MESA_SW_glTexCoord4d
#define glTexCoord4dv __MESA_SW_glTexCoord4dv
#define glTexCoord4f __MESA_SW_glTexCoord4f
#define glTexCoord4fv __MESA_SW_glTexCoord4fv
#define glTexCoord4i __MESA_SW_glTexCoord4i
#define glTexCoord4iv __MESA_SW_glTexCoord4iv
#define glTexCoord4s __MESA_SW_glTexCoord4s
#define glTexCoord4sv __MESA_SW_glTexCoord4sv
#define glTexCoordPointer __MESA_SW_glTexCoordPointer
#define glTexEnvf __MESA_SW_glTexEnvf
#define glTexEnvfv __MESA_SW_glTexEnvfv
#define glTexEnvi __MESA_SW_glTexEnvi
#define glTexEnviv __MESA_SW_glTexEnviv
#define glTexGend __MESA_SW_glTexGend
#define glTexGendv __MESA_SW_glTexGendv
#define glTexGenf __MESA_SW_glTexGenf
#define glTexGenfv __MESA_SW_glTexGenfv
#define glTexGeni __MESA_SW_glTexGeni
#define glTexGeniv __MESA_SW_glTexGeniv
#define glTexImage1D __MESA_SW_glTexImage1D
#define glTexImage2D __MESA_SW_glTexImage2D
#define glTexParameterf __MESA_SW_glTexParameterf
#define glTexParameterfv __MESA_SW_glTexParameterfv
#define glTexParameteri __MESA_SW_glTexParameteri
#define glTexParameteriv __MESA_SW_glTexParameteriv
#define glTexSubImage1D __MESA_SW_glTexSubImage1D
#define glTexSubImage2D __MESA_SW_glTexSubImage2D
#define glTranslated __MESA_SW_glTranslated
#define glTranslatef __MESA_SW_glTranslatef
#define glVertex2d __MESA_SW_glVertex2d
#define glVertex2dv __MESA_SW_glVertex2dv
#define glVertex2f __MESA_SW_glVertex2f
#define glVertex2fv __MESA_SW_glVertex2fv
#define glVertex2i __MESA_SW_glVertex2i
#define glVertex2iv __MESA_SW_glVertex2iv
#define glVertex2s __MESA_SW_glVertex2s
#define glVertex2sv __MESA_SW_glVertex2sv
#define glVertex3d __MESA_SW_glVertex3d
#define glVertex3dv __MESA_SW_glVertex3dv
#define glVertex3f __MESA_SW_glVertex3f
#define glVertex3fv __MESA_SW_glVertex3fv
#define glVertex3i __MESA_SW_glVertex3i
#define glVertex3iv __MESA_SW_glVertex3iv
#define glVertex3s __MESA_SW_glVertex3s
#define glVertex3sv __MESA_SW_glVertex3sv
#define glVertex4d __MESA_SW_glVertex4d
#define glVertex4dv __MESA_SW_glVertex4dv
#define glVertex4f __MESA_SW_glVertex4f
#define glVertex4fv __MESA_SW_glVertex4fv
#define glVertex4i __MESA_SW_glVertex4i
#define glVertex4iv __MESA_SW_glVertex4iv
#define glVertex4s __MESA_SW_glVertex4s
#define glVertex4sv __MESA_SW_glVertex4sv
#define glVertexPointer __MESA_SW_glVertexPointer
#define glViewport __MESA_SW_glViewport
#endif

#endif /* _MESA_API_WRAP_H_ */
