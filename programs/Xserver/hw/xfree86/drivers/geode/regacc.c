/* $XFree86$ */
/*
 * $Workfile: regacc.c $
 *
 * This is the main file used to add Durango graphics support to a software 
 * project.  The main reason to have a single file include the other files
 * is that it centralizes the location of the compiler options.  This file
 * should be tuned for a specific implementation, and then modified as needed
 * for new Durango releases.  The releases.txt file indicates any updates to
 * this main file, such as a new definition for a new hardware platform. 
 *
 * In other words, this file should be copied from the Durango source files
 * once when a software project starts, and then maintained as necessary.  
 * It should not be recopied with new versions of Durango unless the 
 * developer is willing to tune the file again for the specific project.
 *
 * NSC_LIC_ALTERNATIVE_PREAMBLE
 *
 * Revision 1.0
 *
 * National Semiconductor Alternative GPL-BSD License
 *
 * National Semiconductor Corporation licenses this software 
 * ("Software"):
 *
 *      Durango
 *
 * under one of the two following licenses, depending on how the 
 * Software is received by the Licensee.
 * 
 * If this Software is received as part of the Linux Framebuffer or
 * other GPL licensed software, then the GPL license designated 
 * NSC_LIC_GPL applies to this Software; in all other circumstances 
 * then the BSD-style license designated NSC_LIC_BSD shall apply.
 *
 * END_NSC_LIC_ALTERNATIVE_PREAMBLE */

/* NSC_LIC_BSD
 *
 * National Semiconductor Corporation Open Source License for Durango
 *
 * (BSD License with Export Notice)
 *
 * Copyright (c) 1999-2001
 * National Semiconductor Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 *   * Redistributions of source code must retain the above copyright 
 *     notice, this list of conditions and the following disclaimer. 
 *
 *   * Redistributions in binary form must reproduce the above 
 *     copyright notice, this list of conditions and the following 
 *     disclaimer in the documentation and/or other materials provided 
 *     with the distribution. 
 *
 *   * Neither the name of the National Semiconductor Corporation nor 
 *     the names of its contributors may be used to endorse or promote 
 *     products derived from this software without specific prior 
 *     written permission. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * NATIONAL SEMICONDUCTOR CORPORATION OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE,
 * INTELLECTUAL PROPERTY INFRINGEMENT, OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE.
 *
 * EXPORT LAWS: THIS LICENSE ADDS NO RESTRICTIONS TO THE EXPORT LAWS OF 
 * YOUR JURISDICTION. It is licensee's responsibility to comply with 
 * any export regulations applicable in licensee's jurisdiction. Under 
 * CURRENT (2001) U.S. export regulations this software 
 * is eligible for export from the U.S. and can be downloaded by or 
 * otherwise exported or reexported worldwide EXCEPT to U.S. embargoed 
 * destinations which include Cuba, Iraq, Libya, North Korea, Iran, 
 * Syria, Sudan, Afghanistan and any other country to which the U.S. 
 * has embargoed goods and services. 
 *
 * END_NSC_LIC_BSD */

/* NSC_LIC_GPL
 *
 * National Semiconductor Corporation Gnu General Public License for Durango
 *
 * (GPL License with Export Notice)
 *
 * Copyright (c) 1999-2001
 * National Semiconductor Corporation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted under the terms of the GNU General 
 * Public License as published by the Free Software Foundation; either 
 * version 2 of the License, or (at your option) any later version  
 *
 * In addition to the terms of the GNU General Public License, neither 
 * the name of the National Semiconductor Corporation nor the names of 
 * its contributors may be used to endorse or promote products derived 
 * from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS 
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE 
 * NATIONAL SEMICONDUCTOR CORPORATION OR CONTRIBUTORS BE LIABLE FOR ANY 
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER 
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE, 
 * INTELLECTUAL PROPERTY INFRINGEMENT, OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
 * OF SUCH DAMAGE. See the GNU General Public License for more details. 
 *
 * EXPORT LAWS: THIS LICENSE ADDS NO RESTRICTIONS TO THE EXPORT LAWS OF 
 * YOUR JURISDICTION. It is licensee's responsibility to comply with 
 * any export regulations applicable in licensee's jurisdiction. Under 
 * CURRENT (2001) U.S. export regulations this software 
 * is eligible for export from the U.S. and can be downloaded by or 
 * otherwise exported or reexported worldwide EXCEPT to U.S. embargoed 
 * destinations which include Cuba, Iraq, Libya, North Korea, Iran, 
 * Syria, Sudan, Afghanistan and any other country to which the U.S. 
 * has embargoed goods and services. 
 *
 * You should have received a copy of the GNU General Public License 
 * along with this file; if not, write to the Free Software Foundation, 
 * Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 *
 * END_NSC_LIC_GPL */

