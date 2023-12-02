#include "mempools.h"

#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "boolvals.h"

static DynamicArray *g_pools;

// definitions

typedef struct _BuffData {
	void *buffer;
	boolval busy;
} BuffData;

typedef struct _BuffPool {
	char *identifier;
	BuffData buffers[ MEM_BUFFERS_PER_POOL ];
	size_t buffer_data_size;
} BuffPool;


// static utils

static BuffPool *get_mem_pool( char* identifier )
{
	for ( size_t i = 0; i < g_pools->usage; ++i )
	{
		BuffPool *pool = ( BuffPool* ) g_pools->data + i;
		if ( strcmp( identifier, pool->identifier ) == 0 )
			return pool;
	}
	return NULL;
}

static int get_mem_pool_index( char* identifier )
{
	for ( size_t i = 0; i < g_pools->usage; ++i )
	{
		BuffPool *pool = ( BuffPool* ) g_pools->data + i;
		if ( strcmp( identifier, pool->identifier ) == 0 )
			return i;
	}
	return -1;
}

// functions

void init_mem_pools()
{
	g_pools = createDynamicArray( sizeof( BuffPool ) );
}

void terminate_mem_pools()
{
	deleteDynamicArray( g_pools );
}

int is_mem_pool_registered( char* identifier )
{
	return ( get_mem_pool( identifier ) != NULL );
}

int get_mem_pool_buffer_data_size( char* identifier )
{
	BuffPool* pool = get_mem_pool( identifier );
	if ( pool == NULL ) return -1;
	return pool->buffer_data_size;
}

int gen_mem_pool( char* identifier, unsigned int buffer_data_size )
{
	if ( is_mem_pool_registered( identifier ) ) return 1;
	char *identifier_cpy = malloc( strlen( identifier ) + 1 );
	strcpy( identifier_cpy, identifier );
	BuffPool pool = {
		identifier_cpy,
		{ 0 },
		buffer_data_size	
	};

	for ( size_t i = 0; i < MEM_BUFFERS_PER_POOL; ++i )
	{
		void *buff = malloc( buffer_data_size );
		pool.buffers[i].buffer = buff;
	}

	pushDataInDynamicArray( g_pools, &pool );
	return 0;
}
int remove_mem_pool( char* identifier )
{
	int index = get_mem_pool_index( identifier );
	if ( index < 0 ) return 1;
	BuffPool *pool = ( BuffPool* ) g_pools->data + index;

	for ( size_t i = 0; i < MEM_BUFFERS_PER_POOL; ++i )
	{
		void *buff = pool->buffers[i].buffer;
		free( buff );
	}

	free( pool->identifier );
	removeDataAtIndexFromDynamicArray( g_pools, index, false );
	return 0;
}

void* get_mem_pool_buffer( char* identifier )
{
	BuffPool *pool = get_mem_pool( identifier );
	if ( pool == NULL ) return NULL;

	for ( size_t i = 0; i < MEM_BUFFERS_PER_POOL; ++i )
	{
		if ( pool->buffers[ i ].busy == true ) continue;
		pool->buffers[ i ].busy = true;
		return pool->buffers[ i ].buffer;
	}
	return NULL;
}
int yield_mem_pool_buffer( char *identifier, void *buff )
{
	BuffPool *pool = get_mem_pool( identifier );
	if ( pool == NULL ) return 1;

	for ( size_t i = 0; i < MEM_BUFFERS_PER_POOL; ++i )
	{
		if ( pool->buffers[ i ].buffer != buff ) continue;
		pool->buffers[ i ].busy = false;
		return 0;
	}
	return 1;
}
