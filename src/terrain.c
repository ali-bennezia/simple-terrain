#include "terrain.h"

#include <cglm/cglm.h>

#include "objects.h"
#include "generator.h"

extern vec3 g_cameraPosition;

QuadTreeNode root;
float g_terrain_root_size = 5000;
float g_terrain_min_distance = 354;

void initialize_terrain()
{
	root.state = QTNS_NULL;
	root.children = NULL;
}

void terminate_terrain()
{

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

	return level;	
}

void subdivide_node( QuadTreeNode *node )
{
	if ( !( node->state == QTNS_SUBDIVIDED ) ){

		delete_node_children( node );
		node->state = QTNS_SUBDIVIDED;
		node->children = malloc( sizeof( QuadTreeNode* ) * 4 );	

		for ( size_t i = 0; i < 4; ++i ){

			QuadTreeNode *new_node = calloc( 1, sizeof( QuadTreeNode ) );
			new_node->state = QTNS_NULL;
			new_node->children = NULL;
			*( ( QuadTreeNode** ) node->children + i ) = new_node;
	
		}

	}
}

void delete_node_children( QuadTreeNode *node )
{
	if ( node->state != QTNS_NULL && node->children != NULL )
	{
		if ( node->state == QTNS_SUBDIVIDED ){
		
			for ( size_t i = 0; i < 4; ++i ){
			
				delete_node_children( ( ( QuadTreeNode* ) node->children ) + i );
				free( *( ( ( QuadTreeNode** ) node->children ) + i ) );

			}

			if ( node->children != NULL ) free( node->children );

		}else if ( node->state == QTNS_LOADED_SINGULAR || node->state == QTNS_PENDING ){

			deletePerspectiveObject( (PerspectiveObject*) node->children );

		}

		node->state = QTNS_NULL;
		node->children = NULL;
	}
}

void request_terrain_generation( QuadTreeNode* node, int x_coord, int z_coord, size_t level )
{
	if ( !( node->state == QTNS_LOADED_SINGULAR || node->state == QTNS_PENDING ) ){
		delete_node_children( node );	
		node->state = QTNS_PENDING;
		request_generation( x_coord, z_coord, level, 500 );
	}
}

void convert_coords_level( int from_x, int from_z, size_t from_level, size_t to_level, int *result_x, int *result_z )
{
	float quotient = pow( 2, (int)to_level - (int)from_level );

	*result_x = ceil( from_x * quotient );
	*result_z = ceil( from_z * quotient );
}

void push_generation_result( int x, int z, size_t level, PerspectiveObject* obj )
{

	QuadTreeNode *node_iteration = &root; 

	for ( size_t current_level = 0; current_level <= level; ++current_level )
	{
		boolval last = ( current_level == level );

		if ( last == false ){

			if ( node_iteration->state != QTNS_SUBDIVIDED){
				subdivide_node( node_iteration );
			}

			int current_level_x, current_level_z;

			convert_coords_level( x, z, level, current_level, &current_level_x, &current_level_z );

			int local_coord_x = current_level_x % 2, local_coord_z = current_level_z % 2;
			int local_index = z*2 + x;	
		
			node_iteration = *( (QuadTreeNode**) node_iteration->children + local_index );
	
		}else{

			if ( node_iteration->state == QTNS_LOADED_SINGULAR ){
				deletePerspectiveObject( obj );
			}else{
				delete_node_children( node_iteration );
				node_iteration->state = QTNS_LOADED_SINGULAR;
				node_iteration->children = obj;	
			}
	
		}

	}

}

void poll_qtn( QuadTreeNode *node, int x, int z, size_t level )
{
	float quad_level_size = g_terrain_root_size / pow( 2, level );
	float quad_middle_pos_x = x*quad_level_size + quad_level_size / 2.0, quad_middle_pos_z = z*quad_level_size + quad_level_size / 2.0;
	size_t target_level = get_position_level( quad_middle_pos_x, 0, quad_middle_pos_z );
	
	if ( target_level == level ){

		if ( !( node->state == QTNS_LOADED_SINGULAR || node->state == QTNS_PENDING ) ){

			delete_node_children( node );
			request_terrain_generation( node, x, z, level );	

		}

	}else if ( target_level > level ){

		if ( !( node->state == QTNS_SUBDIVIDED ) ){

			delete_node_children( node );
			node->state = QTNS_SUBDIVIDED;
			node->children = malloc( sizeof( QuadTreeNode* ) * 4 );	

			for ( size_t i = 0; i < 4; ++i ){

				int child_x = x * 2 + ( i % 2 );
				int child_z = z * 2 + ( i - child_x ) / 2;

				QuadTreeNode *new_node = calloc( 1, sizeof( QuadTreeNode ) );
				new_node->state = QTNS_NULL;
				new_node->children = NULL;
				*( ( QuadTreeNode** ) node->children + i ) = new_node;
	
				poll_qtn( new_node, child_x, child_z, level + 1 );			
	
			}

		}

	}	
}

void poll_terrain()
{
	poll_qtn( &root, 0, 0, 0 );
}
