/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/linux/lnx_wrapper.c,v 3.7 1996/12/23 06:50:00 dawes Exp geert $ */
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
#include <unistd.h>

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

