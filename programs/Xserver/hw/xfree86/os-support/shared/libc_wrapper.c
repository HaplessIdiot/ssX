/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/libc_wrapper.c,v 1.33 1998/12/05 14:40:26 dawes Exp $ */
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
#include <sys/types.h>
#if defined(__bsdi__)
#undef _POSIX_SOURCE
#undef _ANSI_SOURCE
#endif
#include <sys/time.h>
#if defined(__bsdi__)
#define _POSIX_SOURCE
#define _ANSI_SOURCE
#endif
#include <math.h>
#include <stdarg.h>
#include <fcntl.h>
#include "Xfuncproto.h"
#include "os.h"
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#if !defined(ISC)
#include <stdlib.h>
#endif

#define NEED_XF86_TYPES
#define DONT_DEFINE_WRAPPERS
#include "xf86_ansic.h"

#include "xf86.h"
#include "xf86Priv.h"
#define NO_OSLIB_PROTOTYPES
#define XF86_OS_PRIVS
#define HAVE_WRAPPER_DECLS
#include "xf86_OSlib.h"

extern void xf86DisableIO(void);

#ifndef X_NOT_POSIX
#include <dirent.h>
#else
#ifdef SYSV
#include <dirent.h>
#else
#ifdef USG
#include <dirent.h>
#else
#include <sys/dir.h>
#ifndef dirent
#define dirent direct
#endif
#endif
#endif
#endif
typedef struct dirent DIRENTRY;

#ifdef __EMX__
#define _POSIX_SOURCE
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
#if defined(MINIX) || defined(AMOEBA) || (defined(ISC) && defined(_POSIX_SOURCE)) || defined(Lynx) || (defined (__alpha__) && defined(linux))
#include <sys/types.h>
#endif
#include <sys/wait.h>
#endif
#endif

#if 0
#define SETBUF_RETURNS_INT
#endif

double xf86HUGE_VAL;

/*
 * This file contains the XFree86 wrappers for libc functions that can be
 * called by loadable modules
 */

double
xf86hypot(double x, double y)
{
	return(hypot(x,y));
}

/* string functions */

char*
xf86strcat(char* dest, const char* src)
{
	return(strcat(dest,src));
}

char*
xf86strchr(const char* s, int c)
{
	return strchr(s,c);
}

int
xf86strcmp(const char* s1, const char* s2)
{
	return strcmp(s1,s2);
}

char*
xf86strcpy(char* dest, const char* src)
{
	return strcpy(dest,src);
}

INT32
xf86strcspn(const char* s1, const char* s2)
{
	return (INT32)strcspn(s1,s2);
}

INT32
xf86strlen(const char* s)
{
	return (INT32)strlen(s);
}

char*
xf86strncat(char* dest, const char* src, INT32 n)
{
	return strncat(dest,src,n);
}

int
xf86strncmp(const char* s1, const char* s2, INT32 n)
{
	return strncmp(s1,s2,n);
}

char*
xf86strncpy(char* dest, const char* src, INT32 n)
{
	return strncpy(dest,src,n);
}

char*
xf86strpbrk(const char* s1, const char* s2)
{
	return strpbrk(s1,s2);
}

char*
xf86strrchr(const char* s, int c)
{
	return strrchr(s,c);
}

INT32
xf86strspn(const char* s1, const char* s2)
{
	return strspn(s1,s2);
}

char*
xf86strstr(const char* s1, const char* s2)
{
	return strstr(s1,s2);
}

char*
xf86strtok(char* s1, const char* s2)
{
	return strtok(s1,s2);
}

