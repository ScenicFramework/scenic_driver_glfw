/*
#  Created by Boyd Multerer on 2/14/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#

Functions to facilitate messages coming up or down from the all via stdin
The caller will typically be erlang, so use the 2-byte length indicator
*/

#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <sys/select.h>

#include "comms.h"
#include "render_script.h"
#include "tx.h"
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
#define MSG_OUT_CACHE_MISS 0x20

#define MSG_OUT_FONT_MISS 0x22

#define MSG_OUT_NEW_TX_ID 0x31
#define MSG_OUT_NEW_FONT_ID 0x32

#define CMD_RENDER_GRAPH 0x01
#define CMD_CLEAR_GRAPH 0x02
#define CMD_SET_ROOT 0x03

#define CMD_CLEAR_COLOR 0x05

#define CMD_INPUT 0x0A

#define CMD_QUIT 0x20
#define CMD_QUERY_STATS 0x21
#define CMD_RESHAPE 0x22
#define CMD_POSITION 0x23
#define CMD_FOCUS 0x24
#define CMD_ICONIFY 0x25
#define CMD_MAXIMIZE 0x26
#define CMD_RESTORE 0x27
#define CMD_SHOW 0x28
#define CMD_HIDE 0x29

#define CMD_NEW_TX_ID 0x32
#define CMD_FREE_TX_ID 0x33
#define CMD_PUT_TX_BLOB 0x34
#define CMD_PUT_TX_RAW 0x35

#define CMD_LOAD_FONT_FILE 0X37
#define CMD_LOAD_FONT_BLOB 0X38
#define CMD_FREE_FONT 0X39

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

// https://stackoverflow.com/questions/2182002/convert-big-endian-to-little-endian-in-c-without-using-provided-func
#define SWAP_UINT16(x) (((x) >> 8) | ((x) << 8))
#define SWAP_UINT32(x)                                                         \
  (((x) >> 24) | (((x) &0x00FF0000) >> 8) | (((x) &0x0000FF00) << 8) |         \
   ((x) << 24))

static bool f_little_endian;

//=============================================================================
// raw comms with host app
// from erl_comm.c
// http://erlang.org/doc/tutorial/c_port.html#id64377

void test_endian()
{
  uint32_t i      = 0x01234567;
  f_little_endian = (*((uint8_t*) (&i))) == 0x67;
}

//---------------------------------------------------------
int read_exact(byte* buf, int len)
{
  int i, got = 0;

  do
  {
    if ((i = read(0, buf + got, len - got)) <= 0)
      return (i);
    got += i;
  } while (got < len);

  return (len);
}

//---------------------------------------------------------
int write_exact(byte* buf, int len)
{
  int i, wrote = 0;

  do
  {
    if ((i = write(1, buf + wrote, len - wrote)) <= 0)
      return (i);
    wrote += i;
  } while (wrote < len);

  return (len);
}

//---------------------------------------------------------
// the length indicator from erlang is always big-endian
int write_cmd(byte* buf, unsigned int len)
{
  int written = 0;

  // since this can be called from both the main and comms thread, need to
  // synchronize it
  // if ( pthread_rwlock_wrlock(&comms_out_lock) == 0 ) {
  uint32_t len_big = len;
  if (f_little_endian)
    len_big = SWAP_UINT32(len_big);
  write_exact((byte*) &len_big, sizeof(uint32_t));
  written = write_exact(buf, len);

  return written;
}

//---------------------------------------------------------
// Starts by using select to see if there is any data to be read
// if not in timeout, then returns with -1
// Setting the timeout too high means input will be laggy as you
// are starving the input polling. Setting it too low means using
// energy for no purpose. Probably best if set similar to the
// frame rate
int read_msg_length(struct timeval* ptv)
{
  byte buff[4];

  fd_set rfds;
  int    retval;

  // Watch stdin (fd 0) to see when it has input.
  FD_ZERO(&rfds);
  FD_SET(0, &rfds);

  // look for data
  retval = select(1, &rfds, NULL, NULL, ptv);
  if (retval == -1)
  {
    return -1; // error
  }
  else if (retval)
  {
    if (read_exact(buff, 4) != 4)
      return (-1);
    // length from erlang is always big endian
    uint32_t len = *((uint32_t*) &buff);
    if (f_little_endian)
      len = SWAP_UINT32(len);
    return len;
  }
  else
  {
    // no data within the timeout
    return -1;
  }
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

  if (f_little_endian)
    cmd_len = SWAP_UINT32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) msg, msg_len);
}

