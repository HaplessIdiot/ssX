/* $XConsortium: t1intf.h,v 1.2 91/10/10 11:19:39 rws Exp $ */
/* Copyright International Business Machines,Corp. 1991
 * All Rights Reserved
 *
 * License to use, copy, modify, and distribute this software
 * and its documentation for any purpose and without fee is
 * hereby granted, provided that the above copyright notice
 * appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation,
 * and that the name of IBM not be used in advertising or
 * publicity pertaining to distribution of the software without
 * specific, written prior permission.
 *
 * IBM PROVIDES THIS SOFTWARE "AS IS", WITHOUT ANY WARRANTIES
 * OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING, BUT NOT
 * LIMITED TO ANY IMPLIED WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS.  THE ENTIRE RISK AS TO THE QUALITY AND
 * PERFORMANCE OF THE SOFTWARE, INCLUDING ANY DUTY TO SUPPORT
 * OR MAINTAIN, BELONGS TO THE LICENSEE.  SHOULD ANY PORTION OF
 * THE SOFTWARE PROVE DEFECTIVE, THE LICENSEE (NOT IBM) ASSUMES
 * THE ENTIRE COST OF ALL SERVICING, REPAIR AND CORRECTION.  IN
 * NO EVENT SHALL IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */
/* Copyright (c) 1994-1999 Silicon Graphics, Inc. All Rights Reserved.
 *
 * The contents of this file are subject to the CID Font Code Public Licence
 * Version 1.0 (the "License"). You may not use this file except in compliance
 * with the Licence. You may obtain a copy of the License at Silicon Graphics,
 * Inc., attn: Legal Services, 2011 N. Shoreline Blvd., Mountain View, CA
 * 94043 or at http://www.sgi.com/software/opensource/cid/license.html.
 *
 * Software distributed under the License is distributed on an "AS IS" basis.
 * ALL WARRANTIES ARE DISCLAIMED, INCLUDING, WITHOUT LIMITATION, ANY IMPLIED
 * WARRANTIES OF MERCHANTABILITY, OF FITNESS FOR A PARTICULAR PURPOSE OR OF
 * NON-INFRINGEMENT. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Software is CID font code that was developed by Silicon
 * Graphics, Inc.
 */
/* $XFree86: xc/lib/font/Type1/t1intf.h,v 1.3 1999/05/03 05:58:47 dawes Exp $ */

#ifdef BUILDCID
#include "AFM.h"
#endif
 
struct type1font {
       CharInfoPtr  pDefault;
       CharInfoRec  glyphs[256];
};

#ifdef BUILDCID
typedef struct cid_glyphs {
       char           *CIDFontName;
       char           *CMapName;
       long            dataoffset;
       double          pixel_matrix[4];
       CharInfoPtr     pDefault;
       CharInfoRec   **glyphs;
       FontInfo       *AFMinfo;
#ifdef USE_MMAP
       unsigned char  *CIDdata;
       long            CIDsize;
#endif
} cidglyphs;
#endif