char*
xf86strdup(const char* s)
{
	return xstrdup(s);
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

void
xf86bzero(void* s, unsigned int n)
{
  bzero(s, n);
}
  
#ifdef HAVE_VSSCANF
int
xf86sscanf(char *s, const char *format, ...)
#else
int
xf86sscanf(char *s, const char *format, char *a0, char *a1, char *a2,
	   char *a3, char *a4, char *a5, char *a6, char *a7, char *a8,
	   char *a9) /* limit of ten args */
#endif
{
#ifdef HAVE_VSSCANF
	int ret;
	va_list args;
	va_start(args, format);

	ret = vsscanf(s,format,args);
	va_end(args);
	return ret;
#else
	return sscanf(s, format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif
}
  
/* Basic I/O */

int xf86errno;

/* XXX This is not complete */

static int
xfToOsOpenFlags(int xfflags)
{
    int flags = 0;

    /* XXX This assumes O_RDONLY is 0 */
    if (xfflags & XF86_O_WRONLY)
	flags |= O_WRONLY;
    if (xfflags & XF86_O_RDWR)
	flags |= O_RDWR;
    if (xfflags & XF86_O_CREAT)
	flags |= O_CREAT;

    return flags;
}

int 
xf86open(const char *path, int flags, ...)
{
    int fd;
    va_list ap;

    va_start(ap, flags);
    flags = xfToOsOpenFlags(flags);
    if (flags & XF86_O_CREAT) {
	mode_t mode = va_arg(ap, mode_t);
	fd = open(path, flags, mode);
    } else {
	fd = open(path, flags);
    }
    va_end(ap);
    xf86errno = xf86GetErrno();
    return fd;
}

int
xf86close(int fd)
{
    int status = close(fd);

    xf86errno = xf86GetErrno();
    return status;
}

int
xf86ioctl(int fd, unsigned long request, char *argp)
{
    int status = ioctl(fd, request, argp);

    xf86errno = xf86GetErrno();
    return status;
}

unsigned int
xf86read(int fd, void *buf, INT32 nbytes)
{
    unsigned int n = read(fd, buf, nbytes);

    xf86errno = xf86GetErrno();
    return n;
}

unsigned int
xf86write(int fd, const void *buf, INT32 nbytes)
{
    unsigned int n = write(fd, buf, nbytes);

    xf86errno = xf86GetErrno();
    return n;
}

/* limited stdio support */

#define XF86FILE_magic	0x58464856	/* "XFHV" */

typedef struct _xf86_file_ {
	INT32	fileno;
	INT32	magic;
	FILE*	filehnd;
	char*	fname;
} XF86FILE_priv;

XF86FILE_priv stdhnd[3] = {
	{ 0, XF86FILE_magic, NULL, "$stdinp$" },
	{ 0, XF86FILE_magic, NULL, "$stdout$" },
	{ 0, XF86FILE_magic, NULL, "$stderr$" }
};

XF86FILE* xf86stdin = (XF86FILE*)&stdhnd[0];
XF86FILE* xf86stdout = (XF86FILE*)&stdhnd[1];
XF86FILE* xf86stderr = (XF86FILE*)&stdhnd[2];

void
xf86WrapperInit()
{
    if (stdhnd[0].filehnd == NULL)
	stdhnd[0].filehnd = stdin;
    if (stdhnd[1].filehnd == NULL)
	stdhnd[1].filehnd = stdout;
    if (stdhnd[2].filehnd == NULL)
	stdhnd[2].filehnd = stderr;
    xf86HUGE_VAL = HUGE_VAL;
}

XF86FILE*
xf86fopen(const char* fn, const char* mode)
{
	XF86FILE_priv* fp;
	FILE *f = fopen(fn,mode);
	xf86errno = xf86GetErrno();
	if (!f) return 0;

	fp = xalloc(sizeof(XF86FILE_priv));
	fp->magic = XF86FILE_magic;
	fp->filehnd = f;
	fp->fileno = fileno(f);
	fp->fname = xf86strdup(fn);
#ifdef DEBUG
	ErrorF("xf86fopen(%s,%s) yields FILE %p XF86FILE %p\n",
		fn,mode,f,fp);
#endif
	return (XF86FILE*)fp;
}

static void _xf86checkhndl(XF86FILE_priv* f,const char *func)
{
	if (!f || f->magic != XF86FILE_magic ||
	    !f->filehnd || !f->fname) {
		FatalError("libc_wrapper error: passed invalid FILE handle to %s\n",
			func);
		exit(42);
	}
}

int
xf86fclose(XF86FILE* f) 
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;
	int ret;

	_xf86checkhndl(fp,"xf86fclose");

	/* somewhat bad check */
	if (fp->fileno < 3 && fp->fname[0]=='$') {
		/* assume this is stdin/out/err, don't dispose */
		ret = fclose(fp->filehnd);
	} else {
		ret = fclose(fp->filehnd);
		fp->magic = 0;	/* invalidate */
		xfree(fp->fname);
		xfree(fp);
	}
	return ret ? -1 : 0;
}

int
xf86printf(const char *format, ...)
{
	int ret;
	va_list args;
	va_start(args, format);

	ret = printf(format,args);
	va_end(args);
	return ret;
}

int
xf86fprintf(
#if NeedVarargsPrototypes
XF86FILE* f, const char *format, ...)
#else
f, format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) /* limit of ten args */
    XF86FILE* f;
    const char *format;
    char *a0, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8, *a9;
