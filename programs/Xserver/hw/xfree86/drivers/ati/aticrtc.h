/* $XFree86: xc/programs/Xserver/hw/xfree86/drivers/ati/aticrtc.h,v 1.2tsi Exp $ */
/*
 * Copyright 1997 through 1999 by Marc Aurele La France (TSI @ UQV), tsi@ualberta.ca
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

#ifndef ___ATICRTC_H___
#define ___ATICRTC_H___ 1

#include "atipriv.h"
#include "atiproto.h"
#include "xf86str.h"

/*
 * CRTC related definitions.
 */
typedef enum
{
    ATI_CRTC_VGA,       /* Use VGA CRTC */
    ATI_CRTC_8514,      /* Use 8514/Mach8/Mach32 accelerator CRTC */
    ATI_CRTC_MACH64     /* Use Mach64 accelerator CRTC */
} ATICRTCType;

extern void ATICRTCPreInit   FunctionPrototype((ScrnInfoPtr, ATIPtr,
                                                ATIHWPtr));
extern void ATICRTCSave      FunctionPrototype((ScrnInfoPtr, ATIPtr,
                                                ATIHWPtr));
extern Bool ATICRTCCalculate FunctionPrototype((ScrnInfoPtr, ATIPtr,
                                                ATIHWPtr, DisplayModePtr));
extern void ATICRTCSet       FunctionPrototype((ScrnInfoPtr, ATIPtr,
                                                ATIHWPtr));

#endif /* ___ATICRTC_H___ */
