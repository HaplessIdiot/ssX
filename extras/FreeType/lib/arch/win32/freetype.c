/* This file is part of the FreeType project */

/* single object library component for Win32 */
#define TT_MAKE_OPTION_SINGLE_OBJECT

/* Note, you should define the EXPORT macro here if you want  */
/* to build a Win32 DLL. If undefined, the macro is defined   */
/* as "" (i.e. void) by default. See 'freetype.h' for details */

/* The macro is placed before each high-level API function        */
/* declaration. You can then use it to take any compiler-specific */
/* pragma for DLL-exported symbols                                */

/* first include common core components */

#include "ttapi.c"
#include "ttcache.c"
#include "ttcalc.c"
#include "ttcmap.c"
#include "ttdebug.c"
#include "ttgload.c"
#include "ttinterp.c"
#include "ttload.c"
#include "ttobjs.c"
#include "ttraster.c"

/* then system-specific (or ANSI) components */

#include "ttfile.c"
#include "ttmemory.c"
#include "ttmutex.c"

/* finally, add some extensions */

#ifdef TT_CONFIG_OPTION_EXTEND_ENGINE
#include "ttextend.c"
#endif

/* end of freetype.c */
