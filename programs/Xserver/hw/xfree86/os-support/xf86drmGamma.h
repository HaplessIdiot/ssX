#ifndef __XF86DRI_GAMMA_H__
#define __XF86DRI_GAMMA_H__

/*
 * WARNING: If you change any of these defines, make sure to change
 * the kernel include file as well (gamma_drm.h)
 */

typedef struct {
   int sarea_priv_offset;
   int pcimode;
   unsigned int mmio0;
   unsigned int mmio1;
   unsigned int mmio2;
   unsigned int mmio3;
   unsigned int buffers_offset;
} drmGAMMAInit;

extern int drmGAMMAInitDMA( int fd, drmGAMMAInit *info );
extern int drmGAMMACleanupDMA( int fd );

#endif
