XCOMM $XFree86: xc/lib/GL/mesa/src/drv/radeon/Imakefile.inc,v 1.2 2001/05/02 15:06:04 dawes Exp $

#ifndef MesaDrvSrcDir
#define MesaDrvSrcDir $(GLXLIBSRC)/mesa/src/drv
#endif

MESADRVSRCDIR = MesaDrvSrcDir

#ifndef MesaDrvRadeonBuildDir
#define MesaDrvRadeonBuildDir /**/
#endif
MESADRVRADEONBUILDDIR = MesaDrvRadeonBuildDir

#if Malloc0ReturnsNull
ALLOC_DEFINES = -DMALLOC_0_RETURNS_NULL
#endif

#if BuildXF86DRI
  DRI_DEFINES = GlxDefines -DX_BYTE_ORDER=ByteOrder
 DRI_INCLUDES = -I$(GLXLIBSRC)/dri \
		-I$(GLXLIBSRC)/glx \
		-I$(INCLUDESRC) \
		-I$(INCLUDESRC)/GL \
		-I$(SERVERSRC)/GL/dri \
		-I$(XF86OSSRC) \
		-I$(XF86DRIVERSRC)/ati \
		-I$(XF86COMSRC) \
		-I$(GLXLIBSRC)/dri/drm \
		-I$(GLXLIBSRC)/include
#endif