#endif
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

#if NeedVarargsPrototypes
	int ret;
	va_list args;
	va_start(args, format);

#ifdef DEBUG
	ErrorF("xf86fprintf for XF86FILE %p\n", fp);
#endif
	_xf86checkhndl(fp,"xf86fprintf");

	ret = vfprintf(fp->filehnd,format,args);
	va_end(args);
	return ret;
#else
#ifdef DEBUG
	ErrorF("xf86fprintf for XF86FILE %p\n", fp);
#endif
	_xf86checkhndl(fp,"xf86fprintf");
	return fprintf(fp->filehnd, format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif
}

int
xf86vfprintf(XF86FILE* f, const char *format,...)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;
	int ret;
	va_list args;
	va_start(args, format);

#ifdef DEBUG
	ErrorF("xf86vfprintf for XF86FILE %p\n", fp);
#endif
	_xf86checkhndl(fp,"xf86vfprintf");

	ret = vfprintf(fp->filehnd,format,args);
	va_end(args);
	return ret;
}

int
xf86vsprintf(char *s, const char *format, ...)
{
	int ret;
	va_list args;
	va_start(args, format);

	ret = vsprintf(s,format,args);
	va_end(args);
	return ret;
}

#ifdef HAVE_VFSCANF
int
xf86fscanf(XF86FILE* f, const char *format, ...)
#else
int
xf86fscanf(XF86FILE* f, const char *format, char *a0, char *a1, char *a2,
	   char *a3, char *a4, char *a5, char *a6, char *a7, char *a8,
	   char *a9) /* limit of ten args */
#endif
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

#ifdef HAVE_VFSCANF
	int ret;
	va_list args;
	va_start(args, format);

	_xf86checkhndl(fp,"xf86fscanf");

	ret = vfscanf(fp->filehnd,format,args);
	va_end(args);
	return ret;
#else
	_xf86checkhndl(fp,"xf86fscanf");
	return fscanf(fp->filehnd, format, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
#endif
}

char *xf86fgets(char *buf, INT32 n, XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fgets");
	return fgets(buf,(int)n,fp->filehnd);
}

int
xf86fputs(const char *buf, XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fputs");
	return fputs(buf,fp->filehnd);
}

int
xf86getc(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86getc");
	return getc(fp->filehnd);
}

int
xf86fgetc(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fgetc");
	return fgetc(fp->filehnd);
}

int
xf86fputc(int c,XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fputc");
	return fputc(c,fp->filehnd);
}

int
xf86fflush(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fflush");
	return fflush(fp->filehnd);
}

