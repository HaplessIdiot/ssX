/* $XFree86$ */

#ifndef _LNX_AXP_H_
#define _LNX_AXP_H_

extern void _dense_outb(unsigned char, unsigned long);
extern void _dense_outw(unsigned short, unsigned long);
extern void _dense_outl(unsigned int, unsigned long);
extern unsigned char _dense_inb(unsigned long);
extern unsigned short _dense_inw(unsigned long);
extern unsigned int _dense_inl(unsigned long);

#endif /* _LNX_AXP_H_ */
