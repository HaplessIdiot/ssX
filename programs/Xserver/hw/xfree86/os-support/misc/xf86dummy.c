/* $XFree86: $ */
/*
 * this file is here to let Xprt link when linked again libxf86_os.a
 *
 * so all unresolved symbols should be defined here
 */

#define NO_OSLIB_PROTOTYPES
#include "xf86Procs.h"

xf86InfoRec xf86Info;
ScrnInfoPtr xf86Screens[10];
