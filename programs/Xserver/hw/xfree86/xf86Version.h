/* $XFree86: xc/programs/Xserver/hw/xfree86/xf86Version.h,v 3.177 1996/08/25 14:06:13 dawes Exp $ */

#define XF86_VERSION " 3.1.2Eo "

/* The finer points in versions... */
#define XF86_VERSION_MAJOR	3
#define XF86_VERSION_MINOR	1
#define XF86_VERSION_SUBMINOR	2
#define XF86_VERSION_BETA	5	/* 0="", 1="A", 2="B", etc... */
#define XF86_VERSION_ALPHA	15	/* 0="", 1="a", 2="b", etc... */

#define XF86_VERSION_NUMERIC(major,minor,subminor,beta,alpha)	\
   ((((((((major << 7) | minor) << 7) | subminor) << 5) | beta) << 5) | alpha)
#define XF86_VERSION_CURRENT					\
   XF86_VERSION_NUMERIC(XF86_VERSION_MAJOR,			\
			XF86_VERSION_MINOR,			\
			XF86_VERSION_SUBMINOR,			\
			XF86_VERSION_BETA,			\
			XF86_VERSION_ALPHA)

#define XF86_DATE	"Aug 26 1996"

/* $XConsortium: xf86Version.h /main/36 1996/01/31 10:07:08 kaleb $ */
