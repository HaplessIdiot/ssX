/* $XFree86: xc/programs/Xserver/hw/xfree86/loader/loadmod.c,v 1.3 1997/02/18 17:51:43 hohndel Exp $ */




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

#define LOADERDECLARATIONS
#include "misc.h"
#include "xf86.h"
#include "xf86Priv.h"
#include "vga.h"

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


LoaderFixups()
{
LOADERVAR(endtab) =
	(void *) LoaderSymbol("endtab");
}

LoadModule(module,path)
	char	* module;
	char	* path;
{
	char	* buf;
	void	(*initfunc)() = NULL;
	pointer	  data;
	INT32	  magic;

	char	* p;
	char	* name;


	buf = (char*)xcalloc(1,strlen(module)+strlen(path)+2);

	strcpy(buf,path);
	strcat(buf,"/");
	strcat(buf,module);
	LoaderOpen(buf);
	/*
	 * now check if there's the special function
	 * <modulename>ModuleInit
	 * and if yes, call it.
	 */
	name = (char*)xalloc(strlen(module)+strlen("ModuleInit")+1);
	strcpy(name,module);
	p = strchr(name,'.'); /* get rid of the .o/.a/.so */
	if( p )
	    *p = '\0';
	strcat(name,"ModuleInit");
	initfunc = (void (*)())LoaderSymbol(name);
	xfree(name);
	if( initfunc )
	{
		do
		{
			initfunc(&data,&magic);
			switch(magic)
			{
			case MAGIC_DONE:
				break;
			case MAGIC_ADD_VIDEO_CHIP_REC:
				addChipRec((vgaVideoChipRec*)data);
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
#ifdef PEXEXT
			case MAGIC_PEX_INIT:
				PexExtensionInitPtr = (void((*)()))data;
				break;
#endif
#ifdef XIE
			case MAGIC_XIE_INIT:
				XieInitPtr = (void((*)()))data;
				break;
#endif
			default:
				ErrorF("Unknown magic action %d\n",magic);
				break;
			}
		}
		while(magic != MAGIC_DONE);
	}
}

