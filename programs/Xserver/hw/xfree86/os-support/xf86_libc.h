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
 */
#define XF86FILE	pointer
extern  XF86FILE xf86stdin;
extern  XF86FILE xf86stdout;
extern  XF86FILE xf86stderr;

#define FILE		XF86FILE
#define fopen(a,b)	xf86fopen(a,b)
#define fclose(a)	xf86fclose(a)
#define fread(a,b,c,d)	xf86fread(a,b,c,d)
#define fwrite(a,b,c,d)	xf86fwrite(a,b,c,d)
#define fseek(a,b,c)	xf86fseek(a,b,c)

/*
 * misc other functions we provide
 */
#define getenv(a)	xf86getenv(a)



#endif /* XFree86LOADER */

#endif /* XF86_LIBC_H */
