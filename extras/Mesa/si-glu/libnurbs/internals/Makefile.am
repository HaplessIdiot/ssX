# $Id$

AUTOMAKE_OPTIONS = gnu no-dependencies

INCLUDES = -I../../include -I../nurbtess

noinst_LTLIBRARIES = libNInt.la

# Choose one of STANDALONE, LIBRARYBUILD, and GLBUILD. They are debugging
# switches. grep LIBRARYBUILD `find .. -name '*.h'` for details.
CXXFLAGS += -DLIBRARYBUILD

include libNInt_la_SOURCES


include $(top_srcdir)/common_rules.make
