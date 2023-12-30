#include "standard.h"


#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cglm/cglm.h>


#include "config.h"
#include "utils.h"
#include "materials.h"

/// externs

extern Material g_defaultTerrainMaterialLit;
extern vec3 g_cameraPosition;

/// impl

void initialize_standard()
{

}

void terminate_standard()
{

}

void poll_standard()
{

	for ( int x = -5; x <= 5; ++x )
	{
		for ( int y = -5; y <= 5; ++y )
		{
			int pos[2] = {
				g_cameraPosition[ 0 ] + x * STANDARD_CHUNK_SIZE,
				g_cameraPosition[ 1 ] + y * STANDARD_CHUNK_SIZE
			};		
		}
	}

}
