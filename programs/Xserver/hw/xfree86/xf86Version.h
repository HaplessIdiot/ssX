/* $XFree86: xc/programs/Xserver/hw/xfree86/xf86Version.h,v 3.256 1997/03/12 11:35:44 hohndel Exp $ */

#define XF86_VERSION " 3.2An "

/* The finer points in versions... */
#define XF86_VERSION_MAJOR	3
#define XF86_VERSION_MINOR	2
#define XF86_VERSION_SUBMINOR	0
#define XF86_VERSION_BETA	1	/* 0="", 1="A", 2="B", etc... */
#define XF86_VERSION_ALPHA	14	/* 0="", 1="a", 2="b", etc... */

#define XF86_VERSION_NUMERIC(major,minor,subminor,beta,alpha)	\
   ((((((((major << 7) | minor) << 7) | subminor) << 5) | beta) << 5) | alpha)
#define XF86_VERSION_CURRENT					\
   XF86_VERSION_NUMERIC(XF86_VERSION_MAJOR,			\
			XF86_VERSION_MINOR,			\
			XF86_VERSION_SUBMINOR,			\
			XF86_VERSION_BETA,			\
			XF86_VERSION_ALPHA)

#define XF86_DATE	"Mar 15 1997"

/* $XConsortium: xf86Version.h /main/78 1996/10/28 05:42:10 kaleb $ */
