/* $XConsortium: bdmBank.s,v 1.2 94/03/29 13:32:41 dpw Exp $ */
/*
 * BDM2: Banked dumb monochrome driver
 * Pascal Haible 8/93, haible@izfm.uni-stuttgart.de
 *
 * bdm2/bdm/bdmBank.s
 *
 * derived from:
 * hga2/*
 * Author:  Davor Matic, dmatic@athena.mit.edu
 * and
 * vga256/*
 * Copyright 1990,91 by Thomas Roell, Dinkelscherben, Germany.
 *
 * see bdm2/COPYRIGHT for copyright and disclaimers.
 */

/* $XFree86: mit/server/ddx/x386/bdm2/bdm/bdmBank.s,v 2.2 1993/10/02 16:53:22 dawes Exp $ */

/********** WARNING: **********/
/* If you use bdmBank.s instead of bdmBank.c the lowlevel banking
 * functions must be in assembly language, too.
 * number of bank is in %eax, only %eax and %edx may be used without
 * saving the original contents.
 */

/* Some info from vga256/vga/vgaBank.s */
/*
 * Because a modern VGA has more than 128kBytes (which are mappable into the
 * 386' memory some logic is required. The VGA's memory (logical VGA
 * address space) is devided into smaller parts (called logical segments). 
 * These segments are mapped to logical areas.
 *
 * There are there different logical mapping areas:
 *
 * Read:       an area which can be read from
 * Write:      an area which can be written to
 * ReadWrite:  here is both read an write possible
 *
 * It is permissable to use simultaneously a Read and a Write, but you can use
 * ReadWrite only as a single. 
 * For example the bitblitting code uses a Read area as source and a Write
 * area as destination. Most other routines use only a ReadWrite.
 *
 * A logical mapping area is described by some parameters (here I will for
 * example describe a Read area:
 *
 * ReadBottom     lowest accessable byte relative to the beginning of the
 *                VGA boards mapped memory.
 * 
 * ReadTop        highes accessable byte plus one.
 *
 * SegmentSize    size of such an mapped area (common for all three)
 *
 * SegmentShift   log2(SegmentSize) (used to compute the logical segment)
 *
 * SegmentMask    SegmentSize - 1 (used to mask the offset inter an area)
 *
 * 
 * All that the following routines are doing is computing for a given offset
 * into the logical VGA adress space the offset into such an logical area
 * and the logical segment number. By the way they call also the VGA board's
 * driver to set up the VGA's physical memory mapping according to the logical
 * that was requested by the calliie.
 *
 * For shake of simplicity Write and ReadWrite share the same Bottom & Top.
 * NOTE: Read & Write may have differnt starting addresses, or even common.
 *
 * There are multible routines present for the same effect. This was made
 * for effectivly interface lowlevel assembly language best.
 */

/* Definitions for BDM bank assembler routines */

#include "assyntax.h"

        FILE("bdmBank.s")
        AS_BEGIN

#ifdef __386BSD__
#define BDMBASE CONST(0xFF000000)
#else /* __386BSD__ */
#define BDMBASE CONST(0xF0000000)
#endif /* __386BSD__ */

#ifndef __ASSYNTAX_H__
#define bdmSetReadWrite _bdmSetReadWrite
#define bdmReadWriteNext _bdmReadWriteNext
#define bdmReadWritePrev _bdmReadWritePrev
#define bdmSetRead _bdmSetRead
#define bdmReadNext _bdmReadNext
#define bdmReadPrev _bdmReadPrev
#define bdmSetWrite _bdmSetWrite
#define bdmWriteNext _bdmWriteNext
#define bdmWritePrev _bdmWritePrev
#define bdmSaveBank _bdmSaveBank
#define bdmRestoreBank _bdmRestoreBank
#define bdmPushRead2Banks _bdmPushRead2Banks
#define bdmPopRead2Banks _bdmPopRead2Banks
#define bdmPushRead1Bank _bdmPushRead1Bank
#define bdmPopRead1Bank _bdmPopRead1Bank

#define bdmSegmentShift _bdmSegmentShift
#define bdmSegmentMask _bdmSegmentMask
#define bdmSegmentSize _bdmSegmentSize

#define bdmSetReadFunc _bdmSetReadFunc
#define bdmSetWriteFunc _bdmSetWriteFunc
#define bdmSetReadWriteFunc _bdmSetReadWriteFunc

#define bdmReadBottom _bdmReadBottom
#define bdmReadTop _bdmReadTop
#define bdmWriteBottom _bdmWriteBottom
#define bdmWriteTop _bdmWriteTop
#endif

        SEG_DATA
        GLOBL GLNAME(bdmwriteseg)
GLNAME(bdmwriteseg):
        D_LONG 0
GLNAME(bdmreadseg):
        D_LONG 0