//---------------------------------------------------------
void send_write(const char* msg)
{
  uint32_t msg_len = strlen(msg);
  uint32_t cmd_len = msg_len + sizeof(uint32_t);
  uint32_t cmd     = MSG_OUT_WRITE;

  if (f_little_endian)
    cmd_len = SWAP_UINT32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) msg, msg_len);
}

//---------------------------------------------------------
void send_inspect(void* data, int length)
{
  uint32_t cmd_len = length + sizeof(uint32_t);
  uint32_t cmd     = MSG_OUT_INSPECT;

  if (f_little_endian)
    cmd_len = SWAP_UINT32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact(data, length);
}

//---------------------------------------------------------
void send_cache_miss(const char* key)
{
  uint32_t msg_len = strlen(key);
  uint32_t cmd_len = msg_len + sizeof(uint32_t);
  uint32_t cmd     = MSG_OUT_CACHE_MISS;

  if (f_little_endian)
    cmd_len = SWAP_UINT32(cmd_len);

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

  if (f_little_endian)
    cmd_len = SWAP_UINT32(cmd_len);

  write_exact((byte*) &cmd_len, sizeof(uint32_t));
  write_exact((byte*) &cmd, sizeof(uint32_t));
  write_exact((byte*) key, msg_len);
}

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
{
  uint32_t msg_id;
  uint32_t window_width;
  uint32_t window_height;
  uint32_t frame_width;
  uint32_t frame_height;
} msg_reshape_t;

void send_reshape(int window_width, int window_height, int frame_width,
                  int frame_height)
{
  msg_reshape_t msg = {MSG_OUT_RESHAPE, window_width, window_height,
                       frame_width, frame_height};
  write_cmd((byte*) &msg, sizeof(msg_reshape_t));
}

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
{
  uint32_t msg_id;
  uint32_t key;
  uint32_t scancode;
  uint32_t action;
  uint32_t mods;
} msg_key_t;

void send_key(int key, int scancode, int action, int mods)
{
  msg_key_t msg = {MSG_OUT_KEY, key, scancode, action, mods};
  write_cmd((byte*) &msg, sizeof(msg_key_t));
}

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
{
  uint32_t msg_id;
  uint32_t codepoint;
  uint32_t mods;
} msg_codepoint_t;

void send_codepoint(unsigned int codepoint, int mods)
{
  msg_codepoint_t msg = {MSG_OUT_CODEPOINT, codepoint, mods};
  write_cmd((byte*) &msg, sizeof(msg_codepoint_t));
}

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
{
  uint32_t msg_id;
  float    x;
  float    y;
} msg_cursor_pos_t;

void send_cursor_pos(float xpos, float ypos)
{
  msg_cursor_pos_t msg = {MSG_OUT_CURSOR_POS, xpos, ypos};
  write_cmd((byte*) &msg, sizeof(msg_cursor_pos_t));
}

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
{
  uint32_t msg_id;
  uint32_t button;
  uint32_t action;
  uint32_t mods;
  float    xpos;
  float    ypos;
} msg_mouse_button_t;

void send_mouse_button(int button, int action, int mods, float xpos, float ypos)
{
  msg_mouse_button_t msg = {
      MSG_OUT_MOUSE_BUTTON, button, action, mods, xpos, ypos};
  write_cmd((byte*) &msg, sizeof(msg_mouse_button_t));
}

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
{
  uint32_t msg_id;
  float    x_offset;
  float    y_offset;
  float    x;
  float    y;
} msg_scroll_t;

void send_scroll(float xoffset, float yoffset, float xpos, float ypos)
{
  msg_scroll_t msg = {MSG_OUT_MOUSE_SCROLL, xoffset, yoffset, xpos, ypos};
  write_cmd((byte*) &msg, sizeof(msg_scroll_t));
}

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
{
  uint32_t msg_id;
  int32_t  entered;
  float    x;
  float    y;
} msg_cursor_enter_t;

