
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86DoProbe.c,v 1.7 2000/02/18 17:16:25 tsi Exp $ */
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
#include "xf86Config.h"
#endif /* XFree86LOADER */
#include "xf86.h"
#include "xf86Priv.h"

void
DoProbeArgs(int argc, char **argv, int i)
{
}

void
DoProbe()
{
    int i;
    Bool probeResultISA, probeResultPCI, probeResultFBDEV;

#ifdef XFree86LOADER
    /* Find the list of video driver modules. */
    char **list = xf86DriverlistFromCompile();
    char **l;

    if (list) {
	ErrorF("List of video driver modules:\n");
	for (l = list; *l; l++)
	    ErrorF("\t%s\n", *l);
    } else {
	ErrorF("No video driver modules found\n");
    }

    /* Load all the drivers that were found. */
    xf86LoadModules(list, NULL);
#endif /* XFree86LOADER */

    /* Disable PCI devices */
    xf86AccessInit();

    /* Call all of the probe functions, reporting the results. */
    for (i = 0; i < xf86NumDrivers; i++) {
	if (xf86DriverList[i]->Probe == NULL) continue;

	probeResultPCI =
	    (*xf86DriverList[i]->Probe)(xf86DriverList[i], PROBE_DETECTPCI);
	ErrorF("Probe PCI capabilities in driver `%s' returns %s\n",
	    xf86DriverList[i]->driverName, BOOLTOSTRING(probeResultPCI));
	probeResultISA =
	    (*xf86DriverList[i]->Probe)(xf86DriverList[i], PROBE_DETECTISA);
	ErrorF("Probe ISA capabilities in driver `%s' returns %s\n",
	    xf86DriverList[i]->driverName, BOOLTOSTRING(probeResultISA));
	probeResultFBDEV =
	    (*xf86DriverList[i]->Probe)(xf86DriverList[i], PROBE_DETECTFBDEV);
	ErrorF("Probe FBDEV capabilities in driver `%s' returns %s\n",
	    xf86DriverList[i]->driverName, BOOLTOSTRING(probeResultFBDEV));

	/* If we have a result, then call driver's Identify function */
	if (probeResultISA || probeResultPCI || probeResultFBDEV) {
	    if (xf86DriverList[i]->Identify != NULL) {
		(*xf86DriverList[i]->Identify)(0);
	    }
	}
    }

    OsCleanup();
    AbortDDX();
    fflush(stderr);
    exit(0);
}
