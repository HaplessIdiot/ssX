/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */
/* $XFree86$ */
/* Video status flag */

#define VIDEO_SHOW		0x80000000  /*Video on*/
#define VIDEO_HQV_INUSE		0x04000000  /*Video is used with HQV*/
#define VIDEO_1_INUSE		0x01000000  /*Video 1 is used with software flip*/
#define SW_USE_HQV		0x00000020  /*[ 5] : 1:Capture 1 use HQV,0:Capture 1 not use HQV	*/

/* H/W registers for Video Engine */

/*
 *	video registers
 */
#define V_FLAGS			0x00
#define V_FLIP_STATUS		0x04
#define V_COLOR_KEY		0x20
#define V_CHROMAKEY_LOW		0x28
#define V_CHROMAKEY_HIGH	0x2C
#define V1_CONTROL		0x30
#define V12_QWORD_PER_LINE	0x34
#define V1_STARTADDR_1		0x38
#define V1_STRIDE		0x3C
#define V1_WIN_START_Y		0x40
#define V1_WIN_START_X		0x42
#define V1_WIN_END_Y		0x44
#define V1_WIN_END_X		0x46
#define V1_ZOOM_CONTROL		0x4C
#define V1_MINI_CONTROL		0x50
#define V1_STARTADDR_0		0x54
#define V_FIFO_CONTROL		0x58
#define HI_CONTROL		0x60
#define SND_COLOR_KEY		0x64
#define V1_SOURCE_HEIGHT	0x6C
#define V1_ColorSpaceReg_1	0x84
#define V1_ColorSpaceReg_2	0x88
#define V1_STARTADDR_CB0	0x8C
#define V1_OPAQUE_CONTROL	0x90  /* To be deleted */
#define V_COMPOSE_MODE		0x98
#define V1_STARTADDR_CB1	0xE4
#define V1_STARTADDR_CB2	0xE8
#define V1_STARTADDR_CB3	0xEC
#define V1_STARTADDR_CR0	0xF0
#define V1_STARTADDR_CR1	0xF4
#define V1_STARTADDR_CR2	0xF8
#define V1_STARTADDR_CR3	0xFC

/* HQV Registers */
#define HQV_CONTROL		0x1D0
#define HQV_SRC_STARTADDR_Y	0x1D4
#define HQV_SRC_STARTADDR_U	0x1D8
#define HQV_SRC_STARTADDR_V	0x1DC
#define HQV_SRC_FETCH_LINE	0x1E0
#define HQV_FILTER_CONTROL	0x1E4
#define HQV_MINIFY_CONTROL	0x1E8
#define HQV_DST_STARTADDR0	0x1EC
#define HQV_DST_STARTADDR1	0x1F0
#define HQV_DST_STRIDE		0x1F4
#define HQV_SRC_STRIDE		0x1F8


/*
 *  Video command definition
 */
/* V_CHROMAKEY_LOW	   0x228 */
#define V_CHROMAKEY_V3		0x80000000

/* V1_CONTROL			0x230 */
#define V1_ENABLE		0x00000001
#define V1_FULL_SCREEN		0x00000002
#define V1_YUV422		0x00000000
#define V1_RGB32		0x00000004
#define V1_RGB15		0x00000008
#define V1_RGB16		0x0000000C
#define V1_YCbCr420		0x00000010
#define V1_COLORSPACE_SIGN	0x00000080
#define V1_SRC_IS_FIELD_PIC	0x00000200
#define V1_SRC_IS_FRAME_PIC	0x00000000
#define V1_BOB_ENABLE		0x00400000
#define V1_FIELD_BASE		0x00000000
#define V1_FRAME_BASE		0x01000000
#define V1_SWAP_SW		0x00000000
#define V1_SWAP_HW_HQV		0x02000000
#define V1_SWAP_HW_CAPTURE	0x04000000
#define V1_SWAP_HW_MC		0x06000000
/* #define V1_DOUBLE_BUFFERS	   0x00000000 */
/* #define V1_QUADRUPLE_BUFFERS	   0x18000000 */
#define V1_EXPIRE_NUM		0x00050000
#define V1_EXPIRE_NUM_A		0x000a0000
#define V1_EXPIRE_NUM_F		0x000f0000
#define V1_FIFO_EXTENDED	0x00200000
#define V1_ON_CRT		0x00000000
#define V1_ON_SND_DISPLAY	0x80000000
#define V1_FIFO_32V1_32V2	0x00000000
#define V1_FIFO_48V1_32V2	0x00200000

