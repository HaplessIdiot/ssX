/* $XFree86$ */
/* Author: Bill Conn <conn@bnr.ca> */

/**
 ** register definitions for WD90C33 hardware accelerator.
 **/
/** BITBLT Control Register 1, Block 1 Index 0 **/
#define C33_BLIT_CNTRL1_INDEX    (0 << 12)  /* Index 0 */
#define C33_DRAW_MODE_NOOP       (0 << 9)   /* No operation */
#define C33_DRAW_MODE_BITBLT     (1 << 9)   /* Bit Blit */
#define C33_DRAW_MODE_LINE_STRIP (2 << 9)   /* Line Strip */
#define C33_DRAW_MODE_TRAP_FILL  (3 << 9)   /* Trapezoidal Fill Strip */
#define C33_DRAW_MODE_BRES_LINE  (4 << 9)   /* Bresenham Line */

#define C33_BLT_DIRECT_X_POS     (0 << 8)  /* blit direction +x */
#define C33_BLT_DIRECT_X_NEG     (1 << 8)  /* blit direction -x */

#define C33_BLT_DIRECT_Y_POS     (0 << 7)  /* blit direction +y */
#define C33_BLT_DIRECT_Y_NEG     (1 << 7)  /* blit direction -y */

#define C33_BLT_MAJOR_X          (0 << 6)  /* Major Direction X */
#define C33_BLT_MAJOR_Y          (1 << 6)  /* Major Direction Y */

#define C33_BLT_SOURCE_SCREEN    (0 << 5)  /* Select Screen memory as Source */
#define C33_BLT_SOURCE_HOST      (1 << 5)  /* Select Host I/O or memory as Source */

#define C33_SRC_FMT_COLOR            (0 << 3)  /* Color */
#define C33_SRC_FMT_MONO_FROM_COLOR  (1 << 3)  /* Color */
#define C33_SRC_FMT_FIXED_COLOR      (2 << 3)  /* Color */
#define C33_SRC_FMT_MONO_FROM_HOST   (3 << 3)  /* Color */

#define C33_NO_PATTERN            (0 << 2) /* Pattern not used */
#define C33_PATTERN               (1 << 2) /* Pattern used as source */

#define C33_BLT_DEST_SCREEN       (0 << 1)  /* Select Screen memory as Dest */
#define C33_BLT_DEST_HOST         (1 << 1)  /* Select Host I/O or memory as Dest */

#define C33_LAST_PIXEL_ON         (0 << 0)  /* Last Pixel On for Bresnham Line */ 
#define C33_LAST_PIXEL_OFF        (1 << 0)  /* Last Pixel On for Bresnham Line */ 

/** BITBLT Control Register 2, Block 1 Index 1 **/

#define C33_BLIT_CNTRL2_INDEX     (1 << 12) /* Index 1 */

#define C33_4_BITS_PER_PIXEL      (0 << 10) /* Planar */
#define C33_8_BITS_PER_PIXEL      (1 << 10) /* Packed */
#define C33_16_BITS_PER_PIXEL     (2 << 10) /* Packed */

#define C33_DEST_TRANSPARENT_DIS  (0 << 9)  /* Destination Transparency Not Enabled */
#define C33_DEST_TRANSPARENT_ENA  (1 << 9)  /* Destination Transparency Not Enabled */

#define C33_DEST_TRANSPARENT_POS  (0 << 8)  /* Destination Transparency Positive */
#define C33_DEST_TRANSPARENT_NEG  (1 << 8)  /* Destination Transparency Positive */

#define C33_MONO_TRANSP_SRC_OFF   (0 << 7)  /* Monochrome Transparency OFF */
#define C33_MONO_TRANSP_SRC_ON    (1 << 7)  /* Monochrome Transparency ON */

#define C33_CMD_BUFFER_CLEAR_DIS  (0 << 6)  /* Command Buffer Empty Clear & Disable Int */
#define C33_CMD_BUFFER_CLEAR_ENA  (1 << 6)  /* Command Buffer Empty Enable Interrupt */

#define C33_RESERVED_BIT5         (1 << 5)  /* Should be 1 */

#define C33_DATA_PATH_FIFO_4      (0 << 4)  /* Data Path FIFO 4 Levels Deep */
#define C33_DATA_PATH_FIFO_2      (1 << 4)  /* Data Path FIFO 2 Levels Deep */

#define C33_HOST_BLT_THRU_IO      (0 << 3)  /* Host Blit through IO port */
#define C33_HOST_BLT_THRU_MEM     (1 << 3)  /* Host Blit through memory */

                                            /* Host Blit Color Expand Data Bits/Host Write */
