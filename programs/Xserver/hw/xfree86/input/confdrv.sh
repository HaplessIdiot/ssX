#!/bin/sh

# $XFree86: xc/programs/Xserver/hw/xfree86/input/confdrv.sh,v 1.2 1999/05/09 12:31:10 dawes Exp $
#
# This script generates drvConf.c
#
# usage: confdrv.sh driver1 driver2 ...
#

DRVCONF=./drvConf.c
BUILTIN="xf86KEYBOARD xf86MOUSE"

cat > $DRVCONF <<EOF
/*
 * This file is generated automatically -- DO NOT EDIT
 */

#include "xf86.h"
#include "xf86Xinput.h"

extern InputDriverRec
EOF
Args="$BUILTIN `echo $* | tr '[a-z]' '[A-Z]'`"
set - $Args
while [ $# -gt 1 ]; do
  echo "	$1," >> $DRVCONF
  shift
done
echo "	$1;" >> $DRVCONF
cat >> $DRVCONF <<EOF

InputDriverPtr xf86InputDriverList[] =
{
#ifdef NEW_INPUT
EOF
for i in $Args; do
  echo "	&$i," >> $DRVCONF
done
cat >> $DRVCONF <<EOF
#else
	NULL
#endif
};

int xf86NumInputDrivers = sizeof(xf86InputDriverList) / sizeof(xf86InputDriverList[0]);

EOF
