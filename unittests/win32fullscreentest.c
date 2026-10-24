/* win32fullscreentest.c: Tests for the Win32 full screen layout helpers
   Copyright (c) 2026 jfroco

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
 */

#include <config.h>

#include <stdio.h>

#include "ui/win32/win32_fullscreen_layout.h"

#define UNIT_WIDTH 320
#define UNIT_HEIGHT 240

/* Scaler sizes for size 1x-4x of some families, as scaler_for_size() gives
   them: families without a 1x scaler repeat their 2x one */
static const int normal_sizes[] = { 1, 2, 3, 4 };
static const int hq_sizes[] = { 2, 2, 3, 4 };
static const int advmame_sizes[] = { 2, 2, 3, 3 };
static const int single_2x_sizes[] = { 2 };

#define COUNT( array ) ( sizeof( array ) / sizeof( array[0] ) )

static int
max_size_uses_the_limiting_direction( void )
{
  static const struct {
    int width, height, expected;
  } cases[] = {
    { 1366,  768, 3 },
    { 1920, 1080, 4 },		/* Height limits */
    { 1920, 1200, 5 },
    { 2560, 1440, 6 },
    { 3440, 1440, 6 },		/* Ultrawide: height limits */
    { 3840, 2160, 9 },
    { 1080, 1920, 3 },		/* Portrait: width limits */
    {  200,  100, 1 },		/* Smaller than 1x: clamp */
  };
  size_t i;

  for( i = 0; i < COUNT( cases ); i++ ) {
    int size = win32_fullscreen_max_size( cases[i].width, cases[i].height,
                                          UNIT_WIDTH, UNIT_HEIGHT );
    if( size != cases[i].expected ) {
      fprintf( stderr, "max size %dx%d: expected %d, got %d\n",
               cases[i].width, cases[i].height, cases[i].expected, size );
      return 1;
    }
  }

  return 0;
}

static int
check_best( int max_size, const int *sizes, size_t count, int expected,
            const char *family )
{
  int size = sizes[ win32_fullscreen_best_size( max_size, sizes, count ) ];

  if( size != expected ) {
    fprintf( stderr, "best %s size for %dx: expected %d, got %d\n",
             family, max_size, expected, size );
    return 1;
  }

  return 0;
}

static int
best_size_covers_most_of_the_screen( void )
{
  return
    check_best( 4, normal_sizes, COUNT( normal_sizes ), 4, "normal" ) ||
    /* 1x replicated 5 times beats 4x */
    check_best( 5, normal_sizes, COUNT( normal_sizes ), 1, "normal" ) ||
    check_best( 6, normal_sizes, COUNT( normal_sizes ), 3, "normal" ) ||
    check_best( 8, normal_sizes, COUNT( normal_sizes ), 4, "normal" ) ||
    check_best( 9, normal_sizes, COUNT( normal_sizes ), 3, "normal" ) ||
    /* No 1x HQ scaler: 4x covers as much as 2x and keeps more detail */
    check_best( 5, hq_sizes, COUNT( hq_sizes ), 4, "HQ" ) ||
    check_best( 6, hq_sizes, COUNT( hq_sizes ), 3, "HQ" ) ||
    check_best( 7, hq_sizes, COUNT( hq_sizes ), 3, "HQ" ) ||
    check_best( 9, hq_sizes, COUNT( hq_sizes ), 3, "HQ" ) ||
    check_best( 8, advmame_sizes, COUNT( advmame_sizes ), 2, "AdvMAME" ) ||
    check_best( 9, single_2x_sizes, COUNT( single_2x_sizes ), 2, "2xSaI" );
}

static int
best_size_when_nothing_fits_is_the_smallest( void )
{
  static const int sizes[] = { 4, 3, 2 };

  return check_best( 1, sizes, COUNT( sizes ), 2, "oversized" );
}

static int
check_layout( int width, int height, int scaler_size, int replicate,
              int image_width, int image_height, int x, int y )
{
  win32_fullscreen_layout layout;

  win32_fullscreen_get_layout( width, height, UNIT_WIDTH, UNIT_HEIGHT,
                               scaler_size, &layout );

  if( layout.replicate != replicate || layout.width != image_width ||
      layout.height != image_height || layout.x != x || layout.y != y ) {
    fprintf( stderr, "layout %dx%d at %dx: expected x%d %dx%d+%d+%d, "
             "got x%d %dx%d+%d+%d\n", width, height, scaler_size, replicate,
             image_width, image_height, x, y, layout.replicate, layout.width,
             layout.height, layout.x, layout.y );
    return 1;
  }

  return 0;
}

static int
layout_centres_the_replicated_image( void )
{
  return
    check_layout( 1920, 1080, 4, 1, 1280,  960, 320,  60 ) ||
    check_layout( 1920, 1200, 1, 5, 1600, 1200, 160,   0 ) ||
    check_layout( 2560, 1440, 3, 2, 1920, 1440, 320,   0 ) ||
    check_layout( 3840, 2160, 3, 3, 2880, 2160, 480,   0 ) ||
    check_layout( 1080, 1920, 3, 1,  960,  720,  60, 600 ) ||
    /* An explicitly chosen scaler that does not divide the maximum size */
    check_layout( 1920, 1200, 2, 2, 1280,  960, 320, 120 ) ||
    /* Too big for the screen: centred and cropped on both sides */
    check_layout(  600,  400, 2, 1,  640,  480, -20, -40 );
}

static const struct test_t {
  const char *name;
  int ( *fn )( void );
} tests[] = {
  { "max_size_uses_the_limiting_direction",
    max_size_uses_the_limiting_direction },
  { "best_size_covers_most_of_the_screen",
    best_size_covers_most_of_the_screen },
  { "best_size_when_nothing_fits_is_the_smallest",
    best_size_when_nothing_fits_is_the_smallest },
  { "layout_centres_the_replicated_image",
    layout_centres_the_replicated_image },
  { NULL, NULL }
};

int
main( void )
{
  const struct test_t *test;

  for( test = tests; test->fn; test++ ) {
    if( test->fn() ) {
      fprintf( stderr, "Test failed: %s\n", test->name );
      return 1;
    }
  }

  return 0;
}
