/*
 * Acceleration for the Creator and Creator3D framebuffer - register caching.
 *
 * Copyright (C) 1999 David S. Miller (davem@redhat.com)
 * Copyright (C) 1999 Jakub Jelinek (jakub@redhat.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * JAKUB JELINEK OR DAVID MILLER BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */
/* $XFree86$ */

#ifndef FFBRCACHE_H
#define FFBRCACHE_H

/* We only need to write the bits which actually change,
 * writing unnessary bits causes the operation to go more
 * slowly. -DaveM
 *
 * We used to have some hairy code here which tried to
 * avoid writing attribute bits unnecessarily.  It has been
 * removed because various two bit fields have different
 * semantics.  For example, for some the higher bit is the
 * edit bit, for others there are three state patterns
 * and zero is a special "no edit" value.  Ho hum, it was
 * a nice idea...
 */
#define FFB_WRITE_PPC(__fpriv, __ffb, __val, __chg_mask) \
do {	unsigned int oldval = (__fpriv)->ppc_cache; \
	unsigned int __t; \
	__t = (oldval & (__chg_mask)) ^ (__val); \
	if (__t) { \
		unsigned int newval = oldval & ~(__chg_mask); \
		newval |= (__val); \
		(__fpriv)->ppc_cache = newval; \
		FFBFifo((__fpriv), 1); \
		(__ffb)->ppc = (__val); \
	} \
} while(0)

#define FFB_WRITE_PMASK(__fpriv, __ffb, __val) \
do {	if((__fpriv)->pmask_cache != (__val)) { \
		(__fpriv)->pmask_cache = (__val); \
		FFBFifo((__fpriv), 1); \
		(__ffb)->pmask = (__val); \
	} \
} while(0)

#define FFB_WRITE_ROP(__fpriv, __ffb, __val) \
do {	if((__fpriv)->rop_cache != (__val)) { \
		(__fpriv)->rop_cache = (__val); \
		FFBFifo((__fpriv), 1); \
		(__ffb)->rop = (__val); \
	} \
} while(0)

#define FFB_WRITE_DRAWOP(__fpriv, __ffb, __val) \
do {	if((__fpriv)->drawop_cache != (__val)) { \
		(__fpriv)->drawop_cache = (__val); \
		FFBFifo((__fpriv), 1); \
		(__ffb)->drawop = (__val); \
	} \
} while(0)

#define FFB_WRITE_FG(__fpriv, __ffb, __val) \
do {	if((__fpriv)->fg_cache != (__val)) { \
		(__fpriv)->fg_cache = (__val); \
		FFBFifo((__fpriv), 1); \
		(__ffb)->fg = (__val); \
	} \
} while(0)

#define FFB_WRITE_BG(__fpriv, __ffb, __val) \
do {	if((__fpriv)->bg_cache != (__val)) { \
		(__fpriv)->bg_cache = (__val); \
		FFBFifo((__fpriv), 1); \
		(__ffb)->bg = (__val); \
	} \
} while(0)

#define FFB_WRITE_FONTW(__fpriv, __ffb, __val) \
do {	if((__fpriv)->fontw_cache != (__val)) { \
		(__fpriv)->fontw_cache = (__val); \
		FFBFifo((__fpriv), 1); \
		(__ffb)->fontw = (__val); \
	} \
} while(0)

#define FFB_WRITE_FONTINC(__fpriv, __ffb, __val) \
do {	if((__fpriv)->fontinc_cache != (__val)) { \
		(__fpriv)->fontinc_cache = (__val); \
		FFBFifo((__fpriv), 1); \
		(__ffb)->fontinc = (__val); \
	} \
} while(0)

#define FFB_WRITE_FBC(__fpriv, __ffb, __val) \
do {	if((__fpriv)->fbc_cache != (__val)) { \
		(__fpriv)->fbc_cache = (__val); \
		FFBFifo((__fpriv), 1); \
		(__ffb)->fbc = (__val); \
	} \
} while(0)

extern void __FFB_Write_Attributes(FFBPtr pFfb,
				   unsigned int ppc,
				   unsigned int ppc_mask,
				   unsigned int pmask,
				   unsigned int rop,
				   int drawop, int fg,
				   unsigned int fbc);

#define FFB_WRITE_ATTRIBUTES(__fpriv, __ppc, __ppc_mask, __pmask, __rop, __drawop, __fg, __fbc) \
	if((((__fpriv)->ppc_cache & (__ppc_mask)) != (__ppc))			|| \
	   ((__fpriv)->pmask_cache != (__pmask))				|| \
	   ((__fpriv)->rop_cache != (__rop))					|| \
	   (((__drawop) != -1) && ((__fpriv)->drawop_cache != (__drawop)))	|| \
	   ((__fpriv)->fg_cache != (__fg))					|| \
	   ((__fpriv)->fbc_cache != (__fbc))) \
		__FFB_Write_Attributes((__fpriv), (__ppc), (__ppc_mask), (__pmask), \
				       (__rop), (__drawop), (__fg), (__fbc))

