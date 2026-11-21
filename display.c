/* display.c: Routines for printing the Spectrum screen
   Copyright (c) 1999-2026 Philip Kendall, Thomas Harte, Witold Filipczyk
                           and Fredrick Meunier

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

   E-mail: philip-fuse@shadowmagic.org.uk

*/

#include "config.h"

#include <stddef.h>
#include <string.h>

#include "display.h"
#include "display_internal.h"
#include "infrastructure/startup_manager.h"
#include "machine.h"
#include "movie.h"
#include "peripherals/scld.h"
#include "rectangle.h"
#include "settings.h"
#include "ui/ui.h"
#include "ui/uidisplay.h"

/* Set once we have initialised the UI */
int display_ui_initialised = 0;

/* Stores the pixel, attribute and SCLD screen mode information used to
   draw each 8x1 group of pixels (including border) last frame */
libspectrum_dword
  display_last_screen[ DISPLAY_SCREEN_WIDTH_COLS * DISPLAY_SCREEN_HEIGHT ];

/* Force UI redraws independently of the image consumed by movie recording. */
libspectrum_byte
  display_force_redraw[ DISPLAY_SCREEN_WIDTH_COLS * DISPLAY_SCREEN_HEIGHT ];

/* Offsets as to where the data and the attributes for each pixel
   line start */
libspectrum_word display_line_start[ DISPLAY_HEIGHT ];
libspectrum_word display_attr_start[ DISPLAY_HEIGHT ];

/* Used to signify that we're redrawing the entire screen */
static int display_redraw_all;

int
display_init( int *argc, char ***argv )
{
  int i, j, k, y;
  int error;

  if( ui_init( argc, argv ) )
    return 1;

  display_dirty_init();

  for(i = 0; i < 3; i++)
    for(j = 0; j < 8; j++)
      for(k = 0; k < 8; k++)
        display_line_start[ ( 64 * i ) + ( 8 * j ) + k ] =
          32 * ( ( 64 * i ) + j + ( k * 8 ) );

  for(y = 0; y < DISPLAY_HEIGHT; y++) {
    display_attr_start[y] = DISPLAY_PIXEL_BYTES +
                            ( DISPLAY_WIDTH_COLS * ( y / 8 ) );
  }

  /* A defined blank image, also before the first frame has been rendered. */
  memset( display_last_screen, 0, sizeof( display_last_screen ) );
  display_render_init();

  display_refresh_all();

  error = display_border_init(); if( error ) return error;

  return 0;
}

#ifndef GEKKO
static int
display_init_wrapper( void *context )
{
  display_startup_context *typed_context =
    (display_startup_context *)context;

  return display_init( typed_context->argc, typed_context->argv );
}
#endif                          /* #ifndef GEKKO */

void
display_register_startup( display_startup_context *context )
{
  /* The Wii has an explicit call to display_init for now */
#ifndef GEKKO
  startup_manager_register_no_dependencies( STARTUP_MANAGER_MODULE_DISPLAY,
                                            display_init_wrapper, context,
                                            NULL );
#else                           /* #ifndef GEKKO */
  /* On the Wii, fuse_init() calls display_init() itself before anything
     else, so that messages can be shown on screen. The display module must
     still be registered, with no init function, because other modules
     (e.g. in fuse.c and the IDE peripherals) depend on it: without it the
     startup manager can never call them and Fuse fails to start with
     "N startup modules could not be called". */
  startup_manager_register_no_dependencies( STARTUP_MANAGER_MODULE_DISPLAY,
                                            NULL, NULL, NULL );
#endif                          /* #ifndef GEKKO */
}

/* Send the updated screen to the UI-specific code */
static void
update_ui_screen( void )
{
  static int frame_count = 0;
  int scale = machine_current->timex ? 2 : 1;
  size_t i;
  struct rectangle *ptr;

  if( settings_current.frame_rate <= ++frame_count ) {
    frame_count = 0;
    if( movie_recording ) {
      movie_start_frame();
    }

    if( display_redraw_all ) {
      if( movie_recording ) {
        movie_add_area( 0, 0, DISPLAY_ASPECT_WIDTH >> 3,
                        DISPLAY_SCREEN_HEIGHT );
      }
      uidisplay_area( 0, 0,
                      scale * DISPLAY_ASPECT_WIDTH,
                      scale * DISPLAY_SCREEN_HEIGHT );
      display_redraw_all = 0;
    } else {
      for( i = 0, ptr = rectangle_inactive;
           i < rectangle_inactive_count;
           i++, ptr++ ) {
        if( movie_recording ) {
          movie_add_area( ptr->x, ptr->y, ptr->w, ptr->h );
        }
        uidisplay_area( 8 * scale * ptr->x, scale * ptr->y,
                        8 * scale * ptr->w, scale * ptr->h );
      }
    }

    rectangle_inactive_count = 0;

    uidisplay_frame_end();
  }
}

int
display_frame( void )
{
  display_dirty_frame_begin();
  display_border_frame();
  display_dirty_frame_end();
  update_ui_screen();

  display_render_frame();

  return 0;
}

void
display_refresh_all( void )
{
  display_redraw_all = 1;

  display_refresh_main_screen();
  display_dirty_refresh_all();

  /* Readers such as FMF recording still need the last rendered image while
     the cache is being refreshed (notably at embedded RZX snapshots). */
  memset( display_force_redraw, 1, sizeof( display_force_redraw ) );
}

/* Fetch pixel (x, y). On a Timex this will be a point on a 640x480 canvas,
   on a Sinclair/Amstrad/Russian clone this will be a point on a 320x240
   canvas */
int
display_getpixel( int x, int y )
{
  libspectrum_byte ink, paper;
  libspectrum_byte data, data2;
  int mask = 1 << ( 7 - ( x % 8 ) );
  int index;

  if( machine_current->timex ) {
    int column = x >> 4;
    scld mode_data;

    y >>= 1;
    index = column + y * DISPLAY_SCREEN_WIDTH_COLS;

    data = display_last_screen[ index ] & 0xff;
    data2 = ( display_last_screen[ index ] & 0xff00 ) >> 8;
    mode_data.byte = ( display_last_screen[ index ] & 0xff0000 ) >> 16;

    if( mode_data.name.hires ) {
      if( x % 16 > 7 ) data = data2;
      display_parse_attr( hires_convert_dec( mode_data.byte ), &ink, &paper );
    } else {
      /* divide x by two to get the same value for adjacent pixels */
      mask = 1 << ( 7 - ( ( x >> 1 ) % 8 ) );
      display_parse_attr( data2, &ink, &paper );
    }
  } else {
    int column = x >> 3;

    index = column + y * DISPLAY_SCREEN_WIDTH_COLS;

    data = display_last_screen[ index ] & 0xff;
    data2 = ( display_last_screen[ index ] & 0xff00 ) >> 8;

    display_parse_attr( data2, &ink, &paper );
  }

  if( data & mask ) return ink;

  return paper;
}
