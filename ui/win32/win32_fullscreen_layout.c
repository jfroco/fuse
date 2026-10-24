/* win32_fullscreen_layout.c: Integer-scaled full screen layout for Win32
   Copyright (c) 2026 jfroco

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#include "config.h"

#include "win32_fullscreen_layout.h"

int
win32_fullscreen_max_size( int area_width, int area_height,
                           int unit_width, int unit_height )
{
  int size, height_size;

  size = area_width / unit_width;
  height_size = area_height / unit_height;
  if( height_size < size ) size = height_size;

  return size < 1 ? 1 : size;
}

size_t
win32_fullscreen_best_size( int max_size, const int *sizes, size_t count )
{
  size_t i, best = 0;
  int best_total = -1;

  for( i = 0; i < count; i++ ) {
    int size = sizes[i];
    int total = size <= max_size ? size * ( max_size / size ) : 0;

    /* If nothing fits, the smallest size overflows the screen the least */
    if( total > best_total ||
        ( total == best_total &&
          ( total ? size > sizes[ best ] : size < sizes[ best ] ) ) ) {
      best = i;
      best_total = total;
    }
  }

  return best;
}

void
win32_fullscreen_get_layout( int area_width, int area_height,
                             int unit_width, int unit_height,
                             int scaler_size,
                             win32_fullscreen_layout *layout )
{
  int max_size;

  if( scaler_size < 1 ) scaler_size = 1;

  max_size = win32_fullscreen_max_size( area_width, area_height,
                                        unit_width, unit_height );

  layout->replicate = max_size / scaler_size;
  if( layout->replicate < 1 ) layout->replicate = 1;

  layout->width = unit_width * scaler_size * layout->replicate;
  layout->height = unit_height * scaler_size * layout->replicate;
  layout->x = ( area_width - layout->width ) / 2;
  layout->y = ( area_height - layout->height ) / 2;
}
