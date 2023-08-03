#ifndef _RENDERER_H_
#define _RENDERER_H_

typedef unsigned int GLuint;

struct FrameBufferObject
{
	GLuint id;
	char identifier[20];
};

void initialize_renderer();
void terminate_renderer();

struct FrameBufferObject* gen_fbo( const char* identifier );
void dst_fbo( struct FrameBufferObject* fbo );

#endif
