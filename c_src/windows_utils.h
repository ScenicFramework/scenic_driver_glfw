/*
Utilities to provide Windows compatibility.
*/

#ifndef _WIN32_UTILS_H
#define _WIN32_UTILS_H
#define WIN32_LEAN_AND_MEAN

#include "windows.h"
#include <io.h>
#include <stdint.h>

// MSVC defines this in winsock2.h!?
typedef struct timeval {
    long tv_sec;
    long tv_usec;
} timeval;

int gettimeofday(struct timeval * tp, struct timezone * tzp);

#endif