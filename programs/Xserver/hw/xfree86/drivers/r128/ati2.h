/* $XFree86$ */
/**************************************************************************

Copyright 2000 ATI Technologies Inc. and VA Linux Systems, Inc.,
                                         Sunnyvale, California.
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ATI, VA LINUX SYSTEMS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *
 */

#ifndef _ATI2_H_
#define _ATI2_H_

				/* ATI2_NAME is used for the server-side
                                   ddx driver, the client-side DRI driver,
                                   and the kernel-level DRM driver. */
#define ATI2_NAME "ati2"
#define ATI2_VERSION_MAJOR 4
#define ATI2_VERSION_MINOR 0
#define ATI2_VERSION_PATCH 0
#define ATI2_VERSION ((ATI2_VERSION_MAJOR << 16)  \
		      | (ATI2_VERSION_MINOR << 8) \
		      |  ATI2_VERSION_PATCH)

#endif