GLNAME(bdmsaveseg):
        D_LONG 0
GLNAME(bdmsavereadseg):
        D_LONG 0
GLNAME(bdmsavewriteseg):
        D_LONG 0


        SEG_TEXT
/*
 *-----------------------------------------------------------------------
 * bdmSetReadWrite ---
 *     select a memory bank of the BDM board for read & write access
 *-----------------------------------------------------------------------
 *
void *bdmSetReadWrite(p)
    register void *p;
{
    bdmwriteseg = ((unsigned long)p - BDMBASE) >> bdmSegmentShift;
    (bdmSetReadWriteFunc)(bdmwriteseg);
    return (void *)(bdmWriteBottom + ((unsigned int)p & bdmSegmentMask));
}
*/
        ALIGNTEXT4
        GLOBL GLNAME(bdmSetReadWrite)
GLNAME(bdmSetReadWrite):
        MOV_L   (REGOFF(4,ESP),EAX)
        PUSH_L  (ECX)
        PUSH_L  (EDX)
        SUB_L   (BDMBASE,EAX)
        MOV_L   (CONTENT(GLNAME(bdmSegmentShift)),ECX)
        SHR_L   (CL,EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmwriteseg)))
        MOV_L   (CONTENT(GLNAME(bdmSetReadWriteFunc)),EDX)
        CALL    (CODEPTR(EDX))
        POP_L   (EDX)
        POP_L   (ECX)
        MOV_L   (REGOFF(4,ESP),EAX)
        AND_L   (CONTENT(GLNAME(bdmSegmentMask)),EAX)
        ADD_L   (CONTENT(GLNAME(bdmWriteBottom)),EAX)
        RET

/*
 *-----------------------------------------------------------------------
 * bdmReadWriteNext ---
 *     switch to next memory bank of the BDM board for read & write access
 *-----------------------------------------------------------------------
 *
void *bdmReadWriteNext(p)
    register void *p;
{
    (bdmSetReadWriteFunc)(++bdmwriteseg);
    return (void *)((unsigned long)p - bdmSegmentSize);
}
*/
        ALIGNTEXT4
        GLOBL   GLNAME(bdmReadWriteNext)
GLNAME(bdmReadWriteNext):
        PUSH_L  (EDX)
        MOV_L   (CONTENT(GLNAME(bdmwriteseg)),EAX)
        INC_L   (EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmwriteseg)))
        MOV_L   (CONTENT(GLNAME(bdmSetReadWriteFunc)),EDX)
        CALL    (CODEPTR(EDX))
        POP_L   (EDX)
        MOV_L   (REGOFF(4,ESP),EAX)
        SUB_L   (CONTENT(GLNAME(bdmSegmentSize)),EAX)
        RET

/*
 *-----------------------------------------------------------------------
 * bdmReadWritePrev ---
 *     switch to previous memory bank of the BDM board for read & write access
 *-----------------------------------------------------------------------
 *
void *bdmReadWritePrev(p)
    register void *p;
{
    (bdmSetReadWriteFunc)(--bdmwriteseg); 
    return (void *)((unsigned long)p + bdmSegmentSize);
}
*/
        ALIGNTEXT4
        GLOBL   GLNAME(bdmReadWritePrev)
GLNAME(bdmReadWritePrev):
        PUSH_L  (EDX)
        MOV_L   (CONTENT(GLNAME(bdmwriteseg)),EAX)
        DEC_L   (EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmwriteseg)))
        MOV_L   (CONTENT(GLNAME(bdmSetReadWriteFunc)),EDX)
        CALL    (CODEPTR(EDX))
        POP_L   (EDX)
        MOV_L   (REGOFF(4,ESP),EAX)
        ADD_L   (CONTENT(GLNAME(bdmSegmentSize)),EAX)
        RET

/*
 *-----------------------------------------------------------------------
 * bdmSetRead ---
 *     select a memory bank of the BDM board for read access
 *-----------------------------------------------------------------------
 *
void *bdmSetRead(p)
    register void *p;
{
    readseg = ((unsigned long)p - BDMBASE) >> bdmSegmentShift;
    (bdmSetReadFunc)(readseg);
    return (void *)(bdmReadBottom + ((unsigned int)p & bdmSegmentMask));
}
*/
        ALIGNTEXT4
        GLOBL   GLNAME(bdmSetRead)
GLNAME(bdmSetRead):
        MOV_L   (REGOFF(4,ESP),EAX)
        PUSH_L  (ECX)
        PUSH_L  (EDX)
        SUB_L   (BDMBASE,EAX)
        MOV_L   (CONTENT(GLNAME(bdmSegmentShift)),ECX)
        SHR_L   (CL,EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmreadseg)))
        MOV_L   (CONTENT(GLNAME(bdmSetReadFunc)),EDX)
        CALL    (CODEPTR(EDX))
        POP_L   (EDX)
        POP_L   (ECX)
        MOV_L   (REGOFF(4,ESP),EAX)
        AND_L   (CONTENT(GLNAME(bdmSegmentMask)),EAX)
        ADD_L   (CONTENT(GLNAME(bdmReadBottom)),EAX)
        RET

