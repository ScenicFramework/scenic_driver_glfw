/*
#  Created by Boyd Multerer on 2021-03-28
#  Copyright © 2021 Kry10 Limited. All rights reserved.
#
*/

#include <string.h>

// #include <GL/glew.h>
// #include <GLFW/glfw3.h>
#include "nanovg/stb_image.h"
// #include "nanovg/nanovg_gl.h"

#include "comms.h"

#include "types.h"
#include "utils.h"
#include "image.h"
#include "comms.h"

// #define HASH_ID(id)  tommy_inthash_u32(id)
#define HASH_ID(p_id)  tommy_hash_u32( 0, (void*)p_id, 32 )
// tommy_uint32_t tommy_hash_u32(tommy_uint32_t init_val, const void* void_key, tommy_size_t key_len);


#define REPEAT_XY   (NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY)


tommy_hashlin   images = {0};


//---------------------------------------------------------
void init_images( void ) {
  // init the hash table
  tommy_hashlin_init( &images );
}


//=============================================================================
// internal utilities for working with the image id map
// isolate all knowledge of the hash table implementation to these functions

//---------------------------------------------------------
static int _comparator(const void* arg, const void* obj) {
  // works because id is the first field in obj
  return memcmp( arg, obj, 32 );
  // return *(const uint32_t*)arg != *(const uint32_t*)obj;
  // return *(const uint32_t*)arg != ((const image_t*)obj)->driver_id;
}

//---------------------------------------------------------
// image_t* get_image( uint32_t driver_id ) {
image_t* get_image( const hash_id_t* p_id ) {
  return tommy_hashlin_search(
    &images,
    _comparator,
    p_id,
    HASH_ID( p_id ) 
  );
}

//---------------------------------------------------------
void image_free( NVGcontext* p_ctx, image_t* p_image ) {
  if ( p_image ) {
    tommy_hashlin_remove_existing( &images, &p_image->node );
    nvgDeleteImage(p_ctx, p_image->nvg_id);

    if ( p_image->p_pixels ) {
      free( p_image->p_pixels );
    }

    free( p_image );
  }
}

//---------------------------------------------------------
void delete_image( hash_id_t* p_id, NVGcontext* p_ctx ) {
  image_t* p_image = get_image( p_id );
  if ( p_image ) {
    image_free( p_ctx, p_image );
  }
}

//---------------------------------------------------------
void reset_images(NVGcontext* p_ctx) {
  // deallocates all the objects iterating the hashtable
  tommy_hashlin_foreach_arg(&images, (tommy_foreach_arg_func*)image_free, p_ctx);

  // deallocates the hashtable
  tommy_hashlin_done( &images );

  // re-init the hash table
  tommy_hashlin_init( &images );
}

//---------------------------------------------------------
void* read_pixels( uint32_t width, uint32_t height, uint32_t format_in, int* p_msg_length ) {
  // read incoming data into a temporary buffer
  int buffer_size = *p_msg_length;
  void* p_buffer = malloc(buffer_size);
  if ( !p_buffer ) return NULL;
  read_bytes_down( p_buffer, buffer_size, p_msg_length );

  void* p_pixels = NULL;
  GLuint pixel_count = width * height;
  GLuint src_i;
  GLuint dst_i;
  int x, y, comp;

  switch (format_in) {
    case 0: // encoded file format
      p_pixels = (void*)stbi_load_from_memory( p_buffer, buffer_size, &x, &y, &comp, 4);
      if ( p_pixels && (x != width || y != height) ) {
        send_puts("Image size mismatch!!");
        free(p_pixels);
        p_pixels = NULL;
      }
      break;

    case 1: // gray scale
      p_pixels = malloc(pixel_count * 4);
      for( unsigned int i = 0; i < pixel_count; i++ ) {
        dst_i = i * 4;
        ((char*)p_pixels)[dst_i] = ((char*)p_buffer)[i];
        ((char*)p_pixels)[dst_i + 1] = ((char*)p_buffer)[i];
        ((char*)p_pixels)[dst_i + 2] = ((char*)p_buffer)[i];
        ((char*)p_pixels)[dst_i + 3] = 0xff;
      }
      break;

    case 2: // gray + alpha
      p_pixels = malloc(pixel_count * 4);
      for( unsigned int i = 0; i < pixel_count; i++ ) {
        dst_i = i * 4;
        src_i = i * 2;
        ((char*)p_pixels)[dst_i] = ((char*)p_buffer)[src_i];
        ((char*)p_pixels)[dst_i + 1] = ((char*)p_buffer)[src_i];
        ((char*)p_pixels)[dst_i + 2] = ((char*)p_buffer)[src_i];
        ((char*)p_pixels)[dst_i + 3] = ((char*)p_buffer)[src_i + 1];
      }
      break;

    case 3: // rgb
      p_pixels = malloc(pixel_count * 4);
      for( unsigned int i = 0; i < pixel_count; i++ ) {
        dst_i = i * 4;
        src_i = i * 3;
        ((char*)p_pixels)[dst_i] = ((char*)p_buffer)[src_i];
        ((char*)p_pixels)[dst_i + 1] = ((char*)p_buffer)[src_i + 1];
        ((char*)p_pixels)[dst_i + 2] = ((char*)p_buffer)[src_i + 2];
        ((char*)p_pixels)[dst_i + 3] = 0xff;
      }
      break;

    case 4: // rgba
      p_pixels = p_buffer;
      p_buffer = NULL;
      break;
  }

  // clean up
  if ( p_buffer ) free(p_buffer);
  return p_pixels;
}

