#!/bin/sh

# $XConsortium: mfbmap.sh /main/3 1996/02/21 17:50:10 kaleb $
# $XFree86: xc/programs/Xserver/hw/xfree86/vga2/mfb/mfbmap.sh,v 1.2tsi Exp $
#
# This script recreates the mapping list that maps the mfb external
#  symbols * to vga2_*
# This should only be rerun if there have been changes in the mfb code
#  that affect the external symbols.
#  It assumes that Xserver/mfb has been compiled.
# The output goes to stdout.
echo "/* mfbmap.h */"
echo ""
echo "#ifndef _MFBMAP_H"
echo "#define _MFBMAP_H"
echo ""

nm ../../../../mfb/*.o | \
awk "{ if ((\$2 == \"D\") || (\$2 == \"T\") || (\$2 == \"C\")) print \$3 }" | \
sed s/^_// | \
grep -v "^ModuleInit$" | \
sort | \
awk "{ print \"#define \" \$1 \"  vga2_\"\$1 }"

echo ""
echo "#endif"
