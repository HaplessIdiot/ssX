# $XFree86$

SHARED=		../shared
I951SHAREDFILES= \
		i915.h \
		i915_dma.c \
		i915_drm.h \
		i915_drv.h \
		i915_irq.c \
		i915_mem.c \

SHAREDFILES=	drm.h \
		drm_sarea.h \
		drm_pciids.txt \
		mach64.h \
		mach64_dma.c \
		mach64_drm.h \
		mach64_drv.h \
		mach64_irq.c \
		mach64_state.c \
		mga.h \
		mga_dma.c \
		mga_drm.h \
		mga_drv.h \
		mga_irq.c \
		mga_state.c \
		mga_ucode.h \
		mga_warp.c \
		r128.h \
		r128_cce.c \
		r128_drm.h \
		r128_drv.h \
		r128_irq.c \
		r128_state.c \
		radeon.h \
		radeon_cp.c \
		radeon_drm.h \
		radeon_drv.h \
		radeon_irq.c \
		radeon_mem.c \
		radeon_state.c \
		sis.h \
		sis_drm.h \
		sis_drv.h \
		sis_ds.c \
		sis_ds.h \
		sis_mm.c \
		tdfx.h \
		via.h \
		via_drm.h \
		via_drv.c \
		via_drv.h \
		via_ds.c \
		via_ds.h \
		via_irq.c \
		via_map.c \
		via_mm.c \
		via_mm.h \
		via_3d_reg.h \
		via_dma.c

SCRIPT=		../scripts
SCRIPTFILES=	create_bsd_pci_lists.sh

# SUBDIR = i915 mach64 mga r128 radeon sis tdfx
SUBDIR = mga r128 radeon sis tdfx

.if exists(${SHARED})
CLEANFILES+= ${SHAREDFILES}
.endif

.if exists(${SCRIPT})
CLEANFILES+= ${SCRIPTFILES}
.endif

.include <bsd.obj.mk>

depend: drm_pciids.h ${SHAREDFILES} ${SCRIPTFILES}
all: drm_pciids.h ${SHAREDFILES} ${SCRIPTFILES}

drm_pciids.h: drm_pciids.txt ${SCRIPTFILES}
	sh create_bsd_pci_lists.sh < drm_pciids.txt

.if exists(${SHARED})
CLEANFILES+= ${SHAREDFILES}
${SHAREDFILES}:
	ln -sf ${SHARED}/$@ $@
.endif

.if exists(${SCRIPT})
CLEANFILES+= ${SCRIPTFILES}
${SCRIPTFILES}:
	ln -sf ${SCRIPT}/$@ $@
.endif
