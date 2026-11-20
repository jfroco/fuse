/* wiikeyboard.c: routines for dealing with the Wii USB keyboard
   Copyright (c) 2008 Bjoern Giesler

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

   Author contact information:

   E-mail: bjoern@giesler.de

*/

/* The keyboard is read with libogc's libwiikeyboard (KEYBOARD_* API). The
   old /dev/usb/kbd IOS device used before does not exist in IOS58, the IOS
   the Homebrew Channel launches homebrew with. libwiikeyboard runs its own
   thread, handles keyboards being plugged in and out, and reports each key
   press and release with its USB HID usage code, the same codes the Wii
   keysyms table (wiikeysyms.h) is based on. */

#include "config.h"

#include <string.h>

#include "fuse.h"
#include "keyboard.h"
#include "ui/ui.h"
#include "wiikeyboard.h"

#include <gctypes.h>
#include <wiikeyboard/keyboard.h>

/* Key releases are posted one frame after they arrive, so that a key
   pressed and released between two frames is still seen by the emulated
   Spectrum */
#define QUEUELEN 100
static input_event_t releasequeue[QUEUELEN];
static int releasequeuepos = 0;

static int keyboard_available = 0;

/* USB HID usage codes of the modifier keys, which libwiikeyboard reports as
   ordinary key presses and releases */
static input_key
modifier_key( u8 keycode )
{
  switch( keycode ) {
  case 0xe0: return INPUT_KEY_Control_L;
  case 0xe1: return INPUT_KEY_Shift_L;
  case 0xe2: return INPUT_KEY_Alt_L;
  case 0xe3: return INPUT_KEY_Super_L;
  case 0xe4: return INPUT_KEY_Control_R;
  case 0xe5: return INPUT_KEY_Shift_R;
  case 0xe6: return INPUT_KEY_Alt_R;
  case 0xe7: return INPUT_KEY_Super_R;
  default:   return INPUT_KEY_NONE;
  }
}

int
wiikeyboard_init( void )
{
  s32 error;

  /* A USB keyboard is optional: Fuse can be used with the Wii Remote alone,
     so if the keyboard library cannot be started just warn and carry on
     instead of making Fuse fail to start. A keyboard does not need to be
     connected yet: libwiikeyboard picks it up whenever it is plugged in. */
  error = KEYBOARD_Init( NULL );
  if( error < 0 ) {
    ui_error( UI_ERROR_WARNING,
              "couldn't start USB keyboard support (error %d); "
              "continuing without it", (int)error );
    return 0;
  }

  keyboard_available = 1;
  return 0;
}

int
wiikeyboard_end( void )
{
  if( keyboard_available ) KEYBOARD_Deinit();
  keyboard_available = 0;
  return 0;
}

void
keyboard_update( void )
{
  keyboard_event event;
  input_event_t fuse_event;
  input_key key;
  int i;

  if( !keyboard_available ) return;

  for( i=0; i<releasequeuepos; i++ ) input_event(&(releasequeue[i]));
  releasequeuepos = 0;

  while( KEYBOARD_GetEvent( &event ) ) {

    if( event.type != KEYBOARD_PRESSED && event.type != KEYBOARD_RELEASED )
      continue;		/* keyboard connected or disconnected */

    key = modifier_key( event.keycode );
    if( key == INPUT_KEY_NONE ) key = keysyms_remap( event.keycode );
    if( key == INPUT_KEY_NONE ) continue;

    fuse_event.types.key.native_key = key;
    fuse_event.types.key.spectrum_key = key;

    if( event.type == KEYBOARD_PRESSED ) {
      fuse_event.type = INPUT_EVENT_KEYPRESS;
      input_event( &fuse_event );
    } else {
      fuse_event.type = INPUT_EVENT_KEYRELEASE;
      if( releasequeuepos < QUEUELEN ) {
        releasequeue[releasequeuepos++] = fuse_event;
      } else {
        input_event( &fuse_event );
      }
    }
  }
}
