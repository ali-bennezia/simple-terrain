#ifndef _POOLS_H_
#define _POOLS_H_

#define BUFFERS_PER_POOL 25

void init_pools();
void terminate_pools();

int is_pool_registered( char* identifier );
int get_pool_buffer_data_size( char* identifier );

int gen_pool( char* identifier, unsigned int buffer_data_size );
int remove_pool( char* identifier );

int get_pool_buffer( char* identifier );
int yield_pool_buffer( char *identifier, int buff );

#endif
