/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/libc_wrapper.c,v 1.3 1997/02/18 17:51:50 hohndel Exp $ */
/*
 * Copyright 1997 by The XFree86 Project, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the names of Orest Zborowski and David Wexelblat 
 * not be used in advertising or publicity pertaining to distribution of 
 * the software without specific, written prior permission.  Orest Zborowski
 * and David Wexelblat make no representations about the suitability of this 
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * THE XFREE86 PROJECT, INC. DISCLAIMS ALL WARRANTIES WITH REGARD 
 * TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND 
 * FITNESS, IN NO EVENT SHALL OREST ZBOROWSKI OR DAVID WEXELBLAT BE LIABLE 
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES 
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <X.h>
#include <Xmd.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>
#include "Xfuncproto.h"
#if NeedVarargsPrototypes
#include <stdarg.h>
#endif
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#define NO_OSLIB_PROTOTYPES
#include "xf86Procs.h"
#if NeedVarargsPrototypes
int xf86execl(char *, ...);
#else
int xf86execl();
#endif

#ifdef ISC202
#include <sys/types.h>
#define WIFEXITED(a)  ((a & 0x00ff) == 0)  /* LSB will be 0 */
#define WEXITSTATUS(a) ((a & 0xff00) >> 8)
#define WIFSIGNALED(a) ((a & 0xff00) == 0) /* MSB will be 0 */
#define WTERMSIG(a) (a & 0x00ff)
#else
#if defined(ISC) && !defined(_POSIX_SOURCE)
#define _POSIX_SOURCE
#include <sys/types.h>
#include <sys/wait.h>
#undef _POSIX_SOURCE
#else
#if defined(MINIX) || defined(AMOEBA) || (defined(ISC) && defined(_POSIX_SOURCE)) || defined(Lynx)
#include <sys/types.h>
#endif
#include <sys/wait.h>
#endif
#endif


/*
 * This file contains the XFree86 wrappers for libc functions that can be
 * called by loadable modules
 */
void *
xf86memmove(void * dest, const void * src, INT32 n)
{
	return(memmove(dest,src,n));
}

void *
xf86memset(void * s, int c, INT32 n)
{
	return(memset(s,c,n));
}

void *
xf86memcpy(void * dest, const void * src, INT32 n)
{
	return(memcpy(dest,src,n));
}

int
xf86memcmp(const void * s1, const void * s2, INT32 n)
{
	return(memcmp(s1,s2,n));
}

char *
xf86strcat(char * dest, const char * src)
{
	return(strcat(dest,src));
}

char *
xf86strcpy(char * dest, const char * src)
{
	return(strcpy(dest,src));
}

int
xf86strcmp(const char * s1, const char * s2)
{
	return(strcmp(s1,s2));
}

int
xf86strncmp(const char * s1, const char * s2, INT32 n)
{
	return(strncmp(s1,s2,n));
}

size_t
xf86strlen(const char * s)
{
	return(strlen(s));
}

void
xf86getsecs(INT32 * secs, INT32 * usecs)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	*secs = tv.tv_sec;
	*usecs= tv.tv_usec;

	return;
}

double 
xf86exp(double x)
{
	return(exp(x));
}

double 
xf86log(double x)
{
	return(log(x));
}

double 
xf86pow(double x, double y)
{
	return(pow(x,y));
}

double 
xf86sqrt(double x)
{
	return(sqrt(x));
}

double 
xf86cos(double x)
{
	return(cos(x));
}

void
xf86bzero(s, n)
    void *s;
    unsigned int n;
{
    bzero(s, n);
}

int
xf86getbitsperpixel(myNum)
    int myNum;
{
    return xf86Screens[myNum]->bitsPerPixel;
}

int
xf86sprintf(
#if NeedVarargsPrototypes
char *s, const char *format, ...)
#else
s, format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) /* limit of ten args */
    char *s;
    const char *format;
    char *a0, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
