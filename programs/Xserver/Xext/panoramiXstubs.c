/*
 * HISTORY
 * $Log: panoramiXstubs.c,v $
 * Revision 3.1  1997/11/16 06:17:51  dawes
 * Lots of TOG fix-trackers
 *
 * Revision 1.1.2.3  1996/05/21  18:18:34  Madeline_Asmus
 * 	Add new dummy panoramiX routines for symbol resolution
 * 	[1996/05/21  15:24:11  Madeline_Asmus]
 *
 * Revision 1.1.2.2  1995/12/06  19:55:26  Madeline_Asmus
 * 	Build in stubs for PanoramiX for symbol resolution
 * 	[1995/12/06  19:28:14  Madeline_Asmus]
 * 
 * $EndLog$
 */
/* $TOG: panoramiXstubs.c /main/3 1997/10/30 16:04:17 kaleb $ */

typedef int Bool;

void *panoramiXdataPtr;

void PanoramiXConsolidate()
{
	/* Not supported with Xnest */ 
}

Bool PanoramiXCreateConnectionBlock()
{
	/* Not supported with Xnest */ 
	return 0;
}

void* PanoramiXWinRoot;

void* PanoramiXGCRoot;

void* PanoramiXCmapRoot;

void* PanoramiXPmapRoot;

int PanoramiXNumScreens;

void PanoramiXExtensionInit()
{
	/* Not supported with Xnest */ 
}

Bool PanoramiXCreateScreenRegion()
{
	/* Not supported with Xnest */
	return 0;
}

void* PanoramiXScreenRegion;

