/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/os2/os2_init.c,v 3.2 1996/01/30 15:26:31 dawes Exp $ */
/*
 * (c) Copyright 1994 by Holger Veit
 *			<Holger.Veit@gmd.de>
 * Modified 1996 Sebastien Marineau <marineau@genie.uottawa.ca>
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
#define INCL_KBD
#define INCL_VIO
#define INCL_DOSMISC
#define INCL_DOSPROCESS
#include "xf86.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"

VIOMODEINFO OriginalVideoMode;
void os2VideoNotify();
void os2HardErrorNotify();

void xf86OpenConsole()
{
    if (serverGeneration == 1)
    {
	HKBD fd;
	ULONG dummy;
	KBDHWID hwid;
	APIRET rc;
	int VioTid;

	ErrorF("xf86-OS/2: Console opened\n");
	OriginalVideoMode.cb=sizeof(VIOMODEINFO);
	rc=VioGetMode(&OriginalVideoMode,(HVIO)0);
	if(rc!=0) ErrorF("xf86-OS/2: Could not get original video mode. RC=%d\n",rc);
	xf86Info.consoleFd = -1;

/* Start up the VIO monitor thread */
	VioTid=_beginthread(os2VideoNotify,NULL,0x4000,(void *)NULL);
	ErrorF("xf86-OS/2: Started Vio thread, Tid=%d\n",VioTid);

/* Start up the hard-error VIO monitor thread */
	VioTid=_beginthread(os2HardErrorNotify,NULL,0x4000,(void *)NULL);
	ErrorF("xf86-OS/2: Started hard error Vio mode monitor thread, Tid=%d\n",VioTid);

/* Disable hard-errors through DosError */
	rc = DosSuppressPopUps(0x0001L,'c');     /* Disable popups */
	ErrorF("xf86-OS/2: Harderror popups disabled, redirected to c:\\popuplog.os2. Rc=%d\n",rc);

	/* grab the keyboard */
	rc = KbdGetFocus(0,0);
	if (rc != 0)
		FatalError("xf86OpenConsole: cannot grab kbd focus, rc=%d\n",rc);
	

	/* open the keyboard */
	rc = KbdOpen(&fd);
	if (rc != 0)
		FatalError("xf86OpenConsole: cannot open keyboard, rc=%d\n",rc);
	xf86Info.consoleFd = fd;

	ErrorF("xf86-OS/2: Keyboard opened\n");

	/* assign logical keyboard */
	KbdFreeFocus(0);
	rc = KbdGetFocus(0,fd);
	if (rc != 0)
		FatalError("xf86OpenConsole: cannot set local kbd focus, rc=%d\n",rc);

	rc = KbdSetCp(0,0,fd);
	if(rc != 0)
		FatalError("xf86OpenCOnsole: cannot set keyboard codepage, rc=%d\n",rc);
	rc = KbdGetHWID(&hwid, fd);
	if (rc == 0) {
		switch (hwid.idKbd) {
		default:
		case 0xab54: /* 88/89 key */
		case 0:	/*unknown*/
		case 1: /*real AT 84 key*/
			xf86Info.kbdType = KB_84; break;
		case 0xab85: /* 122 key */
			FatalError("OS/2 has detected an extended 122key keyboard: unsupported!\n");
		case 0xab41: /* 101/102 key */
			xf86Info.kbdType = KB_101; break;
		}				
	} else
		xf86Info.kbdType = KB_84; /*defensive*/

	xf86Config(FALSE); /* Read XF86Config */
    }
    return;
}

void xf86CloseConsole()
{
	APIRET rc;

	if (xf86Info.consoleFd != -1) {
		KbdClose(xf86Info.consoleFd);
	}
	VioSetMode(&OriginalVideoMode,(HVIO)0);
	rc = DosSuppressPopUps(0x0000L,'c');    /* Reenable popups */
	ErrorF("xf86-OS/2: Harderror popups enabled. Rc=%d\n",rc);
	return;
}

/* ARGSUSED */
int xf86ProcessArgument (argc, argv, i)
int argc;
char *argv[];
int i;
{
	return(0);
}

void xf86UseMsg()
{
	return;
}

char *__SrvRedirRoot(char *fname)
{
    /* This adds a further redirection by allowing the ProjectRoot
     * to be prepended by the content of the envvar X11ROOT.
     * This is for the purpose to move the whole X11 stuff to a different
     * disk drive.
     * The feature was added despite various environment variables
     * because not all file opens respect them.
     */
    static char redirname[300]; /* enough for long filenames */
    char *root;

    /* if name does not start with /, assume it is not root-based */
    if (fname==0 || !(fname[0]=='/' || fname[0]=='\\'))
	return fname;

    root = (char*)getenv("X11ROOT");
    if (root==0 || 
	(fname[1]==':' && isalpha(fname[0]) ||
        (strlen(fname)+strlen(root)+2) > 300))
	return fname;
    sprintf(redirname,"%s%s",root,fname);
    return access(redirname,R_OK)==0 ? redirname : fname;
}

char *__SrvRedirRoot1(char *format, char *arg1, char *arg2, char *arg3)
{
    /* this first constructs a name from a format and up to three
     * components, then adds a path
     */
    char buf[300];
    sprintf(buf,format,arg1,arg2,arg3);
    return __SrvRedirRoot(buf);
}
