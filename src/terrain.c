#include "terrain.h"

#include <cglm/cglm.h>

#include "objects.h"
#include "generator.h"

extern vec3 g_cameraPosition;

QuadTreeNode root;
float g_terrain_root_size = 10000;
float g_terrain_min_distance = 15000;
size_t max_level = 8;

void initialize_terrain()
{
	root.state = QTNS_NULL;
	root.children = NULL;
	root.parent = NULL;
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
	return ( node->state == QTNS_PENDING_WHILE_NULL || node->state == QTNS_PENDING_WHILE_SUBDIVIDED ) ? true : false;
}

boolval is_node_subdivided( QuadTreeNode *node )
{
	return ( node->state == QTNS_SUBDIVIDED || node->state == QTNS_PENDING_WHILE_SUBDIVIDED || node->state == QTNS_LOADED_AND_SUBDIVIDED );
}

boolval is_node_loaded( QuadTreeNode *node )
{
	return ( node->state == QTNS_LOADED || node->state == QTNS_LOADED_AND_SUBDIVIDED );
}

// True if all of this node's lowest level child nodes are loaded.
boolval is_node_covered( QuadTreeNode *node, boolval check_root )
{
	if ( node->state == QTNS_LOADED || check_root == true && node->state == QTNS_LOADED_AND_SUBDIVIDED ){
		return true;
	}else if ( is_node_subdivided( node ) ){
		return (
			is_node_covered( *( ( QuadTreeNode** ) node->children ), true ) &&
			is_node_covered( *( ( QuadTreeNode** ) node->children + 1 ), true ) &&	
			is_node_covered( *( ( QuadTreeNode** ) node->children + 2 ), true ) &&	
			is_node_covered( *( ( QuadTreeNode** ) node->children + 3 ), true )	
		);
	}
	else return false;
}

// Set all of the children covering this node's quad's visibilities
void set_covering_children_visibility( QuadTreeNode *node, boolval visibility, boolval affect_root )
{
	if ( affect_root && node->state == QTNS_LOADED )
	{
		( ( PerspectiveObject* ) node->children )->visible = visibility;
	}else if ( affect_root && node->state == QTNS_LOADED_AND_SUBDIVIDED )
	{
	  	( *( ( PerspectiveObject** ) ( ( QuadTreeNode** ) node->children + 4 ) ) )->visible = visibility;
	}else if ( node->state == QTNS_SUBDIVIDED )
	{
		set_covering_children_visibility( *( ( QuadTreeNode** ) node->children ), visibility, true );
		set_covering_children_visibility( *( ( QuadTreeNode** ) node->children + 1 ), visibility, true );
		set_covering_children_visibility( *( ( QuadTreeNode** ) node->children + 2 ), visibility, true );
		set_covering_children_visibility( *( ( QuadTreeNode** ) node->children + 3 ), visibility, true );
	}
}

boolval should_node_be_hidden( QuadTreeNode *node )
{
	QuadTreeNode *ancestor = node;

	while ( ( ancestor = ancestor->parent ) )
	{
		if ( ancestor->state == QTNS_LOADED_AND_SUBDIVIDED ) return true;
	}
	return false;
}

/*
	Should generate a new, empty node, with parent, children fields set to NULL and state field set to QTNS_NULL.
*/
QuadTreeNode *generate_node( )
{
	QuadTreeNode *node = malloc( sizeof( QuadTreeNode ) );	
	node->state = QTNS_NULL;
	node->parent = NULL;
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
			deletePerspectiveObject( (PerspectiveObject*) node->children );
			break;
		case QTNS_SUBDIVIDED:
		case QTNS_PENDING_WHILE_SUBDIVIDED:
			for ( size_t i = 0; i < 4; ++i )
			{
				QuadTreeNode *child_node = *( (QuadTreeNode**)node->children + i );
				clear_node_children( child_node );
				free( child_node );	
			}
			free( node->children );
			break;
		case QTNS_LOADED_AND_SUBDIVIDED:
			for ( size_t i = 0; i < 4; ++i )
			{
				QuadTreeNode *child_node = *( (QuadTreeNode**)node->children + i );
				clear_node_children( child_node );
				free( child_node );	
			}
			deletePerspectiveObject( *( ( PerspectiveObject** ) ( ( QuadTreeNode** ) node->children + 4 ) ) );
			free( node->children );
			break;

	}

	node->children = NULL;
	node->state = QTNS_NULL;
}

