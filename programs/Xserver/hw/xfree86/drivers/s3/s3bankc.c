#include "compiler.h"

void
S3SetRead(unsigned char segment)
{	
/*	Segment b0..3 -> CRTC[0x35] b0..3 */
	outb(0x3d4, 0x35);
	outb(0x3d5, ((segment & 0xf) | (inb(0x3d5) & 0xf0)));
/*	Segment b4.5 -> CRTC[0x51] b2.3 */
	outb(0x3d4, 0x51);
	outb(0x3d5, ((segment & 0x30) | (inb(0x3d5) & 0xf3)));
}
