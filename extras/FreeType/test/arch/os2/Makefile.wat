# This file is part of the FreeType project
#
# This builds the test programs with the Watcom compiler
#
# You'll need Watcom's wmake
#
# Invoke by "wmake -f arch\os2\Makefile.wat" when in the "test" directory

ARCH = arch\os2
FT_MAKEFILE = $(ARCH)\Makefile.wat
FT_MAKE = wmake -h

.EXTENSIONS:
.EXTENSIONS: .exe .obj .c .h
.obj:.
.c:.
.h:.;..\lib

CC = wcl386

LIBDIR  = ..\lib
INCDIRS = -I$(LIBDIR) -I$(LIBDIR)\$(ARCH) -I$(LIBDIR)\extend
LIBFILE = $(LIBDIR)\libttf.lib

LINK_OPTS = 

OBJ_CFLAGS = /c /otexanl+ /s /w4 /zq $(INCDIRS)

CCFLAGS = /otexanl+ /s /w4 /zq $(INCDIRS)

GFSDRIVER = $(ARCH)\gfs_os2.obj
GFSDRIVER_SRC = $(ARCH)\gfs_os2.c

GPMDRIVER = $(ARCH)\gpm_os2.obj
GPMDRIVER_SRC = $(ARCH)\gpm_os2.c
GPMDRIVER_DEF = $(ARCH)\gpm_os2.def

DISPLAYSRC = display.c

SRC = gmain.c display.c &
      fttimer.c ftview.c ftlint.c ftzoom.c ftdump.c ftstring.c &
      $(GPMDRIVER_SRC) $(GFSDRIVER_SRC)

GFSOBJ = gmain.obj $(GFSDRIVER)
GPMOBJ = gmain.obj $(GPMDRIVER)

PM = $(LIBFILE) $(GPMOBJ) common.obj
FS = $(LIBFILE) $(GFSOBJ) common.obj

DISP = display.obj


# graphics utility and test driver

EXEFILES = ftview.exe ftviewfs.exe &
           fttimer.exe fttimefs.exe &
           ftlint.exe &
           ftdump.exe &
           ftstring.exe ftstrfs.exe &
           ftzoom.exe ftzoomfs.exe


all: freetype $(EXEFILES)

debug: freetype_debug $(EXEFILES)


freetype: .symbolic
  cd ..\lib
  $(FT_MAKE) -f $(FT_MAKEFILE) all
  cd ..\test

freetype_debug: .symbolic
  cd ..\lib
  $(FT_MAKE) -f $(FT_MAKEFILE) debug
  cd ..\test

# implicit rules
#
.c.obj :
  $(CC) $(OBJ_CFLAGS) $[* /fo=$[*.obj


# the full-screen graphics driver
#
$(GFSDRIVER): $(GFSDRIVER_SRC)
    $(CC) $(OBJ_CFLAGS) $[*.c /fo=$[*.obj

# the pm graphics driver
#
$(GPMDRIVER): $(GPMDRIVER_SRC)
  $(CC) $(OBJ_CFLAGS) $[*.c /fo=$[*.obj

ftzoom.exe : ftzoom.obj $(LIBFILE) $(PM) $(GPMDRIVER_DEF)
  $(CC) $(CCFLAGS) -l=os2v2_pm $(PM) $[*.c /fe=$[*.exe

ftzoomfs.exe : ftzoom.obj $(LIBFILE) $(FS)
  $(CC) $(CCFLAGS) $(FS) $[@ /fe=$^*.exe

ftview.exe : ftview.obj $(LIBFILE) $(PM) $(DISP) $(GPMDRIVER_DEF)
  $(CC) $(CCFLAGS) -l=os2v2_pm $(PM) $(DISP) $[*.c /fe=$[*.exe

ftviewfs.exe : ftview.obj $(LIBFILE) $(FS) $(DISP)
  $(CC) $(CCFLAGS) $(FS) $(DISP) $[*.c /fe=ftviewfs.exe

ftstring.exe : ftstring.obj $(LIBFILE) $(PM) $(GPMDRIVER_DEF)
  $(CC) $(CCFLAGS) -l=os2v2_pm $(PM) $(DISP) $[*.c /fe=$[*.exe 

ftstrfs.exe : ftstring.obj $(LIBFILE) $(FS) $(DISP)
  $(CC) $(CCFLAGS) $(FS) $(DISP) $[*.c /fe=ftstrfs.exe

fttimer.exe: fttimer.obj $(LIBFILE) $(PM) $(GPMDRIVER_DEF)
  $(CC) $(CCFLAGS) -l=os2v2_pm $(PM) $[*.c /fe=$[*.exe

fttimefs.exe: fttimer.obj $(LIBFILE) $(FS)
  $(CC) $(CCFLAGS) $(FS) $[*.c /fe=fttimefs.exe

ftlint.exe: ftlint.obj $(LIBFILE)
  $(CC) $(CCFLAGS) $(LIBFILE) common.obj $[*.c /fe=$[*.exe

ftdump.exe: ftdump.obj $(LIBFILE)
  $(CC) $(CCFLAGS) $(LIBFILE) common.obj $[*.c /fe=$[*.exe

clean: .symbolic
  @-erase *.obj
  @-erase $(ARCH)\*.obj

distclean: .symbolic  clean
  @-erase *.exe
  @-erase *.err
  cd ..\lib
  $(FT_MAKE) -f $(FT_MAKEFILE) distclean
  cd ..\test

new: .symbolic
  @-wtouch *.c

# end of Makefile.wat
