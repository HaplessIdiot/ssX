#!/bin/sh

# $XFree86: xc/programs/Xserver/hw/xfree86/etc/preinst.sh,v 3.1 1996/03/03 03:57:23 dawes Exp $
#
# preinst.sh  (for XFree86 3.1.2F)
#
# This script should be run before installing a beta version.
# It removes symbolic links to old beta versions.
#

OLDBETADIR_1=/usr/XFree86-3.1.2A
OLDBETADIR_2=/usr/XFree86-3.1.2B
RUNDIR=/usr/X11R6
LIBLIST=" \
	libICE.so \
	libPEX5.so \
	libSM.so \
	libX11.so \
	libXIE.so \
	libXaw.so \
	libXext.so \
	libXi.so \
	libXmu.so \
	libXt.so \
	libXtst.so \
	liboldX.so \
	libICE.so.6 \
	libPEX5.so.6 \
	libSM.so.6 \
	libX11.so.6 \
	libXIE.so.6 \
	libXaw.so.6 \
	libXext.so.6 \
	libXi.so.6 \
	libXmu.so.6 \
	libXt.so.6 \
	libXtst.so.6 \
	liboldX.so.6 \
	"

if [ ! -d $RUNDIR/. ]; then
	echo $RUNDIR does not exist
	echo "There is no need to run this script if you don't have an older"
	echo "version of XFree86 installed"
	exit 1
fi

echo ""
echo "You are strongly advised to backup your /usr/X11R6 directory before"
echo "proceeding with this installation.  This installation will overwrite"
echo "existing files."
echo ""
echo "Do you want to continue? (y/n) "
read response
case "$response" in
	[yY]*)
		;;
	*)
		echo Aborting
		exit 1
		;;
esac

if [ -d $OLDBETADIR_2 ]; then
	cd $OLDBETADIR_2
	for i in `find * -type f -print`; do
		if [ -h $RUNDIR/$i ]; then
			echo Removing link to $OLDBETADIR_2/$i
			rm -f $RUNDIR/$i
		fi
	done
fi
if [ -d $OLDBETADIR_1 ]; then
	cd $OLDBETADIR_1
	for i in `find * -type f -print`; do
		if [ -h $RUNDIR/$i ]; then
			echo Removing link to $OLDBETADIR_1/$i
			rm -f $RUNDIR/$i
		fi
	done
fi
for i in $LIBLIST; do
	if [ -h $RUNDIR/lib/$i ]; then
		echo Removing old library link $RUNDIR/lib/$i
		rm -f $RUNDIR/lib/$i
	fi
done

exit 0
