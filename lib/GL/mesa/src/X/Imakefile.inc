XCOMM $XFree86$

#ifndef MesaBuildDir
#define MesaBuildDir $(GLXLIBSRC)/mesa/src
#endif

MESABUILDDIR = MesaBuildDir

   XMESA_SRCS = $(MESABUILDDIR)/X/fakeglx.c \
		$(MESABUILDDIR)/X/glxapi.c \
		$(MESABUILDDIR)/X/realglx.c \
		$(MESABUILDDIR)/X/xfonts.c \
		$(MESABUILDDIR)/X/xm_api.c \
		$(MESABUILDDIR)/X/xm_dd.c \
		$(MESABUILDDIR)/X/xm_line.c \
		$(MESABUILDDIR)/X/xm_span.c \
		$(MESABUILDDIR)/X/xm_tri.c

   XMESA_OBJS = $(MESABUILDDIR)/X/fakeglx.o \
		$(MESABUILDDIR)/X/glxapi.o \
		$(MESABUILDDIR)/X/realglx.o \
		$(MESABUILDDIR)/X/xfonts.o \
		$(MESABUILDDIR)/X/xm_api.o \
		$(MESABUILDDIR)/X/xm_dd.o \
		$(MESABUILDDIR)/X/xm_line.o \
		$(MESABUILDDIR)/X/xm_span.o \
		$(MESABUILDDIR)/X/xm_tri.o

  XMESA_DOBJS = $(MESABUILDDIR)/X/debugger/fakeglx.o \
		$(MESABUILDDIR)/X/debugger/glxapi.o \
		$(MESABUILDDIR)/X/debugger/realglx.o \
		$(MESABUILDDIR)/X/debugger/xfonts.o \
		$(MESABUILDDIR)/X/debugger/xm_api.o \
		$(MESABUILDDIR)/X/debugger/xm_dd.o \
		$(MESABUILDDIR)/X/debugger/xm_line.o \
		$(MESABUILDDIR)/X/debugger/xm_span.o \
		$(MESABUILDDIR)/X/debugger/xm_tri.o

  XMESA_POBJS = $(MESABUILDDIR)/X/profiled/fakeglx.o \
		$(MESABUILDDIR)/X/profiled/glxapi.o \
		$(MESABUILDDIR)/X/profiled/realglx.o \
		$(MESABUILDDIR)/X/profiled/xfonts.o \
		$(MESABUILDDIR)/X/profiled/xm_api.o \
		$(MESABUILDDIR)/X/profiled/xm_dd.o \
		$(MESABUILDDIR)/X/profiled/xm_line.o \
		$(MESABUILDDIR)/X/profiled/xm_span.o \
		$(MESABUILDDIR)/X/profiled/xm_tri.o

  XMESA_UOBJS = $(MESABUILDDIR)/X/unshared/fakeglx.o \
		$(MESABUILDDIR)/X/unshared/glxapi.o \
		$(MESABUILDDIR)/X/unshared/realglx.o \
		$(MESABUILDDIR)/X/unshared/xfonts.o \
		$(MESABUILDDIR)/X/unshared/xm_api.o \
		$(MESABUILDDIR)/X/unshared/xm_dd.o \
		$(MESABUILDDIR)/X/unshared/xm_line.o \
		$(MESABUILDDIR)/X/unshared/xm_span.o \
		$(MESABUILDDIR)/X/unshared/xm_tri.o
