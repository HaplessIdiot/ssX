#include "xf86.h"
#include "vga.h"
#include "xf86Version.h"

#if defined(XFree86LOADER)
#ifdef S3_NEWMMIO
XF86ModuleVersionInfo s3newmmioVersRec =
#else
#ifdef S3_MMIO
XF86ModuleVersionInfo s3mmioVersRec =
#else
XF86ModuleVersionInfo s3pioVersRec =
#endif
#endif
{
#ifdef S3_NEWMMIO
	"libs3newmmio.a",
#else
#ifdef S3_MMIO
	"libs3mmio.a",
#else
	"libs3pio.a",
#endif
#endif
	"The XFree86 Project",
	MODINFOSTRING1,
	MODINFOSTRING2,
	XF86_VERSION_CURRENT,
	0x00010001,
	{0,0,0,0}	/* signature, to be patched into the file by a tool */
};

/*
 * this function returns the vgaVideoChipPtr for this driver
 *
 * it name has to be ModuleInit()
 */
void
ModuleInit(data,magic)
    pointer	* data;
    INT32	* magic;
{
    extern vgaVideoChipRec MGA;
    static int cnt = 0;

    switch(cnt++)
    {
    default:
        * magic= MAGIC_DONE;
        break;
    }
    return;
}
#endif  /* XFree86LOADER */
