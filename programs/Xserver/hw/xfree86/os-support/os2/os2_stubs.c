/* $XFree86$ */
/*
 * (c) Copyright 1996 by Holger Veit
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
#include "Xpoll.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>

/* This code is duplicated from XLibInt.c, because the same problems with
 * the drive letter as in clients also exist in the server
 * Unfortunately the standalone servers don't link against libX11
 */

char *__XOS2RedirRoot(char *fname)
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
    return redirname;
}

char *__XOS2RedirRoot1(char *format, char *arg1, char *arg2, char *arg3)
{
    /* this first constructs a name from a format and up to three
     * components, then adds a path
     */
    char buf[300];
    sprintf(buf,format,arg1,arg2,arg3);
    return __XOS2RedirRoot(buf);
}

/*
 * This declares a missing function in the __EMX__ library, used in
 * various places
 */
void usleep(delay)
	unsigned long delay;
{
	DosSleep(delay ? (delay/1000) : 1l);
}

/* This is there to resolve a symbol in Xvfb 
 * this version is somewhat crippled compared to the one in os2_io.c
 */
#ifdef OS2NULLSELECT
int os2PseudoSelect(nfds,readfds,writefds,exceptfds,timeout)
	int nfds;
	fd_set *readfds,*writefds,*exceptfds;
	struct timeval *timeout;
{
	long max_time,start_time,time_remaining,elapsed;
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
	if (readfds != NULL) { XFD_COPYSET(readfds,&read_copy); }
	if (writefds != NULL) { XFD_COPYSET(writefds,&write_copy); }
	if (exceptfds != NULL) { XFD_COPYSET(exceptfds,&except_copy); }
	j=0;
	do {
		dummy_timeout.tv_sec=0;
		dummy_timeout.tv_usec=0;
        	i=select(nfds,(readfds!=NULL)?(int *)&read_copy:NULL,
			(writefds!=NULL)?(int *)&write_copy:NULL,
			(exceptfds!=NULL)?(int *)&except_copy:NULL,
			&dummy_timeout);
		if (i<0) return(i);  /* Error on select */
		if (i>0) {           /* One of the descriptors is awake */
			/* We set the timeval to the remaining time */
			time_remaining=max_time-elapsed;
			if(timeout!=NULL){
				timeout->tv_sec=time_remaining/1000;
				timeout->tv_usec=(time_remaining % 1000) *1000;
			}
			/* Put the masks from select() into the original pointers */
			if (readfds!=NULL) {XFD_COPYSET(&read_copy,readfds);}
			if (writefds!=NULL) {XFD_COPYSET(&write_copy,writefds);}
			if (exceptfds!=NULL) {XFD_COPYSET(&except_copy,exceptfds);}
			/* And return i */
			return(i);
		}
		if (((j++)%5)==0) usleep(1000);  /* Give CPU to other apps */
	} while((elapsed=(GetTimeInMillis()-start_time))<max_time);
	/* Well, we have time'd out */

	if (timeout!=NULL) {
		timeout->tv_sec=0;
		timeout->tv_usec=0;
	}

	/* Put the masks from select() into the original pointers */
	if(readfds!=NULL) {XFD_COPYSET(&read_copy,readfds);}
	if(writefds!=NULL) {XFD_COPYSET(&write_copy,writefds);}
	if(exceptfds!=NULL) {XFD_COPYSET(&except_copy,exceptfds);}
	return(0);
}
#endif
