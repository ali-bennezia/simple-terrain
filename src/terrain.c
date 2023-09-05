#include "terrain.h"

#include <cglm/cglm.h>
#include <pthread.h>

#include "objects.h"
#include "generator.h"

/// definitions

enum NodeType
{
	NODE_TYPE_UNIQUE,
	NODE_TYPE_MANIFOLD	
};

enum NodeState
{
	NODE_STATE_EMPTY,
	NODE_STATE_AWAITING,
	NODE_STATE_CHUNK
};

typedef struct Node {
	enum NodeType type;
	enum NodeState state;
	void *data;
} Node;

// data going from the main thread to the quadtree worker thread

typedef struct InboundData {
	float x_pos, z_pos, size;
	PerspectiveObject *obj;
	boolval done;
} InboundData;

// data going from the quadtree worker thread to the main thread

enum OutboundDataType {
	OUTBOUND_DATA_TYPE_GENERATE_CHUNK,
	OUTBOUND_DATA_TYPE_REMOVE_CHUNK,
	OUTBOUND_DATA_TYPE_SET_CHUNK_VISIBLE,
	OUTBOUND_DATA_TYPE_SET_CHUNK_INVISIBLE
};

typedef struct OutboundData {
	float x_pos, z_pos, size;
	enum OutboundDataType type;
	PerspectiveObject *obj;
	boolval done;
} OutboundData;

/// quadtree worker thread

static pthread_t g_quadtree_thread;
static boolval g_quadtree_thread_running = true;
static pthread_mutex_t g_quadtree_mtx;

// inter-thread data transferts

#define MAX_DATA_TRANSFERTS 80

static InboundData g_inbound[ MAX_DATA_TRANSFERTS ] = { 0 };
static OutboundData g_outbound[ MAX_DATA_TRANSFERTS ] = { 0 };

static pthread_mutex_t g_inbound_mtx, g_outbound_mtx;

/// inter-thread data transfert utilities

static void push_inbound_data( float x_pos, float z_pos, float size, PerspectiveObject *obj )
{

	pthread_mutex_lock( &g_inbound_mtx );	

	for ( size_t i = 0; i < MAX_DATA_TRANSFERTS; ++i )
	{
	
		InboundData data = g_inbound[ i ];
		if ( data.done )
		{
			data.x_pos = x_pos;
			data.z_pos = z_pos;
			data.size = size;
			data.obj = obj;
			data.done = false;
			g_inbound[ i ] = data;
			break;
		}		
	
	}

	pthread_mutex_unlock( &g_inbound_mtx );	
	
}

static void push_outbound_data( float x_pos, float z_pos, float size, enum OutboundDataType type, PerspectiveObject *obj )
{

	pthread_mutex_lock( &g_outbound_mtx );	

	for ( size_t i = 0; i < MAX_DATA_TRANSFERTS; ++i )
	{
	
		OutboundData data = g_outbound[ i ];
		if ( data.done )
		{
			data.x_pos = x_pos;
			data.z_pos = z_pos;
			data.size = size;
			data.type = type;
			data.obj = obj;
			data.done = false;
			g_outbound[ i ] = data;
			break;
		}		
	
	}

	pthread_mutex_unlock( &g_outbound_mtx );	
	
}

/// quadtree parameters

Node g_quadtree_root;
float g_quadtree_root_size = 10000;
float g_quadtree_min_distance = 15000;
size_t g_quadtree_max_level = 7;

#define TESSELLATIONS 4

/// runtime values

extern vec3 g_cameraPosition;
Vec3fl g_quadtree_subject_position;

/// quadtree utilities

static void convert_coords_level( int from_x, int from_z, size_t from_level, size_t to_level, int *result_x, int *result_z )
{
	float quotient = pow( 2, (int)to_level - (int)from_level );

	*result_x = floor( from_x * quotient );
	*result_z = floor( from_z * quotient );
}

static size_t get_position_level( float x, float y, float z )
{
	Vec3fl diff = {
		x - g_quadtree_subject_position.x,
		y - g_quadtree_subject_position.y,
		z - g_quadtree_subject_position.z
	};
	float distance = vec3fl_magnitude( diff );

	float tested_distance = g_quadtree_min_distance;
	int level = 0;
	
	while ( distance < tested_distance ){
		tested_distance /= 2.0;
		++level;
	}

	return min( level, g_quadtree_max_level );	
}

static size_t get_quad_level( int x_coord, int z_coord, size_t level )
{
	float quad_size = g_quadtree_root_size / pow( 2, level );

	float x_pos = quad_size * x_coord + quad_size / 2.0;
	float z_pos = quad_size * z_coord + quad_size / 2.0;

	return get_position_level( x_pos, 0, z_pos );
}

static boolval is_node_awaiting( Node *node )
{
	return node->state == NODE_STATE_AWAITING;
}

static boolval is_node_subdivided( Node *node )
{
	return node->type == NODE_TYPE_MANIFOLD;
}

