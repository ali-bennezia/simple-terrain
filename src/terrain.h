#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include "utils.h"

struct PerspectiveObject;
typedef struct PerspectiveObject PerspectiveObject;

enum QuadTreeNodeState
{
	QTNS_LOADED_SINGULAR,
	QTNS_SUBDIVIDED,
	QTNS_NULL,
	QTNS_PENDING
};

typedef struct QuadTreeNode
{
	enum QuadTreeNodeState state;
	void *children;
} QuadTreeNode;

void initialize_terrain();
void terminate_terrain();

size_t get_position_level( float x, float y, float z );
void subdivide_node( QuadTreeNode *node );
void delete_node_children( QuadTreeNode *node );
void request_terrain_generation( QuadTreeNode *node, int x, int z, size_t level );
void convert_coords_level( int from_x, int from_z, size_t from_level, size_t to_level, int *result_x, int *result_z );
void push_generation_result( int x, int z, size_t level, PerspectiveObject* obj );

void poll_qtn( QuadTreeNode* node, int x, int z, size_t level );
void poll_terrain();

#endif

