XCOMM!/bin/sh

XCOMM $XFree86$

XCOMM
XCOMM Build Makefile for LinkKit
XCOMM

if [ x"$XWINHOME" != x ]; then
    CONFIG_DIR_SPEC=-I$XWINHOME/lib/X11/config
else
    CONFIG_DIR_SPEC=CONFIGDIRSPEC
fi

if [ -f Makefile ]; then
  (set -x
    rm -f Makefile.bak
    mv Makefile Makefile.bak
  )
fi
rm -f Makefile
(set -x
  imake -I. $CONFIG_DIR_SPEC -DUseInstalled -DXF86LinkKit=1 \
        -DTOPDIR=. -DCURDIR=.
  make Makefiles
XCOMM make clean
  make depend
)

