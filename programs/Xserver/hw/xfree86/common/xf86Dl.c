/* $XFree86$ */

#include <dlfcn.h>

void*
xf86LoadModule(const char * path)
{
    void	*module;
    
    module = dlopen(path, RTLD_NOW);
    
    if (!module) {
	Error(path);
    } else {
	void	(*init_module)() = dlsym(module, "init_module");
	
	if (init_module) {
	    (*init_module)();
	} else {
	    Error("unable to find init hook in module");
	}
    }
    
    return module;
}

/* end of xf86dl.c */