//---------------------------------------------------------
image_t* put_image( int* p_msg_length, NVGcontext* p_ctx ) {
  // read the driver_id of the script, which is in the first four bytes 
  // is also in native ordering

  hash_id_t id;
  uint32_t width;
  uint32_t height;
  uint32_t format;
  read_bytes_down( &id, 32, p_msg_length );
  read_bytes_down( &width, sizeof(uint32_t), p_msg_length );
  read_bytes_down( &height, sizeof(uint32_t), p_msg_length );
  read_bytes_down( &format, sizeof(uint32_t), p_msg_length );

  // get the existing image record, if there is one
  image_t* p_image = get_image( &id );

  // if the height or width have changed, we need to delete the existing record to reset it
  if ( p_image && (width != p_image->width || height != p_image->height )) {
    image_free( p_ctx, p_image );
    p_image = NULL;
  }

  // if there is no existing record, create a new one
  if ( !p_image ) {
    p_image = malloc( sizeof(image_t) );
    if (!p_image) return NULL;
    memcpy( &p_image->id, &id, 32 );
    // p_image->driver_id = driver_id;
    p_image->width = width;
    p_image->height = height;

    // get the image data in pixel format
    void* p_pixels = read_pixels( width, height, format, p_msg_length );
    if ( !p_pixels ) {
      free(p_image);
      return NULL;
    }

    // create an nvg texture from the pixel data
    p_image->nvg_id = nvgCreateImageRGBA( p_ctx, width, height, REPEAT_XY, p_pixels );

    // done with the pixel data. Safe to delete it now
    p_image->p_pixels = p_pixels;
    // free( p_pixels );

    // save the image record into the tommyhash
    tommy_hashlin_insert( &images, &p_image->node, p_image, HASH_ID(&id) );

  } else {
    // the image already exists and is the right size.
    // can save some bit of work by replacing the pixels of the existing image
    void* p_pixels = read_pixels( width, height, format, p_msg_length );
    if (!p_pixels) return NULL;

    nvgUpdateImage( p_ctx, p_image->nvg_id, p_pixels );

    // done with the pixel data. Safe to delete it now
    free( p_image->p_pixels );
    p_image->p_pixels = p_pixels;
    // free( p_pixels );
  }

  return p_image;
}


//=============================================================================
// called when rendering scripts

//---------------------------------------------------------
void set_fill_image( NVGcontext* p_ctx, const hash_id_t* p_id ) {
  // get the mapped nvg_id for this image_id
  image_t* p_image = get_image( p_id );
  if ( !p_image ) {
    // send_image_miss( image_id );
    return;
  }

  // get the dimensions of the image
  int w,h;
  nvgImageSize(p_ctx, p_image->nvg_id, &w, &h);


  // the image is loaded and ready for use
  nvgFillPaint(p_ctx,
    nvgImagePattern( p_ctx, 0, 0, w, h, 0, p_image->nvg_id, 1.0 )
  );
}

//---------------------------------------------------------
void set_stroke_image( NVGcontext* p_ctx, const hash_id_t* p_id ) {
  // get the mapped nvg_id for this image_id
  image_t* p_image = get_image( p_id );
  if ( !p_image ) {
    // send_image_miss( image_id );
    return;
  }

  // get the dimensions of the image
  int w,h;
  nvgImageSize(p_ctx, p_image->nvg_id, &w, &h);


  // the image is loaded and ready for use
  nvgStrokePaint(p_ctx,
    nvgImagePattern( p_ctx, 0, 0, w, h, 0, p_image->nvg_id, 1.0 )
  );
}



//---------------------------------------------------------
// see: https://github.com/memononen/nanovg/issues/348
void draw_image( NVGcontext* p_ctx, const hash_id_t* p_id,
GLfloat sx, GLfloat sy, GLfloat sw, GLfloat sh,
GLfloat dx, GLfloat dy, GLfloat dw, GLfloat dh) {
  GLfloat ax, ay;
  NVGpaint img_pattern;
  
  // get the mapped nvg_id for this driver_id
  image_t* p_image = get_image( p_id );
  if ( !p_image ) {
    // send_image_miss( p_id );
    return;
  }

  // get the dimensions of the image
  int iw,ih;
  nvgImageSize(p_ctx, p_image->nvg_id, &iw, &ih);

  // Aspect ration of pixel in x an y dimensions. This allows us to scale
  // the sprite to fill the whole rectangle.
  ax = dw / sw;
  ay = dh / sh;

  // create the temporary pattern
  img_pattern = nvgImagePattern(
    p_ctx,
    dx - sx*ax, dy - sy*ay, (float)iw*ax, (float)ih*ay,
    0, p_image->nvg_id, 1.0
  );

  // draw the image into a rect
  nvgBeginPath( p_ctx );
  nvgRect( p_ctx, dx, dy, dw, dh );
  nvgFillPaint( p_ctx, img_pattern );
  nvgFill( p_ctx );

  // the data for the paint pattern is a struct on the stack.
  // no need to clean it up
}










