# $Id: Makefile.am,v 1.1 2002/02/22 17:13:25 dawes Exp $

AUTOMAKE_OPTIONS = gnu no-dependencies

INCLUDES = -I../../include -I../nurbtess

noinst_LTLIBRARIES = libNInt.la

# Choose one of STANDALONE, LIBRARYBUILD, and GLBUILD. They are debugging
# switches. grep LIBRARYBUILD `find .. -name '*.h'` for details.
CXXFLAGS += -DLIBRARYBUILD

include libNInt_la_SOURCES


include $(top_srcdir)/common_rules.make
