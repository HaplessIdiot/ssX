/* just a few dummies to resolve symbols on the PowerPC
 * until we translate the x86 assembler back to C code
 */

CirrusImageWriteTransfer( int width, int height, void *srcaddr,
     int srcwidth, void *vaddr )
{
	ErrorF("CirrusImageWriteTransfer\n");
}

CirrusImageReadTransfer( int width, int height, void *srcaddr,
     int srcwidth, void *vaddr )
{
	ErrorF("CirrusImageWriteTransfer\n");
}

CirrusBitmapTransfer( int bytewidth, int h, int bwidth, void *srcaddr,
	void *vaddr )
{
	ErrorF("CirrusBitmapTransfer\n");
}	
