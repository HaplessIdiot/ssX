lib_LTLIBRARIES=libfontconfig.la
libfontconfig_la_LDFLAGS=-version-info $(LT_CURRENT):$(LT_REVISION):$(LT_AGE)
CDEBUGFLAGS=-g
CFLAGS = $(CDEBUGFLAGS)  -Wall -Wpointer-arith -Wstrict-prototypes \
	-Wmissing-prototypes -Wmissing-declarations \
	-Wnested-externs

libfontconfig_la_SOURCES=\
	fcblanks.c \
        fccache.c \
        fccfg.c \
	fccharset.c \
        fcdbg.c \
        fcdefault.c \
        fcdir.c \
        fcfreetype.c \
        fcfs.c \
        fcinit.c \
        fclist.c \
        fcmatch.c \
        fcmatrix.c \
        fcname.c \
        fcpat.c \
        fcstr.c \
	fcxml.c

noinst_HEADERS=fcint.h
