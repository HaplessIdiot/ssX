/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/xf86_libc.h,v 3.40 1999/09/25 14:37:43 dawes Exp $ */



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
typedef unsigned int xf86gid_t;

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

#undef abort
#define abort()			xf86abort()
#undef abs
#define abs(i)			xf86abs(i)
#undef acos
#define acos(d)			xf86acos(d)
#undef asin
#define asin(d)			xf86asin(d)
#undef atan
#define atan(d)			xf86atan(d)
#undef atan2
#define atan2(d1,d2)		xf86atan2(d1,d2)
#undef atof
#define atof(ccp)		xf86atof(ccp)
#undef atoi
#define atoi(ccp)		xf86atoi(ccp)
#undef atol
#define atol(ccp)		xf86atol(ccp)
#undef bsearch
#define bsearch(a,b,c,d,e)	xf86bsearch(a,b,c,d,e)
#undef ceil
#define ceil(d)			xf86ceil(d)
#undef calloc
#define calloc(I1,I2)		xf86calloc(I1,I2)
#undef clearerr
#define clearerr(FP)		xf86clearerr(FP)
#undef cos
#define cos(d)			xf86cos(d)
#undef exit
#define exit(i)			xf86exit(i)
#undef exp
#define exp(d)			xf86exp(d)
#undef fabs
#define fabs(d)			xf86fabs(d)
#undef fclose
#define fclose(FP)		xf86fclose(FP)
#undef feof
#define feof(FP)		xf86feof(FP)
#undef ferror
#define ferror(FP)		xf86ferror(FP)
#undef fflush
#define fflush(FP)		xf86fflush(FP)
#undef fgetc
#define fgetc(FP)		xf86fgetc(FP)
#undef getc
#define getc(FP)		xf86getc(FP)
#undef fgetpos
#define fgetpos(FP,fpp)		xf86fgetpos(FP,fpp)
#undef fgets
#define fgets(cp,i,FP)		xf86fgets(cp,i,FP)
#undef floor
#define floor(d)		xf86floor(d)
#undef fmod
#define fmod(d1,d2)		xf86fmod(d1,d2)
#undef fopen
#define fopen(ccp1,ccp2)	xf86fopen(ccp1,ccp2)
#undef printf
#define printf			xf86printf
#undef fprintf
#define fprintf			xf86fprintf
#undef fputc
#define fputc(i,FP)		xf86fputc(i,FP)
#undef fputs
#define fputs(ccp,FP)		xf86fputs(ccp,FP)
#undef fread
#define fread(vp,I1,I2,FP)	xf86fread(vp,I1,I2,FP)
#undef free
#define free(vp)		xf86free(vp)
#undef freopen
#define freopen(ccp1,ccp2,FP)	xf86freopen(ccp1,ccp2,FP)
#undef fscanf
#define fscanf			xf86fscanf
#undef fseek
#define fseek(FP,l,i)		xf86fseek(FP,l,i)
#undef fsetpos
#define fsetpos(FP,cfpp)	xf86fsetpos(FP,cfpp)
#undef ftell
#define ftell(FP)		xf86ftell(FP)
#undef fwrite
#define fwrite(cvp,I1,I2,FP)	xf86fwrite(cvp,I1,I2,FP)
#undef getenv
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
#undef labs
#define labs(l)			xf86labs(l)
#undef log
#define log(d)			xf86log(d)
#undef log10
#define log10(d)		xf86log10(d)
#undef malloc
#define malloc(I)		xf86malloc(I)
#undef memchr
#define memchr(cvp,i,I)		xf86memchr(cvp,i,I)
#undef memcmp
#define memcmp(cvp1,cvp2,I)	xf86memcmp(cvp1,cvp2,I)
#undef memcpy
#define memcpy(vp,cvp,I)	xf86memcpy(vp,cvp,I)
#undef memmove
#define memmove(vp,cvp,I)	xf86memmove(vp,cvp,I)
#undef memset
#define memset(vp,int,I)	xf86memset(vp,int,I)
#undef modf
#define modf(d,dp)		xf86modf(d,dp)
#undef perror
#define perror(ccp)		xf86perror(ccp)
#undef pow
#define pow(d1,d2)		xf86pow(d1,d2)
#undef realloc
#define realloc(vp,I)		xf86realloc(vp,I)
#undef remove
#define remove(ccp)		xf86remove(ccp)
#undef rename
#define rename(ccp1,ccp2)	xf86rename(ccp1,ccp2)
#undef rewind
#define rewind(FP)		xf86rewind(FP)
#undef setbuf
#define setbuf(FP,cp)		xf86setbuf(FP,cp)
#undef setvbuf
#define setvbuf(FP,cp,i,I)	xf86setvbuf(FP,cp,i,I)
#undef sin
#define sin(d)			xf86sin(d)
#undef snprintf
#define snprintf		xf86snprintf
#undef sprintf
#define sprintf			xf86sprintf
#undef sqrt
#define sqrt(d)			xf86sqrt(d)
#undef sscanf
#define sscanf			xf86sscanf
#undef strcat
#define strcat(cp,ccp)		xf86strcat(cp,ccp)
#undef strcmp
#define strcmp(ccp1,ccp2)	xf86strcmp(ccp1,ccp2)
#undef strcasecmp
#define strcasecmp(ccp1,ccp2)	xf86strcasecmp(ccp1,ccp2)
#undef strcpy
#define strcpy(cp,ccp)		xf86strcpy(cp,ccp)
#undef strcspn
#define strcspn(ccp1,ccp2)	xf86strcspn(ccp1,ccp2)
#undef strerror
#define strerror(i)		xf86strerror(i)
#undef strlen
#define strlen(ccp)		xf86strlen(ccp)
#undef strncmp
#define strncmp(ccp1,ccp2,I)	xf86strncmp(ccp1,ccp2,I)
#undef strncasecmp
#define strncasecmp(ccp1,ccp2,I) xf86strncasecmp(ccp1,ccp2,I)
#undef strncpy
#define strncpy(cp,ccp,I)	xf86strncpy(cp,ccp,I)
#undef strpbrk
#define strpbrk(ccp1,ccp2)	xf86strpbrk(ccp1,ccp2)
#undef strchr
#define strchr(ccp,i)		xf86strchr(ccp,i)
#undef strrchr
#define strrchr(ccp,i)		xf86strrchr(ccp,i)
#undef strspn
#define strspn(ccp1,ccp2)	xf86strspn(ccp1,ccp2)
#undef strstr
#define strstr(ccp1,ccp2)	xf86strstr(ccp1,ccp2)
#undef srttod
#define strtod(ccp,cpp)		xf86strtod(ccp,cpp)
#undef strtok
#define strtok(cp,ccp)		xf86strtok(cp,ccp)
#undef strtol
#define strtol(ccp,cpp,i)	xf86strtol(ccp,cpp,i)
#undef strtoul
#define strtoul(ccp,cpp,i)	xf86strtoul(ccp,cpp,i)
#undef tan
#define tan(d)			xf86tan(d)
#undef tmpfile
#define tmpfile()		xf86tmpfile()
#undef tolower
#define tolower(i)		xf86tolower(i)
#undef toupper
#define toupper(i)		xf86toupper(i)
#undef ungetc
#define ungetc(i,FP)		xf86ungetc(i,FP)
#undef vfprinf
#define vfprintf		xf86vfprintf
#undef vsnprintf
#define vsnprintf		xf86vsnprintf
#undef vsprintf
#define vsprintf		xf86vsprintf
/* XXX Disable assert as if NDEBUG was defined */
/* Some X headers defined this away too */
#undef assert
#define assert(a)		((void)0)
#undef HUGE_VAL
#define HUGE_VAL		xf86HUGE_VAL;

