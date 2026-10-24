/* win32_fullscreen_layout.h: Integer-scaled full screen layout for Win32
   Copyright (c) 2026 jfroco

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#ifndef FUSE_WIN32_FULLSCREEN_LAYOUT_H
#define FUSE_WIN32_FULLSCREEN_LAYOUT_H

#include <stddef.h>

/* Sizes are in units of the 1x image (DISPLAY_ASPECT_WIDTH x
   DISPLAY_SCREEN_HEIGHT). The scaler produces an image of 'scaler_size'
   units, which is then replicated by an integer factor so every emulated
   pixel stays a whole number of screen pixels in both directions. */
typedef struct win32_fullscreen_layout {
  int replicate;		/* Integer pixel replication after the scaler */
  int width, height;		/* Size of the displayed image in pixels */
  int x, y;			/* Position of the image in the screen area;
				   negative if the image does not fit */
} win32_fullscreen_layout;

/* The largest integer size of the unit image which fits in the area,
   limited by whichever of the width or the height runs out first */
int win32_fullscreen_max_size( int area_width, int area_height,
                               int unit_width, int unit_height );

/* Index into 'sizes' of the scaler size which, replicated by an integer
   factor, covers the most of 'max_size'. Ties go to the larger scaler size,
   which keeps more of the filter's detail */
size_t win32_fullscreen_best_size( int max_size, const int *sizes,
                                   size_t count );

void win32_fullscreen_get_layout( int area_width, int area_height,
                                  int unit_width, int unit_height,
                                  int scaler_size,
                                  win32_fullscreen_layout *layout );

#endif			/* #ifndef FUSE_WIN32_FULLSCREEN_LAYOUT_H */
