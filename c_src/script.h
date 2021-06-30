/*
#  Created by Boyd Multerer on 2021-03-09.
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#pragma once

#include "types.h"
#include "tommyds/src/tommyhashlin.h"

//---------------------------------------------------------
typedef struct _script_t {
  tommy_hashlin_node  node;
  uint32_t id;
  uint32_t size;
  void* p_data;
} script_t;



void init_scripts( void );

script_t* put_script( int* p_msg_length );
void delete_script( int* p_msg_length );

script_t* get_script( uint32_t id );
void reset_scripts();

void render_script( uint32_t id, NVGcontext* p_ctx );
