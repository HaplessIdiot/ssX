
/* $XFree86$ */
/*
 * finish setting up the server
 * Load the driver modules and call their probe functions.
 *
 * Copyright 1999 by The XFree86 Project, Inc.
 *
 */

#include <ctype.h>
#include <stdlib.h>
#include "X.h"
#include "Xmd.h"
#include "os.h"
#ifdef XFree86LOADER
#include "loaderProcs.h"
#endif
#include "xf86.h"
#include "xf86Priv.h"

void
DoProbeArgs(int argc, char **argv, int i)
{
}

void
DoProbe()
{
    int i, skip, globalVerbose, probeVerbose;
    Bool probeResult;

  /* Find the list of video driver modules. */
    char **list, **l;
    const char *subdirs[] = {
	"drivers",
	NULL
    };
    const char *patlist[] = {
	"(.*)_drv\\.so",
	"(.*)_drv\\.o",
	NULL
    };

    list = LoaderListDirs(subdirs, NULL);
    if (list) {
	ErrorF("List of video driver modules:\n");
	for (l = list; *l; l++)
	    ErrorF("\t%s\n", *l);
    } else {
	ErrorF("No video driver modules found\n");
    }

    /* Load all the drivers that were found. */
    xf86LoadModules(list, NULL);

    /* Call all of the probe functions, reporting the results. */
    for (i = 0; i < xf86NumDrivers; i++) {
	probeResult = FALSE;
	if (xf86DriverList[i]->Probe != NULL)
	    probeResult = xf86DriverList[i]->Probe(xf86DriverList[i],
						   PROBE_DETECT);
	ErrorF("Probe in driver `%s' returns %s\n",
	       xf86DriverList[i]->driverName, BOOLTOSTRING(probeResult));
    }

    LoaderFreeDirList(list);

    OsCleanup();
    AbortDDX();
    fflush(stderr);
    exit(0);
}
