/*
 * $XFree86: $
 *
 * Copyright © 2002 Keith Packard, member of The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Keith Packard not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Keith Packard makes no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * KEITH PACKARD DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _XFIXESINT_H_
#define _XFIXESINT_H_

#define NEED_EVENTS
#include "X.h"
#include "Xproto.h"
#include "misc.h"
#include "os.h"
#include "dixstruct.h"
#include "extnsionst.h"
#include "xfixesproto.h"
#include "windowstr.h"
#include "selection.h"

extern unsigned char	XFixesReqCode;
extern int		XFixesEventBase;

int
ProcXFixesChangeSaveSet(ClientPtr client);
    
int
SProcXFixesChangeSaveSet(ClientPtr client);
    
int
ProcXFixesSelectSelectionInput (ClientPtr client);

int
SProcXFixesSelectSelectionInput (ClientPtr client);

void
SXFixesSelectionNotifyEvent (xXFixesSelectionNotifyEvent *from,
			     xXFixesSelectionNotifyEvent *to);
Bool
XFixesSelectionInit (void);

void
XFixesExtensionInit(void);

int
ProcXFixesSelectCursorInput (ClientPtr client);

int
SProcXFixesSelectCursorInput (ClientPtr client);

void
SXFixesCursorNotifyEvent (xXFixesCursorNotifyEvent *from,
			  xXFixesCursorNotifyEvent *to);

int
ProcXFixesGetCursorImage (ClientPtr client);

int
SProcXFixesGetCursorImage (ClientPtr client);

Bool
XFixesCursorInit (void);
    
#endif /* _XFIXESINT_H_ */
