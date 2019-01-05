/*
#  Created by Boyd Multerer on 2/18/18.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#

Functions to load textures onto the graphics card
*/

#ifndef _TEXTURE_H
#define _TEXTURE_H

GLenum translate_texture_format(int channels);
void receive_put_tx_file(int* p_msg_length, GLFWwindow* window);
void receive_put_tx_raw(int* p_msg_length, GLFWwindow* window);

#endif