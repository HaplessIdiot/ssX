#!/bin/sh

# $XFree86: xc/programs/Xserver/hw/xfree98/vga256/cwabs.sh,v 3.0 1995/12/17 10:06:32 dawes Exp $
#
# This script generates wabsConf.c
#
# usage: cwabs.sh driver1 driver2 ...
#
# $XConsortium: cwabs.sh /main/2 1995/12/29 11:51:23 kaleb $

VGACONF=./wabsConf.c

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
