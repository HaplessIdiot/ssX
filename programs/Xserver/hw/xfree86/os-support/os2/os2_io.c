/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/os2/os2_io.c,v 3.6 1996/02/22 05:12:20 dawes Exp $ */
/*
 * (c) Copyright 1994 by Holger Veit
 *			<Holger.Veit@gmd.de>
 * Modified 1996 by Sebastien Marineau <marineau@genie.uottawa.ca>
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
#include "Xpoll.h"
#include "compiler.h"
#include <time.h>

#define I_NEED_OS2_H
#define INCL_DOSPROCESS
#define INCL_KBD
#define INCL_MOU
#include "xf86.h"
#include "xf86Procs.h"
#include "xf86_OSlib.h"

int os2MouseQueueQuery();
int os2KbdQueueQuery();
void os2RecoverFromPopup();
void os2CheckPopupPending();
extern BOOL os2PopupErrorPending;

/***************************************************************************/

void xf86SoundKbdBell(loudness, pitch, duration)
int loudness;
int pitch;
int duration;
{
	DosBeep((ULONG)pitch, (ULONG)duration);
}

void xf86SetKbdLeds(leds)
int leds;
{
	KBDINFO kinfo;
	APIRET rc;

	rc = KbdGetStatus(&kinfo,(HKBD)xf86Info.consoleFd);
	if (!rc) {
		kinfo.fsMask = 0x10;
		kinfo.fsState &= ~0x70;
		kinfo.fsState |= (leds&0x70);
		KbdSetStatus(&kinfo,(HKBD)xf86Info.consoleFd);
	}
}

int xf86GetKbdLeds()
{
	KBDINFO kinfo;
	APIRET rc;

	rc = KbdGetStatus(&kinfo,(HKBD)xf86Info.consoleFd);
	return rc ? 0 : kinfo.fsState & 0x70;
}

#if NeedFunctionPrototypes
void xf86SetKbdRepeat(char rad)
#else
void xf86SetKbdRepeat(rad)
char rad;
#endif
{
	/*notyet*/
}

void xf86KbdInit()
{
	/*none required*/
}


USHORT OrigKbdState;
USHORT OrigKbdInterim;

typedef struct {
    USHORT state;
    UCHAR makeCode;
    UCHAR breakCode;
    USHORT keyID;
} HOTKEYPARAM;


int xf86KbdOn()
{
	KBDINFO info;
	APIRET rc;
	int i,k;
	ULONG len;


	KbdGetStatus(&info,(HKBD)xf86Info.consoleFd);
	OrigKbdState=info.fsMask;
	OrigKbdInterim=info.fsInterim;
	info.fsMask &= ~0x09;
	info.fsMask |= 0x136;
	info.fsInterim &= ~0x20;
	KbdSetStatus(&info,(HKBD)xf86Info.consoleFd);
	return -1;
}

int xf86KbdOff()
{
	ULONG len;
	APIRET rc;
	KBDINFO info;

	info.fsMask=OrigKbdState;
	info.fsInterim=OrigKbdInterim;
	KbdSetStatus(&info,(HKBD)xf86Info.consoleFd);
	return -1;
}

void xf86MouseInit(mouse)
MouseDevPtr mouse;
{
	return;
}

int xf86MouseOn(mouse)
MouseDevPtr mouse;
{
	HMOU fd;
	APIRET rc;
	USHORT nbut;

ErrorF ("\nxf86-OS/2: Calling MouseOn, a bad thing.... Must be some bug in the code!\n");
	if (serverGeneration == 1) {
		rc = MouOpen((PSZ)NULL,(PHMOU)&fd);
		if (rc != 0)
			FatalError("Cannot open mouse, rc=%d\n", rc);
		mouse->mseFd = fd;
	}
	
	/* flush mouse queue */
	MouFlushQue(fd);

	/* check buttons */
	rc = MouGetNumButtons(&nbut,fd);
	if (rc == 0)
		ErrorF("OsMouse has %d button(s).\n",nbut);

	return (mouse->mseFd);
}

/* This table is a bit irritating, because these mouse types are infact
 * defined in the OS/2 kernel, but I want to force the user to put
 * ïOsMouseï in the config file, and not worry about the particular mouse
 * type that is connected.
 */
