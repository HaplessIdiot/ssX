# $Id$

AUTOMAKE_OPTIONS = gnu no-dependencies

INCLUDES = -I../../include -I../../../include -I../internals -I../nurbtess

noinst_LTLIBRARIES = libNIFac.la

# Choose one of STANDALONE, LIBRARYBUILD, and GLBUILD. They are debugging
# switches. grep LIBRARYBUILD `find .. -name '*.h'` for details.
CXXFLAGS += -DLIBRARYBUILD

include libNIFac_la_SOURCES


include $(top_srcdir)/common_rules.make
