/*
#  Created by Boyd Multerer on 2/14/18.
#  Copyright © 2018 Kry10 Limited. All rights reserved.
#

Functions to facilitate messages coming up or down from the all via stdin
The caller will typically be erlang, so use the 2-byte length indicator
*/

#include "comms.h"

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// #include "render_script.h"
// #include "tx.h"
#include "script.h"
#include "image.h"
#include "types.h"
#include "utils.h"

#define MSG_OUT_CLOSE 0x00
#define MSG_OUT_STATS 0x01
#define MSG_OUT_PUTS 0x02
#define MSG_OUT_WRITE 0x03
#define MSG_OUT_INSPECT 0x04
#define MSG_OUT_RESHAPE 0x05
#define MSG_OUT_READY 0x06
#define MSG_OUT_DRAW_READY 0x07

#define MSG_OUT_KEY 0x0A
#define MSG_OUT_CODEPOINT 0x0B
#define MSG_OUT_CURSOR_POS 0x0C
#define MSG_OUT_MOUSE_BUTTON 0x0D
#define MSG_OUT_MOUSE_SCROLL 0x0E
#define MSG_OUT_CURSOR_ENTER 0x0F
#define MSG_OUT_DROP_PATHS 0x10
#define MSG_OUT_STATIC_TEXTURE_MISS 0x20
#define MSG_OUT_DYNAMIC_TEXTURE_MISS 0x21

#define MSG_OUT_FONT_MISS 0x22
#define MSG_IMG_MISS 0x23

#define MSG_OUT_NEW_TX_ID 0x31
#define MSG_OUT_NEW_FONT_ID 0x32



#define CMD_PUT_SCRIPT  0x01
#define CMD_DEL_SCRIPT  0x02
#define CMD_RESET       0x03

#define CMD_RENDER 0x06

#define CMD_INPUT 0x0A

#define CMD_QUIT 0x20

#define CMD_PUT_FONT 0x40
#define CMD_PUT_IMG 0x41


// #define CMD_QUERY_STATS 0x21
// #define CMD_RESHAPE 0x22
// #define CMD_POSITION 0x23
// #define CMD_FOCUS 0x24
// #define CMD_ICONIFY 0x25
// #define CMD_MAXIMIZE 0x26
// #define CMD_RESTORE 0x27
// #define CMD_SHOW 0x28
// #define CMD_HIDE 0x29

// #define CMD_NEW_TX_ID 0x32
// #define CMD_FREE_TX_ID 0x33
// #define CMD_PUT_TX_BLOB 0x34
// #define CMD_PUT_TX_RAW 0x35

// #define CMD_LOAD_FONT_FILE 0X37
// #define CMD_LOAD_FONT_BLOB 0X38
// #define CMD_FREE_FONT 0X39

// here to test recovery
#define CMD_CRASH 0xFE

// handy time definitions in microseconds
#define MILLISECONDS_8 8000
#define MILLISECONDS_16 16000
#define MILLISECONDS_20 20000
#define MILLISECONDS_32 32000
#define MILLISECONDS_64 64000
#define MILLISECONDS_128 128000

// Setting the timeout too high means input will be laggy as you
// are starving the input polling. Setting it too low means using
// energy for no purpose. Probably best if set similar to the
// frame rate of the application
#define STDIO_TIMEOUT MILLISECONDS_32


//=============================================================================
// raw comms with host app
// from erl_comm.c
// http://erlang.org/doc/tutorial/c_port.html#id64377

//---------------------------------------------------------
// the length indicator from erlang is always big-endian
int write_cmd(byte* buf, unsigned int len)
{
  int written = 0;

  // since this can be called from both the main and comms thread, need to
  // synchronize it
  // if ( pthread_rwlock_wrlock(&comms_out_lock) == 0 ) {
  uint32_t cmd_len = len;
  cmd_len = hton_ui32(cmd_len);
  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  written = write_exact(buf, len);

  return written;
}

//---------------------------------------------------------
bool read_bytes_down(void* p_buff, int bytes_to_read, int* p_bytes_to_remaining)
{
  if (p_bytes_to_remaining <= 0)
    return false;
  if (bytes_to_read > *p_bytes_to_remaining)
  {
    // read in the remaining bytes
    read_exact(p_buff, *p_bytes_to_remaining);
    *p_bytes_to_remaining = 0;
    // return false
    return false;
  }

  // read in the requested bytes
  read_exact(p_buff, bytes_to_read);
  // do accounting on the bytes remaining
  *p_bytes_to_remaining -= bytes_to_read;
  return true;
}

//=============================================================================
// send messages up to caller

//---------------------------------------------------------
void send_puts(const char* msg)
{
  uint32_t msg_len = strlen(msg);
  uint32_t cmd_len = msg_len + sizeof(uint32_t);
  uint32_t cmd     = MSG_OUT_PUTS;

  cmd_len = ntoh_ui32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) msg, msg_len);
}


