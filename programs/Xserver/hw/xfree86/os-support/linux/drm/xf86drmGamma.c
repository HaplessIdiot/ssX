#ifdef XFree86Server
# include "xf86.h"
# include "xf86_OSproc.h"
# include "xf86_ansic.h"
# include "xf86Priv.h"
# define _DRM_MALLOC xalloc
# define _DRM_FREE   xfree
# ifndef XFree86LOADER
#  include <sys/stat.h>
#  include <sys/mman.h>
# endif
#else
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <string.h>
# include <ctype.h>
# include <fcntl.h>
# include <errno.h>
# include <signal.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <sys/ioctl.h>
# include <sys/mman.h>
# include <sys/time.h>
# ifdef DRM_USE_MALLOC
#  define _DRM_MALLOC malloc
#  define _DRM_FREE   free
extern int xf86InstallSIGIOHandler(int fd, void (*f)(int, void *), void *);
extern int xf86RemoveSIGIOHandler(int fd);
# else
#  include <X11/Xlibint.h>
#  define _DRM_MALLOC Xmalloc
#  define _DRM_FREE   Xfree
# endif
#endif

/* Not all systems have MAP_FAILED defined */
#ifndef MAP_FAILED
#define MAP_FAILED ((void *)-1)
#endif

#ifdef __linux__
#include <sys/sysmacros.h>	/* for makedev() */
#endif
#include "xf86drm.h"
#include "xf86drmGamma.h"
#include "drm.h"

int drmGAMMAInitDMA( int fd, drmGAMMAInit *info )
{
   drm_gamma_init_t init;

   memset( &init, 0, sizeof(drm_gamma_init_t) );

   init.func			= GAMMA_INIT_DMA;

   init.mmio0			= info->mmio0;
   init.mmio1			= info->mmio1;
   init.mmio2			= info->mmio2;
   init.mmio3			= info->mmio3;
   init.sarea_priv_offset	= info->sarea_priv_offset;
   init.pcimode			= info->pcimode;
   if (!init.pcimode)
   	init.buffers_offset		= info->buffers_offset;

   if ( ioctl( fd, DRM_IOCTL_GAMMA_INIT, &init ) ) {
      return -errno;
   } else {
      return 0;
   }
}

int drmGAMMACleanupDMA( int fd )
{
   drm_gamma_init_t init;

   memset( &init, 0, sizeof(drm_gamma_init_t) );

   init.func = GAMMA_CLEANUP_DMA;

   if ( ioctl( fd, DRM_IOCTL_GAMMA_INIT, &init ) ) {
      return -errno;
   } else {
      return 0;
   }
}
