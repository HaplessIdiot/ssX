XCOMM $XFree86: xc/lib/font/X-TrueType/BIG5/Imakefile.inc,v 1.1 1999/03/28 15:31:39 dawes Exp $

#define ModuleName BIG5ETEN
MODULENAME = ModuleName
MODULESRCDIR = $(XTTSOURCEDIR)/$(MODULENAME)

SRCS =  main.c BIG5ETENtoUCS2.c
OBJS =  main.o BIG5ETENtoUCS2.o

LinkSourceFile(main.c,$(MODULESRCDIR))
LinkSourceFile(BIG5ETENtoUCS2.c,$(MODULESRCDIR))