/*
 *-----------------------------------------------------------------------
 * bdmReadNext ---
 *     switch to next memory bank of the BDM board for read access
 *-----------------------------------------------------------------------
 *
void *bdmReadNext(p)
    register void *p;
{
    (bdmSetReadFunc)(++readseg);
    return (void *)((unsigned long)p - bdmSegmentSize);
}
*/
        ALIGNTEXT4
        GLOBL   GLNAME(bdmReadNext)
GLNAME(bdmReadNext):
        PUSH_L  (EDX)
        MOV_L   (CONTENT(GLNAME(bdmreadseg)),EAX)
        INC_L   (EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmreadseg)))
        MOV_L   (CONTENT(GLNAME(bdmSetReadFunc)),EDX)
        CALL    (CODEPTR(EDX))
        POP_L   (EDX)
        MOV_L   (REGOFF(4,ESP),EAX)
        SUB_L   (CONTENT(GLNAME(bdmSegmentSize)),EAX)
        RET

/*
 *-----------------------------------------------------------------------
 * bdmReadPrev ---
 *     switch to previous memory bank of the BDM board for read access
 *-----------------------------------------------------------------------
 *
void *bdmReadPrev(p)
    register void *p;
{
    (bdmSetReadFunc)(--readseg); 
    return (void *)((unsigned long)p + bdmSegmentSize);
}
*/
        ALIGNTEXT4
        GLOBL   GLNAME(bdmReadPrev)
GLNAME(bdmReadPrev):
        PUSH_L  (EDX)
        MOV_L   (CONTENT(GLNAME(bdmreadseg)),EAX)
        DEC_L   (EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmreadseg)))
        MOV_L   (CONTENT(GLNAME(bdmSetReadFunc)),EDX)
        CALL    (CODEPTR(EDX))
        POP_L   (EDX)
        MOV_L   (REGOFF(4,ESP),EAX)
        ADD_L   (CONTENT(GLNAME(bdmSegmentSize)),EAX)
        RET

/*
 *-----------------------------------------------------------------------
 * bdmSetWrite ---
 *     select a memory bank of the BDM board for write access
 *-----------------------------------------------------------------------
 *
void *bdmSetWrite(p)
    register void *p;
{
    bdmwriteseg = ((unsigned int)p - BDMBASE) >> bdmSegmentShift;
    (bdmSetWriteFunc)(bdmwriteseg);
    return (void *)(bdmWriteBottom + ((unsigned int)p & bdmSegmentMask));
}
*/
        ALIGNTEXT4
        GLOBL   GLNAME(bdmSetWrite)
GLNAME(bdmSetWrite):
        MOV_L   (REGOFF(4,ESP),EAX)
        PUSH_L  (ECX)
        PUSH_L  (EDX)
        SUB_L   (BDMBASE,EAX)
        MOV_L   (CONTENT(GLNAME(bdmSegmentShift)),ECX)
        SHR_L   (CL,EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmwriteseg)))
        MOV_L   (CONTENT(GLNAME(bdmSetWriteFunc)),EDX)
        CALL    (CODEPTR(EDX))
        POP_L   (EDX)
        POP_L   (ECX)
        MOV_L   (REGOFF(4,ESP),EAX)
        AND_L   (CONTENT(GLNAME(bdmSegmentMask)),EAX)
        ADD_L   (CONTENT(GLNAME(bdmWriteBottom)),EAX)
        RET

/*
 *-----------------------------------------------------------------------
 * bdmWriteNext ---
 *     switch to next memory bank of the BDM board for write access
 *-----------------------------------------------------------------------
 *
void *bdmWriteNext(p)
    register void *p;
{
    (bdmSetWriteFunc)(++bdmwriteseg);
    return (void *)((unsigned int)p - bdmSegmentSize);
}
*/
        ALIGNTEXT4
        GLOBL   GLNAME(bdmWriteNext)
GLNAME(bdmWriteNext):
        PUSH_L  (EDX)
        MOV_L   (CONTENT(GLNAME(bdmwriteseg)),EAX)
        INC_L   (EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmwriteseg)))
        MOV_L   (CONTENT(GLNAME(bdmSetWriteFunc)),EDX)
        CALL    (CODEPTR(EDX))
        POP_L   (EDX)
        MOV_L   (REGOFF(4,ESP),EAX)
        SUB_L   (CONTENT(GLNAME(bdmSegmentSize)),EAX)
        RET

