#ifndef _QUADTREE_H_
#define _QUADTREE_H_

#include "utils.h"
#include "boolvals.h"

struct PerspectiveObject;
typedef struct PerspectiveObject PerspectiveObject;

// quadtree mutators
void push_quadtree_chunk( int x_coord, int z_coord, size_t level, PerspectiveObject *obj );

// terrain control

void initialize_quadtree();
void terminate_quadtree();
void poll_quadtree();

#endif