#undef hypot
#define hypot(x,y)		xf86hypot(x,y)

/* non-ANSI C functions */
#undef opendir
#define opendir(cp)		xf86opendir(cp)
#undef closedir
#define closedir(DP)		xf86closedir(DP)
#undef readdir
#define readdir(DP)		xf86readdir(DP)
#undef rewinddir
#define rewinddir(DP)		xf86rewinddir(DP)
#undef bcopy
#define bcopy(vp,cvp,I)		xf86memmove(cvp,vp,I)
#undef ffs
#define ffs(i)			xf86ffs(i)
#undef strdup
#define strdup(ccp)		xf86strdup(ccp)
#undef bzero
#define bzero(vp,ui)		xf86bzero(vp,ui)
#undef execl
#define execl	        	xf86execl
#undef chmod
#define chmod(a,b)              xf86chmod(a,b)
#undef chown
#define chown(a,b,c)            xf86chown(a,b,c)
#undef geteuid
#define geteuid                 xf86geteuid
#undef mknod
#define mknod(a,b,c)            xf86mknod(a,b,c)
#undef sleep
#define sleep(a)                xf86sleep(a)
#undef mkdir
#define mkdir(a,b)              xf86mkdir(a,b)
#undef getpagesize
#define getpagesize		xf86getpagesize
#ifdef S_ISUID
#undef S_ISUID
#endif
#define S_ISUID XF86_S_ISUID
#ifdef S_ISGID
#undef S_ISGID
#endif
#define S_ISGID XF86_S_ISGID
#ifdef S_ISVTX
#undef S_ISVTX
#endif
#define S_ISVTX XF86_S_ISVTX
#ifdef S_IRUSR
#undef S_IRUSR
#endif
#define S_IRUSR XF86_S_IRUSR
#ifdef S_IWUSR
#undef S_IWUSR
#endif
#define S_IWUSR XF86_S_IWUSR
#ifdef S_IXUSR
#undef S_IXUSR
#endif
#define S_IXUSR XF86_S_IXUSR
#ifdef S_IRGRP
#undef S_IRGRP
#endif
#define S_IRGRP XF86_S_IRGRP
#ifdef S_IWGRP
#undef S_IWGRP
#endif
#define S_IWGRP XF86_S_IWGRP
#ifdef S_IXGRP
#undef S_IXGRP
#endif
#define S_IXGRP XF86_S_IXGRP
#ifdef S_IROTH
#undef S_IROTH
#endif
#define S_IROTH XF86_S_IROTH
#ifdef S_IWOTH
#undef S_IWOTH
#endif
#define S_IWOTH XF86_S_IWOTH
#ifdef S_IXOTH
#undef S_IXOTH
#endif
#define S_IXOTH XF86_S_IXOTH
#ifdef S_IFREG
#undef S_IFREG
#endif
#define S_IFREG XF86_S_IFREG
#ifdef S_IFCHR
#undef S_IFCHR
#endif
#define S_IFCHR XF86_S_IFCHR
#ifdef S_IFBLK
#undef S_IFBLK
#endif
#define S_IFBLK XF86_S_IFBLK
#ifdef S_IFIFO
#undef S_IFIFO
#endif
#define S_IFIFO XF86_S_IFIFO

