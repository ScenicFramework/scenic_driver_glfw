/*
#  Created by Boyd Multerer on 2/25/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#

Functions to load fonts and render text
*/

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>

#include "comms.h"

void check_gl_error(char* msg)
{
  char buff[400];
  char buff2[100];

  strncpy(buff, msg, 300);

  GLenum err;
  while (true)
  {
    err = glGetError();
    // check if there was a gl error
    switch (err)
    {
      case GL_NO_ERROR:
        return;
      case GL_INVALID_ENUM:
        strncat(buff, " GL_INVALID_ENUM", 399);
        break;
      case GL_INVALID_VALUE:
        strncat(buff, " GL_INVALID_VALUE", 399);
        break;
      case GL_INVALID_OPERATION:
        strncat(buff, " GL_INVALID_OPERATION", 399);
        break;
      case GL_OUT_OF_MEMORY:
        strncat(buff, " GL_OUT_OF_MEMORY", 399);
        break;
      case GL_STACK_UNDERFLOW:
        strncat(buff, " GL_STACK_UNDERFLOW", 399);
        break;
      case GL_STACK_OVERFLOW:
        strncat(buff, " GL_STACK_OVERFLOW", 399);
        break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        strncat(buff, " GL_INVALID_FRAMEBUFFER_OPERATION", 399);
        break;
      default:
        sprintf(buff2, " GL_OTHER: %d", err);
        strncat(buff, buff2, 399);
        break;
    }
    send_puts(buff);
  }
}
