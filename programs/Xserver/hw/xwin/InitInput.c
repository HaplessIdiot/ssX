/* $TOG: InitInput.c /main/12 1998/02/10 13:23:52 kaleb $ */
/*

  Copyright 1993, 1998  The Open Group

  All Rights Reserved.

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.

  Except as contained in this notice, the name of The Open Group shall
  not be used in advertising or otherwise to promote the sale, use or
  other dealings in this Software without prior written authorization
  from The Open Group.

*/
/* $XFree86: xc/programs/Xserver/hw/xwin/InitInput.c,v 1.1 2000/08/10 17:40:37 dawes Exp $ */

#include "win.h"
#include "winkeymap.h"

/* Called from dix/devices.c */
/* All of our keys generate up and down transition notifications,
   so all of our keys can be used as modifiers.

   An example of a modifier is mapping the A key to the Control key.
   A has to be a legal modifier.  I think.
*/
Bool
LegalModifier (unsigned int uiKey, DevicePtr pDevice)
{
  return TRUE;
}

/* Called from dix/dispatch.c */
/* We tell mi to dequeue the events that we have sent it */
void
ProcessInputEvents (void)
{
  mieqProcessInputEvents ();
  miPointerUpdate ();
}

/* We call this function from winKeybdProc when we are
   initializing the keyboard.
*/
void
winGetKeyMappings (KeySymsPtr pKeySyms, CARD8 *pModMap)
{
  int			i;
  KeySym		*pKeySym = g_winKeySym;

  /* MAP_LENGTH is defined in Xserver/include/input.h to be 256 */
  for (i = 0; i < MAP_LENGTH; i++)
    {
      pModMap[i] = NoSymbol;  /* make sure it is restored */
    }

  /* Loop through all valid entries in the key symbol table */
  for (i = MIN_KEYCODE;
       i < (MIN_KEYCODE + NUM_KEYCODES);
       i++, pKeySym += GLYPHS_PER_KEY)
    {
      switch(*pKeySym)
	{
	case XK_Shift_L:
	case XK_Shift_R:
	  pModMap[i] = ShiftMask;
	  break;

	case XK_Control_L:
	case XK_Control_R:
	  pModMap[i] = ControlMask;
	  break;

	case XK_Caps_Lock:
	  pModMap[i] = LockMask;
	  break;

	case XK_Alt_L:
	case XK_Alt_R:
	  pModMap[i] = AltMask;
	  break;

	case XK_Num_Lock:
	  pModMap[i] = NumLockMask;
	  break;

	case XK_Scroll_Lock:
	  pModMap[i] = ScrollLockMask;
	  break;

	/* kana support */
	case XK_Kana_Lock:
	case XK_Kana_Shift:
	  pModMap[i] = KanaMask;
	  break;

	/* alternate toggle for multinational support */
	case XK_Mode_switch:
	  pModMap[i] = AltLangMask;
	  break;
	}
    }

  pKeySyms->map        = (KeySym*)g_winKeySym;
  pKeySyms->mapWidth   = GLYPHS_PER_KEY;
  pKeySyms->minKeyCode = MIN_KEYCODE;
  pKeySyms->maxKeyCode = MAX_STD_KEYCODE;
}

void
winKeybdBell (int iPercent, DeviceIntPtr pDeviceInt,
	      pointer pCtrl, int iClass)
{
}

void
winKeybdCtrl (DeviceIntPtr pDevice, KeybdCtrl *pCtrl)
{
}

/* See Porting Layer Definition - p. 18
 * This function is known as a DeviceProc.
 */
int
winKeybdProc (DeviceIntPtr pDeviceInt, int onoff)
{
  KeySymsRec		keySyms;
  CARD8 		modMap[MAP_LENGTH];
  DevicePtr		pDevice = (DevicePtr) pDeviceInt;

  switch (onoff)
    {
    case DEVICE_INIT: 
      winGetKeyMappings (&keySyms, modMap);
      InitKeyboardDeviceStruct (pDevice,
				&keySyms,
				modMap,
			        winKeybdBell,
			        winKeybdCtrl);
      break;
    case DEVICE_ON: 
      pDevice->on = TRUE;
      break;

    case DEVICE_CLOSE:
    case DEVICE_OFF: 
      pDevice->on = FALSE;
      break;
    }

  return Success;
}

