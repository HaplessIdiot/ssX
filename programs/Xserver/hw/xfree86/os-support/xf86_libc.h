/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86_libc.h,v 3.7 1997/06/25 08:25:05 hohndel Exp $ */



/*
 * This file is an attempt to make developing code for the new loadable module
 * architecure simpler. It tries to use macros to hide all libc wrappers so
 * that all that is needed to "port" a module to this architecture is to
 * include this one header file
 *
 * Revision history:
 *
 * 0.3	Feb 24 1997	handle getenv
 * 0.2	Feb 24 1997	hide few FILE functions
 * 0.1	Feb 24 1997	hide the trivial functions mem* str*
 */

#ifndef	XF86_LIBC_H

#define XF86_LIBC_H

#ifdef XFree86LOADER

/*
 * First, the new data types
 *
 * note: if some pointer is declared "opaque" here, pass it between
 * xf86* functions only, and don't rely on it having a whatever internal
 * structure, even if some source file might reveal the existence of
 * such a structure.
 */
typedef pointer XF86FILE;	/* opaque FILE* replacement */
extern  XF86FILE xf86stdin;
extern  XF86FILE xf86stdout;
extern  XF86FILE xf86stderr;

typedef pointer XF86FPOS_T;	/* opaque fpos_t* replacement */

#define _XF86NAMELEN	263	/* enough for a larger filename */
				/* (divisble by 8) */
typedef pointer XF86DIR;	/* opaque DIR* replacement */

/* Note: the following is POSIX! POSIX only requires the d_name member. 
 * Normal Unix has often a number of other members, but don't rely on that
 */
struct _xf86dirent {		/* types in struct dirent/direct: */
	char	d_name[_XF86NAMELEN+1];	/* char [MAXNAMLEN]; might be smaller or unaligned */
};
typedef struct _xf86dirent *XF86DIRENT;

/*
 * the rest of this file should only be included for code that is supposed
 * to go into modules
 */

#ifndef DONT_DEFINE_WRAPPERS
/*
 * the mem.../bcopy family
 */
#define memcpy(a,b,c)	xf86memcpy(a,b,c)
#define memset(a,b,c)	xf86memset(a,b,c)
#define memmove(a,b,c)	xf86memmove(a,b,c)
#define memcmp(a,b,c)	xf86memcmp(a,b,c)
#ifdef  bcopy
#undef  bcopy
#endif
#define bcopy(a,b,c)	xf86memmove(b,a,c)

/*
 * the string functions 
 */
#define strcpy(a,b)	xf86strcpy(a,b)
#define strncpy(a,b,c)	xf86strncpy(a,b,c)
#define strcat(a,b)	xf86strcat(a,b)
#define strcmp(a,b)	xf86strcmp(a,b)
#define strncmp(a,b,c)	xf86strncmp(a,b,c)
#define strlen(a)	xf86strlen(a)
#define strdup(a)       xf86strdup(a)

/*
 * some math functions
 *
 * hopefully calling and return conventions using doubles won't mess this up
 */
#define log(a)		xf86log(a)
#define exp(a)		xf86exp(a)
#define pow(a,b)	xf86pow(a,b)
#define sqrt(a)		xf86sqrt(a)
#define cos(a)		xf86cos(a)
#define ffs(a)          xf86ffs(a)
#define fabs(a)         xf86fabs(a)

/*
 * Basic I/O functions 
 */
#define open(a,b,c)     xf86open(a,b,c)
#define close(a)        xf86close(a)
#define ioctl(a,b,c)    xf86ioctl(a,b,c)
#define read(a,b,c)     xf86read(a,b,c)
#define write(a,b,c)    xf86write(a,b,c)

/*
 * FILE is not really compatible accross operating systems
 *
 */
#define FILE		XF86FILE
#define fopen(a,b)	xf86fopen(a,b)
#define fclose(a)	xf86fclose(a)
#define fread(a,b,c,d)	xf86fread(a,b,c,d)
#define fwrite(a,b,c,d)	xf86fwrite(a,b,c,d)
#define fseek(a,b,c)	xf86fseek(a,b,c)

/*
 * DIR and DIRENT need to be replaced as well
 */
#define opendir(a)	xf86opendir(a)
#define readdir(a)	xf86readdir(a)
#define rewinddir(a)	xf86rewinddir(a)
#define closedir(a)	xf86closedir(a)

/*
 * misc other functions we provide
 */
#define getenv(a)	xf86getenv(a)
#define strerror(a)     xf86strerror(a)
#define errno           xf86errno

#endif /* DONT_DEFINE_WRAPPERS */

/*
 * at this point I don't think we support any non-ANSI compilers...
 */
extern int xf86errno;

extern void xf86getsecs(INT32 *, INT32 *);

Bool xf86setexternclock(char *, int, int);

int xf86getbitsperpixel(int);

int xf86sprintf(char *, const char *, ...);

int xf86execl(char *, ...);

int xf86fprintf(XF86FILE f, const char *format, ...);

int xf86fscanf(XF86FILE f, const char *format, ...);

char *xf86fgets(char *buf, INT32 n, XF86FILE f);

int xf86fputs(char *buf, XF86FILE f);

int xf86fgetc(XF86FILE f);

int xf86fputc(int c,XF86FILE f);

int xf86fflush(XF86FILE f);

long xf86ftell(XF86FILE f);

long xf86fpossize();

int xf86fgetpos(XF86FILE f,XF86FPOS_T pos);

int xf86fsetpos(XF86FILE f,const XF86FPOS_T pos);

void xf86perror(const char *s);

int xf86remove(const char *s);

int xf86rename(const char *old, const char *new);

void xf86rewind(XF86FILE f);


extern void * xf86memmove(void *, const void *, INT32);

extern void * xf86memset(void *, int, INT32);

extern void * xf86memcpy(void *, const void *, INT32);

extern int xf86memcmp(const void *, const void *, INT32);

extern char * xf86strcat(char *, const char *);

extern char * xf86strcpy(char *, const char *);

extern char * xf86strncpy(char *, const char *, INT32);

extern int xf86strcmp(const char *, const char *);

extern int xf86strncmp(const char *, const char *, INT32);

extern char *xf86strdup(const char *);

extern size_t xf86strlen(const char *);

double xf86exp(double);

double xf86log(double);

double xf86pow(double, double);

double xf86sqrt(double);

double xf86cos(double);

double xf86fabs(double);

void xf86bzero(void *, unsigned int);

char * xf86strerror(int);

void xf86usleep(unsigned long);

XF86FILE xf86fopen(const char* fn, const char* mode);

int xf86fclose(XF86FILE f);

size_t xf86fread(void* buf, size_t sz, size_t cnt, XF86FILE f);

size_t xf86fwrite(void* buf, size_t sz, size_t cnt, XF86FILE f);

int xf86fseek(XF86FILE f, long pos, int loc);

int xf86ffs(int mask);

char * xf86getenv(const char *);

XF86DIR	xf86opendir(const char *name);

XF86DIRENT xf86readdir(XF86DIR dirp);

void xf86rewinddir(XF86DIR dirp);

int xf86closedir(XF86DIR dirp);

int xf86open(const char *path, int flags, ...);

int xf86close(int fd);

int xf86ioctl(int fd, unsigned long request, char *argp);

unsigned int xf86read(int fd, void *buf, size_t nbytes);

unsigned int xf86write(int fd, void *buf, size_t nbytes);

#endif /* XFree86LOADER */

#endif /* XF86_LIBC_H */
