XCOMM $XFree86: xc/lib/GL/mesa/src/drv/i915/Imakefile.inc,v 1.2 2002/09/09 19:18:47 dawes Exp $

#ifndef MesaDrvSrcDir
#define MesaDrvSrcDir $(MESASRCDIR)/src/mesa/drivers/dri
#endif

MESADRVSRCDIR = MesaDrvSrcDir

#ifndef MesaDrvGDGBuildDir
#define MesaDrvGDGBuildDir /**/
#endif
MESADRVGDGBUILDDIR = MesaDrvGDGBuildDir

#if Malloc0ReturnsNull
ALLOC_DEFINES = -DMALLOC_0_RETURNS_NULL
#endif

#if BuildXF86DRI
  DRI_DEFINES = GlxDefines
  DRI_INCLUDES = -I$(GLXLIBSRC)/dri \
		-I$(XINCLUDESRC) \
		-I$(GLXLIBSRC)/glx \
		-I$(INCLUDESRC) \
		-I$(INCLUDESRC)/GL \
		-I$(GLXLIBSRC)/dri \
		-I$(GLXLIBSRC)/include \
		-I$(SERVERSRC)/GL/dri \
 		-I$(XF86OSSRC) \
 		-I$(XF86OSSRC)/shared/drm/kernel \
 		-I$(XF86DRIVERSRC)/i810 \
		-I$(GLXLIBSRC)/dri/drm \
		-I$(GLXLIBSRC)/include
#endif
 
MESA_INCLUDES = -I$(MESASRCDIR)/src/mesa \
		-I$(MESASRCDIR)/src/mesa/main \
		-I$(MESASRCDIR)/src/mesa/glapi \
		-I$(MESASRCDIR)/include \
		-I$(MESADRVSRCDIR)/common
   X_INCLUDES = -I$(XINCLUDESRC) -I$(EXTINCSRC)

     GDGSRCS = 	$(MESADRVGDGBUILDDIR)i830_metaops.c \
		$(MESADRVGDGBUILDDIR)i830_context.c \
		$(MESADRVGDGBUILDDIR)i830_state.c \
		$(MESADRVGDGBUILDDIR)i830_tex.c \
		$(MESADRVGDGBUILDDIR)i830_texstate.c \
		$(MESADRVGDGBUILDDIR)i830_texblend.c \
		$(MESADRVGDGBUILDDIR)i830_vtbl.c \
		$(MESADRVGDGBUILDDIR)i915_metaops.c \
		$(MESADRVGDGBUILDDIR)i915_context.c \
		$(MESADRVGDGBUILDDIR)i915_debug.c \
		$(MESADRVGDGBUILDDIR)i915_fragprog.c \
		$(MESADRVGDGBUILDDIR)i915_state.c \
		$(MESADRVGDGBUILDDIR)i915_tex.c \
		$(MESADRVGDGBUILDDIR)i915_texprog.c \
		$(MESADRVGDGBUILDDIR)i915_texstate.c \
		$(MESADRVGDGBUILDDIR)i915_program.c \
		$(MESADRVGDGBUILDDIR)i915_vtbl.c \
		$(MESADRVGDGBUILDDIR)intel_batchbuffer.c \
		$(MESADRVGDGBUILDDIR)intel_context.c \
		$(MESADRVGDGBUILDDIR)intel_ioctl.c \
		$(MESADRVGDGBUILDDIR)intel_pixel.c \
		$(MESADRVGDGBUILDDIR)intel_render.c \
		$(MESADRVGDGBUILDDIR)intel_screen.c \
		$(MESADRVGDGBUILDDIR)intel_span.c \
		$(MESADRVGDGBUILDDIR)intel_state.c \
		$(MESADRVGDGBUILDDIR)intel_tex.c \
		$(MESADRVGDGBUILDDIR)intel_tris.c 



     GDGOBJS = 	$(MESADRVGDGBUILDDIR)i830_metaops.o \
		$(MESADRVGDGBUILDDIR)i830_context.o \
		$(MESADRVGDGBUILDDIR)i830_state.o \
		$(MESADRVGDGBUILDDIR)i830_tex.o \
		$(MESADRVGDGBUILDDIR)i830_texstate.o \
		$(MESADRVGDGBUILDDIR)i830_texblend.o \
		$(MESADRVGDGBUILDDIR)i830_vtbl.o \
		$(MESADRVGDGBUILDDIR)i915_metaops.o \
		$(MESADRVGDGBUILDDIR)i915_context.o \
		$(MESADRVGDGBUILDDIR)i915_debug.o \
		$(MESADRVGDGBUILDDIR)i915_fragprog.o \
		$(MESADRVGDGBUILDDIR)i915_state.o \
		$(MESADRVGDGBUILDDIR)i915_tex.o \
		$(MESADRVGDGBUILDDIR)i915_texprog.o \
		$(MESADRVGDGBUILDDIR)i915_texstate.o \
		$(MESADRVGDGBUILDDIR)i915_program.o \
		$(MESADRVGDGBUILDDIR)i915_vtbl.o \
		$(MESADRVGDGBUILDDIR)intel_batchbuffer.o \
		$(MESADRVGDGBUILDDIR)intel_context.o \
		$(MESADRVGDGBUILDDIR)intel_ioctl.o \
		$(MESADRVGDGBUILDDIR)intel_pixel.o \
		$(MESADRVGDGBUILDDIR)intel_render.o \
		$(MESADRVGDGBUILDDIR)intel_screen.o \
		$(MESADRVGDGBUILDDIR)intel_span.o \
		$(MESADRVGDGBUILDDIR)intel_state.o \
		$(MESADRVGDGBUILDDIR)intel_tex.o \
		$(MESADRVGDGBUILDDIR)intel_tris.o 

    GDGUOBJS = 
    GDGDOBJS = 
    GDGPOBJS = 

#ifdef NeedToLinkMesaSrc
LinkSourceFile(i830_metaops.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_context.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_context.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_state.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_reg.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_tex.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_texstate.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_texblend.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i830_vtbl.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_metaops.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_context.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_context.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_debug.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_fragprog.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_state.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_tex.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_texprog.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_texstate.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_program.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_program.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_reg.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(i915_vtbl.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_batchbuffer.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_batchbuffer.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_context.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_context.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_ioctl.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_ioctl.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_pixel.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_reg.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_render.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_screen.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_screen.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_span.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_span.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_state.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_tex.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_tex.h, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_tris.c, $(MESADRVSRCDIR)/i915)
LinkSourceFile(intel_tris.h, $(MESADRVSRCDIR)/i915)
#endif
