/* ===EmacsMode: -*- Mode: C; tab-width:4; c-basic-offset: 4; -*- === */
/* ===FileName: ===
   Copyright (c) 1997 Jyunji Takagi, All rights reserved.
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

   Major Release ID: X-TrueType Server Version 1.2 [Aoi MATSUBARA Release 2]

Notice===
*/

/*
 * B-Tree
 */

#ifndef _XTTCACHE_H_
#define _XTTCACHE_H_

#define BTreeHalfSize 4

typedef struct _NodeRec {
    int key;
    void *value;
} NodeRec, *NodePtr;

typedef struct _PageRec {
    int n;
    NodeRec node[BTreeHalfSize * 2];
    struct _PageRec *branch[BTreeHalfSize * 2 + 1];
} PageRec, *PagePtr;

typedef struct _BTree {
/* private */
    NodeRec node;
    PagePtr page;
    int del, under;
/* public */
    PagePtr root;
} BTreeRec, *BTreePtr;

extern BTreePtr BTree_Alloc(void);
extern int BTree_Search(BTreePtr this, int key, void **value);
extern void BTree_Insert(BTreePtr this, int key, void *value);
extern void BTree_Free(BTreePtr this);
extern void BTree_Print(BTreePtr this);

#endif /* _BTREE_H_ */

/*
 * End Of File
 */
