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

#define HASH_ID(id) tommy_hash_u32( 0, id.p_data, id.size )


#define REPEAT_XY   (NVG_IMAGE_REPEATX | NVG_IMAGE_REPEATY)


tommy_hashlin   images = {0};



//---------------------------------------------------------
typedef struct _image_t {
  sid_t id;
  uint32_t nvg_id;
  uint32_t width;
  uint32_t height;
  uint32_t format;
  void* p_pixels;
  tommy_hashlin_node  node;
} image_t;


//---------------------------------------------------------
void init_images( void ) {
  // init the hash table
  tommy_hashlin_init( &images );
}


//=============================================================================
// internal utilities for working with the image id map
// isolate all knowledge of the hash table implementation to these functions

//---------------------------------------------------------
static int _comparator(const void* p_arg, const void* p_obj) {
  const sid_t* p_id = p_arg;
  const image_t* p_img = p_obj;
  return (p_id->size != p_img->id.size)
    || memcmp(p_id->p_data, p_img->id.p_data, p_id->size);
}


//---------------------------------------------------------
// image_t* get_image( uint32_t driver_id ) {
image_t* get_image( sid_t id ) {
  return tommy_hashlin_search(
    &images,
    _comparator,
    &id,
    HASH_ID( id ) 
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
void delete_image( sid_t id, NVGcontext* p_ctx ) {
  image_t* p_image = get_image( id );
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
void put_image( int* p_msg_length, NVGcontext* p_ctx ) {
  // read in the fixed size data
  uint32_t id_length;
  uint32_t blob_size;
  uint32_t width;
  uint32_t height;
  uint32_t format;
  read_bytes_down( &id_length, sizeof(uint32_t), p_msg_length );
  read_bytes_down( &blob_size, sizeof(uint32_t), p_msg_length );
  read_bytes_down( &width, sizeof(uint32_t), p_msg_length );
  read_bytes_down( &height, sizeof(uint32_t), p_msg_length );
  read_bytes_down( &format, sizeof(uint32_t), p_msg_length );

  // read the id into a temp buffer
  void* p_temp_id = calloc( 1, id_length + 1 );
  if ( !p_temp_id ) {
    send_puts( "Unable to allocate image p_temp_id" );
    return;
  };
  read_bytes_down( p_temp_id, id_length, p_msg_length );
  sid_t id;
  id.size = id_length;
  id.p_data = p_temp_id;


  // get the existing image record, if there is one
  image_t* p_image = get_image( id );


  // if the height or width have changed, we need to delete the existing record to reset it
  if ( p_image && (width != p_image->width || height != p_image->height || format != p_image->format )) {
    image_free( p_ctx, p_image );
    p_image = NULL;
  }

  // if there is no existing record, create a new one
  if ( !p_image ) {
    // initialize a record to hold the image
    int struct_size = ALIGN_UP(sizeof(image_t), 8);
    // the +1 is so the id is null terminated
    int id_size = id_length + 1;
    int alloc_size = struct_size + id_size;

    p_image = calloc( 1, alloc_size );
    if ( !p_image ) {
      send_puts( "Unable to allocate image struct" );
      free( p_temp_id );
      return;
    };
    p_image->width = width;
    p_image->height = height;
    p_image->format = format;

    // initialize the id
    p_image->id.size = id_length;
    p_image->id.p_data = ((void*)p_image) + struct_size;
    memcpy( p_image->id.p_data, p_temp_id, id_length );


    // get the image data in pixel format
    p_image->p_pixels = read_pixels( width, height, format, p_msg_length );
    if ( !p_image->p_pixels ) {
      free( p_image );
      free( p_temp_id );
      send_puts( "Unable to alloc img pixels" );
      return;
    }

    // create an nvg texture from the pixel data
    p_image->nvg_id = nvgCreateImageRGBA( p_ctx, width, height, REPEAT_XY, p_image->p_pixels );

    // save the image record into the tommyhash
    tommy_hashlin_insert( &images, &p_image->node, p_image, HASH_ID(p_image->id) );

  } else {
    // the image already exists and is the right size.
    // can save some bit of work by replacing the pixels of the existing image
    void* p_pixels = read_pixels( width, height, format, p_msg_length );
    if ( !p_pixels ) {
      free( p_temp_id );
      send_puts( "Unable to alloc img pixels" );
      return;
    }

    nvgUpdateImage( p_ctx, p_image->nvg_id, p_pixels );

    // done with the old pixel data.
    free( p_image->p_pixels );

    // save the new pixel data
    p_image->p_pixels = p_pixels;
  }

  free( p_temp_id );
}


//=============================================================================
// called when rendering scripts

//---------------------------------------------------------
void set_fill_image( NVGcontext* p_ctx, sid_t id ) {
  // get the mapped nvg_id for this image_id
  image_t* p_image = get_image( id );
  if ( !p_image ) return;

  // get the dimensions of the image
  int w,h;
  nvgImageSize(p_ctx, p_image->nvg_id, &w, &h);


  // the image is loaded and ready for use
  nvgFillPaint(p_ctx,
    nvgImagePattern( p_ctx, 0, 0, w, h, 0, p_image->nvg_id, 1.0 )
  );
}

//---------------------------------------------------------
void set_stroke_image( NVGcontext* p_ctx, sid_t id ) {
  // get the mapped nvg_id for this image_id
  image_t* p_image = get_image( id );
  if ( !p_image ) return;

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
void draw_image( NVGcontext* p_ctx, sid_t id,
GLfloat sx, GLfloat sy, GLfloat sw, GLfloat sh,
GLfloat dx, GLfloat dy, GLfloat dw, GLfloat dh) {
  GLfloat ax, ay;
  NVGpaint img_pattern;
  
  // get the mapped nvg_id for this driver_id
  image_t* p_image = get_image( id );
  if ( !p_image ) return;

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
