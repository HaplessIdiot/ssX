/*
 * Dynamic module support v1.0 for Mesa 4.0.2
 *
 *  Copyright (C) 2002 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */

#ifndef DMESADXE_H_included
#define DMESADXE_H_included

#include <assert.h>
#include <ctype.h>
#include <dpmi.h>
#include <fcntl.h>
#include <float.h>
#include <math.h>
#include <pc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stubinfo.h>
#include <sys/exceptn.h>
#include <sys/farptr.h>
#include <unistd.h>

extern void __djgpp_hw_exception (void);

#include "dxe2.h"

DXE_EXPORT_TABLE_AUTO (syms)
    DXE_EXPORT (abort)
    DXE_EXPORT (atoi)
    DXE_EXPORT (bsearch)
    DXE_EXPORT (calloc)
    DXE_EXPORT (close)
    DXE_EXPORT (cos)
    DXE_EXPORT (dup)
    DXE_EXPORT (dup2)
    DXE_EXPORT (exp)
    DXE_EXPORT (fclose)
    DXE_EXPORT (fflush)
    DXE_EXPORT (fgets)
    DXE_EXPORT (fopen)
    DXE_EXPORT (fprintf)
    DXE_EXPORT (fputc)
    DXE_EXPORT (fputs)
    DXE_EXPORT (free)
    DXE_EXPORT (fwrite)
    DXE_EXPORT (getc)
    DXE_EXPORT (getenv)
    DXE_EXPORT (inportb)
    DXE_EXPORT (log)
    DXE_EXPORT (malloc)
    DXE_EXPORT (memcpy)
    DXE_EXPORT (memset)
    DXE_EXPORT (movedata)
    DXE_EXPORT (open)
    DXE_EXPORT (outportb)
    DXE_EXPORT (pow)
    DXE_EXPORT (printf)
    DXE_EXPORT (putchar)
    DXE_EXPORT (puts)
    DXE_EXPORT (qsort)
    DXE_EXPORT (realloc)
    DXE_EXPORT (remove)
    DXE_EXPORT (sin)
    DXE_EXPORT (sprintf)
    DXE_EXPORT (sqrt)
    DXE_EXPORT (strcmp)
    DXE_EXPORT (strcpy)
    DXE_EXPORT (strlen)
    DXE_EXPORT (strncmp)
    DXE_EXPORT (strncpy)
    DXE_EXPORT (strstr)
    DXE_EXPORT (tan)
    DXE_EXPORT (tmpnam)
    DXE_EXPORT (ungetc)
    DXE_EXPORT (_farnspeekb)
    DXE_EXPORT (_farnspeekw)
    DXE_EXPORT (_farnspokeb)
    DXE_EXPORT (_farnspokew)
    DXE_EXPORT (_farpeekw)
    DXE_EXPORT (_farpokel)
    DXE_EXPORT (_farsetsel)
    DXE_EXPORT (_go32_dpmi_lock_code)
    DXE_EXPORT (_go32_dpmi_lock_data)
    DXE_EXPORT (_stubinfo)
    DXE_EXPORT (__djgpp_dos_sel)
    DXE_EXPORT (__djgpp_ds_alias)
    DXE_EXPORT (__djgpp_hw_exception)
    DXE_EXPORT (__dj_assert)
    DXE_EXPORT (__dj_ctype_flags)
    DXE_EXPORT (__dj_float_min)
    DXE_EXPORT (__dj_stderr)
    DXE_EXPORT (__dj_stdout)
    DXE_EXPORT (__dpmi_allocate_ldt_descriptors)
    DXE_EXPORT (__dpmi_free_ldt_descriptor)
    DXE_EXPORT (__dpmi_free_physical_address_mapping)
    DXE_EXPORT (__dpmi_get_segment_base_address)
    DXE_EXPORT (__dpmi_int)
    DXE_EXPORT (__dpmi_physical_address_mapping)
    DXE_EXPORT (__dpmi_set_segment_base_address)
    DXE_EXPORT (__dpmi_set_segment_limit)
DXE_EXPORT_END

DECLARE_STATIC_DXE (GL)
DECLARE_STATIC_DXE (GLU)
DECLARE_STATIC_DXE (GLUT)

static void __attribute__((constructor)) GL_load (void)
{
 load_GL();
}

static void __attribute__((destructor)) GL_unload (void)
{
 unload_GL();
}

static void __attribute__((constructor)) GLU_load (void)
{
 load_GLU();
}

static void __attribute__((destructor)) GLU_unload (void)
{
 unload_GLU();
}

static void __attribute__((constructor)) GLUT_load (void)
{
 load_GLUT();
}

static void __attribute__((destructor)) GLUT_unload (void)
{
 unload_GLUT();
}

#endif
