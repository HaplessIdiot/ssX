XCOMM $XFree86: xc/lib/GL/mesa/drivers/dri/i830/Imakefile.inc,v 1.1tsi Exp $

#ifndef MesaDrvSrcDir
#define MesaDrvSrcDir $(MESASRCDIR)/src/mesa/drivers/dri
#endif

MESADRVSRCDIR = MesaDrvSrcDir

#ifndef MesaDrvI830BuildDir
#define MesaDrvI830BuildDir /**/
#endif
MESADRVI830BUILDDIR = MesaDrvI830BuildDir

#if Malloc0ReturnsNull
ALLOC_DEFINES = -DMALLOC_0_RETURNS_NULL
#endif

#if BuildXF86DRI
  DRI_DEFINES = GlxDefines -DX_BYTE_ORDER=$(X_BYTE_ORDER)
 DRI_INCLUDES = -I$(GLXLIBSRC)/dri \
		-I$(XINCLUDESRC) \
		-I$(GLXLIBSRC)/glx \
		-I$(GLXLIBSRC)/include \
		-I$(SERVERSRC)/GL/dri \
		-I$(XF86OSSRC) \
		-I$(XF86OSSRC)/shared/drm/kernel \
		-I$(XF86DRIVERSRC)/i810 \
		-I$(GLXLIBSRC)/dri/drm
#endif

MESA_INCLUDES = -I$(MESASRCDIR)/src/mesa \
		-I$(MESASRCDIR)/src/mesa/main \
		-I$(MESASRCDIR)/src/mesa/glapi \
		-I$(MESASRCDIR)/include \
		-I$(MESADRVSRCDIR)/common \
		-I$(MESADRVSRCDIR)/i830
   X_INCLUDES = -I$(XINCLUDESRC) -I$(EXTINCSRC)

     I830SRCS = $(MESADRVI830BUILDDIR)i830_context.c \
		$(MESADRVI830BUILDDIR)i830_debug.c \
		$(MESADRVI830BUILDDIR)i830_ioctl.c \
		$(MESADRVI830BUILDDIR)i830_render.c \
		$(MESADRVI830BUILDDIR)i830_screen.c \
		$(MESADRVI830BUILDDIR)i830_span.c \
		$(MESADRVI830BUILDDIR)i830_state.c \
		$(MESADRVI830BUILDDIR)i830_tex.c \
		$(MESADRVI830BUILDDIR)i830_texmem.c \
		$(MESADRVI830BUILDDIR)i830_texstate.c \
		$(MESADRVI830BUILDDIR)i830_tris.c 

     I830OBJS = $(MESADRVI830BUILDDIR)i830_context.o \
		$(MESADRVI830BUILDDIR)i830_debug.o \
		$(MESADRVI830BUILDDIR)i830_ioctl.o \
		$(MESADRVI830BUILDDIR)i830_render.o \
		$(MESADRVI830BUILDDIR)i830_screen.o \
		$(MESADRVI830BUILDDIR)i830_span.o \
		$(MESADRVI830BUILDDIR)i830_state.o \
		$(MESADRVI830BUILDDIR)i830_tex.o \
		$(MESADRVI830BUILDDIR)i830_texmem.o \
		$(MESADRVI830BUILDDIR)i830_texstate.o \
		$(MESADRVI830BUILDDIR)i830_tris.o

    I830EOBJS = $(MESADRVI830BUILDDIR)$(EXTRASUBDIR)/i830_context.o \
		$(MESADRVI830BUILDDIR)$(EXTRASUBDIR)/i830_debug.o \
		$(MESADRVI830BUILDDIR)$(EXTRASUBDIR)/i830_ioctl.o \
		$(MESADRVI830BUILDDIR)$(EXTRASUBDIR)/i830_render.o \
		$(MESADRVI830BUILDDIR)$(EXTRASUBDIR)/i830_screen.o \
		$(MESADRVI830BUILDDIR)$(EXTRASUBDIR)/i830_span.o \
		$(MESADRVI830BUILDDIR)$(EXTRASUBDIR)/i830_state.o \
		$(MESADRVI830BUILDDIR)$(EXTRASUBDIR)/i830_tex.o \
		$(MESADRVI830BUILDDIR)$(EXTRASUBDIR)/i830_texmem.o \
		$(MESADRVI830BUILDDIR)$(EXTRASUBDIR)/i830_texstate.o \
		$(MESADRVI830BUILDDIR)$(EXTRASUBDIR)/i830_tris.o 

    I830UOBJS = $(MESADRVI830BUILDDIR)unshared/i830_context.o \
		$(MESADRVI830BUILDDIR)unshared/i830_debug.o \
		$(MESADRVI830BUILDDIR)unshared/i830_ioctl.o \
		$(MESADRVI830BUILDDIR)unshared/i830_render.o \
		$(MESADRVI830BUILDDIR)unshared/i830_screen.o \
		$(MESADRVI830BUILDDIR)unshared/i830_span.o \
		$(MESADRVI830BUILDDIR)unshared/i830_state.o \
		$(MESADRVI830BUILDDIR)unshared/i830_tex.o \
		$(MESADRVI830BUILDDIR)unshared/i830_texmem.o \
		$(MESADRVI830BUILDDIR)unshared/i830_texstate.o \
		$(MESADRVI830BUILDDIR)unshared/i830_tris.o 

    I830DOBJS = $(MESADRVI830BUILDDIR)debugger/i830_context.o \
		$(MESADRVI830BUILDDIR)debugger/i830_debug.o \
		$(MESADRVI830BUILDDIR)debugger/i830_ioctl.o \
		$(MESADRVI830BUILDDIR)debugger/i830_render.o \
		$(MESADRVI830BUILDDIR)debugger/i830_screen.o \
		$(MESADRVI830BUILDDIR)debugger/i830_span.o \
		$(MESADRVI830BUILDDIR)debugger/i830_state.o \
		$(MESADRVI830BUILDDIR)debugger/i830_tex.o \
		$(MESADRVI830BUILDDIR)debugger/i830_texmem.o \
		$(MESADRVI830BUILDDIR)debugger/i830_texstate.o \
		$(MESADRVI830BUILDDIR)debugger/i830_tris.o

    I830POBJS = $(MESADRVI830BUILDDIR)profiled/i830_context.o \
		$(MESADRVI830BUILDDIR)profiled/i830_debug.o \
		$(MESADRVI830BUILDDIR)profiled/i830_ioctl.o \
		$(MESADRVI830BUILDDIR)profiled/i830_render.o \
		$(MESADRVI830BUILDDIR)profiled/i830_screen.o \
		$(MESADRVI830BUILDDIR)profiled/i830_span.o \
		$(MESADRVI830BUILDDIR)profiled/i830_state.o \
		$(MESADRVI830BUILDDIR)profiled/i830_tex.o \
		$(MESADRVI830BUILDDIR)profiled/i830_texmem.o \
		$(MESADRVI830BUILDDIR)profiled/i830_texstate.o \
		$(MESADRVI830BUILDDIR)profiled/i830_tris.o 

#ifdef NeedToLinkMesaSrc
LinkSourceFile(i830_3d_reg.h, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_context.c, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_context.h, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_debug.c, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_debug.h, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_ioctl.c, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_ioctl.h, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_render.c, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_screen.c, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_screen.h, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_span.c, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_span.h, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_state.c, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_state.h, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_tex.c, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_tex.h, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_texmem.c, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_texstate.c, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_tris.c, $(MESADRVSRCDIR)/i830)
LinkSourceFile(i830_tris.h, $(MESADRVSRCDIR)/i830)
#endif
