/*
 * Copyright 1993,1994 by David Wexelblat <dwex@goblin.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of David Wexelblat not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  David Wexelblat makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as is" without express or implied warranty.
 *
 * DAVID WEXELBLAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL DAVID WEXELBLAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* $XFree86: mit/server/ddx/x386/SuperProbe/MemProbe.c,v 2.1 1994/02/28 14:09:34 dawes Exp $ */

#include "Probe.h"

static Word Ports[] = {SEQ_IDX, SEQ_REG, GRC_IDX, GRC_REG};
#define NUMPORTS (sizeof(Ports)/sizeof(Ports[0]))

#define SAVESIZE	100
#define BYTE_AT(s,i,j)	(*((s)+((i)*SAVESIZE)+(j)))
#define TAG		"SuperProbe"

int Probe_Mem(Check)
MemCheck_Descriptor Check;
{
	Byte Save256;
	Byte SaveMap;
	Byte SaveSeq;
	Byte SaveMask;
	Byte *Mem;
	Byte *Store;
	int BankNum;
	register int i, j;
	Bool IsMem = TRUE;
	int SaveRBank;
	int SaveWBank;
	int Total;

	Store = (Byte *)malloc((Check.max_bank+1)*SAVESIZE);
	if (Store == (Byte *)0)
	{
		return(0);
	}
	Mem = MapVGA();
	if (Mem == (Byte *)0)
	{
		return(0);
	}
	EnableIOPorts(NUMPORTS, Ports);
	Save256 = rdinx(GRC_IDX, 0x05);
	SaveMap = rdinx(GRC_IDX, 0x06);
	SaveMask = rdinx(SEQ_IDX, 0x02);
	SaveSeq = rdinx(SEQ_IDX, 0x04);
	wrinx(GRC_IDX, 0x05, ((Save256 & 0x0F) | 0x40));
	wrinx(GRC_IDX, 0x06, (SaveMap & 0xF0) | 0x05);
	wrinx(SEQ_IDX, 0x02, (SaveMask & 0xF0) | 0x0F);
	wrinx(SEQ_IDX, 0x04, (SaveSeq | 0x0E));
	Check.get_bank(Check.chipset, &SaveRBank, &SaveWBank);
	for (BankNum = 0; ((BankNum <= Check.max_bank) && (IsMem)); BankNum++)
	{
		Check.set_bank(Check.chipset, BankNum, BankNum);
		/*
		 * See if this bank is tagged.
		 */
		if (strncmp((char *)Mem, TAG, strlen(TAG)) == 0)
		{
			break;
		}
		for (i=0; i < SAVESIZE; i++)
		{
			BYTE_AT(Store,BankNum,i) = Mem[i];
			Mem[i] ^= 0xAA;
		}
		IsMem = TRUE;
		for (i=0; i < SAVESIZE; i++)
		{
			if (Mem[i] != (BYTE_AT(Store,BankNum,i) ^ 0xAA))
			{
				IsMem = FALSE;
				break;
			}
		}
		for (i = 0; i < SAVESIZE; i++)
		{
			Mem[i] = BYTE_AT(Store,BankNum,i);
		}
		if (IsMem)
		{
			for (i=0; i < SAVESIZE; i++)
			{
				Mem[i] ^= 0x55;
			}
			IsMem = TRUE;
			for (i=0; i < SAVESIZE; i++)
			{
				if (Mem[i] != (BYTE_AT(Store,BankNum,i) ^ 0x55))
				{
					IsMem = FALSE;
					break;
				}
			}
			for (i = 0; i < SAVESIZE; i++)
			{
				Mem[i] = BYTE_AT(Store,BankNum,i);
			}
		}
		if (IsMem)
		{
			/*
			 * Tag this bank
			 */
			strcpy((char *)Mem, TAG);
		}
	}
	Total = (BankNum * Check.bank_size) / 1024; 

	for (j=0; j < BankNum; j++)
	{
		Check.set_bank(Check.chipset, j, j);
		for (i=0; i < SAVESIZE; i++)
		{
			Mem[i] = BYTE_AT(Store,j,i);
		}
	}

	Check.set_bank(Check.chipset, SaveRBank, SaveWBank);
	wrinx(SEQ_IDX, 0x04, SaveSeq);
	wrinx(SEQ_IDX, 0x02, SaveMask);
	wrinx(GRC_IDX, 0x06, SaveMap);
	wrinx(GRC_IDX, 0x05, Save256);
	DisableIOPorts(NUMPORTS, Ports);
	UnMapVGA(Mem);

	return(Total);
}