#endif
{
#if NeedVarargsPrototypes
    int ret;
    va_list args;
    va_start(args, format);
    ret = vsprintf(s, format, args);
    va_end(args);
    return ret;
#else
    return sprintf(s, format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif
}

char *
xf86strerror(n)
    int n;
{
    return strerror(n);
}

void
xf86usleep(usec)
    unsigned long usec;
{
    usleep(usec);
}


Bool
xf86setexternclock(pathname, clock_arg, clock_index)
    char *pathname;
    int clock_arg;
    int clock_index;
{
    char *progname, clockarg[8], clockindex[4];

    if ((progname = rindex(pathname, '/')))
	progname++;
    else
	progname = pathname;

    snprintf(clockarg, 8, "%.3f", clock_arg / 1000.0);
    snprintf(clockindex, 4, "%d", clock_index);
    if (xf86execl(pathname, progname, clockarg, clockindex))
	return FALSE;
    else
	return TRUE;
}

/*VARARGS1*/
int
xf86execl(
#if NeedVarargsPrototypes
char *pathname, ...)
#else
pathname, a0, a1, a2, a3) /* limit of four args */
    char *pathname;
    char *a0, *a1, *a2, *a3;
#endif
{
    int i;
    pid_t pid;
#ifdef MACH386
    union wait exit_status;
#else
    int exit_status;
#endif
#if NeedVarargsPrototypes
    char *arglist[4];
    va_list args;
    va_start(args, pathname);
    i = 0;
    while ((arglist[i++] = va_arg(args, char *)) != NULL)
	;
    va_end(args);
#endif

    if ((pid = fork()) < 0) {
        ErrorF("Fork failed (%s)\n", strerror(errno));
        return -1;
    } else if (pid == 0) { /* child */
	/* 
	 * Make sure that the child doesn't inherit any I/O permissions it
	 * shouldn't have.  It's better to put constraints on the development
	 * of a clock program than to give I/O permissions to a bogus program
	 * in someone's XF86Config file
	 */
	for (i = 0; i < MAXSCREENS; i++)
	  xf86DisableIOPorts(i);
        setuid(getuid());
#if !defined(AMOEBA) && !defined(MINIX)
        /* set stdin, stdout to the consoleFD, and leave stderr alone */
        for (i = 0; i < 2; i++)
        {
          if (xf86Info.consoleFd != i)
          {
            close(i);
            dup(xf86Info.consoleFd);
          }
        }
#endif

#if NeedVarargsPrototypes
	execv(pathname, arglist);
#else
	execl(pathname, a0, a1, a2, a3, NULL);
#endif
	ErrorF("Exec failed for command \"%s\" (%s)\n",
	       pathname, strerror(errno));
	exit(255);
    }

    /* parent */
    wait(&exit_status);
    if (WIFEXITED(exit_status))
    {
	switch (WEXITSTATUS(exit_status))
	    {
	    case 0:     /* OK */
		return 0;
	    case 255:   /* exec() failed */
		return(255);
	    default:    /* bad exit status */
		ErrorF("Program \"%s\" had bad exit status %d\n",
		       pathname, WEXITSTATUS(exit_status));
		return(WEXITSTATUS(exit_status));
	    }
    }
    else if (WIFSIGNALED(exit_status))
    {
	ErrorF("Program \"%s\" died on signal %d\n",
	       pathname, WTERMSIG(exit_status));
	return(WTERMSIG(exit_status));
    }
#ifdef WIFSTOPPED
    else if (WIFSTOPPED(exit_status))
    {
	ErrorF("Program \"%s\" stopped by signal %d\n",
	       pathname, WSTOPSIG(exit_status));
	return(WSTOPSIG(exit_status));
    }
#endif
    else /* should never get to this point */
    {
	ErrorF("Program \"%s\" has unknown exit condition\n",
	       pathname);
	return(1);
    }
}
