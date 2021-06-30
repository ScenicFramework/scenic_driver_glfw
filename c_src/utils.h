/*
#  Created by Boyd Multerer on 2/25/18.
#  Copyright © 2018 Kry10 Limited. All rights reserved.
#

Functions to load fonts and render text
*/

#pragma once

#ifndef _UTILS_H
#define _UTILS_H

#define ALIGN_UP(n, s) (((n) + (s) - 1) & ~((s) - 1))
#define ALIGN_DOWN(n, s) ((n) & ~((s) - 1))

// void check_gl_error(char* msg);
void check_gl_error();

#endif