/* $XFree86$ */
/*
 * Copyright 2000 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of Marc Aurele La France not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  Marc Aurele La France makes no representations
 * about the suitability of this software for any purpose.  It is provided
 * "as-is" without express or implied warranty.
 *
 * MARC AURELE LA FRANCE DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO
 * EVENT SHALL MARC AURELE LA FRANCE BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef ___ATIMACH64IO_H___
#define ___ATIMACH64IO_H___ 1

#include "atiproto.h"
#include "atistruct.h"

#define inm(_Register)                                        \
    MMIO_IN32(pATI->pBlock[GetBits(_Register, BLOCK_SELECT)], \
              (_Register) & MM_IO_SELECT)

extern void ATIMach64PollEngineStatus FunctionPrototype((ATIPtr));

/*
 * MMIO cache definitions
 */
#define CacheByte(___Register) pATI->MMIOCached[CacheSlotOf(___Register >> 3)]
#define CacheBit(___Register)  (0x80U >> (CacheSlotOf(___Register) & 0x07U))

#define RegisterIsCached(__Register) \
    (CacheByte(__Register) & CacheBit(__Register))
#define CacheSlot(__Register) pATI->MMIOCache[CacheSlotOf(__Register)]

#define CacheRegister(__Register) \
    CacheByte(__Register) |= CacheBit(__Register)
#define UncacheRegister(__Register) \
    CacheByte(__Register) &= ~CacheBit(__Register)

/* This would be quite a bit slower as a function */
#define outm(_Register, _Value)                                        \
    do                                                                 \
    {                                                                  \
        CARD32 _IOValue = (_Value);                                    \
                                                                       \
        if (!RegisterIsCached(_Register) ||                            \
            (_IOValue != CacheSlot(_Register)))                        \
        {                                                              \
            while (!pATI->nAvailableFIFOEntries--)                     \
                ATIMach64PollEngineStatus(pATI);                       \
            MMIO_OUT32(pATI->pBlock[GetBits(_Register, BLOCK_SELECT)], \
                       (_Register) & MM_IO_SELECT, _IOValue);          \
            CacheSlot(_Register) = _IOValue;                           \
            pATI->EngineIsBusy = TRUE;                                 \
        }                                                              \
    } while (0)

/*
 * This is no longer as critical, especially for _n == 1.  However,
 * there is still a need to ensure _n <= pATI-<nFIFOEntries.
 */
#define ATIMach64WaitForFIFO(_pATI, _n)        \
    while (pATI->nAvailableFIFOEntries < (_n)) \
        ATIMach64PollEngineStatus(pATI);

#define ATIMach64WaitForIdle(_pATI)      \
    while (pATI->EngineIsBusy)           \
        ATIMach64PollEngineStatus(pATI);

#endif /* ___ATIMACH64IO_H___ */
