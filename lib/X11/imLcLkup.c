/* $XConsortium: imLcLkup.c,v 1.5 94/03/26 17:00:36 rws Exp $ */
/******************************************************************

              Copyright 1992 by Fuji Xerox Co., Ltd.
              Copyright 1992, 1994  by FUJITSU LIMITED

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and
that both that copyright notice and this permission notice appear
in supporting documentation, and that the name of Fuji Xerox,
FUJITSU LIMITED not be used in advertising or publicity pertaining
to distribution of the software without specific, written prior
permission.  Fuji Xerox, FUJITSU LIMITED  make no representations
about the suitability of this software for any purpose.
It is provided "as is" without express or implied warranty.

FUJI XEROX, FUJITSU LIMITED DISCLAIM ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL FUJI XEROX, 
FUJITSU LIMITED BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA
OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

  Author: Kazunori Nishihara   Fuji Xerox
          Takashi Fujiwara     FUJITSU LIMITED 
                               fujiwara@a80.tech.yk.fujitsu.co.jp

******************************************************************/

#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include "Xlibint.h"
#include "Xlcint.h"
#include "Ximint.h"

Public int
_XimLocalMbLookupString(xic, ev, buffer, bytes, keysym, status)
    XIC		 xic;
    XKeyEvent	*ev;
    char	*buffer;
    int		 bytes;
    KeySym	 *keysym;
    Status	 *status;
{
    Xic		 ic = (Xic)xic;
    int		 ret;

    if(ev->type != KeyPress) {
	if(status) *status = XLookupNone;
	return(0);
    }
    if(ev->keycode == 0) { /* Composed Event */
	ret = strlen(ic->private.local.composed->mb);
	if(ret > bytes) {
	    if(status) *status = XBufferOverflow;
	    return(ret);
	}
	memcpy(buffer, ic->private.local.composed->mb, ret);
	if(keysym) *keysym = ic->private.local.composed->ks;
	if (ret > 0) {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupChars;
	    } else {
		if(status) *status = XLookupChars;
	    }
	} else {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupKeySym;
	    } else {
		if(status) *status = XLookupNone;
	    }
	}
	return (ret);
    } else { /* Throughed Event */
	ret = _XimLookupMBText(ic, ev, (unsigned char *)buffer, bytes, keysym,
			       NULL);
	if(ret > 0) {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupBoth;
	    } else {
		if(status) *status = XLookupChars;
	    }
	} else {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupKeySym;
	    } else {
		if(status) *status = XLookupNone;
	    }
	}
    }
    return (ret);
}

Public int
_XimLocalWcLookupString(xic, ev, buffer, wlen, keysym, status)
    XIC		  xic;
    XKeyEvent	 *ev;
    wchar_t	 *buffer;
    int		  wlen;
    KeySym	 *keysym;
    Status	 *status;
{
    Xic		 ic = (Xic)xic;
    int		 ret;

    if(ev->type != KeyPress) {
	if(status) *status = XLookupNone;
	return(0);
    }
    if(ev->keycode == 0) { /* Composed Event */
	ret = _Xwcslen(ic->private.local.composed->wc);
	if(ret > wlen) {
	    if(status) *status = XBufferOverflow;
	    return (ret);
	}
	memcpy((char *)buffer, (char *)ic->private.local.composed->wc,
	       ret * sizeof(wchar_t));
	if(keysym) *keysym = ic->private.local.composed->ks;
	if (ret > 0) {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupChars;
	    } else {
		if(status) *status = XLookupChars;
	    }
	} else {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupKeySym;
	    } else {
		if(status) *status = XLookupNone;
	    }
	}
	return (ret);
    } else { /* Throughed Event */
	ret = _XimLookupWCText(ic, ev, buffer, wlen, keysym, NULL);
	if(ret > 0) {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupBoth;
	    } else {
		if(status) *status = XLookupChars;
	    }
	} else {
	    if(keysym && *keysym != NoSymbol) {
		if(status) *status = XLookupKeySym;
	    } else {
		if(status) *status = XLookupNone;
	    }
	}
    }
    return (ret);
}
