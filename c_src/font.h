/*
#  Created by Boyd Multerer on 2021-09-16.
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#pragma once

#include "types.h"
#include "tommyds/src/tommyhashlin.h"

void init_fonts( void );
void put_font( int* p_msg_length, NVGcontext* p_ctx );
void set_font( sid_t id, NVGcontext* p_ctx );
