#include "renderer.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string.h>

#include "utils.h"

//TODO

DynamicArray* fbos;

void initialize_renderer()
{
	fbos = createDynamicArray( sizeof( struct FrameBufferObject ) );
}

void terminate_renderer()
{
	deleteDynamicArray( fbos );
}

struct FrameBufferObject* gen_fbo( const char* identifier )
{
	struct FrameBufferObject fbo;
	capped_strcpy( &fbo.identifier[0], identifier, 20 );

	glGenFramebuffers( 1, &(fbo.id) );

	return (struct FrameBufferObject*) pushDataInDynamicArray( fbos, (void*) &fbo );
}

void dst_fbo( struct FrameBufferObject* fbo )
{
	glDeleteFramebuffers( 1, &(fbo->id) );
}
