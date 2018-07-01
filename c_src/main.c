/*
#  Created by Boyd Multerer on 02/14/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <poll.h>
// #include <pthread.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define NANOVG_GL2_IMPLEMENTATION
#include "nanovg/nanovg.h"
#include "nanovg/nanovg_gl.h"

#include "types.h"
#include "comms.h"
#include "utils.h"
#include "render_script.h"


#define STDIN_FILENO 0


#define   MSG_KEY_MASK              0x0001
#define   MSG_CHAR_MASK             0x0002
#define   MSG_MOUSE_MOVE_MASK       0x0004
#define   MSG_MOUSE_BUTTON_MASK     0x0008
#define   MSG_MOUSE_SCROLL_MASK     0x0010
#define   MSG_MOUSE_ENTER_MASK      0x0020
#define   MSG_DROP_PATHS_MASK       0x0040
#define   MSG_RESHAPE_MASK          0x0080



//=============================================================================
// window callbacks

void errorcb(int error, const char* desc)
{
  char buff[200];
  sprintf(buff, "GLFW error %d: %s\n", error, desc);
  send_puts(buff);
}

void render_frame() {

}

//---------------------------------------------------------
void reshape_framebuffer(GLFWwindow* window, int w, int h) {
  window_data_t*  p_data = glfwGetWindowUserPointer( window );

 // char buff[200];
 // sprintf(buff, "reshape_framebuffer, w: %d, h: %d", w, h);
 // send_puts(buff);


  // calculate the framebuffer to window size ratios
  // this will be used for things like oversampling fonts
  p_data->context.frame_width = w;
  p_data->context.frame_height = h;

  send_reshape(
    p_data->context.window_width, p_data->context.window_height,
    w, h
  );

  // if ( pthread_rwlock_rdlock(&p_data->context.gl_lock) == 0 ) {
    p_data->context.frame_ratio.x = p_data->context.frame_width / p_data->context.window_width;
    p_data->context.frame_ratio.y = p_data->context.frame_height / p_data->context.window_height;

    // set the view port to the new size passed in
    glViewport(0, 0, w, h);

    // set the 2D Projection matrix
    // Preserve Aspect Ratio
    // This means changing this size will
    // alter the position and size but NOT
    // the actual look of the scene.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho( 0,
      p_data->context.window_width,
      p_data->context.window_height,
      0, -1.0, 1.0
    );

    // set the modelView to Identity
    // since our UI will already be in viewport space
    // for now!
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    p_data->redraw = true;

  //   pthread_rwlock_unlock(&p_data->context.gl_lock);
  // }
}

//---------------------------------------------------------
void reshape_window( GLFWwindow* window, int w, int h) {
  window_data_t*  p_data = glfwGetWindowUserPointer( window );

 // char buff[200];
 // sprintf(buff, "reshape_window, w: %d, h: %d", w, h);
 // send_puts(buff);

  // calculate the framebuffer to window size ratios
  // this will be used for things like oversampling fonts
  p_data->context.window_width = w;
  p_data->context.window_height = h;

  // if ( pthread_rwlock_rdlock(&p_data->context.gl_lock) == 0 ) {
  p_data->context.frame_ratio.x = p_data->context.frame_width / p_data->context.window_width;
  p_data->context.frame_ratio.y = p_data->context.frame_height / p_data->context.window_height;

    // // Render the scene
    // glClear(GL_COLOR_BUFFER_BIT);
    // glCallList( p_data->context.root_dl );
    // // Swap front and back buffers
    // glfwSwapBuffers(window);

    // // clear the buffer
    // glClear(GL_COLOR_BUFFER_BIT);
    // // render the scene
    // nvgBeginFrame(
    //   p_data->context.p_ctx,
    //   p_data->context.frame_width,
    //   p_data->context.frame_height,
    //   1.0f
    // );

    // if ( p_data->root_script >= 0 ) {
    //   run_script( p_data->root_script, p_data );
    // }
    // nvgEndFrame(p_data->context.p_ctx);
    // // Swap front and back buffers
    // glfwSwapBuffers(window);


  //   pthread_rwlock_unlock(&p_data->context.gl_lock);
  // };

  p_data->redraw = true;

  glfwPostEmptyEvent();
}


//---------------------------------------------------------
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
  window_data_t*  p_data = glfwGetWindowUserPointer( window );
  if ( p_data->input_flags & MSG_KEY_MASK ) {
    send_key( key, scancode, action, mods );
  }
}

//---------------------------------------------------------
void charmods_callback(GLFWwindow* window, unsigned int codepoint, int mods)
{
  window_data_t*  p_data = glfwGetWindowUserPointer( window );
  if ( p_data->input_flags & MSG_CHAR_MASK ) {
    send_codepoint( codepoint, mods );
  }
}

//---------------------------------------------------------
static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
  float x, y;
  window_data_t*  p_data = glfwGetWindowUserPointer( window );
  if ( p_data->input_flags & MSG_MOUSE_MOVE_MASK  || true) {
    x = xpos;
    y = ypos;
    // only send the message if the postion changed
    if ( (p_data->last_x != x) || (p_data->last_y != y) ) {
      send_cursor_pos(x, y);   
      p_data->last_x = x;
      p_data->last_y = y;
    }
  }
}

//---------------------------------------------------------
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
  double x, y;
  window_data_t*  p_data = glfwGetWindowUserPointer( window );
  if ( p_data->input_flags & MSG_MOUSE_BUTTON_MASK ) {
    glfwGetCursorPos(window, &x, &y);
    send_mouse_button( button, action, mods, x, y );
  }
}

//---------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  double x, y;
  window_data_t*  p_data = glfwGetWindowUserPointer( window );
  if ( p_data->input_flags & MSG_MOUSE_SCROLL_MASK ) {
    glfwGetCursorPos(window, &x, &y);
    send_scroll(xoffset, yoffset, x, y);
  }
}
//---------------------------------------------------------
void cursor_enter_callback(GLFWwindow* window, int entered)
{
  double x, y;
  window_data_t*  p_data = glfwGetWindowUserPointer( window );
  if ( p_data->input_flags & MSG_MOUSE_ENTER_MASK ) {
    glfwGetCursorPos(window, &x, &y);
    send_cursor_enter(entered, x, y);
  }
}

//---------------------------------------------------------
void window_close_callback( GLFWwindow* window )
{
  // let the calling app filter the close event. Send a message up.
  // if the app wants to let the window close, it needs to send a close back down.
  send_close();
  glfwSetWindowShouldClose(window, false);
}

//=============================================================================
// main setup

//---------------------------------------------------------
// done before the window is created
void set_window_hints( const char* resizable ) {
  if ( strncmp(resizable,"true",4) != 0 ) {
    // don't let the user resize the window
    glfwWindowHint(GLFW_RESIZABLE, false);
  }

  // claim the focus right on creation
  glfwWindowHint(GLFW_FOCUSED, true);

  // we want OpenGL 2.1
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);


  // turn on anti-aliasing
  // glfwWindowHint(GLFW_SAMPLES, 4);

  // turn on stencil buffer
  // glfwWindowHint(GLFW_STENCIL_BITS, 1);
}

// //---------------------------------------------------------
// // set up one-time features of the window
// void setup_window( GLFWwindow* window, int width, int height, int dl_block_size ) {
//   window_data_t*  p_data;

//   // set up the window's private data
//   p_data = malloc( sizeof(window_data_t) );
//   memset( p_data, 0, sizeof(window_data_t) );
//   p_data->keep_going = true;

//   p_data->input_flags = 0xFFFF;
//   p_data->last_x = -1.0f;
//   p_data->last_y = -1.0f;

//   p_data->context.root_dl = -1;

//   p_data->context.window_width = width;
//   p_data->context.window_height = height;

//   p_data->context.glew_ok = false;

//   // pthread_rwlock_init(&p_data->context.gl_lock, NULL);

//   glfwSetWindowUserPointer( window, p_data );

//   // Make the window's context current 
//   glfwMakeContextCurrent(window);
//   glfwSwapInterval(1);

//   // get the actual framebuffer size to set it up
//   int frame_width, frame_height;
//   glfwGetFramebufferSize( window, &frame_width, &frame_height );

//   // reshape up the framebuffer
//   reshape_framebuffer(window, frame_width, frame_height );

//   p_data->context.p_ctx = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);

//   // set up one-time gl properties
//   glClearColor(0.0, 0.0, 0.0, 0.0);
//   glClearDepth(1);

//   // This turns on/off depth test.
//   // With this ON, whatever we draw FIRST is 
//   // "on top" and each subsequent draw is BELOW 
//   // the draw calls before it.
//   // With this OFF, whatever we draw LAST is
//   // "on top" and each subsequent draw is ABOVE
//   // the draw calls before it.
//   glDisable(GL_DEPTH_TEST);

//   // Probably need this on, enables Gouraud Shading
//   glShadeModel(GL_SMOOTH);

//   // Turn on Alpha Blending
//   // There are some efficiencies to be gained by ONLY
//   // turning this on when we have a primitive with a
//   // style that has an alpha channel != 1.0f but we
//   // don't have code to detect that.  Easy to do if we need it!
//   glEnable (GL_BLEND); 
//   glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);


// #ifndef LINUX
//   // Turn on multisampling
//   // Again, efficiencies to be gained here if we're okay
//   // with buttons that look jaggy
// //  glEnable(GL_MULTISAMPLE);
// //  glHint(GL_MULTISAMPLE_FILTER_HINT_NV, GL_NICEST);
// #endif

//   // set up callbacks
//   glfwSetFramebufferSizeCallback( window, reshape_framebuffer );
//   glfwSetWindowSizeCallback( window, reshape_window );
//   glfwSetKeyCallback( window, key_callback );
//   glfwSetCharModsCallback( window, charmods_callback );
//   glfwSetCursorPosCallback( window, cursor_pos_callback );
//   glfwSetCursorEnterCallback( window, cursor_enter_callback );
//   glfwSetMouseButtonCallback( window, mouse_button_callback );
//   glfwSetScrollCallback( window, scroll_callback );
//   glfwSetWindowCloseCallback( window, window_close_callback );

//   // initialize glew - do after setting up window and making current
//   p_data->context.glew_ok = glewInit() == GLEW_OK;

//   // setup the display lists

//   // setup the private window data
//   GLuint first_dl = glGenLists( dl_block_size );
//   // p_data->context.empty_dl = glGenLists( dl_block_size );
  
//   // initialize the dls to empty graphs
//   for (int i = first_dl; i < first_dl + dl_block_size; i++ ) {
//     glNewList(i, GL_COMPILE);
//     glEndList();
//   }

//   // root starts out as the empty dl
//   p_data->context.root_dl = first_dl;

//   // signal the app that the window is ready
//   send_ready( first_dl );
// }



//---------------------------------------------------------
// set up one-time features of the window
void setup_window( GLFWwindow* window, int width, int height, int num_scripts ) {
  window_data_t*  p_data;

  // set up the window's private data
  p_data = malloc( sizeof(window_data_t) );
  memset( p_data, 0, sizeof(window_data_t) );
  p_data->p_scripts = NULL;
  
  p_data->keep_going = true;

  p_data->input_flags = 0xFFFF;
  p_data->last_x = -1.0f;
  p_data->last_y = -1.0f;

  p_data->root_script = -1;

  p_data->context.window_width = width;
  p_data->context.window_height = height;

  p_data->context.glew_ok = false;

  // pthread_rwlock_init(&p_data->context.gl_lock, NULL);

  glfwSetWindowUserPointer( window, p_data );

  // Make the window's context current 
  glfwMakeContextCurrent(window);
  // glfwSwapInterval(1);

  // initialize glew - do after setting up window and making current
  p_data->context.glew_ok = glewInit() == GLEW_OK;

  p_data->context.p_ctx = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
  if (p_data->context.p_ctx == NULL) {
    send_puts("Could not init nanovg!!!");
    return;
  }

  // set up callbacks
  glfwSetFramebufferSizeCallback( window, reshape_framebuffer );
  glfwSetWindowSizeCallback( window, reshape_window );
  glfwSetKeyCallback( window, key_callback );
  glfwSetCharModsCallback( window, charmods_callback );
  glfwSetCursorPosCallback( window, cursor_pos_callback );
  glfwSetCursorEnterCallback( window, cursor_enter_callback );
  glfwSetMouseButtonCallback( window, mouse_button_callback );
  glfwSetScrollCallback( window, scroll_callback );
  glfwSetWindowCloseCallback( window, window_close_callback );

  // set up the scripts table
  p_data->p_scripts = malloc( sizeof(void*) * num_scripts );
  memset(p_data->p_scripts, 0, sizeof(void*) * num_scripts );
  p_data->num_scripts = num_scripts;

  // set the initial clear color
  glClearColor(0.0, 0.0, 0.0, 1.0);

  // signal the app that the window is ready
  send_ready( 0 );
}


//---------------------------------------------------------
// tear down one-time features of the window
void cleanup_window( GLFWwindow* window ) {
  // free the window's private data
  free( glfwGetWindowUserPointer( window ) );
}

//---------------------------------------------------------
// return true if the caller side of the stdin pipe is open and in
// business. If it closes, then return false
// http://pubs.opengroup.org/onlinepubs/7908799/xsh/poll.html
// see https://stackoverflow.com/questions/25147181/pollhup-vs-pollnval-or-what-is-pollhup
bool isCallerDown() 
{
    struct pollfd ufd;
    memset(&ufd, 0, sizeof ufd);
    ufd.fd = STDIN_FILENO;
    ufd.events = POLLIN;
    if ( poll(&ufd, 1, 0) < 0 ) 
        return true;
    return ufd.revents & POLLHUP;
}


// //---------------------------------------------------------
// pthread_t start_comms_thread( GLFWwindow* window ) {
//   pthread_t pthread;
//   if ( pthread_create(&pthread, NULL, comms_thread, (void*)window) != 0 ) { return 0; }
//   return pthread;
// }


//---------------------------------------------------------
int main(int argc, char **argv) {
  GLFWwindow*     window;
  GLuint          root_dl_id;

// char buff[200];


  test_endian();

  // super simple arg check
  if ( argc != 6 ) {
    printf("\r\nscenic_driver_glfw should be launched via the Scenic.Driver.Glfw library.\r\n\r\n");
    return 0;
  }
  int width  = atoi(argv[1]);
  int height = atoi(argv[2]);
  int dl_block_size = atoi(argv[5]);


  // send_puts( "in glfw");

  /* Initialize the library */
  if (!glfwInit()) {return -1;}
  glfwSetErrorCallback(errorcb);

  // set the glfw window hints - done before window creation
  set_window_hints( argv[4] );

  /* Create a windowed mode window and its OpenGL context */
  window = glfwCreateWindow(width, height, argv[3], NULL, NULL);
  if (!window)
  {
      glfwTerminate();
      return -1;
  }

  // set up one-time features of the window
  setup_window( window, width, height, dl_block_size );
  window_data_t*  p_data = glfwGetWindowUserPointer( window );

  // start listening for commands from the calling app
  // start_comms_thread( window );

  // put stdio into non-blocking mode
  // int flags = fcntl(0, F_GETFL, 0);
  // fcntl(0, F_SETFL, flags | O_NONBLOCK);


  // signal the app that the window is ready
  // send_ready( p_data->context.empty_dl );

  /* Loop until the calling app closes the window */
  while ( p_data->keep_going && !isCallerDown() ) {

    // // check for incoming messages - blocks with a timeout
    if ( p_data->redraw || handle_stdio_in(window) ) {
      p_data->redraw = false;

      // sprintf(buff, "---------- Start frame root: %d", p_data->root_script);
      // send_puts(buff);

      // clear the buffer
      glClear(GL_COLOR_BUFFER_BIT);
      // render the scene
      nvgBeginFrame(
        p_data->context.p_ctx,
        p_data->context.window_width,
        p_data->context.window_height,
        1.0f
      );
      if ( p_data->root_script >= 0 ) {
        run_script( p_data->root_script, p_data );
      }
      nvgEndFrame(p_data->context.p_ctx);
      // Swap front and back buffers
      glfwSwapBuffers(window);
    }

    // wait for events - timeout is in seconds
    // the timeout is the max time the app will stay alive
    // after the host BEAM environment shuts down.
    // glfwWaitEventsTimeout(1.01f);

    // poll for events and return immediately
    glfwPollEvents();
  }

  // one more lock just to make sure any running messages have a chance
  // to complete before tearing down the graphics environment.
  // if ( pthread_rwlock_rdlock(&p_data->context.gl_lock) == 0 ) {
  //   pthread_rwlock_unlock(&p_data->context.gl_lock);
  // }

  // clean up
  cleanup_window( window );
  glfwTerminate();

  return 0;
}
