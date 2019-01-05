/*
#  Created by Boyd Multerer on June 6, 2018.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#

Functions to load textures onto the graphics card
*/

#include <stdlib.h>

#include <stdio.h>

#include <GLFW/glfw3.h>
#include "nanovg/nanovg.h"
#include "types.h"
#include "comms.h"

#include "uthash.h"

#define MAX_KEY_LENGTH      64

//=============================================================================
// uthash setup

//---------------------------------------------------------
typedef struct
{
  const char*     key;
  int             id;
  UT_hash_handle  hh;
} tx_id_t;

//---------------------------------------------------------
static tx_id_t* put_tx_id(tx_id_t* p_tx_ids, char* p_key, int key_size, int id, int* old_id) {
  tx_id_t *found;

  // check if the key is already assigned.
  HASH_FIND_STR(p_tx_ids, p_key, found);
  if (found) {
    // pass out the old id.
    *old_id = found->id;
    // store the new id in the existing record
    found->id = id;
    // return
    return p_tx_ids;
  }

  // prepare a new id record
  unsigned int size = sizeof(tx_id_t) + key_size;
  tx_id_t* p_tx_id = malloc(size);
  memset(p_tx_id, 0, size );
  p_tx_id->id = id;
  p_tx_id->key = (void*)p_tx_id + sizeof(tx_id_t);
  memcpy((char*)p_tx_id->key, p_key, key_size);

  HASH_ADD_KEYPTR( hh, p_tx_ids, p_tx_id->key, strlen(p_tx_id->key), p_tx_id );

  return p_tx_ids;
}

//---------------------------------------------------------
int get_tx_id(void* p_tx_ids, char* p_key) {
  if (p_tx_ids == NULL) {return -1;}
  tx_id_t* found;
  HASH_FIND_STR( (tx_id_t*)p_tx_ids, p_key, found );
  if (found) return found->id;
  return -1;
}

//---------------------------------------------------------
// removes from the hash by id, and frees the pointer
static tx_id_t* delete_tx_id(tx_id_t* p_tx_ids, char* p_key) {
  if (p_tx_ids == NULL) {return NULL;}

  tx_id_t* found;
  HASH_FIND_STR( p_tx_ids, p_key, found );
  if (found != NULL) {
    HASH_DEL( p_tx_ids, found );
    free( found );
  }
  return p_tx_ids;
}

//=============================================================================

//---------------------------------------------------------
void receive_put_tx_blob( int* p_msg_length, GLFWwindow* window ) {
  window_data_t*  p_data = glfwGetWindowUserPointer( window );
  if ( p_data == NULL ) {
    send_puts( "receive_put_tx_file BAD WINDOW" );
    return;
  }
  NVGcontext* p_ctx = p_data->context.p_ctx;

  // read in the data from the stream
  GLuint key_size;
  GLuint file_size;
  read_bytes_down( &key_size, sizeof(GLuint), p_msg_length);
  read_bytes_down( &file_size, sizeof(GLuint), p_msg_length);

  // Allocate and read the key. Need to free from now on
  char* p_key = malloc(key_size);
  read_bytes_down( p_key, key_size, p_msg_length);

  // Allocate and read the main data. Need to free from now on
  void* p_tx_file = malloc(file_size);
  read_bytes_down( p_tx_file, file_size, p_msg_length);

  // load the texture
  int id = nvgCreateImageMem(p_ctx, NVG_IMAGE_GENERATE_MIPMAPS, p_tx_file, file_size);

  // store the key/id pair
  int old_id;
  p_data->p_tx_ids = put_tx_id( p_data->p_tx_ids, p_key, key_size, id, &old_id );

  free(p_key);
  free(p_tx_file);
}

//---------------------------------------------------------
void receive_free_tx_id( int* p_msg_length, GLFWwindow* window ) {
  window_data_t*  p_data = glfwGetWindowUserPointer( window );
  if ( p_data == NULL ) {
    send_puts( "receive_put_tx_file BAD WINDOW" );
    return;
  }
  NVGcontext* p_ctx = p_data->context.p_ctx;

  GLuint key_size;
  read_bytes_down( &key_size, sizeof(GLuint), p_msg_length);

  // Allocate and read the key. Need to free from now on
  char* p_key = malloc(key_size);
  read_bytes_down( p_key, key_size, p_msg_length);

  int id = get_tx_id(p_data->p_tx_ids, p_key);
  if (id >= 0) {
    p_data->p_tx_ids = delete_tx_id(p_data->p_tx_ids, p_key);
    nvgDeleteImage(p_ctx, id);
  }

  free(p_key);
}