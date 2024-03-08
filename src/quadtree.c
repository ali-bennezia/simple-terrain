#include "quadtree.h"

#include <string.h>
#include <math.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cglm/cglm.h>

#include "objects.h"
#include "generator.h"
#include "factory.h"
#include "noises.h"
#include "mempools.h"
#include "vbopools.h"
#include "boolvals.h"
#include "config.h"

#include "debug.h"

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
	boolval covered;
	struct Node *parent;
	struct Node *neighbors[ 4 ]; // N, E, S, W, +x -> eastwards, -z -> northwards,
	float *vertices_cache, *normals_cache;
	size_t stitchings[ 4 ];
} Node;

/// quadtree parameters

Node g_quadtree_root;
float g_quadtree_root_size = 10000;
float g_quadtree_min_distance = 20000;
size_t g_quadtree_max_level = 7;


// quadtree mutators prototypes

static Node *generate_node( Node *parent );
static void empty_node_cache( Node *node );
static void empty_node( Node *node );
static void delete_node( Node *node );
static void subdivide_node( Node *node );
static void remerge_node( Node *node );
static void request_node_terrain_generation( Node* node, int x_coord, int z_coord, size_t level );
static boolval are_node_immediate_children_covered( Node *node );
static boolval is_border_node( Node *node );
static void establish_node_coverage_chain( Node *node );
static PerspectiveObject *get_node_object( Node *node );
static void set_domain_boundary_visibility( boolval domain_root, boolval visibility, Node *node );
static void set_domain_root_boundary_visibility( boolval visibility, Node *node );
static void evaluate_child_node_neighbors( Node *parent_node, size_t child_index, Node **destination );
static int get_node_index( Node *node );
static int get_neighbor_opposite_direction( int dir );
static void update_node_neighbors( Node *node );
static boolval is_node_visible( Node *node );
static void evaluate_node_visible_neighbors( int x_coord, int z_coord, size_t level, Node **nodes_dest, size_t *levels_dest );
static void stitch_node_side( Node *node, int x_coord, int z_coord, size_t level, size_t side, size_t delta_level );

/// quadtree utilities

static void convert_coords_level( int from_x, int from_z, size_t from_level, size_t to_level, int *result_x, int *result_z )
{
	float quotient = pow( 2, (int)to_level - (int)from_level );

	*result_x = floor( from_x * quotient );
	*result_z = floor( from_z * quotient );
}

static Node *search_node( int x_coord, int z_coord, size_t level, boolval *terrain_present )
{
	int root_x_coord, root_z_coord;
	convert_coords_level( x_coord, z_coord, level, 0, &root_x_coord, &root_z_coord );	
	Node *recursive_node = root_x_coord == 0 && root_z_coord == 0 ? &g_quadtree_root : NULL;
	size_t recursive_level = 0;
	boolval found_terrain = false;
	while ( recursive_node )
	{
		if ( recursive_level < level )
		{
			if ( recursive_node->state == NODE_STATE_CHUNK ) found_terrain = true;
			if ( recursive_node->type != NODE_TYPE_MANIFOLD ){
				if ( terrain_present != NULL ) *terrain_present = found_terrain;
				return NULL;
			}
			
			++recursive_level;
			int child_x_coord, child_z_coord, child_local_x_coord, child_local_z_coord, child_local_index;
			convert_coords_level( x_coord, z_coord, level, recursive_level, &child_x_coord, &child_z_coord );
			child_local_x_coord = child_x_coord % 2; child_local_z_coord = child_z_coord % 2; child_local_index = child_local_z_coord * 2 + child_local_x_coord;
			recursive_node = *( ( Node** ) recursive_node->data + child_local_index );	
		}else{
			if ( terrain_present != NULL ) *terrain_present = found_terrain;
			if ( recursive_level == 0 ) {
				if ( x_coord == 0 && z_coord == 0 ) return recursive_node;
				else return NULL;
			}
			return recursive_node;
		}
	}

	if ( terrain_present != NULL ) *terrain_present = found_terrain;
	return NULL;
}

