/* External functions */
extern void TRITECHAccelInit(void);
extern void TRITECHInitializeAccelerator(void);

extern unsigned long Tritech_nbase,Tritech_pbase;
extern unsigned char* Tritech_ctrl;

#define TYPE_UNKNOWN -1
#define TYPE_TR25202 PCI_CHIP_TR25202

#define P3D_PINMUX 16

/* Shading Program Defines */

/* Op codes */
#define P3D_COLOR_OP		(1 << 19)
#define P3D_STIPPLE_BLEND	(2 << 19)
#define P3D_LOGIC_OP		(3 << 19)
#define P3D_CREAD		(4 << 19)
#define P3D_ZREAD		(6 << 19)
#define P3D_ZWRITE		(7 << 19)
#define P3D_TEXTFETCH		(8 << 19)
#define P3D_TEXTFETCH_MODULATE	(9 << 19)
#define P3D_BILIN		(10 << 19)
#define P3D_PALETTE		(12 << 19)

#define P3D_DEST_FB		0
#define P3D_DEST_TMP1		1
#define P3D_DEST_TMP2		2
#define P3D_DEST_TMP3		3

#define P3D_AADDR_COEF0		(0 << 10)
#define P3D_AADDR_COEF1		(1 << 10)
#define P3D_AADDR_COEF2		(2 << 10)
#define P3D_AADDR_COEF3		(3 << 10)
#define P3D_AADDR_TMP1		(5 << 10)
#define P3D_AADDR_TMP2		(6 << 10)
#define P3D_AADDR_TMP3		(7 << 10)


#define P3D_BADDR_COEF0		(0 << 6)
#define P3D_BADDR_COEF1		(1 << 6)
#define P3D_BADDR_COEF2		(2 << 6)
#define P3D_BADDR_COEF3		(3 << 6)
#define P3D_BADDR_TMP1		(5 << 6)
#define P3D_BADDR_TMP2		(6 << 6)
#define P3D_BADDR_TMP3		(7 << 6)


#define P3D_END			(1 << 23)



/* Registers in the TR25202 */

/* PCI Config Space Registers */

#define P3D_ID_REG		0x00
#define P3D_STATUS_CMD		0x04
#define P3D_CLASS_REV 		0x08
#define P3D_CFG0		0x0c
#define P3D_GR_RAM_BAR		0x10
#define P3D_CTRL_REG_BAR	0x14
#define P3D_SUB_ID		0x2c
#define P3D_EXP_ROM_BAR		0x30
#define P3D_CFG1		0x3c
#define P3D_CORE_CLK_CFG	0x40
#define P3D_MEM_CFG		0x44
#define P3D_VIDEO_CLK_CFG	0x48
#define P3D_REG_ACC_ADDR	0x4c
#define P3D_REG_ACC_DATA	0x50


/* System Control Registers */

#define P3D_MA_CMD_ADDR		0xa8
#define P3D_MASTER_STATE	0xac
#define P3D_MA_INT_ADDR		0xb0
#define P3D_MA_EXT_ADDR		0xb4
#define P3D_STATUS		0xc0
#define P3D_REF_REG		0xc4
#define P3D_IO_REG		0xcc
#define P3D_EXT_IO_REG		0xd0
#define P3D_MEM_APT0_CFG	0xd8
#define P3D_MEM_APT1_CFG	0xdc


/* Video Interface Registers */
#define P3D_VIDEO_WIDTH_HEIGHT	0x84
#define P3D_SCREEN_WIDTH_HEIGHT	0x88
#define P3D_VIDEO_VBLANK	0x8c
#define P3D_VIDEO_HBLANK	0x90
#define P3D_VIDEO_VSYNC		0x94
#define P3D_VIDEO_HSYNC		0x98
#define P3D_VIDEO_BASE_CONF	0x9c
#define P3D_VIDEO_BIT_CONFIG	0xa0

