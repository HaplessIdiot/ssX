/* Simple debugging component. Should be replaced by Pavel's version */
/* real soon now :)                                                  */

/* $XFree86: $ */
  
#ifndef TTDEBUG_H
#define TTDEBUG_H

#include <stdio.h>
#include "ttcommon.h"

  typedef char  DebugStr[128];

  DebugStr*  Cur_U_Line( void*  exec );

#endif /* TTDEBUG_H */
