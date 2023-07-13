#ifndef GENERATION_HEADERGUARD
#define GENERATION_HEADERGUARD

float perlin_noise2D(float x, float y, int seed, int octaves);
float ridged_noise2D(float x, float y, int seed);
float ridged_multifractal_noise2D(float x, float y, int seed, int octaves);
float terrain_heightmap_func(float x, float y);

#endif
