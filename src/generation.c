#include "generation.h"

#include <math.h>

#include "libs/perlin.h"

float terrain_heightmap_func(float x, float y)
{
	const double plane_mapping_factor = 0.0625;
	const float amplitude = 4;
	return amplitude*pnoise2d(x*plane_mapping_factor, y*plane_mapping_factor, 0.5, 6, 45645656);
	//return 3*sin((1.0/2.0)*x);
}
