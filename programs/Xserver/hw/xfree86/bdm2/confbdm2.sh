# $XConsortium: confbdm2.sh,v 1.1 94/03/28 21:18:42 dpw Exp $
#!/bin/sh

#
# This script generates bdm2Conf.c
#
# usage: configbdm.sh driver1 driver2 ...
#

BDMCONF=./bdm2Conf.c

cat > $BDMCONF <<EOF
/*
 * This file is generated automatically -- DO NOT EDIT
 */

#include "bdm.h"

extern bdmVideoChipRec
EOF
Args="`echo $* | tr '[a-z]' '[A-Z]'`"
set - $Args
while [ $# -gt 1 ]; do
  echo "        $1," >> $BDMCONF
  shift
done
echo "        $1;" >> $BDMCONF
cat >> $BDMCONF <<EOF

bdmVideoChipPtr bdmDrivers[] =
{
EOF
for i in $Args; do
  echo "        &$i," >> $BDMCONF
done
echo "        NULL" >> $BDMCONF
echo "};" >> $BDMCONF