//---------------------------------------------------------
void put_sp( const char* msg, void* p ) {
  char buff[400];
  sprintf(buff, "%s %p", msg, p);
  send_puts(buff);
}

//---------------------------------------------------------
void put_sn( const char* msg, int n ) {
  char buff[400];
  sprintf(buff, "%s %d", msg, n);
  send_puts(buff);
}

//---------------------------------------------------------
void put_sf( const char* msg, float f ) {
  char buff[400];
  sprintf(buff, "%s %f", msg, f);
  send_puts(buff);
}



//---------------------------------------------------------
void send_write(const char* msg)
{
  uint32_t msg_len = strlen(msg);
  uint32_t cmd_len = msg_len + sizeof(uint32_t);
  uint32_t cmd     = MSG_OUT_WRITE;

  cmd_len = ntoh_ui32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) msg, msg_len);
}

//---------------------------------------------------------
void send_inspect(void* data, int length)
{
  uint32_t cmd_len = length + sizeof(uint32_t);
  uint32_t cmd     = MSG_OUT_INSPECT;

  ntoh_ui32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact(data, length);
}

//---------------------------------------------------------
void send_static_texture_miss(const char* key)
{
  uint32_t msg_len = strlen(key);
  uint32_t cmd_len = msg_len + sizeof(uint32_t);
  uint32_t cmd     = MSG_OUT_STATIC_TEXTURE_MISS;

  ntoh_ui32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) key, msg_len);
}

//---------------------------------------------------------
void send_dynamic_texture_miss(const char* key)
{
  uint32_t msg_len = strlen(key);
  uint32_t cmd_len = msg_len + sizeof(uint32_t);
  uint32_t cmd     = MSG_OUT_DYNAMIC_TEXTURE_MISS;

  ntoh_ui32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) key, msg_len);
}

//---------------------------------------------------------
void send_font_miss(const char* key)
{
  uint32_t msg_len = strlen(key);
  uint32_t cmd_len = msg_len + sizeof(uint32_t);
  uint32_t cmd     = MSG_OUT_FONT_MISS;

  ntoh_ui32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) key, msg_len);
}

//---------------------------------------------------------
PACK(typedef struct msg_reshape_t
{
  uint32_t msg_id;
  uint32_t window_width;
  uint32_t window_height;
  uint32_t frame_width;
  uint32_t frame_height;
}) msg_reshape_t;

void send_reshape(int window_width, int window_height, int frame_width,
                  int frame_height)
{
  msg_reshape_t msg = {MSG_OUT_RESHAPE, window_width, window_height,
                       frame_width, frame_height};
  write_cmd((byte*) &msg, sizeof(msg_reshape_t));
}

//---------------------------------------------------------
PACK(typedef struct msg_key_t
{
  uint32_t msg_id;
  uint32_t key;
  uint32_t scancode;
  uint32_t action;
  uint32_t mods;
}) msg_key_t;

void send_key(int key, int scancode, int action, int mods)
{
  msg_key_t msg = {MSG_OUT_KEY, key, scancode, action, mods};
  write_cmd((byte*) &msg, sizeof(msg_key_t));
}

//---------------------------------------------------------
PACK(typedef struct msg_codepoint_t
{
  uint32_t msg_id;
  uint32_t codepoint;
  uint32_t mods;
}) msg_codepoint_t;

void send_codepoint(unsigned int codepoint, int mods)
{
  msg_codepoint_t msg = {MSG_OUT_CODEPOINT, codepoint, mods};
  write_cmd((byte*) &msg, sizeof(msg_codepoint_t));
}

//---------------------------------------------------------
PACK(typedef struct msg_cursor_pos_t
{
  uint32_t msg_id;
  float    x;
  float    y;
}) msg_cursor_pos_t;

void send_cursor_pos(float xpos, float ypos)
{
  msg_cursor_pos_t msg = {MSG_OUT_CURSOR_POS, xpos, ypos};
  write_cmd((byte*) &msg, sizeof(msg_cursor_pos_t));
}

//---------------------------------------------------------
PACK(typedef struct msg_mouse_button_t
{
  uint32_t msg_id;
  uint32_t button;
  uint32_t action;
  uint32_t mods;
  float    xpos;
  float    ypos;
}) msg_mouse_button_t;

void send_mouse_button(int button, int action, int mods, float xpos, float ypos)
{
  msg_mouse_button_t msg = {
    MSG_OUT_MOUSE_BUTTON,
    button,
    action,
    mods,
    xpos,
    ypos
  };
  write_cmd((byte*) &msg, sizeof(msg_mouse_button_t));
}
  // #define hton_ui16(x) (ntoh_ui16(x))
  // #define hton_ui32(x) (ntoh_ui32(x))
  // #define hton_f32(x) (ntoh_f32(x))