/* ROUTINES added accessing hardware reg */
void
gfx_write_reg_8(unsigned long offset, unsigned char value)
{
   WRITE_REG8(offset, value);
}

void
gfx_write_reg_16(unsigned long offset, unsigned short value)
{
   WRITE_REG16(offset, value);
}

void
gfx_write_reg_32(unsigned long offset, unsigned long value)
{
   WRITE_REG32(offset, value);
}
unsigned short
gfx_read_reg_16(unsigned long offset)
{
   unsigned short value;

   value = READ_REG16(offset);
   return value;
}
unsigned long
gfx_read_reg_32(unsigned long offset)
{
   unsigned long value;

   value = READ_REG32(offset);
   return value;
}

void
gfx_write_vid_32(unsigned long offset, unsigned long value)
{
   WRITE_VID32(offset, value);
}
unsigned long
gfx_read_vid_32(unsigned long offset)
{
   unsigned long value;

   value = READ_VID32(offset);
   return value;
}

/*Addition for the VIP code */
unsigned long
gfx_read_vip_32(unsigned long offset)
{
   unsigned long value;

   value = READ_VIP32(offset);
   return value;
}

void
gfx_write_vip_32(unsigned long offset, unsigned long value)
{
   WRITE_VIP32(offset, value);
}

unsigned int
GetVideoMemSize()
{
   unsigned int graphicsMemBaseAddr;
   unsigned int totalMem = 0;
   int i;
   unsigned int graphicsMemMask, graphicsMemShift;

   /* Read graphics base address. */

   graphicsMemBaseAddr = gfx_read_reg_32(0x8414);

   if (1) {
      unsigned int mcBankCfg = gfx_read_reg_32(0x8408);
      unsigned int dimmShift = 4;

      graphicsMemMask = 0x7FF;
      graphicsMemShift = 19;

      /* Calculate total memory size for GXm. */

      for (i = 0; i < 2; i++) {
	 if (((mcBankCfg >> dimmShift) & 0x7) != 0x7) {
	    switch ((mcBankCfg >> (dimmShift + 4)) & 0x7) {
	    case 0:
	       totalMem += 0x400000;
	       break;
	    case 1:
	       totalMem += 0x800000;
	       break;
	    case 2:
	       totalMem += 0x1000000;
	       break;
	    case 3:
	       totalMem += 0x2000000;
	       break;
	    case 4:
	       totalMem += 0x4000000;
	       break;
	    case 5:
	       totalMem += 0x8000000;
	       break;
	    case 6:
	       totalMem += 0x10000000;
	       break;
	    case 7:
	       totalMem += 0x20000000;
	       break;
	    default:
	       break;
	    }
	 }
	 dimmShift += 16;
      }
   } else {
      unsigned int mcMemCntrl1 = gfx_read_reg_32(0x8400);
      unsigned int bankSizeShift = 12;

      graphicsMemMask = 0x3FF;
      graphicsMemShift = 17;

      /* Calculate total memory size for GX. */

      for (i = 0; i < 4; i++) {
	 switch ((mcMemCntrl1 >> bankSizeShift) & 0x7) {
	 case 1:
	    totalMem += 0x200000;
	    break;
	 case 2:
	    totalMem += 0x400000;
	    break;
	 case 3:
	    totalMem += 0x800000;
	    break;
	 case 4:
	    totalMem += 0x1000000;
	    break;
	 case 5:
	    totalMem += 0x2000000;
	    break;
	 default:
	    break;
	 }
	 bankSizeShift += 3;
      }
   }

   /* Calculate graphics memory base address */

   graphicsMemBaseAddr &= graphicsMemMask;
   graphicsMemBaseAddr <<= graphicsMemShift;

   return (totalMem - graphicsMemBaseAddr);
}

/* END OF FILE */