/* V12_QWORD_PER_LINE		0x234 */
#define V1_FETCH_COUNT		0x3ff00000
#define V1_FETCHCOUNT_ALIGNMENT 0x0000000f
#define V1_FETCHCOUNT_UNIT	0x00000004   /* Doubld QWORD */

/* V1_STRIDE */
#define V1_STRIDE_YMASK		0x00001fff
#define V1_STRIDE_UVMASK	0x1ff00000

/* V1_ZOOM_CONTROL		0x24C */
#define V1_X_ZOOM_ENABLE	0x80000000
#define V1_Y_ZOOM_ENABLE	0x00008000

/* V1_MINI_CONTROL		0x250 */
#define V1_X_INTERPOLY		0x00000002  /* X interpolation */
#define V1_Y_INTERPOLY		0x00000001  /* Y interpolation */
#define V1_YCBCR_INTERPOLY	0x00000004  /* Y, Cb, Cr all interpolation */
#define V1_X_DIV_2		0x01000000
#define V1_X_DIV_4		0x03000000
#define V1_X_DIV_8		0x05000000
#define V1_X_DIV_16		0x07000000
#define V1_Y_DIV_2		0x00010000
#define V1_Y_DIV_4		0x00030000
#define V1_Y_DIV_8		0x00050000
#define V1_Y_DIV_16		0x00070000

/* V_FIFO_CONTROL		0x258
 * IA2 has 32 level FIFO for packet mode video format
 *	   32 level FIFO for planar mode video YV12. with extension reg 230 bit 21 enable
 *	   16 level FIFO for planar mode video YV12. with extension reg 230 bit 21 disable
 * BCos of 128 bits. 1 level in IA2 = 2 level in VT3122
 */
#define V1_FIFO_DEPTH12		0x0000000B
#define V1_FIFO_DEPTH16		0x0000000F
#define V1_FIFO_DEPTH32		0x0000001F
#define V1_FIFO_DEPTH48		0x0000002F
#define V1_FIFO_THRESHOLD6	0x00000600
#define V1_FIFO_THRESHOLD8	0x00000800
#define V1_FIFO_THRESHOLD12	0x00000C00
#define V1_FIFO_THRESHOLD16	0x00001000
#define V1_FIFO_THRESHOLD24	0x00001800
#define V1_FIFO_THRESHOLD32	0x00002000
#define V1_FIFO_THRESHOLD40	0x00002800
#define V1_FIFO_PRETHRESHOLD10	0x0A000000
#define V1_FIFO_PRETHRESHOLD12	0x0C000000
#define V1_FIFO_PRETHRESHOLD29	0x1d000000
#define V1_FIFO_PRETHRESHOLD40	0x28000000
#define V1_FIFO_PRETHRESHOLD44	0x2c000000

/* IA2 */
#define ColorSpaceValue_1	0x140020f2
#define ColorSpaceValue_2	0x0a0a2c00

/* V_COMPOSE_MODE		0x298 */
#define SELECT_VIDEO_IF_COLOR_KEY		0x00000001  /* select video if (color key),otherwise select graphics */
#define SELECT_VIDEO_IF_CHROMA_KEY		0x00000002  /* 0x0000000a  //select video if (chroma key ),otherwise select graphics */
#define ALWAYS_SELECT_VIDEO			0x00000000  /* always select video,Chroma key and Color key disable */
#define COMPOSE_V1_TOP		0x00000000
#define V1_COMMAND_FIRE		0x80000000  /* V1 commands fire */
#define V_COMMAND_LOAD		0x20000000  /* Video register always loaded */
#define V_COMMAND_LOAD_VBI	0x10000000  /* Video register always loaded at vbi without waiting source flip */

