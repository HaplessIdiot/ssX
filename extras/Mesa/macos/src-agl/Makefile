AGL_SRC_DIR = ":"
OBJ_DIR	= "::Binaries.MPW:"
OPENGL_INC_DIR = ":::Headers:"
GLI_INC_DIR	=  ":::Headers:GLI:"

OBJECTS = {OBJ_DIR}maglContext.o ¶
		  {OBJ_DIR}maglError.o ¶
		  {OBJ_DIR}maglFont.o ¶
		  {OBJ_DIR}maglMemory.o ¶
		  {OBJ_DIR}maglPixelFormat.o ¶
		  {OBJ_DIR}maglRendererInfo.o ¶
		  {OBJ_DIR}maglRenderersMesa.o ¶
		  {OBJ_DIR}maglVirtualSrceen.o
		  
		  
COptions	= -i {OPENGL_INC_DIR},{GLI_INC_DIR}  -enum int -typecheck relaxed -opt speed -inline 1

MakeDepend	= -make depend.mpw

######## RULES ############

{OBJ_DIR} Ä {AGL_SRC_DIR}

.o Ä .c Makefile
	MrC4 {depDir}{default}.c -o {targDir}{default}.o {COptions} {MakeDepend}

All Ä {OBJECTS}