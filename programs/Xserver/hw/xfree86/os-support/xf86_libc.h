/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86_libc.h,v 3.36 1999/05/04 09:35:27 dawes Exp $ */



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
#define XF86_LIBC_H 1

#include "Xfuncs.h"

/*
 * The first set of definitions are required both for modules and
 * libc_wrapper.c.
 */

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

typedef unsigned long xf86size_t;
typedef signed long xf86ssize_t;
typedef unsigned long xf86dev_t;
typedef unsigned int xf86mode_t;
typedef unsigned int xf86uid_t;

struct xf86stat {
    xf86dev_t st_rdev;	/* This is incomplete */
};

/* for setvbuf */
#define XF86_IONBF    1
#define XF86_IOFBF    2
#define XF86_IOLBF    3

/* for open (XXX not complete) */
#define XF86_O_RDONLY	0x0000
#define XF86_O_WRONLY	0x0001
#define XF86_O_RDWR	0x0002
#define XF86_O_CREAT	0x0200

/* for mmap */
#define XF86_PROT_EXEC		0x0001
#define XF86_PROT_READ		0x0002
#define XF86_PROT_WRITE		0x0004
#define XF86_PROT_NONE		0x0008
#define XF86_MAP_FIXED		0x0001
#define XF86_MAP_SHARED		0x0002
#define XF86_MAP_PRIVATE	0x0004
#define XF86_MAP_FAILED		((void *)-1)

/* for fseek */
#define XF86_SEEK_SET	0
#define XF86_SEEK_CUR	1
#define XF86_SEEK_END	2

/* for access */
#define XF86_R_OK       0
#define XF86_W_OK       1
#define XF86_X_OK       2
#define XF86_F_OK       3

/* for chmod */
#define XF86_S_ISUID   04000 /* set user ID on execution */
#define XF86_S_ISGID   02000 /* set group ID on execution */
#define XF86_S_ISVTX   01000 /* sticky bit */
#define XF86_S_IRUSR   00400 /* read by owner */
#define XF86_S_IWUSR   00200 /* write by owner */
#define XF86_S_IXUSR   00100 /* execute/search by owner */
#define XF86_S_IRGRP   00040 /* read by group */
#define XF86_S_IWGRP   00020 /* write by group */
#define XF86_S_IXGRP   00010 /* execute/search by group */
#define XF86_S_IROTH   00004 /* read by others */
#define XF86_S_IWOTH   00002 /* write by others */
#define XF86_S_IXOTH   00001 /* execute/search by others */

/* for mknod */
#define XF86_S_IFREG 0010000
#define XF86_S_IFCHR 0020000
#define XF86_S_IFBLK 0040000
#define XF86_S_IFIFO 0100000

/*
 * errno values
 * They start at 1000 just so they don't match real errnos at all
 */
#define xf86_UNKNOWN		1000
#define xf86_EACCES		1001
#define xf86_EAGAIN		1002
#define xf86_EBADF		1003
#define xf86_EEXIST		1004
#define xf86_EFAULT		1005
#define xf86_EINTR		1006
#define xf86_EINVAL		1007
#define xf86_EISDIR		1008
#define xf86_ELOOP		1009
#define xf86_EMFILE		1010
#define xf86_ENAMETOOLONG	1011
#define xf86_ENFILE		1012
#define xf86_ENOENT		1013
#define xf86_ENOMEM		1014
#define xf86_ENOSPC		1015
#define xf86_ENOTDIR		1016
#define xf86_EPIPE		1017
#define xf86_EROFS		1018
#define xf86_ETXTBSY		1019
#define xf86_ENOTTY		1020
#define xf86_ENOSYS		1021
#define xf86_EBUSY		1022
#define xf86_ENODEV		1023
#define xf86_EIO		1024

#endif /* defined(XFree86LOADER) || defined(NEED_XF86_TYPES) */

/*
 * the rest of this file should only be included for code that is supposed
 * to go into modules
 */

#if defined(XFree86LOADER) && !defined(DONT_DEFINE_WRAPPERS)

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
#define bsearch(a,b,c,d,e)	xf86bsearch(a,b,c,d,e)
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
#undef getc
#define getc(FP)		xf86getc(FP)
#define fgetpos(FP,fpp)		xf86fgetpos(FP,fpp)
#define fgets(cp,i,FP)		xf86fgets(cp,i,FP)
#define floor(d)		xf86floor(d)
#define fmod(d1,d2)		xf86fmod(d1,d2)
#define fopen(ccp1,ccp2)	xf86fopen(ccp1,ccp2)
#define printf			xf86printf
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
#define strcasecmp(ccp1,ccp2)	xf86strcasecmp(ccp1,ccp2)
#define strcpy(cp,ccp)		xf86strcpy(cp,ccp)
#define strcspn(ccp1,ccp2)	xf86strcspn(ccp1,ccp2)
#define strerror(i)		xf86strerror(i)
#define strlen(ccp)		xf86strlen(ccp)
#define strncmp(ccp1,ccp2,I)	xf86strncmp(ccp1,ccp2,I)
#define strncasecmp(ccp1,ccp2,I) xf86strncasecmp(ccp1,ccp2,I)
#define strncpy(cp,ccp,I)	xf86strncpy(cp,ccp,I)
#define strpbrk(ccp1,ccp2)	xf86strpbrk(ccp1,ccp2)
#define strchr(ccp,i)		xf86strchr(ccp,i)
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
/* XXX Disable assert as if NDEBUG was defined */
/* Some X headers defined this away too */
#undef assert
#define assert(a)		((void)0)
#undef HUGE_VAL
#define HUGE_VAL		xf86HUGE_VAL;