static Node *search_node( int x_coord, int z_coord, size_t level, boolval *is_covered_out )
{
	Node *index_node = NULL;
	boolval is_covered = false;

	for ( size_t index_level = 0; index_level <= level; ++index_level )
	{
		int index_x_coord, index_z_coord;
		convert_coords_level( x_coord, z_coord, level, index_level, &index_x_coord, &index_z_coord );
		int local_x_coord = index_x_coord % 2, local_z_coord = index_z_coord % 2;
		int local_coord_index = local_z_coord * 2 + local_x_coord;

		if ( index_level == 0 ){
	
			if ( index_x_coord == 0 && index_z_coord == 0 )
			{
				index_node = &g_quadtree_root;
			}else{
				index_node = NULL;
			}

		}else{

			if ( is_node_subdivided( index_node ) )
			{
				index_node = *( ( Node** ) index_node->data + local_coord_index );
			}else{
				index_node = NULL;
			}

		}

		if ( index_node && index_level < level && index_node->state == NODE_STATE_CHUNK && !is_covered )
		{
			is_covered = true;
		}

		if ( !index_node ) break;
	}

	if ( is_covered_out ) *is_covered_out = is_covered;

	return index_node;
}

static PerspectiveObject *get_node_perspective_object( Node *node )
{
	if ( node->state == NODE_STATE_CHUNK )
	{
		if ( node->type == NODE_TYPE_UNIQUE )
		{
			return node->data;
		}else{
			PerspectiveObject *obj = *( ( PerspectiveObject** ) ( ( Node** ) node->data + 4 ) );
			return obj;
		}	
	}else return NULL;
}

/// quadtree mutators

// returns a newly generated empty node
static Node *generate_node()
{
	Node *node = malloc( sizeof( Node ) );
	node->type = NODE_TYPE_UNIQUE;
	node->state = NODE_STATE_EMPTY;
	node->data = NULL;
	return node;
}

// transforms a node such that its state becomes empty .ie requests deletion of its perspective object if it has one
static void empty_node( Node *node )
{
	if ( node->state == NODE_STATE_CHUNK )
	{
		if ( node->type == NODE_TYPE_UNIQUE )
		{
			push_outbound_data( 0, 0, 0, OUTBOUND_DATA_TYPE_REMOVE_CHUNK, node->data );
			node->data = NULL;
		}else{
			PerspectiveObject *obj =  *( ( PerspectiveObject** ) ( ( Node** ) node->data + 4 ) );
			push_outbound_data( 0, 0, 0, OUTBOUND_DATA_TYPE_REMOVE_CHUNK, obj );
			node->data = realloc( node->data, sizeof( Node* ) * 4 );
		}
	}

	node->state = NODE_STATE_EMPTY;
}

// deletes a node and its children
static void delete_node( Node *node )
{
	empty_node( node );

	if ( node->type == NODE_TYPE_MANIFOLD )
	{
		for ( size_t i = 0; i < 4; ++i )
		{
			Node *child_node = *( ( Node** ) node->data + i );
			delete_node( child_node );
		}

		free( node->data );
	}

	free( node );	
}

// transforms a unique node into a manifold node
static void subdivide_node( Node *node )
{
	switch ( node->state )
	{
		case NODE_STATE_EMPTY:
		case NODE_STATE_AWAITING:
		node->data = malloc( sizeof( Node* ) * 4 );
			break;
		case NODE_STATE_CHUNK:
		PerspectiveObject *obj = node->data;
		node->data = malloc( sizeof( Node* ) * 4 + sizeof( PerspectiveObject* ) );
		PerspectiveObject **obj_dest = ( PerspectiveObject** ) ( ( Node** ) node->data + 4 );
		*obj_dest = obj;	
			break;
	}	

	for ( size_t i = 0; i < 4; ++i )
	{
		Node **sub_node_dest = ( Node** ) node->data + i;
		*sub_node_dest = generate_node();
	}

	node->type = NODE_TYPE_MANIFOLD;
}

// transforms a manifold node into a unique node
static void merge_node( Node *node )
{
	void *initial_node_data = node->data;

	if ( node->state == NODE_STATE_CHUNK )
	{
		PerspectiveObject *obj = *( ( PerspectiveObject** ) ( ( Node** ) node->data + 4 ) );
		node->data = obj;
	}else{
		node->data = NULL;
	}

	for ( size_t i = 0; i < 4; ++i )
	{
		delete_node( *( ( Node** ) initial_node_data + i ) );
	}
	free( initial_node_data );
}

// registers a perspective object within an awaiting node
static void fill_awaiting_node( Node *node, PerspectiveObject *obj )
{

	if ( node->type == NODE_TYPE_UNIQUE )
	{
		node->data = obj;	
	}else{
		node->data = realloc( node->data, sizeof( Node** ) * 4 + sizeof( PerspectiveObject** ) );
		*( ( PerspectiveObject** ) ( ( Node** ) node->data + 4 ) ) = obj;
	}

	node->state = NODE_STATE_CHUNK;

}

