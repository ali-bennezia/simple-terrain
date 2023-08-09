#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include "utils.h"
#include "boolvals.h"

struct PerspectiveObject;
typedef struct PerspectiveObject PerspectiveObject;

enum QuadTreeNodeState
{
	QTNS_LOADED,
	QTNS_SUBDIVIDED,
	QTNS_LOADED_AND_SUBDIVIDED,
	QTNS_NULL,
	QTNS_PENDING_WHILE_SUBDIVIDED,
	QTNS_PENDING_WHILE_NULL
};

typedef struct QuadTreeNode
{
	enum QuadTreeNodeState state;
	void *children;
} QuadTreeNode;

void initialize_terrain();
void terminate_terrain();

void convert_coords_level( int from_x, int from_z, size_t from_level, size_t to_level, int *result_x, int *result_z );
size_t get_position_level( float x, float y, float z );
size_t get_quad_level( int x_coord, int z_coord, size_t level );

boolval is_node_pending( QuadTreeNode *node );
boolval is_node_subdivided( QuadTreeNode *node );
boolval is_node_loaded( QuadTreeNode *node );

QuadTreeNode *generate_node( );
void clear_node_children( QuadTreeNode *node );
void subdivide_node( QuadTreeNode *node );
QuadTreeNode *search_node( int x, int z, size_t level );
void request_node_generation( QuadTreeNode *node, int x, int z, size_t level );
void push_generation_result( int x, int z, size_t level, PerspectiveObject* obj );

void poll_node( QuadTreeNode* node, int x, int z, size_t level );
void poll_terrain();

#endif