/* Primitive Processor Registers */
#define P3D_CR_INIT		0x100
#define P3D_CR_DY		0x104
#define P3D_CR_DX		0x108
#define P3D_CG_INIT		0x10c
#define P3D_CG_DY		0x110
#define P3D_CG_DX		0x114
#define P3D_CB_INIT		0x118
#define P3D_CB_DY		0x11c
#define P3D_CB_DX		0x120
#define P3D_CT_INIT		0x124
#define P3D_CT_DY		0x128
#define P3D_CT_DX		0x12C
#define P3D_CT_INIT		0x124
#define P3D_CT_DY		0x128
#define P3D_CT_DX		0x12C
#define P3D_ATU_INIT		0x130
#define P3D_ATU_DY		0x134
#define P3D_ATU_DX		0x138
#define P3D_ATV_INIT		0x13C
#define P3D_ATV_DY		0x140
#define P3D_ATV_DX		0x144
#define P3D_BTU_INIT		0x148
#define P3D_BTU_DY		0x14C
#define P3D_BTU_DX		0x150
#define P3D_BTV_INIT		0x154
#define P3D_BTV_DY		0x158
#define P3D_BTV_DX		0x15C
#define P3D_Z_SHR		0x160
#define P3D_Z_INIT		0x164
#define P3D_Z_DY		0x168
#define P3D_Z_DX		0x16C
#define P3D_EDGE_ORDER		0x170
#define P3D_EDGE0_INIT		0x174
#define P3D_EDGE0_DX		0x178
#define P3D_EDGE0_DY		0x17C
#define P3D_EDGE1_INIT		0x180
#define P3D_EDGE1_DX		0x184
#define P3D_EDGE1_DY		0x188
#define P3D_EDGE2_INIT		0x18C
#define P3D_EDGE2_DX		0x190
#define P3D_EDGE2_DY		0x194
#define P3D_GRID_REG		0x198
#define P3D_P_INIT		0x19C
#define P3D_P_DY		0x1A0
#define P3D_P_DX		0x1A4
#define P3D_X_INIT		0x1A8
#define P3D_Y_INIT		0x1AC
#define P3D_Y_END		0x1B0
#define P3D_RASTER_EXT		0x1B4

/* Pixel Processor Registers */
#define P3D_COEF_REG0		0x04
#define P3D_COEF_REG1		0x08
#define P3D_COEF_REG2		0x0C
#define P3D_COEF_REG3		0x10
#define P3D_ATEX_CONF1		0x14
#define P3D_ATEX_CONF2		0x18
#define P3D_BTEX_CONF1		0x1C
#define P3D_BTEX_CONF2		0x20
#define P3D_BASE_ADDR		0x24
#define P3D_DITHER		0x28
#define P3D_MODULATION		0x2C
#define P3D_PPU_MODE		0x30
#define P3D_FRAME_MODE		0x34
#define P3D_PPU_CODE_START	0x38
#define P3D_PALETTE_BASE	0x3C

#define P3D_SHADE_PROG_MEM	0x200



/* more or less straight from mga.h */
extern PCITAG TritechPciTag;

#if defined(__alpha__)
#define mb() __asm__ __volatile__("mb": : :"memory")
#define INREG8(addr) xf86ReadSparse8(Tritech_ctrl, addr)
#define INREG16(addr) xf86ReadSparse16(Tritech_ctrl, addr)
#define INREG(addr) xf86ReadSparse32(Tritech_ctrl, addr)
#define OUTREG8(addr,val) do { xf86WriteSparse8((val),Tritech_ctrl,addr); \
 mb();} while(0)
#define OUTREG16(addr,val) do { xf86WriteSparse16((val),Tritech_ctrl,addr); \
 mb();} while(0)
#define OUTREG(addr, val) do { xf86WriteSparse32((val),Tritech_ctrl,addr); \
 mb();} while(0)
#else /* not __alpha__ */
#define INREG8(addr) *(volatile CARD8 *)(Tritech_ctrl + addr)
#define INREG16(addr) *(volatile CARD16 *)(Tritech_ctrl + addr)
#define INREG(addr) *(volatile CARD32 *)(Tritech_ctrl + addr)
#define OUTREG8(addr, val) *(volatile CARD8 *)(Tritech_ctrl + addr) = (val)
#define OUTREG16(addr, val) *(volatile CARD16 *)(Tritech_ctrl + addr) = (val)
#define OUTREG(addr, val) *(volatile CARD32 *)(Tritech_ctrl + addr) = (val)
#endif /* not __alpha__ */


#define WAIT_IDLE 		while(((INREG(P3D_STATUS)) & 0x0c) != 0x0c);
#define WAIT_PIX_PROC_IDLE 	while(!((INREG(P3D_STATUS)) & 0x04));
#define WAIT_PRIM_PROC_IDLE 	while(!((INREG(P3D_STATUS)) & 0x08));
#define CONVERT_TO_888(c)	(((c & 0x1F) << 3) |	\
				((c & 0x7E0) << 5) |	\
				((c & 0xF800) << 8))