// sets the visibility of the perspective object tied to a node, if one exists
static void set_node_visibility( Node *node, boolval visibility )
{
	PerspectiveObject *obj = get_node_perspective_object( node );
	if ( obj ) push_outbound_data( 0, 0, 0, visibility ? OUTBOUND_DATA_TYPE_SET_CHUNK_VISIBLE : OUTBOUND_DATA_TYPE_SET_CHUNK_INVISIBLE, obj );
}

// sets the visibilities of all of the direct and indirect children perspective objects of a node ( doesn't alter the visibility of the node's own perspective object )
static void set_node_children_visibility( Node *node, boolval visibility )
{
	if ( node->type == NODE_TYPE_MANIFOLD )
	{
		for ( size_t i = 0; i < 4; ++i )
		{
			Node *child_node = *( ( Node** ) node->data + i );
			set_node_visibility( child_node, visibility );
			set_node_children_visibility( child_node, visibility );
		}
	}
}

/// quadtree worker thread utilities

static void poll_inbound_data()
{
	pthread_mutex_lock( &g_inbound_mtx );

	for ( size_t i = 0; i < MAX_DATA_TRANSFERTS; ++i )
	{

		InboundData data = g_inbound[ i ];	

		if ( !data.done && data.obj )
		{
		
			int x_coord = data.x_pos / data.size, z_coord = data.z_pos;
			size_t level = sqrt( g_quadtree_root_size / data.size );
			boolval is_covered; // is the node a direct/indirect child of a manifold node posessing a perspective object ?
			Node *node = search_node( x_coord, z_coord, level, &is_covered );
			size_t current_level = get_quad_level( x_coord, z_coord, level );

			// ...
			

			if ( node && is_node_awaiting( node ) )
			{
				fill_awaiting_node( node, data.obj );	
			}

			g_inbound[ i ].done = true;
			break;
		}

	}

	pthread_mutex_unlock( &g_inbound_mtx );
}

static void poll_node( int x_coord, int z_coord, size_t level, Node *node )
{

}

static void poll_quadtree()
{
	poll_node( 0, 0, 0, &g_quadtree_root );
}

/// quadtree worker thread runtime

static void *quadtree_worker_thread( void *data )
{
	boolval running = true;

	while ( running )
	{

		pthread_mutex_lock( &g_quadtree_mtx );

		poll_inbound_data();
		poll_quadtree();

		running = g_quadtree_thread_running;
		pthread_mutex_unlock( &g_quadtree_mtx );
	

	}

	pthread_exit( EXIT_SUCCESS );
}

/// terrain control utilities

static void poll_outbound_data()
{
	pthread_mutex_lock( &g_outbound_mtx );

	for ( size_t i = 0; i < MAX_DATA_TRANSFERTS; ++i )
	{

		OutboundData data = g_outbound[ i ];	

		if ( !data.done )
		{
		
			switch ( data.type )
			{
				case OUTBOUND_DATA_TYPE_GENERATE_CHUNK:
					request_generation( data.x_pos, data.z_pos, data.size, TESSELLATIONS );
					break;
				case OUTBOUND_DATA_TYPE_REMOVE_CHUNK:
					if ( data.obj ) deletePerspectiveObject( data.obj );
					break;
				case OUTBOUND_DATA_TYPE_SET_CHUNK_VISIBLE:
					if ( data.obj ) data.obj->visible = true;
					break;
				case OUTBOUND_DATA_TYPE_SET_CHUNK_INVISIBLE:
					if ( data.obj ) data.obj->visible = false;
					break;
			}
			g_outbound[ i ].done = true;
			
		}

	}

	pthread_mutex_unlock( &g_outbound_mtx );
}

static void fetch_subject_position()
{
	g_quadtree_subject_position.x = g_cameraPosition[ 0 ];
	g_quadtree_subject_position.y = g_cameraPosition[ 1 ];
	g_quadtree_subject_position.z = g_cameraPosition[ 2 ];
}

/// terrain control

void push_generation_result( float x_pos, float z_pos, float size, PerspectiveObject* obj )
{
	push_inbound_data( x_pos, z_pos, size, obj );
}

void initialize_terrain()
{
	pthread_mutex_init( &g_quadtree_mtx, NULL );
	pthread_mutex_init( &g_inbound_mtx, NULL );
	pthread_mutex_init( &g_outbound_mtx, NULL );

	pthread_create( &g_quadtree_thread, NULL, quadtree_worker_thread, NULL );
}

void terminate_terrain()
{
	pthread_mutex_lock( &g_quadtree_mtx );
	g_quadtree_thread_running = false;
	pthread_mutex_unlock( &g_quadtree_mtx );

	pthread_join( g_quadtree_thread, NULL );

	pthread_mutex_destroy( &g_outbound_mtx );
	pthread_mutex_destroy( &g_inbound_mtx );
	pthread_mutex_destroy( &g_quadtree_mtx );
}

void poll_terrain()
{
	pthread_mutex_lock( &g_quadtree_mtx );
	fetch_subject_position();
	pthread_mutex_unlock( &g_quadtree_mtx );

	poll_outbound_data();
}


