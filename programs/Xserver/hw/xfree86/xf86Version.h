/* $XConsortium: xf86Version.h,v 1.6 95/01/23 15:33:26 kaleb Exp $ */
/* $XFree86: xc/programs/Xserver/hw/xfree86/xf86Version.h,v 3.136 1996/01/26 09:04:49 dawes Exp $ */

#define XF86_VERSION " 3.1.2Bn "

/* The finer points in versions... */
#define XF86_VERSION_MAJOR	3
#define XF86_VERSION_MINOR	1
#define XF86_VERSION_SUBMINOR	2
#define XF86_VERSION_BETA	2	/* 0="", 1="A", 2="B", etc... */
#define XF86_VERSION_ALPHA	14	/* 0="", 1="a", 2="b", etc... */

#define XF86_VERSION_NUMERIC(major,minor,subminor,beta,alpha)	\
   ((((((((major << 7) | minor) << 7) | subminor) << 5) | beta) << 5) | alpha)
#define XF86_VERSION_CURRENT					\
   XF86_VERSION_NUMERIC(XF86_VERSION_MAJOR,			\
			XF86_VERSION_MINOR,			\
			XF86_VERSION_SUBMINOR,			\
			XF86_VERSION_BETA,			\
			XF86_VERSION_ALPHA)

#define XF86_DATE	"Jan 28 1996"
