
# Run this makefile to regenerate the sources files from APIspec.


default: glapitable.h \
	glapioffsets.h \
	glapitemp.h \
	glprocs.h \
	glapi_x86.S \
	glapi_sparc.S


clean:
	rm -f *.h *.S *~


install:
	mv -f *.h ../src
	mv -f glapi_x86.S ../src/X86
	mv -f glapi_sparc.S ../src/SPARC


glapitable.h: gltable.py APIspec
	python gltable.py > $@

glapioffsets.h: gloffsets.py APIspec
	python gloffsets.py > $@

glapi_x86.S: glx86asm.py APIspec
	python glx86asm.py > $@

glapi_sparc.S: glsparcasm.py APIspec
	python glsparcasm.py > $@

glapitemp.h: glapitemp.py APIspec
	python glapitemp.py > $@

glprocs.h: glprocs.py APIspec
	python glprocs.py > $@
