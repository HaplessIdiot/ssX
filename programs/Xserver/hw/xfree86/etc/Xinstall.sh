#!/bin/sh

#
# $XFree86: xc/programs/Xserver/hw/xfree86/etc/Xinstall.sh,v 1.3 2000/02/26 04:56:02 dawes Exp $
#
# Copyright © 1995-2000 by The XFree86 Project, Inc
# Portions Copyright © 2000 by Precision Insight, Inc
#
# Xinstall.sh  (for XFree86 4.0)
#
# This script should be used to install XFree86 4.0.
#
# Set tabs to 4 spaces to view/edit this file
#

VERSION=4.0

RUNDIR=/usr/X11R6
ETCDIR=/etc/X11
VARDIR=/var

if [ X"$1" = "X-test" -o X"$XINST_TEST" != X ]; then
	RUNDIR=/home1/test/X11R6
	ETCDIR=/home1/test/etcX11
	VARDIR=/home1/test/var
	if [ X"$1" = "X-test" ]; then
		shift
	fi
	echo ""
	echo "Running in test mode"
fi

OLDFILES=""

OLDDIRS=" \
	$RUNDIR/lib/X11/xkb/compiled \
	"

BASEDIST=" \
	Xbin.tgz \
	Xlib.tgz \
	Xman.tgz \
	Xdoc.tgz \
	Xfnts.tgz \
	Xfenc.tgz \
	"

ETCDIST="Xetc.tgz"

VARDIST="Xvar.tgz"

SERVDIST=" \
	Xxserv.tgz \
	Xmod.tgz \
	"
OPTDIST=" \
	Xfsrv.tgz \
	Xnest.tgz \
	Xprog.tgz \
	Xprt.tgz \
	Xvfb.tgz \
	Xf100.tgz \
	Xfcyr.tgz \
	Xflat2.tgz \
	Xfnon.tgz \
	Xfscl.tgz \
	Xhtml.tgz \
	Xjdoc.tgz \
	Xps.tgz \
	"

REQUIREDFILES=" \
	extract \
	$BASEDIST \
	$ETCDIST \
	$VARDIST \
	$SERVDIST \
	"

ETCLINKS=" \
	app-defaults \
	fs \
	lbxproxy \
	proxymngr \
	rstart \
	twm \
	xdm \
	xinit \
	xsm \
	xserver \
	"
# Check how to suppress newlines with echo (from perl's Configure)
(echo "xxx\c"; echo " ") > .echotmp
if grep c .echotmp >/dev/null 2>&1; then
	n='-n'
	c=''
else
	n=''
	c='\c'
fi
rm -f .echotmp

Echo()
{
	echo $n "$@""$c"
}

ContinueNo()
{
	Echo "Do you wish to continue? (y/n) [n] "
	read response
	case "$response" in
	[yY]*)
		echo ""
		;;
	*)
		echo "Aborting the installation."
		exit 2
		;;
	esac
}

ContinueYes()
{
	Echo "Do you wish to continue? (y/n) [y] "
	read response
	case "$response" in
	[nN]*)
		echo "Aborting the installation."
		exit 2
		;;
	*)
		echo ""
		;;
	esac
}

Description()
{
	case $1 in
	Xfsrv*)
		echo "font server";;
	Xnest*)
		echo "Nested X server";;
	Xprog*)
		echo "programmer support";;
	Xprt*)
		echo "X print server";;
	Xvfb*)
		echo "Virtual framebuffer X server";;
	Xf100*)
		echo "100dpi fonts";;
	Xfcyr*)
		echo "Cyrillic fonts";;
	Xflat2*)
		echo "Latin-2 fonts";;
	Xfnon*)
		echo "Some large fonts";;
	Xfscl*)
		echo "Scaled fonts (Speedo and Type1)";;
	Xhtml*)
		echo "Docs in HTML";;
	Xjdoc*)
		echo "Docs in Japanese";;
	Xps*)
		echo "Docs in PostScript";;
	*)
		echo "unknown";;
	esac
}

