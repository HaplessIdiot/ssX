                 Building XFree86 from a Source Distribution

                         David Dawes, Matthieu Herrb

                              20 December 2002

                                  Abstract

     This document describes how to build XFree86 from the source dis-
     tribution.  It is designed to be used in conjunction with the OS-
     specific README files.

     NOTE:  Refer to the appropriate OS-specific README file before
     attempting to build XFree86.  These files contain additional infor-
     mation that you may need to successfully build under your OS.

We highly recommend using gcc to build XFree86, but it generally also builds
with the native compiler for each platform;

1.  How to get the XFree86 4.2.99.3 source

The source for the XFree86 4.2.99.3 snapshot is available from the XFree86
CVS repository.  See See http://www.xfree86.org/cvs/ for details.  The tag
for this snapshot is "xf-4_2_99_3".  To get the current development version,
don't specify any tag.

2.  Configuring the source before building

It is recommended that you start the configuration process by going to the
xc/config/cf directory, and copying the file xf86site.def to host.def.  Then
read through the host.def file (which is heavily commented), and set any
parameters that you want for your configuration.  You can usually find out
what the default settings are by checking the .cf file(s) relevant to your
OS.

Unlike previous versions, imake can now automatically detect and set the var-
ious OS*Version parameters, so you shouldn't need to enter those settings
explicitly.

3.  Using a shadow directory of symbolic links for the build

A recommended practice is to use a shadow directory of symbolic links to do
the build of XFree86. This permits to keep the source directory unmodified
during the build, which has the following benefits:

   o In the case where CVS is used to maintain the source tree up to date,
     the update process is not disturbed by foreign files not under the con-
     trol of CVS.

   o It is possible to build XFree86 for several different Operating System
     or architectures from the same sources, shared by NFS.

   o It is possible to build XFree86 with different configuration options,
     just by putting a real copyhost.def in each build tree and by customiz-
     ing it separately in each build tree.

To make a shadow directory of symbolic links, use the following steps:

   o create the directory at the top of the build tree. It is often created
     at the same level that the xc directory, but this is not mandatory.

          cd the directory containing the xcdirectory

          mkdir build

   o use the lndircommand to make the shadow tree:

          lndir ../xc

     Note that you can refer to the xc directory with an absolute path if
     needed.

     See the lndir(1) manual page for details.

If lndir is not already installed on your system, you can build it manually
from the XFree86 sources by running the following commands:

     cd xc/config/util

     make -f Makefile.ini lndir

     cp lndir some directory in your PATH

4.  Building and installing the distribution

Before building the distribution, read through the OS-specific README file in
xc/programs/Xserver/hw/xfree86/doc that is relevant to you.  Once those OS-
specific details have been taken care of, go your build directory (either the
xc directory or the shadow tree created before) and run ``make World'' with
the BOOTSTRAPCFLAGS set as described in the OS-specific README (if necessary,
but most systems supported by XFree86 don't need BOOTSTRAPCFLAGS).  It is
advisable to redirect stdout and stderr to World.Log so that you can track
down problems that might occur during the build.

With Bourne-like shells (Bash, the Korn shell, zsh, etc.) use a command like:

     make World > World.log 2>&1

Witch C-shell variants (csh, tcsh, etc), use:

     make World >& World.log

You can follow the progress of the build by running:

     tail -f World.log

 in a terminal.

When the build is finished, you should check World.Log to see if there were
any problems.  If there weren't any then you can install the binaries.  To do
the install, run ``make install'' and ``make install.man''.  Make sure you
have enough space in /usr/X11R6 for the install to succeed.  If you want to
install on a filesystem other than /usr, make a symbolic link to /usr/X11R6
before installing.

5.  Reconfiguring the server (source distribution)

To build a different set of servers or servers with a different set of
drivers installed:

  1.  Make sure the source for any new drivers is in the correct place (e.g.,
      driver source should be in a subdirectory of xc/pro-
      grams/Xserver/hw/xfree86/drivers).

  2.  Change the settings of the server defines in host.def to specify which
      servers you wish to build.  Also, change the driver lists to suit your
      needs.

  3.  From xc/programs/Xserver, run:

                make Makefile
                make Makefiles
                make includes
                make depend
                make

6.  Other useful make targets

There are some other useful targets defined in the top level Makefileof
XFree86:

   o Everything after a make World, make Everything does everything a make
     World does, except the cleaning of the tree. It is a way to quickly
     rebuild the tree after a source patch, but it is not 100% bullet proof.
     There are cases were it is better to force a full build by using make
     World.

   o clean does a partial cleaning of the source tree. Removes object files
     and generated manual pages, but leaves the Makefiles and the generated
     dependencies files in place. After a make clean you need to re-run

                  make includes
                  make depend
                  make

     to rebuild the XFree86.

   o distclean does a full cleaning of the source tree, removing all gener-
     ated files. After a make distclean, make World is the only option to
     rebuild XFree86.

   o includes generates all generated header files and in-tree symbolic links
     needed by the build. These files are removed by a make clean.

   o depend recomputes the dependencies for the various targets in all Make-
     files. Depending on the operating system, the dependencies are stored in
     the Makefile, or as a separate file, called .depend. This target needs
     the generated include files produced by make includes.

   o VerifyOS displays the detected operating system version. If the numbers
     shown do not match your system, you probably need to set them manually
     in host.def and report the problem to XFree86@XFree86.org.

     Generated from XFree86: xc/programs/Xserver/hw/xfree86/doc/sgml/BUILD.sgml,v 3.8 2002/12/21 03:47:40 dawes Exp $


$XFree86: $
