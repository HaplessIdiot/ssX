@echo off
rem $XFree86: xc/config/util/buildos2.cmd,v 3.1 1995/03/11 14:03:29 dawes Exp $
rem this file is supposed to run from the xc/ dir.
rem you must copy it manually to there before using. It is just here
rem in order not to be in the root dir.
rem
rem copy some essential files to a location where we find them again
copy config\util\indir.cmd \ > nul
copy config\util\mkdirhier.cmd \ > nul
copy config\imake\imakesvc.cmd \ > nul
copy Makefile Makefile.os2
rem
set GCCOPT=-pipe
set EMXLOAD=5
rem set LIBRARY_PATH=d:/emx/lib
rem set C_INCLUDE_PATH=d:/emx/include
emxload make.exe gcc.exe rm.exe mv.exe
make SHELL= MFLAGS="CC=gcc BOOTSTRAPCFLAGS=-DBSD43 SHELL= " World.OS2 2>&1 | tee buildxc.log
copy Makefile.os2 Makefile
rem
rem cleanup the mess
rem del \indir.cmd
rem del \mkdirhier.cmd
rem del \imakesvc.cmd
rem del \imake.exe
rem del \makedepend.exe