Bool xf86SupportedMouseTypes[] =
{
	FALSE,	/* Microsoft */
	FALSE,	/* MouseSystems */
	FALSE,	/* MMSeries */
	FALSE,	/* Logitech */
	FALSE,	/* BusMouse */
	FALSE,	/* MouseMan */
	FALSE,	/* PS/2 */
	FALSE,	/* Hitachi Tablet */
};

int xf86NumMouseTypes = sizeof(xf86SupportedMouseTypes) /
			sizeof(xf86SupportedMouseTypes[0]);

                     /* Necessary to check input events in OS/2     */
		     /* This function performs select() on sockets  */
		     /* but uses OS/2 internal fncts to check mouse */
		     /* and keyboard. S. Marineau, 18/1/96          */

int os2PseudoSelect(nfds,readfds,writefds,exceptfds,timeout)
int nfds;
fd_set *readfds,*writefds,*exceptfds;
struct timeval *timeout;
{

CARD32 max_time,start_time,time_remaining,elapsed;
int i,j;
struct timeval dummy_timeout;
fd_set read_copy,write_copy,except_copy;

start_time=GetTimeInMillis();
if(timeout!=NULL) {
	max_time=timeout->tv_sec*1000+timeout->tv_usec/1000;
	}
else { max_time=10000000; }  /* This should be large enough... */

dummy_timeout.tv_sec=0;
dummy_timeout.tv_usec=0;

	elapsed=0;
	      /* Keep a copy bec. select() will     */
	      /* select() will destroy the bitmasks */
	if(readfds!=NULL){ XFD_COPYSET(readfds,&read_copy);}
	if(writefds!=NULL) {XFD_COPYSET(writefds,&write_copy);}
	if(exceptfds!=NULL) {XFD_COPYSET(exceptfds,&except_copy);}
        j=0;
	do {
	     os2CheckPopupPending();
	     dummy_timeout.tv_sec=0;
	     dummy_timeout.tv_usec=0;
             i=select(nfds,(readfds!=NULL)?(int *)&read_copy:NULL,
	     	(writefds!=NULL)?(int *)&write_copy:NULL,
	     	(exceptfds!=NULL)?(int *)&except_copy:NULL,
		&dummy_timeout);
	     if(i<0) return(i);  /* Error on select */
	     if(i>0) {           /* One of the descriptors is awake */
		      /* We set the timeval to the remaining time */
		  time_remaining=max_time-elapsed;
		  if(timeout!=NULL){
			timeout->tv_sec=time_remaining/1000;
			timeout->tv_usec=(time_remaining % 1000) *1000;
		  }
	/* Put the masks from select() into the original pointers */
	          if(readfds!=NULL) {XFD_COPYSET(&read_copy,readfds);}
	          if(writefds!=NULL) {XFD_COPYSET(&write_copy,writefds);}
	          if(exceptfds!=NULL) {XFD_COPYSET(&except_copy,exceptfds);}
		      /* And return i */
		  return(i);
	     }

	     /* Now we check for activity on mouse/kbd handles */
	     if((!os2MouseQueueQuery()) || (!os2KbdQueueQuery()) || 
                        xf86Info.vtRequestsPending || os2PopupErrorPending){
		  if(os2PopupErrorPending) os2RecoverFromPopup();
		  time_remaining=max_time-elapsed;
                  if(timeout!=NULL){
                        timeout->tv_sec=time_remaining/1000;
                        timeout->tv_usec=(time_remaining % 1000) *1000;
                        }
		  /* Put the masks from select() into the original pointers */
	          if(readfds!=NULL) {XFD_COPYSET(&read_copy,readfds);}
	          if(writefds!=NULL) {XFD_COPYSET(&write_copy,writefds);}
	          if(exceptfds!=NULL) {XFD_COPYSET(&except_copy,exceptfds);}
		  return(0);  /* Not to confuse the networking with mouse/kbd */
	     }
		  		   
	    if(((j++)%5)==0) usleep(1000);  /* Give CPU to other apps */
	} while((elapsed=(GetTimeInMillis()-start_time))<max_time);
		/* Well, we have time'd out */

if(timeout!=NULL){
        timeout->tv_sec=0;
        timeout->tv_usec=0;
        }
		      /* Put the masks from select() into the original pointers */
if(readfds!=NULL) {XFD_COPYSET(&read_copy,readfds);}
if(writefds!=NULL) {XFD_COPYSET(&write_copy,writefds);}
if(exceptfds!=NULL) {XFD_COPYSET(&except_copy,exceptfds);}

return(0);
}