GetOsInfo()
{
	echo "Checking which OS your running..."

	OsName="`uname`"
	OsVersion="`uname -r`"
	case "$OsName" in
	SunOS) # Assumes SunOS 5.x
		OsArch="`uname -p`"
		;;
	*)
		OsArch="`uname -m`"
		;;
	esac
	# Some SVR4.0 versions have a buggy uname that reports the node name
	# for the OS name.  Try to catch that here.  Need to check what is
	# reported for non-buggy versions.
	if [ "$OsName" = "`uname -n`" -a -f /stand/unix ]; then
		OsName=UNIX_SV
	fi
	echo "uname reports '$OsName' version '$OsVersion', architecture '$OsArch'"

	# Find the object type, where needed

	case "$OsName" in
	Linux|FreeBSD)
		if file -L /bin/sh | grep ELF > /dev/null 2>&1; then
			OsObjFormat=ELF
		else
			OsObjFormat=a.out
		fi
		;;
	esac

	if [ X"$OsObjFormat" != X ]; then
		Echo "Object format is '$OsObjFormat' "
	fi

	# Find the libc version, where needed
	case "$OsName" in
	Linux)
		tmp="`ldd /bin/sh | grep libc.so 2> /dev/null`"
		OsLibcMajor=`expr "$tmp" : '.*libc.so.\([0-9]*\)'`
		case "$OsLibcMajor" in
		6)
			LibcPath=`expr "$tmp" : '[^/]*\(/[^ ]*\)'`
			tmp="`ls -l $LibcPath 2> /dev/null`"
			OsLibcMinor=`expr "$tmp" : '.*libc-2.\([0-9]*\)'`
			;;
		esac
		;;
	esac

	if [ X"$OsLibcMajor" != X ]; then
		Echo "libc version is '$OsLibcMajor"
		if [ X"$OsLibcMinor" != X ]; then
			Echo ".$OsLibcMinor'"
		else
			Echo "'"
		fi
	fi
	echo ""
	echo ""
	
	# test's flag for symlinks
	#
	# For OSs that don't support symlinks, choose a type that is guaranteed to
	# return false for regular files and directories.

	case "$OsName" in
	FreeBSD)
		case "$OsVersion" in
		2.*)
			L="-h"
			;;
		*)
			L="-L"
		esac
		;;
	*)
		L="-L"
		;;
	esac
}

DoOsChecks()
{
	# Do some OS-specific checks

	case "$OsName" in
	Linux)
		case "$OsObjFormat" in
		ELF)
			# Check ldconfig
			LDSO=`/sbin/ldconfig -v -n | awk '{ print $3 }'`
			LDSOMIN=`echo $LDSO | awk -F[.-] '{ print $3 }'`
			LDSOMID=`echo $LDSO | awk -F[.-] '{ print $2 }'`
			LDSOMAJ=`echo $LDSO | awk -F[.-] '{ print $1 }'`
			if [ "$LDSOMAJ" -gt 1 ]; then
				: OK
			else
				if [ "$LDSOMID" -gt 7 ]; then
					: OK
				else
					if [ "$LDSOMIN" -ge 14 ]; then
						: OK
					else
						echo ""
						echo "Before continuing, you will need to get a"
						echo "current version of ld.so.  Version 1.7.14 or"
						echo "newer will do."
						NEEDSOMETHING=YES
					fi
				fi
			fi
			;;
		esac
		# The /dev/tty0 check is left out.  Presumably nobody has a system where
		# this is missing any more.
		;;
	esac
}

