/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86_libc.h,v 3.20 1998/06/27 12:54:30 hohndel Exp $ */



/*
 * This file is an attempt to make developing code for the new loadable module
 * architecure simpler. It tries to use macros to hide all libc wrappers so
 * that all that is needed to "port" a module to this architecture is to
 * include this one header file
 *
 * Revision history:
 *
 *
 * 0.4	Apr 12 1997	add the ANSI defines
 * 0.3	Feb 24 1997	handle getenv
 * 0.2	Feb 24 1997	hide few FILE functions
 * 0.1	Feb 24 1997	hide the trivial functions mem* str*
 */

#ifndef	XF86_LIBC_H

#define XF86_LIBC_H


#if defined(XFree86LOADER) || defined(NEED_XF86_TYPES)

/*
 * First, the new data types
 *
 * note: if some pointer is declared "opaque" here, pass it between
 * xf86* functions only, and don't rely on it having a whatever internal
 * structure, even if some source file might reveal the existence of
 * such a structure.
 */
typedef void XF86FILE;		/* opaque FILE replacement */
extern  XF86FILE* xf86stdin;
extern  XF86FILE* xf86stdout;
extern  XF86FILE* xf86stderr;

typedef void XF86fpos_t;	/* opaque fpos_t replacement */

#define _XF86NAMELEN	263	/* enough for a larger filename */
				/* (divisble by 8) */
typedef void XF86DIR;		/* opaque DIR replacement */

/* Note: the following is POSIX! POSIX only requires the d_name member. 
 * Normal Unix has often a number of other members, but don't rely on that
 */
struct _xf86dirent {		/* types in struct dirent/direct: */
	char	d_name[_XF86NAMELEN+1];	/* char [MAXNAMLEN]; might be smaller or unaligned */
};
typedef struct _xf86dirent XF86DIRENT;

/* for setvbuf */
#define XF86_IONBF    1
#define XF86_IOFBF    2
#define XF86_IOLBF    3

#endif /* defined(XFree86LOADER) || defined(NEED_XF86_TYPES) */

#ifdef XFree86LOADER

/*
 * the rest of this file should only be included for code that is supposed
 * to go into modules
 */


#ifndef DONT_DEFINE_WRAPPERS

