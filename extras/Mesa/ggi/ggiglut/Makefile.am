INCLUDES = -I$(top_srcdir)/include $(extra_includes)
libglut_la_LIBADD = -lggi -lgg -lGL

lib_LTLIBRARIES = libglut.la

libglut_la_SOURCES = ggiglut.c
libglut_la_LDFLAGS = $(extra_libraries) -version-info 1:0 -no-undefined \
	-export-symbols $(srcdir)/EXPSYMS

ggiincludedir = $(includedir)/ggi
# ggiinclude_HEADERS = ggiglut.h

EXTRA_DIST = EXPSYMS
