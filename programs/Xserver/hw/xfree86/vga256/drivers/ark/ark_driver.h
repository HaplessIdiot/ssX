/* $XFree86$ */

/* ChipRec declared in ark_driver.c. */

extern vgaVideoChipRec ARK;

/* Driver variables. */

extern int arkChip;
extern unsigned char *arkMMIOBase;

/* Framebuffer functions. */

extern void ArkBitBlt();	/* 8bpp vga256LowlevFunc */
extern RegionPtr Ark16CopyArea();
extern RegionPtr Ark24CopyArea();
extern RegionPtr Ark32CopyArea();
extern void ArkCopyWindow();	/* 16/24/32bpp */

/* Chip types. */

#define ARK1000VL	0
#define ARK1000PV	1
#define ARK2000PV	2
#define ARK2000MT	3	/* not tested */
