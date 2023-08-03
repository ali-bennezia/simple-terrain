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
	QTNS_PENDING_CURRENTLY_LOADED_SINGULAR,
	QTNS_PENDING_CURRENTLY_NULL
};

typedef struct QuadTreeNode
{
	enum QuadTreeNodeState state;
	void *children;
} QuadTreeNode;

void initialize_terrain();
void terminate_terrain();

size_t get_position_level( float x, float y, float z );

void delete_node_children( QuadTreeNode *node );
void request_terrain_generation( QuadTreeNode *node, int x, int z, size_t level );
void push_generation_result( int x, int z, size_t level, PerspectiveObject* obj );

void poll_qtn( QuadTreeNode* node, int x, int z, size_t level );
void poll_terrain();

#endif

