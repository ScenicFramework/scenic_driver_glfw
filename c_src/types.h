/*
#  Created by Boyd Multerer on 12/05/17.
#  Copyright © 2017 Kry10 Limited. All rights reserved.
#
*/

// one unified place for the various structures

#pragma once

#ifndef bool
#include <stdbool.h>
#endif

#include <stdint.h>

#ifndef NANOVG_H
#include "nanovg/nanovg.h"
#endif

#ifndef PACK
  #ifdef _MSC_VER
    #define PACK( __Declaration__ ) \
        __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop) )
  #elif defined(__GNUC__)
    #define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
  #endif
#endif

typedef unsigned char byte;

//---------------------------------------------------------
PACK(typedef struct Vector2f
{
  float x;
  float y;
}) Vector2f;

//---------------------------------------------------------
typedef struct
{
  int         window_width;
  int         window_height;
  int         frame_width;
  int         frame_height;
  Vector2f    frame_ratio;
  NVGcontext* p_ctx;
  bool        glew_ok;
  void*       p_fonts;
} context_t;

//---------------------------------------------------------
// the data pointed to by the window private data pointer
typedef struct
{
  bool      keep_going;
  bool      redraw;
  unsigned int input_flags;
  float     last_x;
  float     last_y;
  // void**    p_scripts;
  // int       root_script;
  // void*     p_tx_ids;
  context_t context;
} window_data_t;


//---------------------------------------------------------
// combination of a size and location. Do NOT assume the
// p_data can be free'd. It is usually in a larger block
// that was the thing that was allocated.
typedef struct _data_t {
  void* p_data;
  uint32_t size;
} data_t;

typedef data_t sid_t;
