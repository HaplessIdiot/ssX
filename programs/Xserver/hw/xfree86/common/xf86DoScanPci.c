
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86DoScanPci.c,v 1.5 1999/03/29 09:41:28 dawes Exp $ */
/*
 * finish setting up the server
 * call the functions from the scanpci module
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
#include "xf86Pci.h"
#define DECLARE_CARD_DATASTRUCTURES
#include "xf86PciInfo.h"
#include "xf86ScanPci.h"

static void (*xf86ScanPciFunc)(int);

void DoScanPci(int argc, char **argv, int i)
{
  int j,skip,globalVerbose,scanpciVerbose;
#ifdef XFree86LOADER
  pointer DataSetupFunc;
  int errmaj, errmin;
  char *name;
#endif

  /*
   * first we need to finish setup of the OS so that we can call other
   * functions in the server
   */
  OsInit();

  /*
   * now we decrease verbosity and remember the value, in case a later
   * -verbose on the command line increases it, because that is a 
   * verbose flag for scanpci...
   */
  globalVerbose = --xf86Verbose;
  /*
   * next we process the arguments that are remaining on the command line,
   * so that things like the module path can be set there
   */
  for ( j = i+1; j < argc; j++ ) {
    if ((skip = ddxProcessArgument(argc, argv, j)))
	j += (skip - 1);
  } 
  /*
   * was the verbosity level increased?
   */
  if( (globalVerbose == 0) && (xf86Verbose > 0) )
    scanpciVerbose = xf86Verbose - globalVerbose -1;
  else
    scanpciVerbose = xf86Verbose - globalVerbose;
  xf86Verbose = globalVerbose;
  /*
   * now get the loader set up and load the scanpci module
   */
#ifdef XFree86LOADER
  /* Initialise the loader */
  LoaderInit();
  /* Tell the loader the default module search path */
  LoaderSetPath(xf86ModulePath);


  /* Normalise the module name */
  name = xf86NormalizeName("scanpci");

  if (!LoadModule(name, NULL, NULL, NULL, NULL, NULL,
                  &errmaj, &errmin)) {
    LoaderErrorMsg(NULL, name, errmaj, errmin);
    exit(1);
  }
  if (LoaderCheckUnresolved(LD_RESOLV_IFDONE)) {
      /* For now, just a warning */
      xf86Msg(X_WARNING, "Some symbols could not be resolved!\n");
  }
  xf86ScanPciFunc = (void (*)(int))LoaderSymbol("xf86DisplayPCICardInfo");
  DataSetupFunc = LoaderSymbol("xf86SetupPciData");
  ((void (*)(SymTabPtr*,pciVendorDeviceInfo**,pciVendorCardInfo**))
   DataSetupFunc)(&xf86PCIVendorNameInfo, &xf86PCIVendorInfo,
		    &xf86PCICardInfo);
#else
  xf86ScanPciFunc = xf86DisplayPCICardInfo;
  xf86SetupScanPci(&xf86PCIVendorNameInfo,
		     &xf86PCIVendorInfo,&xf86PCICardInfo);
#endif

  (*xf86ScanPciFunc)(scanpciVerbose);

  /*
   * that's it; we really should clean things up, but a simple
   * exit seems to be all we need
   */
  exit(0);
}
