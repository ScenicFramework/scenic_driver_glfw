/*
#  Created by Boyd Multerer on 2/25/18.
#  Copyright © 2018 Kry10 Limited. All rights reserved.
#

Functions to load fonts and render text
*/

#include "comms.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
// #include <stdio.h>
// #include <string.h>

void check_gl_error() {
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
        send_puts( "GL_INVALID_ENUM" );
        break;
      case GL_INVALID_VALUE:
        send_puts( "GL_INVALID_VALUE" );
        break;
      case GL_INVALID_OPERATION:
        send_puts( "GL_INVALID_OPERATION" );
        break;
      case GL_OUT_OF_MEMORY:
        send_puts( "GL_OUT_OF_MEMORY" );
        break;
      case GL_STACK_UNDERFLOW:
        send_puts( "GL_STACK_UNDERFLOW" );
        break;
      case GL_STACK_OVERFLOW:
        send_puts( "GL_STACK_OVERFLOW" );
        break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:
        send_puts( "GL_INVALID_FRAMEBUFFER_OPERATION" );
        break;
      default:
        put_sn( "GL_OTHER:", err );
        break;
    }
  }

}

