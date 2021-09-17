/*
#  Created by Boyd Multerer on 2021-03-28
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#pragma once

#include "types.h"
#include "tommyds/src/tommyhashlin.h"



void init_images( void );
void put_image( int* p_msg_length, NVGcontext* p_ctx );
void reset_images(NVGcontext* p_ctx);

void set_fill_image( NVGcontext* p_ctx, sid_t id );
void set_stroke_image( NVGcontext* p_ctx, sid_t id );


void draw_image(
  NVGcontext* p_ctx, sid_t id,
  GLfloat sx, GLfloat sy, GLfloat sw, GLfloat sh,
  GLfloat dx, GLfloat dy, GLfloat dw, GLfloat dh
);