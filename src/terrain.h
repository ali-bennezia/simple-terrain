#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include "utils.h"
#include "boolvals.h"

#define TESSELLATIONS 2

struct PerspectiveObject;
typedef struct PerspectiveObject PerspectiveObject;

// quadtree mutators
void push_node_terrain( int x_coord, int z_coord, size_t level, PerspectiveObject *obj );

// terrain control

void initialize_terrain();
void terminate_terrain();
void poll_terrain();

#endif