/* some types */
#ifdef FILE
#undef FILE
#endif
#define FILE			XF86FILE
#ifdef fpos_t
#undef fpos_t
#endif
#define fpos_t			XF86fpos_t
#ifdef DIR
#undef DIR
#endif
#define DIR			XF86DIR
#ifdef DIRENT
#undef DIRENT
#endif
#define DIRENT			XF86DIRENT
#ifdef size_t
#undef size_t
#endif
#define size_t			xf86size_t
#ifdef ssize_t
#undef ssize_t
#endif
#define ssize_t			xf86ssize_t
#ifdef dev_t
#undef dev_t
#endif
#define dev_t                   xf86dev_t
#ifdef mode_t
#undef mode_t
#endif
#define mode_t                  xf86mode_t
#ifdef uid_t
#undef uid_t
#endif
#define uid_t                   xf86uid_t
#ifdef gid_t
#undef gid_t
#endif
#define gid_t                   xf86gid_t

/*
 * There should be no need to #undef any of these.  If they are already
 * defined it is because some illegal header has been included.
 */

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

#ifdef SEEK_SET
#undef SEEK_SET
#endif
#define SEEK_SET		XF86_SEEK_SET
#ifdef SEEK_CUR
#undef SEEK_CUR
#endif
#define SEEK_CUR		XF86_SEEK_CUR
#ifdef SEEK_END
#undef SEEK_END
#endif
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
#ifdef O_RDONLY
#undef O_RDONLY
#endif
#define O_RDONLY		XF86_O_RDONLY
#ifdef O_WRONLY
#undef O_WRONLY
#endif
#define O_WRONLY		XF86_O_WRONLY
#ifdef O_RDWR
#undef O_RDWR
#endif
#define O_RDWR			XF86_O_RDWR
#ifdef O_CREAT
#undef O_CREAT
#endif
#define O_CREAT			XF86_O_CREAT
#ifdef PROT_EXEC
#undef PROT_EXEC
#endif
#define PROT_EXEC		XF86_PROT_EXEC
#ifdef PROT_READ
#undef PROT_READ
#endif
#define PROT_READ		XF86_PROT_READ
#ifdef PROT_WRITE
#undef PROT_WRITE
#endif
#define PROT_WRITE		XF86_PROT_WRITE
#ifdef PROT_NONE
#undef PROT_NONE
#endif
#define PROT_NONE		XF86_PROT_NONE
#ifdef MAP_FIXED
#undef MAP_FIXED
#endif
#define MAP_FIXED		XF86_MAP_FIXED
#ifdef MAP_SHARED
#undef MAP_SHARED
#endif
#define MAP_SHARED		XF86_MAP_SHARED
#ifdef MAP_PRIVATE
#undef MAP_PRIVATE
#endif
#define MAP_PRIVATE		XF86_MAP_PRIVATE
#ifdef MAP_FAILED
#undef MAP_FAILED
#endif
#define MAP_FAILED		XF86_MAP_FAILED
#ifdef R_OK
#undef R_OK
#endif
#define R_OK                    XF86_R_OK
#ifdef W_OK
#undef W_OK
#endif
#define W_OK                    XF86_W_OK
#ifdef X_OK
#undef X_OK
#endif
#define X_OK                    XF86_X_OK
#ifdef F_OK
#undef F_OK
#endif
#define F_OK                    XF86_F_OK
#ifndef __EMX__
#ifdef errno
#undef errno
#endif
#define errno			xf86errno
#endif

