
/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/tseng/tseng_bank.c,v 1.1.2.3 1998/07/24 11:36:31 dawes Exp $ */





#include "tseng.h"

/*
 * We really need to cache the bank registers, because IO reads are too
 * expensive
 */

static unsigned char SegSelL = 0, SegSelH = 0;

int
ET4000W32SetRead(ScreenPtr pScrn, unsigned int iBank)
{
    SegSelL = (SegSelL & 0x0f) | (iBank << 4);
    SegSelH = (SegSelH & 0x03) | (iBank & 0x30);
    outb(0x3CB, SegSelH);
    outb(0x3CD, SegSelL);
    return 0;
}

int
ET4000W32SetWrite(ScreenPtr pScrn, unsigned int iBank)
{
    SegSelL = (SegSelL & 0xf0) | (iBank & 0x0f);
    SegSelH = (SegSelH & 0x30) | (iBank >> 4);
    outb(0x3CB, SegSelH);
    outb(0x3CD, SegSelL);
    return 0;
}

int
ET4000W32SetReadWrite(ScreenPtr pScrn, unsigned int iBank)
{
    SegSelL = (iBank & 0x0f) | (iBank << 4);
    SegSelH = (iBank & 0x30) | (iBank >> 4);
    outb(0x3CB, SegSelH);
    outb(0x3CD, SegSelL);
    return 0;
}

int
ET4000SetRead(ScreenPtr pScrn, unsigned int iBank)
{
    SegSelL = (SegSelL & 0x0f) | (iBank << 4);
    outb(0x3CD, SegSelL);
    return 0;
}

int
ET4000SetWrite(ScreenPtr pScrn, unsigned int iBank)
{
    SegSelL = (SegSelL & 0xf0) | (iBank & 0x0f);
    outb(0x3CD, SegSelL);
    return 0;
}

int
ET4000SetReadWrite(ScreenPtr pScrn, unsigned int iBank)
{
    SegSelL = (iBank & 0x0f) | (iBank << 4);
    outb(0x3CD, SegSelL);
    return 0;
}
