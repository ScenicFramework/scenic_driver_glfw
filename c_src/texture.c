/*
#  Created by Boyd Multerer on 2/18/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#

Functions to load textures onto the graphics card
*/

#ifdef _MSC_VER
#include "windows_utils.h"
#endif

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdlib.h> // malloc

#include "comms.h"
#include "texture.h"
#include "types.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_BMP
#define STBI_FAILURE_USERMSG
#include "stb_image.h"

GLenum translate_texture_format(int channels)
{
  switch (channels)
  {
    case 1:
      return GL_ALPHA;
    case 2:
      return GL_LUMINANCE_ALPHA;
    case 3:
      return GL_RGB;
    case 4:
      return GL_RGBA;
  }
  return -1;
}

//---------------------------------------------------------
void receive_put_tx_file(int* p_msg_length, GLFWwindow* window)
{
  window_data_t* p_window_data = glfwGetWindowUserPointer(window);
  if (p_window_data == NULL)
  {
    send_puts("receive_put_tx_file BAD WINDOW");
    return;
  }

  // read in the data from the stream
  GLuint tx_id;
  GLuint file_size;
  read_bytes_down(&tx_id, sizeof(GLuint), p_msg_length);
  read_bytes_down(&file_size, sizeof(GLuint), p_msg_length);

  // Allocate and read the main data. Need to free from now on
  void* p_tx_file = malloc(file_size);
  read_bytes_down(p_tx_file, file_size, p_msg_length);

  // get data about the image without parsing it first
  int x, y, comp;
  if (stbi_info_from_memory(p_tx_file, file_size, &x, &y, &comp) != 1)
  {
    send_puts("Unable to parse texture data");
    send_puts(stbi_failure_reason());
    free(p_tx_file);
    return;
  }

  // interpret the image data
  stbi_uc* p_img;
  int      actual_channels;
  p_img = stbi_load_from_memory(p_tx_file, file_size, &x, &y, &actual_channels,
                                comp);
  if (p_img == NULL)
  {
    send_puts("Unable to parse texture data");
    send_puts(stbi_failure_reason());
    free(p_tx_file);
    return;
  }

  // translate the channels into an internal format code
  GLint format = translate_texture_format(actual_channels);

  // set the texture onto the graphics card
  glBindTexture(GL_TEXTURE_2D, tx_id);

  // https://www.opengl.org/discussion_boards/showthread.php/140246-texture-not-showing
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  // the data is tightly packed
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

  // set the texture into place
  glTexImage2D(GL_TEXTURE_2D, 0, format, x, y, 0, format, GL_UNSIGNED_BYTE,
               p_img);

  if (p_window_data->context.glew_ok)
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, log2(x));
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  // check if there was a gl error
  if (glGetError() != GL_NO_ERROR)
  {
    send_puts("GL error while setting texture");
  }

  // cleanup
  stbi_image_free(p_img);
  free(p_tx_file);

  // post a message to kick the display loop
  glfwPostEmptyEvent();
}

//---------------------------------------------------------
void receive_put_tx_raw(int* p_msg_length, GLFWwindow* window)
{
  window_data_t* p_window_data = glfwGetWindowUserPointer(window);
  if (p_window_data == NULL)
  {
    send_puts("receive_put_tx_raw BAD WINDOW");
    return;
  }

  // read in the data from the stream
  GLuint tx_id;
  GLuint width;
  GLuint height;
  byte   channels;
  GLuint pixels_size;
  read_bytes_down(&tx_id, sizeof(tx_id), p_msg_length);
  read_bytes_down(&width, sizeof(width), p_msg_length);
  read_bytes_down(&height, sizeof(height), p_msg_length);
  read_bytes_down(&channels, sizeof(channels), p_msg_length);
  read_bytes_down(&pixels_size, sizeof(pixels_size), p_msg_length);
  GLenum format = translate_texture_format(channels);

  // sanity check the incoming data
  if (width * height * channels != pixels_size)
  {
    send_puts("receive_put_tx_raw INVALID PIXEL SIZE!");
    return;
  }

  // Allocate and read the pixel data. Need to free from now on
  void* p_tx_pixels = malloc(pixels_size);
  read_bytes_down(p_tx_pixels, pixels_size, p_msg_length);

  // set the texture onto the graphics card

  glBindTexture(GL_TEXTURE_2D, tx_id);

  // https://www.opengl.org/discussion_boards/showthread.php/140246-texture-not-showing
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

  // the data is tightly packed
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
  glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

  // set the texture into place
  glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format,
               GL_UNSIGNED_BYTE, p_tx_pixels);

  if (p_window_data->context.glew_ok)
  {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, log2(width));
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  glBindTexture(GL_TEXTURE_2D, 0);

  // cleanup
  free(p_tx_pixels);

  // post a message to kick the display loop
  glfwPostEmptyEvent();
}
