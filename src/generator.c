#include "generator.h"
#include "boolvals.h"
#include "utils.h"
#include "factory.h"
#include "noises.h"
#include "objects.h"
#include "materials.h"
#include "terrain.h"
#include "pools.h"

#include <stdlib.h>
#include <pthread.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <string.h>

#define MAX_THREADS 10
#define MAX_PENDING_REQUESTS 100

extern float g_quadtree_root_size;
extern const int QUAD_COUNT;

struct thread_state {

	pthread_t thread;
	boolval running;

};

struct thread_state threads[MAX_THREADS];
pthread_mutex_t threads_mtx;

struct generation_request_buffer_data {
	GLuint buffer_id;
	void *buffer_data;
};

struct generation_request {
	int x_coord, z_coord;
	size_t level, tessellations;

	float x_pos, z_pos, size;

	void *vertices, *normals;
	size_t verticesCount, normalsCount;

	struct generation_request_buffer_data vertices_vbo_data;
	struct generation_request_buffer_data normals_vbo_data;

	boolval pending, done, fetched;
};
struct generation_request pending_requests[MAX_PENDING_REQUESTS];
boolval pending_fetches = false;
pthread_mutex_t pending_requests_mtx;

void initialize_generator()
{
	pthread_mutex_init( &threads_mtx, NULL );
	pthread_mutex_init( &pending_requests_mtx, NULL );

	pthread_mutex_lock( &threads_mtx );

	for ( size_t i = 0; i < MAX_THREADS; ++i ){
		threads[i].running = false;
	}

	pthread_mutex_unlock( &threads_mtx );

	pthread_mutex_lock( &pending_requests_mtx );

	for ( size_t i = 0; i < MAX_PENDING_REQUESTS; ++i ){
		pending_requests[i].pending = false;
		pending_requests[i].done = true;
		pending_requests[i].fetched = true;
	}

	pthread_mutex_unlock( &pending_requests_mtx );
}

extern Material g_defaultTerrainMaterialLit;
extern GLFWwindow* g_window;

void poll_generator()
{

	pthread_mutex_lock( &pending_requests_mtx );


	if ( pending_fetches == false ) {
		pthread_mutex_unlock( &pending_requests_mtx );
		return;
	}

	for ( size_t i = 0; i < MAX_PENDING_REQUESTS; ++i ){

		if ( pending_requests[i].pending == false && pending_requests[i].done == true && pending_requests[i].fetched == false ) {

			pending_requests[i].fetched = true;

			glBindBuffer( GL_ARRAY_BUFFER, pending_requests[i].vertices_vbo_data.buffer_id );
			glUnmapBuffer( GL_ARRAY_BUFFER );
			glBindBuffer( GL_ARRAY_BUFFER, pending_requests[i].normals_vbo_data.buffer_id );
			glUnmapBuffer( GL_ARRAY_BUFFER );

			float requested_terrain_size = g_quadtree_root_size / pow( 2, pending_requests[i].level );
			float x_pos = pending_requests[i].x_coord * requested_terrain_size, z_pos = pending_requests[i].z_coord * requested_terrain_size;

			PerspectiveObject *requested_terrain = createPerspectiveObject( );

			requested_terrain->position.x = x_pos;
			requested_terrain->position.y = 0;
			requested_terrain->position.z = z_pos;

			setObjectVBO(
				requested_terrain, 
				pending_requests[i].vertices_vbo_data.buffer_id, 
				VERTICES
			);
			setObjectVBO(
				requested_terrain, 
				pending_requests[i].normals_vbo_data.buffer_id, 
				NORMALS
			);

			/*setObjectVBO(
				requested_terrain, 
				createAndFillVBO(
					pending_requests[i].vertices, 
					pending_requests[i].verticesCount*3*sizeof(float), 
					GL_ARRAY_BUFFER, 
					GL_STATIC_DRAW
				), 
				VERTICES
			);
			setObjectVBO(
				requested_terrain, 
				createAndFillVBO(
					pending_requests[i].normals, 
					pending_requests[i].normalsCount*3*sizeof(float), 
					GL_ARRAY_BUFFER, 
					GL_STATIC_DRAW
				), 
				NORMALS
			);*/

			free( pending_requests[i].vertices );
			free( pending_requests[i].normals );

			requested_terrain->material = &g_defaultTerrainMaterialLit;
			requested_terrain->vertices = pending_requests[i].verticesCount;

			push_node_terrain( pending_requests[i].x_coord, pending_requests[i].z_coord, pending_requests[i].level, requested_terrain );

		}

	}

	pending_fetches = false;

	pthread_mutex_unlock( &pending_requests_mtx );	

}

void terminate_generator()
{
	pthread_mutex_destroy( &threads_mtx );
	pthread_mutex_destroy( &pending_requests_mtx );
}

int find_request_index( boolval pending_predicate, boolval done_predicate, boolval fetched_predicate )
{
	for ( size_t i = 0; i < MAX_PENDING_REQUESTS; ++i ){
		
		if ( pending_requests[i].pending == pending_predicate && pending_requests[i].done == done_predicate && pending_requests[i].fetched == fetched_predicate )
			return i;

	}

	return -1;
}

