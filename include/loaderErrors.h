/*
 *
 * Copyright 1995-1997 by Metro Link, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of Metro Link, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software without
 * specific, written prior permission.  Metro Link, Inc. makes no
 * representations about the suitability of this software for any purpose.
 *  It is provided "as is" without express or implied warranty.
 *
 * METRO LINK, INC. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL METRO LINK, INC. BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */


/*
 * Error Return Codes from LoadModule and LoadDriver for errmaj
 */
#define LDR_NOMEM 1 /* memory allocation failed */
#define LDR_NOENT 2 /* Module file does not exist */
#define LDR_NOSUBENT 3 /* pre-requsite file to be sub-loaded does not exist */
#define LDR_NOSPACE 4 /* internal module array full */
#define LDR_NOMODOPEN 5 /* module file could not be opened (check errmin) */
#define LDR_UNKTYPE 6 /* file is not a recognized module type */
#define LDR_NOLOAD 7 /* type specific loader failed */

/*
 * Additional Error Return Codes from module SetupProc for errmaj
 */
#define LDR_NOPORTOPEN 8 /* could not open port (check errmin) */
#define LDR_NOHARDWARE 9 /* could not query/initialize the hardware device */

/*
 * system errno's can be mapped to OS neutral xf86 errno's by xf86errno(n)
 * They start at 1000 just so they don't match real errnos at all
 */
#define xf86_UNKNOWN	1000
#define xf86_EACCES	1001
#define xf86_EAGAIN	1002
#define xf86_EBADF	1003
#define xf86_EEXIST	1004
#define xf86_EFAULT	1005
#define xf86_EINTR	1006
#define xf86_EINVAL	1007
#define xf86_EISDIR	1008
#define xf86_ELOOP	1009
#define xf86_EMFILE	1010
#define xf86_ENAMETOOLONG	1011
#define xf86_ENFILE	1012
#define xf86_ENOENT	1013
#define xf86_ENOMEM	1014
#define xf86_ENOSPC	1015
#define xf86_ENOTDIR	1016
#define xf86_EPIPE	1017
#define xf86_EROFS	1018
#define xf86_ETXTBSY	1019

