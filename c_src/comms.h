/*
#  Created by Boyd Multerer on 11/18/17.
#  Copyright © 2017 Kry10 Industries. All rights reserved.
#
*/

#ifndef COMMS_HEADER
#define COMMS_HEADER

#ifndef bool
#include <stdbool.h>
#endif

bool read_bytes_down(void *p_buff, int bytes_to_read,
                     int *p_bytes_to_remaining);

// basic events to send up to the caller
void send_puts(const char *msg);
void send_write(const char *msg);
void send_inspect(void *data, int length);

void send_cache_miss(const char *key);
void send_font_miss(const char *key);
void send_reshape(int window_width, int window_height, int frame_width,
                  int frame_height);
void send_key(int key, int scancode, int action, int mods);
void send_codepoint(unsigned int codepoint, int mods);
void send_cursor_pos(float xpos, float ypos);
void send_mouse_button(int button, int action, int mods, float xpos,
                       float ypos);
void send_scroll(float xoffset, float yoffset, float xpos, float ypos);
void send_cursor_enter(int entered, float xpos, float ypos);
void send_close();
void send_ready(int root_id);
void send_draw_ready(unsigned int id);

void *comms_thread(void *window);

void test_endian();

bool handle_stdio_in(GLFWwindow *window);

#endif
