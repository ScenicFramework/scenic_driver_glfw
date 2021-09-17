/*
#  Created by Boyd Multerer on 2021-03-09.
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#include <string.h>

#include "types.h"
#include "utils.h"
#include "comms.h"
#include "font.h"

#define HASH_ID(id) tommy_hash_u32( 0, id.p_data, id.size )


//---------------------------------------------------------
typedef struct _font_t {
  int nvg_id;
  sid_t id;
  data_t blob;
  tommy_hashlin_node  node;
} font_t;

tommy_hashlin   fonts = {0};

//---------------------------------------------------------
void init_fonts( void ) {
  // init the hash table
  tommy_hashlin_init( &fonts );
}

//=============================================================================
// internal utilities for working with the script hash

// isolate all knowledge of the hash table implementation to these functions

//---------------------------------------------------------
static int _comparator(const void* p_arg, const void* p_obj) {
  const sid_t* p_id = p_arg;
  const font_t* p_font = p_obj;
  return (p_id->size != p_font->id.size)
    || memcmp(p_id->p_data, p_font->id.p_data, p_id->size);
}

//---------------------------------------------------------
font_t* get_font( sid_t id ) {
  return tommy_hashlin_search(
    &fonts,
    _comparator,
    &id,
    HASH_ID( id ) 
  );
}

//---------------------------------------------------------
void put_font( int* p_msg_length, NVGcontext* p_ctx ) {

  // read in the id size, which is in the first four bytes
  uint32_t id_length;
  read_bytes_down( &id_length, sizeof(uint32_t), p_msg_length );

  // read in the blob size, which is in the next four bytes
  uint32_t blob_size;
  read_bytes_down( &blob_size, sizeof(uint32_t), p_msg_length );

  // initialize a record to hold the font
  int struct_size = ALIGN_UP(sizeof(font_t), 8);
  // the +1 is so the id is null terminated
  int id_size = ALIGN_UP(id_length + 1, 8);
  int alloc_size = struct_size + id_size + blob_size;
  font_t* p_font = calloc( 1, alloc_size );
  if ( !p_font ) {
    send_puts( "Unable to allocate font font" );
    return;
  };

  // initialize the id
  p_font->id.size = id_length;
  p_font->id.p_data = ((void*)p_font) + struct_size;
  read_bytes_down( p_font->id.p_data, id_length, p_msg_length );

  // read the data into the blob buffer
  p_font->blob.size = id_length;
  p_font->blob.p_data = ((void*)p_font) + struct_size + id_size;
  read_bytes_down( p_font->blob.p_data, blob_size, p_msg_length );

  // if there is already is a font with the same id, abort
  if ( get_font(p_font->id) ) {
    free(p_font);
    return;
  }

  // create the nvg font
  p_font->nvg_id = nvgCreateFontMem(
    p_ctx, p_font->id.p_data, p_font->blob.p_data, blob_size, true
  );
  if ( p_font->nvg_id < 0 ) {
    send_puts( "Unable to create nvg font" );
    free(p_font);
    return;
  };

  // insert the script into the tommy hash
  tommy_hashlin_insert( &fonts, &p_font->node, p_font, HASH_ID(p_font->id) );
}

//---------------------------------------------------------
void set_font( sid_t id, NVGcontext* p_ctx ) {
  font_t* p_font = get_font( id );
  if (p_font) nvgFontFaceId( p_ctx, p_font->nvg_id );
}
