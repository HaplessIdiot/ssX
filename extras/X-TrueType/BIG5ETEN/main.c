/* ===EmacsMode: -*- Mode: C; tab-width:4; c-basic-offset: 4; -*- === */
/* ===FileName: ===
   Copyright (c) 1998 Takuya SHIOZAKI, All Rights reserved.
   Copyright (c) 1998 X-TrueType Server Project, All rights reserved. 
   
===Notice
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
   ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
   OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
   HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
   LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
   OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
   SUCH DAMAGE.

   Major Release ID: X-TrueType Server Version 1.3 [Aoi MATSUBARA Release 3]

Notice===
 */

#include "xttversion.h"

static char const * const releaseID =
    _XTT_RELEASE_NAME;

#include "xttcommon.h"
#include "xttcap.h"
#include "xttcconv.h"
#include "xttcconvP.h"


typedef enum
{
    BIG5ETEN
} CharSetMagic;

static CharSetRelation const charSetRelations[] = {
    { "big5",  "eten", NULL, BIG5ETEN, { 0x40, 0xff, 0xa1, 0xf9, 0xa140 } },
    { NULL, NULL, NULL, 0, { 0, 0, 0, 0, 0 } }
};


CODECONV_TEMPLATE(cc_big5eten_to_ucs2);
static MapIDRelation const mapIDRelations[] = {
    { BIG5ETEN,     EPlfmISO,     EEncISO10646,
                                  cc_big5eten_to_ucs2,                 NULL },
    { BIG5ETEN,     EPlfmUnicode, EEncAny,
                                  cc_big5eten_to_ucs2,                 NULL },
    { BIG5ETEN,     EPlfmMS,      EEncMSUnicode,
                                  cc_big5eten_to_ucs2,                 NULL },
    { BIG5ETEN,     EPlfmMS,      EEncMSBig5WGL4,    NULL,             NULL },
    { -1, 0, 0, NULL, NULL }
};

STD_ENTRYFUNC_TEMPLATE(BIG5ETEN_entrypoint)

/* end of file */
