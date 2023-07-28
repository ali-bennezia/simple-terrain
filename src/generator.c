#include "generator.h"
#include "boolvals.h"
#include "utils.h"
#include "factory.h"
#include "noises.h"
#include "objects.h"
#include "materials.h"

#include <stdlib.h>
#include <pthread.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define MAX_THREADS 4
#define MAX_PENDING_REQUESTS 50


struct thread_state {

	pthread_t thread;
	boolval running;

};

struct thread_state threads[MAX_THREADS];
pthread_mutex_t threads_mtx;


struct generation_request {
	int x, z;
	float size;
	size_t tessellations;

	void *vertices, *normals;
	size_t verticesCount, normalsCount;

	boolval pending, done, fetched;
};
struct generation_request pending_requests[MAX_PENDING_REQUESTS];
boolval pending_fetches = false;
pthread_mutex_t pending_requests_mtx;

void initialize_generator()
{
	pthread_mutex_init( &threads_mtx, NULL );
	pthread_mutex_init( &pending_requests_mtx, NULL );

	for ( size_t i = 0; i < MAX_THREADS; ++i ){
		threads[i].running = false;
	}

	for ( size_t i = 0; i < MAX_PENDING_REQUESTS; ++i ){
		pending_requests[i].pending == false;
		pending_requests[i].done == true;
		pending_requests[i].fetched == true;
	}
}

extern Material g_defaultTerrainMaterialLit;

void poll_generator()
{

	pthread_mutex_lock( &pending_requests_mtx );

	if ( !pending_fetches ) {

		pthread_mutex_unlock( &pending_requests_mtx );
		return;

	}

	for ( size_t i = 0; i < MAX_PENDING_REQUESTS; ++i ){

		if ( pending_requests[i].pending == false && pending_requests[i].done == true && pending_requests[i].fetched == false ) {

			pending_requests[i].fetched = true;


			PerspectiveObject *requested_terrain = createPerspectiveObject();

			requested_terrain->position.x = pending_requests[i].x;
			//requested_terrain->position.y = 0;
			requested_terrain->position.z = pending_requests[i].z;

			setObjectVBO(
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
			);

			free( pending_requests[i].vertices );
			free( pending_requests[i].normals );

			requested_terrain->material = &g_defaultTerrainMaterialLit;
			requested_terrain->vertices = pending_requests[i].verticesCount;
			

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

		if ( !found ) break;
		
		float *vertices = NULL, *normals = NULL;
		size_t verticesCount, normalsCount;

		generateTessellatedQuad(
			request.x, 
			request.z, 
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

		if ( pthread_equal( self, threads[i].thread ) ){
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

		pthread_create( &threads[i].thread, NULL, thread_job, NULL );
	}
	pthread_mutex_unlock( &threads_mtx );

}

void request_generation( int x, int z, float size, size_t tessellations )
{
	pthread_mutex_lock( &pending_requests_mtx );

	boolval found = false;

	for ( size_t i = 0; i < MAX_PENDING_REQUESTS; ++i ){
		if ( pending_requests[i].pending == false && pending_requests[i].done == true && pending_requests[i].fetched == true ){

			pending_requests[i].x = x;
			pending_requests[i].z = z;
			pending_requests[i].size = size;
			pending_requests[i].tessellations = tessellations;

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





