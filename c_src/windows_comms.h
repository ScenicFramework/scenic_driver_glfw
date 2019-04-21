/*
# Windows-specific implementations to handle port communication
*/

#ifndef _WIN32_COMMS_H
  #define _WIN32_COMMS_H
  #define WIN32_LEAN_AND_MEAN

  #include "windows.h"
  #include <io.h>
  #include <stdio.h>
  #include <stdint.h>

  #include "types.h"
  #include "utils.h"

  typedef struct timeval {
      long tv_sec;
      long tv_usec;
  } timeval;

  int gettimeofday(struct timeval * tp, struct timezone * tzp);
#endif