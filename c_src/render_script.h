/*
#  Created by Boyd Multerer on June 1, 2018.
#  Copyright © 2018 Kry10 Industries. All rights reserved.
#

functions to play a compiled render script
*/

void put_script( window_data_t* p_data, GLuint id, void* p_script );
void* get_script( window_data_t* p_data, GLuint id );
void delete_script( window_data_t* p_data, GLuint id );
void delete_all( window_data_t* p_data );

void run_script( GLuint script_id, window_data_t* p_data );