#define abort()			xf86abort()
#undef abs
#define abs(i)			xf86abs(i)
#define acos(d)			xf86acos(d)
#define asin(d)			xf86asin(d)
#define atan(d)			xf86atan(d)
#define atan2(d1,d2)		xf86atan2(d1,d2)
#define atof(ccp)		xf86atof(ccp)
#define atoi(ccp)		xf86atoi(ccp)
#define atol(ccp)		xf86atol(ccp)
#define ceil(d)			xf86ceil(d)
#define calloc(I1,I2)		xf86calloc(I1,I2)
#undef clearerr
#define clearerr(FP)		xf86clearerr(FP)
#define cos(d)			xf86cos(d)
#define exit(i)			xf86exit(i)
#define exp(d)			xf86exp(d)
#define fabs(d)			xf86fabs(d)
#define fclose(FP)		xf86fclose(FP)
#undef feof
#define feof(FP)		xf86feof(FP)
#undef ferror
#define ferror(FP)		xf86ferror(FP)
#define fflush(FP)		xf86fflush(FP)
#define fgetc(FP)		xf86fgetc(FP)
#define fgetpos(FP,fpp)		xf86fgetpos(FP,fpp)
#define fgets(cp,i,FP)		xf86fgets(cp,i,FP)
#define floor(d)		xf86floor(d)
#define fmod(d1,d2)		xf86fmod(d1,d2)
#define fopen(ccp1,ccp2)	xf86fopen(ccp1,ccp2)
#define fprintf			xf86fprintf
#define fputc(i,FP)		xf86fputc(i,FP)
#define fputs(ccp,FP)		xf86fputs(ccp,FP)
#define fread(vp,I1,I2,FP)	xf86fread(vp,I1,I2,FP)
#define free(vp)		xf86free(vp)
#define freopen(ccp1,ccp2,FP)	xf86freopen(ccp1,ccp2,FP)
#define fscanf			xf86fscanf
#define fseek(FP,l,i)		xf86fseek(FP,l,i)
#define fsetpos(FP,cfpp)	xf86fsetpos(FP,cfpp)
#define ftell(FP)		xf86ftell(FP)
#define fwrite(cvp,I1,I2,FP)	xf86fwrite(cvp,I1,I2,FP)
#define getenv(ccp)		xf86getenv(ccp)
#undef isalnum
#define isalnum(i)		xf86isalnum(i)
#undef isalpha
#define isalpha(i)		xf86isalpha(i)
#undef iscntrl
#define iscntrl(i)		xf86iscntrl(i)
#undef isdigit
#define isdigit(i)		xf86isdigit(i)
#undef isgraph
#define isgraph(i)		xf86isgraph(i)
#undef islower
#define islower(i)		xf86islower(i)
#undef isprint
#define isprint(i)		xf86isprint(i)
#undef ispunct
#define ispunct(i)		xf86ispunct(i)
#undef isspace
#define isspace(i)		xf86isspace(i)
#undef isupper
#define isupper(i)		xf86isupper(i)
#undef isxdigit
#define isxdigit(i)		xf86isxdigit(i)
#define labs(l)			xf86labs(l)
#define log(d)			xf86log(d)
#define log10(d)		xf86log10(d)
#define malloc(I)		xf86malloc(I)
#define memchr(cvp,i,I)		xf86memchr(cvp,i,I)
#define memcmp(cvp1,cvp2,I)	xf86memcmp(cvp1,cvp2,I)
#define memcpy(vp,cvp,I)	xf86memcpy(vp,cvp,I)
#define memmove(vp,cvp,I)	xf86memmove(vp,cvp,I)
#undef memset
#define memset(vp,int,I)	xf86memset(vp,int,I)
#define modf(d,dp)		xf86modf(d,dp)
#define perror(ccp)		xf86perror(ccp)
#define pow(d1,d2)		xf86pow(d1,d2)
#define realloc(vp,I)		xf86realloc(vp,I)
#define remove(ccp)		xf86remove(ccp)
#define rename(ccp1,ccp2)	xf86rename(ccp1,ccp2)
#define rewind(FP)		xf86rewind(FP)
#define setbuf(FP,cp)		xf86setbuf(FP,cp)
#define setvbuf(FP,cp,i,I)	xf86setvbuf(FP,cp,i,I)
#define sin(d)			xf86sin(d)
#define sprintf			xf86sprintf
#define sqrt(d)			xf86sqrt(d)
#define sscanf			xf86sscanf
#define strcat(cp,ccp)		xf86strcat(cp,ccp)
#define strcmp(ccp1,ccp2)	xf86strcmp(ccp1,ccp2)
#define strcpy(cp,ccp)		xf86strcpy(cp,ccp)
#define strcspn(ccp1,ccp2)	xf86strcspn(ccp1,ccp2)
#define strerror(i)		xf86strerror(i)
#define strlen(ccp)		xf86strlen(ccp)
#define strncmp(ccp1,ccp2,I)	xf86strncmp(ccp1,ccp2,I)
#define strncpy(cp,ccp,I)	xf86strncpy(cp,ccp,I)
#define strpbrk(ccp1,ccp2)	xf86strpbrk(ccp1,ccp2)
#define strrchr(ccp,i)		xf86strrchr(ccp,i)
#define strspn(ccp1,ccp2)	xf86strspn(ccp1,ccp2)
#define strstr(ccp1,ccp2)	xf86strstr(ccp1,ccp2)
#define strtod(ccp,cpp)		xf86strtod(ccp,cpp)
#define strtok(cp,ccp)		xf86strtok(cp,ccp)
#define strtol(ccp,cpp,i)	xf86strtol(ccp,cpp,i)
#define strtoul(ccp,cpp,i)	xf86strtoul(ccp,cpp,i)
#define tan(d)			xf86tan(d)
#define tmpfile()		xf86tmpfile()
#undef tolower
#define tolower(i)		xf86tolower(i)
#undef toupper
#define toupper(i)		xf86toupper(i)
#define ungetc(i,FP)		xf86ungetc(i,FP)
#define vfprintf		xf86vfprintf
#define vsprintf		xf86vsprintf

/* non-ANSI C functions */
#define opendir(cp)		xf86opendir(cp)
#define closedir(DP)		xf86closedir(DP)
#define readdir(DP)		xf86readdir(DP)
#define rewinddir(DP)		xf86rewinddir(DP)
#undef bcopy
#define bcopy(vp,cvp,I)		xf86memmove(cvp,vp,I)
#define ffs(i)			xf86ffs(i)
#define strdup(ccp)		xf86strdup(ccp)
#undef usleep
#define usleep(ul)		xf86usleep(ul)
#undef bzero
#define bzero(vp,ui)		xf86bzero(vp,ui)
#define execl	        	xf86execl

/* some types */
#define FILE			XF86FILE
#define fpos_t			XF86fpos_t
#define DIR			XF86DIR
#define DIRENT			XF86DIRENT

/* some vars */
#ifdef stdin
#undef stdin
#endif
#define	stdin			xf86stdin
#ifdef stdout
#undef stdout
#endif
#define stdout			xf86stdout
#ifdef stderr
#undef stderr
#endif
#define stderr			xf86stderr

/*
 * XXX Basic I/O functions BAD,BAD,BAD!
 */
#define open(a,b,c)     xf86open(a,b,c)
#define close(a)        xf86close(a)
#define ioctl(a,b,c)    xf86ioctl(a,b,c)
#define read(a,b,c)     xf86read(a,b,c)
#define write(a,b,c)    xf86write(a,b,c)
#ifndef __EMX__
#define errno           xf86errno
#endif

#endif /*DONT_DEFINE_WRAPPERS*/

#endif /* XFree86LOADER */

#endif /* XF86_LIBC_H */
