/*
** gluos.h - operating system dependencies for GLU
**
** $Header: /vol1/history/xf86/xc/extras/Mesa/si-glu/include/Attic/gluos.h,v 1.1 2002/02/22 17:13:25 dawes Exp $
*/
#ifdef __VMS
#ifdef __cplusplus 
#pragma message disable nocordel
#pragma message disable codeunreachable
#pragma message disable codcauunr
#endif
#endif

#ifdef _WIN32
#include <stdlib.h>         /* For _MAX_PATH definition */
#include <stdio.h>
#include <malloc.h>

#define WIN32_LEAN_AND_MEAN
#define NOGDI
#define NOIME
#define NOMINMAX

#define _WIN32_WINNT 0x0400
#ifndef STRICT
  #define STRICT 1
#endif

#include <windows.h>

/* Disable warnings */
#pragma warning(disable : 4101)
#pragma warning(disable : 4244)
#pragma warning(disable : 4761)

#pragma comment(linker, "/OPT:NOWIN98")

#else

/* Disable Microsoft-specific keywords */
#define GLAPIENTRY
#define WINGDIAPI

#endif
