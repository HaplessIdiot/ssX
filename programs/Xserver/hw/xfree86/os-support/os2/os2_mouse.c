/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/os2/os2_mouse.c,v 3.3 1996/02/09 08:20:57 dawes Exp $ */
/*
 * (c) Copyright 1994 by Holger Veit
 *			<Holger.Veit@gmd.de>
 * Modified (c) 1996 Sebastien Marineau <marineau@genie.uottawa.ca>
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

HMOU hMouse=65535;
BOOL HandleValid=FALSE;
extern BOOL SwitchedToWPS;
extern CARD32 LastSwitchTime;

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
DeviceIntPtr pPointer;
int what;
{
	APIRET rc=0;
	USHORT nbutton,state;
	unsigned char *map;
	int i;

	switch (what) {
	case DEVICE_INIT: 
		pPointer->public.on = FALSE;
		if(hMouse==65535) rc = MouOpen((PSZ)0, &hMouse);
		if (rc != 0)
			FatalError("Cannot open mouse, rc=%d\n",rc);
		xf86Info.mseFd = -1;

		/* flush mouse queue */
		MouFlushQue(hMouse);

		/* check buttons */
		rc = MouGetNumButtons(&nbutton,hMouse);
		if (rc == 0)
			ErrorF("OsMouse has %d button(s).\n",nbutton);
		if(nbutton==2) nbutton++;
		map = (unsigned char *) xalloc(nbutton + 1);
		if (map == (unsigned char *) NULL)
			FatalError("Failed to allocate OsMouse map structure\n");

		for (i = 1; i <= nbutton; i++)
			map[i] = i;

		InitPointerDeviceStruct((DevicePtr)pPointer, map, nbutton,
			GetMotionEvents, (PtrCtrlProcPtr)xf86MseCtrl, 0);

		xfree(map);
		HandleValid=TRUE;
		break;
      
	case DEVICE_ON:
		ErrorF("xf86-OS/2: mouse DEVICE_ON being called, hMouse=%d\n",hMouse);
		/*AddEnabledDevice(xf86Info.mseFd);*/
		if(!HandleValid) return(-1);
		xf86Info.lastButtons = 0;
		xf86Info.emulateState = 0;
		pPointer->public.on = TRUE;
		state = 0x300;
		rc=MouSetDevStatus(&state,hMouse);
		state = 0x7f;
		rc=MouSetEventMask(&state,hMouse);
		MouFlushQue(hMouse);
		break;
      
	case DEVICE_CLOSE:
	case DEVICE_OFF:
		ErrorF("xf86-OS/2: mouse DEVICE_OFF/DEVICE_CLOSE being called, hMouse=%d\n",hMouse);
		if(!HandleValid) return(-1);
		pPointer->public.on = FALSE;
		state = 0x300;
		MouSetDevStatus(&state,hMouse);
		state = 0;
		MouSetEventMask(&state,hMouse);
		/*RemoveEnabledDevice(xf86Info.mseFd);*/
		if (what == DEVICE_CLOSE) {
			/* MouClose(hMouse);
			hMouse=65535;
			xf86Info.mseFd = -1;
		        HandleValid=FALSE;  */ /* Comment out for now as this seems to break server */
		}
		break;
	}
	return Success;
}

void xf86OsMouseEvents()
{
	MOUEVENTINFO mev;
	MOUQUEINFO mqif;
	APIRET rc;	
	int buttons;
	int state;
	int i;
	USHORT waitflg = 0;

	if(!HandleValid) return;
	if((rc=MouGetNumQueEl(&mqif,hMouse))!=0){
	     ErrorF("xf86-OS/2: Bad return code from MouGetNumQueEl, rc=%d, hMouse=%d\n",rc,hMouse);
	     return;
	}
	if(mqif.cEvents<1) return;  /* There are no events */
	for(i=0;i<mqif.cEvents;i++){
	     if((rc=MouReadEventQue(&mev,&waitflg,hMouse))!=0){
	          ErrorF("xf86-OS/2: Bad return code from MouReadEventQue, rc=%d\n",rc);
	          return;
	     }
		waitflg = 0;
		state=mev.fs;
#if DEBUG
			ErrorF("mouse event: fs(%x) row(%d) col(%d)\n",
				state, mev.row, mev.col);
#endif

		/* Contrary to other systems, OS/2 has mouse buttons *
		 * in the proper order, so we reverse them before    *
		 * sending the event.                                */
		 
			buttons = ((state & 0x06) ? 4 : 0) |
				  ((state & 0x18) ? 1 : 0) |
				  ((state & 0x60) ? 2 : 0);
			xf86PostMseEvent(buttons, mev.col, mev.row);
	}
	xf86Info.inputPending = TRUE;
}

int os2MouseQueueQuery()
{
	     /* Now we check for activity on mouse handles */
MOUQUEINFO mouQueInfo;
APIRET rc;
	     if(!HandleValid) return(1);
	     rc=MouGetNumQueEl(&mouQueInfo,hMouse);
	     if(rc!=0){
		  ErrorF("xf86-OS/2: Mouse queue check has rc=%d\n",rc);
		  return(-1); /* We could also check here for focus */
	     }
	     
	     if (mouQueInfo.cEvents>0){     /* Something in mouse queue! */
		  return(0);  /* Will this work? */
	     }

return(1);
}

