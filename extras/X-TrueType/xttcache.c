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

#include "xttversion.h"

static char const * const releaseID =
    _XTT_RELEASE_NAME;

/*
 * B-Tree
 */

#include "xttcommon.h"
#include "fontmisc.h"
#include "xttcache.h"

BTreePtr BTree_Alloc()
{
    BTreePtr this = (BTreePtr) xalloc(sizeof(BTreeRec));
    if (this != NULL) {
        memset(this, 0x00, sizeof(BTreeRec));
    }
    return this;
}

int BTree_Search(BTreePtr this, int key, void **value)
{
    PagePtr ptr;
    int i;
    NodePtr np;

    for (ptr = this->root; ptr; ) {
        i = 0;
    np = ptr->node;
        while ((i < ptr->n) && np->key < key) {
        i++;
        np++;
    }
    if ((i < ptr->n) && (np->key == key)) {
        *value = np->value;
        return 1;
    }
    ptr = ptr->branch[i];
    }
    *value = NULL;
    return 0;
}

static void BTree_InsertItem(BTreePtr this, PagePtr ptr, int index)
{
    int i;

    for (i = ptr->n; i > index; i--) {
        ptr->node[i] = ptr->node[i - 1];
    ptr->branch[i + 1] = ptr->branch[i];
    }
    ptr->node[index] = this->node;
    ptr->branch[index + 1] = this->page;
    ptr->n++;
}

static void BTree_Split(BTreePtr this, PagePtr ptr, int index)
{
    PagePtr p;
    int i, m;

    p = (PagePtr) xalloc(sizeof(PageRec));
    m = (index <= BTreeHalfSize) ? BTreeHalfSize : BTreeHalfSize + 1;
    for (i = m + 1; i <= BTreeHalfSize * 2; i++) {
        p->node[i - m - 1] = ptr->node[i - 1];
    p->branch[i - m] = ptr->branch[i];
    }
    ptr->n = m;
    p->n = BTreeHalfSize * 2 - m;
    if (index <= BTreeHalfSize) {
        BTree_InsertItem(this, ptr, index);
    } else {
        BTree_InsertItem(this, p, index - m);
    }
    this->node = ptr->node[ptr->n - 1];
    p->branch[0] = ptr->branch[ptr->n];
    ptr->n--;
    this->page = p;
}

static int BTree_InsertSub(BTreePtr this, PagePtr ptr)
{
    int i;
    NodePtr np;

    if (ptr == NULL) {
    this->page = NULL;
    return 0;
    }
    i = 0;
    np = ptr->node;
    while ((i < ptr->n) && (np->key < this->node.key)) {
        i++;
    np++;
    }
    if ((i < ptr->n) && np->key == this->node.key) {
        /* already registered */
    return 1;
    }
    if (BTree_InsertSub(this, ptr->branch[i])) {
      return 1;
    }
    if (ptr->n < BTreeHalfSize * 2) {
        BTree_InsertItem(this, ptr, i);
    return 1;
    } else {
        BTree_Split(this, ptr, i);
    return 0;
    }
}

void BTree_Insert(BTreePtr this, int key, void *value)
{
    PagePtr ptr;

    this->node.key = key;
    this->node.value = value;
    if (BTree_InsertSub(this, this->root)) {
        return;
    }
    ptr = (PagePtr) xalloc(sizeof(PageRec));
    ptr->n = 1;
    ptr->branch[0] = this->root;
    ptr->node[0] = this->node;
    ptr->branch[1] = this->page;
    this->root = ptr;
}

#if 0 /* delete functions are not tested. */
static void BTree_RemoveItem(BTreePtr this, PagePtr ptr, int index)
{
    while (++index < ptr->n) {
        ptr->node[index - 1] = ptr->node[index];
    ptr->branch[index] = ptr->branch[index + 1];
    }
    this->under = --(ptr->n) < BTreeHalfSize;
}

static void BTree_MoveRight(BTreePtr this, PagePtr ptr, int index)
{
    PagePtr left, right;
    int i;

    left = ptr->branch[index - 1];
    right = ptr->branch[index];
    for (i = right->n; i > 0; i--) {
        right->node[i] = right->node[i - 1];
    right->branch[i + 1] = right->branch[i];
    }
    right->branch[1] = right->branch[0];
    right->n++;
    right->node[0] = ptr->node[index - 1];
    ptr->node[index - 1] = left->node[left->n - 1];
    right->branch[0] = left->branch[left->n];
    left->n--;
}

