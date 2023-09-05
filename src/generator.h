#ifndef _GENERATOR_H_
#define _GENERATOR_H_

#include <stddef.h>

#include "boolvals.h"

void initialize_generator();
void poll_generator();
void terminate_generator();

int find_request_index( boolval pending_predicate, boolval done_predicate, boolval fetched_predicate );
void *thread_job( void* data );
void create_threads();
void request_generation( float x_pos, float z_pos, float size, size_t tessellations );

#endif
