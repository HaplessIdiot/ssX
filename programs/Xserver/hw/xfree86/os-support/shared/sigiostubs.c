/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/shared/sigio.c,v 1.3 1999/10/13 22:33:07 dawes Exp $ */

#ifdef XFree86Server
# include "X.h"
# include "xf86.h"
# include "xf86drm.h"
# include "xf86Priv.h"
# include "xf86_OSlib.h"
# include "xf86drm.h"
#else
# include <unistd.h>
# include <signal.h>
# include <fcntl.h>
# include <sys/time.h>
# include <errno.h>
#endif

int
xf86InstallSIGIOHandler(int fd, void (*f)(int, void *), void *closure)
{
    return 0;
}

int
xf86RemoveSIGIOHandler(int fd)
{
    return 0;
}

int
xf86BlockSIGIO (void)
{
    return 0;
}

void
xf86UnblockSIGIO (int wasset)
{
}

#ifdef XFree86Server
void
xf86AssertBlockedSIGIO (char *where)
{
}
#endif