#define C33_HOST_BLIT_2_BITS      (2 << 0)  /* 2 bits/CPU write 16 bits/pixel only */
#define C33_HOST_BLIT_4_BITS      (3 << 0)  /* 4 bits/CPU write 8 or 16 bits/pixel */

/** BITBLT SOURCE X, Block 1 Index 2 */
#define C33_BLIT_SRC_X_INDEX            (2 << 12)

/** BITBLT SOURCE Y, Block 1 Index 3 */
#define C33_BLIT_SRC_Y_INDEX            (3 << 12)

/** BITBLT DEST X, Block 1 Index 4 */
#define C33_BLIT_DEST_X_INDEX           (4 << 12)

/** BITBLT DEST Y, Block 1 Index 5 */
#define C33_BLIT_DEST_Y_INDEX           (5 << 12)

/** BITBLT Dim X, Block 1 Index 6 **/
#define C33_BLIT_DIM_X_INDEX            (6 << 12)

/** BITBLT Dim Y, Block 1 Index 7 **/
#define C33_BLIT_DIM_Y_INDEX            (7 << 12)

/** BITBLT Raster Op, Block 1 Index 8 **/
#define C33_BLIT_RAS_OP_INDEX           (8 << 12)

/** BLT Left Clip, Block 1 Index 9 **/
#define C33_BLIT_CLIP_LEFT_INDEX   ( 9 << 12)

/** BLT Right Clip, Block 1 Index 10 **/
#define C33_BLIT_CLIP_RIGHT_INDEX  (10 << 12)

/** BLT Top Clip, Block 1 Index 11 **/
#define C33_BLIT_CLIP_TOP_INDEX    (11 << 12)

/** BLT Bottom Clip, Block 1 Index 12 **/
#define C33_BLIT_CLIP_BOTTOM_INDEX    (12 << 12)

/** BLT REGISTER BLOCK INDEX Clip, Block 1 Index 15 **/
#define C33_BLIT_REGISTER_BLOCK1_INDEX_INDEX    (15 << 12)


/** BLT Map Base Address, Block 3 Index 0 **/
#define C33_BLIT_MAP_BASE_ADDR_INDEX (0 << 12)

/** BLT Row Pitch, Block 3 Index 1 **/
#define C33_BLIT_ROW_PITCH_INDEX (1 << 12)

/** BLT Foreground Color Byte 0, Block 3 Index 2 **/
#define C33_BLIT_FOREGROUND_COLOR_BYTE_0_INDEX (2 << 12)

/** BLT Foreground Color Byte 1 (for 16 bit color), Block 3 Index 3 **/
#define C33_BLIT_FOREGROUND_COLOR_BYTE_1_INDEX (3 << 12)

/** BLT Background Color Byte 0, Block 3 Index 4 **/
#define C33_BLIT_BACKGROUND_COLOR_BYTE_0_INDEX (4 << 12)

/** BLT Background Color Byte 1, Block 3 Index 5 **/
#define C33_BLIT_BACKGROUND_COLOR_BYTE_1_INDEX (5 << 12)

/** BLT Transparency Color Byte 0, Block 3 Index 6 **/
#define C33_BLIT_TRANSPARENCY_COLOR_BYTE_0_INDEX (6 << 12)

/** BLT Transparency Color Byte 1 (for 16 bit color), Block 3 Index 7 **/
#define C33_BLIT_TRANSPARENCY_COLOR_BYTE_1_INDEX (7 << 12)

/** BLT Transparency Mask Byte 0, Block 3 Index 8 **/
#define C33_BLIT_TRANSPARENCY_MASK_BYTE_0_INDEX (8 << 12)

/** BLT Transparency Mask Byte 1 (for 16 bit color), Block 3 Index 9 **/
#define C33_BLIT_TRANSPARENCY_MASK_BYTE_1_INDEX (9 << 12)

/** BLT Mask Byte 0, Block 3 Index 10 **/
#define C33_BLIT_MASK_BYTE_0_INDEX  (10 << 12)

/** BLT Mask Byte 1 (for 16 bit color), Block 3 Index 11 **/
#define C33_BLIT_MASK_BYTE_1_INDEX  (11 << 12)

/** BLT REGISTER BLOCK INDEX Clip, Block 3 Index 15 **/
#define C33_BLIT_REGISTER_BLOCK3_INDEX_INDEX    (15 << 12)


#define C33_COMMAND_BUFFER_ENABLE (1 << 5)
#define C33_LOCATIONS_USED_MASK (0x0f)
#define C33_DRAWING_ENGINE_BUSY (1 << 7)
#define C33_MAX_LOCATIONS_AVAILABLE 8

