/* $XFree86: xc/programs/Xserver/hw/xfree86/common/xf86Dl.c,v 3.0 1996/02/09 08:20:25 dawes Exp $ */

/*    
 * Copyright 1995 by Frederic Lepied, France. <fred@sugix.frmug.fr.net>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is  hereby granted without fee, provided that
 * the  above copyright   notice appear  in   all  copies and  that both  that
 * copyright  notice   and   this  permission   notice  appear  in  supporting
 * documentation, and that   the  name of  Frederic   Lepied not  be  used  in
 * advertising or publicity pertaining to distribution of the software without
 * specific,  written      prior  permission.     Frederic  Lepied   makes  no
 * representations about the suitability of this software for any purpose.  It
 * is provided "as is" without express or implied warranty.
 *
 * FREDERIC  LEPIED DISCLAIMS ALL   WARRANTIES WITH REGARD  TO  THIS SOFTWARE,
 * INCLUDING ALL IMPLIED   WARRANTIES OF MERCHANTABILITY  AND   FITNESS, IN NO
 * EVENT  SHALL FREDERIC  LEPIED BE   LIABLE   FOR ANY  SPECIAL, INDIRECT   OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA  OR PROFITS, WHETHER  IN  AN ACTION OF  CONTRACT,  NEGLIGENCE OR OTHER
 * TORTIOUS  ACTION, ARISING    OUT OF OR   IN  CONNECTION  WITH THE USE    OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <dlfcn.h>
#include "xf86Version.h"

#ifdef CSRG_BASED
#define PREPEND_UNDERSCORE
#endif

void*
xf86LoadModule(const char * path)
{
    void	*module;
    
#ifdef RTLD_NOW
    module = dlopen(path, RTLD_NOW);
#else
    module = dlopen(path, 1);	/* FreeBSD at least */
#endif
    
    if (!module) {
	Error(path);
    } else {
#ifdef PREPEND_UNDERSCORE
	int	(*init_module)() =
			(int (*)(unsigned long)) dlsym(module, "_init_module");
#else
	int	(*init_module)() =
			(int (*)(unsigned long)) dlsym(module, "init_module");
#endif
	
	if (init_module) {
	   if (!(*init_module)(XF86_VERSION_CURRENT)) {
		ErrorF("Warning: the module %s doesn't match server "
		       "version%s\n", path, XF86_VERSION);
	    }
	} else {
	    Error("unable to find init hook in module");
	}
    }
    
    return module;
}

/* end of xf86dl.c */
