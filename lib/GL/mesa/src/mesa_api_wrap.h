/* $XFree86: xc/lib/GL/mesa/src/mesa_api_wrap.h,v 1.1 1999/06/14 07:23:44 dawes Exp $ */
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
 * $PI: xc/lib/GL/mesa/src/mesa_api_wrap.h,v 1.3 1999/06/14 21:10:40 faith Exp $
 */

#ifndef _MESA_API_WRAP_H_
#define _MESA_API_WRAP_H_

#if defined(GLX_DIRECT_RENDERING) && defined(NEED_MESA_FUNCS_WRAPPED)

/* NOTE: This file could be automatically generated */

#define glAccum __glAccum
#define glAlphaFunc __glAlphaFunc
#define glAreTexturesResident __glAreTexturesResident
#define glArrayElement __glArrayElement
#define glBegin __glBegin
#define glBindTexture __glBindTexture
#define glBitmap __glBitmap
#define glBlendFunc __glBlendFunc
#define glCallList __glCallList
#define glCallLists __glCallLists
#define glClear __glClear
#define glClearAccum __glClearAccum
#define glClearColor __glClearColor
#define glClearDepth __glClearDepth
#define glClearIndex __glClearIndex
#define glClearStencil __glClearStencil
#define glClipPlane __glClipPlane
#define glColor3b __glColor3b
#define glColor3bv __glColor3bv
#define glColor3d __glColor3d
#define glColor3dv __glColor3dv
#define glColor3f __glColor3f
#define glColor3fv __glColor3fv
#define glColor3i __glColor3i
#define glColor3iv __glColor3iv
#define glColor3s __glColor3s
#define glColor3sv __glColor3sv
#define glColor3ub __glColor3ub
#define glColor3ubv __glColor3ubv
#define glColor3ui __glColor3ui
#define glColor3uiv __glColor3uiv
#define glColor3us __glColor3us
#define glColor3usv __glColor3usv
#define glColor4b __glColor4b
#define glColor4bv __glColor4bv
#define glColor4d __glColor4d
#define glColor4dv __glColor4dv
#define glColor4f __glColor4f
#define glColor4fv __glColor4fv
#define glColor4i __glColor4i
#define glColor4iv __glColor4iv
#define glColor4s __glColor4s
#define glColor4sv __glColor4sv
#define glColor4ub __glColor4ub
#define glColor4ubv __glColor4ubv
#define glColor4ui __glColor4ui
#define glColor4uiv __glColor4uiv
#define glColor4us __glColor4us
#define glColor4usv __glColor4usv
#define glColorMask __glColorMask
#define glColorMaterial __glColorMaterial
#define glColorPointer __glColorPointer
#define glCopyPixels __glCopyPixels
#define glCopyTexImage1D __glCopyTexImage1D
#define glCopyTexImage2D __glCopyTexImage2D
#define glCopyTexSubImage1D __glCopyTexSubImage1D
#define glCopyTexSubImage2D __glCopyTexSubImage2D
#define glCullFace __glCullFace
#define glDeleteLists __glDeleteLists
#define glDeleteTextures __glDeleteTextures
#define glDepthFunc __glDepthFunc
#define glDepthMask __glDepthMask
#define glDepthRange __glDepthRange
#define glDisable __glDisable
#define glDisableClientState __glDisableClientState
#define glDrawArrays __glDrawArrays
#define glDrawBuffer __glDrawBuffer
#define glDrawElements __glDrawElements
#define glDrawPixels __glDrawPixels
#define glEdgeFlag __glEdgeFlag
#define glEdgeFlagPointer __glEdgeFlagPointer
#define glEdgeFlagv __glEdgeFlagv
#define glEnable __glEnable
#define glEnableClientState __glEnableClientState
#define glEnd __glEnd
#define glEndList __glEndList
#define glEvalCoord1d __glEvalCoord1d
#define glEvalCoord1dv __glEvalCoord1dv
#define glEvalCoord1f __glEvalCoord1f
#define glEvalCoord1fv __glEvalCoord1fv
#define glEvalCoord2d __glEvalCoord2d
#define glEvalCoord2dv __glEvalCoord2dv
#define glEvalCoord2f __glEvalCoord2f
#define glEvalCoord2fv __glEvalCoord2fv
#define glEvalMesh1 __glEvalMesh1
#define glEvalMesh2 __glEvalMesh2
#define glEvalPoint1 __glEvalPoint1
#define glEvalPoint2 __glEvalPoint2
#define glFeedbackBuffer __glFeedbackBuffer
#define glFinish __glFinish
#define glFlush __glFlush
#define glFogf __glFogf
#define glFogfv __glFogfv
#define glFogi __glFogi
#define glFogiv __glFogiv
#define glFrontFace __glFrontFace
#define glFrustum __glFrustum
#define glGenLists __glGenLists
#define glGenTextures __glGenTextures
#define glGetBooleanv __glGetBooleanv
#define glGetClipPlane __glGetClipPlane
#define glGetDoublev __glGetDoublev
#define glGetError __glGetError
#define glGetFloatv __glGetFloatv
#define glGetIntegerv __glGetIntegerv
#define glGetLightfv __glGetLightfv
#define glGetLightiv __glGetLightiv
#define glGetMapdv __glGetMapdv
#define glGetMapfv __glGetMapfv
#define glGetMapiv __glGetMapiv
#define glGetMaterialfv __glGetMaterialfv
#define glGetMaterialiv __glGetMaterialiv
#define glGetPixelMapfv __glGetPixelMapfv
#define glGetPixelMapuiv __glGetPixelMapuiv
#define glGetPixelMapusv __glGetPixelMapusv
#define glGetPointerv __glGetPointerv
#define glGetPolygonStipple __glGetPolygonStipple
#define glGetString __glGetString
#define glGetTexEnvfv __glGetTexEnvfv
#define glGetTexEnviv __glGetTexEnviv
#define glGetTexGendv __glGetTexGendv
#define glGetTexGenfv __glGetTexGenfv
#define glGetTexGeniv __glGetTexGeniv
#define glGetTexImage __glGetTexImage
#define glGetTexLevelParameterfv __glGetTexLevelParameterfv
#define glGetTexLevelParameteriv __glGetTexLevelParameteriv
#define glGetTexParameterfv __glGetTexParameterfv
#define glGetTexParameteriv __glGetTexParameteriv
#define glHint __glHint
#define glIndexMask __glIndexMask
#define glIndexPointer __glIndexPointer
#define glIndexd __glIndexd
#define glIndexdv __glIndexdv
#define glIndexf __glIndexf
#define glIndexfv __glIndexfv
#define glIndexi __glIndexi
#define glIndexiv __glIndexiv
#define glIndexs __glIndexs
#define glIndexsv __glIndexsv
#define glIndexub __glIndexub
#define glIndexubv __glIndexubv
#define glInitNames __glInitNames
#define glInterleavedArrays __glInterleavedArrays
#define glIsEnabled __glIsEnabled
#define glIsList __glIsList
#define glIsTexture __glIsTexture
#define glLightModelf __glLightModelf
#define glLightModelfv __glLightModelfv
#define glLightModeli __glLightModeli
#define glLightModeliv __glLightModeliv
#define glLightf __glLightf
#define glLightfv __glLightfv
#define glLighti __glLighti
#define glLightiv __glLightiv
#define glLineStipple __glLineStipple
#define glLineWidth __glLineWidth
#define glListBase __glListBase
#define glLoadIdentity __glLoadIdentity
#define glLoadMatrixd __glLoadMatrixd
#define glLoadMatrixf __glLoadMatrixf
#define glLoadName __glLoadName
#define glLogicOp __glLogicOp
#define glMap1d __glMap1d
#define glMap1f __glMap1f
#define glMap2d __glMap2d
#define glMap2f __glMap2f
#define glMapGrid1d __glMapGrid1d
#define glMapGrid1f __glMapGrid1f
#define glMapGrid2d __glMapGrid2d
#define glMapGrid2f __glMapGrid2f
#define glMaterialf __glMaterialf
#define glMaterialfv __glMaterialfv
#define glMateriali __glMateriali
#define glMaterialiv __glMaterialiv
#define glMatrixMode __glMatrixMode
#define glMultMatrixd __glMultMatrixd
#define glMultMatrixf __glMultMatrixf
#define glNewList __glNewList
#define glNormal3b __glNormal3b
#define glNormal3bv __glNormal3bv
#define glNormal3d __glNormal3d
#define glNormal3dv __glNormal3dv
#define glNormal3f __glNormal3f
#define glNormal3fv __glNormal3fv
#define glNormal3i __glNormal3i
#define glNormal3iv __glNormal3iv
#define glNormal3s __glNormal3s
#define glNormal3sv __glNormal3sv
#define glNormalPointer __glNormalPointer
#define glOrtho __glOrtho
#define glPassThrough __glPassThrough
#define glPixelMapfv __glPixelMapfv
#define glPixelMapuiv __glPixelMapuiv
#define glPixelMapusv __glPixelMapusv
#define glPixelStoref __glPixelStoref
#define glPixelStorei __glPixelStorei
#define glPixelTransferf __glPixelTransferf
#define glPixelTransferi __glPixelTransferi
#define glPixelZoom __glPixelZoom
#define glPointSize __glPointSize
#define glPolygonMode __glPolygonMode
#define glPolygonOffset __glPolygonOffset
#define glPolygonStipple __glPolygonStipple
#define glPopAttrib __glPopAttrib
#define glPopClientAttrib __glPopClientAttrib
#define glPopMatrix __glPopMatrix
#define glPopName __glPopName
#define glPrioritizeTextures __glPrioritizeTextures
#define glPushAttrib __glPushAttrib
#define glPushClientAttrib __glPushClientAttrib
#define glPushMatrix __glPushMatrix
#define glPushName __glPushName
#define glRasterPos2d __glRasterPos2d
#define glRasterPos2dv __glRasterPos2dv
#define glRasterPos2f __glRasterPos2f
#define glRasterPos2fv __glRasterPos2fv
#define glRasterPos2i __glRasterPos2i
#define glRasterPos2iv __glRasterPos2iv
#define glRasterPos2s __glRasterPos2s
#define glRasterPos2sv __glRasterPos2sv
#define glRasterPos3d __glRasterPos3d
#define glRasterPos3dv __glRasterPos3dv
#define glRasterPos3f __glRasterPos3f
#define glRasterPos3fv __glRasterPos3fv
#define glRasterPos3i __glRasterPos3i
#define glRasterPos3iv __glRasterPos3iv
#define glRasterPos3s __glRasterPos3s
#define glRasterPos3sv __glRasterPos3sv
#define glRasterPos4d __glRasterPos4d
#define glRasterPos4dv __glRasterPos4dv
#define glRasterPos4f __glRasterPos4f
#define glRasterPos4fv __glRasterPos4fv
#define glRasterPos4i __glRasterPos4i
#define glRasterPos4iv __glRasterPos4iv
#define glRasterPos4s __glRasterPos4s
#define glRasterPos4sv __glRasterPos4sv
#define glReadBuffer __glReadBuffer
#define glReadPixels __glReadPixels
#define glRectd __glRectd
#define glRectdv __glRectdv
#define glRectf __glRectf
#define glRectfv __glRectfv
#define glRecti __glRecti
#define glRectiv __glRectiv
#define glRects __glRects
#define glRectsv __glRectsv
#define glRenderMode __glRenderMode
#define glRotated __glRotated
#define glRotatef __glRotatef
#define glScaled __glScaled
#define glScalef __glScalef
#define glScissor __glScissor
#define glSelectBuffer __glSelectBuffer
#define glShadeModel __glShadeModel
#define glStencilFunc __glStencilFunc
#define glStencilMask __glStencilMask
#define glStencilOp __glStencilOp
#define glTexCoord1d __glTexCoord1d
#define glTexCoord1dv __glTexCoord1dv
#define glTexCoord1f __glTexCoord1f
#define glTexCoord1fv __glTexCoord1fv
#define glTexCoord1i __glTexCoord1i
#define glTexCoord1iv __glTexCoord1iv
#define glTexCoord1s __glTexCoord1s
#define glTexCoord1sv __glTexCoord1sv
#define glTexCoord2d __glTexCoord2d
#define glTexCoord2dv __glTexCoord2dv
#define glTexCoord2f __glTexCoord2f
#define glTexCoord2fv __glTexCoord2fv
#define glTexCoord2i __glTexCoord2i
#define glTexCoord2iv __glTexCoord2iv
#define glTexCoord2s __glTexCoord2s
#define glTexCoord2sv __glTexCoord2sv
#define glTexCoord3d __glTexCoord3d
#define glTexCoord3dv __glTexCoord3dv
#define glTexCoord3f __glTexCoord3f
#define glTexCoord3fv __glTexCoord3fv
#define glTexCoord3i __glTexCoord3i
#define glTexCoord3iv __glTexCoord3iv
#define glTexCoord3s __glTexCoord3s
#define glTexCoord3sv __glTexCoord3sv
#define glTexCoord4d __glTexCoord4d
#define glTexCoord4dv __glTexCoord4dv
#define glTexCoord4f __glTexCoord4f
#define glTexCoord4fv __glTexCoord4fv
#define glTexCoord4i __glTexCoord4i
#define glTexCoord4iv __glTexCoord4iv
#define glTexCoord4s __glTexCoord4s
#define glTexCoord4sv __glTexCoord4sv
#define glTexCoordPointer __glTexCoordPointer
#define glTexEnvf __glTexEnvf
#define glTexEnvfv __glTexEnvfv
#define glTexEnvi __glTexEnvi
#define glTexEnviv __glTexEnviv
#define glTexGend __glTexGend
#define glTexGendv __glTexGendv
#define glTexGenf __glTexGenf
#define glTexGenfv __glTexGenfv
#define glTexGeni __glTexGeni
#define glTexGeniv __glTexGeniv
#define glTexImage1D __glTexImage1D
#define glTexImage2D __glTexImage2D
#define glTexParameterf __glTexParameterf
#define glTexParameterfv __glTexParameterfv
#define glTexParameteri __glTexParameteri
#define glTexParameteriv __glTexParameteriv
#define glTexSubImage1D __glTexSubImage1D
#define glTexSubImage2D __glTexSubImage2D
#define glTranslated __glTranslated
#define glTranslatef __glTranslatef
#define glVertex2d __glVertex2d
#define glVertex2dv __glVertex2dv
#define glVertex2f __glVertex2f
#define glVertex2fv __glVertex2fv
#define glVertex2i __glVertex2i
#define glVertex2iv __glVertex2iv
#define glVertex2s __glVertex2s
#define glVertex2sv __glVertex2sv
#define glVertex3d __glVertex3d
#define glVertex3dv __glVertex3dv
#define glVertex3f __glVertex3f
#define glVertex3fv __glVertex3fv
#define glVertex3i __glVertex3i
#define glVertex3iv __glVertex3iv
#define glVertex3s __glVertex3s
#define glVertex3sv __glVertex3sv
#define glVertex4d __glVertex4d
#define glVertex4dv __glVertex4dv
#define glVertex4f __glVertex4f
#define glVertex4fv __glVertex4fv
#define glVertex4i __glVertex4i
#define glVertex4iv __glVertex4iv
#define glVertex4s __glVertex4s
#define glVertex4sv __glVertex4sv
#define glVertexPointer __glVertexPointer
#define glViewport __glViewport
#endif

#endif /* _MESA_API_WRAP_H_ */
