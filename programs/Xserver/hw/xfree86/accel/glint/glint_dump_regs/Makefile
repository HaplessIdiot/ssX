# $XFree86: $
#
# Makefile
#

XC=../../../../../../..

CC=gcc
CFLAGS=-O -Wall -ansi -pedantic $(MYCFLAGS)
MYCFLAGS=-g -I${XC}/programs/Xserver/hw/xfree86/common_hw -I${XC}/programs/Xserver/hw/xfree86/accel/glint
LIBS=

AR = ar
ARFLAGS = rcs

all:		glint_dump_regs 

glint_dump_regs:     glint_dump_regs.o lnx_video.o xf86_PCI.o miscutil.o xf86_IlHack.o
		$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

glint_dump_regs.o:	glint_dump_regs.c
		$(CC) $(CFLAGS) -c $^ 

lnx_video.o:	
		./cp_objfile ${XC} $@

xf86_PCI.o:	 
		./cp_objfile ${XC} $@

miscutil.o:	
		./cp_objfile ${XC} $@

xf86_IlHack.o:	
		./cp_objfile ${XC} $@

clean:		
		rm -f *.o glint_dump_regs

distclean:	clean
		rm -f *~













