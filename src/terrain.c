#include "terrain.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cglm/cglm.h>

#include "objects.h"
#include "generator.h"
#include "factory.h"
#include "noises.h"
#include "pools.h"

#define MAX_TRANSFERTS 80
const int QUAD_COUNT = 1 * pow( 2, TESSELLATIONS*2 );


/// externs

extern Material g_defaultTerrainMaterialLit;
extern vec3 g_cameraPosition;

/// definitions

enum NodeType {
	NODE_TYPE_UNIQUE,
	NODE_TYPE_MANIFOLD
};

enum NodeState {
	NODE_STATE_EMPTY,
	NODE_STATE_AWAITING,
	NODE_STATE_CHUNK
};

typedef struct Node {
	enum NodeType type;
	enum NodeState state;
	void *data;	
} Node;

/// quadtree parameters

Node g_quadtree_root;
float g_quadtree_root_size = 10000;
float g_quadtree_min_distance = 15000;
size_t g_quadtree_max_level = 7;


/// quadtree utilities

static void convert_coords_level( int from_x, int from_z, size_t from_level, size_t to_level, int *result_x, int *result_z )
{
	float quotient = pow( 2, (int)to_level - (int)from_level );

	*result_x = floor( from_x * quotient );
	*result_z = floor( from_z * quotient );
}

static Node *search_node( int x_coord, int z_coord, size_t level )
{
	Node *recursive_node = &g_quadtree_root;
	size_t recursive_level = 0;
	while ( recursive_node )
	{
		if ( recursive_level < level )
		{
			if ( recursive_node->type != NODE_TYPE_MANIFOLD ) return NULL;
			
			++recursive_level;
			int child_x_coord, child_z_coord, child_local_x_coord, child_local_z_coord, child_local_index;
			convert_coords_level( x_coord, z_coord, level, recursive_level, &child_x_coord, &child_z_coord );
			child_local_x_coord = child_x_coord % 2; child_local_z_coord = child_z_coord % 2; child_local_index = child_local_z_coord * 2 + child_local_x_coord;
			recursive_node = *( ( Node** ) recursive_node->data + child_local_index );	
		}else{
			if ( recursive_level == 0 ) {
				if ( x_coord == 0 && z_coord == 0 ) return recursive_node;
				else return NULL;
			}
			return recursive_node;
		}
	}
	return NULL;
}

