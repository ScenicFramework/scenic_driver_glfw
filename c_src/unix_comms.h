/*
# Implementations to handle port communication on UNIX-based systems
*/

#pragma once

  #include <stdio.h>
  #include <unistd.h>
  #include <poll.h>
  #include <sys/time.h>
  #include <sys/select.h>
  #include <stdint.h>
  #include <string.h>

  #include "types.h"
  // #include "utils.h"
