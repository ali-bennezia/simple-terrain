#include "generation.h"

#include <math.h>

#include "./../libs/perlin/perlin.h"

float perlin_noise2D(float x, float y, int seed, int octaves)
{
	float result = 0;//pnoise2d(x, y, 1, 1, seed);

	float amplitude = 1;
	float scaleDown = 1;
	for (size_t i = 0; i < octaves; ++i){
		result += amplitude*pnoise2d(x*scaleDown, y*scaleDown, 1, 1, seed+i);

		amplitude /= 2;
		scaleDown *= 2;	
	}
	return result;
}

float ridged_noise2D(float x, float y, int seed)
{
	float val = pnoise2d(x, y, 1, 4, seed);
	float res = 1.0 - fabs( val - 0.5 )*2;
	return res;
}

float ridged_multifractal_noise2D(float x, float y, int seed, int octaves)
{
	float result = 0;

	float amplitude = 1, coordsScaleFactor = 1;

	for (size_t i = 0; i < octaves; ++i){
		result += amplitude*ridged_noise2D(x * coordsScaleFactor, y * coordsScaleFactor, seed);
		amplitude /= 2.0;
		coordsScaleFactor *= 2.0;
	}
	
	return result;
}

float terrain_heightmap_func(float x, float y)
{
	const float scale = 16;
	const double plane_mapping_factor = 0.0625/scale;
	const float amplitude = 4*scale;

	float result = 0;// amplitude*pnoise2d(x*plane_mapping_factor, y*plane_mapping_factor, 1, 1, 45645656);
	/*result += (amplitude/2.0)*pnoise2d(x*plane_mapping_factor*2, y*plane_mapping_factor*2, 1, 1, 45645656);
	result += (amplitude/4.0)*pnoise2d(x*plane_mapping_factor*4, y*plane_mapping_factor*4, 1, 1, 45645656);
	result += (amplitude/8.0)*pnoise2d(x*plane_mapping_factor*8, y*plane_mapping_factor*8, 1, 1, 45645656);
	result += (amplitude/16.0)*pnoise2d(x*plane_mapping_factor*16, y*plane_mapping_factor*16, 1, 1, 45645656);
	result += (amplitude/32.0)*pnoise2d(x*plane_mapping_factor*32, y*plane_mapping_factor*32, 1, 1, 45645656);*/

	result += amplitude*pnoise2d(x*plane_mapping_factor, y*plane_mapping_factor, 1, 1, 45645656);
	//result += 16*ridged_noise2D(x*(1.0/128.0), y*(1.0/128.0), 415646549);
	result += 16*ridged_multifractal_noise2D(x*(1.0/128.0), y*(1.0/128.0), 415646549, 6);
	result -= 300;
	//result += amplitude*ridged_noise2D(x*plane_mapping_factor, y*plane_mapping_factor, 446549);
	return result;
	//return 3*sin((1.0/2.0)*x);
}
