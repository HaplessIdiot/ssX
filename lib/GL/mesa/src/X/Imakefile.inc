XCOMM $XFree86: xc/lib/GL/mesa/src/X/Imakefile.inc,v 1.1 2002/02/22 21:32:56 dawes Exp $

#ifndef MesaXBuildDir
#define MesaXBuildDir $(GLXLIBSRC)/mesa/src/X/
#endif

MESAXBUILDDIR = MesaXBuildDir

   XMESA_SRCS = $(MESAXBUILDDIR)fakeglx.c \
		$(MESAXBUILDDIR)glxapi.c \
		$(MESAXBUILDDIR)realglx.c \
		$(MESAXBUILDDIR)xfonts.c \
		$(MESAXBUILDDIR)xm_api.c \
		$(MESAXBUILDDIR)xm_dd.c \
		$(MESAXBUILDDIR)xm_line.c \
		$(MESAXBUILDDIR)xm_span.c \
		$(MESAXBUILDDIR)xm_tri.c

#ifdef NeedToLinkMesaSrc
LinkSourceFile(fakeglx.c, $(MESASRCDIR)/src/X)
LinkSourceFile(glxapi.c, $(MESASRCDIR)/src/X)
LinkSourceFile(glxapi.h, $(MESASRCDIR)/src/X)
LinkSourceFile(realglx.c, $(MESASRCDIR)/src/X)
LinkSourceFile(xfonts.c, $(MESASRCDIR)/src/X)
LinkSourceFile(xm_api.c, $(MESASRCDIR)/src/X)
LinkSourceFile(xm_dd.c, $(MESASRCDIR)/src/X)
LinkSourceFile(xm_line.c, $(MESASRCDIR)/src/X)
LinkSourceFile(xm_span.c, $(MESASRCDIR)/src/X)
LinkSourceFile(xm_tri.c, $(MESASRCDIR)/src/X)
#endif

   XMESA_OBJS = $(MESAXBUILDDIR)fakeglx.o \
		$(MESAXBUILDDIR)glxapi.o \
		$(MESAXBUILDDIR)realglx.o \
		$(MESAXBUILDDIR)xfonts.o \
		$(MESAXBUILDDIR)xm_api.o \
		$(MESAXBUILDDIR)xm_dd.o \
		$(MESAXBUILDDIR)xm_line.o \
		$(MESAXBUILDDIR)xm_span.o \
		$(MESAXBUILDDIR)xm_tri.o

#if DoSharedLib
  XMESA_UOBJS = $(MESAXBUILDDIR)unshared/fakeglx.o \
		$(MESAXBUILDDIR)unshared/glxapi.o \
		$(MESAXBUILDDIR)unshared/realglx.o \
		$(MESAXBUILDDIR)unshared/xfonts.o \
		$(MESAXBUILDDIR)unshared/xm_api.o \
		$(MESAXBUILDDIR)unshared/xm_dd.o \
		$(MESAXBUILDDIR)unshared/xm_line.o \
		$(MESAXBUILDDIR)unshared/xm_span.o \
		$(MESAXBUILDDIR)unshared/xm_tri.o
#else
  XMESA_UOBJS = $(XMESA_OBJS)
#endif

  XMESA_DOBJS = $(MESAXBUILDDIR)debugger/fakeglx.o \
		$(MESAXBUILDDIR)debugger/glxapi.o \
		$(MESAXBUILDDIR)debugger/realglx.o \
		$(MESAXBUILDDIR)debugger/xfonts.o \
		$(MESAXBUILDDIR)debugger/xm_api.o \
		$(MESAXBUILDDIR)debugger/xm_dd.o \
		$(MESAXBUILDDIR)debugger/xm_line.o \
		$(MESAXBUILDDIR)debugger/xm_span.o \
		$(MESAXBUILDDIR)debugger/xm_tri.o

  XMESA_POBJS = $(MESAXBUILDDIR)profiled/fakeglx.o \
		$(MESAXBUILDDIR)profiled/glxapi.o \
		$(MESAXBUILDDIR)profiled/realglx.o \
		$(MESAXBUILDDIR)profiled/xfonts.o \
		$(MESAXBUILDDIR)profiled/xm_api.o \
		$(MESAXBUILDDIR)profiled/xm_dd.o \
		$(MESAXBUILDDIR)profiled/xm_line.o \
		$(MESAXBUILDDIR)profiled/xm_span.o \
		$(MESAXBUILDDIR)profiled/xm_tri.o