/* Setup to access the smart frame buffer (SFB) directly where the
 * pixel color comes from the cpu on writes.
 */
#define FFB_WRITE_ATTRIBUTES_SFB_VAR(__fpriv, __pmask, __alu) \
do {	unsigned int ppc = FFB_PPC_FW_DISABLE | FFB_PPC_VCE_DISABLE | FFB_PPC_APE_DISABLE | FFB_PPC_CS_VAR; \
	unsigned int ppc_mask = FFB_PPC_FW_MASK | FFB_PPC_VCE_MASK | FFB_PPC_APE_MASK | FFB_PPC_CS_MASK; \
	unsigned int rop = FFB_ROP_EDIT_BIT | (__alu); \
	if(((__fpriv)->ppc_cache & ppc_mask) != ppc || \
	   (__fpriv)->fbc_cache != FFB_FBC_DEFAULT || \
	   (__fpriv)->rop_cache != rop || \
	   (__fpriv)->pmask_cache != (__pmask)) { \
		ffb_fbcPtr __ffb = (__fpriv)->regs; \
		(__fpriv)->ppc_cache &= ~ppc_mask; \
		(__fpriv)->ppc_cache |= ppc; \
		(__fpriv)->fbc_cache = FFB_FBC_DEFAULT; \
		(__fpriv)->rop_cache = rop; \
		(__fpriv)->pmask_cache = (__pmask); \
		(__fpriv)->rp_active = 1; \
		FFBFifo(__fpriv, 4); \
		(__ffb)->ppc = ppc; \
		(__ffb)->fbc = FFB_FBC_DEFAULT; \
		(__ffb)->rop = rop; \
		(__ffb)->pmask = (__pmask); \
	} \
} while(0)

/* VSCROLL Attributes:
 *
 * PPC) VCE_DISABLE must be set, all other attributes are effectively
 *      ignored since this drawop just copies pixels within the ram
 *      chips and bypasses the pixel processor entirely
 * FBC) all options are allowed, but the SRC/DST buffers are determined
 *      solely by the WB_* setting, that is, the RB_* setting is effectively
 *      ignored since the pixels are copied directly through the write buffer
 * ROP) must be FFB_ROP_OLD
 * PMASK) all options allowed
 */
#define FFB_WRITE_ATTRIBUTES_VSCROLL(__fpriv, __pmask) \
do {	unsigned int ppc = FFB_PPC_FW_DISABLE | FFB_PPC_VCE_DISABLE; \
	unsigned int ppc_mask = FFB_PPC_FW_MASK | FFB_PPC_VCE_MASK; \
	unsigned int rop = FFB_ROP_OLD; \
	if(((__fpriv)->ppc_cache & ppc_mask) != ppc || \
	   (__fpriv)->fbc_cache != FFB_FBC_DEFAULT || \
	   (__fpriv)->rop_cache != rop || \
	   (__fpriv)->pmask_cache != (__pmask) || \
	   (__fpriv)->drawop_cache != FFB_DRAWOP_VSCROLL) { \
		ffb_fbcPtr __ffb = (__fpriv)->regs; \
		(__fpriv)->ppc_cache &= ~ppc_mask; \
		(__fpriv)->ppc_cache |= ppc; \
		(__fpriv)->fbc_cache = FFB_FBC_DEFAULT; \
		(__fpriv)->rop_cache = rop; \
		(__fpriv)->pmask_cache = (__pmask); \
		(__fpriv)->drawop_cache = FFB_DRAWOP_VSCROLL; \
		(__fpriv)->rp_active = 1; \
		FFBFifo(__fpriv, 5); \
		(__ffb)->ppc = ppc; \
		(__ffb)->fbc = FFB_FBC_DEFAULT; \
		(__ffb)->rop = rop; \
		(__ffb)->pmask = (__pmask); \
		(__ffb)->drawop = FFB_DRAWOP_VSCROLL; \
	} \
} while(0)

#define FFB_FBC_DEFAULT				\
(FFB_FBC_WB_A|FFB_FBC_WM_COMBINED|		\
 FFB_FBC_RB_A|FFB_FBC_SB_BOTH|			\
 FFB_FBC_ZE_OFF|FFB_FBC_YE_OFF|FFB_FBC_XE_OFF|	\
 FFB_FBC_RGBE_MASK)

#define FFB_FBC_FASTFILL			\
(FFB_FBC_WB_A|FFB_FBC_WM_COMBINED|		\
 FFB_FBC_RB_A|FFB_FBC_SB_BOTH|			\
 FFB_FBC_ZE_OFF|FFB_FBC_YE_OFF|FFB_FBC_XE_ON|	\
 FFB_FBC_RGBE_ON)

#endif /* FFBRCACHE_H */
