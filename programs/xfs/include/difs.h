/* $XFree86$ */
/*
 * Copyright (C) 1999 The XFree86 Project, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * XFREE86 PROJECT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of the XFree86 Project shall
 * not be used in advertising or otherwise to promote the sale, use or other
 * dealings in this Software without prior written authorization from the
 * XFree86 Project.
 */
#ifndef _DIFS_H
#define _DIFS_H

#include "difsfn.h"
#include "globals.h"

typedef int (*InitialFunc)(ClientPtr);
typedef int (*ProcFunc)(ClientPtr);
typedef int (*SwappedProcFunc)(ClientPtr);
typedef void (*EventSwapFunc)(fsError *, fsError *);
typedef void (*ReplySwapFunc)(ClientPtr, int, void *);

extern InitialFunc InitialVector[3];
extern ReplySwapFunc ReplySwapVector[NUM_PROC_VECTORS];

/* FIXME: this is derived from fontstruct.h; should integrate it */
typedef int (*InitFpeFunc) (FontPathElementPtr fpe);
typedef int (*FreeFpeFunc) (FontPathElementPtr fpe);
typedef int (*ResetFpeFunc) (FontPathElementPtr fpe);
typedef int (*OpenFontFunc) (
		pointer client,
		FontPathElementPtr fpe,
		int flags,
		char* name,
		int namelen,
		fsBitmapFormat format,
		fsBitmapFormatMask fmask,
		unsigned long id,
		FontPtr* pFont,
		char** aliasName,
		FontPtr non_cachable_font);
typedef int (*CloseFontFunc) (FontPathElementPtr fpe, FontPtr pFont);
typedef int (*ListFontsFunc) (
		pointer client,
		FontPathElementPtr fpe,
		char* pat,
		int len,
		int max,
		FontNamesPtr names);

typedef int (*StartLfwiFunc) (
		pointer client,
		FontPathElementPtr fpe,
		char* pat,
		int len,
		int max,
		pointer* privatep);

typedef int (*NextLfwiFunc) (
		pointer client,
		FontPathElementPtr fpe,
		char** name,
		int* namelen,
		FontInfoPtr* info,
		int* numFonts,
		pointer private);

typedef int (*WakeupFpeFunc) (
		FontPathElementPtr fpe,
		unsigned long* LastSelectMask);

typedef int (*ClientDiedFunc) (
		pointer client,
		FontPathElementPtr fpe);

typedef int (*LoadGlyphsFunc) (
		pointer client,
		FontPtr pfont,
		Bool range_flag,
		unsigned int nchars,
		int item_size,
		unsigned char* data);

typedef int (*StartLaFunc) (
		pointer client,
		FontPathElementPtr fpe,
		char* pat,
		int len,
		int max,
		pointer* privatep);

typedef int (*NextLaFunc) (
		pointer client,
		FontPathElementPtr fpe,
		char** namep,
		int* namelenp,
		char** resolvedp,
		int* resolvedlenp,
		pointer private);

/* difs/atom.c */
extern Atom MakeAtom ( char *string, unsigned len, Bool makeit );
extern int ValidAtom ( Atom atom );
extern char * NameForAtom ( Atom atom );
extern void InitAtoms ( void );

/* difs/charinfo.c */
extern int GetExtents ( ClientPtr client, FontPtr pfont, Mask flags, unsigned long num_ranges, fsRange *range, unsigned long *num_extents, fsXCharInfo **data );
extern int GetBitmaps ( ClientPtr client, FontPtr pfont, fsBitmapFormat format, Mask flags, unsigned long num_ranges, fsRange *range, int *size, unsigned long *num_glyphs, fsOffset32 **offsets, pointer *data, int *freeData );

/* difs/initfonts.c */
extern void InitFonts ( void );

/* difs/fonts.c */
extern int FontToFSError ( int err );
extern void UseFPE ( FontPathElementPtr fpe );
extern void FreeFPE ( FontPathElementPtr fpe );
extern void QueueFontWakeup ( FontPathElementPtr fpe );
extern void RemoveFontWakeup ( FontPathElementPtr fpe );
extern void FontWakeup ( pointer data, int count, unsigned long *LastSelectMask );
extern int OpenFont ( ClientPtr client, Font fid, fsBitmapFormat format, fsBitmapFormatMask format_mask, int namelen, char *name );
extern int CloseClientFont ( ClientFontPtr cfp, FSID fid );
extern int SetFontCatalogue ( char *str, int *badpath );
extern int ListFonts ( ClientPtr client, int length, unsigned char *pattern, int maxNames );
#if 0
extern int StartListFontsWithInfo ( ClientPtr client, int length, unsigned char *pattern, int maxNames );
#endif
extern int LoadGlyphRanges ( ClientPtr client, FontPtr pfont, Bool range_flag, int num_ranges, int item_size, fsChar2b *data );
extern int RegisterFPEFunctions ( Bool          (*name_func) (char *name),
				  InitFpeFunc   init_func,
				  FreeFpeFunc   free_func,
				  ResetFpeFunc  reset_func,
				  OpenFontFunc  open_func,
				  CloseFontFunc close_func,
				  ListFontsFunc list_func,
				  StartLfwiFunc start_lfwi_func,
				  NextLfwiFunc  next_lfwi_func,
				  WakeupFpeFunc wakeup_func,
				  ClientDiedFunc client_died,
				  LoadGlyphsFunc load_glyphs,
				  StartLaFunc   start_list_alias_func,
				  NextLaFunc    next_list_alias_func,
				  void	  (*set_path_func) (void));
extern void FreeFonts ( void );
extern FontPtr find_old_font ( FSID id );
extern Font GetNewFontClientID ( void );
extern int StoreFontClientFont ( FontPtr pfont, Font id );
extern void DeleteFontClientID ( Font id );
extern int init_fs_handlers ( FontPathElementPtr fpe, DifsBlockFunc block_handler );
extern void remove_fs_handlers ( FontPathElementPtr fpe, DifsBlockFunc block_handler, Bool all );
extern void DeleteClientFontStuff ( ClientPtr client );

/* difs/fontinfo.c */
extern void CopyCharInfo ( CharInfoPtr ci, fsXCharInfo *dst );
extern int convert_props ( FontInfoPtr pinfo, fsPropInfo **props );
#if 0
extern int QueryExtents ( ClientPtr client, ClientFontPtr cfp, int item_size, int nranges, Bool range_flag, pointer range_data );
extern int QueryBitmaps ( ClientPtr client, ClientFontPtr cfp, int item_size, fsBitmapFormat format, int nranges, Bool range_flag, pointer range_data );
#endif

/* difs/main.c */
extern void NotImplemented(void);

#endif