static size_t get_position_level( float x, float y, float z )
{
	Vec3fl diff = {
		x - g_cameraPosition[0],
		y - g_cameraPosition[1],
		z - g_cameraPosition[2]
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

/// quadtree mutators

static Node *generate_node();
static void empty_node( Node *node );
static void delete_node( Node *node );
static void subdivide_node( Node *node );
static void remerge_node( Node *node );
static void request_node_terrain_generation( Node* node, int x_coord, int z_coord, size_t level );

// generates a node
static Node *generate_node()
{
	Node *node = malloc( sizeof( Node ) );
	node->type = NODE_TYPE_UNIQUE;
	node->state = NODE_STATE_EMPTY;
	node->data = NULL;
	return node;	
}

// turns a node into an empty node, by deleting its perspective obj. if it has one
static void empty_node( Node *node )
{
	if ( node->state == NODE_STATE_AWAITING ) {
		node->state = NODE_STATE_EMPTY;
		return;
	}else if ( node->state == NODE_STATE_EMPTY ) return;
	PerspectiveObject *obj = NULL;
	if ( node->type == NODE_TYPE_UNIQUE )
	{
		obj = node->data;
		node->data = NULL;
	}else{
		obj = *( ( PerspectiveObject** ) ( ( Node** ) node->data + 4 ) );
		node->data = realloc( node->data, sizeof( Node* ) * 4 );
	}
	deletePerspectiveObject( obj );
	node->state = NODE_STATE_EMPTY;
}

// deletes a node, along with its children
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
	if ( node->type == NODE_TYPE_MANIFOLD ) return;

	if ( node->state == NODE_STATE_CHUNK )
	{
		PerspectiveObject *obj = node->data; 
		node->data = malloc( sizeof( Node* ) * 4 + sizeof( PerspectiveObject* ) );
		*( ( PerspectiveObject** ) ( ( Node** ) node->data + 4 ) ) = obj;
	}else{
		node->data = malloc( sizeof( Node* ) * 4 );
	}
	
	for ( size_t i = 0; i < 4; ++i )
	{
		Node *child_node = generate_node();
		*( ( Node** ) node->data + i ) = child_node;
	}

	node->type = NODE_TYPE_MANIFOLD;
}

// transforms a manifold node into a unique node
static void remerge_node( Node *node )
{
	if ( node->type != NODE_TYPE_MANIFOLD ) return;

	void *buffer = node->data;

	if ( node->state == NODE_STATE_CHUNK )
	{
		PerspectiveObject *obj = *( ( PerspectiveObject** ) ( ( Node** ) node->data + 4 ) );
		node->data = obj; 
	}else{
		node->data = NULL;
	}

	for ( size_t i = 0; i < 4; ++i )
	{
		Node *child_node = *( ( Node** ) buffer + i );
		delete_node( child_node );
	}

	free( buffer );
	node->type = NODE_TYPE_UNIQUE;
}

// transforms a node into a chunk node
void push_node_terrain( int x_coord, int z_coord, size_t level, PerspectiveObject *obj )
{
	Node *node = search_node( x_coord, z_coord, level );
	if ( !node || node->state != NODE_STATE_AWAITING ){
		deletePerspectiveObject( obj );
		return;
	}

	if ( node->type == NODE_TYPE_UNIQUE )
	{
		node->data = obj;
	}else{
		node->data = realloc( node->data, sizeof( Node* ) * 4 + sizeof( PerspectiveObject* ) );
		*( ( PerspectiveObject** ) ( ( Node** ) node->data + 4 ) ) = obj;
	}
	
	node->state = NODE_STATE_CHUNK;
}

// sets a node in an awaiting state, and requests terrain generation for it
static void request_node_terrain_generation( Node* node, int x_coord, int z_coord, size_t level )
{
	if ( node->state != NODE_STATE_EMPTY ) return;
	node->state = NODE_STATE_AWAITING;	
	request_generation( x_coord, z_coord, level, TESSELLATIONS );
}

/// terrain control

static void poll_node( Node *node, int x_coord, int z_coord, size_t level )
{
	size_t target_level = get_quad_level( x_coord, z_coord, level );

	if ( level < target_level )
	{
		if ( node->type != NODE_TYPE_MANIFOLD ){
			subdivide_node( node );
		}

		if ( node->state == NODE_STATE_CHUNK || node->state == NODE_STATE_AWAITING )
			empty_node( node );

		for ( size_t i = 0; i < 4; ++i )
		{
			Node *child_node = *( ( Node** ) node->data + i );
			int child_x_coord = i % 2;
			int child_z_coord = ( i - child_x_coord ) / 2;
			poll_node( child_node, x_coord * 2 + child_x_coord, z_coord * 2 + child_z_coord, level + 1 );
		}

	}else {
		if ( node->type == NODE_TYPE_MANIFOLD )
			remerge_node( node );

		if ( node->state == NODE_STATE_EMPTY )
			request_node_terrain_generation( node, x_coord, z_coord, level );
	}

}


void initialize_terrain()
{
	Node empty_node = {
		NODE_TYPE_UNIQUE,
		NODE_STATE_EMPTY,
		NULL
	};
	g_quadtree_root = empty_node;
	
	gen_pool( "Quadtree", sizeof( float ) * 3 * QUAD_COUNT * 6 );
}

void terminate_terrain()
{
	remove_pool( "Quadtree" );
}

void poll_terrain()
{
	poll_node( &g_quadtree_root, 0, 0, 0 );
}