#ifdef EACCES
#undef EACCES
#endif
#define EACCES		xf86_EACCES
#ifdef EAGAIN
#undef EAGAIN
#endif
#define EAGAIN		xf86_EAGAIN
#ifdef EBADF
#undef EBADF
#endif
#define EBADF		xf86_EBADF
#ifdef EEXIST
#undef EEXIST
#endif
#define EEXIST		xf86_EEXIST
#ifdef EFAULT
#undef EFAULT
#endif
#define EFAULT		xf86_EFAULT
#ifdef EINTR
#undef EINTR
#endif
#define EINTR		xf86_EINTR
#ifdef EINVAL
#undef EINVAL
#endif
#define EINVAL		xf86_EINVAL
#ifdef EISDIR
#undef EISDIR
#endif
#define EISDIR		xf86_EISDIR
#ifdef ELOOP
#undef ELOOP
#endif
#define ELOOP		xf86_ELOOP
#ifdef EMFILE
#undef EMFILE
#endif
#define EMFILE		xf86_EMFILE
#ifdef ENAMETOOLONG
#undef ENAMETOOLONG
#endif
#define ENAMETOOLONG	xf86_ENAMETOOLONG
#ifdef ENFILE
#undef ENFILE
#endif
#define ENFILE		xf86_ENFILE
#ifdef ENOENT
#undef ENOENT
#endif
#define ENOENT		xf86_ENOENT
#ifdef ENOMEM
#undef ENOMEM
#endif
#define ENOMEM		xf86_ENOMEM
#ifdef ENOSPC
#undef ENOSPC
#endif
#define ENOSPC		xf86_ENOSPC
#ifdef ENOTDIR
#undef ENOTDIR
#endif
#define ENOTDIR		xf86_ENOTDIR
#ifdef EPIPE
#undef EPIPE
#endif
#define EPIPE		xf86_EPIPE
#ifdef EROFS
#undef EROFS
#endif
#define EROFS		xf86_EROFS
#ifdef ETXTBSY
#undef ETXTBSY
#endif
#define ETXTBSY		xf86_ETXTBSY
#ifdef ENOTTY
#undef ENOTTY
#endif 
#define ENOTTY		xf86_ENOTTY
#ifdef ENOSYS
#undef ENOSYS
#endif
#define ENOSYS		xf86_ENOSYS
#ifdef EBUSY
#undef EBUSY
#endif
#define EBUSY		xf86_EBUSY
#ifdef ENODEV
#undef ENODEV
#endif
#define ENODEV		xf86_ENODEV
#ifdef EIO
#undef EIO
#endif
#define EIO		xf86_EIO

/* Some ANSI macros */
#ifdef FILENAME_MAX
#undef FILENAME_MAX
#endif
#define FILENAME_MAX		1024

#endif /* XFree86LOADER */

#endif /* XF86_LIBC_H */
