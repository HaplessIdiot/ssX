/*
 * Mesa 3-D graphics library
 * Version:  3.0
 * Copyright (C) 1995-1998  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include <allegro.h>
#include "GL/glut.h"
#include "GL/amesa.h"
#include "internal.h"


static AMesaVisual  visual  = NULL;
static AMesaContext context = NULL;
static AMesaBuffer  buffer  = NULL;


int APIENTRY glutCreateWindow(const char *title)
    {
    GLboolean ok;

    visual  = AMesaCreateVisual(g_display_mode & GLUT_DOUBLE,
                                DEFAULT_DEPTH,
                                g_display_mode & GLUT_DEPTH   ? DEPTH_SIZE   : 0,
                                g_display_mode & GLUT_STENCIL ? STENCIL_SIZE : 0,
                                g_display_mode & GLUT_ACCUM   ? ACCUM_SIZE   : 0);
    if (!visual)
        return GL_FALSE;

    context = AMesaCreateContext(visual, NULL);
    if (!context)
        {
        AMesaDestroyVisual(visual);
        return GL_FALSE;
        }

    buffer = AMesaCreateBuffer(visual, g_width, g_height);
    if (!buffer)
        {
        AMesaDestroyContext(context);
        AMesaDestroyVisual(visual);
        return GL_FALSE;
        }

    ok = AMesaMakeCurrent(context, buffer);
    if (!ok)
        {
        AMesaDestroyContext(context);
        AMesaDestroyVisual(visual);
        return GL_FALSE;
        }

    return GL_TRUE;
    }


int APIENTRY glutCreateSubWindow(int win, int x, int y, int width, int height)
    {
    return GL_FALSE;
    }


void APIENTRY glutDestroyWindow(int win)
    {
    }


void APIENTRY glutPostRedisplay(void)
    {
    g_redisplay = GL_TRUE;
    }


void APIENTRY glutSwapBuffers(void)
    {
    if (g_mouse) scare_mouse();
    AMesaSwapBuffers(buffer);
    if (g_mouse) unscare_mouse();
    }


int APIENTRY glutGetWindow(void)
    {
    return 0;
    }


void APIENTRY glutSetWindow(int win)
    {
    }


void APIENTRY glutSetWindowTitle(const char *title)
    {
    }


void APIENTRY glutSetIconTitle(const char *title)
    {
    }


void APIENTRY glutPositionWindow(int x, int y)
    {
    }


void APIENTRY glutReshapeWindow(int width, int height)
    {
    }


void APIENTRY glutPopWindow(void)
    {
    }


void APIENTRY glutPushWindow(void)
    {
    }


void APIENTRY glutIconifyWindow(void)
    {
    }


void APIENTRY glutShowWindow(void)
    {
    }


void APIENTRY glutHideWindow(void)
    {
    }
