#!/bin/sh

# $XFree86: xc/programs/Xserver/hw/xfree86/etc/postinst.sh,v 3.7 1996/09/01 04:46:21 dawes Exp $
#
# postinst.sh (for XFree86 3.1.2F)
#
# This script should be run after installing a new version of XFree86.
#

RUNDIR=/usr/X11R6

if [ ! -d $RUNDIR/. ]; then
	echo $RUNDIR does not exist
	exit 1
fi

# Since the misc fonts are distributed in two parts, make sure that the
# fonts.dir file is correct if only one part has been installed.
if [ -d $RUNDIR/lib/X11/fonts/misc ]; then
	echo ""
	echo "Updating fonts.dir file in $RUNDIR/lib/X11/fonts/misc"
	echo "This might take a while ..."
	$RUNDIR/bin/mkfontdir $RUNDIR/lib/X11/fonts/misc
fi

# Check if the system has a termcap file
TERMCAP1DIR=/usr/share
TERMCAP2=/etc/termcap
if [ -d $TERMCAP1DIR ]; then
	TERMCAP1=`find $TERMCAP1DIR -type f -name termcap -print 2> /dev/null`
	if [ x"$TERMCAP1" != x ]; then
		TERMCAPFILE="$TERMCAP1"
	fi
else
	if [ -f $TERMCAP2 ]; then
		TERMCAPFILE="$TERMCAP2"
	fi
fi
if [ x"$TERMCAPFILE" != x ]; then
	echo ""
	echo "You appear to have a termcap file: $TERMCAPFILE"
	echo "This should be edited manually to replace the xterm entries"
	echo "with those in $RUNDIR/lib/X11/etc/xterm.termcap"
fi

# Check for terminfo, and update the xterm entry
TINFODIR=/usr/lib/terminfo
OLDTINFO=" \
	x/xterms \
	x/xterm-24 \
	x/xterm-vi \
	v/vs100"
	
if [ -d $TINFODIR ]; then
	echo ""
	for t in $OLDTINFO; do
		if [ -f $TINFODIR/$t ]; then
			echo "Removing old terminfo file $TINFODIR/$t"
			rm -f $TINFODIR/$t
		fi
	done
	echo ""
	echo "Installing new terminfo entries for xterm"
	echo ""
	echo "On some systems you may get warnings from tic about 'meml'"
	echo "and 'memu'.  These warnings can safely be ignored."
	echo ""
	tic /usr/X11R6/lib/X11/etc/xterm.terminfo
fi

case `uname` in
	Linux|FreeBSD|NetBSD|OpenBSD)
		echo ""
		echo "You may need to reboot (or run ldconfig) before the"
		echo "newly installed shared libraries can be used."
		;;
esac

exit 0
