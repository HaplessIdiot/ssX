/* $XFree86: xc/programs/Xserver/Xext/panoramiXstubs.c,v 3.6 1999/06/27 16:56:07 dawes Exp $ */
/*
 * HISTORY
 * Log
 * Revision 1.1.2.3  1996/05/21  18:18:34  Madeline_Asmus
 * 	Add new dummy panoramiX routines for symbol resolution
 * 	[1996/05/21  15:24:11  Madeline_Asmus]
 *
 * Revision 1.1.2.2  1995/12/06  19:55:26  Madeline_Asmus
 * 	Build in stubs for PanoramiX for symbol resolution
 * 	[1995/12/06  19:28:14  Madeline_Asmus]
 * 
 * EndLog
 */
/* $TOG: panoramiXstubs.c /main/3 1997/10/30 16:04:17 kaleb $ */

typedef int Bool;

void *panoramiXdataPtr = 0;

void PanoramiXConsolidate()
{
	/* Not supported with Xnest */ 
}

Bool PanoramiXCreateConnectionBlock()
{
	/* Not supported with Xnest */ 
	return 0;
}

void* PanoramiXWinRoot = 0;

void* PanoramiXGCRoot = 0;

void* PanoramiXCmapRoot = 0;

void* PanoramiXPmapRoot = 0;

int PanoramiXNumScreens = 0;

void PanoramiXExtensionInit()
{
	/* Not supported with Xnest */ 
}

Bool PanoramiXCreateScreenRegion()
{
	/* Not supported with Xnest */
	return 0;
}

void* PanoramiXScreenRegion = 0;

Bool PanoramiXWinRootFreeable = 0;
Bool PanoramiXGCRootFreeable = 0;
Bool PanoramiXCmapRootFreeable = 0;
Bool PanoramiXPmapRootFreeable = 0;

void XineramaGetImageData()
{
	/* Not supported with Xnest */ 
}
