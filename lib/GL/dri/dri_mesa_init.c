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
 * $PI: xc/lib/GL/dri/dri_mesa_init.c,v 1.2 1999/04/05 05:24:31 martin Exp $
 */

#ifdef GLX_DIRECT_RENDERING

#define USE_GAMMA 1

#ifdef GLX_USE_DL
#include <stdio.h>
#include <dlfcn.h>
#endif
#if USE_GAMMA
#include "gamma_gl.h"
#include "gamma_xmesa.h"
#else
#include "mesa_api.h"
#endif
#include "dri_mesa.h"
#include "dri_glapi.h"
#include "dri_xmesaapi.h"
#include "dri_mesa_init.h"


#ifdef GLX_USE_DL
typedef void (*voidFunc)(void);

static voidFunc resolveSymbol(void *handle, char *name)
{
    char *error;
    voidFunc func;

    func = dlsym(handle, name);
    if ((error = dlerror()) != NULL)
	fputs(error, stderr);

    return func;
}
#endif

void *driMesaInitAPI(char *name, __XMESAapi *XMesaAPI, __GLapi *glAPI)
{
    void *handle = (void *)NULL;
#ifdef GLX_USE_DL
    char libname[256] = "generic.so";

    if (name)
	snprintf(libname, 256, "%s.so", name);
    handle = dlopen(libname, RTLD_NOW);
    if (!handle) {
	fputs(dlerror(), stderr);
	return NULL;
    }

    XMesaAPI->InitDriver = resolveSymbol(handle, "XMesaInitDriver");
    XMesaAPI->ResetDriver = resolveSymbol(handle, "XMesaResetDriver");
    XMesaAPI->CreateVisual = resolveSymbol(handle, "XMesaCreateVisual");
    XMesaAPI->DestroyVisual = resolveSymbol(handle, "XMesaDestroyVisual");
    XMesaAPI->CreateContext = resolveSymbol(handle, "XMesaCreateContext");
    XMesaAPI->DestroyContext = resolveSymbol(handle, "XMesaDestroyContext");
    XMesaAPI->CreateWindowBuffer = resolveSymbol(handle,
						 "XMesaCreateWindowBuffer");
    XMesaAPI->CreatePixmapBuffer = resolveSymbol(handle,
						 "XMesaCreatePixmapBuffer");
    XMesaAPI->DestroyBuffer = resolveSymbol(handle, "XMesaDestroyBuffer");
    XMesaAPI->SwapBuffers = resolveSymbol(handle, "XMesaSwapBuffers");
    XMesaAPI->MakeCurrent = resolveSymbol(handle, "XMesaMakeCurrent");

    /* ...... insert all of the glAPI funcs here ....... */
#else

#if USE_GAMMA

/*
** The "__GAMMA_" prefix should be removed after we implement dynamic
** loading in the DRI.
*/

    XMesaAPI->InitDriver = __GAMMA_XMesaInitDriver;
    XMesaAPI->ResetDriver = __GAMMA_XMesaResetDriver;
    XMesaAPI->CreateVisual = __GAMMA_XMesaCreateVisual;
    XMesaAPI->DestroyVisual = __GAMMA_XMesaDestroyVisual;
    XMesaAPI->CreateContext = __GAMMA_XMesaCreateContext;
    XMesaAPI->DestroyContext = __GAMMA_XMesaDestroyContext;
    XMesaAPI->CreateWindowBuffer = __GAMMA_XMesaCreateWindowBuffer;
    XMesaAPI->CreatePixmapBuffer = __GAMMA_XMesaCreatePixmapBuffer;
    XMesaAPI->DestroyBuffer = __GAMMA_XMesaDestroyBuffer;
    XMesaAPI->SwapBuffers = __GAMMA_XMesaSwapBuffers;
    XMesaAPI->MakeCurrent = __GAMMA_XMesaMakeCurrent;

    glAPI->Accum = __GAMMA_glAccum;
    glAPI->AlphaFunc = __GAMMA_glAlphaFunc;
    glAPI->AreTexturesResident = __GAMMA_glAreTexturesResident;
    glAPI->ArrayElement = __GAMMA_glArrayElement;
    glAPI->Begin = __GAMMA_glBegin;
    glAPI->BindTexture = __GAMMA_glBindTexture;
    glAPI->Bitmap = __GAMMA_glBitmap;
    glAPI->BlendFunc = __GAMMA_glBlendFunc;
    glAPI->CallList = __GAMMA_glCallList;
    glAPI->CallLists = __GAMMA_glCallLists;
    glAPI->Clear = __GAMMA_glClear;
    glAPI->ClearAccum = __GAMMA_glClearAccum;
    glAPI->ClearColor = __GAMMA_glClearColor;
    glAPI->ClearDepth = __GAMMA_glClearDepth;
    glAPI->ClearIndex = __GAMMA_glClearIndex;
    glAPI->ClearStencil = __GAMMA_glClearStencil;
    glAPI->ClipPlane = __GAMMA_glClipPlane;
    glAPI->Color3b = __GAMMA_glColor3b;
    glAPI->Color3bv = __GAMMA_glColor3bv;
    glAPI->Color3d = __GAMMA_glColor3d;
    glAPI->Color3dv = __GAMMA_glColor3dv;
    glAPI->Color3f = __GAMMA_glColor3f;
    glAPI->Color3fv = __GAMMA_glColor3fv;
    glAPI->Color3i = __GAMMA_glColor3i;
    glAPI->Color3iv = __GAMMA_glColor3iv;
    glAPI->Color3s = __GAMMA_glColor3s;
    glAPI->Color3sv = __GAMMA_glColor3sv;
    glAPI->Color3ub = __GAMMA_glColor3ub;
    glAPI->Color3ubv = __GAMMA_glColor3ubv;
    glAPI->Color3ui = __GAMMA_glColor3ui;
    glAPI->Color3uiv = __GAMMA_glColor3uiv;
    glAPI->Color3us = __GAMMA_glColor3us;
    glAPI->Color3usv = __GAMMA_glColor3usv;
    glAPI->Color4b = __GAMMA_glColor4b;
    glAPI->Color4bv = __GAMMA_glColor4bv;
    glAPI->Color4d = __GAMMA_glColor4d;
    glAPI->Color4dv = __GAMMA_glColor4dv;
    glAPI->Color4f = __GAMMA_glColor4f;
    glAPI->Color4fv = __GAMMA_glColor4fv;
    glAPI->Color4i = __GAMMA_glColor4i;
    glAPI->Color4iv = __GAMMA_glColor4iv;
    glAPI->Color4s = __GAMMA_glColor4s;
    glAPI->Color4sv = __GAMMA_glColor4sv;
    glAPI->Color4ub = __GAMMA_glColor4ub;
    glAPI->Color4ubv = __GAMMA_glColor4ubv;
    glAPI->Color4ui = __GAMMA_glColor4ui;
    glAPI->Color4uiv = __GAMMA_glColor4uiv;
    glAPI->Color4us = __GAMMA_glColor4us;
    glAPI->Color4usv = __GAMMA_glColor4usv;
    glAPI->ColorMask = __GAMMA_glColorMask;
    glAPI->ColorMaterial = __GAMMA_glColorMaterial;
    glAPI->ColorPointer = __GAMMA_glColorPointer;
    glAPI->CopyPixels = __GAMMA_glCopyPixels;
    glAPI->CopyTexImage1D = __GAMMA_glCopyTexImage1D;
    glAPI->CopyTexImage2D = __GAMMA_glCopyTexImage2D;
    glAPI->CopyTexSubImage1D = __GAMMA_glCopyTexSubImage1D;
    glAPI->CopyTexSubImage2D = __GAMMA_glCopyTexSubImage2D;
    glAPI->CullFace = __GAMMA_glCullFace;
    glAPI->DeleteLists = __GAMMA_glDeleteLists;
    glAPI->DeleteTextures = __GAMMA_glDeleteTextures;
    glAPI->DepthFunc = __GAMMA_glDepthFunc;
    glAPI->DepthMask = __GAMMA_glDepthMask;
    glAPI->DepthRange = __GAMMA_glDepthRange;
    glAPI->Disable = __GAMMA_glDisable;
    glAPI->DisableClientState = __GAMMA_glDisableClientState;
    glAPI->DrawArrays = __GAMMA_glDrawArrays;
    glAPI->DrawBuffer = __GAMMA_glDrawBuffer;
    glAPI->DrawElements = __GAMMA_glDrawElements;
    glAPI->DrawPixels = __GAMMA_glDrawPixels;
    glAPI->EdgeFlag = __GAMMA_glEdgeFlag;
    glAPI->EdgeFlagPointer = __GAMMA_glEdgeFlagPointer;
    glAPI->EdgeFlagv = __GAMMA_glEdgeFlagv;
    glAPI->Enable = __GAMMA_glEnable;
    glAPI->EnableClientState = __GAMMA_glEnableClientState;
    glAPI->End = __GAMMA_glEnd;
    glAPI->EndList = __GAMMA_glEndList;
    glAPI->EvalCoord1d = __GAMMA_glEvalCoord1d;
    glAPI->EvalCoord1dv = __GAMMA_glEvalCoord1dv;
    glAPI->EvalCoord1f = __GAMMA_glEvalCoord1f;
    glAPI->EvalCoord1fv = __GAMMA_glEvalCoord1fv;
    glAPI->EvalCoord2d = __GAMMA_glEvalCoord2d;
    glAPI->EvalCoord2dv = __GAMMA_glEvalCoord2dv;
    glAPI->EvalCoord2f = __GAMMA_glEvalCoord2f;
    glAPI->EvalCoord2fv = __GAMMA_glEvalCoord2fv;
    glAPI->EvalMesh1 = __GAMMA_glEvalMesh1;
    glAPI->EvalMesh2 = __GAMMA_glEvalMesh2;
    glAPI->EvalPoint1 = __GAMMA_glEvalPoint1;
    glAPI->EvalPoint2 = __GAMMA_glEvalPoint2;
    glAPI->FeedbackBuffer = __GAMMA_glFeedbackBuffer;
    glAPI->Finish = __GAMMA_glFinish;
    glAPI->Flush = __GAMMA_glFlush;
    glAPI->Fogf = __GAMMA_glFogf;
    glAPI->Fogfv = __GAMMA_glFogfv;
    glAPI->Fogi = __GAMMA_glFogi;
    glAPI->Fogiv = __GAMMA_glFogiv;
    glAPI->FrontFace = __GAMMA_glFrontFace;
    glAPI->Frustum = __GAMMA_glFrustum;
    glAPI->GenLists = __GAMMA_glGenLists;
    glAPI->GenTextures = __GAMMA_glGenTextures;
    glAPI->GetBooleanv = __GAMMA_glGetBooleanv;
    glAPI->GetClipPlane = __GAMMA_glGetClipPlane;
    glAPI->GetDoublev = __GAMMA_glGetDoublev;
    glAPI->GetError = __GAMMA_glGetError;
    glAPI->GetFloatv = __GAMMA_glGetFloatv;
    glAPI->GetIntegerv = __GAMMA_glGetIntegerv;
    glAPI->GetLightfv = __GAMMA_glGetLightfv;
    glAPI->GetLightiv = __GAMMA_glGetLightiv;
    glAPI->GetMapdv = __GAMMA_glGetMapdv;
    glAPI->GetMapfv = __GAMMA_glGetMapfv;
    glAPI->GetMapiv = __GAMMA_glGetMapiv;
    glAPI->GetMaterialfv = __GAMMA_glGetMaterialfv;
    glAPI->GetMaterialiv = __GAMMA_glGetMaterialiv;
    glAPI->GetPixelMapfv = __GAMMA_glGetPixelMapfv;
    glAPI->GetPixelMapuiv = __GAMMA_glGetPixelMapuiv;
    glAPI->GetPixelMapusv = __GAMMA_glGetPixelMapusv;
    glAPI->GetPointerv = __GAMMA_glGetPointerv;
    glAPI->GetPolygonStipple = __GAMMA_glGetPolygonStipple;
    glAPI->GetString = __GAMMA_glGetString;
    glAPI->GetTexEnvfv = __GAMMA_glGetTexEnvfv;
    glAPI->GetTexEnviv = __GAMMA_glGetTexEnviv;
    glAPI->GetTexGendv = __GAMMA_glGetTexGendv;
    glAPI->GetTexGenfv = __GAMMA_glGetTexGenfv;
    glAPI->GetTexGeniv = __GAMMA_glGetTexGeniv;
    glAPI->GetTexImage = __GAMMA_glGetTexImage;
    glAPI->GetTexLevelParameterfv = __GAMMA_glGetTexLevelParameterfv;
    glAPI->GetTexLevelParameteriv = __GAMMA_glGetTexLevelParameteriv;
    glAPI->GetTexParameterfv = __GAMMA_glGetTexParameterfv;
    glAPI->GetTexParameteriv = __GAMMA_glGetTexParameteriv;
    glAPI->Hint = __GAMMA_glHint;
    glAPI->IndexMask = __GAMMA_glIndexMask;
    glAPI->IndexPointer = __GAMMA_glIndexPointer;
    glAPI->Indexd = __GAMMA_glIndexd;
    glAPI->Indexdv = __GAMMA_glIndexdv;
    glAPI->Indexf = __GAMMA_glIndexf;
    glAPI->Indexfv = __GAMMA_glIndexfv;
    glAPI->Indexi = __GAMMA_glIndexi;
    glAPI->Indexiv = __GAMMA_glIndexiv;
    glAPI->Indexs = __GAMMA_glIndexs;
    glAPI->Indexsv = __GAMMA_glIndexsv;
    glAPI->Indexub = __GAMMA_glIndexub;
    glAPI->Indexubv = __GAMMA_glIndexubv;
    glAPI->InitNames = __GAMMA_glInitNames;
    glAPI->InterleavedArrays = __GAMMA_glInterleavedArrays;
    glAPI->IsEnabled = __GAMMA_glIsEnabled;
    glAPI->IsList = __GAMMA_glIsList;
    glAPI->IsTexture = __GAMMA_glIsTexture;
    glAPI->LightModelf = __GAMMA_glLightModelf;
    glAPI->LightModelfv = __GAMMA_glLightModelfv;
    glAPI->LightModeli = __GAMMA_glLightModeli;
    glAPI->LightModeliv = __GAMMA_glLightModeliv;
    glAPI->Lightf = __GAMMA_glLightf;
    glAPI->Lightfv = __GAMMA_glLightfv;
    glAPI->Lighti = __GAMMA_glLighti;
    glAPI->Lightiv = __GAMMA_glLightiv;
    glAPI->LineStipple = __GAMMA_glLineStipple;
    glAPI->LineWidth = __GAMMA_glLineWidth;
    glAPI->ListBase = __GAMMA_glListBase;
    glAPI->LoadIdentity = __GAMMA_glLoadIdentity;
    glAPI->LoadMatrixd = __GAMMA_glLoadMatrixd;
    glAPI->LoadMatrixf = __GAMMA_glLoadMatrixf;
    glAPI->LoadName = __GAMMA_glLoadName;
    glAPI->LogicOp = __GAMMA_glLogicOp;
    glAPI->Map1d = __GAMMA_glMap1d;
    glAPI->Map1f = __GAMMA_glMap1f;
    glAPI->Map2d = __GAMMA_glMap2d;
    glAPI->Map2f = __GAMMA_glMap2f;
    glAPI->MapGrid1d = __GAMMA_glMapGrid1d;
    glAPI->MapGrid1f = __GAMMA_glMapGrid1f;
    glAPI->MapGrid2d = __GAMMA_glMapGrid2d;
    glAPI->MapGrid2f = __GAMMA_glMapGrid2f;
    glAPI->Materialf = __GAMMA_glMaterialf;
    glAPI->Materialfv = __GAMMA_glMaterialfv;
    glAPI->Materiali = __GAMMA_glMateriali;
    glAPI->Materialiv = __GAMMA_glMaterialiv;
    glAPI->MatrixMode = __GAMMA_glMatrixMode;
    glAPI->MultMatrixd = __GAMMA_glMultMatrixd;
    glAPI->MultMatrixf = __GAMMA_glMultMatrixf;
    glAPI->NewList = __GAMMA_glNewList;
    glAPI->Normal3b = __GAMMA_glNormal3b;
    glAPI->Normal3bv = __GAMMA_glNormal3bv;
    glAPI->Normal3d = __GAMMA_glNormal3d;
    glAPI->Normal3dv = __GAMMA_glNormal3dv;
    glAPI->Normal3f = __GAMMA_glNormal3f;
    glAPI->Normal3fv = __GAMMA_glNormal3fv;
    glAPI->Normal3i = __GAMMA_glNormal3i;
    glAPI->Normal3iv = __GAMMA_glNormal3iv;
    glAPI->Normal3s = __GAMMA_glNormal3s;
    glAPI->Normal3sv = __GAMMA_glNormal3sv;
    glAPI->NormalPointer = __GAMMA_glNormalPointer;
    glAPI->Ortho = __GAMMA_glOrtho;
    glAPI->PassThrough = __GAMMA_glPassThrough;
    glAPI->PixelMapfv = __GAMMA_glPixelMapfv;
    glAPI->PixelMapuiv = __GAMMA_glPixelMapuiv;
    glAPI->PixelMapusv = __GAMMA_glPixelMapusv;
    glAPI->PixelStoref = __GAMMA_glPixelStoref;
    glAPI->PixelStorei = __GAMMA_glPixelStorei;
    glAPI->PixelTransferf = __GAMMA_glPixelTransferf;
    glAPI->PixelTransferi = __GAMMA_glPixelTransferi;
    glAPI->PixelZoom = __GAMMA_glPixelZoom;
    glAPI->PointSize = __GAMMA_glPointSize;
    glAPI->PolygonMode = __GAMMA_glPolygonMode;
    glAPI->PolygonOffset = __GAMMA_glPolygonOffset;
    glAPI->PolygonStipple = __GAMMA_glPolygonStipple;
    glAPI->PopAttrib = __GAMMA_glPopAttrib;
    glAPI->PopClientAttrib = __GAMMA_glPopClientAttrib;
    glAPI->PopMatrix = __GAMMA_glPopMatrix;
    glAPI->PopName = __GAMMA_glPopName;
    glAPI->PrioritizeTextures = __GAMMA_glPrioritizeTextures;
    glAPI->PushAttrib = __GAMMA_glPushAttrib;
    glAPI->PushClientAttrib = __GAMMA_glPushClientAttrib;
    glAPI->PushMatrix = __GAMMA_glPushMatrix;
    glAPI->PushName = __GAMMA_glPushName;
    glAPI->RasterPos2d = __GAMMA_glRasterPos2d;
    glAPI->RasterPos2dv = __GAMMA_glRasterPos2dv;
    glAPI->RasterPos2f = __GAMMA_glRasterPos2f;
    glAPI->RasterPos2fv = __GAMMA_glRasterPos2fv;
    glAPI->RasterPos2i = __GAMMA_glRasterPos2i;
    glAPI->RasterPos2iv = __GAMMA_glRasterPos2iv;
    glAPI->RasterPos2s = __GAMMA_glRasterPos2s;
    glAPI->RasterPos2sv = __GAMMA_glRasterPos2sv;
    glAPI->RasterPos3d = __GAMMA_glRasterPos3d;
    glAPI->RasterPos3dv = __GAMMA_glRasterPos3dv;
    glAPI->RasterPos3f = __GAMMA_glRasterPos3f;
    glAPI->RasterPos3fv = __GAMMA_glRasterPos3fv;
    glAPI->RasterPos3i = __GAMMA_glRasterPos3i;
    glAPI->RasterPos3iv = __GAMMA_glRasterPos3iv;
    glAPI->RasterPos3s = __GAMMA_glRasterPos3s;
    glAPI->RasterPos3sv = __GAMMA_glRasterPos3sv;
    glAPI->RasterPos4d = __GAMMA_glRasterPos4d;
    glAPI->RasterPos4dv = __GAMMA_glRasterPos4dv;
    glAPI->RasterPos4f = __GAMMA_glRasterPos4f;
    glAPI->RasterPos4fv = __GAMMA_glRasterPos4fv;
    glAPI->RasterPos4i = __GAMMA_glRasterPos4i;
    glAPI->RasterPos4iv = __GAMMA_glRasterPos4iv;
    glAPI->RasterPos4s = __GAMMA_glRasterPos4s;
    glAPI->RasterPos4sv = __GAMMA_glRasterPos4sv;
    glAPI->ReadBuffer = __GAMMA_glReadBuffer;
    glAPI->ReadPixels = __GAMMA_glReadPixels;
    glAPI->Rectd = __GAMMA_glRectd;
    glAPI->Rectdv = __GAMMA_glRectdv;
    glAPI->Rectf = __GAMMA_glRectf;
    glAPI->Rectfv = __GAMMA_glRectfv;
    glAPI->Recti = __GAMMA_glRecti;
    glAPI->Rectiv = __GAMMA_glRectiv;
    glAPI->Rects = __GAMMA_glRects;
    glAPI->Rectsv = __GAMMA_glRectsv;
    glAPI->RenderMode = __GAMMA_glRenderMode;
    glAPI->Rotated = __GAMMA_glRotated;
    glAPI->Rotatef = __GAMMA_glRotatef;
    glAPI->Scaled = __GAMMA_glScaled;
    glAPI->Scalef = __GAMMA_glScalef;
    glAPI->Scissor = __GAMMA_glScissor;
    glAPI->SelectBuffer = __GAMMA_glSelectBuffer;
    glAPI->ShadeModel = __GAMMA_glShadeModel;
    glAPI->StencilFunc = __GAMMA_glStencilFunc;
    glAPI->StencilMask = __GAMMA_glStencilMask;
    glAPI->StencilOp = __GAMMA_glStencilOp;
    glAPI->TexCoord1d = __GAMMA_glTexCoord1d;
    glAPI->TexCoord1dv = __GAMMA_glTexCoord1dv;
    glAPI->TexCoord1f = __GAMMA_glTexCoord1f;
    glAPI->TexCoord1fv = __GAMMA_glTexCoord1fv;
    glAPI->TexCoord1i = __GAMMA_glTexCoord1i;
    glAPI->TexCoord1iv = __GAMMA_glTexCoord1iv;
    glAPI->TexCoord1s = __GAMMA_glTexCoord1s;
    glAPI->TexCoord1sv = __GAMMA_glTexCoord1sv;
    glAPI->TexCoord2d = __GAMMA_glTexCoord2d;
    glAPI->TexCoord2dv = __GAMMA_glTexCoord2dv;
    glAPI->TexCoord2f = __GAMMA_glTexCoord2f;
    glAPI->TexCoord2fv = __GAMMA_glTexCoord2fv;
    glAPI->TexCoord2i = __GAMMA_glTexCoord2i;
    glAPI->TexCoord2iv = __GAMMA_glTexCoord2iv;
    glAPI->TexCoord2s = __GAMMA_glTexCoord2s;
    glAPI->TexCoord2sv = __GAMMA_glTexCoord2sv;
    glAPI->TexCoord3d = __GAMMA_glTexCoord3d;
    glAPI->TexCoord3dv = __GAMMA_glTexCoord3dv;
    glAPI->TexCoord3f = __GAMMA_glTexCoord3f;
    glAPI->TexCoord3fv = __GAMMA_glTexCoord3fv;
    glAPI->TexCoord3i = __GAMMA_glTexCoord3i;
    glAPI->TexCoord3iv = __GAMMA_glTexCoord3iv;
    glAPI->TexCoord3s = __GAMMA_glTexCoord3s;
    glAPI->TexCoord3sv = __GAMMA_glTexCoord3sv;
    glAPI->TexCoord4d = __GAMMA_glTexCoord4d;
    glAPI->TexCoord4dv = __GAMMA_glTexCoord4dv;
    glAPI->TexCoord4f = __GAMMA_glTexCoord4f;
    glAPI->TexCoord4fv = __GAMMA_glTexCoord4fv;
    glAPI->TexCoord4i = __GAMMA_glTexCoord4i;
    glAPI->TexCoord4iv = __GAMMA_glTexCoord4iv;
    glAPI->TexCoord4s = __GAMMA_glTexCoord4s;
    glAPI->TexCoord4sv = __GAMMA_glTexCoord4sv;
    glAPI->TexCoordPointer = __GAMMA_glTexCoordPointer;
    glAPI->TexEnvf = __GAMMA_glTexEnvf;
    glAPI->TexEnvfv = __GAMMA_glTexEnvfv;
    glAPI->TexEnvi = __GAMMA_glTexEnvi;
    glAPI->TexEnviv = __GAMMA_glTexEnviv;
    glAPI->TexGend = __GAMMA_glTexGend;
    glAPI->TexGendv = __GAMMA_glTexGendv;
    glAPI->TexGenf = __GAMMA_glTexGenf;
    glAPI->TexGenfv = __GAMMA_glTexGenfv;
    glAPI->TexGeni = __GAMMA_glTexGeni;
    glAPI->TexGeniv = __GAMMA_glTexGeniv;
    glAPI->TexImage1D = __GAMMA_glTexImage1D;
    glAPI->TexImage2D = __GAMMA_glTexImage2D;
    glAPI->TexParameterf = __GAMMA_glTexParameterf;
    glAPI->TexParameterfv = __GAMMA_glTexParameterfv;
    glAPI->TexParameteri = __GAMMA_glTexParameteri;
    glAPI->TexParameteriv = __GAMMA_glTexParameteriv;
    glAPI->TexSubImage1D = __GAMMA_glTexSubImage1D;
    glAPI->TexSubImage2D = __GAMMA_glTexSubImage2D;
    glAPI->Translated = __GAMMA_glTranslated;
    glAPI->Translatef = __GAMMA_glTranslatef;
    glAPI->Vertex2d = __GAMMA_glVertex2d;
    glAPI->Vertex2dv = __GAMMA_glVertex2dv;
    glAPI->Vertex2f = __GAMMA_glVertex2f;
    glAPI->Vertex2fv = __GAMMA_glVertex2fv;
    glAPI->Vertex2i = __GAMMA_glVertex2i;
    glAPI->Vertex2iv = __GAMMA_glVertex2iv;
    glAPI->Vertex2s = __GAMMA_glVertex2s;
    glAPI->Vertex2sv = __GAMMA_glVertex2sv;
    glAPI->Vertex3d = __GAMMA_glVertex3d;
    glAPI->Vertex3dv = __GAMMA_glVertex3dv;
    glAPI->Vertex3f = __GAMMA_glVertex3f;
    glAPI->Vertex3fv = __GAMMA_glVertex3fv;
    glAPI->Vertex3i = __GAMMA_glVertex3i;
    glAPI->Vertex3iv = __GAMMA_glVertex3iv;
    glAPI->Vertex3s = __GAMMA_glVertex3s;
    glAPI->Vertex3sv = __GAMMA_glVertex3sv;
    glAPI->Vertex4d = __GAMMA_glVertex4d;
    glAPI->Vertex4dv = __GAMMA_glVertex4dv;
    glAPI->Vertex4f = __GAMMA_glVertex4f;
    glAPI->Vertex4fv = __GAMMA_glVertex4fv;
    glAPI->Vertex4i = __GAMMA_glVertex4i;
    glAPI->Vertex4iv = __GAMMA_glVertex4iv;
    glAPI->Vertex4s = __GAMMA_glVertex4s;
    glAPI->Vertex4sv = __GAMMA_glVertex4sv;
    glAPI->VertexPointer = __GAMMA_glVertexPointer;
    glAPI->Viewport = __GAMMA_glViewport;
#else

/*
** The "__MESA_SW_" prefix should be removed after we implement dynamic
** loading in the DRI.
*/

    XMesaAPI->InitDriver = NULL;
    XMesaAPI->ResetDriver = NULL;
    XMesaAPI->CreateVisual = XMesaCreateVisual;
    XMesaAPI->DestroyVisual = XMesaDestroyVisual;
    XMesaAPI->CreateContext = XMesaCreateContext;
    XMesaAPI->DestroyContext = XMesaDestroyContext;
    XMesaAPI->CreateWindowBuffer = XMesaCreateWindowBuffer;
    XMesaAPI->CreatePixmapBuffer = XMesaCreatePixmapBuffer;
    XMesaAPI->DestroyBuffer = XMesaDestroyBuffer;
    XMesaAPI->SwapBuffers = XMesaSwapBuffers;
    XMesaAPI->MakeCurrent = XMesaMakeCurrent;

    glAPI->Accum = __MESA_SW_glAccum;
    glAPI->AlphaFunc = __MESA_SW_glAlphaFunc;
    glAPI->AreTexturesResident = __MESA_SW_glAreTexturesResident;
    glAPI->ArrayElement = __MESA_SW_glArrayElement;
    glAPI->Begin = __MESA_SW_glBegin;
    glAPI->BindTexture = __MESA_SW_glBindTexture;
    glAPI->Bitmap = __MESA_SW_glBitmap;
    glAPI->BlendFunc = __MESA_SW_glBlendFunc;
    glAPI->CallList = __MESA_SW_glCallList;
    glAPI->CallLists = __MESA_SW_glCallLists;
    glAPI->Clear = __MESA_SW_glClear;
    glAPI->ClearAccum = __MESA_SW_glClearAccum;
    glAPI->ClearColor = __MESA_SW_glClearColor;
    glAPI->ClearDepth = __MESA_SW_glClearDepth;
    glAPI->ClearIndex = __MESA_SW_glClearIndex;
    glAPI->ClearStencil = __MESA_SW_glClearStencil;
    glAPI->ClipPlane = __MESA_SW_glClipPlane;
    glAPI->Color3b = __MESA_SW_glColor3b;
    glAPI->Color3bv = __MESA_SW_glColor3bv;
    glAPI->Color3d = __MESA_SW_glColor3d;
    glAPI->Color3dv = __MESA_SW_glColor3dv;
    glAPI->Color3f = __MESA_SW_glColor3f;
    glAPI->Color3fv = __MESA_SW_glColor3fv;
    glAPI->Color3i = __MESA_SW_glColor3i;
    glAPI->Color3iv = __MESA_SW_glColor3iv;
    glAPI->Color3s = __MESA_SW_glColor3s;
    glAPI->Color3sv = __MESA_SW_glColor3sv;
    glAPI->Color3ub = __MESA_SW_glColor3ub;
    glAPI->Color3ubv = __MESA_SW_glColor3ubv;
    glAPI->Color3ui = __MESA_SW_glColor3ui;
    glAPI->Color3uiv = __MESA_SW_glColor3uiv;
    glAPI->Color3us = __MESA_SW_glColor3us;
    glAPI->Color3usv = __MESA_SW_glColor3usv;
    glAPI->Color4b = __MESA_SW_glColor4b;
    glAPI->Color4bv = __MESA_SW_glColor4bv;
    glAPI->Color4d = __MESA_SW_glColor4d;
    glAPI->Color4dv = __MESA_SW_glColor4dv;
    glAPI->Color4f = __MESA_SW_glColor4f;
    glAPI->Color4fv = __MESA_SW_glColor4fv;
    glAPI->Color4i = __MESA_SW_glColor4i;
    glAPI->Color4iv = __MESA_SW_glColor4iv;
    glAPI->Color4s = __MESA_SW_glColor4s;
    glAPI->Color4sv = __MESA_SW_glColor4sv;
    glAPI->Color4ub = __MESA_SW_glColor4ub;
    glAPI->Color4ubv = __MESA_SW_glColor4ubv;
    glAPI->Color4ui = __MESA_SW_glColor4ui;
    glAPI->Color4uiv = __MESA_SW_glColor4uiv;
    glAPI->Color4us = __MESA_SW_glColor4us;
    glAPI->Color4usv = __MESA_SW_glColor4usv;
    glAPI->ColorMask = __MESA_SW_glColorMask;
    glAPI->ColorMaterial = __MESA_SW_glColorMaterial;
    glAPI->ColorPointer = __MESA_SW_glColorPointer;
    glAPI->CopyPixels = __MESA_SW_glCopyPixels;
    glAPI->CopyTexImage1D = __MESA_SW_glCopyTexImage1D;
    glAPI->CopyTexImage2D = __MESA_SW_glCopyTexImage2D;
    glAPI->CopyTexSubImage1D = __MESA_SW_glCopyTexSubImage1D;
    glAPI->CopyTexSubImage2D = __MESA_SW_glCopyTexSubImage2D;
    glAPI->CullFace = __MESA_SW_glCullFace;
    glAPI->DeleteLists = __MESA_SW_glDeleteLists;
    glAPI->DeleteTextures = __MESA_SW_glDeleteTextures;
    glAPI->DepthFunc = __MESA_SW_glDepthFunc;
    glAPI->DepthMask = __MESA_SW_glDepthMask;
    glAPI->DepthRange = __MESA_SW_glDepthRange;
    glAPI->Disable = __MESA_SW_glDisable;
    glAPI->DisableClientState = __MESA_SW_glDisableClientState;
    glAPI->DrawArrays = __MESA_SW_glDrawArrays;
    glAPI->DrawBuffer = __MESA_SW_glDrawBuffer;
    glAPI->DrawElements = __MESA_SW_glDrawElements;
    glAPI->DrawPixels = __MESA_SW_glDrawPixels;
    glAPI->EdgeFlag = __MESA_SW_glEdgeFlag;
    glAPI->EdgeFlagPointer = __MESA_SW_glEdgeFlagPointer;
    glAPI->EdgeFlagv = __MESA_SW_glEdgeFlagv;
    glAPI->Enable = __MESA_SW_glEnable;
    glAPI->EnableClientState = __MESA_SW_glEnableClientState;
    glAPI->End = __MESA_SW_glEnd;
    glAPI->EndList = __MESA_SW_glEndList;
    glAPI->EvalCoord1d = __MESA_SW_glEvalCoord1d;
    glAPI->EvalCoord1dv = __MESA_SW_glEvalCoord1dv;
    glAPI->EvalCoord1f = __MESA_SW_glEvalCoord1f;
    glAPI->EvalCoord1fv = __MESA_SW_glEvalCoord1fv;
    glAPI->EvalCoord2d = __MESA_SW_glEvalCoord2d;
    glAPI->EvalCoord2dv = __MESA_SW_glEvalCoord2dv;
    glAPI->EvalCoord2f = __MESA_SW_glEvalCoord2f;
    glAPI->EvalCoord2fv = __MESA_SW_glEvalCoord2fv;
    glAPI->EvalMesh1 = __MESA_SW_glEvalMesh1;
    glAPI->EvalMesh2 = __MESA_SW_glEvalMesh2;
    glAPI->EvalPoint1 = __MESA_SW_glEvalPoint1;
    glAPI->EvalPoint2 = __MESA_SW_glEvalPoint2;
    glAPI->FeedbackBuffer = __MESA_SW_glFeedbackBuffer;
    glAPI->Finish = __MESA_SW_glFinish;
    glAPI->Flush = __MESA_SW_glFlush;
    glAPI->Fogf = __MESA_SW_glFogf;
    glAPI->Fogfv = __MESA_SW_glFogfv;
    glAPI->Fogi = __MESA_SW_glFogi;
    glAPI->Fogiv = __MESA_SW_glFogiv;
    glAPI->FrontFace = __MESA_SW_glFrontFace;
    glAPI->Frustum = __MESA_SW_glFrustum;
    glAPI->GenLists = __MESA_SW_glGenLists;
    glAPI->GenTextures = __MESA_SW_glGenTextures;
    glAPI->GetBooleanv = __MESA_SW_glGetBooleanv;
    glAPI->GetClipPlane = __MESA_SW_glGetClipPlane;
    glAPI->GetDoublev = __MESA_SW_glGetDoublev;
    glAPI->GetError = __MESA_SW_glGetError;
    glAPI->GetFloatv = __MESA_SW_glGetFloatv;
    glAPI->GetIntegerv = __MESA_SW_glGetIntegerv;
    glAPI->GetLightfv = __MESA_SW_glGetLightfv;
    glAPI->GetLightiv = __MESA_SW_glGetLightiv;
    glAPI->GetMapdv = __MESA_SW_glGetMapdv;
    glAPI->GetMapfv = __MESA_SW_glGetMapfv;
    glAPI->GetMapiv = __MESA_SW_glGetMapiv;
    glAPI->GetMaterialfv = __MESA_SW_glGetMaterialfv;
    glAPI->GetMaterialiv = __MESA_SW_glGetMaterialiv;
    glAPI->GetPixelMapfv = __MESA_SW_glGetPixelMapfv;
    glAPI->GetPixelMapuiv = __MESA_SW_glGetPixelMapuiv;
    glAPI->GetPixelMapusv = __MESA_SW_glGetPixelMapusv;
    glAPI->GetPointerv = __MESA_SW_glGetPointerv;
    glAPI->GetPolygonStipple = __MESA_SW_glGetPolygonStipple;
    glAPI->GetString = __MESA_SW_glGetString;
    glAPI->GetTexEnvfv = __MESA_SW_glGetTexEnvfv;
    glAPI->GetTexEnviv = __MESA_SW_glGetTexEnviv;
    glAPI->GetTexGendv = __MESA_SW_glGetTexGendv;
    glAPI->GetTexGenfv = __MESA_SW_glGetTexGenfv;
    glAPI->GetTexGeniv = __MESA_SW_glGetTexGeniv;
    glAPI->GetTexImage = __MESA_SW_glGetTexImage;
    glAPI->GetTexLevelParameterfv = __MESA_SW_glGetTexLevelParameterfv;
    glAPI->GetTexLevelParameteriv = __MESA_SW_glGetTexLevelParameteriv;
    glAPI->GetTexParameterfv = __MESA_SW_glGetTexParameterfv;
    glAPI->GetTexParameteriv = __MESA_SW_glGetTexParameteriv;
    glAPI->Hint = __MESA_SW_glHint;
    glAPI->IndexMask = __MESA_SW_glIndexMask;
    glAPI->IndexPointer = __MESA_SW_glIndexPointer;
    glAPI->Indexd = __MESA_SW_glIndexd;
    glAPI->Indexdv = __MESA_SW_glIndexdv;
    glAPI->Indexf = __MESA_SW_glIndexf;
    glAPI->Indexfv = __MESA_SW_glIndexfv;
    glAPI->Indexi = __MESA_SW_glIndexi;
    glAPI->Indexiv = __MESA_SW_glIndexiv;
    glAPI->Indexs = __MESA_SW_glIndexs;
    glAPI->Indexsv = __MESA_SW_glIndexsv;
    glAPI->Indexub = __MESA_SW_glIndexub;
    glAPI->Indexubv = __MESA_SW_glIndexubv;
    glAPI->InitNames = __MESA_SW_glInitNames;
    glAPI->InterleavedArrays = __MESA_SW_glInterleavedArrays;
    glAPI->IsEnabled = __MESA_SW_glIsEnabled;
    glAPI->IsList = __MESA_SW_glIsList;
    glAPI->IsTexture = __MESA_SW_glIsTexture;
    glAPI->LightModelf = __MESA_SW_glLightModelf;
    glAPI->LightModelfv = __MESA_SW_glLightModelfv;
    glAPI->LightModeli = __MESA_SW_glLightModeli;
    glAPI->LightModeliv = __MESA_SW_glLightModeliv;
    glAPI->Lightf = __MESA_SW_glLightf;
    glAPI->Lightfv = __MESA_SW_glLightfv;
    glAPI->Lighti = __MESA_SW_glLighti;
    glAPI->Lightiv = __MESA_SW_glLightiv;
    glAPI->LineStipple = __MESA_SW_glLineStipple;
    glAPI->LineWidth = __MESA_SW_glLineWidth;
    glAPI->ListBase = __MESA_SW_glListBase;
    glAPI->LoadIdentity = __MESA_SW_glLoadIdentity;
    glAPI->LoadMatrixd = __MESA_SW_glLoadMatrixd;
    glAPI->LoadMatrixf = __MESA_SW_glLoadMatrixf;
    glAPI->LoadName = __MESA_SW_glLoadName;
    glAPI->LogicOp = __MESA_SW_glLogicOp;
    glAPI->Map1d = __MESA_SW_glMap1d;
    glAPI->Map1f = __MESA_SW_glMap1f;
    glAPI->Map2d = __MESA_SW_glMap2d;
    glAPI->Map2f = __MESA_SW_glMap2f;
    glAPI->MapGrid1d = __MESA_SW_glMapGrid1d;
    glAPI->MapGrid1f = __MESA_SW_glMapGrid1f;
    glAPI->MapGrid2d = __MESA_SW_glMapGrid2d;
    glAPI->MapGrid2f = __MESA_SW_glMapGrid2f;
    glAPI->Materialf = __MESA_SW_glMaterialf;
    glAPI->Materialfv = __MESA_SW_glMaterialfv;
    glAPI->Materiali = __MESA_SW_glMateriali;
    glAPI->Materialiv = __MESA_SW_glMaterialiv;
    glAPI->MatrixMode = __MESA_SW_glMatrixMode;
    glAPI->MultMatrixd = __MESA_SW_glMultMatrixd;
    glAPI->MultMatrixf = __MESA_SW_glMultMatrixf;
    glAPI->NewList = __MESA_SW_glNewList;
    glAPI->Normal3b = __MESA_SW_glNormal3b;
    glAPI->Normal3bv = __MESA_SW_glNormal3bv;
    glAPI->Normal3d = __MESA_SW_glNormal3d;
    glAPI->Normal3dv = __MESA_SW_glNormal3dv;
    glAPI->Normal3f = __MESA_SW_glNormal3f;
    glAPI->Normal3fv = __MESA_SW_glNormal3fv;
    glAPI->Normal3i = __MESA_SW_glNormal3i;
    glAPI->Normal3iv = __MESA_SW_glNormal3iv;
    glAPI->Normal3s = __MESA_SW_glNormal3s;
    glAPI->Normal3sv = __MESA_SW_glNormal3sv;
    glAPI->NormalPointer = __MESA_SW_glNormalPointer;
    glAPI->Ortho = __MESA_SW_glOrtho;
    glAPI->PassThrough = __MESA_SW_glPassThrough;
    glAPI->PixelMapfv = __MESA_SW_glPixelMapfv;
    glAPI->PixelMapuiv = __MESA_SW_glPixelMapuiv;
    glAPI->PixelMapusv = __MESA_SW_glPixelMapusv;
    glAPI->PixelStoref = __MESA_SW_glPixelStoref;
    glAPI->PixelStorei = __MESA_SW_glPixelStorei;
    glAPI->PixelTransferf = __MESA_SW_glPixelTransferf;
    glAPI->PixelTransferi = __MESA_SW_glPixelTransferi;
    glAPI->PixelZoom = __MESA_SW_glPixelZoom;
    glAPI->PointSize = __MESA_SW_glPointSize;
    glAPI->PolygonMode = __MESA_SW_glPolygonMode;
    glAPI->PolygonOffset = __MESA_SW_glPolygonOffset;
    glAPI->PolygonStipple = __MESA_SW_glPolygonStipple;
    glAPI->PopAttrib = __MESA_SW_glPopAttrib;
    glAPI->PopClientAttrib = __MESA_SW_glPopClientAttrib;
    glAPI->PopMatrix = __MESA_SW_glPopMatrix;
    glAPI->PopName = __MESA_SW_glPopName;
    glAPI->PrioritizeTextures = __MESA_SW_glPrioritizeTextures;
    glAPI->PushAttrib = __MESA_SW_glPushAttrib;
    glAPI->PushClientAttrib = __MESA_SW_glPushClientAttrib;
    glAPI->PushMatrix = __MESA_SW_glPushMatrix;
    glAPI->PushName = __MESA_SW_glPushName;
    glAPI->RasterPos2d = __MESA_SW_glRasterPos2d;
    glAPI->RasterPos2dv = __MESA_SW_glRasterPos2dv;
    glAPI->RasterPos2f = __MESA_SW_glRasterPos2f;
    glAPI->RasterPos2fv = __MESA_SW_glRasterPos2fv;
    glAPI->RasterPos2i = __MESA_SW_glRasterPos2i;
    glAPI->RasterPos2iv = __MESA_SW_glRasterPos2iv;
    glAPI->RasterPos2s = __MESA_SW_glRasterPos2s;
    glAPI->RasterPos2sv = __MESA_SW_glRasterPos2sv;
    glAPI->RasterPos3d = __MESA_SW_glRasterPos3d;
    glAPI->RasterPos3dv = __MESA_SW_glRasterPos3dv;
    glAPI->RasterPos3f = __MESA_SW_glRasterPos3f;
    glAPI->RasterPos3fv = __MESA_SW_glRasterPos3fv;
    glAPI->RasterPos3i = __MESA_SW_glRasterPos3i;
    glAPI->RasterPos3iv = __MESA_SW_glRasterPos3iv;
    glAPI->RasterPos3s = __MESA_SW_glRasterPos3s;
    glAPI->RasterPos3sv = __MESA_SW_glRasterPos3sv;
    glAPI->RasterPos4d = __MESA_SW_glRasterPos4d;
    glAPI->RasterPos4dv = __MESA_SW_glRasterPos4dv;
    glAPI->RasterPos4f = __MESA_SW_glRasterPos4f;
    glAPI->RasterPos4fv = __MESA_SW_glRasterPos4fv;
    glAPI->RasterPos4i = __MESA_SW_glRasterPos4i;
    glAPI->RasterPos4iv = __MESA_SW_glRasterPos4iv;
    glAPI->RasterPos4s = __MESA_SW_glRasterPos4s;
    glAPI->RasterPos4sv = __MESA_SW_glRasterPos4sv;
    glAPI->ReadBuffer = __MESA_SW_glReadBuffer;
    glAPI->ReadPixels = __MESA_SW_glReadPixels;
    glAPI->Rectd = __MESA_SW_glRectd;
    glAPI->Rectdv = __MESA_SW_glRectdv;
    glAPI->Rectf = __MESA_SW_glRectf;
    glAPI->Rectfv = __MESA_SW_glRectfv;
    glAPI->Recti = __MESA_SW_glRecti;
    glAPI->Rectiv = __MESA_SW_glRectiv;
    glAPI->Rects = __MESA_SW_glRects;
    glAPI->Rectsv = __MESA_SW_glRectsv;
    glAPI->RenderMode = __MESA_SW_glRenderMode;
    glAPI->Rotated = __MESA_SW_glRotated;
    glAPI->Rotatef = __MESA_SW_glRotatef;
    glAPI->Scaled = __MESA_SW_glScaled;
    glAPI->Scalef = __MESA_SW_glScalef;
    glAPI->Scissor = __MESA_SW_glScissor;
    glAPI->SelectBuffer = __MESA_SW_glSelectBuffer;
    glAPI->ShadeModel = __MESA_SW_glShadeModel;
    glAPI->StencilFunc = __MESA_SW_glStencilFunc;
    glAPI->StencilMask = __MESA_SW_glStencilMask;
    glAPI->StencilOp = __MESA_SW_glStencilOp;
    glAPI->TexCoord1d = __MESA_SW_glTexCoord1d;
    glAPI->TexCoord1dv = __MESA_SW_glTexCoord1dv;
    glAPI->TexCoord1f = __MESA_SW_glTexCoord1f;
    glAPI->TexCoord1fv = __MESA_SW_glTexCoord1fv;
    glAPI->TexCoord1i = __MESA_SW_glTexCoord1i;
    glAPI->TexCoord1iv = __MESA_SW_glTexCoord1iv;
    glAPI->TexCoord1s = __MESA_SW_glTexCoord1s;
    glAPI->TexCoord1sv = __MESA_SW_glTexCoord1sv;
    glAPI->TexCoord2d = __MESA_SW_glTexCoord2d;
    glAPI->TexCoord2dv = __MESA_SW_glTexCoord2dv;
    glAPI->TexCoord2f = __MESA_SW_glTexCoord2f;
    glAPI->TexCoord2fv = __MESA_SW_glTexCoord2fv;
    glAPI->TexCoord2i = __MESA_SW_glTexCoord2i;
    glAPI->TexCoord2iv = __MESA_SW_glTexCoord2iv;
    glAPI->TexCoord2s = __MESA_SW_glTexCoord2s;
    glAPI->TexCoord2sv = __MESA_SW_glTexCoord2sv;
    glAPI->TexCoord3d = __MESA_SW_glTexCoord3d;
    glAPI->TexCoord3dv = __MESA_SW_glTexCoord3dv;
    glAPI->TexCoord3f = __MESA_SW_glTexCoord3f;
    glAPI->TexCoord3fv = __MESA_SW_glTexCoord3fv;
    glAPI->TexCoord3i = __MESA_SW_glTexCoord3i;
    glAPI->TexCoord3iv = __MESA_SW_glTexCoord3iv;
    glAPI->TexCoord3s = __MESA_SW_glTexCoord3s;
    glAPI->TexCoord3sv = __MESA_SW_glTexCoord3sv;
    glAPI->TexCoord4d = __MESA_SW_glTexCoord4d;
    glAPI->TexCoord4dv = __MESA_SW_glTexCoord4dv;
    glAPI->TexCoord4f = __MESA_SW_glTexCoord4f;
    glAPI->TexCoord4fv = __MESA_SW_glTexCoord4fv;
    glAPI->TexCoord4i = __MESA_SW_glTexCoord4i;
    glAPI->TexCoord4iv = __MESA_SW_glTexCoord4iv;
    glAPI->TexCoord4s = __MESA_SW_glTexCoord4s;
    glAPI->TexCoord4sv = __MESA_SW_glTexCoord4sv;
    glAPI->TexCoordPointer = __MESA_SW_glTexCoordPointer;
    glAPI->TexEnvf = __MESA_SW_glTexEnvf;
    glAPI->TexEnvfv = __MESA_SW_glTexEnvfv;
    glAPI->TexEnvi = __MESA_SW_glTexEnvi;
    glAPI->TexEnviv = __MESA_SW_glTexEnviv;
    glAPI->TexGend = __MESA_SW_glTexGend;
    glAPI->TexGendv = __MESA_SW_glTexGendv;
    glAPI->TexGenf = __MESA_SW_glTexGenf;
    glAPI->TexGenfv = __MESA_SW_glTexGenfv;
    glAPI->TexGeni = __MESA_SW_glTexGeni;
    glAPI->TexGeniv = __MESA_SW_glTexGeniv;
    glAPI->TexImage1D = __MESA_SW_glTexImage1D;
    glAPI->TexImage2D = __MESA_SW_glTexImage2D;
    glAPI->TexParameterf = __MESA_SW_glTexParameterf;
    glAPI->TexParameterfv = __MESA_SW_glTexParameterfv;
    glAPI->TexParameteri = __MESA_SW_glTexParameteri;
    glAPI->TexParameteriv = __MESA_SW_glTexParameteriv;
    glAPI->TexSubImage1D = __MESA_SW_glTexSubImage1D;
    glAPI->TexSubImage2D = __MESA_SW_glTexSubImage2D;
    glAPI->Translated = __MESA_SW_glTranslated;
    glAPI->Translatef = __MESA_SW_glTranslatef;
    glAPI->Vertex2d = __MESA_SW_glVertex2d;
    glAPI->Vertex2dv = __MESA_SW_glVertex2dv;
    glAPI->Vertex2f = __MESA_SW_glVertex2f;
    glAPI->Vertex2fv = __MESA_SW_glVertex2fv;
    glAPI->Vertex2i = __MESA_SW_glVertex2i;
    glAPI->Vertex2iv = __MESA_SW_glVertex2iv;
    glAPI->Vertex2s = __MESA_SW_glVertex2s;
    glAPI->Vertex2sv = __MESA_SW_glVertex2sv;
    glAPI->Vertex3d = __MESA_SW_glVertex3d;
    glAPI->Vertex3dv = __MESA_SW_glVertex3dv;
    glAPI->Vertex3f = __MESA_SW_glVertex3f;
    glAPI->Vertex3fv = __MESA_SW_glVertex3fv;
    glAPI->Vertex3i = __MESA_SW_glVertex3i;
    glAPI->Vertex3iv = __MESA_SW_glVertex3iv;
    glAPI->Vertex3s = __MESA_SW_glVertex3s;
    glAPI->Vertex3sv = __MESA_SW_glVertex3sv;
    glAPI->Vertex4d = __MESA_SW_glVertex4d;
    glAPI->Vertex4dv = __MESA_SW_glVertex4dv;
    glAPI->Vertex4f = __MESA_SW_glVertex4f;
    glAPI->Vertex4fv = __MESA_SW_glVertex4fv;
    glAPI->Vertex4i = __MESA_SW_glVertex4i;
    glAPI->Vertex4iv = __MESA_SW_glVertex4iv;
    glAPI->Vertex4s = __MESA_SW_glVertex4s;
    glAPI->Vertex4sv = __MESA_SW_glVertex4sv;
    glAPI->VertexPointer = __MESA_SW_glVertexPointer;
    glAPI->Viewport = __MESA_SW_glViewport;
#endif

    handle = (void *)1;
#endif

    return handle;
}

void driMesaDestroyAPI(void *handle)
{
#ifdef GLX_USE_DL
    if (handle) dlclose(handle);
#endif
}

#endif
