/* gmx_setup.c -- Example GLINT GMX driver stub -*- linux-c -*-
 * Created: Wed Jan 13 13:06:20 1999 by faith@precisioninsight.com
 * Revised: Fri Mar 19 14:31:27 1999 by faith@precisioninsight.com
 *
 * Copyright 1999 Precision Insight, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * PRECISION INSIGHT AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 * 
 * $PI: xc/programs/Xserver/hw/xfree86/os-support/linux/drm/gmx/gmx_setup.c,v 1.5 1999/03/26 17:47:01 faith Exp $
 * $XFree86$
 *
 */

#define EXPORT_SYMTAB
#include "gmx.h"
#include "gmx_version.h"
EXPORT_SYMBOL(gmx_init);
EXPORT_SYMBOL(gmx_cleanup);

#define GMX_DESC "Example GLINT GMX Driver"

static int gmx_reg(void);
static int gmx_unreg(void);
static int gmx_load(void);
static int gmx_unload(void);
static int gmx_open(struct inode *inode, struct file *filp);
static int gmx_release(struct inode *inode, struct file *filp);

static int                    gmx_flags      = 0;
static int                    gmx_registered = 0;
static drm_version_t          gmx_version = {
	GMX_MAJOR,
	GMX_MINOR,
	GMX_PATCHLEVEL,
	sizeof(GMX_NAME),
	(char *)GMX_NAME,
	sizeof(GMX_DATE),
	(char *)GMX_DATE,
	sizeof(GMX_DESC),
	(char *)GMX_DESC
};
static drm_func_desc_t        gmx_funcs = {
	reg:     gmx_reg,
	unreg:   gmx_unreg,
	load:    gmx_load,
	unload:  gmx_unload,
	open:    gmx_open,
	release: gmx_release,
};

#ifdef MODULE
int                           init_module(void);
void                          cleanup_module(void);
static char                   *gmx = NULL;

MODULE_AUTHOR("Precision Insight, Inc., Cedar Park, Texas.");
MODULE_DESCRIPTION("GLINT GMX Example Module");
MODULE_PARM(gmx, "s");

/* init_module is called when insmod is used to load the module */

int init_module(void)
{
	return gmx_init();
}

/* cleanup_module is called when rmmod is used to unload the module */

void cleanup_module(void)
{
	gmx_cleanup();
}
#endif

/* gmx_parse_option parses a single option.  See description for
   gmx_parse_gmx for details. */

static void gmx_parse_option(char *s)
{
	char *c, *r;
	
	GMX_TRACE("\"%s\"\n", s);
	if (!s || !*s) return;
	for (c = s; *c && *c != ':'; c++); /* find : or \0 */
	if (*c) r = c + 1; else r = NULL;  /* remember remainder */
	*c = '\0';		           /* terminate */
	if (!strcmp(s, "debug")) {
		char *h, *t, *n;

		if (!r) {
			GMX_ERROR("Usage: debug:<flag>[:<flag>]*\n");
			return;
		}

		for (h = t = n = r; h && *h; h = n) {
			for (; *t && *t != ':'; t++);     /* find : or \0 */
			if (*t) n = t + 1; else n = NULL; /* remember next */
			*t = '\0';                        /* terminate */
			if (!strcmp(h, "on")) {
				gmx_flags |= GMX_FLAG_DEBUG;
				continue;
			}
			if (!strcmp(h, "off")) {
				gmx_flags = 0;
				continue;
			}
			if (!strcmp(h, "trace")) {
				gmx_flags |= GMX_FLAG_TRACE;
				continue;
			}
			GMX_ERROR("\"%s\" is not a valid debug flag\n", h);
		}
		GMX_DEBUG("Debug mask = 0x%08x\n", gmx_flags);
		return;
	}
	GMX_ERROR("\"%s\" is not a valid option\n", s);
	return;
}

/* gmx_parse_gmx parse the insmod "gmx=" options, or the command-line
 * options passed to the kernel via LILO.  The grammar of the format is as
 * follows:
 *
 * gmx          ::= 'gmx=' option_list
 * option_list  ::= option [ ';' option_list ]
 * option       ::= 'debug:' debug_list
 * debug_list   ::= debug_option [ ':' debug_list ]
 * debug_option ::= 'on' | 'off' | 'trace'
 *
 * Note that 's' contains option_list without the 'gmx=' part.
 *
 * debug=on specifies that debugging messages will be printk'd
 * debug=trace specifies that each function call will be logged via printk
 * debug=off turns off all debugging options
 *
 */

static void gmx_parse_gmx(char *s)
{
	char *h, *t, *n;
	
	GMX_TRACE("\"%s\"\n", s);
	if (!s || !*s) return;

	for (h = t = n = s; h && *h; h = n) {
		for (; *t && *t != ';'; t++);          /* find ; or \0 */
		if (*t) n = t + 1; else n = NULL;      /* remember next */
		*t = '\0';	                       /* terminate */
		gmx_parse_option(h);                   /* parse */
	}
}

#ifndef MODULE
/* gmx_setup is called by the kernel to parse command-line options passed
 * via the boot-loader (e.g., LILO).  It calls the insmod option routine,
 * gmx_parse_gmx.
 *
 * This is not currently supported, since it requires changes to
 * linux/init/main.c. */
 

void __init gmx_setup(char *str, int *ints)
{
	GMX_TRACE("\n");
	if (ints[0] != 0) {
		GMX_ERROR("Illegal command line format, ignored\n");
		return;
	}
	gmx_parse_gmx(str);
}
#endif

/* gmx_init is called via init_module at module load time, or via
 * linux/init/main.c (this is not currently supported). */

int gmx_init(void)
{
	if (GMX_DEBUG_CODE>1) gmx_flags |= GMX_FLAG_TRACE | GMX_FLAG_DEBUG;
	GMX_TRACE("\n");

	gmx_parse_gmx(gmx);
	drm_register_driver(GMX_NAME, &gmx_version, 0, NULL, &gmx_funcs, NULL);
	
	return 0;
}

/* gmx_cleanup is called via cleanup_module at module unload time. */

void gmx_cleanup(void)
{
	GMX_TRACE("\n");

	drm_unregister_driver(GMX_NAME);
}

static int gmx_reg(void)
{
	GMX_TRACE("\n");
	if (gmx_registered) return -EBUSY;
	++gmx_registered;
	return 0;
}

static int gmx_unreg(void)
{
	GMX_TRACE("\n");
	if (!gmx_registered) return -EBUSY;
	--gmx_registered;
	return 0;
}

static int gmx_load(void)
{
	GMX_TRACE("\n");
	return 0;
}

static int gmx_unload(void)
{
	GMX_TRACE("\n");
	return 0;
}

static int gmx_open(struct inode *inode, struct file *filp)
{
	GMX_TRACE("\n");
	MOD_INC_USE_COUNT;
	return 0;
}

static int gmx_release(struct inode *inode, struct file *filp)
{
	GMX_TRACE("\n");
	MOD_DEC_USE_COUNT;
	return 0;
}

