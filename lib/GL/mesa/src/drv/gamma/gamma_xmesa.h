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
 * $PI: xc/lib/GL/mesa/src/drv/gamma/gamma_xmesa.h,v 1.1 1999/04/05 05:24:36 martin Exp $
 */

#ifndef _GAMMA_XMESA_H_
#define _GAMMA_XMESA_H_

#ifdef GLX_DIRECT_RENDERING

#include "GL/xmesa.h"

/*
** The "__GAMMA_" prefix should be removed after we implement dynamic
** loading in the DRI.
*/

GLboolean __GAMMA_XMesaInitDriver(__DRIscreenPrivate *driScrnPriv);
void __GAMMA_XMesaResetDriver(__DRIscreenPrivate *driScrnPriv);
XMesaVisual __GAMMA_XMesaCreateVisual(XMesaDisplay *display,
				      XMesaVisualInfo visinfo,
				      GLboolean rgb_flag,
				      GLboolean alpha_flag,
				      GLboolean db_flag,
				      GLboolean stereo_flag,
				      GLboolean ximage_flag,
				      GLint depth_size,
				      GLint stencil_size,
				      GLint accum_size,
				      GLint level);
void __GAMMA_XMesaDestroyVisual(XMesaVisual v);
XMesaContext __GAMMA_XMesaCreateContext(XMesaVisual v, XMesaContext share_list,
					__DRIcontextPrivate *driContextPriv);
void __GAMMA_XMesaDestroyContext(XMesaContext c);
XMesaBuffer __GAMMA_XMesaCreateWindowBuffer(XMesaVisual v, XMesaWindow w,
					    __DRIdrawablePrivate *driDrawPriv);
XMesaBuffer __GAMMA_XMesaCreatePixmapBuffer(XMesaVisual v, XMesaPixmap p,
					    XMesaColormap c,
					    __DRIdrawablePrivate *driDrawPriv);
void __GAMMA_XMesaDestroyBuffer(XMesaBuffer b);
void __GAMMA_XMesaSwapBuffers(XMesaBuffer b);
GLboolean __GAMMA_XMesaMakeCurrent(XMesaContext c, XMesaBuffer b);
#endif

#endif /* _GAMMA_XMESA_H_ */
