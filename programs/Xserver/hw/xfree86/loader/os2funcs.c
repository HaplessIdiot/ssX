/*
 * (c) Copyright 1997 by Sebastien Marineau
 *                      <marineau@genie.uottawa.ca>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * HOLGER VEIT  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Except as contained in this notice, the name of Sebastien Marineau shall not be
 * used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization from Sebastien Marineau.
 *
 */


/* Implements some OS/2 memory allocation functions to allow 
 * execute permissions for modules. We allocate some mem using DosAllocMem
 * and then use the EMX functions to create a heap from which we allocate
 * the requests. We create a heap of 2 megs, hopefully enough for now.
 */

#define INCL_DOSMEMMGR
#include <os2.h>
#include <sys/types.h>
#include <umalloc.h>
#include "os.h"

void *os2_calloc(size_t num_elem, size_t size_elem){
APIRET rc;
static PVOID baseAddress;
static Heap_t heapAddress;
int ret;
static BOOL FirstTime=TRUE;
void *allocMem;

if(FirstTime){
	rc=DosAllocMem(&baseAddress,512*4096, PAG_READ | PAG_WRITE | PAG_EXECUTE | PAG_COMMIT);
	if(rc!=0) {
		ErrorF("OS/2AllocMem: Could not create heap for module loading\n");
		return(NULL);
		}
	ErrorF("OS2Alloc: allocated mem for heap, rc=%d, addr=%p\n",rc,heapAddress);
	if((heapAddress=_ucreate(baseAddress,512*4096,!_BLOCK_CLEAN,
		_HEAP_REGULAR,NULL,NULL))==NULL){
		ErrorF("OS/2AllocMem: Could not create heap for loadable modules\n");
		DosFreeMem(baseAddress);
		return(NULL);
		}
	
	ret=_uopen(heapAddress);
	if(ret!=0){
		ErrorF("OS/2AllocMem: Could not open heap for loadable modules\n");
		ret=_udestroy(heapAddress,_FORCE);
		DosFreeMem(baseAddress);
		return(NULL);
		}
	FirstTime=FALSE;
	ErrorF("OS/2: done creating heap, addr=%p\n",heapAddress);
	}

allocMem=_ucalloc(heapAddress,num_elem,size_elem);
return(allocMem);
}
