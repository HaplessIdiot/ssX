/* $XFree86 $ */



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
 * structure, even if yome source file might reveal the existance of
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
#ifdef  bcopy
#undef  bcopy
#endif
#define bcopy(a,b,c)	xf86memmove(b,a,c)

/*
 * the string functions 
 */
#define strcpy(a,b)	xf86strcpy(a,b)
#define strncmp(a,b,c)	xf86strncmp(a,b,c)
#define strcat(a,b)	xf86strcat(a,b)
#define strcmp(a,b)	xf86strcmp(a,b)

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


#endif /* DONT_DEFINE_WRAPPERS */

#endif /* XFree86LOADER */

#endif /* XF86_LIBC_H */
