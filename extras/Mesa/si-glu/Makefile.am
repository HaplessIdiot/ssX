# $Id: Makefile.am,v 1.1 2002/02/22 17:13:25 dawes Exp $
# Makefile for SGI SI GLU

AUTOMAKE_OPTIONS =

SUBDIRS = include libnurbs libtess libutil

lib_LTLIBRARIES = libGLU.la

libGLU_la_SOURCES = dummy.cc
libGLU_la_LDFLAGS = -version-info @SI_GLU_VERSION_INFO@ -L../src
libGLU_la_LIBADD = \
	-lGL \
	libnurbs/interface/libNIFac.la \
	libnurbs/internals/libNInt.la \
	libnurbs/nurbtess/libNTess.la \
	libtess/libtess.la \
	libutil/libutil.la


include $(top_srcdir)/common_rules.make

EXTRA_DIST = descrip.mms mesaglu.opt
