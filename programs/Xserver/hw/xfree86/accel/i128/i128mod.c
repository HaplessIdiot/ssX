/* $XConsortium: $ */
/*
 * Copyright 1995 by Robin Cutshaw <robin@XFree86.Org>
 * Copyright 1997 by Metro Link Incorporated ("Metro Link")
 *
 *                         All Rights Reserved
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Robin Cutshaw or Metro Link
 * ("copyright holders") not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior permission.
 * The copyright holders make no representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
 * IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: $ */

#if defined(XFree86LOADER)

#include "xf86_HWlib.h"
#include "xf86_PCI.h"
#include "xf86_Config.h"
#include "xf86Version.h"
#define _NO_XF86_PROTOTYPES
#include "xf86.h"

extern ScrnInfoRec i128InfoRec;

#define MAX_I128_CLOCK		175000

extern void *videoDrivers[];

int i128MaxClock = MAX_I128_CLOCK;

int i128ValidTokens[] =
{
  STATICGRAY,
  GRAYSCALE,
  STATICCOLOR,
  PSEUDOCOLOR,
  TRUECOLOR,
  DIRECTCOLOR,
  MODES,
  OPTION,
  VIDEORAM,
  VIEWPORT,
  VIRTUAL,
  CLOCKPROG,
  INSTANCE,
  -1
};


ScrnInfoRec *
ServerInit(void)
{
return &i128InfoRec;
}


XF86ModuleVersionInfo i128VersRec =
{
	"libi128.a",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00010001,
	{0,0,0,0},
};

void
ModuleInit(data,magic)
    pointer	* data;
    INT32	* magic;
{
    static int cnt = 0;
    extern Bool xf86issvgatype;

    switch(cnt++)
    {
    case 0:
	* data = (pointer) &i128VersRec;
	* magic= MAGIC_VERSION;
	break;
    case 1:
        * data = (pointer) &i128InfoRec;
        * magic= MAGIC_ADD_VIDEO_CHIP_REC;
        break;
    case 2:
	* data = (pointer) "libmfb";
	* magic= MAGIC_LOAD;
	break;
    case 3:
	* data = (pointer) "libcfb";
	* magic= MAGIC_LOAD;
	break;
    case 4:
	* data = (pointer) "libcfb16";
	* magic= MAGIC_LOAD;
	break;
    case 5:
	* data = (pointer) "libcfb32";
	* magic= MAGIC_LOAD;
	break;
    case 6:
	* data = (pointer) "xaa8";
	* magic= MAGIC_LOAD;
	break;
    case 7:
	* data = (pointer) "xaa16";
	* magic= MAGIC_LOAD;
	break;
    case 8:
	* data = (pointer) "xaa32";
	* magic= MAGIC_LOAD;
	break;
    case 9:
	* data = (pointer) "libxaa";
	* magic= MAGIC_LOAD;
	xf86xaaloaded = TRUE;
	break;
    case 10:
	* data = (pointer) "libxf86cache";
	* magic= MAGIC_LOAD;
	break;
    default:
	xf86issvgatype = FALSE;
        * magic= MAGIC_DONE;
        break;
    }
    return;
}
#endif
