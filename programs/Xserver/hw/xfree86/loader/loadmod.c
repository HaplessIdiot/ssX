/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loadmod.c,v 1.10 1997/03/11 13:06:13 hohndel Exp $ */




/*
 *
 * Copyright 1995,96 by Metro Link, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#define NO_OSLIB_PROTOTYPES
#include "os.h"
#include "xf86_OSlib.h"
#define LOADERDECLARATIONS
#include "misc.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "vga.h"
#include "xf86_ldext.h"

void * LOADERVAR(cfbGCPrivateIndex);
void * LOADERVAR(endtab);

int  *xf86ccdScreenPrivateIndex = NULL;
void (*xf86ccdDoBitblt)() = NULL;
int  (*xf86ccdXAAScreenInit)() = NULL;
void *(*xf86xaacfbfuncs)() = NULL;
#ifdef PEXEXT
extern void (*PexExtensionInitPtr)(void);
#endif
#ifdef XIE
extern void (*XieInitPtr)(void);
#endif

extern int check_unresolved_sema;

LoaderFixups()
{
LOADERVAR(endtab) =
	(void *) LoaderSymbol("endtab");
}

static char *subdirs[] = 
{
	"",
	"drivers/",
	"extensions/",
	"internal/",
};
static char *prefixes[] =
{
	"",
	"lib",
};
static char *suffixes[] =
{
	"",
	".o",
	"_drv.o",
	".a",
};

char *
FindModule(module,dir)
	char	* module;
	char	* dir;
{
	char		* buf;
	int		  d,p,s;
	struct stat	  stat_buf;

	buf = (char*)xcalloc(1,strlen(module)+strlen(dir)+40);
	for( d = 0; d < sizeof(subdirs)/sizeof(char*); d++ )
	    for( p = 0; p < sizeof(prefixes)/sizeof(char*); p++ )
		for( s = 0; s < sizeof(suffixes)/sizeof(char*); s++ )
		{
			/*
			 * put together a possible filename
			 */
			strcpy(buf,dir);
			strcat(buf,subdirs[d]);
			strcat(buf,prefixes[p]);
			strcat(buf,module);
			strcat(buf,suffixes[s]);
#ifdef __EMX__
			strcpy(buf,(char*)__XOS2RedirRoot(buf));
#endif
			if ((stat(buf, &stat_buf) == 0) &&
			    ((S_IFMT & stat_buf.st_mode) == S_IFREG)) 
			{
				return(buf);
			}
		}
	xfree(buf);
	return(NULL);
}

void CheckVersion(module, data, cnt)
	char* module;
	XF86ModuleVersionInfo* data;
	int cnt;
{
	switch (cnt) {
	case 0:
		/* okay */
		break;
	case -1:
		/* no initfunc */
		ErrorF("LoadModule: Module %s does not have a ModuleInit routine. Please fix\n",
			module);
		return;
	default:
		/* not first item */
		ErrorF("LoadModule: ModuleInit of %s doesn't return MAGIC_VERSION as first data item. Please fix module!\n",
			module);
	}

	if (xf86Verbose) {
		char ver[8];
		ErrorF("\tModule %s: vendor=\"%s\"\n",
			data->modname ? data->modname : "UNKNOWN!",
			data->vendor ? data->vendor : "UNKNOWN!");
		ErrorF("\t  compiled for 0x%x, module version = 0x%x\n",
			data->xf86version, 
			data->modversion);
#if NOTYET
		if (data->checksum) {
			/* verify the checksum field */
			/* TO BE DONE */
		} else {
			ErrorF("\t*** Checksum field is 0 - this module is untrusted!\n");
		}
#endif
	}
}

void
LoadExtension(e)
        ExtensionModule *e;
{
        int i;

	if (e == NULL)
	    return;
	ErrorF("loading extension %s\n", e->name);

	for (i = 0; extension[i].name != NULL; i++) {
	    if (strcmp(extension[i].name, e->name) == 0) {
		extension[i].initFunc = e->initFunc;
		extension[i].disablePtr = e->disablePtr;
		break;
	    }
	} 
}

LoadModule(module,path)
	char	* module;
	char	* path;
{
	void	(*initfunc)() = NULL;
	pointer	  data;
	INT32	  magic;
	int	  version, cnt;

	char	* dir_elem;
	char	* found = NULL;
	char	* keep;
	char	* name;
	char	* path_elem;
	char	* p;

	keep = dir_elem = (char *) xcalloc(1, strlen(path) + 2);
	path_elem = (char *) xcalloc(1, strlen(path) + 2);
	*dir_elem = ',';
	strcpy(dir_elem+1, path);

	/* 
	 * if the module name is not a full pathname, we need to
	 * check the elements in the path
	 */
	if (module[0] == '/') 
		found = module;
	dir_elem = strtok(dir_elem, ",");
	while( (! found) && (dir_elem != NULL) )
	{
	    /*
	     * only allow fully specified path 
	     */
	    if (*dir_elem == '/') {
		strcpy(path_elem, dir_elem);
		if (dir_elem[strlen(dir_elem) - 1] != '/') 
		{
		    path_elem[strlen(dir_elem)] = '/';
		    path_elem[strlen(dir_elem)+1] = '\0';
		}
		found = FindModule(module,path_elem);
	    }
	    dir_elem = strtok(NULL, ",");
	}

	/*
	 * did we find the module?
	 */
	if( !found )
	{
		ErrorF("Warning, couldn't open module %s\n",module);
		goto LoadModule_exit;
	}
	LoaderOpen(found);
	/*
	 * now check if there's the special function
	 * <modulename>ModuleInit
	 * and if yes, call it.
	 */
	name = (char*)xalloc(strlen(found)+strlen("ModuleInit")+1);
	strcpy(name,found);
	p = strchr(name,'.'); /* get rid of the .o/.a/.so */
	if( p )
	    *p = '\0';
	strcat(name,"ModuleInit");
	p = strrchr(name,'/');
	if( p )
	    p++;
	else
	    p=name;
	initfunc = (void (*)())LoaderSymbol(p);
	xfree(name);

	version = 0;
	if( initfunc )
	{
		do
		{
			cnt = 0;

			initfunc(&data,&magic);
			switch(magic)
			{
			case MAGIC_DONE:
				break;
			case MAGIC_ADD_VIDEO_CHIP_REC:
				addChipRec((void *)data);
				break;
			case MAGIC_LOAD:
				LoadModule((char*)data,path);
				break;
			case MAGIC_CCD_DO_BITBLT:
				xf86ccdDoBitblt = (void((*)()))data;
				break;
			case MAGIC_CCD_SCREEN_PRIV_IDX:
				xf86ccdScreenPrivateIndex = data;
				break;
			case MAGIC_CCD_XAA_SCREEN_INIT:
				xf86ccdXAAScreenInit = (int((*)()))data;
				break;
			case MAGIC_LOAD_EXTENSION:
			        LoadExtension((ExtensionModule *)data);
			        break;
			case MAGIC_VERSION:
				version++;
				CheckVersion(module,(XF86ModuleVersionInfo*)data,cnt);
				break;

			case MAGIC_DONT_CHECK_UNRESOLVED:
				check_unresolved_sema++;
				break;

			default:
				ErrorF("Unknown magic action %d\n",magic);
				break;
			}
			cnt++;
		}
		while(magic != MAGIC_DONE);
	} else
	{
		/* no initfunc, complain */
		CheckVersion(module,0,-1);
	}
	xfree(found);

LoadModule_exit:

	xfree(path_elem);
	xfree(keep);
}

