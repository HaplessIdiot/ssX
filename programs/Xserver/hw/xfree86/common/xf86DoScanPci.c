
/* $XFree86: $ */
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

static void (*xf86ScanPciFunc)(void);
void xf86DisplayPCICardInfo(void);

void
xf86ScanPciRegister(void(*func)(void))
{
  xf86ScanPciFunc = func;
}

void DoScanPci(int argc, char **argv, int i)
{
  int j,skip;
  int errmaj, errmin;
  char *name;

  /*
   * first we need to finish setup of the OS so that we can call other
   * functions in the server
   */
#ifdef DEBUG
  ErrorF("Calling OsInit\n");
#endif
  OsInit();

  /*
   * next we process the arguments that are remaining on the command line,
   * so that things like the module path can be set there
   */
  for ( j = i+1; j < argc; j++ ) {
#ifdef DEBUG
    ErrorF("Processing argument %d\n",j);
#endif
    if ((skip = ddxProcessArgument(argc, argv, j)))
	j += (skip - 1);
  } 

  /*
   * now get the loader set up and load the scanpci module
   */
#ifdef XFree86LOADER
  /* Initialise the loader */
#ifdef DEBUG
  ErrorF("initializing the loader\n");
#endif
  LoaderInit();
  /* Tell the loader the default module search path */
  LoaderSetPath(xf86ModulePath);


  /* Normalise the module name */
  name = xf86NormalizeName("scanpci");

#ifdef DEBUG
  ErrorF("loading the module \"%s\"\n",name);
#endif
  if (!LoadModule(name, NULL, NULL, NULL, NULL, NULL,
                  &errmaj, &errmin)) {
    LoaderErrorMsg(NULL, name, errmaj, errmin);
    exit(1);
  }
  if (LoaderCheckUnresolved(LD_RESOLV_IFDONE)) {
      /* For now, just a warning */
      xf86Msg(X_WARNING, "Some symbols could not be resolved!\n");
  }
#else
  xf86ScanPciFunc = xf86DisplayPCICardInfo;
#endif
#ifdef DEBUG
  ErrorF("calling scanpci\n");
#endif
  (*xf86ScanPciFunc)();

  /*
   * that's it; we really should clean things up, but a simple
   * exit seems to be all we need
   */
  exit(0);
}