//---------------------------------------------------------
PACK(typedef struct msg_scroll_t
{
  uint32_t msg_id;
  float    x_offset;
  float    y_offset;
  float    x;
  float    y;
}) msg_scroll_t;

void send_scroll(float xoffset, float yoffset, float xpos, float ypos)
{
  msg_scroll_t msg = {MSG_OUT_MOUSE_SCROLL, xoffset, yoffset, xpos, ypos};
  write_cmd((byte*) &msg, sizeof(msg_scroll_t));
}

//---------------------------------------------------------
PACK(typedef struct msg_cursor_enter_t
{
  uint32_t msg_id;
  int32_t  entered;
  float    x;
  float    y;
}) msg_cursor_enter_t;

void send_cursor_enter(int entered, float xpos, float ypos)
{
  msg_cursor_enter_t msg = {MSG_OUT_CURSOR_ENTER, entered, xpos, ypos};
  write_cmd((byte*) &msg, sizeof(msg_cursor_enter_t));
}

//---------------------------------------------------------
PACK(typedef struct msg_close_t
{
  uint32_t msg_id;
  uint32_t reaspn;
}) msg_close_t;
void send_close( int reason )
{
  msg_close_t msg = { MSG_OUT_CLOSE, reason };
  write_cmd((byte*) &msg, sizeof(msg_close_t));
}

//---------------------------------------------------------
PACK(typedef struct img_miss_t
{
  uint32_t msg_id;
  uint32_t img_id;
}) img_miss_t;
void send_image_miss( unsigned int img_id )
{
  img_miss_t msg = { MSG_IMG_MISS, img_id };
  write_cmd((byte*) &msg, sizeof(img_miss_t));
}

//---------------------------------------------------------
void send_ready()
{
  uint32_t msg_id = MSG_OUT_READY;
  write_cmd((byte*) &msg_id, sizeof(msg_id));
}

// //---------------------------------------------------------
// PACK(typedef struct msg_draw_ready_t
// {
//   uint32_t msg_id;
//   uint32_t id;
// }) msg_draw_ready_t;

// void send_draw_ready(unsigned int id)
// {
//   msg_ready_t msg = {MSG_OUT_DRAW_READY, id};
//   write_cmd((byte*) &msg, sizeof(msg_draw_ready_t));
// }

//=============================================================================
// incoming messages

//---------------------------------------------------------
PACK(typedef struct msg_stats_t
{
  uint32_t msg_id;
  uint32_t input_flags;
  int      xpos;
  int      ypos;
  int      width;
  int      height;
  bool     focused;
  bool     resizable;
  bool     iconified;
  bool     maximized;
  bool     visible;
}) msg_stats_t;
void receive_query_stats(GLFWwindow* window)
{
  msg_stats_t    msg;
  int            a, b;
  window_data_t* p_window_data = glfwGetWindowUserPointer(window);

  msg.msg_id      = MSG_OUT_STATS;
  msg.input_flags = p_window_data->input_flags;

  // can't point into packed structure...
  glfwGetWindowPos(window, &a, &b);
  msg.xpos = a;
  msg.ypos = b;

  // can't point into packed structure...
  glfwGetWindowSize(window, &a, &b);
  msg.width  = a;
  msg.height = b;

  msg.focused   = glfwGetWindowAttrib(window, GLFW_FOCUSED);
  msg.resizable = glfwGetWindowAttrib(window, GLFW_RESIZABLE);
  msg.iconified = glfwGetWindowAttrib(window, GLFW_ICONIFIED);
  msg.maximized = false;
  msg.visible   = glfwGetWindowAttrib(window, GLFW_VISIBLE);

  write_cmd((byte*) &msg, sizeof(msg_stats_t));
}

//---------------------------------------------------------
void receive_input(int* p_msg_length, GLFWwindow* window)
{
  window_data_t* p_window_data = glfwGetWindowUserPointer(window);
  read_bytes_down(&p_window_data->input_flags, sizeof(uint32_t), p_msg_length);
}

//---------------------------------------------------------
PACK(typedef struct cmd_move_t
{
  int32_t x_w;
  int32_t y_h;
}) cmd_move_t;
void receive_reshape(int* p_msg_length, GLFWwindow* window)
{
  cmd_move_t move_data;
  if (read_bytes_down(&move_data, sizeof(cmd_move_t), p_msg_length))
  {
    // act on the data
    glfwSetWindowSize(window, move_data.x_w, move_data.y_h);
  }
}

//---------------------------------------------------------
void receive_position(int* p_msg_length, GLFWwindow* window)
{
  cmd_move_t move_data;
  if (read_bytes_down(&move_data, sizeof(cmd_move_t), p_msg_length))
  {
    // act on the data
    glfwSetWindowPos(window, move_data.x_w, move_data.y_h);
  }
}

