/*
#  Created by Boyd Multerer on 2018-14-02
#  Rewritten by Boyd Multerer starting on 2021-24-02
#  Copyright © 2018-2021 Kry10 Limited. All rights reserved.
#
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#include <fcntl.h> //O_BINARY
#endif

#include "comms.h"
#include "script.h"
#include "image.h"
#include "font.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg/nanovg.h"
#include "nanovg/nanovg_gl.h"


// #include "render_script.h"
#include "types.h"
#include "utils.h"

#define STDIN_FILENO 0

//=============================================================================
// window callbacks

void errorcb(int error, const char* desc)
{
  put_sn( desc, error );
}

//---------------------------------------------------------
void reshape_framebuffer(GLFWwindow* window, int w, int h)
{
  window_data_t* p_data = glfwGetWindowUserPointer(window);

  p_data->context.frame_width  = w;
  p_data->context.frame_height = h;
}

//---------------------------------------------------------
void reshape_window(GLFWwindow* window, int w, int h)
{
  window_data_t* p_data = glfwGetWindowUserPointer(window);

  // calculate the framebuffer to window size ratios
  // this will be used for things like oversampling fonts
  p_data->context.window_width  = w;
  p_data->context.window_height = h;

  glfwGetFramebufferSize( window, &p_data->context.frame_width, &p_data->context.frame_height );


  p_data->context.frame_ratio.x = (float) p_data->context.frame_width /
                                  (float) p_data->context.window_width;
  p_data->context.frame_ratio.y = (float) p_data->context.frame_height /
                                  (float) p_data->context.window_height;

  glViewport( 0, 0, p_data->context.frame_width, p_data->context.frame_height );
  glClear( GL_COLOR_BUFFER_BIT );

  send_reshape(p_data->context.window_width, p_data->context.window_height, w, h);
}


//---------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action,
                  int mods)
{
  window_data_t* p_data = glfwGetWindowUserPointer(window);
  if (p_data->input_flags & INPUT_KEY_MASK)
  {
    send_key(key, scancode, action, mods);
  }
}

//---------------------------------------------------------
void charmods_callback(GLFWwindow* window, unsigned int codepoint, int mods)
{
  window_data_t* p_data = glfwGetWindowUserPointer(window);
  if (p_data->input_flags & INPUT_CODEPOINT_MASK)
  {
    send_codepoint(codepoint, mods);
  }
}

//---------------------------------------------------------
static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
  float          x, y;
  window_data_t* p_data = glfwGetWindowUserPointer(window);
  if (p_data->input_flags & INPUT_CURSOR_POS_MASK)
  {
    x = xpos;
    y = ypos;
    // only send the message if the postion changed
    if ((p_data->last_x != x) || (p_data->last_y != y))
    {
      send_cursor_pos(x, y);
      p_data->last_x = x;
      p_data->last_y = y;
    }
  }
}

//---------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  double         x, y;
  window_data_t* p_data = glfwGetWindowUserPointer(window);
  if (p_data->input_flags & INPUT_CURSOR_BUTTON_MASK)
  {
    glfwGetCursorPos(window, &x, &y);
    send_mouse_button(button, action, mods, x, y);
  }
}

//---------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  double         x, y;
  window_data_t* p_data = glfwGetWindowUserPointer(window);
  if (p_data->input_flags & INPUT_CURSOR_SCROLL_MASK)
  {
    glfwGetCursorPos(window, &x, &y);
    send_scroll(xoffset, yoffset, x, y);
  }
}
//---------------------------------------------------------
void cursor_enter_callback(GLFWwindow* window, int entered)
{
  double         x, y;
  window_data_t* p_data = glfwGetWindowUserPointer(window);
  if (p_data->input_flags & INPUT_CURSOR_ENTER_MASK)
  {
    glfwGetCursorPos(window, &x, &y);
    send_cursor_enter(entered, x, y);
  }
}

//---------------------------------------------------------
void window_close_callback(GLFWwindow* window)
{
// send_puts("window_close_callback");

  // let the calling app filter the close event. Send a message up.
  // if the app wants to let the window close, it needs to send a close back
  // down.
  send_close( 0 );
  glfwSetWindowShouldClose(window, false);
}

// //=============================================================================
// // main setup

//---------------------------------------------------------
// done before the window is created
void set_window_hints(const char* resizable)
{
  // is the window resizable
  glfwWindowHint(GLFW_RESIZABLE, strncmp(resizable, "true", 4) == 0);

  // claim the focus right on creation
  glfwWindowHint(GLFW_FOCUSED, true);

  // we want OpenGL 2.1
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
}

//---------------------------------------------------------
// set up one-time features of the window
void setup_window(GLFWwindow* window, int width, int height)
{
  window_data_t* p_data;


  // set up the window's private data
  p_data = malloc(sizeof(window_data_t));
  memset(p_data, 0, sizeof(window_data_t));
  // p_data->p_scripts = NULL;

  p_data->keep_going = true;

  p_data->input_flags = 0;

  // start position tracking with values that are obviously out of the window
  p_data->last_x      = -1.0f;
  p_data->last_y      = -1.0f;

  p_data->context.window_width  = width;
  p_data->context.window_height = height;
  p_data->context.frame_ratio.x = 1.0f;
  p_data->context.frame_ratio.y = 1.0f;

  p_data->context.glew_ok = false;

  glfwSetWindowUserPointer(window, p_data);

  // Make the window's context current
  glfwMakeContextCurrent(window);

  // initialize glew - do after setting up window and making current
  p_data->context.glew_ok = glewInit() == GLEW_OK;

  // get the actual framebuffer size to set it up
  int frame_width, frame_height;
  glfwGetFramebufferSize(window, &frame_width, &frame_height);
  reshape_framebuffer(window, frame_width, frame_height);

  // get the actual window size to set it up
  int window_width, window_height;
  glfwGetWindowSize(window, &window_width, &window_height);
  reshape_window(window, window_width, window_height);

  p_data->context.p_ctx = nvgCreateGL2(NVG_ANTIALIAS | NVG_DEBUG);
  // nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
  
  if (p_data->context.p_ctx == NULL)
  {
    send_puts("Could not init nanovg!!!");
    return;
  }

  // set up callbacks
  glfwSetFramebufferSizeCallback(window, reshape_framebuffer);
  glfwSetWindowSizeCallback(window, reshape_window);
  glfwSetKeyCallback(window, key_callback);
  glfwSetCharModsCallback(window, charmods_callback);
  glfwSetCursorPosCallback(window, cursor_pos_callback);
  glfwSetCursorEnterCallback(window, cursor_enter_callback);
  glfwSetMouseButtonCallback(window, mouse_button_callback);
  glfwSetScrollCallback(window, scroll_callback);
  glfwSetWindowCloseCallback(window, window_close_callback);

  // set the initial clear color
send_puts( "setup_window pre glClearColor" );
  glClearColor(0.0, 0.0, 0.0, 1.0);
}

//---------------------------------------------------------
// tear down one-time features of the window
void cleanup_window(GLFWwindow* window)
{
  // free the window's private data
  free(glfwGetWindowUserPointer(window));
}

//---------------------------------------------------------
int main(int argc, char** argv)
{
  GLFWwindow* window;
  GLuint      root_dl_id;

  // super simple arg check
  if (argc != 5) {
    printf("\r\nscenic_driver_glfw should be launched via the "
           "Scenic.Driver.Glfw library.\r\n\r\n");
    return 0;
  }
  // argv[1] is the width of the window
  int width = atoi(argv[1]);

  // argv[2] is the height of the window
  int height = atoi(argv[2]);

  /* Initialize the library */
  if (!glfwInit())
  {
    return -1;
  }
  glfwSetErrorCallback(errorcb);

  // init the hashtables
  init_scripts();
  init_fonts();
  init_images();

  // set the glfw window hints - done before window creation
  // argv[  3] is the resizable flag
  set_window_hints( argv[3] );

  /* Create a windowed mode window and its OpenGL context */
  // argv[4] is the window title
  window = glfwCreateWindow(width, height, argv[4], NULL, NULL);
  if (!window) {
    glfwTerminate();
    return -1;
  }

  // set up one-time features of the window
  setup_window( window, width, height );
  window_data_t* p_data = glfwGetWindowUserPointer(window);

