/* just a few dummies to resolve symbols on the PowerPC
 * until we translate the x86 assembler back to C code
 */

CirrusTransferText32bit( int nglyph, int height, unsigned long **glyphp,
     int glyphwidth, void *vaddr )
{
	ErrorF("CirrusTransferText32bit\n");
}

CirrusTransferText32bitSpecial( int nglyph, int height, unsigned long **glyphp,
     int glyphwidth, void *vaddr )
{
	ErrorF("CirrusTransferText32bitSpecial\n");
}

