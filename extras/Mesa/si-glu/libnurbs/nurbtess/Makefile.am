# $Id: Makefile.am,v 1.1 2002/02/22 17:13:27 dawes Exp $

AUTOMAKE_OPTIONS = 

INCLUDES = -I../../include  -I../../../include

noinst_LTLIBRARIES = libNTess.la

# Choose one of STANDALONE, LIBRARYBUILD, and GLBUILD. They are debugging
# switches. grep LIBRARYBUILD `find .. -name '*.h'` for details.
CFLAGS += -DLIBRARYBUILD

include libNTess_la_SOURCES


include $(top_srcdir)/common_rules.make
