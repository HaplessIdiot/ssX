#ifdef XFree86LOADER

#include "xf86.h"
#include "xf86Version.h"

extern void BitmapRegisterFontFileFunctions();

static XF86ModuleVersionInfo VersRec =
{
	"libbitmap.a",
	MODULEVENDORSTRING,
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00010001,
	{0,0,0,0}       /* signature, to be patched into the file by a tool */
};

void libbitmapModuleInit(data,magic)
    pointer *	data;
    INT32 *	magic;
{
    static int  cnt = 0;

    switch(cnt++)
    {
    case 0:
	* magic = MAGIC_VERSION;
	* data = (pointer) &VersRec;
	break;
    case 1:
	* magic = MAGIC_LOAD_FONT;
	* data = (pointer) &BitmapRegisterFontFileFunctions;
	break;
    default:
    	* magic = MAGIC_DONE;
	break;
    }
    return;
}
#endif
