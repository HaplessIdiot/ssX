/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/os2/os2_VTsw.c,v 3.0 1996/01/30 15:26:30 dawes Exp $ */
/*
 * Copyright 1993 by David Wexelblat <dwex@goblin.org>
 * Modified 1996 by Sebastien Marineau <marineau@genie.uottawa.ca>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */
/* $XConsortium: VTsw_noop.c /main/2 1995/11/13 06:14:57 kaleb $ */

#define NEED_EVENTS
#include "X.h"
#include "input.h"
#include "scrnintstr.h"

#define I_NEED_OS2_H
#define INCL_WINSWITCHLIST
#define INCL_VIO
#define INCL_DOSPROCESS
#define INCL_DOSSEMAPHORES
#undef RT_FONT
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86_OSlib.h"
#include "atKeynames.h"

BOOL SwitchedToWPS=FALSE;
HSWITCH GetDesktopSwitchHandle();
void os2PostKbdEvent();
HEV hevServerHasFocus;

/*
 * Added OS/2 code to handle switching back to WPS
 */

Bool xf86VTSwitchPending()
{
	return(xf86Info.vtRequestsPending ? TRUE : FALSE);
}

Bool xf86VTSwitchAway()
{
        HSWITCH hSwitch;
        APIRET rc;

        xf86Info.vtRequestsPending=FALSE;
        hSwitch=GetDesktopSwitchHandle();
        ErrorF("xf86-OS/2: Switching to desktop. Handle: %d\n",hSwitch);
        if(hSwitch==NULLHANDLE) return(FALSE);
        SwitchedToWPS=TRUE;
	rc=WinSwitchToProgram(hSwitch);
	rc = DosSuppressPopUps(0x0000L,'c');	/* Disable popups */
	ErrorF("xf86-OS/2: Harderror popups enabled. Rc=%d\n",rc);
	usleep(30000);
	return(TRUE);
}

Bool xf86VTSwitchTo()
{
	APIRET rc;

        xf86Info.vtRequestsPending=FALSE;
        SwitchedToWPS=FALSE;
        ErrorF("Switching back to server. \n");
	ErrorF("xf86-OS/2: Switching back to server. \n");
	rc = DosSuppressPopUps(0x0001L,'c');     /* Disable popups */
	ErrorF("xf86-OS/2: Harderror popups disabled, redirected to c:\\popuplog.os2. Rc=%d\n",rc);
	/* We reset the state of the control key */
	os2PostKbdEvent(KEY_LCtrl,1);
	os2PostKbdEvent(KEY_LCtrl,0);
	return(TRUE);
}

HSWITCH GetDesktopSwitchHandle()
{

  PSWBLOCK pswb;
  ULONG ulcEntries;
  ULONG usSize;
  HSWITCH hSwitch;
/* Get the switch list information */

  ulcEntries=WinQuerySwitchList(0, NULL, 0);
  usSize=sizeof(SWBLOCK)+sizeof(HSWITCH)+(ulcEntries+4)*(long)sizeof(SWENTRY);
  /* Allocate memory for list */
  if ((pswb=malloc((unsigned)usSize)) != NULL)
  {
    /* Put the info in the list */
    ulcEntries=WinQuerySwitchList(0, pswb, (USHORT)(usSize-sizeof(SWENTRY)));

    /* Return first entry in list, usually the desktop */

    hSwitch=pswb->aswentry[0].hswitch;
    if (pswb)
    free(pswb);
    return(hSwitch);
    }
return(NULLHANDLE);
}


/* This function is run as a thread and will notify of switch-to/switch-away events */
void os2VideoNotify(arg)
void * arg;
{
   USHORT Indic;
   USHORT NotifyType;
   APIRET rc;
   ULONG postCount;

	rc=DosCreateEventSem(NULL,&hevServerHasFocus,0L,FALSE);
	ErrorF("xf86-OS/2: Screen access semaphore created. RC=%d\n",rc);
	rc=DosPostEventSem(hevServerHasFocus);

	while(1) {
	  Indic=0;
	  rc=VioSavRedrawWait(Indic,&NotifyType,(HVIO)0);

/* Here we handle the semaphore used to indicate wether we have screen access */
	  if(NotifyType==0) rc=DosResetEventSem(hevServerHasFocus,&postCount);
	  if(NotifyType==1) rc=DosPostEventSem(hevServerHasFocus);

	  if(NotifyType==1){
		if (SwitchedToWPS) {
			xf86Info.vtRequestsPending=TRUE;
		} else {
			ErrorF("xf86-OS/2: abnormal switching from server detected\n"); 
		}
	  }
          if((NotifyType==0)&&(!SwitchedToWPS))
                ErrorF("xf86-OS/2: abnormal switching away from server!\n");
	} /* endwhile */

/* End of thread */
}

/* This function is run as a thread and will notify of hard-error events */
void os2HardErrorNotify(arg)
void * arg;
{
   USHORT Indic;
   USHORT NotifyType;
   APIRET rc;

	while(1) {
	   Indic=0;
	   rc=VioModeWait(Indic,&NotifyType,(HVIO)0);
	   if(NotifyType==0){
		ErrorF("xf86-OS/2: hard error popup notification received. Attempting to recover.\n");
		/* Normally we should never get to the above. Call server reset */
		AutoResetServer(0);
	   }
	} /* endwhile */

/* End of thread */
}

void os2ServerVideoAccess()
{
   APIRET rc;

/* Wait for screen access. This is called at server reset. */
        rc=DosWaitEventSem(hevServerHasFocus,SEM_INDEFINITE_WAIT);
        SwitchedToWPS=FALSE;  /* In case server has reset while we were switched to WPS */
}