FindDistName()
{
	case "$OsName" in
	DGUX)	# Check this string
		case "$OsArch" in
		i*86)
			DistName="DGUX-ix86"
			;;
		*)
			Message="DGUX binaries are only available for ix86 platforms"
			;;
		esac
		;;
	FreeBSD)
		case "$OsArch" in
		i386)
			case "$OsVersion" in
			2.2*)
				DistName="FreeBSD-2.2.x"
				;;
			3.*)
				case "$OsObjFormat" in
				ELF)
					DistName="FreeBSD-3.x"
					;;
				*)
					Message="FreeBSD 3.x binaries are only available in ELF format"
					;;
				esac
				;;
			4.*)
				DistName="FreeBSD-4.x"
				;;
			*)
				Message="FreeBSD/i386 binaries are not available for this version"
				;;
			esac
			;;
		alpha)
			case "$OsVersion" in
			3.*)
				DistName="FreeBSD-alpha-3.x"
				;;
			4.*)
				DistName="FreeBSD-alpha-4.x"
				;;
			*)
				Message="FreeBSD/alpha binaries are not available for this version"
				;;
			esac
			;;
		*)
			Message="FreeBSD binaries are not available for this architecture"
			;;
		esac
		;;
	Linux)
		case "$OsArch" in
		i*86)
			case "$OsLibcMajor" in
			5)
				DistName="Linux-ix86-libc5"
				;;
			6)
				case "$OsLibcMinor" in
				0)
					DistName="Linux-ix86-glibc20"
					;;
				1)
					DistName="Linux-ix86-glibc21"
					;;
				*)
					Message="No dist available for glibc 2.$OsLibcMinor.  Try Linux-ix86-glibc21"
					;;
				esac
				;;
			*)
				case "$OsObjFormat" in
				a.out)
					Message="Linux a.out is no longer supported"
					;;
				*)
					Message="No Linux/ix86 binaries for this libc version"
					;;
				esac
				;;
			esac
			;;
		alpha)
			case "$OsLibcMajor.$OsLibcMinor" in
			6.1)
				DistName="Linux-alpha-glibc21"
				;;
			6.*)
				Message="No Linux/alpha binaries for glibc 2.$OsLibcMinor.  Try Linux-alpha-glibc21"
				;;
			*)
				Message="No Linux/alpha binaries for this libc version"
				;;
			esac
			;;
		*)
			Message="No Linux binaries available for this architecture"
			;;
		esac
		;;
	LynxOS)	# Check this
		DistName="LynxOS"
		;;
	NetBSD)
		case "$OsArch" in
		i386)
			case "$OsVersion" in
			1.[3-9]*)	# Check this
				DistName="NetBSD-1.3"
				;;
			*)
				Message="No NetBSD/i386 binaries available for this version"
				;;
			esac
			;;
		*)
			Message="No NetBSD binaries available for this architecture"
			;;
		esac
		;;
	OpenBSD)
		case "$OsArch" in
		i386)
			case "$OsVersion" in
			2.[6-9]*)	# Check this
				DistName="OpenBSD-2.6"
				;;
			*)
				Message="No OpenBSD/i386 binaries available for this version"
				;;
			esac
			;;
		*)
			Message="No OpenBSD binaries available for this architecture"
			;;
		esac
		;;
	SunOS)
		case "$OsArch" in
		i386)
			case "$OsVersion" in
			5.[67]*)
				DistName="Solaris"
				;;
			5.8*)
				DistName="Solaris-8"
				;;
			*)
				Message="No Solaris/x86 binaries available for this version"
				;;
			esac
			;;
		*)
			Message="No SunOS/Solaris binaries available for this architecture"
			;;
		esac
		;;
	UNIX_SV)
		case "$OsArch" in
		i386)
			case "$OsVersion" in
			4.0*)
				DistName="SVR4.0"
				;;
			*)
				# More detailed version check??
				DistName="UnixWare"
				;;
			esac
			;;
		*)
			Message="No SYSV binaries available for this architecture"
			;;
		esac
		;;
	*)
		Message="No binaries available for this OS"
		;;
	esac

	if [ X"$DistName" != X ]; then
		echo "Binary distribution name is '$DistName'"
		echo ""
	else
		if [ X"$Message" = X ]; then
			echo "Can't find which binary distribution you should use."
			echo "Please send the output of this script to XFree86@XFree86.org"
			echo ""
		else
			echo "$Message"
			echo ""
		fi
	fi
}

if [ X"$1" = "X-check" ]; then
	GetOsInfo
	FindDistName
	exit 0
fi

echo ""
echo "		Welcome to the XFree86 $VERSION installer"
echo ""
echo "You are strongly advised to backup your existing XFree86 installation"
echo "Before proceeding.  This includes the /usr/X11R6 and /etc/X11"
echo "directories.  The installation process will overwrite existing files"
echo "in those directories, and this may include some configuration files"
echo "that may have been customised."
echo ""
ContinueNo

# Should check if uid is zero

# Check if $DISPLAY is set, and warn

if [ X"$DISPLAY" != X ]; then
	echo "\$DISPLAY is set, which may indicate that you are running this"
	echo "installation from an X session.  It is recommended that X not be"
	echo "running while doing the installation."
	echo ""
	ContinueNo
fi

# First, do some preliminary checks

GetOsInfo

