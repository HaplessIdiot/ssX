XCOMM $XFree86: xc/lib/GL/mesa/src/drv/ffb/Imakefile.inc,v 1.2 2001/05/02 15:06:03 dawes Exp $

#ifndef MesaDrvSrcDir
#define MesaDrvSrcDir $(GLXLIBSRC)/mesa/src/drv
#endif

MESADRVSRCDIR = MesaDrvSrcDir

#ifndef MesaDrvFfbBuildDir
#define MesaDrvFfbBuildDir /**/
#endif
MESADRVRADEONBUILDDIR = MesaDrvFfbBuildDir

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
		-I$(MESADRVSRCDIR)/ffb
   X_INCLUDES = -I$(XINCLUDESRC) -I$(EXTINCSRC)

   FFBSRCS    = $(MESADRVRADEONBUILDDIR)ffb_bitmap.c \
		$(MESADRVRADEONBUILDDIR)ffb_clear.c \
		$(MESADRVRADEONBUILDDIR)ffb_dd.c \
		$(MESADRVRADEONBUILDDIR)ffb_depth.c \
                $(MESADRVRADEONBUILDDIR)ffb_fog.c \
		$(MESADRVRADEONBUILDDIR)ffb_lines.c \
		$(MESADRVRADEONBUILDDIR)ffb_points.c \
		$(MESADRVRADEONBUILDDIR)ffb_span.c \
		$(MESADRVRADEONBUILDDIR)ffb_state.c \
		$(MESADRVRADEONBUILDDIR)ffb_stencil.c \
		$(MESADRVRADEONBUILDDIR)ffb_tex.c \
		$(MESADRVRADEONBUILDDIR)ffb_tris.c \
		$(MESADRVRADEONBUILDDIR)ffb_vb.c \
		$(MESADRVRADEONBUILDDIR)ffb_xmesa.c

   FFBOBJS    = $(MESADRVRADEONBUILDDIR)ffb_bitmap.o \
		$(MESADRVRADEONBUILDDIR)ffb_clear.o \
		$(MESADRVRADEONBUILDDIR)ffb_dd.o \
		$(MESADRVRADEONBUILDDIR)ffb_depth.o \
                $(MESADRVRADEONBUILDDIR)ffb_fog.o \
		$(MESADRVRADEONBUILDDIR)ffb_lines.o \
		$(MESADRVRADEONBUILDDIR)ffb_points.o \
		$(MESADRVRADEONBUILDDIR)ffb_span.o \
		$(MESADRVRADEONBUILDDIR)ffb_state.o \
		$(MESADRVRADEONBUILDDIR)ffb_stencil.o \
		$(MESADRVRADEONBUILDDIR)ffb_tex.o \
		$(MESADRVRADEONBUILDDIR)ffb_tris.o \
		$(MESADRVRADEONBUILDDIR)ffb_vb.o \
		$(MESADRVRADEONBUILDDIR)ffb_xmesa.o

   FFBUOBJS   = $(MESADRVRADEONBUILDDIR)unshared/ffb_bitmap.o \
		$(MESADRVRADEONBUILDDIR)unshared/ffb_clear.o \
		$(MESADRVRADEONBUILDDIR)unshared/ffb_dd.o \
		$(MESADRVRADEONBUILDDIR)unshared/ffb_depth.o \
                $(MESADRVRADEONBUILDDIR)unshared/ffb_fog.o \
		$(MESADRVRADEONBUILDDIR)unshared/ffb_lines.o \
		$(MESADRVRADEONBUILDDIR)unshared/ffb_points.o \
		$(MESADRVRADEONBUILDDIR)unshared/ffb_span.o \
		$(MESADRVRADEONBUILDDIR)unshared/ffb_state.o \
		$(MESADRVRADEONBUILDDIR)unshared/ffb_stencil.o \
		$(MESADRVRADEONBUILDDIR)unshared/ffb_tex.o \
		$(MESADRVRADEONBUILDDIR)unshared/ffb_tris.o \
		$(MESADRVRADEONBUILDDIR)unshared/ffb_vb.o \
		$(MESADRVRADEONBUILDDIR)unshared/ffb_xmesa.o

   FFBDOBJS   = $(MESADRVRADEONBUILDDIR)debugger/ffb_bitmap.o \
		$(MESADRVRADEONBUILDDIR)debugger/ffb_clear.o \
		$(MESADRVRADEONBUILDDIR)debugger/ffb_dd.o \
		$(MESADRVRADEONBUILDDIR)debugger/ffb_depth.o \
                $(MESADRVRADEONBUILDDIR)debugger/ffb_fog.o \
		$(MESADRVRADEONBUILDDIR)debugger/ffb_lines.o \
		$(MESADRVRADEONBUILDDIR)debugger/ffb_points.o \
		$(MESADRVRADEONBUILDDIR)debugger/ffb_span.o \
		$(MESADRVRADEONBUILDDIR)debugger/ffb_state.o \
		$(MESADRVRADEONBUILDDIR)debugger/ffb_stencil.o \
		$(MESADRVRADEONBUILDDIR)debugger/ffb_tex.o \
		$(MESADRVRADEONBUILDDIR)debugger/ffb_tris.o \
		$(MESADRVRADEONBUILDDIR)debugger/ffb_vb.o \
		$(MESADRVRADEONBUILDDIR)debugger/ffb_xmesa.o

   FFBPOBJS   = $(MESADRVRADEONBUILDDIR)profiled/ffb_bitmap.o \
		$(MESADRVRADEONBUILDDIR)profiled/ffb_clear.o \
		$(MESADRVRADEONBUILDDIR)profiled/ffb_dd.o \
		$(MESADRVRADEONBUILDDIR)profiled/ffb_depth.o \
                $(MESADRVRADEONBUILDDIR)profiled/ffb_fog.o \
		$(MESADRVRADEONBUILDDIR)profiled/ffb_lines.o \
		$(MESADRVRADEONBUILDDIR)profiled/ffb_points.o \
		$(MESADRVRADEONBUILDDIR)profiled/ffb_span.o \
		$(MESADRVRADEONBUILDDIR)profiled/ffb_state.o \
		$(MESADRVRADEONBUILDDIR)profiled/ffb_stencil.o \
		$(MESADRVRADEONBUILDDIR)profiled/ffb_tex.o \
		$(MESADRVRADEONBUILDDIR)profiled/ffb_tris.o \
		$(MESADRVRADEONBUILDDIR)profiled/ffb_vb.o \
		$(MESADRVRADEONBUILDDIR)profiled/ffb_xmesa.o

#ifdef NeedToLinkMesaSrc
LinkSourceFile(ffb_bitmap.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_clear.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_dd.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_depth.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_fog.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_lines.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_points.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_span.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_state.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_stencil.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_tex.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_tris.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_vb.c, $(MESADRVSRCDIR)/ffb)
LinkSourceFile(ffb_xmesa.c, $(MESADRVSRCDIR)/ffb)
#endif