//---------------------------------------------------------
void receive_quit(GLFWwindow* window)
{
  // clear the keep_going control flag, this ends the main thread loop
  window_data_t* p_window_data = glfwGetWindowUserPointer(window);
  p_window_data->keep_going    = false;
  // post an empty window event to trigger immediate quitting
  glfwPostEmptyEvent();
}

//---------------------------------------------------------
void receive_crash()
{
  send_puts("receive_crash - exit");
  exit(EXIT_FAILURE);
}


//---------------------------------------------------------
void render(GLFWwindow* window)
{
  window_data_t* p_data = glfwGetWindowUserPointer(window);

  // render the scene
  nvgBeginFrame(p_data->context.p_ctx, p_data->context.window_width,
                p_data->context.window_height,
                p_data->context.frame_ratio.x);

  // render the root script
  render_script( 0, p_data->context.p_ctx );

  // End frame and swap front and back buffers
  nvgEndFrame(p_data->context.p_ctx);
  glfwSwapBuffers(window);

  // all done
  send_ready();
}



//---------------------------------------------------------
void put_font(int* p_msg_length, NVGcontext* p_ctx)
{
  // read the size of the font name
  uint32_t name_bytes;
  read_bytes_down(&name_bytes, sizeof(uint32_t), p_msg_length);

  // allocate and copy in the font name
  char* p_name = malloc( name_bytes + 1 );
  if ( !p_name ) return;
  read_bytes_down(p_name, name_bytes, p_msg_length);
  // make sure the name is null terminated
  p_name[name_bytes] = 0;

  // allocate and read the font blob
  unsigned int blob_bytes = *p_msg_length;
  void* p_blob = malloc( blob_bytes );
  read_bytes_down( p_blob, blob_bytes, p_msg_length );

  // if the font is NOT loaded, then put it into nvg
  if (nvgFindFont(p_ctx, p_name) < 0) {
    nvgCreateFontMem(p_ctx, p_name, p_blob, blob_bytes, true);
  } else {
    free( p_blob );
  }

  free(p_name);
}


//---------------------------------------------------------
void dispatch_message( int msg_length, GLFWwindow* window )
{

  // read the message id
  uint32_t msg_id;
  read_bytes_down(&msg_id, sizeof(uint32_t), &msg_length);

  // put_sn( "dispatch_message:", msg_id );

  switch (msg_id)
  {
    case CMD_QUIT:
      receive_quit( window );
      return;

    case CMD_PUT_SCRIPT:
      put_script( &msg_length );
      break;

    case CMD_DEL_SCRIPT:
      delete_script( &msg_length );
      break;

    case CMD_RESET:
      reset_scripts();
      reset_images(((window_data_t*)glfwGetWindowUserPointer(window))->context.p_ctx);
      break;

    case CMD_RENDER:
      render( window );
      break;

    case CMD_INPUT:
      receive_input(&msg_length, window);
      break;

    case CMD_PUT_FONT:
      put_font(
        &msg_length,
        ((window_data_t*)glfwGetWindowUserPointer(window))->context.p_ctx
      );
      break;

    case CMD_PUT_IMG:
      put_image(
        &msg_length,
        ((window_data_t*)glfwGetWindowUserPointer(window))->context.p_ctx
      );
      break;

    case CMD_CRASH:
      receive_crash();
      break;

    default:
      put_sn( "Unknown message:", msg_id );
  }

  // if there are any bytes left to read in the message, need to get rid of them
  // here...
  if (msg_length > 0)
  {
    put_sn( "Excess message bytes:", msg_length );
    put_sn( "message id:", msg_id );
    void* p = malloc(msg_length);
    read_bytes_down(p, msg_length, &msg_length);
    free(p);
  }

  check_gl_error();

  return;
}

//=============================================================================
// non-threaded command reading

uint64_t get_time_stamp()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * (uint64_t) 1000000 + tv.tv_usec;
}

// read from the stdio in buffer and act on one message. Return true
// if we need to redraw the screen. false if we do not
void handle_stdio_in(GLFWwindow* window)
{
  int64_t        time_remaining = STDIO_TIMEOUT;
  int64_t        end_time       = get_time_stamp() + STDIO_TIMEOUT;
  struct timeval tv;
  bool           redraw = false;

  while (time_remaining > 0)
  {
    tv.tv_sec  = 0;
    tv.tv_usec = time_remaining;

    int len = read_msg_length(&tv);
    if (len <= 0)
      break;

    // process the message
    dispatch_message(len, window);

    // see if time is remaining, so we can process another one
    time_remaining = end_time - get_time_stamp();
  }

  return;
}
