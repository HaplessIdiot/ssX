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
 * $PI: xc/lib/GL/glx/indirect_wrap.h,v 1.1 1999/03/23 06:20:14 martin Exp $
 */

#ifndef _INDIRECT_WRAP_H_
#define _INDIRECT_WRAP_H_

#if defined(GLX_DIRECT_RENDERING) && defined(NEED_GL_FUNCS_WRAPPED)

/* NOTE: This file could be automatically generated */

#define glAccum __indirect_glAccum
#define glAlphaFunc __indirect_glAlphaFunc
#define glAreTexturesResident __indirect_glAreTexturesResident
#define glArrayElement __indirect_glArrayElement
#define glBegin __indirect_glBegin
#define glBindTexture __indirect_glBindTexture
#define glBitmap __indirect_glBitmap
#define glBlendFunc __indirect_glBlendFunc
#define glCallList __indirect_glCallList
#define glCallLists __indirect_glCallLists
#define glClear __indirect_glClear
#define glClearAccum __indirect_glClearAccum
#define glClearColor __indirect_glClearColor
#define glClearDepth __indirect_glClearDepth
#define glClearIndex __indirect_glClearIndex
#define glClearStencil __indirect_glClearStencil
#define glClipPlane __indirect_glClipPlane
#define glColor3b __indirect_glColor3b
#define glColor3bv __indirect_glColor3bv
#define glColor3d __indirect_glColor3d
#define glColor3dv __indirect_glColor3dv
#define glColor3f __indirect_glColor3f
#define glColor3fv __indirect_glColor3fv
#define glColor3i __indirect_glColor3i
#define glColor3iv __indirect_glColor3iv
#define glColor3s __indirect_glColor3s
#define glColor3sv __indirect_glColor3sv
#define glColor3ub __indirect_glColor3ub
#define glColor3ubv __indirect_glColor3ubv
#define glColor3ui __indirect_glColor3ui
#define glColor3uiv __indirect_glColor3uiv
#define glColor3us __indirect_glColor3us
#define glColor3usv __indirect_glColor3usv
#define glColor4b __indirect_glColor4b
#define glColor4bv __indirect_glColor4bv
#define glColor4d __indirect_glColor4d
#define glColor4dv __indirect_glColor4dv
#define glColor4f __indirect_glColor4f
#define glColor4fv __indirect_glColor4fv
#define glColor4i __indirect_glColor4i
#define glColor4iv __indirect_glColor4iv
#define glColor4s __indirect_glColor4s
#define glColor4sv __indirect_glColor4sv
#define glColor4ub __indirect_glColor4ub
#define glColor4ubv __indirect_glColor4ubv
#define glColor4ui __indirect_glColor4ui
#define glColor4uiv __indirect_glColor4uiv
#define glColor4us __indirect_glColor4us
#define glColor4usv __indirect_glColor4usv
#define glColorMask __indirect_glColorMask
#define glColorMaterial __indirect_glColorMaterial
#define glColorPointer __indirect_glColorPointer
#define glCopyPixels __indirect_glCopyPixels
#define glCopyTexImage1D __indirect_glCopyTexImage1D
#define glCopyTexImage2D __indirect_glCopyTexImage2D
#define glCopyTexSubImage1D __indirect_glCopyTexSubImage1D
#define glCopyTexSubImage2D __indirect_glCopyTexSubImage2D
#define glCullFace __indirect_glCullFace
#define glDeleteLists __indirect_glDeleteLists
#define glDeleteTextures __indirect_glDeleteTextures
#define glDepthFunc __indirect_glDepthFunc
#define glDepthMask __indirect_glDepthMask
#define glDepthRange __indirect_glDepthRange
#define glDisable __indirect_glDisable
#define glDisableClientState __indirect_glDisableClientState
#define glDrawArrays __indirect_glDrawArrays
#define glDrawBuffer __indirect_glDrawBuffer
#define glDrawElements __indirect_glDrawElements
#define glDrawPixels __indirect_glDrawPixels
#define glEdgeFlag __indirect_glEdgeFlag
#define glEdgeFlagPointer __indirect_glEdgeFlagPointer
#define glEdgeFlagv __indirect_glEdgeFlagv
#define glEnable __indirect_glEnable
#define glEnableClientState __indirect_glEnableClientState
#define glEnd __indirect_glEnd
#define glEndList __indirect_glEndList
#define glEvalCoord1d __indirect_glEvalCoord1d
#define glEvalCoord1dv __indirect_glEvalCoord1dv
#define glEvalCoord1f __indirect_glEvalCoord1f
#define glEvalCoord1fv __indirect_glEvalCoord1fv
#define glEvalCoord2d __indirect_glEvalCoord2d
#define glEvalCoord2dv __indirect_glEvalCoord2dv
#define glEvalCoord2f __indirect_glEvalCoord2f
#define glEvalCoord2fv __indirect_glEvalCoord2fv
#define glEvalMesh1 __indirect_glEvalMesh1
#define glEvalMesh2 __indirect_glEvalMesh2
#define glEvalPoint1 __indirect_glEvalPoint1
#define glEvalPoint2 __indirect_glEvalPoint2
#define glFeedbackBuffer __indirect_glFeedbackBuffer
#define glFinish __indirect_glFinish
#define glFlush __indirect_glFlush
#define glFogf __indirect_glFogf
#define glFogfv __indirect_glFogfv
#define glFogi __indirect_glFogi
#define glFogiv __indirect_glFogiv
#define glFrontFace __indirect_glFrontFace
#define glFrustum __indirect_glFrustum
#define glGenLists __indirect_glGenLists
#define glGenTextures __indirect_glGenTextures
#define glGetBooleanv __indirect_glGetBooleanv
#define glGetClipPlane __indirect_glGetClipPlane
#define glGetDoublev __indirect_glGetDoublev
#define glGetError __indirect_glGetError
#define glGetFloatv __indirect_glGetFloatv
#define glGetIntegerv __indirect_glGetIntegerv
#define glGetLightfv __indirect_glGetLightfv
#define glGetLightiv __indirect_glGetLightiv
#define glGetMapdv __indirect_glGetMapdv
#define glGetMapfv __indirect_glGetMapfv
#define glGetMapiv __indirect_glGetMapiv
#define glGetMaterialfv __indirect_glGetMaterialfv
#define glGetMaterialiv __indirect_glGetMaterialiv
#define glGetPixelMapfv __indirect_glGetPixelMapfv
#define glGetPixelMapuiv __indirect_glGetPixelMapuiv
#define glGetPixelMapusv __indirect_glGetPixelMapusv
#define glGetPointerv __indirect_glGetPointerv
#define glGetPolygonStipple __indirect_glGetPolygonStipple
#define glGetString __indirect_glGetString
#define glGetTexEnvfv __indirect_glGetTexEnvfv
#define glGetTexEnviv __indirect_glGetTexEnviv
#define glGetTexGendv __indirect_glGetTexGendv
#define glGetTexGenfv __indirect_glGetTexGenfv
#define glGetTexGeniv __indirect_glGetTexGeniv
#define glGetTexImage __indirect_glGetTexImage
#define glGetTexLevelParameterfv __indirect_glGetTexLevelParameterfv
#define glGetTexLevelParameteriv __indirect_glGetTexLevelParameteriv
#define glGetTexParameterfv __indirect_glGetTexParameterfv
#define glGetTexParameteriv __indirect_glGetTexParameteriv
#define glHint __indirect_glHint
#define glIndexMask __indirect_glIndexMask
#define glIndexPointer __indirect_glIndexPointer
#define glIndexd __indirect_glIndexd
#define glIndexdv __indirect_glIndexdv
#define glIndexf __indirect_glIndexf
#define glIndexfv __indirect_glIndexfv
#define glIndexi __indirect_glIndexi
#define glIndexiv __indirect_glIndexiv
#define glIndexs __indirect_glIndexs
#define glIndexsv __indirect_glIndexsv
#define glIndexub __indirect_glIndexub
#define glIndexubv __indirect_glIndexubv
#define glInitNames __indirect_glInitNames
#define glInterleavedArrays __indirect_glInterleavedArrays
#define glIsEnabled __indirect_glIsEnabled
#define glIsList __indirect_glIsList
#define glIsTexture __indirect_glIsTexture
#define glLightModelf __indirect_glLightModelf
#define glLightModelfv __indirect_glLightModelfv
#define glLightModeli __indirect_glLightModeli
#define glLightModeliv __indirect_glLightModeliv
#define glLightf __indirect_glLightf
#define glLightfv __indirect_glLightfv
#define glLighti __indirect_glLighti
#define glLightiv __indirect_glLightiv
#define glLineStipple __indirect_glLineStipple
#define glLineWidth __indirect_glLineWidth
#define glListBase __indirect_glListBase
#define glLoadIdentity __indirect_glLoadIdentity
#define glLoadMatrixd __indirect_glLoadMatrixd
#define glLoadMatrixf __indirect_glLoadMatrixf
#define glLoadName __indirect_glLoadName
#define glLogicOp __indirect_glLogicOp
#define glMap1d __indirect_glMap1d
#define glMap1f __indirect_glMap1f
#define glMap2d __indirect_glMap2d
#define glMap2f __indirect_glMap2f
#define glMapGrid1d __indirect_glMapGrid1d
#define glMapGrid1f __indirect_glMapGrid1f
#define glMapGrid2d __indirect_glMapGrid2d
#define glMapGrid2f __indirect_glMapGrid2f
#define glMaterialf __indirect_glMaterialf
#define glMaterialfv __indirect_glMaterialfv
#define glMateriali __indirect_glMateriali
#define glMaterialiv __indirect_glMaterialiv
#define glMatrixMode __indirect_glMatrixMode
#define glMultMatrixd __indirect_glMultMatrixd
#define glMultMatrixf __indirect_glMultMatrixf
#define glNewList __indirect_glNewList
#define glNormal3b __indirect_glNormal3b
#define glNormal3bv __indirect_glNormal3bv
#define glNormal3d __indirect_glNormal3d
#define glNormal3dv __indirect_glNormal3dv
#define glNormal3f __indirect_glNormal3f
#define glNormal3fv __indirect_glNormal3fv
#define glNormal3i __indirect_glNormal3i
#define glNormal3iv __indirect_glNormal3iv
#define glNormal3s __indirect_glNormal3s
#define glNormal3sv __indirect_glNormal3sv
#define glNormalPointer __indirect_glNormalPointer
#define glOrtho __indirect_glOrtho
#define glPassThrough __indirect_glPassThrough
#define glPixelMapfv __indirect_glPixelMapfv
#define glPixelMapuiv __indirect_glPixelMapuiv
#define glPixelMapusv __indirect_glPixelMapusv
#define glPixelStoref __indirect_glPixelStoref
#define glPixelStorei __indirect_glPixelStorei
#define glPixelTransferf __indirect_glPixelTransferf
#define glPixelTransferi __indirect_glPixelTransferi
#define glPixelZoom __indirect_glPixelZoom
#define glPointSize __indirect_glPointSize
#define glPolygonMode __indirect_glPolygonMode
#define glPolygonOffset __indirect_glPolygonOffset
#define glPolygonStipple __indirect_glPolygonStipple
#define glPopAttrib __indirect_glPopAttrib
#define glPopClientAttrib __indirect_glPopClientAttrib
#define glPopMatrix __indirect_glPopMatrix
#define glPopName __indirect_glPopName
#define glPrioritizeTextures __indirect_glPrioritizeTextures
#define glPushAttrib __indirect_glPushAttrib
#define glPushClientAttrib __indirect_glPushClientAttrib
#define glPushMatrix __indirect_glPushMatrix
#define glPushName __indirect_glPushName
#define glRasterPos2d __indirect_glRasterPos2d
#define glRasterPos2dv __indirect_glRasterPos2dv
#define glRasterPos2f __indirect_glRasterPos2f
#define glRasterPos2fv __indirect_glRasterPos2fv
#define glRasterPos2i __indirect_glRasterPos2i
#define glRasterPos2iv __indirect_glRasterPos2iv
#define glRasterPos2s __indirect_glRasterPos2s
#define glRasterPos2sv __indirect_glRasterPos2sv
#define glRasterPos3d __indirect_glRasterPos3d
#define glRasterPos3dv __indirect_glRasterPos3dv
#define glRasterPos3f __indirect_glRasterPos3f
#define glRasterPos3fv __indirect_glRasterPos3fv
#define glRasterPos3i __indirect_glRasterPos3i
#define glRasterPos3iv __indirect_glRasterPos3iv
#define glRasterPos3s __indirect_glRasterPos3s
#define glRasterPos3sv __indirect_glRasterPos3sv
#define glRasterPos4d __indirect_glRasterPos4d
#define glRasterPos4dv __indirect_glRasterPos4dv
#define glRasterPos4f __indirect_glRasterPos4f
#define glRasterPos4fv __indirect_glRasterPos4fv
#define glRasterPos4i __indirect_glRasterPos4i
#define glRasterPos4iv __indirect_glRasterPos4iv
#define glRasterPos4s __indirect_glRasterPos4s
#define glRasterPos4sv __indirect_glRasterPos4sv
#define glReadBuffer __indirect_glReadBuffer
#define glReadPixels __indirect_glReadPixels
#define glRectd __indirect_glRectd
#define glRectdv __indirect_glRectdv
#define glRectf __indirect_glRectf
#define glRectfv __indirect_glRectfv
#define glRecti __indirect_glRecti
#define glRectiv __indirect_glRectiv
#define glRects __indirect_glRects
#define glRectsv __indirect_glRectsv
#define glRenderMode __indirect_glRenderMode
#define glRotated __indirect_glRotated
#define glRotatef __indirect_glRotatef
#define glScaled __indirect_glScaled
#define glScalef __indirect_glScalef
#define glScissor __indirect_glScissor
#define glSelectBuffer __indirect_glSelectBuffer
#define glShadeModel __indirect_glShadeModel
#define glStencilFunc __indirect_glStencilFunc
#define glStencilMask __indirect_glStencilMask
#define glStencilOp __indirect_glStencilOp
#define glTexCoord1d __indirect_glTexCoord1d
#define glTexCoord1dv __indirect_glTexCoord1dv
#define glTexCoord1f __indirect_glTexCoord1f
#define glTexCoord1fv __indirect_glTexCoord1fv
#define glTexCoord1i __indirect_glTexCoord1i
#define glTexCoord1iv __indirect_glTexCoord1iv
#define glTexCoord1s __indirect_glTexCoord1s
#define glTexCoord1sv __indirect_glTexCoord1sv
#define glTexCoord2d __indirect_glTexCoord2d
#define glTexCoord2dv __indirect_glTexCoord2dv
#define glTexCoord2f __indirect_glTexCoord2f
#define glTexCoord2fv __indirect_glTexCoord2fv
#define glTexCoord2i __indirect_glTexCoord2i
#define glTexCoord2iv __indirect_glTexCoord2iv
#define glTexCoord2s __indirect_glTexCoord2s
#define glTexCoord2sv __indirect_glTexCoord2sv
#define glTexCoord3d __indirect_glTexCoord3d
#define glTexCoord3dv __indirect_glTexCoord3dv
#define glTexCoord3f __indirect_glTexCoord3f
#define glTexCoord3fv __indirect_glTexCoord3fv
#define glTexCoord3i __indirect_glTexCoord3i
#define glTexCoord3iv __indirect_glTexCoord3iv
#define glTexCoord3s __indirect_glTexCoord3s
#define glTexCoord3sv __indirect_glTexCoord3sv
#define glTexCoord4d __indirect_glTexCoord4d
#define glTexCoord4dv __indirect_glTexCoord4dv
#define glTexCoord4f __indirect_glTexCoord4f
#define glTexCoord4fv __indirect_glTexCoord4fv
#define glTexCoord4i __indirect_glTexCoord4i
#define glTexCoord4iv __indirect_glTexCoord4iv
#define glTexCoord4s __indirect_glTexCoord4s
#define glTexCoord4sv __indirect_glTexCoord4sv
#define glTexCoordPointer __indirect_glTexCoordPointer
#define glTexEnvf __indirect_glTexEnvf
#define glTexEnvfv __indirect_glTexEnvfv
#define glTexEnvi __indirect_glTexEnvi
#define glTexEnviv __indirect_glTexEnviv
#define glTexGend __indirect_glTexGend
#define glTexGendv __indirect_glTexGendv
#define glTexGenf __indirect_glTexGenf
#define glTexGenfv __indirect_glTexGenfv
#define glTexGeni __indirect_glTexGeni
#define glTexGeniv __indirect_glTexGeniv
#define glTexImage1D __indirect_glTexImage1D
#define glTexImage2D __indirect_glTexImage2D
#define glTexParameterf __indirect_glTexParameterf
#define glTexParameterfv __indirect_glTexParameterfv
#define glTexParameteri __indirect_glTexParameteri
#define glTexParameteriv __indirect_glTexParameteriv
#define glTexSubImage1D __indirect_glTexSubImage1D
#define glTexSubImage2D __indirect_glTexSubImage2D
#define glTranslated __indirect_glTranslated
#define glTranslatef __indirect_glTranslatef
#define glVertex2d __indirect_glVertex2d
#define glVertex2dv __indirect_glVertex2dv
#define glVertex2f __indirect_glVertex2f
#define glVertex2fv __indirect_glVertex2fv
#define glVertex2i __indirect_glVertex2i
#define glVertex2iv __indirect_glVertex2iv
#define glVertex2s __indirect_glVertex2s
#define glVertex2sv __indirect_glVertex2sv
#define glVertex3d __indirect_glVertex3d
#define glVertex3dv __indirect_glVertex3dv
#define glVertex3f __indirect_glVertex3f
#define glVertex3fv __indirect_glVertex3fv
#define glVertex3i __indirect_glVertex3i
#define glVertex3iv __indirect_glVertex3iv
#define glVertex3s __indirect_glVertex3s
#define glVertex3sv __indirect_glVertex3sv
#define glVertex4d __indirect_glVertex4d
#define glVertex4dv __indirect_glVertex4dv
#define glVertex4f __indirect_glVertex4f
#define glVertex4fv __indirect_glVertex4fv
#define glVertex4i __indirect_glVertex4i
#define glVertex4iv __indirect_glVertex4iv
#define glVertex4s __indirect_glVertex4s
#define glVertex4sv __indirect_glVertex4sv
#define glVertexPointer __indirect_glVertexPointer
#define glViewport __indirect_glViewport

#endif
#endif /* _INDIRECT_WRAP_H_ */