INT32
xf86fread(void* buf, INT32 sz, INT32 cnt, XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

#ifdef DEBUG
	ErrorF("xf86fread for XF86FILE %p\n", fp);
#endif
	_xf86checkhndl(fp,"xf86fread");
	return fread(buf,sz,cnt,fp->filehnd);
}

INT32
xf86fwrite(const void* buf, INT32 sz, INT32 cnt, XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fwrite");
	return fwrite(buf,sz,cnt,fp->filehnd);
}

int
xf86fseek(XF86FILE* f, long pos, int loc)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fseek");
	switch (pos) {
	case XF86_SEEK_SET:
		pos = SEEK_SET;
		break;
	case XF86_SEEK_CUR:
		pos = SEEK_CUR;
		break;
	case XF86_SEEK_END:
		pos = SEEK_END;
		break;
	}
	return fseek(fp->filehnd,pos,loc);
}

long
xf86ftell(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86ftell");
	return ftell(fp->filehnd);
}

char*
xf86strerror(int n)
{
	return strerror(n);
}

/* required for portable fgetpos/fsetpos,
 * use as
 *	XF86fpos_t* pos = xalloc(xf86fpossize());
 */
long
xf86fpossize()
{
	return sizeof(fpos_t);
}

int
xf86fgetpos(XF86FILE* f,XF86fpos_t* pos)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;
	fpos_t *ppos = (fpos_t*)pos;

	_xf86checkhndl(fp,"xf86fgetpos");
#ifndef ISC
	return fgetpos(fp->filehnd,ppos);
#else
	*ppos = ftell(fp->filehnd);
	if (*ppos < 0L)
		return(-1);
	return(0);
#endif
}

int
xf86fsetpos(XF86FILE* f,const XF86fpos_t* pos)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;
	fpos_t *ppos = (fpos_t*)pos;

	/* XXX need to handle xf86errno here */
	_xf86checkhndl(fp,"xf86fsetpos");
#ifndef ISC
	return fsetpos(fp->filehnd,ppos);
#else
	if (ppos == NULL)
	{
		errno = EINVAL;
		return EOF;
	}
	return fseek(fp->filehnd, *ppos, SEEK_SET);
#endif
}

void
xf86perror(const char *s)
{
	perror(s);
}

int
xf86remove(const char *s)
{
#ifdef _POSIX_SOURCE
	return remove(s);
#else
	return unlink(s);
#endif
}

int
xf86rename(const char *old, const char *new)
{
#ifdef _POSIX_SOURCE
	return rename(old,new);
#else
	int ret = link(old,new);
	if (!ret) {
		ret = unlink(old);
		if (ret) unlink(new);
	} else
		ret = unlink(new);
	return ret;
#endif
}

void
xf86rewind(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fsetpos");
	rewind(fp->filehnd);
}

void
xf86clearerr(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86clearerr");
	clearerr(fp->filehnd);
}

int
xf86feof(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86feof");
	return feof(fp->filehnd);
}

int
xf86ferror(XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86ferror");
	return ferror(fp->filehnd);
}

XF86FILE*
xf86freopen(const char* fname,const char* mode,XF86FILE* fold)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)fold;
	FILE *fnew;

	_xf86checkhndl(fp,"xf86freopen");
	fnew = freopen(fname,mode,fp->filehnd);
	xf86errno = xf86GetErrno();
	if (!fnew) {
		xf86fclose(fold);	/* discard old XF86FILE structure */
		return 0;
	}
	/* recycle the old XF86FILE structure */
	fp->magic = XF86FILE_magic;
	fp->filehnd = fnew;
	fp->fileno = fileno(fnew);
	fp->fname = xf86strdup(fname);
#ifdef DEBUG
	ErrorF("xf86freopen(%s,%s,%p) yields FILE %p XF86FILE %p\n",
		fname,mode,fold,fnew,fp);
#endif
	return (XF86FILE*)fp;
}

