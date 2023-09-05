#ifndef _TERRAIN_H_
#define _TERRAIN_H_

#include "utils.h"
#include "boolvals.h"

struct PerspectiveObject;
typedef struct PerspectiveObject PerspectiveObject;

// terrain control

void push_generation_result( float x_pos, float z_pos, float size, PerspectiveObject* obj );

void initialize_terrain();
void terminate_terrain();
void poll_terrain();

#endif

