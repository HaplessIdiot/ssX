# $Id$

AUTOMAKE_OPTIONS = 

INCLUDES = -I../../include  -I../../../include

noinst_LTLIBRARIES = libNTess.la

# Choose one of STANDALONE, LIBRARYBUILD, and GLBUILD. They are debugging
# switches. grep LIBRARYBUILD `find .. -name '*.h'` for details.
CFLAGS += -DLIBRARYBUILD

include libNTess_la_SOURCES


include $(top_srcdir)/common_rules.make