echo "Checking for required files ..."
Needed=""

# Check for extract and extract.exe, and check that they are usable.
if [ -f extract ]; then
	ExtractExists=YES
	chmod +x extract
	if ./extract --version | head -1 | \
	  fgrep "extract (XFree86 version" > /dev/null 2>&1; then
		ExtractOK=YES
	else
		echo "extract doesn't work properly, renaming it to 'extract.bad'"
		rm -f extract.bad
		mv extract extract.bad
	fi
fi
if [ X"$ExtractOK" != XYES ]; then
	if [ -f extract.exe ]; then
		ExtractExeExists=YES
		rm -f extract
		ln extract.exe extract
		chmod +x extract
		if ./extract --version | head -1 | \
		  fgrep "extract (XFree86 version" > /dev/null 2>&1; then
			ExtractOK=YES
		else
			echo "extract.exe doesn't work properly, renaming it to"
			echo "'extract.exe.bad'"
			rm -f extract.exe.bad
			mv extract.exe extract.exe.bad
			rm -f extract
		fi
	fi
fi
if [ X"$ExtractOK" != XYES ]; then
	echo ""
	if [ X"$ExtractExists" = XYES -a X"$ExtractExeExists" = XYES ]; then
		echo "The versions of 'extract' and 'extract.exe' you have do not run'"
		echo "correctly.  Make sure that you have downloaded the correct"
		echo "binaries for your system.  To find out which is correct,"
		echo "run 'sh $0 -check'."
	fi
	if [ X"$ExtractExists" = XYES -a X"$ExtractExeExists" != XYES ]; then
		echo "The version of 'extract' you have does not run correctly."
		echo "This is most commonly due to problems downloading this file"
		echo "with some web browsers.  You may get better results if you"
		echo "download the version called 'extract.exe' and try again."
	fi
	if [ X"$ExtractExists" != XYES -a X"$ExtractExeExists" = XYES ]; then
		echo "The version of 'extract.exe' you have does not run correctly."
		echo "Make sure that you have downloaded the correct binaries for your"
		echo "system.  To find out which is correct, run 'sh $0 -check'."
	fi
	if [ X"$ExtractExists" != XYES -a X"$ExtractExeExists" != XYES ]; then
		echo "You need to download the 'extract' (or 'extract.exe') utility"
		echo "and put it in this directory."
	fi
	echo ""
	echo "When you have corrected the problem, please re-run 'sh $0'"
	echo "to proceed with the installation."
	echo ""
	exit 1
fi

for i in $REQUIREDFILES; do
	if [ ! -f $i ]; then
		Needed="$Needed $i"
	fi
done
if [ X"$Needed" != X ]; then
	echo ""
	echo "The files:"
	echo ""
	echo "$Needed"
	echo ""
	echo "must be present in the current directory to proceed with the"
	echo "installation.  You should be able to find it at the same place"
	echo "you picked up the rest of the XFree86 binary distribution."
	echo "Please re-run 'sh $0' to proceed with the installation when"
	echo "you have them."
	echo ""
	exit 1
fi

# Link extract to gnu-tar so it can also be used as a regular tar
rm -f gnu-tar
ln extract gnu-tar

WDIR=`pwd`
EXTRACT=$WDIR/extract
TAR=$WDIR/gnu-tar

DoOsChecks

if [ X"$NEEDSOMETHING" != X ]; then
	echo ""
	echo "Please re-run 'sh $0' to proceed with the installation after you"
	echo "have made the required updates."
	echo ""
	exit 1
fi

if [ X"$OLDFILES" != X ]; then
	echo ""
	echo "Removing some old files that are no longer required..."
	for i in $OLDFILES; do
		if [ -f $i ]; then
			echo "	removing old file $i"
			rm -f $i
		fi
	done
	echo ""
fi

if [ X"$OLDDIRS" != X ]; then
	echo ""
	echo "Removing some old directories that are no longer required..."
	for i in $OLDDIRS; do
		if [ -d $i ]; then
			echo "	removing old directory $i"
			rm -fr $i
		fi
	done
	echo ""
fi

# Create $RUNDIR and $ETCDIR if they don't already exist

if [ ! -d $RUNDIR ]; then
	NewRunDir=YES
	echo "Creating $RUNDIR"
	mkdir $RUNDIR