void
winMouseCtrl (DeviceIntPtr pDevice, PtrCtrl *pCtrl)
{
}

/* See Porting Layer Definition - p. 18
 * This is known as a DeviceProc
 */
int
winMouseProc (DeviceIntPtr pDeviceInt, int onoff)
{
  CARD8			map[6];
  DevicePtr		pDevice = (DevicePtr) pDeviceInt;

  switch (onoff)
    {
    case DEVICE_INIT:
      map[1] = 1;
      map[2] = 2;
      map[3] = 3;
      map[4] = 4;
      map[5] = 5;
      InitPointerDeviceStruct (pDevice,
			       map,
			       5, /* Buttons 4 and 5 are mouse wheel events */
			       miPointerGetMotionEvents,
			       winMouseCtrl,
			       miPointerGetMotionBufferSize ());
      break;

    case DEVICE_ON:
      pDevice->on = TRUE;
      break;

    case DEVICE_CLOSE:
    case DEVICE_OFF:
      pDevice->on = FALSE;
      break;
    }
  return Success;
}

static
void
winInitializeModeKeyStates (void)
{
  xEvent	xCurrentEvent;

  /* Restore NumLock */
  if (GetKeyState (VK_NUMLOCK) & 0x0001)
    {
      xCurrentEvent.u.u.detail = MIN_KEYCODE + VK_NUMLOCK;

      /* Push the key */
      xCurrentEvent.u.u.type = KeyPress;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);

      /* Release the key */
      xCurrentEvent.u.u.type = KeyRelease;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
    }

  /* Restore CapsLock */
  if (GetKeyState (VK_CAPITAL) & 0x0001)
    {
      xCurrentEvent.u.u.detail = MIN_KEYCODE + VK_CAPITAL;

      /* Push the key */
      xCurrentEvent.u.u.type = KeyPress;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);

      /* Release the key */
      xCurrentEvent.u.u.type = KeyRelease;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
    }

  /* Restore ScrollLock */
  if (GetKeyState (VK_SCROLL) & 0x0001)
    {
      xCurrentEvent.u.u.detail = MIN_KEYCODE + VK_SCROLL;

      /* Push the key */
      xCurrentEvent.u.u.type = KeyPress;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);

      /* Release the key */
      xCurrentEvent.u.u.type = KeyRelease;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
    }

  /* Restore KanaLock */
  if (GetKeyState (VK_KANA) & 0x0001)
    {
      xCurrentEvent.u.u.detail = MIN_KEYCODE + VK_KANA;

      /* Push the key */
      xCurrentEvent.u.u.type = KeyPress;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);

      /* Release the key */
      xCurrentEvent.u.u.type = KeyRelease;
      xCurrentEvent.u.keyButtonPointer.time = GetTickCount ();
      mieqEnqueue (&xCurrentEvent);
    }
}

/* See Porting Layer Definition - p. 17 */
void
InitInput (int argc, char *argv[])
{
  DeviceIntPtr		pMouse, pKeyboard;
  
  ErrorF ("InitInput ()\n");

  pMouse = AddInputDevice (winMouseProc, TRUE);
  pKeyboard = AddInputDevice (winKeybdProc, TRUE);
  
  RegisterPointerDevice (pMouse);
  RegisterKeyboardDevice (pKeyboard);

  miRegisterPointerDevice (screenInfo.screens[0], pMouse);
  mieqInit ((DevicePtr)pKeyboard, (DevicePtr)pMouse);

  /* Initialize the mode key states */
  winInitializeModeKeyStates ();
}

#ifdef XTESTEXT1
void
XTestGenerateEvent (int dev_type, int keycode, int keystate,
		    int mousex, int mousey)
{
  ErrorF ("XTestGenerateEvent ()\n");
}

void
XTestGetPointerPos (short *fmousex, short *fmousey)
{
  ErrorF ("XTestGetPointerPos ()\n");
}

void
XTestJumpPointer (int jx, int jy, int dev_type)
{
  ErrorF ("XTestJumpPointer ()\n");
}
#endif

