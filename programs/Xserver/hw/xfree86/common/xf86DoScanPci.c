
/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86DoScanPci.c,v 1.3 1999/02/13 16:44:56 hohndel Exp $ */
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
#include "os.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "xf86Pci.h"
#define DECLARE_CARD_DATASTRUCTURES TRUE
#include "xf86PciInfo.h"

static void (*xf86ScanPciFunc)(int);

void xf86DisplayPCICardInfo(int);

void DoScanPci(int argc, char **argv, int i)
{
  int j,skip,globalVerbose,scanpciVerbose;
  int errmaj, errmin;
  char *name;

  /*
   * first we need to finish setup of the OS so that we can call other
   * functions in the server
   */
  OsInit();

  /*
   * now we decrese verbosity and remember the value, in case a later
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
  /*
   * we need to get the pointer to the pci data structures initialized
   */
  xf86PCIVendorInfo = 
    (pciVendorDeviceInfo*)LoaderSymbol("xf86PCIVendorInfoData");
  xf86PCIVendorNameInfo = 
    (SymTabPtr)LoaderSymbol("xf86PCIVendorNameInfoData");
  xf86PCICardInfo = 
    (pciVendorCardInfo*)LoaderSymbol("xf86PCICardInfoData");
#else
  xf86ScanPciFunc = xf86DisplayPCICardInfo;
  xf86PCIVendorNameInfo = xf86PCIVendorNameInfoData;
  xf86PCIVendorInfo = xf86PCIVendorInfoData;
  xf86PCICardInfo = xf86PCICardInfoData;
#endif

  (*xf86ScanPciFunc)(scanpciVerbose);

  /*
   * that's it; we really should clean things up, but a simple
   * exit seems to be all we need
   */
  exit(0);
}
