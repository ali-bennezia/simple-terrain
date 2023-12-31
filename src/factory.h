#ifndef FACTORY_HEADERGUARD
#define FACTORY_HEADERGUARD

#include <stddef.h>

#include "utils.h"

Vec3fl neighborNormalsAverage(Vec3fl *a, Vec3fl *b, Vec3fl *c, Vec3fl *d);

Vec3fl getQuadNormal(float xCoordsOffset, float zCoordsOffset, int xQuadCoord, int zQuadCoord, float quadSideSize, float(*heightMapFunction)(float, float));

int generateTessellatedQuad(
	float xCoordsOffset,
	float zCoordsOffset,
	float** meshDestination, 
	float** normalsDestination, 
	unsigned int tessellations, 
	float size, 
	float(*heightMapFunction)(float, float),
	size_t *verticesCount, 
	size_t *normalsCount
);

#endif
