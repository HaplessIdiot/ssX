/* $XConsortium: xf86_IlHack.c,v 1.1 94/03/28 21:30:08 dpw Exp $ */
/*
 * This file is an incredible crock to get the normally-inline functions
 * built into the server so that things can be debugged properly.
 */


#define static /**/
#define __inline__ /**/
#undef NO_INLINE
#include "compiler.h"
