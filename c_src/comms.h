/*
#  Created by Boyd Multerer on 11/18/17.
#  Copyright © 2017 Kry10 Limited. All rights reserved.
#
*/

#pragma once

#ifndef bool
#include <stdbool.h>
#endif

#ifdef _MSC_VER
  #include "windows_comms.h"
#else
  #include "unix_comms.h"
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>


// ntoh means network-to-host endianness
// hton means host-to-network endianness
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
  #define ntoh_ui16(x) (x)
  #define ntoh_ui32(x) (x)
  #define ntoh_f32(x) (x)
  #define hton_ui16(x) (x)
  #define hton_ui32(x) (x)
  #define hton_f32(x) (x)
#else
// https://stackoverflow.com/questions/2182002/convert-big-endian-to-little-endian-in-c-without-using-provided-func
  #define ntoh_ui16(x) (((x) >> 8) | ((x) << 8))
  #define ntoh_ui32(x) \
    (((x) >> 24) | (((x) &0x00FF0000) >> 8) | (((x) &0x0000FF00) << 8) | ((x) << 24))
  static inline GLfloat ntoh_f32( GLfloat f ) {
    union {
      GLuint i;
      GLfloat f;
    } sw;
    sw.f = f;
    sw.i = ntoh_ui32(sw.i);
    return sw.f;
  }
  #define hton_ui16(x) (ntoh_ui16(x))
  #define hton_ui32(x) (ntoh_ui32(x))
  #define hton_f32(x) (ntoh_f32(x))
#endif


#define INPUT_KEY_MASK 0x0001
#define INPUT_CODEPOINT_MASK 0x0002
#define INPUT_CURSOR_POS_MASK 0x0004
#define INPUT_CURSOR_BUTTON_MASK 0x0008
#define INPUT_CURSOR_SCROLL_MASK 0x0010
#define INPUT_CURSOR_ENTER_MASK 0x0020



int read_exact(byte* buf, int len);
int write_exact(byte* buf, int len);
int read_msg_length(struct timeval * ptv);
bool isCallerDown();

bool read_bytes_down(void* p_buff, int bytes_to_read,
                     int* p_bytes_to_remaining);

// basic events to send up to the caller
void send_puts(const char* msg);
void send_write(const char* msg);
void send_inspect(void* data, int length);

void put_sp( const char* msg, void* p );
void put_sn( const char* msg, int n );
void put_sf( const char* msg, float f );


void send_image_miss( unsigned int img_id );

// void send_static_texture_miss(const char* key);
// void send_dynamic_texture_miss(const char* key);
// void send_font_miss(const char* key);
void send_reshape(int window_width, int window_height, int frame_width,
                  int frame_height);
void send_key(int key, int scancode, int action, int mods);
void send_codepoint(unsigned int codepoint, int mods);
void send_cursor_pos(float xpos, float ypos);
void send_mouse_button(int button, int action, int mods, float xpos,
                       float ypos);
void send_scroll(float xoffset, float yoffset, float xpos, float ypos);
void send_cursor_enter(int entered, float xpos, float ypos);
void send_close( int reason );
void send_ready();
// void send_draw_ready(unsigned int id);

void* comms_thread(void* window);

void handle_stdio_in(GLFWwindow* window);
