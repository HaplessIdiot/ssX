
/* $XFree86: $ */



/*
 * if PEX is built as a module, it shouldn't call libc functions.
 * The following macros should wrap all calls in PEX
 */
#ifdef XFree86LOADER
#define memmove(a,b,c)	xf86memmove(a,b,c)
#define memcpy(a,b,c)	xf86memcpy(a,b,c)
#define memset(a,b,c)	xf86memset(a,b,c)
#define strcpy(a,b)	xf86strcpy(a,b)
#define strncmp(a,b,c)	xf86strncmp(a,b,c)
#define strcat(a,b)	xf86strcat(a,b)
#define strcmp(a,b)	xf86strcmp(a,b)
#define log(a)		xf86log(a)
#define exp(a)		xf86exp(a)
#define pow(a,b)	xf86pow(a,b)
#define sqrt(a)		xf86sqrt(a)
#define cos(a)		xf86cos(a)
#endif