#define hypot(x,y)		xf86hypot(x,y)

/* non-ANSI C functions */
#define opendir(cp)		xf86opendir(cp)
#define closedir(DP)		xf86closedir(DP)
#define readdir(DP)		xf86readdir(DP)
#define rewinddir(DP)		xf86rewinddir(DP)
#undef bcopy
#define bcopy(vp,cvp,I)		xf86memmove(cvp,vp,I)
#define ffs(i)			xf86ffs(i)
#define strdup(ccp)		xf86strdup(ccp)
#undef bzero
#define bzero(vp,ui)		xf86bzero(vp,ui)
#define execl	        	xf86execl
#define chmod(a,b)              xf86chmod(a,b)
#define geteuid                 xf86geteuid
#define mknod(a,b,c)            xf86mknod(a,b,c)
#define sleep(a)                xf86sleep(a)
#define S_ISUID XF86_S_ISUID
#define S_ISGID XF86_S_ISGID
#define S_ISVTX XF86_S_ISVTX
#define S_IRUSR XF86_S_IRUSR
#define S_IWUSR XF86_S_IWUSR
#define S_IXUSR XF86_S_IXUSR
#define S_IRGRP XF86_S_IRGRP
#define S_IWGRP XF86_S_IWGRP
#define S_IXGRP XF86_S_IXGRP
#define S_IROTH XF86_S_IROTH
#define S_IWOTH XF86_S_IWOTH
#define S_IXOTH XF86_S_IXOTH
#define S_IFREG XF86_S_IFREG
#define S_IFCHR XF86_S_IFCHR
#define S_IFBLK XF86_S_IFBLK
#define S_IFIFO XF86_S_IFIFO

/* some types */
#define FILE			XF86FILE
#define fpos_t			XF86fpos_t
#define DIR			XF86DIR
#define DIRENT			XF86DIRENT
#define size_t			xf86size_t
#define ssize_t			xf86ssize_t
#define dev_t                   xf86dev_t
#define mode_t                  xf86mode_t
#define uid_t                   xf86uid_t

/*
 * There should be no need to #undef any of these.  If they are already
 * defined it is because some illegal header has been included.
 */

/* some vars */
#define	stdin			xf86stdin
#define stdout			xf86stdout
#define stderr			xf86stderr

#define SEEK_SET		XF86_SEEK_SET
#define SEEK_CUR		XF86_SEEK_CUR
#define SEEK_END		XF86_SEEK_END

/*
 * XXX Basic I/O functions BAD,BAD,BAD!
 */
#define open(a,b,c)		xf86open(a,b,c)
#define close(a)		xf86close(a)
#define lseek(a,b,c)		xf86lseek(a,b,c)
#define ioctl(a,b,c)		xf86ioctl(a,b,c)
#define read(a,b,c)		xf86read(a,b,c)
#define write(a,b,c)		xf86write(a,b,c)
#define mmap(a,b,c,d,e,f)	xf86mmap(a,b,c,d,e,f)
#define munmap(a,b)		xf86munmap(a,b)
#define stat(a,b)               xf86stat(a,b)
#define fstat(a,b)              xf86fstat(a,b)
#define access(a,b)             xf86access(a,b)
#define O_RDONLY		XF86_O_RDONLY
#define O_WRONLY		XF86_O_WRONLY
#define O_RDWR			XF86_O_RDWR
#define O_CREAT			XF86_O_CREAT
#define PROT_EXEC		XF86_PROT_EXEC
#define PROT_READ		XF86_PROT_READ
#define PROT_WRITE		XF86_PROT_WRITE
#define PROT_NONE		XF86_PROT_NONE
#define MAP_FIXED		XF86_MAP_FIXED
#define MAP_SHARED		XF86_MAP_SHARED
#define MAP_PRIVATE		XF86_MAP_PRIVATE
#define MAP_FAILED		XF86_MAP_FAILED
#define R_OK                    XF86_R_OK
#define W_OK                    XF86_W_OK
#define X_OK                    XF86_X_OK
#define F_OK                    XF86_F_OK
#ifndef __EMX__
#define errno			xf86errno
#endif

#define EACCES		xf86_EACCES
#define EAGAIN		xf86_EAGAIN
#define EBADF		xf86_EBADF
#define EEXIST		xf86_EEXIST
#define EFAULT		xf86_EFAULT
#define EINTR		xf86_EINTR
#define EINVAL		xf86_EINVAL
#define EISDIR		xf86_EISDIR
#define ELOOP		xf86_ELOOP
#define EMFILE		xf86_EMFILE
#define ENAMETOOLONG	xf86_ENAMETOOLONG
#define ENFILE		xf86_ENFILE
#define ENOENT		xf86_ENOENT
#define ENOMEM		xf86_ENOMEM
#define ENOSPC		xf86_ENOSPC
#define ENOTDIR		xf86_ENOTDIR
#define EPIPE		xf86_EPIPE
#define EROFS		xf86_EROFS
#define ETXTBSY		xf86_ETXTBSY
#define ENOTTY		xf86_ENOTTY
#define ENOSYS		xf86_ENOSYS
#define EBUSY		xf86_EBUSY
#define ENODEV		xf86_ENODEV
#define EIO		xf86_EIO

/* Some ANSI macros */
#define FILENAME_MAX		1024

#endif /* XFree86LOADER */

#endif /* XF86_LIBC_H */
