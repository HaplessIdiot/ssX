/* Id: hash.h,v 1.2 1999/02/26 08:52:33 martin Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * Log: hash.h,v $
 * Revision 1.2  1999/02/26 08:52:33  martin
 * Updated Mesa to the official version checked into the Mesa cvs archive
 * Cleaned up the XMesa interface
 * Fixed support for standalone Mesa
 * Removed several unused files in the lib/GL/mesa/src subdirectory
 * Moved glcore.h back to include/GL/internal/glcore.h
 *
 * Revision 3.1  1999/02/14 03:46:34  brianp
 * new copyright
 *
 * Revision 3.0  1998/01/31 20:53:58  brianp
 * initial rev
 *
 */


#ifndef HASH_H
#define HASH_H


#include "GL/gl.h"


struct HashTable;



extern struct HashTable *NewHashTable(void);

extern void DeleteHashTable(struct HashTable *table);

extern void *HashLookup(const struct HashTable *table, GLuint key);

extern void HashInsert(struct HashTable *table, GLuint key, void *data);

extern void HashRemove(struct HashTable *table, GLuint key);

extern GLuint HashFirstEntry(const struct HashTable *table);

extern void HashPrint(const struct HashTable *table);

extern GLuint HashFindFreeKeyBlock(const struct HashTable *table, GLuint numKeys);


#endif