MESA_INCLUDES = -I$(MESASRCDIR)/src \
		-I$(MESADRVSRCDIR)/common \
		-I$(MESADRVSRCDIR)/radeon
   X_INCLUDES = -I$(XINCLUDESRC) -I$(EXTINCSRC)

   RADEONSRCS = $(MESADRVRADEONBUILDDIR)radeon_context.c \
		$(MESADRVRADEONBUILDDIR)radeon_ioctl.c \
		$(MESADRVRADEONBUILDDIR)radeon_lock.c \
		$(MESADRVRADEONBUILDDIR)radeon_render.c \
	        $(MESADRVRADEONBUILDDIR)radeon_screen.c \
		$(MESADRVRADEONBUILDDIR)radeon_span.c \
		$(MESADRVRADEONBUILDDIR)radeon_state.c \
		$(MESADRVRADEONBUILDDIR)radeon_tex.c \
		$(MESADRVRADEONBUILDDIR)radeon_texmem.c \
		$(MESADRVRADEONBUILDDIR)radeon_texstate.c \
		$(MESADRVRADEONBUILDDIR)radeon_tris.c \
		$(MESADRVRADEONBUILDDIR)radeon_vb.c 

   RADEONOBJS = $(MESADRVRADEONBUILDDIR)radeon_context.o \
		$(MESADRVRADEONBUILDDIR)radeon_ioctl.o \
		$(MESADRVRADEONBUILDDIR)radeon_lock.o \
		$(MESADRVRADEONBUILDDIR)radeon_render.o \
	        $(MESADRVRADEONBUILDDIR)radeon_screen.o \
		$(MESADRVRADEONBUILDDIR)radeon_span.o \
		$(MESADRVRADEONBUILDDIR)radeon_state.o \
		$(MESADRVRADEONBUILDDIR)radeon_tex.o \
		$(MESADRVRADEONBUILDDIR)radeon_texmem.o \
		$(MESADRVRADEONBUILDDIR)radeon_texstate.o \
		$(MESADRVRADEONBUILDDIR)radeon_tris.o \
		$(MESADRVRADEONBUILDDIR)radeon_vb.o 

   RADEONUOBJS = $(MESADRVRADEONBUILDDIR)unshared/radeon_context.o \
		$(MESADRVRADEONBUILDDIR)unshared/radeon_ioctl.o \
		$(MESADRVRADEONBUILDDIR)unshared/radeon_lock.o \
		$(MESADRVRADEONBUILDDIR)unshared/radeon_render.o \
	        $(MESADRVRADEONBUILDDIR)unshared/radeon_screen.o \
		$(MESADRVRADEONBUILDDIR)unshared/radeon_span.o \
		$(MESADRVRADEONBUILDDIR)unshared/radeon_state.o \
		$(MESADRVRADEONBUILDDIR)unshared/radeon_tex.o \
		$(MESADRVRADEONBUILDDIR)unshared/radeon_texmem.o \
		$(MESADRVRADEONBUILDDIR)unshared/radeon_texstate.o \
		$(MESADRVRADEONBUILDDIR)unshared/radeon_tris.o \
		$(MESADRVRADEONBUILDDIR)unshared/radeon_vb.o 

   RADEONDOBJS = $(MESADRVRADEONBUILDDIR)debugger/radeon_context.o \
		$(MESADRVRADEONBUILDDIR)debugger/radeon_ioctl.o \
		$(MESADRVRADEONBUILDDIR)debugger/radeon_lock.o \
		$(MESADRVRADEONBUILDDIR)debugger/radeon_render.o \
	        $(MESADRVRADEONBUILDDIR)debugger/radeon_screen.o \
		$(MESADRVRADEONBUILDDIR)debugger/radeon_span.o \
		$(MESADRVRADEONBUILDDIR)debugger/radeon_state.o \
		$(MESADRVRADEONBUILDDIR)debugger/radeon_tex.o \
		$(MESADRVRADEONBUILDDIR)debugger/radeon_texmem.o \
		$(MESADRVRADEONBUILDDIR)debugger/radeon_texstate.o \
		$(MESADRVRADEONBUILDDIR)debugger/radeon_tris.o \
		$(MESADRVRADEONBUILDDIR)debugger/radeon_vb.o 

   RADEONPOBJS = $(MESADRVRADEONBUILDDIR)profiled/radeon_context.o \
		$(MESADRVRADEONBUILDDIR)profiled/radeon_ioctl.o \
		$(MESADRVRADEONBUILDDIR)profiled/radeon_lock.o \
		$(MESADRVRADEONBUILDDIR)profiled/radeon_render.o \
	        $(MESADRVRADEONBUILDDIR)profiled/radeon_screen.o \
		$(MESADRVRADEONBUILDDIR)profiled/radeon_span.o \
		$(MESADRVRADEONBUILDDIR)profiled/radeon_state.o \
		$(MESADRVRADEONBUILDDIR)profiled/radeon_tex.o \
		$(MESADRVRADEONBUILDDIR)profiled/radeon_texmem.o \
		$(MESADRVRADEONBUILDDIR)profiled/radeon_texstate.o \
		$(MESADRVRADEONBUILDDIR)profiled/radeon_tris.o \
		$(MESADRVRADEONBUILDDIR)profiled/radeon_vb.o 

#ifdef NeedToLinkMesaSrc
LinkSourceFile(radeon_context.c, $(MESADRVSRCDIR)/radeon)
LinkSourceFile(radeon_ioctl.c, $(MESADRVSRCDIR)/radeon)
LinkSourceFile(radeon_lock.c, $(MESADRVSRCDIR)/radeon)
LinkSourceFile(radeon_render.c, $(MESADRVSRCDIR)/radeon)
LinkSourceFile(radeon_screen.c, $(MESADRVSRCDIR)/radeon)
LinkSourceFile(radeon_span.c, $(MESADRVSRCDIR)/radeon)
LinkSourceFile(radeon_state.c, $(MESADRVSRCDIR)/radeon)
LinkSourceFile(radeon_tex.c, $(MESADRVSRCDIR)/radeon)
LinkSourceFile(radeon_texmem.c, $(MESADRVSRCDIR)/radeon)
LinkSourceFile(radeon_texstate.c, $(MESADRVSRCDIR)/radeon)
LinkSourceFile(radeon_tris.c, $(MESADRVSRCDIR)/radeon)
LinkSourceFile(radeon_vb.c, $(MESADRVSRCDIR)/radeon)
#endif