int
xf86setbuf(XF86FILE* f, char *buf)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86fsetbuf");
#ifdef SETBUF_RETURNS_INT
	return setbuf(fp->filehnd, buf);
#else
	setbuf(fp->filehnd, buf);
	return 0;
#endif
}

int
xf86setvbuf(XF86FILE* f, char *buf, int mode, INT32 size)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;
	int vbufmode;

	_xf86checkhndl(fp,"xf86fsetvbuf");

	switch (mode) {
	case XF86_IONBF:
		vbufmode = _IONBF;
		break;
	case XF86_IOLBF:
		vbufmode = _IOFBF;
		break;
	case XF86_IOFBF:
		vbufmode = _IOLBF;
		break;
	default:
		FatalError("libc_wrapper error: mode in setvbuf incorrect\n");
		exit(42);
	}

	return setvbuf(fp->filehnd,buf,vbufmode,(size_t)size);
}

XF86FILE*
xf86tmpfile(void)
{
#ifdef NEED_TMPFILE
	return xf86fopen(tmpnam((char*)0),"w+");
#else
	XF86FILE_priv* fp;
	FILE *f = tmpfile();
	xf86errno = xf86GetErrno();
	if (!f) return 0;

	fp = xalloc(sizeof(XF86FILE_priv));
	fp->magic = XF86FILE_magic;
	fp->filehnd = f;
	fp->fileno = fileno(f);
	fp->fname = xf86strdup("*tmpfile*"); /* so that it can be xfree()'d */
#ifdef DEBUG
	ErrorF("xf86tmpfile() yields FILE %p XF86FILE %p\n",f,fp);
#endif
	return (XF86FILE*)fp;
}
#endif /* HAS_TMPFILE */


int
xf86ungetc(int c,XF86FILE* f)
{
	XF86FILE_priv* fp = (XF86FILE_priv*)f;

	_xf86checkhndl(fp,"xf86ungetc");
	return ungetc(c,fp->filehnd);
}

/* misc functions */

