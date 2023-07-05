#ifndef FACTORY_HEADERGUARD
#define FACTORY_HEADERGUARD

#include <stddef.h>

#include "utils.h"

Vec3fl neighborNormalsAverage(Vec3fl *a, Vec3fl *b, Vec3fl *c, Vec3fl *d);

int generateTessellatedQuad(
	float** meshDestination, 
	float** normalsDestination, 
	unsigned int tessellations, 
	float size, 
	float(*heightMapFunction)(float, float),
	size_t *verticesCount, 
	size_t *normalsCount
);

#endif