void *thread_job( void *data )
{

	pthread_t self = pthread_self();



	boolval found = true;

	while ( found == true ) {

		found = false;

		int request_index;
		struct generation_request request;
		
		pthread_mutex_lock( &pending_requests_mtx );

		int search_result = find_request_index( true, false, false );	

		if ( search_result != -1 ) {
			found = true;
			request_index = search_result;
			request = pending_requests[ search_result ];
		}

		pthread_mutex_unlock( &pending_requests_mtx );	

		if ( found == false ) break;
		
		float *vertices = NULL, *normals = NULL;
		size_t verticesCount, normalsCount;

		generateTessellatedQuad(
			request.x_pos, 
			request.z_pos, 
			&vertices, 
			&normals, 
			request.tessellations, 
			request.size, 
			terrain_heightmap_func, 
			&verticesCount, 
			&normalsCount
		);

		request.vertices = vertices;
		request.normals = normals;
		request.verticesCount = verticesCount;
		request.normalsCount = normalsCount;

		memcpy( request.vertices_vbo_data.buffer_data, vertices, verticesCount * sizeof( float ) * 3 );
		memcpy( request.normals_vbo_data.buffer_data, normals, normalsCount * sizeof( float ) * 3 );

		request.pending = false;
		request.done = true;
		request.fetched = false;

		// data output

		pthread_mutex_lock( &pending_requests_mtx );

		pending_requests[ request_index ] = request;
		pending_fetches = true;	

		pthread_mutex_unlock( &pending_requests_mtx );

	}


	// thread end

	// set running to false

	pthread_mutex_lock( &threads_mtx );

	for ( size_t i = 0; i < MAX_THREADS; ++i ){

		if ( pthread_equal( self, threads[i].thread ) != 0 ){
			threads[i].running = false;
			break;
		}

	}
	
	pthread_mutex_unlock( &threads_mtx );	

	// exit

	pthread_exit( NULL );

}

void create_threads()
{
	pthread_mutex_lock( &threads_mtx );
	for ( size_t i = 0; i < MAX_THREADS; ++i ){
		if ( threads[i].running == true) continue;

		threads[i].running = true;
		pthread_create( &(threads[i].thread), NULL, thread_job, NULL );
	}
	pthread_mutex_unlock( &threads_mtx );

}

/*
	struct generation_request_buffer_data vertices_vbo_data;
	struct generation_request_buffer_data UVs_vbo_data;
	struct generation_request_buffer_data normals_vbo_data;
*/

void request_generation( int x_coord, int z_coord, size_t level, size_t tessellations )
{
	float terrain_size = g_quadtree_root_size / pow( 2, level );

	pthread_mutex_lock( &pending_requests_mtx );

	boolval found = false;

	for ( size_t i = 0; i < MAX_PENDING_REQUESTS; ++i ){
		if ( pending_requests[i].pending == false && pending_requests[i].done == true && pending_requests[i].fetched == true ){

			pending_requests[i].x_coord = x_coord;
			pending_requests[i].z_coord = z_coord;
			pending_requests[i].level = level;

			pending_requests[i].x_pos = x_coord * terrain_size;
			pending_requests[i].z_pos = z_coord * terrain_size;
			pending_requests[i].size = terrain_size;

			pending_requests[i].tessellations = tessellations;

			// vertices buffer
			pending_requests[i].vertices_vbo_data.buffer_id = createVBO();
			//pending_requests[i].vertices_vbo_data.buffer_id = get_pool_buffer( "Quadtree" );
			glBindBuffer( GL_ARRAY_BUFFER, pending_requests[i].vertices_vbo_data.buffer_id );
			glBufferData( GL_ARRAY_BUFFER, QUAD_COUNT * 6 * 3 * sizeof( float ), NULL, GL_DYNAMIC_DRAW );
			pending_requests[i].vertices_vbo_data.buffer_data = glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY );	

			// normals buffer
			pending_requests[i].normals_vbo_data.buffer_id = createVBO();
			//pending_requests[i].normals_vbo_data.buffer_id = get_pool_buffer( "Quadtree" );
			glBindBuffer( GL_ARRAY_BUFFER, pending_requests[i].normals_vbo_data.buffer_id );
			glBufferData( GL_ARRAY_BUFFER, QUAD_COUNT * 6 * 3 * sizeof( float ), NULL, GL_DYNAMIC_DRAW );
			pending_requests[i].normals_vbo_data.buffer_data = glMapBuffer( GL_ARRAY_BUFFER, GL_WRITE_ONLY );	

			pending_requests[i].pending = true;	
			pending_requests[i].done = false;
			pending_requests[i].fetched = false;
			found = true;

			break;

		}
	}

	pthread_mutex_unlock( &pending_requests_mtx );	

	if ( found == true ){
		create_threads();
	}	
}