void
xf86usleep(usec)
    unsigned long usec;
{
#if (defined(SYSV) || defined(SVR4)) && !defined(sun)
    syscall(3112, (usec) / 1000 + 1);
#else
    usleep(usec);
#endif
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

int
xf86ffs(int mask)
{
	int n;
	if (mask == 0) return 0;
	for (n = 1; (mask & 1)==0; n++)
		mask >>= 1;
	return n;
}

char *
xf86getenv(const char * a)
{
	return(getenv(a));
}

/*VARARGS1*/
int
xf86execl(const char *pathname, const char *arg, ...)
{
#ifndef __EMX__
    int i;
    pid_t pid;
#ifdef MACH386
    union wait exit_status;
#else
    int exit_status;
#endif
    char *arglist[5];
    va_list args;
    va_start(args, arg);
    arglist[0] = (char*)&args;
    i = 1;
    while (i < 5 && (arglist[i++] = va_arg(args, char *)) != NULL)
	;
    va_end(args);

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
	xf86DisableIO();
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

	execv(pathname, arglist);
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
#else
    return(1);
#endif /* __EMX__ Disable this crazy business for now */
}

Bool
xf86setexternclock(pathname, clock_arg, clock_index)
    char *pathname;
    int clock_arg;
    int clock_index;
{
#ifndef __EMX__
    char *progname, *clockarg, *clockindex;
    int ret;

    if ((progname = strrchr(pathname, '/')))
	progname++;
    else
	progname = pathname;

    /*
     * Not everything has snprintf, but this should be safe given the
     * range checks.
     */
    if (clock_arg > 1000000.0 || clock_arg < 0.0)
	return FALSE;
    if (clock_index > MAXCLOCKS || clock_index < 0)
	return FALSE;

    /* Largest value is 1000.000 (1GHz) */
    clockarg = xalloc(10);
    /* Largest value is MAXCLOCKS (currently 128) */
    clockindex = xalloc(5);
    sprintf(clockarg, "%.3f", clock_arg / 1000.0);
    sprintf(clockindex, "%d", clock_index);
    ret = xf86execl(pathname, progname, clockarg, clockindex);
    xfree(clockarg);
    xfree(clockindex);
    return ret ? FALSE : TRUE;
#else
    return FALSE;
#endif /* __EMX__ Disable this for now*/
}

void
xf86abort(void)
{
	ErrorF("Module called abort() function\n");
	abort();
}

void
xf86exit(int ex)
{
	ErrorF("Module called exit() function with value=%d\n",ex);
	exit(ex);
}

/* directory handling functions */
#define XF86DIR_magic	0x78666876	/* "xfhv" */

typedef struct _xf86_dir_ {
	DIR		*dir;
	INT32		magic;
	XF86DIRENT	*dirent;
} XF86DIR_priv;

static void _xf86checkdirhndl(XF86DIR_priv* f,const char *func)
{
	if (!f || f->magic != XF86DIR_magic || !f->dir || !f->dirent) {
		FatalError("libc_wrapper error: passed invalid DIR handle to %s\n",
			func);
		exit(42);
	}
}

XF86DIR*	xf86opendir(const char *name)
{
	XF86DIR_priv *dp;
	DIR *dirp;

	dirp = opendir(name);
	if (!dirp)
		return (XF86DIR*)0;

	dp = xalloc(sizeof(XF86DIR_priv));
	dp->magic = XF86DIR_magic; /* This time I have this, Dirk! :-) */
	dp->dir = dirp;
	dp->dirent = xalloc(sizeof(struct _xf86dirent));

	return (XF86DIR*)dp;
}

XF86DIRENT*
xf86readdir(XF86DIR* dirp)
{
	XF86DIR_priv* dp = (XF86DIR_priv*)dirp;
	DIRENTRY *de;
	XF86DIRENT* xde;
	int sz;

	_xf86checkdirhndl(dp,"xf86readdir");

	de = readdir(dp->dir);
	if (!de)
		return (XF86DIRENT*)0;
	xde = dp->dirent;
	sz = strlen(de->d_name);
	strncpy(xde->d_name,de->d_name, sz>_XF86NAMELEN ? (_XF86NAMELEN+1) : (sz+1));
	xde->d_name[_XF86NAMELEN] = '\0';	/* be sure to have a 0 byte */
	return xde;
}

void
xf86rewinddir(XF86DIR* dirp)
{
	XF86DIR_priv* dp = (XF86DIR_priv*)dirp;

	_xf86checkdirhndl(dp,"xf86readdir");
	rewinddir(dp->dir);
}

int
xf86closedir(XF86DIR* dir)
{
	XF86DIR_priv* dp = (XF86DIR_priv*)dir;
	int n;

	_xf86checkdirhndl(dp,"xf86readdir");

	n = closedir(dp->dir);
	dp->magic = 0;
	xfree(dp->dirent);
	xfree(dp);

	return n;
}

/* Several math functions */

int
xf86abs(int x)
{
	return abs(x);
}

double
xf86acos(double x)
{
	return acos(x);
}

double
xf86asin(double x)
{
	return asin(x);
}

double
xf86atan(double x)
{
	return atan(x);
}

double
xf86atan2(double x,double y)
{
	return atan2(x,y);
}

double
xf86atof(const char* s)
{
	return atof(s);
}

int
xf86atoi(const char* s)
{
	return atoi(s);
}

long
xf86atol(const char* s)
{
	return atol(s);
}

double
xf86ceil(double x)
{
	return ceil(x);
}

double
xf86cos(double x)
{
	return(cos(x));
}

double
xf86exp(double x)
{
	return(exp(x));
}

double
xf86fabs(double x)
{
        return(fabs(x));
}

double
xf86floor(double x)
{
	return floor(x);
}

double
xf86fmod(double x,double y)
{
	return fmod(x,y);
}

long
xf86labs(long x)
{
	return labs(x);
}

double
xf86log(double x)
{
	return(log(x));
}

double
xf86log10(double x)
{
	return(log10(x));
}

double
xf86modf(double x,double* y)
{
	return modf(x,y);
}

double
xf86pow(double x, double y)
{
	return(pow(x,y));
}

double
xf86sin(double x)
{
	return sin(x);
}

double
xf86sqrt(double x)
{
	return(sqrt(x));
}

double
xf86strtod(const char *s, char **end)
{
	return strtod(s,end);
}

long
xf86strtol(const char *s, char **end, int radix)
{
	return strtol(s,end,radix);
}

unsigned long
xf86strtoul(const char *s, char **end,int radix)
{
	return strtoul(s,end,radix);
}

double
xf86tan(double x)
{
	return tan(x);
}

/* memory functions */
void*
xf86memchr(const void* s, int c, INT32 n)
{
	return memchr(s,c,n);
}

int
xf86memcmp(const void* s1, const void* s2, INT32 n)
{
	return(memcmp(s1,s2,n));
}

void*
xf86memcpy(void* dest, const void* src, INT32 n)
{
	return(memcpy(dest,src,n));
}

void*
xf86memmove(void* dest, const void* src, INT32 n)
{
	return(memmove(dest,src,n));
}

void*
xf86memset(void* s, int c, INT32 n)
{
	return(memset(s,c,n));
}

/* ctype functions */

int
xf86isalnum(int c)
{
	return isalnum(c) ? 1 : 0;
}

int
xf86isalpha(int c)
{
	return isalpha(c) ? 1 : 0;
}

int
xf86iscntrl(int c)
{
	return iscntrl(c) ? 1 : 0;
}

int
xf86isdigit(int c)
{
	return isdigit(c) ? 1 : 0;
}

int
xf86isgraph(int c)
{
	return isgraph(c) ? 1 : 0;
}

int
xf86islower(int c)
{
	return islower(c) ? 1 : 0;
}

int
xf86isprint(int c)
{
	return isprint(c) ? 1 : 0;
}

int
xf86ispunct(int c)
{
	return ispunct(c) ? 1 : 0;
}

int
xf86isspace(int c)
{
	return isspace(c) ? 1 : 0;
}

int
xf86isupper(int c)
{
	return isupper(c) ? 1 : 0;
}

int
xf86isxdigit(int c)
{
	return isxdigit(c) ? 1 : 0;
}

int
xf86tolower(int c)
{
	return tolower(c);
}

int
xf86toupper(int c)
{
	return toupper(c);
}

/* memory allocation functions */
void*
xf86calloc(INT32 sz,INT32 n)
{
	return xcalloc(sz, n);
}

void
xf86free(void* p)
{
	xfree(p);
}

void*
xf86malloc(INT32 n)
{
	return xalloc(n);
}

void*
xf86realloc(void* p, INT32 n)
{
	return xrealloc(p,n);
}


#define mapnum(e) case (e): return (xf86_##e)

int
xf86GetErrno ()
{
	switch (errno)
	{
		case 0: return 0;
		mapnum (EACCES);
		mapnum (EAGAIN);
		mapnum (EBADF);
		mapnum (EEXIST);
		mapnum (EFAULT);
		mapnum (EINTR);
		mapnum (EINVAL);
		mapnum (EISDIR);
		mapnum (ELOOP);
		mapnum (EMFILE);
		mapnum (ENAMETOOLONG);
		mapnum (ENFILE);
		mapnum (ENOENT);
		mapnum (ENOMEM);
		mapnum (ENOSPC);
		mapnum (ENOTDIR);
		mapnum (EPIPE);
		mapnum (EROFS);
		mapnum (ETXTBSY);
		mapnum (ENOTTY);

		default:
			return (xf86_UNKNOWN);
	}
}

#undef mapnum
