/*
#  Created by Boyd Multerer on 2021-03-28
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#pragma once

#include "types.h"
#include "tommyds/src/tommyhashlin.h"

//---------------------------------------------------------
typedef struct _hash_id_t {
  byte id[32];
} hash_id_t;

//---------------------------------------------------------
typedef struct _image_t {
  // uint32_t driver_id;
  hash_id_t id;
  uint32_t nvg_id;
  uint32_t width;
  uint32_t height;
  void* p_pixels;
  tommy_hashlin_node  node;
} image_t;


void init_images( void );
image_t* put_image( int* p_msg_length, NVGcontext* p_ctx );
void reset_images(NVGcontext* p_ctx);

void set_fill_image( NVGcontext* p_ctx, const hash_id_t* p_id );
void set_stroke_image( NVGcontext* p_ctx, const hash_id_t* p_id );


void draw_image(
  NVGcontext* p_ctx, const hash_id_t* p_id,
  GLfloat sx, GLfloat sy, GLfloat sw, GLfloat sh,
  GLfloat dx, GLfloat dy, GLfloat dw, GLfloat dh
);