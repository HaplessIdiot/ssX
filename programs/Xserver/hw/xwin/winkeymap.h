/*
 *Copyright (C) 1994-2000 The XFree86 Project, Inc. All Rights Reserved.
 *
 *Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 *"Software"), to deal in the Software without restriction, including
 *without limitation the rights to use, copy, modify, merge, publish,
 *distribute, sublicense, and/or sell copies of the Software, and to
 *permit persons to whom the Software is furnished to do so, subject to
 *the following conditions:
 *
 *The above copyright notice and this permission notice shall be
 *included in all copies or substantial portions of the Software.
 *
 *THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *NONINFRINGEMENT. IN NO EVENT SHALL THE XFREE86 PROJECT BE LIABLE FOR
 *ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 *CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *Except as contained in this notice, the name of the XFree86 Project
 *shall not be used in advertising or otherwise to promote the sale, use
 *or other dealings in this Software without prior written authorization
 *from the XFree86 Project.
 *
 * Authors:	Dakshinamurthy Karra
 *		Suhaib M Siddiqi
 *		Peter Busch
 *		Harold L Hunt II
 */
/* $XFree86$ */

/* This table maps Windows Virtual Key Codes to X11 KeySyms.
   
   To map a particular VKey, lookup the value of the VKey
   in MSDN Library->Platform SDK->User Interface Services->
   Windows User Interface->User Input->Virtual-Key Codes,
   find the array position corresponding to the VKey code
   (e.g. 0x09 for VK_TAB), and fill in the first column
   in that row with the primary key cap, the second column
   with the secondary key (shift), and so on.  Note that
   there are not generally any symbols in the third and fourth
   columns of a particular row.
   
   Lookup the XK constants in xc/include/keysymdef.h.
*/
static KeySym g_winKeySym[NUM_KEYCODES * GLYPHS_PER_KEY] = { 
    /* 0x00 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x01 through 0x06 are mouse buttons */
    /* 0x01 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x02 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x03 */  XK_Cancel,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x04 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x05 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x06 */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x07 undefined */
    /* 0x07 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x08 */  XK_BackSpace,   NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x09 */  XK_Tab,		NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x0a through 0x0b reserved */
    /* 0x0a */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x0b */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x0c */  XK_Clear,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x0d */  XK_Return,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x0e through 0x0f undefined */
    /* We use 0x0e for the numeric keypad return key */
    /* 0x0e */  XK_KP_Enter,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x0f */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x10, 0x11, and 0x12 are Shift, Control, and Alt, but we don't use
       them because they do not distinquish between Left and Right keys.
       
       See keys 0xa0 through 0xa5 for Shift, Control, and Alt symbols.
    */
    /* 0x10 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x11 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x12 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x13 */  XK_Pause,       NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x14 */  XK_Caps_Lock,   NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x15 */  XK_Kana_Lock,   NoSymbol,	NoSymbol,	NoSymbol,
    
    /* 0x16 undefined */
    /* 0x16 */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,

    /* I don't know what to pass for Junja and Final mode keys,
       so I'm just passing a mode siwtch.
    */
    /* 0x19 for Kanji mode may be the wrong symbol */
    /* 0x17 */  XK_Mode_switch,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x18 */  XK_Mode_switch,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x19 */  XK_Kanji,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x1a undefined */
    /* 0x1a */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x1c, 0x1d, and 0x1e are probably wrong.
       Having a symbol passed at least allows people to map the key
       to something more useful.
    */
    /* 0x1b */  XK_Escape,      NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x1c */  XK_Henkan,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x1d */  XK_Muhenkan,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x1e */  XK_Kanji_Bangou,NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x1f */  XK_Mode_switch,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x20 */  XK_space,       NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x21 */  XK_Page_Up,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x22 */  XK_Page_Down,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x23 */  XK_End,         NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x24 */  XK_Home,        NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x25 */  XK_Left,        NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x26 */  XK_Up,          NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x27 */  XK_Right,       NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x28 */  XK_Down,        NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x29 */  XK_Select,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x2a */  XK_Print,       NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x2b */  XK_Execute,	NoSymbol,       NoSymbol,	NoSymbol,
    /* 0x2c */  XK_Print,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x2d */  XK_Insert,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x2e */  XK_Delete,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x2f */  XK_Help,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x30 */  XK_0,           XK_parenright,	NoSymbol,	NoSymbol,
    /* 0x31 */  XK_1,           XK_exclam,	NoSymbol,	NoSymbol,
    /* 0x32 */  XK_2,           XK_at,  	NoSymbol,	NoSymbol,
    /* 0x33 */  XK_3,           XK_numbersign,	NoSymbol,	NoSymbol,
    /* 0x34 */  XK_4,           XK_dollar,	NoSymbol,	NoSymbol,
    /* 0x35 */  XK_5,           XK_percent,	NoSymbol,	NoSymbol,
    /* 0x36 */  XK_6,           XK_asciicircum,	NoSymbol,	NoSymbol,
    /* 0x37 */  XK_7,           XK_ampersand,	NoSymbol,	NoSymbol,
    /* 0x38 */  XK_8,	        XK_asterisk,	NoSymbol,	NoSymbol,
    /* 0x39 */  XK_9,           XK_parenleft,	NoSymbol,	NoSymbol,

    /* 0x3a through 0x40 undefined */
    /* 0x3a */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x3b */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x3c */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x3d */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x3e */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x3f */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x40 */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x41 */  XK_A,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x42 */  XK_B,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x43 */  XK_C,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x44 */  XK_D,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x45 */  XK_E,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x46 */  XK_F,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x47 */  XK_G,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x48 */  XK_H,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x49 */  XK_I,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x4a */  XK_J,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x4b */  XK_K,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x4c */  XK_L,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x4d */  XK_M,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x4e */  XK_N,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x4f */  XK_O,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x50 */  XK_P,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x51 */  XK_Q,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x52 */  XK_R,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x53 */  XK_S,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x54 */  XK_T,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x55 */  XK_U,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x56 */  XK_V,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x57 */  XK_W,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x58 */  XK_X,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x59 */  XK_Y,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x5a */  XK_Z,		NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x5b through 0x5f undefined */
    /* 0x5b */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x5c */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x5d */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x5e */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x5f */  NoSymbol,       NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x60 */  XK_KP_Insert,	XK_KP_0,	NoSymbol,	NoSymbol,
    /* 0x61 */  XK_KP_End,	XK_KP_1,	NoSymbol,	NoSymbol,
    /* 0x62 */  XK_KP_Down,	XK_KP_2,	NoSymbol,	NoSymbol,
    /* 0x63 */  XK_KP_Page_Down,XK_KP_3,	NoSymbol,	NoSymbol,
    /* 0x64 */  XK_KP_Left,	XK_KP_4,	NoSymbol,	NoSymbol,
    /* 0x65 */  NoSymbol,	XK_KP_5,	NoSymbol,	NoSymbol,
    /* 0x66 */  XK_KP_Right,	XK_KP_6,	NoSymbol,	NoSymbol,
    /* 0x67 */  XK_KP_Home,	XK_KP_7,	NoSymbol,	NoSymbol,
    /* 0x68 */  XK_KP_Up,	XK_KP_8,	NoSymbol,	NoSymbol,
    /* 0x69 */  XK_KP_Page_Up,	XK_KP_9,	NoSymbol,	NoSymbol,
    /* 0x6a */  XK_KP_Multiply,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x6b */  XK_KP_Add,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x6c */  XK_KP_Separator,NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x6d */  XK_KP_Subtract,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x6e */  XK_KP_Delete,	XK_KP_Decimal,	NoSymbol,	NoSymbol,
    /* 0x6f */  XK_KP_Divide,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x70 */  XK_F1,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x71 */  XK_F2,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x72 */  XK_F3,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x73 */  XK_F4,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x74 */  XK_F5,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x75 */  XK_F6,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x76 */  XK_F7,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x77 */  XK_F8,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x78 */  XK_F9,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x79 */  XK_F10,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x7a */  XK_F11,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x7b */  XK_F12,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x7c */  XK_F13,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x7d */  XK_F14,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x7e */  XK_F15,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x7f */  XK_F16,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x80 */  XK_F17,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x81 */  XK_F18,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x82 */  XK_F19,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x83 */  XK_F20,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x84 */  XK_F21,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x85 */  XK_F22,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x86 */  XK_F23,		NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x87 */  XK_F24,		NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x88 through 0x8f unassigned */
    /* 0x88 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x89 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x8a */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x8b */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x8c */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x8d */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x8e */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x8f */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x90 */  XK_Num_Lock,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x91 */  XK_Scroll_Lock,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x92 through 0x96 OEM specific */
    /* 0x92 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x93 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x94 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x95 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x96 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0x97 through 0x9f unassigned */
    /* 0x97 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x98 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x99 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x9a */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x9b */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x9c */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x9d */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x9e */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x9f */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xa0 */  XK_Shift_L,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xa1 */  XK_Shift_R,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xa2 */  XK_Control_L,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xa3 */  XK_Control_R,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xa4 */	XK_Alt_L,	XK_Meta_L,	NoSymbol,	NoSymbol,
    /* 0xa5 */	XK_Alt_R,	XK_Meta_R,	NoSymbol,	NoSymbol,

    /* 0xa6 through 0xb7 are MS Natural Keyboard Pro browser buttons, etc. */
    /* 0xa6 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xa7 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xa8 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xa9 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xaa */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xab */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xac */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xad */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xae */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xaf */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xb0 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xb1 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xb2 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xb3 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xb4 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xb5 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xb6 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xb7 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xb8 through 0xb9 reserved */
    /* 0xb8 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xb9 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xba */  XK_semicolon,	XK_colon,	NoSymbol,	NoSymbol,
    /* 0xbb */  XK_equal,	XK_plus,	NoSymbol,	NoSymbol,
    /* 0xbc */  XK_comma,	XK_less,	NoSymbol,	NoSymbol,
    /* 0xbd */  XK_minus,	XK_underscore,	NoSymbol,	NoSymbol,
    /* 0xbe */	XK_period,	XK_greater,	NoSymbol,	NoSymbol,
    /* 0xbf */	XK_slash,	XK_question,	NoSymbol,	NoSymbol,
    /* 0xc0 */	XK_quoteleft,	XK_asciitilde,	NoSymbol,	NoSymbol,
    
    /* 0xc1 through 0xd7 reserved */
    /* 0xc1 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xc2 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xc3 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xc4 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xc5 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xc6 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xc7 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xc8 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xc9 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xca */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xcb */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xcc */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xcd */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xce */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xcf */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xd0 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xd1 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xd2 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xd3 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xd4 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xd5 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xd6 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xd7 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xd8 through 0xda unassigned */
    /* 0xd8 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xd9 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xda */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xdb */	XK_bracketleft,	XK_braceleft,	NoSymbol,	NoSymbol,
    /* 0xdc */	XK_backslash,	XK_bar,		NoSymbol,	NoSymbol,
    /* 0xdd */	XK_bracketright,XK_braceright,	NoSymbol,	NoSymbol,
    /* 0xde */	XK_apostrophe,	XK_quotedbl,	NoSymbol,	NoSymbol,
    /* 0xdf */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xe0 reserved */
    /* 0xe0 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xe1 OEM specific */
    /* 0xe1 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xe2 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xe3 through 0xe4 OEM specific */
    /* 0xe3 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xe4 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xe5 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    
    /* 0xe6 OEM specific */
    /* 0xe6 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xe7 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xe8 unassigned */
    /* 0xe8 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xe9 through 0xf5 OEM specific */
    /* 0xe9 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xea */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xeb */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xec */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xed */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xef */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xf0 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xf1 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xf2 */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xf3 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xf4 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xf5 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* 0xf6 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xf7 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xf8 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xf9 */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xfa */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xfb */	NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xfc */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xfd */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xfe */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0xff */  NoSymbol,	NoSymbol,	NoSymbol,	NoSymbol,

    /* For X Server NumLock handling */
    /* 0x100 */ XK_KP_7,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x101 */ XK_KP_8,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x102 */ XK_KP_9,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x103 */ XK_KP_4,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x104 */ XK_KP_5,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x105 */ XK_KP_6,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x106 */ XK_KP_1,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x107 */ XK_KP_2,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x108 */ XK_KP_3,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x109 */ XK_KP_0,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x10a */ XK_KP_Decimal,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x10b */ XK_KP_Home,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x10c */ XK_KP_Up,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x10d */ XK_KP_Prior,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x10e */ XK_KP_Left,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x10f */ XK_KP_Begin,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x110 */ XK_KP_Right,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x111 */ XK_KP_End,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x112 */ XK_KP_Down,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x113 */ XK_KP_Next,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x114 */ XK_KP_Insert,	NoSymbol,	NoSymbol,	NoSymbol,
    /* 0x115 */ XK_KP_Delete,	NoSymbol,	NoSymbol,	NoSymbol
};
