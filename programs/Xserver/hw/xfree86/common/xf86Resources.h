/* $XFree86$ */

#ifndef _XF86_RESOURCES_H

#define _XF86_RESOURCES_H

#include "xf86str.h"

#define _END {ResEnd,0,0}

#if 1
#define _VGA_EXCLUSIVE {ResExcMemBlock,0xA0000,0xAFFFF},\
		      {ResExcMemBlock,0xB0000,0xB7FFF},\
		      {ResExcMemBlock,0xB8000,0xBFFFF},\
                      {ResExcIoBlock,0x3B0,0x3BB},\
                      {ResExcIoBlock,0x3C0,0x3DF}

#define _VGA_SHARED {ResShrMemBlock,0xA0000,0xAFFFF},\
		      {ResShrMemBlock,0xB0000,0xB7FFF},\
		      {ResShrMemBlock,0xB8000,0xBFFFF},\
                      {ResShrIoBlock,0x3B0,0x3BB},\
                      {ResShrIoBlock,0x3C0,0x3DF}

/* exclusive unused VGA: resource uneeded but cannot be disabled. */
/* like old Millennium                                              */
#define _VGA_EXCLUSIVE_UNUSED {ResExcUusdMemBlock,0xA0000,0xAFFFF},\
			       {ResExcUusdMemBlock,0xB0000,0xB7FFF},\
			       {ResExcUusdMemBlock,0xB8000,0xBFFFF},\
                               {ResExcUusdIoBlock,0x3B0,0x3BB},\
                               {ResExcUusdIoBlock,0x3C0,0x3DF}
/* shared unused VGA: resources unneeded but cannot be disabled */
/* independently. This is used to determine if a device needs rac */
#define _VGA_SHARED_UNUSED {ResShrUusdMemBlock,0xA0000,0xAFFFF},\
			       {ResShrUusdMemBlock,0xB0000,0xB7FFF},\
			       {ResShrUusdMemBlock,0xB8000,0xBFFFF},\
                               {ResShrUusdIoBlock,0x3B0,0x3BB},\
                               {ResShrUusdIoBlock,0x3C0,0x3DF}
#else
#define _VGA_EXCLUSIVE {ResExcMemBlock,0xA0000,0xAFFFF},\
		       {ResExcMemBlock,0xB0000,0xB7FFF},\
		       {ResExcMemBlock,0xB8000,0xBFFFF},\
                       {ResExcIoBlock,0x3B0,0x3DF}

#define _VGA_SHARED    {ResShrMemBlock,0xA0000,0xAFFFF},\
		       {ResShrMemBlock,0xB0000,0xB7FFF},\
		       {ResShrMemBlock,0xB8000,0xBFFFF},\
                       {ResShrIoBlock,0x3B0,0x3DF}

#define _VGA_EXCLUSIVE_UNUSED {ResExcUusdMemBlock,0xA0000,0xAFFFF},\
			       {ResExcUusdMemBlock,0xB0000,0xB7FFF},\
			       {ResExcUusdMemBlock,0xB8000,0xBFFFF},\
                               {ResExcUusdIoBlock,0x3B0,0x3DF}

#define _VGA_SHARED_UNUSED {ResShrUusdMemBlock,0xA0000,0xAFFFF},\
			       {ResShrUusdMemBlock,0xB0000,0xB7FFF},\
			       {ResShrUusdMemBlock,0xB8000,0xBFFFF},\
                               {ResShrUusdIoBlock,0x3B0,0x3DF}
#endif

#if 0
#define _8514_EXCLUSIVE {ResExcIoSparse,0x2E8,0x3FF},\
                        {ResExcIoSparse,0x2EA,0x3FF},\
                        {ResExcIoSparse,0x2EB,0x3FF},\
                        {ResExcIoSparse,0x2EC,0x3FC}

#define _8514_SHARED    {ResShrIoSparse,0x2E8,0x3FF},\
                        {ResShrIoSparse,0x2EA,0x3FF},\
                        {ResShrIoSparse,0x2EB,0x3FF},\
                        {ResShrIoSparse,0x2EC,0x3FC}
#else
#define _8514_EXCLUSIVE {ResExcIoSparse,0x2EC,0x3F8}

#define _8514_SHARED    {ResShrIoSparse,0x2EC,0x3F8}
#endif

extern resRange resVgaExclusive[];
extern resRange resVgaShared[];
extern resRange resVgaUnusedExclusive[];
extern resRange resVgaUnusedShared[];
extern resRange res8514Exclusive[];
extern resRange res8514Shared[];

#define RES_EXCLUSIVE_VGA   resVgaExclusive
#define RES_SHARED_VGA      resVgaShared
#define RES_EXCLUSIVE_8514  res8514Exclusive
#define RES_SHARED_8514     res8514Shared

#define _PCI_AVOID {ResExcIoSparse, 0x100, 0x300},\
                  {ResExcIoSparse, 0x200, 0x200}

extern resRange PciAvoid[];

#define RES_UNDEFINED NULL
#endif