/*
	Checks if a node under the QTNS_LOADED_AND_SUBDIVIDED state must be mutated to the QTNS_SUBDIVIDED state.
	If so, deletes its perspective object and turns all children that covers its quad surface visible.
*/
void check_node_for_mutation( QuadTreeNode *node )
{
	if ( node->state != QTNS_LOADED_AND_SUBDIVIDED || !is_node_covered( node, false ) ) return;

	PerspectiveObject *obj = *( ( PerspectiveObject** ) ( ( QuadTreeNode** ) node->children + 4 ) );
	deletePerspectiveObject( obj );

	node->children = realloc( node->children, sizeof( QuadTreeNode* ) * 4 );
	node->state = QTNS_SUBDIVIDED;
	set_covering_children_visibility( node, true, false );

	check_node_ancestry_for_mutation( node );
}

/*
	Checks for a possible node mutation in a node's ancestry & executes it if found.
*/
void check_node_ancestry_for_mutation( QuadTreeNode *node )
{
	QuadTreeNode *ancestor = node;

	while ( ( ancestor = ancestor->parent ) )
	{
		if ( ancestor->state == QTNS_LOADED_AND_SUBDIVIDED )
		{
			check_node_for_mutation( ancestor );	
			return;
		}
	}
}

/*
	Transforms a node from the state QTNS_LOADED_AND_SUBDIVIDED to the state QTNS_LOADED.
	Delete its child nodes.
*/
void regroup_node( QuadTreeNode *node )
{
	if ( node->state != QTNS_LOADED_AND_SUBDIVIDED ) return;

	PerspectiveObject *obj = *( ( PerspectiveObject** ) ( ( QuadTreeNode** ) node->children + 4 ) );

	for ( size_t i = 0; i < 4; ++i )
	{
		QuadTreeNode *child_node = *( (QuadTreeNode**)node->children + i );
		clear_node_children( child_node );
		free( child_node );	
	}
	free( node->children );
	node->children = obj;
	obj->visible = !should_node_be_hidden( node );
	node->state = QTNS_LOADED;
}

/*
	Should delete all of a node's children, set its children to four new empty sub-node children, and set its state field to QTNS_SUBDIVIDED.
	If the node's previous state was QTNS_LOADED, however, it should set its children pointer as a buffer containing all four new sub-node children along with the
	perspective object which it contained pre-subdivision, and set its state to QTNS_LOADED_AND_SUBDIVIDED.
*/
void subdivide_node( QuadTreeNode *node )
{
	if ( node->state != QTNS_LOADED )
	{
		clear_node_children( node );
		node->state = QTNS_SUBDIVIDED;
		node->children = malloc( sizeof( QuadTreeNode* ) * 4 );
	}else{
		PerspectiveObject *obj = ( PerspectiveObject* ) node->children;
		node->state = QTNS_LOADED_AND_SUBDIVIDED;
		node->children = malloc( sizeof( QuadTreeNode* ) * 4 + sizeof( PerspectiveObject* ) );	
		*( ( PerspectiveObject** ) ( ( QuadTreeNode** ) node->children + 4 ) ) = obj;
		obj->visible = !should_node_be_hidden( node );
	}

	for ( size_t i = 0 ; i < 4; ++i )
	{
		QuadTreeNode *child_node = generate_node();
		child_node->parent = node;	
		*( ( QuadTreeNode** ) node->children + i ) = child_node;
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

			if ( !is_node_subdivided( search_node ) ) break;
			
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
	if ( is_node_pending( node ) || is_node_loaded( node ) ) return;

	enum QuadTreeNodeState previous_state = node->state;

	switch ( previous_state ){

		case QTNS_NULL:
			node->state = QTNS_PENDING_WHILE_NULL;
			break;
		case QTNS_SUBDIVIDED:
			node->state = QTNS_PENDING_WHILE_SUBDIVIDED;
			break;

	}

	request_generation( x_coord, z_coord, level, 5 );
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
		obj->visible = !should_node_be_hidden( node );

		check_node_ancestry_for_mutation( node );
	}	

}

void poll_node( QuadTreeNode *node, int x, int z, size_t level )
{
	size_t target_level = max( level, get_quad_level( x, z, level ) );

	if ( level < target_level ){
	
		if ( !is_node_subdivided( node ) ){
			subdivide_node( node );
		}

		for ( size_t i = 0; i < 4; ++i )
		{
			int local_x_coord = i % 2;
			int local_z_coord = ( i - local_x_coord ) / 2;

			int child_x_coord = x * 2 + local_x_coord, child_z_coord = z * 2 + local_z_coord;

			poll_node( *( (QuadTreeNode**) node->children + i ), child_x_coord, child_z_coord, level + 1 );
		}

	}else{
		if ( !is_node_loaded( node ) && !is_node_pending( node ) ){
			request_node_generation( node, x, z, level );
		}else if ( node->state == QTNS_LOADED_AND_SUBDIVIDED )
		{
			regroup_node( node );
		}
	}	

}

void poll_terrain()
{
	poll_node( &root, 0, 0, 0 );
}
