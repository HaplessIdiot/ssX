#!/bin/sh

# $XConsortium: confvga256.sh,v 1.1 94/03/28 21:43:21 dpw Exp $
# $XFree86: xc/programs/Xserver/hw/xfree86/vga256/cvga256.sh,v 3.0 1995/03/11 14:50:00 dawes Exp $
#
# This script generates cnec480Conf.c
#
# usage: cnec480.sh driver1 driver2 ...
#

VGACONF=./nec480Conf.c

cat > $VGACONF <<EOF
/*
 * This file is generated automatically -- DO NOT EDIT
 */

#include "xf86.h"
#include "vga.h"

extern vgaVideoChipRec
EOF
Args="`echo $* | tr '[a-z]' '[A-Z]'`"
set - $Args
while [ $# -gt 1 ]; do
  echo "        $1," >> $VGACONF
  shift
done
echo "        $1;" >> $VGACONF
cat >> $VGACONF <<EOF

vgaVideoChipPtr vgaDrivers[] =
{
EOF
for i in $Args; do
  echo "        &$i," >> $VGACONF
done
echo "        NULL" >> $VGACONF
echo "};" >> $VGACONF
