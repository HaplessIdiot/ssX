#!/bin/sh

# $XFree86: xc/programs/Xserver/hw/xfree98/vga256/cegc.sh,v 1.1 1997/03/17 12:13:15 hohndel Exp $
#
# This script generates cegcConf.c
#
# usage: cegc.sh driver1 driver2 ...
#
# $XConsortium: cegc.sh /main/3 1996/02/21 18:16:32 kaleb $

VGACONF=./egcConf.c

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

vgaVideoChipPtr videoDrivers[] =
{
EOF
for i in $Args; do
  echo "        &$i," >> $VGACONF
done
echo "        NULL" >> $VGACONF
echo "};" >> $VGACONF