/*
 *-----------------------------------------------------------------------
 * bdmWritePrev ---
 *     switch to previous memory bank of the BDM board for write access
 *-----------------------------------------------------------------------
 *
void *bdmWritePrev(p)
    register void *p;
{
    (bdmSetWriteFunc)(--bdmwriteseg); 
    return (void *)((unsigned int)p + bdmSegmentSize);
}
*/
        ALIGNTEXT4
        GLOBL   GLNAME(bdmWritePrev)
GLNAME(bdmWritePrev):
        PUSH_L  (EDX)
        MOV_L   (CONTENT(GLNAME(bdmwriteseg)),EAX)
        DEC_L   (EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmwriteseg)))
        MOV_L   (CONTENT(GLNAME(bdmSetWriteFunc)),EDX)
        CALL    (CODEPTR(EDX))
        POP_L   (EDX)
        MOV_L   (REGOFF(4,ESP),EAX)
        ADD_L   (CONTENT(GLNAME(bdmSegmentSize)),EAX)
        RET

/*
 *-----------------------------------------------------------------------
 * bdmSaveBank --
 *     save Banking-state
 *-----------------------------------------------------------------------
 *
void bdmSaveBank()
{
    savereadseg = readseg;
    savewriteseg = bdmwriteseg;
}
*/
        ALIGNTEXT4
        GLOBL   GLNAME(bdmSaveBank)
GLNAME(bdmSaveBank):
        MOV_L   (CONTENT(GLNAME(bdmwriteseg)),EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmsavewriteseg)))
        MOV_L   (CONTENT(GLNAME(bdmreadseg)),EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmsavereadseg)))
        RET

/*
 *-----------------------------------------------------------------------
 * bdmRestoreBank --
 *     restore the banking after bdmSaveBank was called
 *-----------------------------------------------------------------------
 *
void bdmRestoreBank()
{
    (bdmSetReadFunc)(savereadseg);
    (bdmSetWriteFunc)(bdmsavewriteseg);
}
*/
        ALIGNTEXT4
        GLOBL   GLNAME(bdmRestoreBank)
GLNAME(bdmRestoreBank):
        PUSH_L  (EDX)
        MOV_L   (CONTENT(GLNAME(bdmsavewriteseg)),EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmwriteseg)))
        MOV_L   (CONTENT(GLNAME(bdmSetWriteFunc)),EDX)
        CALL    (CODEPTR(EDX))
        MOV_L   (CONTENT(GLNAME(bdmsavereadseg)),EAX)
        MOV_L   (EAX,CONTENT(GLNAME(bdmreadseg)))
        MOV_L   (CONTENT(GLNAME(bdmSetReadFunc)),EDX)
        CALL    (CODEPTR(EDX))
        POP_L   (EDX)
        RET

/*
 *-----------------------------------------------------------------------
 * bdmPushRead ---
 *     make the write-bank also readable. no acces to the former read bank !
 *-----------------------------------------------------------------------
 *
void bdmPushRead(p)
	void *p;
{
    (bdmSetReadWriteFunc)(bdmwriteseg);
}
*/
        ALIGNTEXT4
        GLOBL   GLNAME(bdmPushRead2Banks)
GLNAME(bdmPushRead2Banks):
        RET

        ALIGNTEXT4
        GLOBL   GLNAME(bdmPushRead1Bank)
GLNAME(bdmPushRead1Bank):
        PUSH_L  (EDX)
        MOV_L   (CONTENT(GLNAME(bdmwriteseg)),EAX)
        MOV_L   (CONTENT(GLNAME(bdmSetReadWriteFunc)),EDX)
        CALL    (CODEPTR(EDX))
        POP_L   (EDX)
        RET

/*
 *-----------------------------------------------------------------------
 * bdmPopRead ---
 *     restore the banking after bdmPushRead was called
 *-----------------------------------------------------------------------
 *
void bdmPopRead(p)
	void *p;
{
    (bdmSetWriteFunc)(bdmwriteseg);
    (bdmSetReadFunc)(bdmreadseg);
}
*/
        ALIGNTEXT4
        GLOBL   GLNAME(bdmPopRead2Banks)
GLNAME(bdmPopRead2Banks):
	RET

        ALIGNTEXT4
        GLOBL   GLNAME(bdmPopRead1Bank)
GLNAME(bdmPopRead1Bank):
        PUSH_L  (EDX)
        MOV_L   (CONTENT(GLNAME(bdmwriteseg)),EAX)
        MOV_L   (CONTENT(GLNAME(bdmSetWriteFunc)),EDX)
        CALL    (CODEPTR(EDX))
        MOV_L   (CONTENT(GLNAME(bdmreadseg)),EAX)
        MOV_L   (CONTENT(GLNAME(bdmSetReadFunc)),EDX)
        CALL    (CODEPTR(EDX))
        POP_L   (EDX)
        RET
