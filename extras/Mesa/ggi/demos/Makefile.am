## Process this file with automake to produce Makefile.in

INCLUDES = -I$(top_srcdir)/include

if HAVE_GGI
check_PROGRAMS = gears gears2 asc-view bigtest
endif

LDADD = $(top_builddir)/src-glu/libGLU.la \
	$(top_builddir)/src/libGL.la -lm -lggi -lgg -lglut