static Node *search_nearest_visible_node( int x_coord, int z_coord, size_t level, size_t *found_level )
{
	Node *nearest_visible_node = NULL;
	int root_x_coord, root_z_coord;
	convert_coords_level( x_coord, z_coord, level, 0, &root_x_coord, &root_z_coord );	
	Node *recursive_node = root_x_coord == 0 && root_z_coord == 0 ? &g_quadtree_root : NULL;
	size_t recursive_level = 0;
	while ( recursive_node )
	{
		if ( is_node_visible( recursive_node ) ) {
			nearest_visible_node = recursive_node;
			if ( found_level != NULL ) *found_level = recursive_level;
		}
		if ( recursive_level < level )
		{
			if ( recursive_node->type != NODE_TYPE_MANIFOLD ){
				return nearest_visible_node;
			}
			
			++recursive_level;
			int child_x_coord, child_z_coord, child_local_x_coord, child_local_z_coord, child_local_index;
			convert_coords_level( x_coord, z_coord, level, recursive_level, &child_x_coord, &child_z_coord );
			child_local_x_coord = child_x_coord % 2; child_local_z_coord = child_z_coord % 2; child_local_index = child_local_z_coord * 2 + child_local_x_coord;
			recursive_node = *( ( Node** ) recursive_node->data + child_local_index );	
		}else{
			if ( recursive_level == 0 ) {
				if ( x_coord == 0 && z_coord == 0 ) return nearest_visible_node;
				else return NULL;
			}
			return nearest_visible_node;
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

/*	Vec2fl diff = {
		x - g_cameraPosition[ 0 ],
		z - g_cameraPosition[ 2 ]
	};
	float distance = vec2fl_magnitude( diff );
*/

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

// generates a node
static Node *generate_node( Node *parent )
{
	//Node *node = malloc( sizeof( Node ) );
	Node *node = get_mem_pool_buffer( "Node" );
	node->type = NODE_TYPE_UNIQUE;
	node->state = NODE_STATE_EMPTY;
	node->data = NULL;
	node->covered = false;
	node->parent = parent;
	Node *empty_neighbors[ 4 ] = { NULL };
	memcpy( node->neighbors, empty_neighbors, sizeof( Node* ) * 4 );
	node->vertices_cache = NULL;
	node->normals_cache = NULL;
	size_t zero_stitchings[ 4 ] = { 0 };
	memcpy( node->stitchings, zero_stitchings, sizeof( size_t ) * 4 );

	return node;	
}

// frees up a node's vertices, or normals cache and sets them to NULL if it has either
static void empty_node_cache( Node *node )
{
	if ( node->vertices_cache != NULL ){
		free( node->vertices_cache );
		node->vertices_cache = NULL;
	}
	if ( node->normals_cache != NULL ){
		free( node->normals_cache );
		node->normals_cache = NULL;
	}
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
		//node->data = realloc( node->data, sizeof( Node* ) * 4 );
		void *new_data = get_mem_pool_buffer( "EmptyManifold" ), *old_data = node->data;
		memcpy( new_data, old_data, sizeof( Node * ) * 4 );
		node->data = new_data;
		yield_mem_pool_buffer( "ChunkManifold", old_data );
	}

	obj->meshInitialized = false;
	obj->normalsInitialized = false;
	obj->vertices = 0;
	yield_vbo_pool_buffer( "Quadtree", obj->meshVBO );
	yield_vbo_pool_buffer( "Quadtree", obj->normalsVBO );

	deletePerspectiveObject( obj );
	empty_node_cache( node );
	node->state = NODE_STATE_EMPTY;

	establish_node_coverage_chain( node );
}

// deletes a node, along with its children
static void delete_node( Node *node )
{
	if ( node->neighbors[ 0 ] != NULL && node->neighbors[ 0 ]->neighbors[ 2 ] != NULL ) node->neighbors[ 0 ]->neighbors[ 2 ] = NULL;
	if ( node->neighbors[ 1 ] != NULL && node->neighbors[ 1 ]->neighbors[ 3 ] != NULL ) node->neighbors[ 1 ]->neighbors[ 3 ] = NULL;
	if ( node->neighbors[ 2 ] != NULL && node->neighbors[ 2 ]->neighbors[ 0 ] != NULL ) node->neighbors[ 2 ]->neighbors[ 0 ] = NULL;
	if ( node->neighbors[ 3 ] != NULL && node->neighbors[ 3 ]->neighbors[ 1 ] != NULL ) node->neighbors[ 3 ]->neighbors[ 1 ] = NULL;

	empty_node( node );
	if ( node->type == NODE_TYPE_MANIFOLD )
	{
		for ( size_t i = 0; i < 4; ++i )
		{
			Node *child_node = *( ( Node** ) node->data + i );
			child_node->parent = NULL;
			delete_node( child_node );
			*( ( Node** ) node->data + i ) = NULL;
		}
		//free( node->data );
		if ( node->state == NODE_STATE_CHUNK ){
			yield_mem_pool_buffer( "ChunkManifold", node->data );
		}else{
			yield_mem_pool_buffer( "EmptyManifold", node->data );
		}
	} 
	//free( node );	
	yield_mem_pool_buffer( "Node", node );
}

// transforms a unique node into a manifold node
static void subdivide_node( Node *node )
{
	if ( node->type == NODE_TYPE_MANIFOLD ) return;
	node->type = NODE_TYPE_MANIFOLD;

	if ( node->state == NODE_STATE_CHUNK )
	{
		PerspectiveObject *obj = node->data; 
		//node->data = malloc( sizeof( Node* ) * 4 + sizeof( PerspectiveObject* ) );
		node->data = get_mem_pool_buffer( "ChunkManifold" );
		*( ( PerspectiveObject** ) ( ( Node** ) node->data + 4 ) ) = obj;
	}else{
		//node->data = malloc( sizeof( Node* ) * 4 );
		node->data = get_mem_pool_buffer( "EmptyManifold" );
	}
	
	for ( size_t i = 0; i < 4; ++i )
	{
		Node *child_node = generate_node( node );
		child_node->parent = node;
		*( ( Node** ) node->data + i ) = child_node;
	}

	for ( size_t i = 0; i < 4; ++i )
	{
		update_node_neighbors( *( ( Node** ) node->data + i ) );
	}

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
		child_node->parent = NULL;
		delete_node( child_node );
	}

	if ( node->state == NODE_STATE_CHUNK ){
		yield_mem_pool_buffer( "ChunkManifold", buffer );
	}else{
		yield_mem_pool_buffer( "EmptyManifold", buffer );
	}
	//free( buffer );
	node->type = NODE_TYPE_UNIQUE;
}

// transforms a node into a chunk node
void push_quadtree_chunk( int x_coord, int z_coord, size_t level, PerspectiveObject *obj, float *vertices, float *normals )
{
	boolval terrain_present = false;
	Node *node = search_node( x_coord, z_coord, level, &terrain_present );
	if ( !node || node->state != NODE_STATE_AWAITING ){
		deletePerspectiveObject( obj );
		return;
	}

	obj->visible = !terrain_present;

	if ( node->type == NODE_TYPE_UNIQUE )
	{
		node->data = obj;
	}else{
		node->data = realloc( node->data, sizeof( Node* ) * 4 + sizeof( PerspectiveObject* ) );
		*( ( PerspectiveObject** ) ( ( Node** ) node->data + 4 ) ) = obj;
	}
	
	node->state = NODE_STATE_CHUNK;
	node->vertices_cache = vertices;
	node->normals_cache = normals;

	establish_node_coverage_chain( node );

	if ( node->type == NODE_TYPE_MANIFOLD ){
		set_domain_root_boundary_visibility( false, node );
	}

	// debug

//	if ( obj->visible ){
		Node *visible_neighbors[ 4 ];
		size_t vn_levels[ 4 ];
		evaluate_node_visible_neighbors( x_coord, z_coord, level, visible_neighbors, vn_levels );
		if ( visible_neighbors[ 0 ] && vn_levels[ 0 ] < level ){
			stitch_node_side( node, x_coord, z_coord, level, 0, level - vn_levels[ 0 ] );
		}
//	}
}

// returns true if all of a node's immediate children have the covered flag set to true, false otherwise
static boolval are_node_immediate_children_covered( Node *node )
{
	if ( node->type != NODE_TYPE_MANIFOLD ) return false;
	Node **children = node->data;
	boolval result =  (
		(*( children + 0 ))->covered && 
		(*( children + 1 ))->covered && 
		(*( children + 2 ))->covered && 
		(*( children + 3 ))->covered 
	);
	return result;
}

static boolval is_border_node( Node *node )
{
	return ( node->type == NODE_TYPE_UNIQUE || node->state == NODE_STATE_CHUNK );
}

// sets the visiblities of a node's domain's boundary nodes
static void set_domain_boundary_visibility( boolval domain_root, boolval visibility, Node *node )
{	
	if ( node->state == NODE_STATE_CHUNK && domain_root == false )
	{
		get_node_object( node )->visible = visibility;	
	}else{
		if ( node->type == NODE_TYPE_MANIFOLD )
		{
			for ( size_t i = 0; i < 4; ++i )
			{
				set_domain_boundary_visibility(
					false, 
					visibility, 
					*( ( Node** ) node->data + i )
				);
			}	
		}	
	}


}

// sets the visiblities of a node's domain's boundary nodes
static void set_domain_root_boundary_visibility( boolval visibility, Node *node )
{
	set_domain_boundary_visibility( true, visibility, node );
}

// updates a node & its ancestor's covered flag
static void establish_node_coverage_chain( Node *node )
{
	if ( is_border_node( node ) ) {
		node->covered = ( node->state == NODE_STATE_CHUNK );
	}else{
		node->covered = are_node_immediate_children_covered( node );
	}

	Node *iteration = node->parent;
	size_t i = 0;
	while ( iteration != NULL )
	{
		++i;
		if ( iteration->state == NODE_STATE_CHUNK ) break; // break when attaining another domain, to avoid affecting it
		boolval previous_state = iteration->covered;
		iteration->covered = are_node_immediate_children_covered( iteration );
		if ( previous_state == iteration->covered ) break;
		iteration = iteration->parent;
	}	
}

// returns a node's PerspectiveObject if it contains one, NULL otherwise
static PerspectiveObject *get_node_object( Node *node )
{
	if ( node->state != NODE_STATE_CHUNK ) return NULL;
	
	if ( node->type == NODE_TYPE_UNIQUE ){
		return node->data;
	}else{
		return *( ( PerspectiveObject** ) ( ( Node** ) node->data + 4 ) );
	}

}

// sets a node in an awaiting state, and requests terrain generation for it
static void request_node_terrain_generation( Node* node, int x_coord, int z_coord, size_t level )
{
	if ( node->state != NODE_STATE_EMPTY ) return;
	boolval result = request_generation( x_coord, z_coord, level, TESSELLATIONS );
	if ( !result ){
		node->state = NODE_STATE_AWAITING;	
	}
}

// gives the same-level neighbors for a child node specified using a parent node and the child node's index
static void evaluate_child_node_neighbors( Node *parent_node, size_t child_index, Node **destination )
{
	int child_x_coord = child_index % 2;
	int child_z_coord = ( child_index - child_x_coord ) / 2;

	if ( parent_node == NULL ){
		Node **empty_neighbors[ 4 ] = { NULL };
		memcpy( destination, empty_neighbors, sizeof( Node* ) * 4 );
		return;
	}

	Node **parent_neighbors = parent_node->neighbors;

	destination[ 0 ] = child_z_coord == 0 ? 
			( 
				( parent_neighbors[ 0 ] != NULL && parent_neighbors[ 0 ]->type == NODE_TYPE_MANIFOLD )
				? *( ( Node** ) parent_neighbors[ 0 ]->data + 2 + child_x_coord ) 
				: NULL 
			) 
		: *( ( Node** ) parent_node->data + child_x_coord );

	destination[ 1 ] = child_x_coord == 1 ? 
			( 
				( parent_neighbors[ 1 ] != NULL && parent_neighbors[ 1 ]->type == NODE_TYPE_MANIFOLD )
				? *( ( Node** ) parent_neighbors[ 1 ]->data + 2 * child_z_coord ) 
				: NULL 
			) 
		: *( ( Node** ) parent_node->data + child_index + 1 );

	destination[ 2 ] = child_z_coord == 1 ? 
			( 
				( parent_neighbors[ 2 ] != NULL && parent_neighbors[ 2 ]->type == NODE_TYPE_MANIFOLD )
				? *( ( Node** ) parent_neighbors[ 2 ]->data + child_x_coord ) 
				: NULL 
			) 
		: *( ( Node** ) parent_node->data + 2 + child_x_coord );

	destination[ 3 ] = child_x_coord == 0 ? 
			( 
				( parent_neighbors[ 3 ] != NULL && parent_neighbors[ 3 ]->type == NODE_TYPE_MANIFOLD )
				? *( ( Node** ) parent_neighbors[ 3 ]->data + 2 * child_z_coord + 1 ) 
				: NULL 
			) 
		: *( ( Node** ) parent_node->data + child_index - 1 );
}

// returns the node's child index, -1 if error.
static int get_node_index( Node *node )
{
	Node *parent = node->parent;
	if ( parent == NULL || parent->type != NODE_TYPE_MANIFOLD ) return -1;
	for ( size_t i = 0; i < 4; ++i )
	{
		if ( *( ( Node** ) parent->data + i ) == node  ) return i;
	}	
	return -1;
}

// returns the neighbor index representing the opposite direction
static int get_neighbor_opposite_direction( int dir )
{
	return ( dir + 2 ) % 4;
}

// updates a node's neighbor's and its neighbor's information
static void update_node_neighbors( Node *node )
{
	evaluate_child_node_neighbors( node->parent, get_node_index( node ), node->neighbors );	

	for ( size_t i = 0; i < 4; ++i )
	{
		Node *neighbor = node->neighbors[ i ];
		if ( neighbor != NULL ){
			neighbor->neighbors[ get_neighbor_opposite_direction( i ) ] = node;
		}
	}
}

// returns true if a node contains a visible perspective object, false otherwise
static boolval is_node_visible( Node *node )
{
	return !( !node || node->state != NODE_STATE_CHUNK || get_node_object( node )->visible == false );
}

// returns adjacent visible nodes
static void evaluate_node_visible_neighbors( int x_coord, int z_coord, size_t level, Node **nodes_dest, size_t *levels_dest )
{
	size_t rlevels[ 4 ] = { 0 };
	Node *result[ 4 ] = { NULL };

	result[ 0 ] = search_nearest_visible_node( x_coord, z_coord - 1, level, &rlevels[ 0 ] );
	result[ 1 ] = search_nearest_visible_node( x_coord + 1, z_coord, level, &rlevels[ 1 ] );
	result[ 2 ] = search_nearest_visible_node( x_coord, z_coord + 1, level, &rlevels[ 2 ] );
	result[ 3 ] = search_nearest_visible_node( x_coord - 1, z_coord, level, &rlevels[ 3 ] );

	if ( nodes_dest != NULL ) memcpy( nodes_dest, result, sizeof( Node* ) * 4 );
	if ( levels_dest != NULL ) memcpy( levels_dest, rlevels, sizeof( size_t ) * 4 );
}

// stitches a node's given side
static void stitch_node_side( Node *node, int x_coord, int z_coord, size_t level, size_t side, size_t delta_level )
{
	if ( node->state != NODE_STATE_CHUNK ) return;
	side %= 4;
	PerspectiveObject *obj = get_node_object( node );

	glBindBuffer( GL_ARRAY_BUFFER, obj->meshVBO );	

	size_t quads_per_side = 1 * pow( 2, level ), 
		start_q, q_next, v1, v2, v3,
		terrain_quads_per_side_quad = pow( 2, delta_level );
	boolval pairing = false;

	// v values are vertex indices going on the side from left to right, top to bottom, and mapped on the quad's vertices

	switch (side)
	{
		case 0:
			start_q = quads_per_side * ( quads_per_side - 1 );
			q_next = 1;
			// v1, { v2 | v3 }
			pairing = true;
			v1 = 1;
			v2 = 2;
			v3 = 4;
			break;

		case 1:
			start_q = quads_per_side - 1;
			q_next = quads_per_side;
			// v1, { v2 | v3 }
			pairing = true;
			v1 = 5;
			v2 = 4;
			v3 = 2;
			break;
		case 2:
			start_q = 0;
			q_next = 1;
			// { v1 | v2 }, v3
			pairing = false;
			v1 = 0;
			v2 = 3;
			v3 = 5;
			break;
		default:
			start_q = 0;
			q_next = quads_per_side;
			// { v1 | v2 }, v3
			pairing = false;
			v1 = 0;
			v2 = 3;
			v3 = 1;
			break;
	}

	// quad index, mapped from NW to SE


	for ( size_t q = 0; q < quads_per_side; ++q )
	{
		size_t local_q_side_index = q % terrain_quads_per_side_quad,
			q_slice_side_first = q - local_q_side_index, 
			q_slice_side_last = q_slice_side_first + terrain_quads_per_side_quad - 1;

		size_t q_slice_first_index = start_q + q_slice_side_first * q_next,
			q_slice_last_index = start_q + q_slice_side_last * q_next,
			q_index = start_q + q * q_next;

		size_t vertex_slice_first_index = q_slice_first_index * 6 + v1,
			vertex_slice_last_index = q_slice_last_index * 6 + v3;

		Vec3fl vertex_slice_first = *( ( Vec3fl* ) node->vertices_cache + vertex_slice_first_index ),
			vertex_slice_last = *( ( Vec3fl* ) node->vertices_cache + vertex_slice_last_index );

		float vertex_slice_first_distance_quotient = ( 0.0f + ( float ) q ) / ( float ) quads_per_side;
		float vertex_slice_second_distance_quotient = ( 1.0f + ( float ) q ) / ( float ) quads_per_side;

		float first_vert_height = vertex_slice_first_distance_quotient * vertex_slice_last.y +
					( 1.0f - vertex_slice_first_distance_quotient ) * vertex_slice_first.y;

		float second_vert_height = vertex_slice_second_distance_quotient * vertex_slice_last.y +
					( 1.0f - vertex_slice_second_distance_quotient ) * vertex_slice_first.y;

		size_t q_slice_first_vertex_index = q_slice_first_index * 6,
			q_slice_last_vertex_index = q_slice_last_index * 6;

		if ( pairing ){
			size_t first_vert_index_1 = q_slice_first_vertex_index + v1,
				first_vert_index_2 = q_slice_first_vertex_index * 6 + v2,
				last_vert_index = q_slice_last_vertex_index * 6 + v3;

			size_t first_vert_offset_1 = first_vert_index_1 * sizeof( float ) * 3,
				first_vert_offset_2 = first_vert_index_2 * sizeof( float ) * 3,
				last_vert_offset = last_vert_index * sizeof( float ) * 3;

			size_t first_float_offset_1 = first_vert_offset_1 + sizeof( float ),
				first_float_offset_2 = first_vert_offset_2 + sizeof( float ),
				last_float_offset = last_vert_offset + sizeof( float );

			glBindBuffer( GL_ARRAY_BUFFER, obj->meshVBO );
			glBufferSubData( GL_ARRAY_BUFFER, first_float_offset_1, sizeof( float ), &first_vert_height );
			glBufferSubData( GL_ARRAY_BUFFER, first_float_offset_2, sizeof( float ), &first_vert_height );
			glBufferSubData( GL_ARRAY_BUFFER, last_float_offset, sizeof( float ), &second_vert_height );
		}else{
			size_t first_vert_index = q_slice_first_vertex_index * 6 + v1,
				last_vert_index_1 = q_slice_last_vertex_index * 6 + v2,
				last_vert_index_2 = q_slice_last_vertex_index * 6 + v3;

			size_t first_vert_offset = first_vert_index * sizeof( float ) * 3,
				last_vert_offset_1 = last_vert_index_1 * sizeof( float ) * 3,
				last_vert_offset_2 = last_vert_index_2 * sizeof( float ) * 3;

			size_t first_float_offset = first_vert_offset + sizeof( float ),
				last_float_offset_1 = last_vert_offset_1 + sizeof( float ),
				last_float_offset_2 = last_vert_offset_2 + sizeof( float );

			glBindBuffer( GL_ARRAY_BUFFER, obj->meshVBO );
			glBufferSubData( GL_ARRAY_BUFFER, first_float_offset, sizeof( float ), &first_vert_height );
			glBufferSubData( GL_ARRAY_BUFFER, last_float_offset_1, sizeof( float ), &second_vert_height );
			glBufferSubData( GL_ARRAY_BUFFER, last_float_offset_2, sizeof( float ), &second_vert_height );
		}

	}

	//TODO		
}

/// terrain control

static void poll_node( Node *node, int x_coord, int z_coord, size_t level, boolval top_covered )
{
	size_t target_level = get_quad_level( x_coord, z_coord, level );

	if ( level < target_level )
	{
		if ( node->type != NODE_TYPE_MANIFOLD ){
			subdivide_node( node );
		}

		if ( 
			node->state == NODE_STATE_CHUNK  &&
			are_node_immediate_children_covered( node )
		 ){
			empty_node( node );
			set_domain_root_boundary_visibility( !top_covered, node );
		}
		
		for ( size_t i = 0; i < 4; ++i )
		{
			Node *child_node = *( ( Node** ) node->data + i );
			int child_x_coord = i % 2;
			int child_z_coord = ( i - child_x_coord ) / 2;

			poll_node( 
				child_node, 
				x_coord * 2 + child_x_coord, 
				z_coord * 2 + child_z_coord, 
				level + 1, 
				top_covered || node->state == NODE_STATE_CHUNK
			);
		}
	}else if ( level == target_level ) {
		if ( node->type == NODE_TYPE_MANIFOLD && node->state == NODE_STATE_CHUNK ){
			remerge_node( node );
		}
		if ( node->state == NODE_STATE_EMPTY ){
			request_node_terrain_generation( node, x_coord, z_coord, level );
		}
	}
}

void initialize_quadtree()
{
	Node empty_node = {
		NODE_TYPE_UNIQUE,
		NODE_STATE_EMPTY,
		NULL,
		false,
		NULL,
		{ NULL },
		NULL, NULL,
		{ 0 }
	};
	g_quadtree_root = empty_node;
	
	gen_mem_pool( "Node", sizeof( Node ) );
	gen_mem_pool( "EmptyManifold", sizeof( Node* ) * 4 );
	gen_mem_pool( "ChunkManifold", sizeof( Node* ) * 4 + sizeof( PerspectiveObject* ) );

	gen_vbo_pool( "Quadtree", sizeof( float ) * 3 * QUAD_COUNT * 6 );
}

void terminate_quadtree()
{
	remove_vbo_pool( "Quadtree" );

	remove_mem_pool( "ChunkManifold" );
	remove_mem_pool( "EmptyManifold" );
	remove_mem_pool( "Node" );
}

void poll_quadtree()
{
	poll_node( &g_quadtree_root, 0, 0, 0, false );
}