/** wait for command buffer to have enough empty locations **/
#define C33_WAIT_COMMAND_BUFFER(num_needed) \
  outw(COMMAND_BUFFER_PORT,C33_COMMAND_BUFFER_ENABLE); \
  while ((inw(COMMAND_BUFFER_PORT) & C33_LOCATIONS_USED_MASK) > \
             (C33_MAX_LOCATIONS_AVAILABLE - num_needed))

/** wait for drawing engine to complete **/
#define C33_WAIT_DRAWING_ENGINE \
  while ((inw(COMMAND_BUFFER_PORT) & C33_DRAWING_ENGINE_BUSY))


/**
 ** used to save off default functions
 **   sometimes the speedup routines need to bow out to the default funtion
 **   most of the speedups only work if the operation is screen->screen
 **/

extern void (*pvga1_stdcfbFillRectSolidCopy)();
extern int  (*pvga1_stdcfbDoBitbltCopy)();
extern void (*pvga1_stdcfbBitblt)();
extern void (*pvga1_stdcfbFillBoxSolid)();



/**
 ** these variables hold the current known state of the accel registers
 **   that way, time isn't wasted outputing a value to a port that already
 **   has that value.
 **/

/* Block 1 Shadow */

unsigned int c33_blit_cntrl2_shadow=-1;
unsigned int c33_blit_src_x_shadow=-1;
unsigned int c33_blit_src_y_shadow=-1;
unsigned int c33_blit_dim_x_shadow=-1;
unsigned int c33_blit_dim_y_shadow=-1;
unsigned int c33_blit_ras_op_shadow=-1;
unsigned int c33_blit_clip_left_shadow=-1;
unsigned int c33_blit_clip_right_shadow=-1;
unsigned int c33_blit_clip_top_shadow=-1;
unsigned int c33_blit_clip_bottom_shadow=-1;

/* Block 3 Shadow */
unsigned int c33_blit_map_base_addr_shadow=-1;
unsigned int c33_blit_row_pitch_shadow=-1;
unsigned int c33_blit_foreground_color_byte_0_shadow=-1;
unsigned int c33_blit_foreground_color_byte_1_shadow=-1;
unsigned int c33_blit_background_color_byte_0_shadow=-1;
unsigned int c33_blit_background_color_byte_1_shadow=-1;
unsigned int c33_blit_transparency_color_byte_0_shadow=-1;
unsigned int c33_blit_transparency_color_byte_1_shadow=-1;
unsigned int c33_blit_transparency_mask_byte_0_shadow=-1;
unsigned int c33_blit_transparency_mask_byte_1_shadow=-1;
unsigned int c33_blit_mask_byte_0_shadow=-1;
unsigned int c33_blit_mask_byte_1_shadow=-1;


#define INDEX_CONTROL_PORT   0x23c0
#define REGISTER_ACCESS_PORT 0x23c2
#define COMMAND_BUFFER_PORT  0x23ce

/**
 ** macros to set each hardware register
 **/

#define C33_SELECT_REG_BANK1   outw (INDEX_CONTROL_PORT, 0x1)
#define C33_SELECT_REG_BANK3   outw (INDEX_CONTROL_PORT, 0x3)

#define SET_C33_BANK1_CNTRL1(value)  outw(REGISTER_ACCESS_PORT, C33_BLIT_CNTRL1_INDEX | value);

#define SET_C33_BANK1_CNTRL2(value) \
              if (c33_blit_cntrl2_shadow != (value)) \
                 outw(REGISTER_ACCESS_PORT, (C33_BLIT_CNTRL2_INDEX | value)); \
              c33_blit_cntrl2_shadow = value;

#define SET_C33_BANK1_SRC_X(value) \
              if (c33_blit_src_x_shadow != (value)) \
                  outw (REGISTER_ACCESS_PORT, C33_BLIT_SRC_X_INDEX | value);\
              c33_blit_src_x_shadow = value;

#define SET_C33_BANK1_SRC_Y(value) \
              if (c33_blit_src_y_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_SRC_Y_INDEX | value)); \
              c33_blit_src_y_shadow = value;

#define SET_C33_BANK1_DEST_X(value) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_DEST_X_INDEX | value)); 

#define SET_C33_BANK1_DEST_Y(value) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_DEST_Y_INDEX | value)); 

#define SET_C33_BANK1_DIM_X(value) \
              if (c33_blit_dim_x_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_DIM_X_INDEX | (value-1))); \
              c33_blit_dim_x_shadow = value;