/* HQV_CONTROL		   0x3D0 */
#define HQV_RGB32	    0x00000000
#define HQV_RGB16	    0x20000000
#define HQV_RGB15	    0x30000000
#define HQV_YUV422	    0x80000000
#define HQV_YUV420	    0xC0000000
#define HQV_ENABLE	    0x08000000
#define HQV_SRC_SW	    0x00000000
#define HQV_SRC_MC	    0x01000000
#define HQV_SRC_CAPTURE0    0x02000000
#define HQV_SRC_CAPTURE1    0x03000000
#define HQV_FLIP_EVEN	    0x00000000
#define HQV_FLIP_ODD	    0x00000020
#define HQV_SW_FLIP	    0x00000010	 /* Write 1 to flip HQV buffer */
#define HQV_DEINTERLACE	    0x00010000	 /* First line of odd field will be repeated 3 times */
#define HQV_FIELD_2_FRAME   0x00020000	 /* Src is field. Display each line 2 times */
#define HQV_FRAME_2_FIELD   0x00040000	 /* Src is field. Display field */
#define HQV_FRAME_UV	    0x00000000	 /* Src is Non-interleaved */
#define HQV_FIELD_UV	    0x00100000	 /* Src is interleaved */
#define HQV_IDLE	    0x00000008
#define HQV_FLIP_STATUS	    0x00000001

/* IA2 NEW */
#define HQV_V_FILTER2		0x00080000
#define HQV_H_FILTER2		0x00000008
#define HQV_H_TAP2_11		0x00000041
#define HQV_H_TAP4_121		0x00000042
#define HQV_H_TAP4_1111		0x00000401
#define HQV_H_TAP8_1331		0x00000221
#define HQV_H_TAP8_12221	0x00000402
#define HQV_H_TAP16_1991	0x00000159
#define HQV_H_TAP16_141041	0x0000026A
#define HQV_H_TAP32		0x0000015A
#define HQV_V_TAP2_11		0x00410000
#define HQV_V_TAP4_121		0x00420000
#define HQV_V_TAP4_1111		0x04010000
#define HQV_V_TAP8_1331		0x02210000
#define HQV_V_TAP8_12221	0x04020000
#define HQV_V_TAP16_1991	0x01590000
#define HQV_V_TAP16_141041	0x026A0000
#define HQV_V_TAP32		0x015A0000
#define HQV_V_FILTER_DEFAULT	0x00420000
#define HQV_H_FILTER_DEFAULT	0x00000040

/* HQV_MINI_CONTROL	   0x3E8 */
#define HQV_H_MINIFY_ENABLE 0x00000800
#define HQV_V_MINIFY_ENABLE 0x08000000
#define HQV_VDEBLOCK_FILTER 0x80000000
#define HQV_HDEBLOCK_FILTER 0x00008000

#define CHROMA_KEY_LOW		0x00FFFFFF
#define CHROMA_KEY_HIGH		0x00FFFFFF

#define VBI_STATUS		0x00000002

/*
 *	Macros for Video MMIO
 */
#ifndef V4L2
#define VIDInB(port)		*((volatile CARD8 *)(lpVidMEMIO + (port)))
#define VIDInW(port)		*((volatile CARD16 *)(lpVidMEMIO + (port)))
#define VIDInD(port)		*((volatile CARD32 *)(lpVidMEMIO + (port)))
#define VIDOutB(port, data)	*((volatile CARD8 *)(lpVidMEMIO + (port))) = (data)
#define VIDOutW(port, data)	*((volatile CARD16 *)(lpVidMEMIO + (port))) = (data)
#define VIDOutD(port, data)	*((volatile CARD32 *)(lpVidMEMIO + (port))) = (data)
#define MPGOutD(port, data)	*((volatile CARD32 *)(lpMPEGMMIO +(port))) = (data)
#define MPGInD(port)		*((volatile CARD32 *)(lpMPEGMMIO +(port)))
#endif

/*
 *	Macros for GE MMIO
 */
#define GEInW(port)		*((volatile CARD16 *)(lpGEMMIO + (port)))
#define GEInD(port)		*((volatile CARD32 *)(lpGEMMIO + (port)))
#define GEOutW(port, data)	*((volatile CARD16 *)(lpGEMMIO + (port))) = (data)
#define GEOutD(port, data)	*((volatile CARD32 *)(lpGEMMIO + (port))) = (data)

