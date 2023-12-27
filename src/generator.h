#ifndef _GENERATOR_H_
#define _GENERATOR_H_

#include <stddef.h>

#include "boolvals.h"

void initialize_generator();
void poll_generator();
void terminate_generator();

int find_request_index( boolval pending_predicate, boolval done_predicate, boolval fetched_predicate );
void *thread_job( void* data );
boolval request_generation( int x_coord, int z_coord, size_t level, size_t tessellations );

#endif