fi
if [ ! -d $RUNDIR/lib ]; then
	echo "Creating $RUNDIR/lib"
	mkdir $RUNDIR/lib
fi
if [ ! -d $RUNDIR/lib/X11 ]; then
	echo "Creating $RUNDIR/lib/X11"
	mkdir $RUNDIR/lib/X11
fi
if [ ! -d $ETCDIR ]; then
	NewEtcDir=YES
	echo "Creating $ETCDIR"
	mkdir $ETCDIR
fi

if [ -d $RUNDIR -a -d $RUNDIR/bin -a -d $RUNDIR/lib ]; then
	echo "You appear to have an existing installation of X.  Continuing will"
	echo "overwrite it.  You will, however, have the option of being prompted"
	echo "before most configuration files are overwritten."
	ContinueYes
fi

# Check for config file directories that may need to be moved.

EtcToMove=
for i in $ETCLINKS; do
	if [ -d $RUNDIR/lib/X11/$i -a ! $L $RUNDIR/lib/X11/$i ]; then
		EtcToMove="$EtcToMove $i"
	fi
done

if [ X"$EtcToMove" != X ]; then
	echo "XFree86 now installs most customisable configuration files under"
	echo "$ETCDIR instead of under $RUNDIR/lib/X11, and has symbolic links"
	echo "under $RUNDIR/lib/X11 that point to $ETCDIR.  You currently have"
	echo "files under the following subdirectories of $RUNDIR/lib/X11:"
	echo ""
	echo "$EtcToMove"
	echo ""
	echo "Do you want to move them to $ETCDIR and create the necessary"
	Echo "links? (y/n) [y] "
	read response
	case "$response" in
	[nN]*)
		NoSymLinks=YES;
		;;
	esac
	echo ""
	if [ X"NoSymlinks" != YES ]; then
		for i in $EtcToMove; do
			echo "Moving $RUNDIR/lib/X11/$i to $ETCDIR/$i ..."
			if [ ! -d $ETCDIR/$i ]; then
				mkdir $ETCDIR/$i
			fi
			$TAR -C $RUNDIR/lib/X11/$i -c -f - . | \
				$TAR -C $ETCDIR/$i -v -x -p -f - && \
				rm -fr $RUNDIR/lib/X11/$i && \
				ln -s $ETCDIR/$i $RUNDIR/lib/X11/$i
		done
	fi
fi

# Maybe allow a backup of the config files to be made?

# Extract Xetc.tgz into a temporary location, and prompt for moving the
# files.

echo "Extracting $ETCDIST into a temporary location ..."
rm -fr .etctmp
mkdir .etctmp
(cd .etctmp; $EXTRACT $WDIR/$ETCDIST)
for i in $ETCLINKS; do
	DoCopy=YES
	if [ -d $RUNDIR/lib/X11/$i ]; then
		Echo "Do you want to overwrite the $i config files? (y/n) [n] "
		read response
		case "$response" in
		[yY]*)
			: OK
			;;
		*)
			DoCopy=NO
			;;
		esac
	fi
	if [ $DoCopy = YES ]; then
		echo "Installing the $i config files ..."
		if [ ! -d $ETCDIR/$i ]; then
			mkdir $ETCDIR/$i
		fi
		if [ ! -d $RUNDIR/lib/X11/$i ]; then
			ln -s $ETCDIR/$i $RUNDIR/lib/X11/$i
		fi
		$TAR -C .etctmp/$i -c -f - . | $TAR -C $RUNDIR/lib/X11/$i -v -x -p -f -
	fi
done
rm -fr .etctmp

echo "Installing the mandatory parts of the binary distribution"
echo ""
for i in $BASEDIST $SERVDIST; do
	(cd $RUNDIR; $EXTRACT $WDIR/$i)
done
(cd $VARDIR; $EXTRACT $WDIR/$VARDIST)

echo "Checking for optional components to install ..."
for i in $OPTDIST; do
	if [ -f $i ]; then
		Echo "Do you want to install $i (`Description $i`)? (y/n) [y] "
		read response
		case "$response" in
		[nN]*)
			: skip this one
			;;
		*)
			(cd $RUNDIR; $EXTRACT $WDIR/$i)
			;;
		esac
	fi
done

echo ""
echo "Installation complete."

exit 0
