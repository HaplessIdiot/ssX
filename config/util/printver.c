
/*
 * A simple program to make it possible to print the XFree86 version as
 * defined in xf86Version.h very early in the build process.
 */

/* $XFree86$ */

#include <stdio.h>
#include "xf86Version.h"

main()
{
#ifdef XF86_VERSION_MAJOR
	printf(" version %d.%d.%d", XF86_VERSION_MAJOR, XF86_VERSION_MINOR,
		XF86_VERSION_PATCH);
	if (XF86_VERSION_SNAP != 0)
		printf(".%d", XF86_VERSION_SNAP);
#ifdef XF86_DATE
	printf(" (%s)", XF86_DATE);
#endif
#endif
	exit(0);
}

