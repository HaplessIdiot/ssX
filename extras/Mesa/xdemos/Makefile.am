## Process this file with automake to produce Makefile.in

#AUTOMAKE_OPTIONS = no-dependencies

INCLUDES = -I$(top_srcdir)/include $(GLUT_CFLAGS) $(X_CFLAGS) $(SVGA_CFLAGS)

if HAVE_SVGA
SVGA_PROG = vgears vindex vtest
endif

EXTRA_PROGRAMS = shape xfont

check_PROGRAMS = glthreads glxdemo glxheads glxinfo glxpixmap manywin offset \
	xdemo wincopy $(SVGA_PROG)

LDADD = $(GLUT_LIBS) $(top_builddir)/src-glu/libGLU.la \
	$(top_builddir)/src/libGL.la -lm
LDFLAGS = -no-install

# execute all programs
exec: $(check_PROGRAMS)
	@for prog in $(check_PROGRAMS);	\
	do				\
	 echo -n "Running $$prog ..." ;	\
	 ./$$prog ;			\
	 echo ;				\
	done
