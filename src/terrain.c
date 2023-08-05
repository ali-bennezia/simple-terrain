#include "terrain.h"

#include <cglm/cglm.h>

#include "objects.h"
#include "generator.h"

extern vec3 g_cameraPosition;

QuadTreeNode root;
float g_terrain_root_size = 5000;
float g_terrain_min_distance = 3536;
size_t max_level = 4;

void initialize_terrain()
{
	root.state = QTNS_NULL;
	root.children = NULL;
}

void terminate_terrain()
{

}

void convert_coords_level( int from_x, int from_z, size_t from_level, size_t to_level, int *result_x, int *result_z )
{
	float quotient = pow( 2, (int)to_level - (int)from_level );

	*result_x = floor( from_x * quotient );
	*result_z = floor( from_z * quotient );
}

size_t get_position_level( float x, float y, float z )
{
	Vec3fl diff = {
		x - g_cameraPosition[0],
		y - g_cameraPosition[1],
		z - g_cameraPosition[2]
	};
	float distance = vec3fl_magnitude( diff );

	float tested_distance = g_terrain_min_distance;
	int level = 0;
	
	while ( distance < tested_distance ){
		tested_distance /= 2.0;
		++level;
	}

	return min( level, max_level );	
}

size_t get_quad_level( int x_coord, int z_coord, size_t level )
{

	float quad_size = g_terrain_root_size / pow( 2, level );

	float x_pos = quad_size * x_coord + quad_size / 2.0;
	float z_pos = quad_size * z_coord + quad_size / 2.0;

	return get_position_level( x_pos, 0, z_pos );
}

boolval is_node_pending( QuadTreeNode *node )
{
	return ( node->state == QTNS_PENDING_WHILE_LOADED || node->state == QTNS_PENDING_WHILE_SUBDIVIDED || node->state == QTNS_PENDING_WHILE_NULL ) ? true : false;
}

/*
	Should generate a new, empty node, with children field set to NULL and state field set to QTNS_NULL.
*/
QuadTreeNode *generate_node( )
{
	QuadTreeNode *node = malloc( sizeof( QuadTreeNode ) );	
	node->state = QTNS_NULL;
	node->children = NULL;
	return node;
}

/*
	Should delete all of a node's children, set its children field to NULL, and set its state field to QTNS_NULL.	
*/
void clear_node_children( QuadTreeNode *node )
{
	switch ( node->state ){

		case QTNS_LOADED:
		case QTNS_PENDING_WHILE_LOADED:
			deletePerspectiveObject( (PerspectiveObject*) node->children );
			break;
		case QTNS_SUBDIVIDED:
		case QTNS_PENDING_WHILE_SUBDIVIDED:
			for ( size_t i = 0; i < 4; ++i )
			{
				QuadTreeNode *child_node = *( (QuadTreeNode**) node->children + i );
				clear_node_children( child_node );
				free( child_node );	
			}
			free( node->children );
			break;
		default:
			break;

	}

	node->children = NULL;
	node->state = QTNS_NULL;
}



/*
	Should delete all of a node's children, set its children to four new empty sub-node children, and set its state field to QTNS_SUBDIVIDED.
*/
void subdivide_node( QuadTreeNode *node )
{
	clear_node_children( node );
	node->state = QTNS_SUBDIVIDED;

	node->children = malloc( sizeof( QuadTreeNode* ) * 4 );

	for ( size_t i = 0 ; i < 4; ++i )
	{
		QuadTreeNode *child_node = generate_node();	
		*( (QuadTreeNode**) node->children + i ) = child_node;
	}
}

/*
	Looks for the node with given specified coordinates and level.
*/
QuadTreeNode *search_node( int x, int z, size_t level )
{
	QuadTreeNode *search_node = &root;
	for ( size_t search_level = 0; search_level <= level; ++search_level )
	{
		boolval last = ( search_level >= level ) ? true : false;

		if ( last == false ){

			if ( search_node->state != QTNS_SUBDIVIDED ) break;
			
			int next_level_x_coord, next_level_z_coord;
			convert_coords_level( x, z, level, search_level+1, &next_level_x_coord, &next_level_z_coord );
			int local_x_coord = next_level_x_coord % 2, local_z_coord = next_level_z_coord % 2;
			int local_index = local_z_coord * 2 + local_x_coord;
			search_node = *( (QuadTreeNode**) search_node->children + local_index );

		}else{
			return search_node;
		}

				
	}

	return NULL;
}

void request_node_generation( QuadTreeNode* node, int x_coord, int z_coord, size_t level )
{
	if ( is_node_pending( node ) ) return;

	enum QuadTreeNodeState previous_state = node->state;
	clear_node_children( node );

	switch ( previous_state ){

		case QTNS_NULL:
			node->state = QTNS_PENDING_WHILE_NULL;
			break;
		case QTNS_LOADED:
			node->state = QTNS_PENDING_WHILE_LOADED;
			break;
		case QTNS_SUBDIVIDED:
			node->state = QTNS_PENDING_WHILE_SUBDIVIDED;
			break;

	}

	request_generation( x_coord, z_coord, level, 8 );
}

void push_generation_result( int x, int z, size_t level, PerspectiveObject* obj )
{

	QuadTreeNode *node = search_node( x, z, level );
	if ( node == NULL || !( is_node_pending( node ) ) )
	{
		deletePerspectiveObject( obj );
	}else{
		clear_node_children( node );	
		node->state = QTNS_LOADED;
		node->children = obj;
	}	

}



void poll_node( QuadTreeNode *node, int x, int z, size_t level )
{
	size_t target_level = max( level, get_quad_level( x, z, level ) );

	if ( level < target_level ){
		
		if ( node->state != QTNS_SUBDIVIDED ){
			subdivide_node( node );
		}

		for ( size_t i = 0; i < 4; ++i )
		{
			int local_x_coord = i % 2;
			int local_z_coord = ( i - local_x_coord ) / 2;

			int child_x_coord = x * 2 + local_x_coord, child_z_coord = z * 2 + local_z_coord;

			poll_node( *( (QuadTreeNode**) node->children + i ), local_x_coord, local_z_coord, level + 1 );
		}

	}else{
		if ( node->state != QTNS_LOADED && !is_node_pending( node ) ){
			request_node_generation( node, x, z, level );
		}
	}	
}

void poll_terrain()
{
	poll_node( &root, 0, 0, 0 );
}
