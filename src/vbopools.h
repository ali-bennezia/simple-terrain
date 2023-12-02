#ifndef _VBOPOOLS_H_
#define _VBOPOOLS_H_

#define VBO_BUFFERS_PER_POOL 500

void init_vbo_pools();
void terminate_vbo_pools();

int is_vbo_pool_registered( char* identifier );
int get_vbo_pool_buffer_data_size( char* identifier );

int gen_vbo_pool( char* identifier, unsigned int buffer_data_size );
int remove_vbo_pool( char* identifier );

int get_vbo_pool_buffer( char* identifier );
int yield_vbo_pool_buffer( char *identifier, int buff );

#endif