#define SET_C33_BANK1_DIM_Y(value) \
              if (c33_blit_dim_y_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_DIM_Y_INDEX | (value-1))); \
              c33_blit_dim_y_shadow = value;

#define SET_C33_BANK1_RAS_OP(value) \
              if (c33_blit_ras_op_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_RAS_OP_INDEX | value)); \
              c33_blit_ras_op_shadow = value;

#define SET_C33_BANK1_CLIP_LEFT(value) \
              if (c33_blit_clip_left_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_CLIP_LEFT_INDEX | value)); \
              c33_blit_clip_left_shadow = value;

#define SET_C33_BANK1_CLIP_RIGHT(value) \
              if (c33_blit_clip_right_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_CLIP_RIGHT_INDEX | value)); \
              c33_blit_clip_right_shadow = value;

#define SET_C33_BANK1_CLIP_TOP(value) \
              if (c33_blit_clip_top_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_CLIP_TOP_INDEX | value)); \
              c33_blit_clip_top_shadow = value;

#define SET_C33_BANK1_CLIP_BOTTOM(value) \
              if (c33_blit_clip_bottom_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_CLIP_BOTTOM_INDEX | value)); \
              c33_blit_clip_bottom_shadow = value;



#define SET_C33_BANK3_MAP_BASE_ADDR(value) \
              if (c33_blit_map_base_addr_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_MAP_BASE_ADDR_INDEX | value)); \
              c33_blit_map_base_addr_shadow = value;

#define SET_C33_BANK3_ROW_PITCH(value) \
              if (c33_blit_row_pitch_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_ROW_PITCH_INDEX | value)); \
              c33_blit_row_pitch_shadow = value;

#define SET_C33_BANK3_FOREGROUND_COLOR_BYTE_0(value) \
              if (c33_blit_foreground_color_byte_0_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_FOREGROUND_COLOR_BYTE_0_INDEX | value)); \
              c33_blit_foreground_color_byte_0_shadow = value;

#define SET_C33_BANK3_FOREGROUND_COLOR_BYTE_1(value) \
              if (c33_blit_foreground_color_byte_1_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_FOREGROUND_COLOR_BYTE_1_INDEX | value)); \
              c33_blit_foreground_color_byte_1_shadow = value;

#define SET_C33_BANK3_BACKGROUND_COLOR_BYTE_0(value) \
              if (c33_blit_background_color_byte_0_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_BACKGROUND_COLOR_BYTE_0_INDEX | value)); \
              c33_blit_background_color_byte_0_shadow = value;

#define SET_C33_BANK3_BACKGROUND_COLOR_BYTE_1(value) \
              if (c33_blit_background_color_byte_1_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_BACKGROUND_COLOR_BYTE_1_INDEX | value)); \
              c33_blit_background_color_byte_1_shadow = value;

#define SET_C33_BANK3_TRANSPARENCY_COLOR_BYTE_0(value) \
              if (c33_blit_transparency_color_byte_0_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_TRANSPARENCY_COLOR_BYTE_0_INDEX | value)); \
              c33_blit_transparency_color_byte_0_shadow = value;

#define SET_C33_BANK3_TRANSPARENCY_COLOR_BYTE_1(value) \
              if (c33_blit_transparency_color_byte_1_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_TRANSPARENCY_COLOR_BYTE_1_INDEX | value)); \
              c33_blit_transparency_color_byte_1_shadow = value;

#define SET_C33_BANK3_TRANSPARENCY_MASK_BYTE_0(value) \
              if (c33_blit_transparency_mask_byte_0_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_TRANSPARENCY_MASK_BYTE_0_INDEX | value)); \
              c33_blit_transparency_mask_byte_0_shadow = value;

#define SET_C33_BANK3_TRANSPARENCY_MASK_BYTE_1(value) \
              if (c33_blit_transparency_mask_byte_1_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_TRANSPARENCY_MASK_BYTE_1_INDEX | value)); \
              c33_blit_transparency_mask_byte_1_shadow = value;

#define SET_C33_BANK3_MASK_BYTE_0(value) \
              if (c33_blit_mask_byte_0_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_MASK_BYTE_0_INDEX | value)); \
              c33_blit_mask_byte_0_shadow = value;

#define SET_C33_BANK3_MASK_BYTE_1(value) \
              if (c33_blit_mask_byte_1_shadow != (value)) \
                  outw(REGISTER_ACCESS_PORT, (C33_BLIT_MASK_BYTE_1_INDEX | value)); \
              c33_blit_mask_byte_1_shadow = value;

