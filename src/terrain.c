#include "terrain.h"

#include <cglm/cglm.h>

#include "objects.h"

extern vec3 g_cameraPosition;

QuadTreeNode root;
float root_size = 5000;
float min_distance = 354;

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

	float tested_distance = min_distance;
	int level = 0;
	
	while ( distance < tested_distance ){
		tested_distance /= 2.0;
		++level;
	}

	return level;	
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

		}else if ( node->state == QTNS_LOADED_SINGULAR || node->state == QTNS_PENDING_CURRENTLY_LOADED_SINGULAR ){

			deletePerspectiveObject( (PerspectiveObject*) node->children );

		}

		node->state = QTNS_NULL;
		node->children = NULL;
	}
}

void request_terrain_generation( QuadTreeNode* node, int x, int z, size_t level )
{
	if ( !( node->state == QTNS_LOADED_SINGULAR || node->state == QTNS_PENDING_CURRENTLY_LOADED_SINGULAR ) ){
	
		node->state = QTNS_PENDING_CURRENTLY_NULL;
		//request_generation(  )
		//TODO
	}
}

void push_generation_result( int x, int z, size_t level, PerspectiveObject* obj )
{

}

void poll_qtn( QuadTreeNode *node, int x, int z, size_t level )
{
	float quad_level_size = root_size / pow( 2, level );
	float quad_middle_pos_x = x*quad_level_size + quad_level_size / 2.0, quad_middle_pos_z = z*quad_level_size + quad_level_size / 2.0;
	size_t target_level = get_position_level( quad_middle_pos_x, 0, quad_middle_pos_z );
	
	if ( target_level == level ){

		if ( !( node->state == QTNS_LOADED_SINGULAR || node->state == QTNS_PENDING_CURRENTLY_LOADED_SINGULAR ) ){

			delete_node_children( node );
			request_terrain_generation( node, x, z, level );	

		}

	}else if ( target_level > level ){

		if ( !( node->state == QTNS_SUBDIVIDED ) ){

			delete_node_children( node );
			node->state = QTNS_SUBDIVIDED;
			node->children = malloc( sizeof( QuadTreeNode* ) * 4 );	

			for ( size_t i = 0; i < 4; ++i ){

				int child_x = i % 2;
				int child_z = ( i - child_x ) / 2;

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
