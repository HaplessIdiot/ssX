/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/os2/os2_mouse.c,v 3.0 1995/03/11 14:15:27 dawes Exp $ */
/*
 * (c) Copyright 1994 by Holger Veit
 *			<Holger.Veit@gmd.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a 
 * copy of this software and associated documentation files (the "Software"), 
 * to deal in the Software without restriction, including without limitation 
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, 
 * and/or sell copies of the Software, and to permit persons to whom the 
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * HOLGER VEIT  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF 
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
 * SOFTWARE.
 * 
 * Except as contained in this notice, the name of Holger Veit shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Holger Veit.
 *
 */

#include "X.h"
#include "Xmd.h"
#include "input.h"
#include "scrnintstr.h"

#include "compiler.h"

#define I_NEED_OS2_H
#define INCL_DOSFILEMGR
#define INCL_MOU
#include "xf86.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"


#include "xf86_OSlib.h"
#include "xf86Procs.h"
#include "xf86_Config.h"

int xf86MouseOff(doclose)
Bool doclose;
{
	return -1;
}

void xf86SetMouseSpeed(old, new, cflag)
int old;
int new;
unsigned cflag;
{
	/* not required */
}

void xf86OsMouseOption(token, lex_ptr)
int token;
pointer lex_ptr;
{
	/* no options, anything seen ignored */
}

/* almost everything stolen from sco_mouse.c */
int xf86OsMouseProc(pPointer, what)
DevicePtr pPointer;
int what;
{
	APIRET rc;
	HMOU fd;
	USHORT nbutton,state;
	unsigned char *map;
	int i;

	switch (what) {
	case DEVICE_INIT: 
		pPointer->on = FALSE;
		rc = MouOpen((PSZ)0, &fd);
		if (rc != 0)
			FatalError("Cannot open mouse, rc=%d\n",rc);
		xf86Info.mseFd = fd;

		/* flush mouse queue */
		MouFlushQue(fd);

		/* check buttons */
		rc = MouGetNumButtons(&nbutton,fd);
		if (rc == 0)
			ErrorF("OsMouse has %d button(s).\n",nbutton);

		map = (unsigned char *) xalloc(nbutton + 1);
		if (map == (unsigned char *) NULL)
			FatalError("Failed to allocate OsMouse map structure\n");

		for (i = 1; i <= nbutton; i++)
			map[i] = i;

		InitPointerDeviceStruct(pPointer, map, nbutton,
			GetMotionEvents, (PtrCtrlProcPtr)xf86MseCtrl, 0);

		xfree(map);
		break;
      
	case DEVICE_ON:
		AddEnabledDevice(xf86Info.mseFd);
		xf86Info.lastButtons = 0;
		xf86Info.emulateState = 0;
		pPointer->on = TRUE;
		state = 0x300;
		rc=MouSetDevStatus(&state,xf86Info.mseFd);
		state = 0x7f;
		rc=MouSetEventMask(&state,xf86Info.mseFd);
		MouFlushQue(xf86Info.mseFd);
		break;
      
	case DEVICE_CLOSE:
	case DEVICE_OFF:
		pPointer->on = FALSE;
		state = 0x300;
		MouSetDevStatus(&state,xf86Info.mseFd);
		state = 0;
		MouSetEventMask(&state,xf86Info.mseFd);
		RemoveEnabledDevice(xf86Info.mseFd);
		if (what == DEVICE_CLOSE) {
			MouClose(xf86Info.mseFd);
			xf86Info.mseFd = -1;
		}
		break;
	}
	return Success;
}

void xf86OsMouseEvents()
{
	MOUEVENTINFO mev;
	APIRET rc;	
	int buttons;
	int state;
	USHORT waitflg = 0;
	
	while ( (rc=MouReadEventQue(&mev,&waitflg,xf86Info.mseFd)) == 0) {
		waitflg = 0;
		if ((state=mev.fs) != 0) {
/*#if DEBUG*/
			ErrorF("mouse event: fs(%x) row(%d) col(%d)\n",
				state, mev.row, mev.col);
/*#endif*/	
			buttons = ((state & 0x06) ? 1 : 0) |
				  ((state & 0x18) ? 2 : 0) |
				  ((state & 0x60) ? 4 : 0);
			xf86PostMseEvent(buttons, mev.col, mev.row);
		} else break;
	}
	xf86Info.inputPending = TRUE;
}
