/* $XFree86: xc/programs/Xserver/hw/xfree86/os-support/int10/xf86x86emu.c,v 1.3 1999/12/03 19:17:41 eich Exp $ */
/*
 *                   XFree86 int10 module
 *   execute BIOS int 10h calls in x86 real mode environment
 *                 Copyright 1999 Egbert Eich
 */
#include <x86emu.h>
#include "xf86.h"
#include "xf86str.h"
#include "compiler.h"
#include "xf86_OSproc.h"
#include "xf86_ansic.h"
#include "xf86Pci.h"
#include "xf86_libc.h"
#define _INT10_PRIVATE
#include "xf86int10.h"
#include "int10Defines.h"

#define M            _X86EMU_env

static void
x86emu_do_int(int num)
{
    Int10Current->num = num;
    if (!int_handler(Int10Current)) {
	xf86Msg(X_ERROR,"\nUnknown vm86_int: %X\n\n",num);
	X86EMU_halt_sys();
    }
    return;
}
    
void
xf86ExecX86int10(xf86Int10InfoPtr pInt)
{
    setup_int(pInt);

    if (int_handler(pInt)) {
	X86EMU_exec();	
    }
    
    finish_int(pInt);
}
    
Bool
xf86Int10ExecSetup(xf86Int10InfoPtr pInt)
{
    int i;
    X86EMU_intrFuncs intFuncs[256];
    X86EMU_pioFuncs pioFuncs = {
	(CARD8(*)(CARD16))p_inb,
	(u16(*)(CARD16))p_inw,
	(u32(*)(CARD16))p_inl,
	(void(*)(CARD16,CARD8))p_outb,
	(void(*)(CARD16,CARD16))p_outw,
	(void(*)(CARD16,CARD32))p_outl
    };
    
    X86EMU_memFuncs memFuncs = {
	(CARD8(*)(CARD32))Mem_rb,
	(CARD16(*)(CARD32))Mem_rw,
	(CARD32(*)(CARD32))Mem_rl,
	(void(*)(CARD32,CARD8))Mem_wb,
	(void(*)(CARD32,CARD16))Mem_ww,
	(void(*)(CARD32,CARD32))Mem_wl
    };

    X86EMU_setupMemFuncs(&memFuncs);
    
    pInt->cpuRegs =  &M;
    M.mem_base = 0;
    M.mem_size = 1024*1024 + 1024;
    X86EMU_setupPioFuncs(&pioFuncs);

    for (i=0;i<256;i++)
	intFuncs[i] = x86emu_do_int;
    X86EMU_setupIntrFuncs(intFuncs);
    return TRUE;
}

void
printk(const char *fmt, ...)
{
    va_list argptr;
    va_start(argptr, fmt);
    ErrorF(fmt, argptr);
    fflush(stdout);
    va_end(argptr);
}


