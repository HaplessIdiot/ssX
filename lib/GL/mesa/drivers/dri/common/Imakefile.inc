XCOMM $XFree86: xc/lib/GL/mesa/drivers/dri/common/Imakefile.inc,v 1.1tsi Exp $

#ifndef MesaDrvSrcDir
#define MesaDrvSrcDir $(MESASRCDIR)/src/mesa/drivers/dri
#endif

MESADRVSRCDIR = MesaDrvSrcDir

#ifndef MesaDrvCommonBuildDir
#define MesaDrvCommonBuildDir $(MESADRVSRCDIR)/common/
#endif

MESADRVCOMMONBUILDDIR = MesaDrvCommonBuildDir

#if Malloc0ReturnsNull
ALLOC_DEFINES = -DMALLOC_0_RETURNS_NULL
#endif

#if BuildXF86DRI
 COMMON_DRI_DEFINES = GlxDefines -DX_BYTE_ORDER=$(X_BYTE_ORDER)
 COMMON_DRI_INCLUDES = -I$(GLXLIBSRC)/dri \
		-I$(XINCLUDESRC) \
		-I$(GLXLIBSRC)/glx \
		-I$(GLXLIBSRC)/include \
		-I$(SERVERSRC)/GL/dri \
		-I$(XF86OSSRC) \
		-I$(XF86COMSRC) \
		-I$(GLXLIBSRC)/dri/drm \
		-I$(GLXLIBSRC)/include
#endif

COMMON_MESA_INCLUDES = -I$(MESASRCDIR)/src/mesa \
		-I$(MESASRCDIR)/src/mesa/main \
		-I$(MESASRCDIR)/src/mesa/glapi \
		-I$(MESADRVSRCDIR)/common \
		-I$(MESADRVSRCDIR)/../common \
		-I$(MESASRCDIR)/include
   COMMON_X_INCLUDES = -I$(XINCLUDESRC) -I$(EXTINCSRC)

    COMMONSRCS = $(MESADRVCOMMONBUILDDIR)mm.c \
                 $(MESADRVCOMMONBUILDDIR)utils.c \
                 $(MESADRVCOMMONBUILDDIR)vblank.c \
                 $(MESADRVCOMMONBUILDDIR)texmem.c \
                 $(MESADRVCOMMONBUILDDIR)xmlconfig.c

    COMMONOBJS = $(MESADRVCOMMONBUILDDIR)mm.o \
                 $(MESADRVCOMMONBUILDDIR)utils.o \
                 $(MESADRVCOMMONBUILDDIR)vblank.o \
                 $(MESADRVCOMMONBUILDDIR)texmem.o \
                 $(MESADRVCOMMONBUILDDIR)xmlconfig.o

    COMMONEOBJS = $(MESADRVCOMMONBUILDDIR)$(EXTRASUBDIR)/mm.o \
                 $(MESADRVCOMMONBUILDDIR)$(EXTRASUBDIR)/utils.o \
                 $(MESADRVCOMMONBUILDDIR)$(EXTRASUBDIR)/vblank.o \
                 $(MESADRVCOMMONBUILDDIR)$(EXTRASUBDIR)/texmem.o \
                 $(MESADRVCOMMONBUILDDIR)$(EXTRASUBDIR)/xmlconfig.o

    COMMONUOBJS = $(MESADRVCOMMONBUILDDIR)unshared/mm.o \
                 $(MESADRVCOMMONBUILDDIR)unshared/utils.o \
                 $(MESADRVCOMMONBUILDDIR)unshared/vblank.o \
                 $(MESADRVCOMMONBUILDDIR)unshared/texmem.o \
                 $(MESADRVCOMMONBUILDDIR)unshared/xmlconfig.o

    COMMONDOBJS = $(MESADRVCOMMONBUILDDIR)debugger/mm.o \
                 $(MESADRVCOMMONBUILDDIR)debugger/utils.o \
                 $(MESADRVCOMMONBUILDDIR)debugger/vblank.o \
                 $(MESADRVCOMMONBUILDDIR)debugger/texmem.o \
                 $(MESADRVCOMMONBUILDDIR)debugger/xmlconfig.o

    COMMONPOBJS = $(MESADRVCOMMONBUILDDIR)profiled/mm.o \
                 $(MESADRVCOMMONBUILDDIR)profiled/utils.o \
                 $(MESADRVCOMMONBUILDDIR)profiled/vblank.o \
                 $(MESADRVCOMMONBUILDDIR)profiled/texmem.o \
                 $(MESADRVCOMMONBUILDDIR)profiled/xmlconfig.o

#ifdef NeedToLinkMesaSrc
LinkSourceFile(mm.c, $(MESADRVSRCDIR)/common)
LinkSourceFile(mm.h, $(MESADRVSRCDIR)/common)
LinkSourceFile(texmem.c, $(MESADRVSRCDIR)/common)
LinkSourceFile(texmem.h, $(MESADRVSRCDIR)/common)
LinkSourceFile(utils.c, $(MESADRVSRCDIR)/common)
LinkSourceFile(utils.h, $(MESADRVSRCDIR)/common)
LinkSourceFile(vblank.c, $(MESADRVSRCDIR)/common)
LinkSourceFile(vblank.h, $(MESADRVSRCDIR)/common)
LinkSourceFile(xmlconfig.c, $(MESADRVSRCDIR)/common)
LinkSourceFile(xmlconfig.h, $(MESADRVSRCDIR)/common)
#endif
