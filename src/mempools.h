#ifndef _MEMPOOLS_H_
#define _MEMPOOLS_H_

#define MEM_BUFFERS_PER_POOL 3000

void init_mem_pools();
void terminate_mem_pools();

int is_mem_pool_registered( char* identifier );
int get_mem_pool_buffer_data_size( char* identifier );

int gen_mem_pool( char* identifier, unsigned int buffer_data_size );
int remove_mem_pool( char* identifier );

void* get_mem_pool_buffer( char* identifier );
int yield_mem_pool_buffer( char *identifier, void *buff );

#endif