void send_cursor_enter(int entered, float xpos, float ypos)
{
  msg_cursor_enter_t msg = {MSG_OUT_CURSOR_ENTER, entered, xpos, ypos};
  write_cmd((byte*) &msg, sizeof(msg_cursor_enter_t));
}

//---------------------------------------------------------
void send_close()
{
  uint32_t msg = MSG_OUT_CLOSE;
  write_cmd((byte*) &msg, sizeof(uint32_t));
}

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
{
  uint32_t msg_id;
  int32_t  empty_dl;
} msg_ready_t;
void send_ready(int root_id)
{
  msg_ready_t msg = {MSG_OUT_READY, root_id};
  write_cmd((byte*) &msg, sizeof(msg_ready_t));
}

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
{
  uint32_t msg_id;
  uint32_t id;
} msg_draw_ready_t;

void send_draw_ready(unsigned int id)
{
  msg_ready_t msg = {MSG_OUT_DRAW_READY, id};
  write_cmd((byte*) &msg, sizeof(msg_draw_ready_t));
}

//=============================================================================
// incoming messages

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
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
} msg_stats_t;
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
typedef struct __attribute__((__packed__))
{
  int32_t x_w;
  int32_t y_h;
} cmd_move_t;
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
void receive_render(int* p_msg_length, GLFWwindow* window)
{
  window_data_t* p_data = glfwGetWindowUserPointer(window);
  if (p_data == NULL)
  {
    send_puts("receive_set_graph BAD WINDOW");
    return;
  }

  // get the draw list id to compile
  GLuint id;
  read_bytes_down(&id, sizeof(GLuint), p_msg_length);

  // extract the render script itself
  void* p_script = malloc(*p_msg_length);
  read_bytes_down(p_script, *p_msg_length, p_msg_length);

  // save the script away for later
  put_script(p_data, id, p_script);

  // send the signal that drawing is done
  send_draw_ready(id);

  // post a message to kick the display loop
  glfwPostEmptyEvent();
}

//---------------------------------------------------------
void receive_clear(int* p_msg_length, GLFWwindow* window)
{
  window_data_t* p_data = glfwGetWindowUserPointer(window);
  if (p_data == NULL)
  {
    send_puts("receive_set_graph BAD WINDOW");
    return;
  }

  // get and validate the dl_id
  GLuint id;
  read_bytes_down(&id, sizeof(GLuint), p_msg_length);

  // delete the list
  delete_script(p_data, id);
}

//---------------------------------------------------------
void receive_set_root(int* p_msg_length, GLFWwindow* window)
{
  window_data_t* p_data = glfwGetWindowUserPointer(window);
  if (p_data == NULL)
  {
    send_puts("receive_set_graph BAD WINDOW");
    return;
  }

  // get and validate the dl_id
  GLint id;
  read_bytes_down(&id, sizeof(GLint), p_msg_length);

  // update the current_dl with the incoming id
  p_data->root_script = id;

  // post a message to kick the display loop
  glfwPostEmptyEvent();
}

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
{
  GLuint r;
  GLuint g;
  GLuint b;
  GLuint a;
} clear_color_t;
void receive_clear_color(int* p_msg_length)
{
  // get the clear_color
  clear_color_t cc;
  read_bytes_down(&cc, sizeof(clear_color_t), p_msg_length);
  glClearColor(cc.r / 255.0, cc.g / 255.0, cc.b / 255.0, cc.a / 255.0);
}

//---------------------------------------------------------
typedef struct __attribute__((__packed__))
{
  GLuint name_length;
  GLuint data_length;
} font_info_t;

void receive_load_font_file(int* p_msg_length, GLFWwindow* window)
{
  window_data_t* p_data = glfwGetWindowUserPointer(window);
  if (p_data == NULL)
  {
    send_puts("receive_set_graph BAD WINDOW");
    return;
  }
  NVGcontext* p_ctx = p_data->context.p_ctx;

  font_info_t font_info;
  read_bytes_down(&font_info, sizeof(font_info_t), p_msg_length);

  // create the name and data
  void* p_name = malloc(font_info.name_length);
  read_bytes_down(p_name, font_info.name_length, p_msg_length);

  void* p_path = malloc(font_info.data_length);
  read_bytes_down(p_path, font_info.data_length, p_msg_length);

  // only load the font if it is not already loaded!
  if (nvgFindFont(p_ctx, p_name) < 0)
  {
    nvgCreateFont(p_ctx, p_name, p_path);
  }

  free(p_name);
  free(p_path);
}