#ifdef __APPLE__
  // heinous hack to get around macOS Mojave GL issues
  // without this, the window is blank until manually resized
  glfwPollEvents();
  int w, h;
  glfwGetWindowSize(window, &w, &h);
  glfwSetWindowSize(window, w++, h);
  glfwSetWindowSize(window, w, h);
#endif


#ifdef _MSC_VER
  _setmode(_fileno(stdin), O_BINARY);
  _setmode(_fileno(stdout), O_BINARY);
#endif

  // see if any errors happened during startup
  check_gl_error();

  // tell the app to start sending draw scripts
  send_ready();

  // Loop until the calling app closes the window
  while (p_data->keep_going && !isCallerDown())
  {
    // handle incoming messages - blocks with a timeout
    handle_stdio_in(window);
    // if (p_data->redraw || handle_stdio_in(window))
    // {
    //   p_data->redraw = false;

    //   // clear the buffer
    //   // glClear(GL_COLOR_BUFFER_BIT);

    //   // render the scene
    //   nvgBeginFrame(p_data->context.p_ctx, p_data->context.window_width,
    //                 p_data->context.window_height,
    //                 p_data->context.frame_ratio.x);

    //   // render the root script
    //   render_script( 0, p_data->context.p_ctx );

    //   // End frame and swap front and back buffers
    //   nvgEndFrame(p_data->context.p_ctx);
    //   glfwSwapBuffers(window);
    // }

    // poll for events and return immediately
    glfwPollEvents();
  }

  // clean up
  cleanup_window( window );
  glfwTerminate();

  return 0;
}


