/* $XFree86: xc/lib/GL/dri/dri_mesa_init.c,v 1.1 1999/06/14 07:23:31 dawes Exp $ */
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
 * $PI: xc/lib/GL/dri/dri_mesa_init.c,v 1.5 1999/06/21 05:13:54 martin Exp $
 */

#ifdef GLX_DIRECT_RENDERING

#ifdef GLX_USE_DLOPEN
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


int dlopen_error;

#ifdef GLX_USE_DLOPEN
static void *resolveSymbol(void *handle, char *name)
{
    char *error;
    void *func;

    func = dlsym(handle, name);
    if ((error = dlerror()) != NULL) {
	fputs(error, stderr);
	fputs("\n", stderr);
	dlopen_error = True;
	return NULL;
    }

    return func;
}
#endif

void *driMesaInitAPI(char *name, __XMESAapi *XMesaAPI, __GLapi *glAPI)
{
    void *handle = (void *)NULL;
#ifdef GLX_USE_DLOPEN
    char libname[256];

    dlopen_error = False;

    if (name) {
	snprintf(libname, 256, "%s.so", name);
    } else {
	return NULL;
    }

    handle = dlopen(libname, RTLD_NOW);
    if (!handle) {
	fputs(dlerror(), stderr);
	fputs("\n", stderr);
	return NULL;
    }

    XMesaAPI->InitDriver = resolveSymbol(handle, "XMesaInitDriver");
    XMesaAPI->ResetDriver = resolveSymbol(handle, "XMesaResetDriver");
    XMesaAPI->CreateVisual = resolveSymbol(handle, "XMesaCreateVisual");
    XMesaAPI->DestroyVisual = resolveSymbol(handle, "XMesaDestroyVisual");
    XMesaAPI->CreateContext = resolveSymbol(handle, "XMesaCreateContext");
    XMesaAPI->DestroyContext = resolveSymbol(handle, "XMesaDestroyContext");
    XMesaAPI->CreateWindowBuffer =
	resolveSymbol(handle, "XMesaCreateWindowBuffer");
    XMesaAPI->CreatePixmapBuffer =
	resolveSymbol(handle, "XMesaCreatePixmapBuffer");
    XMesaAPI->DestroyBuffer = resolveSymbol(handle, "XMesaDestroyBuffer");
    XMesaAPI->SwapBuffers = resolveSymbol(handle, "XMesaSwapBuffers");
    XMesaAPI->MakeCurrent = resolveSymbol(handle, "XMesaMakeCurrent");

    glAPI->Accum = resolveSymbol(handle, "glAccum");
    glAPI->AlphaFunc = resolveSymbol(handle, "glAlphaFunc");
    glAPI->AreTexturesResident =
	resolveSymbol(handle, "glAreTexturesResident");
    glAPI->ArrayElement = resolveSymbol(handle, "glArrayElement");
    glAPI->Begin = resolveSymbol(handle, "glBegin");
    glAPI->BindTexture = resolveSymbol(handle, "glBindTexture");
    glAPI->Bitmap = resolveSymbol(handle, "glBitmap");
    glAPI->BlendFunc = resolveSymbol(handle, "glBlendFunc");
    glAPI->CallList = resolveSymbol(handle, "glCallList");
    glAPI->CallLists = resolveSymbol(handle, "glCallLists");
    glAPI->Clear = resolveSymbol(handle, "glClear");
    glAPI->ClearAccum = resolveSymbol(handle, "glClearAccum");
    glAPI->ClearColor = resolveSymbol(handle, "glClearColor");
    glAPI->ClearDepth = resolveSymbol(handle, "glClearDepth");
    glAPI->ClearIndex = resolveSymbol(handle, "glClearIndex");
    glAPI->ClearStencil = resolveSymbol(handle, "glClearStencil");
    glAPI->ClipPlane = resolveSymbol(handle, "glClipPlane");
    glAPI->Color3b = resolveSymbol(handle, "glColor3b");
    glAPI->Color3bv = resolveSymbol(handle, "glColor3bv");
    glAPI->Color3d = resolveSymbol(handle, "glColor3d");
    glAPI->Color3dv = resolveSymbol(handle, "glColor3dv");
    glAPI->Color3f = resolveSymbol(handle, "glColor3f");
    glAPI->Color3fv = resolveSymbol(handle, "glColor3fv");
    glAPI->Color3i = resolveSymbol(handle, "glColor3i");
    glAPI->Color3iv = resolveSymbol(handle, "glColor3iv");
    glAPI->Color3s = resolveSymbol(handle, "glColor3s");
    glAPI->Color3sv = resolveSymbol(handle, "glColor3sv");
    glAPI->Color3ub = resolveSymbol(handle, "glColor3ub");
    glAPI->Color3ubv = resolveSymbol(handle, "glColor3ubv");
    glAPI->Color3ui = resolveSymbol(handle, "glColor3ui");
    glAPI->Color3uiv = resolveSymbol(handle, "glColor3uiv");
    glAPI->Color3us = resolveSymbol(handle, "glColor3us");
    glAPI->Color3usv = resolveSymbol(handle, "glColor3usv");
    glAPI->Color4b = resolveSymbol(handle, "glColor4b");
    glAPI->Color4bv = resolveSymbol(handle, "glColor4bv");
    glAPI->Color4d = resolveSymbol(handle, "glColor4d");
    glAPI->Color4dv = resolveSymbol(handle, "glColor4dv");
    glAPI->Color4f = resolveSymbol(handle, "glColor4f");
    glAPI->Color4fv = resolveSymbol(handle, "glColor4fv");
    glAPI->Color4i = resolveSymbol(handle, "glColor4i");
    glAPI->Color4iv = resolveSymbol(handle, "glColor4iv");
    glAPI->Color4s = resolveSymbol(handle, "glColor4s");
    glAPI->Color4sv = resolveSymbol(handle, "glColor4sv");
    glAPI->Color4ub = resolveSymbol(handle, "glColor4ub");
    glAPI->Color4ubv = resolveSymbol(handle, "glColor4ubv");
    glAPI->Color4ui = resolveSymbol(handle, "glColor4ui");
    glAPI->Color4uiv = resolveSymbol(handle, "glColor4uiv");
    glAPI->Color4us = resolveSymbol(handle, "glColor4us");
    glAPI->Color4usv = resolveSymbol(handle, "glColor4usv");
    glAPI->ColorMask = resolveSymbol(handle, "glColorMask");
    glAPI->ColorMaterial = resolveSymbol(handle, "glColorMaterial");
    glAPI->ColorPointer = resolveSymbol(handle, "glColorPointer");
    glAPI->CopyPixels = resolveSymbol(handle, "glCopyPixels");
    glAPI->CopyTexImage1D = resolveSymbol(handle, "glCopyTexImage1D");
    glAPI->CopyTexImage2D = resolveSymbol(handle, "glCopyTexImage2D");
    glAPI->CopyTexSubImage1D = resolveSymbol(handle, "glCopyTexSubImage1D");
    glAPI->CopyTexSubImage2D = resolveSymbol(handle, "glCopyTexSubImage2D");
    glAPI->CullFace = resolveSymbol(handle, "glCullFace");
    glAPI->DeleteLists = resolveSymbol(handle, "glDeleteLists");
    glAPI->DeleteTextures = resolveSymbol(handle, "glDeleteTextures");
    glAPI->DepthFunc = resolveSymbol(handle, "glDepthFunc");
    glAPI->DepthMask = resolveSymbol(handle, "glDepthMask");
    glAPI->DepthRange = resolveSymbol(handle, "glDepthRange");
    glAPI->Disable = resolveSymbol(handle, "glDisable");
    glAPI->DisableClientState = resolveSymbol(handle, "glDisableClientState");
    glAPI->DrawArrays = resolveSymbol(handle, "glDrawArrays");
    glAPI->DrawBuffer = resolveSymbol(handle, "glDrawBuffer");
    glAPI->DrawElements = resolveSymbol(handle, "glDrawElements");
    glAPI->DrawPixels = resolveSymbol(handle, "glDrawPixels");
    glAPI->EdgeFlag = resolveSymbol(handle, "glEdgeFlag");
    glAPI->EdgeFlagPointer = resolveSymbol(handle, "glEdgeFlagPointer");
    glAPI->EdgeFlagv = resolveSymbol(handle, "glEdgeFlagv");
    glAPI->Enable = resolveSymbol(handle, "glEnable");
    glAPI->EnableClientState = resolveSymbol(handle, "glEnableClientState");
    glAPI->End = resolveSymbol(handle, "glEnd");
    glAPI->EndList = resolveSymbol(handle, "glEndList");
    glAPI->EvalCoord1d = resolveSymbol(handle, "glEvalCoord1d");
    glAPI->EvalCoord1dv = resolveSymbol(handle, "glEvalCoord1dv");
    glAPI->EvalCoord1f = resolveSymbol(handle, "glEvalCoord1f");
    glAPI->EvalCoord1fv = resolveSymbol(handle, "glEvalCoord1fv");
    glAPI->EvalCoord2d = resolveSymbol(handle, "glEvalCoord2d");
    glAPI->EvalCoord2dv = resolveSymbol(handle, "glEvalCoord2dv");
    glAPI->EvalCoord2f = resolveSymbol(handle, "glEvalCoord2f");
    glAPI->EvalCoord2fv = resolveSymbol(handle, "glEvalCoord2fv");
    glAPI->EvalMesh1 = resolveSymbol(handle, "glEvalMesh1");
    glAPI->EvalMesh2 = resolveSymbol(handle, "glEvalMesh2");
    glAPI->EvalPoint1 = resolveSymbol(handle, "glEvalPoint1");
    glAPI->EvalPoint2 = resolveSymbol(handle, "glEvalPoint2");
    glAPI->FeedbackBuffer = resolveSymbol(handle, "glFeedbackBuffer");
    glAPI->Finish = resolveSymbol(handle, "glFinish");
    glAPI->Flush = resolveSymbol(handle, "glFlush");
    glAPI->Fogf = resolveSymbol(handle, "glFogf");
    glAPI->Fogfv = resolveSymbol(handle, "glFogfv");
    glAPI->Fogi = resolveSymbol(handle, "glFogi");
    glAPI->Fogiv = resolveSymbol(handle, "glFogiv");
    glAPI->FrontFace = resolveSymbol(handle, "glFrontFace");
    glAPI->Frustum = resolveSymbol(handle, "glFrustum");
    glAPI->GenLists = resolveSymbol(handle, "glGenLists");
    glAPI->GenTextures = resolveSymbol(handle, "glGenTextures");
    glAPI->GetBooleanv = resolveSymbol(handle, "glGetBooleanv");
    glAPI->GetClipPlane = resolveSymbol(handle, "glGetClipPlane");
    glAPI->GetDoublev = resolveSymbol(handle, "glGetDoublev");
    glAPI->GetError = resolveSymbol(handle, "glGetError");
    glAPI->GetFloatv = resolveSymbol(handle, "glGetFloatv");
    glAPI->GetIntegerv = resolveSymbol(handle, "glGetIntegerv");
    glAPI->GetLightfv = resolveSymbol(handle, "glGetLightfv");
    glAPI->GetLightiv = resolveSymbol(handle, "glGetLightiv");
    glAPI->GetMapdv = resolveSymbol(handle, "glGetMapdv");
    glAPI->GetMapfv = resolveSymbol(handle, "glGetMapfv");
    glAPI->GetMapiv = resolveSymbol(handle, "glGetMapiv");
    glAPI->GetMaterialfv = resolveSymbol(handle, "glGetMaterialfv");
    glAPI->GetMaterialiv = resolveSymbol(handle, "glGetMaterialiv");
    glAPI->GetPixelMapfv = resolveSymbol(handle, "glGetPixelMapfv");
    glAPI->GetPixelMapuiv = resolveSymbol(handle, "glGetPixelMapuiv");
    glAPI->GetPixelMapusv = resolveSymbol(handle, "glGetPixelMapusv");
    glAPI->GetPointerv = resolveSymbol(handle, "glGetPointerv");
    glAPI->GetPolygonStipple = resolveSymbol(handle, "glGetPolygonStipple");
    glAPI->GetString = resolveSymbol(handle, "glGetString");
    glAPI->GetTexEnvfv = resolveSymbol(handle, "glGetTexEnvfv");
    glAPI->GetTexEnviv = resolveSymbol(handle, "glGetTexEnviv");
    glAPI->GetTexGendv = resolveSymbol(handle, "glGetTexGendv");
    glAPI->GetTexGenfv = resolveSymbol(handle, "glGetTexGenfv");
    glAPI->GetTexGeniv = resolveSymbol(handle, "glGetTexGeniv");
    glAPI->GetTexImage = resolveSymbol(handle, "glGetTexImage");
    glAPI->GetTexLevelParameterfv =
	resolveSymbol(handle, "glGetTexLevelParameterfv");
    glAPI->GetTexLevelParameteriv =
	resolveSymbol(handle, "glGetTexLevelParameteriv");
    glAPI->GetTexParameterfv = resolveSymbol(handle, "glGetTexParameterfv");
    glAPI->GetTexParameteriv = resolveSymbol(handle, "glGetTexParameteriv");
    glAPI->Hint = resolveSymbol(handle, "glHint");
    glAPI->IndexMask = resolveSymbol(handle, "glIndexMask");
    glAPI->IndexPointer = resolveSymbol(handle, "glIndexPointer");
    glAPI->Indexd = resolveSymbol(handle, "glIndexd");
    glAPI->Indexdv = resolveSymbol(handle, "glIndexdv");
    glAPI->Indexf = resolveSymbol(handle, "glIndexf");
    glAPI->Indexfv = resolveSymbol(handle, "glIndexfv");
    glAPI->Indexi = resolveSymbol(handle, "glIndexi");
    glAPI->Indexiv = resolveSymbol(handle, "glIndexiv");
    glAPI->Indexs = resolveSymbol(handle, "glIndexs");
    glAPI->Indexsv = resolveSymbol(handle, "glIndexsv");
    glAPI->Indexub = resolveSymbol(handle, "glIndexub");
    glAPI->Indexubv = resolveSymbol(handle, "glIndexubv");
    glAPI->InitNames = resolveSymbol(handle, "glInitNames");
    glAPI->InterleavedArrays = resolveSymbol(handle, "glInterleavedArrays");
    glAPI->IsEnabled = resolveSymbol(handle, "glIsEnabled");
    glAPI->IsList = resolveSymbol(handle, "glIsList");
    glAPI->IsTexture = resolveSymbol(handle, "glIsTexture");
    glAPI->LightModelf = resolveSymbol(handle, "glLightModelf");
    glAPI->LightModelfv = resolveSymbol(handle, "glLightModelfv");
    glAPI->LightModeli = resolveSymbol(handle, "glLightModeli");
    glAPI->LightModeliv = resolveSymbol(handle, "glLightModeliv");
    glAPI->Lightf = resolveSymbol(handle, "glLightf");
    glAPI->Lightfv = resolveSymbol(handle, "glLightfv");
    glAPI->Lighti = resolveSymbol(handle, "glLighti");
    glAPI->Lightiv = resolveSymbol(handle, "glLightiv");
    glAPI->LineStipple = resolveSymbol(handle, "glLineStipple");
    glAPI->LineWidth = resolveSymbol(handle, "glLineWidth");
    glAPI->ListBase = resolveSymbol(handle, "glListBase");
    glAPI->LoadIdentity = resolveSymbol(handle, "glLoadIdentity");
    glAPI->LoadMatrixd = resolveSymbol(handle, "glLoadMatrixd");
    glAPI->LoadMatrixf = resolveSymbol(handle, "glLoadMatrixf");
    glAPI->LoadName = resolveSymbol(handle, "glLoadName");
    glAPI->LogicOp = resolveSymbol(handle, "glLogicOp");
    glAPI->Map1d = resolveSymbol(handle, "glMap1d");
    glAPI->Map1f = resolveSymbol(handle, "glMap1f");
    glAPI->Map2d = resolveSymbol(handle, "glMap2d");
    glAPI->Map2f = resolveSymbol(handle, "glMap2f");
    glAPI->MapGrid1d = resolveSymbol(handle, "glMapGrid1d");
    glAPI->MapGrid1f = resolveSymbol(handle, "glMapGrid1f");
    glAPI->MapGrid2d = resolveSymbol(handle, "glMapGrid2d");
    glAPI->MapGrid2f = resolveSymbol(handle, "glMapGrid2f");
    glAPI->Materialf = resolveSymbol(handle, "glMaterialf");
    glAPI->Materialfv = resolveSymbol(handle, "glMaterialfv");
    glAPI->Materiali = resolveSymbol(handle, "glMateriali");
    glAPI->Materialiv = resolveSymbol(handle, "glMaterialiv");
    glAPI->MatrixMode = resolveSymbol(handle, "glMatrixMode");
    glAPI->MultMatrixd = resolveSymbol(handle, "glMultMatrixd");
    glAPI->MultMatrixf = resolveSymbol(handle, "glMultMatrixf");
    glAPI->NewList = resolveSymbol(handle, "glNewList");
    glAPI->Normal3b = resolveSymbol(handle, "glNormal3b");
    glAPI->Normal3bv = resolveSymbol(handle, "glNormal3bv");
    glAPI->Normal3d = resolveSymbol(handle, "glNormal3d");
    glAPI->Normal3dv = resolveSymbol(handle, "glNormal3dv");
    glAPI->Normal3f = resolveSymbol(handle, "glNormal3f");
    glAPI->Normal3fv = resolveSymbol(handle, "glNormal3fv");
    glAPI->Normal3i = resolveSymbol(handle, "glNormal3i");
    glAPI->Normal3iv = resolveSymbol(handle, "glNormal3iv");
    glAPI->Normal3s = resolveSymbol(handle, "glNormal3s");
    glAPI->Normal3sv = resolveSymbol(handle, "glNormal3sv");
    glAPI->NormalPointer = resolveSymbol(handle, "glNormalPointer");
    glAPI->Ortho = resolveSymbol(handle, "glOrtho");
    glAPI->PassThrough = resolveSymbol(handle, "glPassThrough");
    glAPI->PixelMapfv = resolveSymbol(handle, "glPixelMapfv");
    glAPI->PixelMapuiv = resolveSymbol(handle, "glPixelMapuiv");
    glAPI->PixelMapusv = resolveSymbol(handle, "glPixelMapusv");
    glAPI->PixelStoref = resolveSymbol(handle, "glPixelStoref");
    glAPI->PixelStorei = resolveSymbol(handle, "glPixelStorei");
    glAPI->PixelTransferf = resolveSymbol(handle, "glPixelTransferf");
    glAPI->PixelTransferi = resolveSymbol(handle, "glPixelTransferi");
    glAPI->PixelZoom = resolveSymbol(handle, "glPixelZoom");
    glAPI->PointSize = resolveSymbol(handle, "glPointSize");
    glAPI->PolygonMode = resolveSymbol(handle, "glPolygonMode");
    glAPI->PolygonOffset = resolveSymbol(handle, "glPolygonOffset");
    glAPI->PolygonStipple = resolveSymbol(handle, "glPolygonStipple");
    glAPI->PopAttrib = resolveSymbol(handle, "glPopAttrib");
    glAPI->PopClientAttrib = resolveSymbol(handle, "glPopClientAttrib");
    glAPI->PopMatrix = resolveSymbol(handle, "glPopMatrix");
    glAPI->PopName = resolveSymbol(handle, "glPopName");
    glAPI->PrioritizeTextures = resolveSymbol(handle, "glPrioritizeTextures");
    glAPI->PushAttrib = resolveSymbol(handle, "glPushAttrib");
    glAPI->PushClientAttrib = resolveSymbol(handle, "glPushClientAttrib");
    glAPI->PushMatrix = resolveSymbol(handle, "glPushMatrix");
    glAPI->PushName = resolveSymbol(handle, "glPushName");
    glAPI->RasterPos2d = resolveSymbol(handle, "glRasterPos2d");
    glAPI->RasterPos2dv = resolveSymbol(handle, "glRasterPos2dv");
    glAPI->RasterPos2f = resolveSymbol(handle, "glRasterPos2f");
    glAPI->RasterPos2fv = resolveSymbol(handle, "glRasterPos2fv");
    glAPI->RasterPos2i = resolveSymbol(handle, "glRasterPos2i");
    glAPI->RasterPos2iv = resolveSymbol(handle, "glRasterPos2iv");
    glAPI->RasterPos2s = resolveSymbol(handle, "glRasterPos2s");
    glAPI->RasterPos2sv = resolveSymbol(handle, "glRasterPos2sv");
    glAPI->RasterPos3d = resolveSymbol(handle, "glRasterPos3d");
    glAPI->RasterPos3dv = resolveSymbol(handle, "glRasterPos3dv");
    glAPI->RasterPos3f = resolveSymbol(handle, "glRasterPos3f");
    glAPI->RasterPos3fv = resolveSymbol(handle, "glRasterPos3fv");
    glAPI->RasterPos3i = resolveSymbol(handle, "glRasterPos3i");
    glAPI->RasterPos3iv = resolveSymbol(handle, "glRasterPos3iv");
    glAPI->RasterPos3s = resolveSymbol(handle, "glRasterPos3s");
    glAPI->RasterPos3sv = resolveSymbol(handle, "glRasterPos3sv");
    glAPI->RasterPos4d = resolveSymbol(handle, "glRasterPos4d");
    glAPI->RasterPos4dv = resolveSymbol(handle, "glRasterPos4dv");
    glAPI->RasterPos4f = resolveSymbol(handle, "glRasterPos4f");
    glAPI->RasterPos4fv = resolveSymbol(handle, "glRasterPos4fv");
    glAPI->RasterPos4i = resolveSymbol(handle, "glRasterPos4i");
    glAPI->RasterPos4iv = resolveSymbol(handle, "glRasterPos4iv");
    glAPI->RasterPos4s = resolveSymbol(handle, "glRasterPos4s");
    glAPI->RasterPos4sv = resolveSymbol(handle, "glRasterPos4sv");
    glAPI->ReadBuffer = resolveSymbol(handle, "glReadBuffer");
    glAPI->ReadPixels = resolveSymbol(handle, "glReadPixels");
    glAPI->Rectd = resolveSymbol(handle, "glRectd");
    glAPI->Rectdv = resolveSymbol(handle, "glRectdv");
    glAPI->Rectf = resolveSymbol(handle, "glRectf");
    glAPI->Rectfv = resolveSymbol(handle, "glRectfv");
    glAPI->Recti = resolveSymbol(handle, "glRecti");
    glAPI->Rectiv = resolveSymbol(handle, "glRectiv");
    glAPI->Rects = resolveSymbol(handle, "glRects");
    glAPI->Rectsv = resolveSymbol(handle, "glRectsv");
    glAPI->RenderMode = resolveSymbol(handle, "glRenderMode");
    glAPI->Rotated = resolveSymbol(handle, "glRotated");
    glAPI->Rotatef = resolveSymbol(handle, "glRotatef");
    glAPI->Scaled = resolveSymbol(handle, "glScaled");
    glAPI->Scalef = resolveSymbol(handle, "glScalef");
    glAPI->Scissor = resolveSymbol(handle, "glScissor");
    glAPI->SelectBuffer = resolveSymbol(handle, "glSelectBuffer");
    glAPI->ShadeModel = resolveSymbol(handle, "glShadeModel");
    glAPI->StencilFunc = resolveSymbol(handle, "glStencilFunc");
    glAPI->StencilMask = resolveSymbol(handle, "glStencilMask");
    glAPI->StencilOp = resolveSymbol(handle, "glStencilOp");
    glAPI->TexCoord1d = resolveSymbol(handle, "glTexCoord1d");
    glAPI->TexCoord1dv = resolveSymbol(handle, "glTexCoord1dv");
    glAPI->TexCoord1f = resolveSymbol(handle, "glTexCoord1f");
    glAPI->TexCoord1fv = resolveSymbol(handle, "glTexCoord1fv");
    glAPI->TexCoord1i = resolveSymbol(handle, "glTexCoord1i");
    glAPI->TexCoord1iv = resolveSymbol(handle, "glTexCoord1iv");
    glAPI->TexCoord1s = resolveSymbol(handle, "glTexCoord1s");
    glAPI->TexCoord1sv = resolveSymbol(handle, "glTexCoord1sv");
    glAPI->TexCoord2d = resolveSymbol(handle, "glTexCoord2d");
    glAPI->TexCoord2dv = resolveSymbol(handle, "glTexCoord2dv");
    glAPI->TexCoord2f = resolveSymbol(handle, "glTexCoord2f");
    glAPI->TexCoord2fv = resolveSymbol(handle, "glTexCoord2fv");
    glAPI->TexCoord2i = resolveSymbol(handle, "glTexCoord2i");
    glAPI->TexCoord2iv = resolveSymbol(handle, "glTexCoord2iv");
    glAPI->TexCoord2s = resolveSymbol(handle, "glTexCoord2s");
    glAPI->TexCoord2sv = resolveSymbol(handle, "glTexCoord2sv");
    glAPI->TexCoord3d = resolveSymbol(handle, "glTexCoord3d");
    glAPI->TexCoord3dv = resolveSymbol(handle, "glTexCoord3dv");
    glAPI->TexCoord3f = resolveSymbol(handle, "glTexCoord3f");
    glAPI->TexCoord3fv = resolveSymbol(handle, "glTexCoord3fv");
    glAPI->TexCoord3i = resolveSymbol(handle, "glTexCoord3i");
    glAPI->TexCoord3iv = resolveSymbol(handle, "glTexCoord3iv");
    glAPI->TexCoord3s = resolveSymbol(handle, "glTexCoord3s");
    glAPI->TexCoord3sv = resolveSymbol(handle, "glTexCoord3sv");
    glAPI->TexCoord4d = resolveSymbol(handle, "glTexCoord4d");
    glAPI->TexCoord4dv = resolveSymbol(handle, "glTexCoord4dv");
    glAPI->TexCoord4f = resolveSymbol(handle, "glTexCoord4f");
    glAPI->TexCoord4fv = resolveSymbol(handle, "glTexCoord4fv");
    glAPI->TexCoord4i = resolveSymbol(handle, "glTexCoord4i");
    glAPI->TexCoord4iv = resolveSymbol(handle, "glTexCoord4iv");
    glAPI->TexCoord4s = resolveSymbol(handle, "glTexCoord4s");
    glAPI->TexCoord4sv = resolveSymbol(handle, "glTexCoord4sv");
    glAPI->TexCoordPointer = resolveSymbol(handle, "glTexCoordPointer");
    glAPI->TexEnvf = resolveSymbol(handle, "glTexEnvf");
    glAPI->TexEnvfv = resolveSymbol(handle, "glTexEnvfv");
    glAPI->TexEnvi = resolveSymbol(handle, "glTexEnvi");
    glAPI->TexEnviv = resolveSymbol(handle, "glTexEnviv");
    glAPI->TexGend = resolveSymbol(handle, "glTexGend");
    glAPI->TexGendv = resolveSymbol(handle, "glTexGendv");
    glAPI->TexGenf = resolveSymbol(handle, "glTexGenf");
    glAPI->TexGenfv = resolveSymbol(handle, "glTexGenfv");
    glAPI->TexGeni = resolveSymbol(handle, "glTexGeni");
    glAPI->TexGeniv = resolveSymbol(handle, "glTexGeniv");
    glAPI->TexImage1D = resolveSymbol(handle, "glTexImage1D");
    glAPI->TexImage2D = resolveSymbol(handle, "glTexImage2D");
    glAPI->TexParameterf = resolveSymbol(handle, "glTexParameterf");
    glAPI->TexParameterfv = resolveSymbol(handle, "glTexParameterfv");
    glAPI->TexParameteri = resolveSymbol(handle, "glTexParameteri");
    glAPI->TexParameteriv = resolveSymbol(handle, "glTexParameteriv");
    glAPI->TexSubImage1D = resolveSymbol(handle, "glTexSubImage1D");
    glAPI->TexSubImage2D = resolveSymbol(handle, "glTexSubImage2D");
    glAPI->Translated = resolveSymbol(handle, "glTranslated");
    glAPI->Translatef = resolveSymbol(handle, "glTranslatef");
    glAPI->Vertex2d = resolveSymbol(handle, "glVertex2d");
    glAPI->Vertex2dv = resolveSymbol(handle, "glVertex2dv");
    glAPI->Vertex2f = resolveSymbol(handle, "glVertex2f");
    glAPI->Vertex2fv = resolveSymbol(handle, "glVertex2fv");
    glAPI->Vertex2i = resolveSymbol(handle, "glVertex2i");
    glAPI->Vertex2iv = resolveSymbol(handle, "glVertex2iv");
    glAPI->Vertex2s = resolveSymbol(handle, "glVertex2s");
    glAPI->Vertex2sv = resolveSymbol(handle, "glVertex2sv");
    glAPI->Vertex3d = resolveSymbol(handle, "glVertex3d");
    glAPI->Vertex3dv = resolveSymbol(handle, "glVertex3dv");
    glAPI->Vertex3f = resolveSymbol(handle, "glVertex3f");
    glAPI->Vertex3fv = resolveSymbol(handle, "glVertex3fv");
    glAPI->Vertex3i = resolveSymbol(handle, "glVertex3i");
    glAPI->Vertex3iv = resolveSymbol(handle, "glVertex3iv");
    glAPI->Vertex3s = resolveSymbol(handle, "glVertex3s");
    glAPI->Vertex3sv = resolveSymbol(handle, "glVertex3sv");
    glAPI->Vertex4d = resolveSymbol(handle, "glVertex4d");
    glAPI->Vertex4dv = resolveSymbol(handle, "glVertex4dv");
    glAPI->Vertex4f = resolveSymbol(handle, "glVertex4f");
    glAPI->Vertex4fv = resolveSymbol(handle, "glVertex4fv");
    glAPI->Vertex4i = resolveSymbol(handle, "glVertex4i");
    glAPI->Vertex4iv = resolveSymbol(handle, "glVertex4iv");
    glAPI->Vertex4s = resolveSymbol(handle, "glVertex4s");
    glAPI->Vertex4sv = resolveSymbol(handle, "glVertex4sv");
    glAPI->VertexPointer = resolveSymbol(handle, "glVertexPointer");
    glAPI->Viewport = resolveSymbol(handle, "glViewport");
#else

    XMesaAPI->InitDriver = XMesaInitDriver;
    XMesaAPI->ResetDriver = XMesaResetDriver;
    XMesaAPI->CreateVisual = XMesaCreateVisual;
    XMesaAPI->DestroyVisual = XMesaDestroyVisual;
    XMesaAPI->CreateContext = XMesaCreateContext;
    XMesaAPI->DestroyContext = XMesaDestroyContext;
    XMesaAPI->CreateWindowBuffer = XMesaCreateWindowBuffer;
    XMesaAPI->CreatePixmapBuffer = XMesaCreatePixmapBuffer;
    XMesaAPI->DestroyBuffer = XMesaDestroyBuffer;
    XMesaAPI->SwapBuffers = XMesaSwapBuffers;
    XMesaAPI->MakeCurrent = XMesaMakeCurrent;

    glAPI->Accum = __glAccum;
    glAPI->AlphaFunc = __glAlphaFunc;
    glAPI->AreTexturesResident = __glAreTexturesResident;
    glAPI->ArrayElement = __glArrayElement;
    glAPI->Begin = __glBegin;
    glAPI->BindTexture = __glBindTexture;
    glAPI->Bitmap = __glBitmap;
    glAPI->BlendFunc = __glBlendFunc;
    glAPI->CallList = __glCallList;
    glAPI->CallLists = __glCallLists;
    glAPI->Clear = __glClear;
    glAPI->ClearAccum = __glClearAccum;
    glAPI->ClearColor = __glClearColor;
    glAPI->ClearDepth = __glClearDepth;
    glAPI->ClearIndex = __glClearIndex;
    glAPI->ClearStencil = __glClearStencil;
    glAPI->ClipPlane = __glClipPlane;
    glAPI->Color3b = __glColor3b;
    glAPI->Color3bv = __glColor3bv;
    glAPI->Color3d = __glColor3d;
    glAPI->Color3dv = __glColor3dv;
    glAPI->Color3f = __glColor3f;
    glAPI->Color3fv = __glColor3fv;
    glAPI->Color3i = __glColor3i;
    glAPI->Color3iv = __glColor3iv;
    glAPI->Color3s = __glColor3s;
    glAPI->Color3sv = __glColor3sv;
    glAPI->Color3ub = __glColor3ub;
    glAPI->Color3ubv = __glColor3ubv;
    glAPI->Color3ui = __glColor3ui;
    glAPI->Color3uiv = __glColor3uiv;
    glAPI->Color3us = __glColor3us;
    glAPI->Color3usv = __glColor3usv;
    glAPI->Color4b = __glColor4b;
    glAPI->Color4bv = __glColor4bv;
    glAPI->Color4d = __glColor4d;
    glAPI->Color4dv = __glColor4dv;
    glAPI->Color4f = __glColor4f;
    glAPI->Color4fv = __glColor4fv;
    glAPI->Color4i = __glColor4i;
    glAPI->Color4iv = __glColor4iv;
    glAPI->Color4s = __glColor4s;
    glAPI->Color4sv = __glColor4sv;
    glAPI->Color4ub = __glColor4ub;
    glAPI->Color4ubv = __glColor4ubv;
    glAPI->Color4ui = __glColor4ui;
    glAPI->Color4uiv = __glColor4uiv;
    glAPI->Color4us = __glColor4us;
    glAPI->Color4usv = __glColor4usv;
    glAPI->ColorMask = __glColorMask;
    glAPI->ColorMaterial = __glColorMaterial;
    glAPI->ColorPointer = __glColorPointer;
    glAPI->CopyPixels = __glCopyPixels;
    glAPI->CopyTexImage1D = __glCopyTexImage1D;
    glAPI->CopyTexImage2D = __glCopyTexImage2D;
    glAPI->CopyTexSubImage1D = __glCopyTexSubImage1D;
    glAPI->CopyTexSubImage2D = __glCopyTexSubImage2D;
    glAPI->CullFace = __glCullFace;
    glAPI->DeleteLists = __glDeleteLists;
    glAPI->DeleteTextures = __glDeleteTextures;
    glAPI->DepthFunc = __glDepthFunc;
    glAPI->DepthMask = __glDepthMask;
    glAPI->DepthRange = __glDepthRange;
    glAPI->Disable = __glDisable;
    glAPI->DisableClientState = __glDisableClientState;
    glAPI->DrawArrays = __glDrawArrays;
    glAPI->DrawBuffer = __glDrawBuffer;
    glAPI->DrawElements = __glDrawElements;
    glAPI->DrawPixels = __glDrawPixels;
    glAPI->EdgeFlag = __glEdgeFlag;
    glAPI->EdgeFlagPointer = __glEdgeFlagPointer;
    glAPI->EdgeFlagv = __glEdgeFlagv;
    glAPI->Enable = __glEnable;
    glAPI->EnableClientState = __glEnableClientState;
    glAPI->End = __glEnd;
    glAPI->EndList = __glEndList;
    glAPI->EvalCoord1d = __glEvalCoord1d;
    glAPI->EvalCoord1dv = __glEvalCoord1dv;
    glAPI->EvalCoord1f = __glEvalCoord1f;
    glAPI->EvalCoord1fv = __glEvalCoord1fv;
    glAPI->EvalCoord2d = __glEvalCoord2d;
    glAPI->EvalCoord2dv = __glEvalCoord2dv;
    glAPI->EvalCoord2f = __glEvalCoord2f;
    glAPI->EvalCoord2fv = __glEvalCoord2fv;
    glAPI->EvalMesh1 = __glEvalMesh1;
    glAPI->EvalMesh2 = __glEvalMesh2;
    glAPI->EvalPoint1 = __glEvalPoint1;
    glAPI->EvalPoint2 = __glEvalPoint2;
    glAPI->FeedbackBuffer = __glFeedbackBuffer;
    glAPI->Finish = __glFinish;
    glAPI->Flush = __glFlush;
    glAPI->Fogf = __glFogf;
    glAPI->Fogfv = __glFogfv;
    glAPI->Fogi = __glFogi;
    glAPI->Fogiv = __glFogiv;
    glAPI->FrontFace = __glFrontFace;
    glAPI->Frustum = __glFrustum;
    glAPI->GenLists = __glGenLists;
    glAPI->GenTextures = __glGenTextures;
    glAPI->GetBooleanv = __glGetBooleanv;
    glAPI->GetClipPlane = __glGetClipPlane;
    glAPI->GetDoublev = __glGetDoublev;
    glAPI->GetError = __glGetError;
    glAPI->GetFloatv = __glGetFloatv;
    glAPI->GetIntegerv = __glGetIntegerv;
    glAPI->GetLightfv = __glGetLightfv;
    glAPI->GetLightiv = __glGetLightiv;
    glAPI->GetMapdv = __glGetMapdv;
    glAPI->GetMapfv = __glGetMapfv;
    glAPI->GetMapiv = __glGetMapiv;
    glAPI->GetMaterialfv = __glGetMaterialfv;
    glAPI->GetMaterialiv = __glGetMaterialiv;
    glAPI->GetPixelMapfv = __glGetPixelMapfv;
    glAPI->GetPixelMapuiv = __glGetPixelMapuiv;
    glAPI->GetPixelMapusv = __glGetPixelMapusv;
    glAPI->GetPointerv = __glGetPointerv;
    glAPI->GetPolygonStipple = __glGetPolygonStipple;
    glAPI->GetString = __glGetString;
    glAPI->GetTexEnvfv = __glGetTexEnvfv;
    glAPI->GetTexEnviv = __glGetTexEnviv;
    glAPI->GetTexGendv = __glGetTexGendv;
    glAPI->GetTexGenfv = __glGetTexGenfv;
    glAPI->GetTexGeniv = __glGetTexGeniv;
    glAPI->GetTexImage = __glGetTexImage;
    glAPI->GetTexLevelParameterfv = __glGetTexLevelParameterfv;
    glAPI->GetTexLevelParameteriv = __glGetTexLevelParameteriv;
    glAPI->GetTexParameterfv = __glGetTexParameterfv;
    glAPI->GetTexParameteriv = __glGetTexParameteriv;
    glAPI->Hint = __glHint;
    glAPI->IndexMask = __glIndexMask;
    glAPI->IndexPointer = __glIndexPointer;
    glAPI->Indexd = __glIndexd;
    glAPI->Indexdv = __glIndexdv;
    glAPI->Indexf = __glIndexf;
    glAPI->Indexfv = __glIndexfv;
    glAPI->Indexi = __glIndexi;
    glAPI->Indexiv = __glIndexiv;
    glAPI->Indexs = __glIndexs;
    glAPI->Indexsv = __glIndexsv;
    glAPI->Indexub = __glIndexub;
    glAPI->Indexubv = __glIndexubv;
    glAPI->InitNames = __glInitNames;
    glAPI->InterleavedArrays = __glInterleavedArrays;
    glAPI->IsEnabled = __glIsEnabled;
    glAPI->IsList = __glIsList;
    glAPI->IsTexture = __glIsTexture;
    glAPI->LightModelf = __glLightModelf;
    glAPI->LightModelfv = __glLightModelfv;
    glAPI->LightModeli = __glLightModeli;
    glAPI->LightModeliv = __glLightModeliv;
    glAPI->Lightf = __glLightf;
    glAPI->Lightfv = __glLightfv;
    glAPI->Lighti = __glLighti;
    glAPI->Lightiv = __glLightiv;
    glAPI->LineStipple = __glLineStipple;
    glAPI->LineWidth = __glLineWidth;
    glAPI->ListBase = __glListBase;
    glAPI->LoadIdentity = __glLoadIdentity;
    glAPI->LoadMatrixd = __glLoadMatrixd;
    glAPI->LoadMatrixf = __glLoadMatrixf;
    glAPI->LoadName = __glLoadName;
    glAPI->LogicOp = __glLogicOp;
    glAPI->Map1d = __glMap1d;
    glAPI->Map1f = __glMap1f;
    glAPI->Map2d = __glMap2d;
    glAPI->Map2f = __glMap2f;
    glAPI->MapGrid1d = __glMapGrid1d;
    glAPI->MapGrid1f = __glMapGrid1f;
    glAPI->MapGrid2d = __glMapGrid2d;
    glAPI->MapGrid2f = __glMapGrid2f;
    glAPI->Materialf = __glMaterialf;
    glAPI->Materialfv = __glMaterialfv;
    glAPI->Materiali = __glMateriali;
    glAPI->Materialiv = __glMaterialiv;
    glAPI->MatrixMode = __glMatrixMode;
    glAPI->MultMatrixd = __glMultMatrixd;
    glAPI->MultMatrixf = __glMultMatrixf;
    glAPI->NewList = __glNewList;
    glAPI->Normal3b = __glNormal3b;
    glAPI->Normal3bv = __glNormal3bv;
    glAPI->Normal3d = __glNormal3d;
    glAPI->Normal3dv = __glNormal3dv;
    glAPI->Normal3f = __glNormal3f;
    glAPI->Normal3fv = __glNormal3fv;
    glAPI->Normal3i = __glNormal3i;
    glAPI->Normal3iv = __glNormal3iv;
    glAPI->Normal3s = __glNormal3s;
    glAPI->Normal3sv = __glNormal3sv;
    glAPI->NormalPointer = __glNormalPointer;
    glAPI->Ortho = __glOrtho;
    glAPI->PassThrough = __glPassThrough;
    glAPI->PixelMapfv = __glPixelMapfv;
    glAPI->PixelMapuiv = __glPixelMapuiv;
    glAPI->PixelMapusv = __glPixelMapusv;
    glAPI->PixelStoref = __glPixelStoref;
    glAPI->PixelStorei = __glPixelStorei;
    glAPI->PixelTransferf = __glPixelTransferf;
    glAPI->PixelTransferi = __glPixelTransferi;
    glAPI->PixelZoom = __glPixelZoom;
    glAPI->PointSize = __glPointSize;
    glAPI->PolygonMode = __glPolygonMode;
    glAPI->PolygonOffset = __glPolygonOffset;
    glAPI->PolygonStipple = __glPolygonStipple;
    glAPI->PopAttrib = __glPopAttrib;
    glAPI->PopClientAttrib = __glPopClientAttrib;
    glAPI->PopMatrix = __glPopMatrix;
    glAPI->PopName = __glPopName;
    glAPI->PrioritizeTextures = __glPrioritizeTextures;
    glAPI->PushAttrib = __glPushAttrib;
    glAPI->PushClientAttrib = __glPushClientAttrib;
    glAPI->PushMatrix = __glPushMatrix;
    glAPI->PushName = __glPushName;
    glAPI->RasterPos2d = __glRasterPos2d;
    glAPI->RasterPos2dv = __glRasterPos2dv;
    glAPI->RasterPos2f = __glRasterPos2f;
    glAPI->RasterPos2fv = __glRasterPos2fv;
    glAPI->RasterPos2i = __glRasterPos2i;
    glAPI->RasterPos2iv = __glRasterPos2iv;
    glAPI->RasterPos2s = __glRasterPos2s;
    glAPI->RasterPos2sv = __glRasterPos2sv;
    glAPI->RasterPos3d = __glRasterPos3d;
    glAPI->RasterPos3dv = __glRasterPos3dv;
    glAPI->RasterPos3f = __glRasterPos3f;
    glAPI->RasterPos3fv = __glRasterPos3fv;
    glAPI->RasterPos3i = __glRasterPos3i;
    glAPI->RasterPos3iv = __glRasterPos3iv;
    glAPI->RasterPos3s = __glRasterPos3s;
    glAPI->RasterPos3sv = __glRasterPos3sv;
    glAPI->RasterPos4d = __glRasterPos4d;
    glAPI->RasterPos4dv = __glRasterPos4dv;
    glAPI->RasterPos4f = __glRasterPos4f;
    glAPI->RasterPos4fv = __glRasterPos4fv;
    glAPI->RasterPos4i = __glRasterPos4i;
    glAPI->RasterPos4iv = __glRasterPos4iv;
    glAPI->RasterPos4s = __glRasterPos4s;
    glAPI->RasterPos4sv = __glRasterPos4sv;
    glAPI->ReadBuffer = __glReadBuffer;
    glAPI->ReadPixels = __glReadPixels;
    glAPI->Rectd = __glRectd;
    glAPI->Rectdv = __glRectdv;
    glAPI->Rectf = __glRectf;
    glAPI->Rectfv = __glRectfv;
    glAPI->Recti = __glRecti;
    glAPI->Rectiv = __glRectiv;
    glAPI->Rects = __glRects;
    glAPI->Rectsv = __glRectsv;
    glAPI->RenderMode = __glRenderMode;
    glAPI->Rotated = __glRotated;
    glAPI->Rotatef = __glRotatef;
    glAPI->Scaled = __glScaled;
    glAPI->Scalef = __glScalef;
    glAPI->Scissor = __glScissor;
    glAPI->SelectBuffer = __glSelectBuffer;
    glAPI->ShadeModel = __glShadeModel;
    glAPI->StencilFunc = __glStencilFunc;
    glAPI->StencilMask = __glStencilMask;
    glAPI->StencilOp = __glStencilOp;
    glAPI->TexCoord1d = __glTexCoord1d;
    glAPI->TexCoord1dv = __glTexCoord1dv;
    glAPI->TexCoord1f = __glTexCoord1f;
    glAPI->TexCoord1fv = __glTexCoord1fv;
    glAPI->TexCoord1i = __glTexCoord1i;
    glAPI->TexCoord1iv = __glTexCoord1iv;
    glAPI->TexCoord1s = __glTexCoord1s;
    glAPI->TexCoord1sv = __glTexCoord1sv;
    glAPI->TexCoord2d = __glTexCoord2d;
    glAPI->TexCoord2dv = __glTexCoord2dv;
    glAPI->TexCoord2f = __glTexCoord2f;
    glAPI->TexCoord2fv = __glTexCoord2fv;
    glAPI->TexCoord2i = __glTexCoord2i;
    glAPI->TexCoord2iv = __glTexCoord2iv;
    glAPI->TexCoord2s = __glTexCoord2s;
    glAPI->TexCoord2sv = __glTexCoord2sv;
    glAPI->TexCoord3d = __glTexCoord3d;
    glAPI->TexCoord3dv = __glTexCoord3dv;
    glAPI->TexCoord3f = __glTexCoord3f;
    glAPI->TexCoord3fv = __glTexCoord3fv;
    glAPI->TexCoord3i = __glTexCoord3i;
    glAPI->TexCoord3iv = __glTexCoord3iv;
    glAPI->TexCoord3s = __glTexCoord3s;
    glAPI->TexCoord3sv = __glTexCoord3sv;
    glAPI->TexCoord4d = __glTexCoord4d;
    glAPI->TexCoord4dv = __glTexCoord4dv;
    glAPI->TexCoord4f = __glTexCoord4f;
    glAPI->TexCoord4fv = __glTexCoord4fv;
    glAPI->TexCoord4i = __glTexCoord4i;
    glAPI->TexCoord4iv = __glTexCoord4iv;
    glAPI->TexCoord4s = __glTexCoord4s;
    glAPI->TexCoord4sv = __glTexCoord4sv;
    glAPI->TexCoordPointer = __glTexCoordPointer;
    glAPI->TexEnvf = __glTexEnvf;
    glAPI->TexEnvfv = __glTexEnvfv;
    glAPI->TexEnvi = __glTexEnvi;
    glAPI->TexEnviv = __glTexEnviv;
    glAPI->TexGend = __glTexGend;
    glAPI->TexGendv = __glTexGendv;
    glAPI->TexGenf = __glTexGenf;
    glAPI->TexGenfv = __glTexGenfv;
    glAPI->TexGeni = __glTexGeni;
    glAPI->TexGeniv = __glTexGeniv;
    glAPI->TexImage1D = __glTexImage1D;
    glAPI->TexImage2D = __glTexImage2D;
    glAPI->TexParameterf = __glTexParameterf;
    glAPI->TexParameterfv = __glTexParameterfv;
    glAPI->TexParameteri = __glTexParameteri;
    glAPI->TexParameteriv = __glTexParameteriv;
    glAPI->TexSubImage1D = __glTexSubImage1D;
    glAPI->TexSubImage2D = __glTexSubImage2D;
    glAPI->Translated = __glTranslated;
    glAPI->Translatef = __glTranslatef;
    glAPI->Vertex2d = __glVertex2d;
    glAPI->Vertex2dv = __glVertex2dv;
    glAPI->Vertex2f = __glVertex2f;
    glAPI->Vertex2fv = __glVertex2fv;
    glAPI->Vertex2i = __glVertex2i;
    glAPI->Vertex2iv = __glVertex2iv;
    glAPI->Vertex2s = __glVertex2s;
    glAPI->Vertex2sv = __glVertex2sv;
    glAPI->Vertex3d = __glVertex3d;
    glAPI->Vertex3dv = __glVertex3dv;
    glAPI->Vertex3f = __glVertex3f;
    glAPI->Vertex3fv = __glVertex3fv;
    glAPI->Vertex3i = __glVertex3i;
    glAPI->Vertex3iv = __glVertex3iv;
    glAPI->Vertex3s = __glVertex3s;
    glAPI->Vertex3sv = __glVertex3sv;
    glAPI->Vertex4d = __glVertex4d;
    glAPI->Vertex4dv = __glVertex4dv;
    glAPI->Vertex4f = __glVertex4f;
    glAPI->Vertex4fv = __glVertex4fv;
    glAPI->Vertex4i = __glVertex4i;
    glAPI->Vertex4iv = __glVertex4iv;
    glAPI->Vertex4s = __glVertex4s;
    glAPI->Vertex4sv = __glVertex4sv;
    glAPI->VertexPointer = __glVertexPointer;
    glAPI->Viewport = __glViewport;

    handle = (void *)1;
#endif

    if (dlopen_error) {
	return NULL;
    }

    return handle;
}

void driMesaDestroyAPI(void *handle)
{
#ifdef GLX_USE_DLOPEN
    if (handle) dlclose(handle);
#endif
}

#endif
