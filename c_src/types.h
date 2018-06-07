/*
#  Created by Boyd Multerer on 12/05/17.
#  Copyright © 2017 Kry10 Industries. All rights reserved.
#
*/

// one unified place for the various structures


#ifndef RENDER_GLFW_TYPES
#define RENDER_GLFW_TYPES

#ifndef bool
#include <stdbool.h>
#endif

#ifndef NANOVG_H
#include "nanovg/nanovg.h"
#endif

typedef unsigned char byte;

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
{
  float x;
  float y;
} Vector2f;


//---------------------------------------------------------
typedef struct {
  // pthread_rwlock_t  gl_lock;            // read-write lock for gl operations
  // GLuint            empty_dl;           // empty dl id
  int               window_width;
  int               window_height;
  int               frame_width;
  int               frame_height;
  Vector2f          frame_ratio;
  NVGcontext*       p_ctx;
  bool              glew_ok;
  void*             p_fonts;
} context_t;

//---------------------------------------------------------
// the data pointed to by the window private data pointer
typedef struct {
  bool              keep_going;
  uint32_t          input_flags;
  float             last_x;
  float             last_y;
  void**            p_scripts;
  int               root_script;
  int               num_scripts;
  void*             p_tx_ids;
  context_t         context;
} window_data_t;


#endif // RENDER_GLFW_TYPES