static void BTree_MoveLeft(BTreePtr this, PagePtr ptr, int index)
{
    PagePtr left, right;
    int i;

    right = ptr->branch[index];
    left = ptr->branch[index - 1];
    left->n++;
    left->node[left->n - 1] = ptr->node[index - 1];
    left->branch[left->n] = right->branch[0];
    ptr->node[index - 1] = right->node[0];
    right->branch[0] = right->branch[1];
    right->n--;
    for (i = 1; i < right ->n; i++) {
        right->node[i - 1] = right->node[i];
    right->branch[i] = right->branch[i + 1];
    }
}

static void BTree_Combine(BTreePtr this, PagePtr ptr, int index)
{
    PagePtr left, right;
    int i;

    right = ptr->branch[index];
    left = ptr->branch[index - 1];
    left->n++;
    left->node[left->n - 1] = ptr->node[index - 1];
    left->branch[left->n] = right->branch[0];
    for (i = 1; i <= right->n; i++) {
        left->n++;
    left->node[left->n - 1] = right->node[i - 1];
    left->branch[left->n] = right->branch[i];
    }
    BTree_RemoveItem(this, ptr, i - 1);
    xfree(right);
    
}

static void BTree_Restore(BTreePtr this, PagePtr ptr, int index)
{
    this->under = 0;
    if (index > 0) {
        if (ptr->branch[index - 1]->n > BTreeHalfSize) {
        BTree_MoveRight(this, ptr, index);
    } else {
        BTree_Combine(this, ptr, index);
    }
    } else {
        if (ptr->branch[1]->n > BTreeHalfSize) {
        BTree_MoveLeft(this, ptr, 1);
    } else {
        BTree_Combine(this, ptr, 1);
    }
    }
}

static void BTree_DeleteSub(BTreePtr this, PagePtr ptr)
{
    PagePtr p;
    int i;
    NodePtr np;

    if (ptr == NULL) {
        return;
    }
    i = 0;
    np = ptr->node;
    while ((i < ptr->n) && (np->key < this->node.key)) {
        i++;
    np++;
    }
    if ((i < ptr->n) && (np->key == this->node.key)) {
        this->del = 1;
    if ((p = ptr->branch[i + 1]) != NULL) {
        while (p->branch[0] != NULL) {
            p = p->branch[0];
        }
        ptr->node[i] = this->node = p->node[0];
        BTree_DeleteSub(this, ptr->branch[i + 1]);
        if (this->under) {
            BTree_Restore(this, ptr, i + 1);
        }
    } else {
        BTree_RemoveItem(this, ptr, i);
    }
    } else {
        BTree_DeleteSub(this, ptr->branch[i]);
    if (this->under) {
        BTree_Restore(this, ptr, i);
    }
    }
}

int BTree_Delete(BTreePtr this, int key, void *value)
{
    PagePtr ptr;

    this->node.key = key;
    this->node.value = value;
    this->del = this->under = 0;
    BTree_DeleteSub(this, this->root);
    if (this->del) {
        if (this->root->n == 0) {
        ptr = this->root;
        this->root = this->root->branch[0];
        xfree(ptr);
    }
    /* deleted */
    return 1;
    }
    /* not found */
    return 0;
}
#endif /* 0 */

static void BTree_FreeSub(PagePtr ptr)
{
    int i;

    if (ptr != NULL) {
        for (i = 0; i <= ptr->n; i++) {
            BTree_FreeSub(ptr->branch[i]);
        }
        xfree(ptr);
    }
}

void BTree_Free(BTreePtr this)
{
    BTree_FreeSub(this->root);
}

#if 0
static int count, depth, maxdepth;

static int BTree_PrintSub(PagePtr ptr)
{
    int i;

    if (ptr == NULL) {
        fprintf(stderr, ".");
    return;
    }
    fprintf(stderr, " (");
    depth++;
    maxdepth = depth;
    for (i = 0; i < ptr->n; i++) {
        BTree_PrintSub(ptr->branch[i]);
    fprintf(stderr, "%x", ptr->node[i].key);
    count++;
    }
    BTree_PrintSub(ptr->branch[ptr->n]);
    fprintf(stderr, ") ");
    depth--;
}

void BTree_Print(BTreePtr this)
{
    count = depth = maxdepth = 0;
    BTree_PrintSub(this->root);
    fprintf(stderr, "count %d depth %d\n", count, maxdepth);
}
#endif /* 0 */

/*
 * End Of File
 */