//---------------------------------------------------------
void receive_load_font_blob(int* p_msg_length, GLFWwindow* window)
{
  window_data_t* p_data = glfwGetWindowUserPointer(window);
  if (p_data == NULL)
  {
    send_puts("receive_set_graph BAD WINDOW");
    return;
  }
  NVGcontext* p_ctx = p_data->context.p_ctx;

  font_info_t font_info;
  read_bytes_down(&font_info, sizeof(font_info_t), p_msg_length);

  // create the name and data
  void* p_name = malloc(font_info.name_length);
  read_bytes_down(p_name, font_info.name_length, p_msg_length);

  void* p_blob = malloc(font_info.data_length);
  read_bytes_down(p_blob, font_info.data_length, p_msg_length);

  // only load the font if it is not already loaded!
  if (nvgFindFont(p_ctx, p_name) < 0)
  {
    nvgCreateFontMem(p_ctx, p_name, p_blob, font_info.data_length, true);
  }

  free(p_name);
}

//---------------------------------------------------------
bool dispatch_message(int msg_length, GLFWwindow* window)
{

  bool render = false;

  // read the message id
  uint32_t msg_id;
  read_bytes_down(&msg_id, sizeof(uint32_t), &msg_length);

  char buff[200];

  check_gl_error("starting error: ");

  switch (msg_id)
  {
    case CMD_QUIT:
      receive_quit(window);
      return false;

    case CMD_RENDER_GRAPH:
      receive_render(&msg_length, window);
      render = true;
      break;
    case CMD_CLEAR_GRAPH:
      receive_clear(&msg_length, window);
      render = true;
      break;
    case CMD_SET_ROOT:
      receive_set_root(&msg_length, window);
      render = true;
      break;

    case CMD_CLEAR_COLOR:
      receive_clear_color(&msg_length);
      render = true;
      break;

    case CMD_INPUT:
      receive_input(&msg_length, window);
      break;

    case CMD_QUERY_STATS:
      receive_query_stats(window);
      break;
    case CMD_RESHAPE:
      receive_reshape(&msg_length, window);
      break;
    case CMD_POSITION:
      receive_position(&msg_length, window);
      break;

    case CMD_ICONIFY:
      glfwIconifyWindow(window);
      break;

    case CMD_RESTORE:
      glfwRestoreWindow(window);
      break;
    case CMD_SHOW:
      glfwShowWindow(window);
      break;
    case CMD_HIDE:
      glfwHideWindow(window);
      break;

    // font handling
    case CMD_LOAD_FONT_FILE:
      receive_load_font_file(&msg_length, window);
      render = true;
      break;
    case CMD_LOAD_FONT_BLOB:
      receive_load_font_blob(&msg_length, window);
      render = true;
      break;

    // the next two are in texture.c
    case CMD_PUT_TX_BLOB:
      receive_put_tx_blob(&msg_length, window);
      render = true;
      break;
    case CMD_FREE_TX_ID:
      receive_free_tx_id(&msg_length, window);
      break;

    case CMD_CRASH:
      receive_crash();
      break;

    default:
      sprintf(buff, "Unknown message: 0x%02X", msg_id);
      send_puts(buff);
  }

  // if there are any bytes left to read in the message, need to get rid of them
  // here...
  if (msg_length > 0)
  {
    sprintf(buff, "WARNING Excess message bytes! %d", msg_length);
    send_puts(buff);
    void* p = malloc(msg_length);
    read_bytes_down(p, msg_length, &msg_length);
    free(p);
  }

  check_gl_error(buff);

  return render;
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
bool handle_stdio_in(GLFWwindow* window)
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
    redraw = dispatch_message(len, window) || redraw;

    // see if time is remaining, so we can process another one
    time_remaining = end_time - get_time_stamp();
  }

  // return false to not cause a redraw
  return redraw;
}
