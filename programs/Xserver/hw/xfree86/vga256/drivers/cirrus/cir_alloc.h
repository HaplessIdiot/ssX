/* $XFree86$ */

/*
 * Definitions for video memory allocator in cir_alloc.c.
 */

#include <X11/Xfuncproto.h>

_XFUNCPROTOBEGIN

#if NeedFunctionPrototypes

void CirrusInitializeAllocator(int base);
int CirrusAllocate(int size);
void CirrusFree(int vidaddr);
void CirrusUploadPattern(unsigned char *pattern, int w, int h, int vidaddr,
	int srcpitch);

#else

void CirrusInitializeAllocator();
void CirrusAllocate();
void CirrusFree();
void CirrusUploadPattern();

#endif

_XFUNCPROTOEND
