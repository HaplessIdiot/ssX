#!gmake
#
# $Date: 2001/02/01 08:28:31 $ $Revision: 1.1 $
# $Header: /vol1/history/xf86/xc/extras/ogl-sample/main/gfx/lib/opengl/drivers/Attic/GNUmakefile,v 1.1 2001/02/01 08:28:31 alanh Exp $

COMMONPREF = standard
include $(ROOT)/usr/include/make/commondefs

SUBDIRS = \
    DDX

default headers headers_install libs libs_install install apps:

distoss:
	$(MAKE) $(COMMONPREF)$@
	$(SUBDIRS_MAKERULE)

include $(COMMONRULES)
