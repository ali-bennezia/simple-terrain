#include "vbopools.h"

#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "utils.h"
#include "boolvals.h"

static DynamicArray *g_pools;

// definitions

typedef struct _BuffData {
	GLuint id;
	boolval busy;
} BuffData;

typedef struct _BuffPool {
	char *identifier;
	BuffData buffers[ VBO_BUFFERS_PER_POOL ];
	size_t buffer_data_size;
} BuffPool;


// static utils

static BuffPool *get_vbo_pool( char* identifier )
{
	for ( size_t i = 0; i < g_pools->usage; ++i )
	{
		BuffPool *pool = ( BuffPool* ) g_pools->data + i;
		if ( strcmp( identifier, pool->identifier ) == 0 )
			return pool;
	}
	return NULL;
}

static int get_vbo_pool_index( char* identifier )
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

void init_vbo_pools()
{
	g_pools = createDynamicArray( sizeof( BuffPool ) );
}

void terminate_vbo_pools()
{
	deleteDynamicArray( g_pools );
}

int is_vbo_pool_registered( char* identifier )
{
	return ( get_vbo_pool( identifier ) != NULL );
}

int get_vbo_pool_buffer_data_size( char* identifier )
{
	BuffPool* pool = get_vbo_pool( identifier );
	if ( pool == NULL ) return -1;
	return pool->buffer_data_size;
}

int gen_vbo_pool( char* identifier, unsigned int buffer_data_size )
{
	if ( is_vbo_pool_registered( identifier ) ) return 1;
	char *identifier_cpy = malloc( strlen( identifier ) + 1 );
	strcpy( identifier_cpy, identifier );
	BuffPool pool = {
		identifier_cpy,
		{ 0 },
		buffer_data_size	
	};

	for ( size_t i = 0; i < VBO_BUFFERS_PER_POOL; ++i )
	{
		GLuint buff_id;
		glGenBuffers( 1, &buff_id );
		pool.buffers[i].id = buff_id;
		glBindBuffer( GL_ARRAY_BUFFER, buff_id );
		glBufferData( GL_ARRAY_BUFFER, buffer_data_size, NULL, GL_DYNAMIC_DRAW );	
	}

	pushDataInDynamicArray( g_pools, &pool );
	return 0;
}
int remove_vbo_pool( char* identifier )
{
	int index = get_vbo_pool_index( identifier );
	if ( index < 0 ) return 1;
	BuffPool *pool = ( BuffPool* ) g_pools->data + index;

	for ( size_t i = 0; i < VBO_BUFFERS_PER_POOL; ++i )
	{
		GLuint buff_id = pool->buffers[i].id;
		glDeleteBuffers( 1, &buff_id );
	}

	free( pool->identifier );
	removeDataAtIndexFromDynamicArray( g_pools, index, false );
	return 0;
}

int get_vbo_pool_buffer( char* identifier )
{
	BuffPool *pool = get_vbo_pool( identifier );
	if ( pool == NULL ) return -1;

	for ( size_t i = 0; i < VBO_BUFFERS_PER_POOL; ++i )
	{
		if ( pool->buffers[ i ].busy == true ) continue;
		pool->buffers[ i ].busy = true;
		return pool->buffers[ i ].id;
	}
	return -1;
}
int yield_vbo_pool_buffer( char *identifier, int buff )
{
	BuffPool *pool = get_vbo_pool( identifier );
	if ( pool == NULL ) return 1;

	for ( size_t i = 0; i < VBO_BUFFERS_PER_POOL; ++i )
	{
		if ( ( int ) pool->buffers[ i ].id != buff ) continue;
		pool->buffers[ i ].busy = false;
		return 0;
	}
	return 1;
}
