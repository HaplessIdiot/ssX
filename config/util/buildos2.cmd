@echo off
rem $XFree86$
rem this file is supposed to run from the xc/ dir.
rem you must copy it manually to there before using. It is just here
rem in order not to be in the root dir.
rem
rem copy some essential files to a location where we find them again
copy config\util\indir.cmd \ > nul
copy config\util\mkdirhier.cmd \ > nul
copy config\imake\imakesvc.cmd \ > nul
rem
set GCCOPT=-pipe
set EMXLOAD=5
emxload make.exe gcc.exe rm.exe mv.exe
make SHELL= MFLAGS="CC=gcc BOOTSTRAPCFLAGS=-DBSD43 SHELL= " -f Makefile.ini World.OS2 | tee buildxc.log
rem
rem cleanup the mess
rem del \indir.cmd
rem del \mkdirhier.cmd
rem del \imakesvc.cmd
rem del \imake.exe
rem del \makedepend.exe
