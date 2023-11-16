#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

DynamicArray* createDynamicArray(size_t dataSizeInBytes)
{
	DynamicArray *arr = malloc( sizeof(DynamicArray) );
	arr->data = malloc(dataSizeInBytes*10);
	arr->usage = 0;
	arr->size = 10;
	arr->dataSizeInBytes = dataSizeInBytes;

	return arr;
}

void clearDynamicArray(DynamicArray* arr)
{
	arr->usage = 0;
	arr->size = 10;
	arr->data = realloc( arr->data, arr->dataSizeInBytes * arr->size );
}

void deleteDynamicArray(DynamicArray* arr)
{
	if (!arr) return;
	if (arr->data) free(arr->data);
	free(arr);
}

void* pushDataInDynamicArray(DynamicArray* arr, void* data)
{
	const size_t targetIndex = arr->usage;
	
	arr->usage++;

	if (arr->usage >= arr->size){
		arr->size *= 2;
		arr->data = realloc(arr->data, arr->size*arr->dataSizeInBytes);
	}

	char* destination = (char*)arr->data + targetIndex * arr->dataSizeInBytes;
	memcpy(destination, data, arr->dataSizeInBytes);
	return ( void* ) destination;
}

int removeDataAtIndexFromDynamicArray(DynamicArray* arr, size_t index, boolval freeUp)
{
	if ( index > arr->usage - 1 ) return 1;

	--(arr->usage);
	if (freeUp) free(*((void**)( (char*)arr->data + index*arr->dataSizeInBytes)));

	void *moved_elems_source = (void*) ( (char*)arr->data + (index + 1)*arr->dataSizeInBytes );
	void *moved_elems_dest = (void*) ( (char*)arr->data + index*arr->dataSizeInBytes );
	size_t moved_elems_count = (arr->usage+1)-(index+1);
	void *temp_move_buff = malloc( moved_elems_count * arr->dataSizeInBytes );

	memcpy( temp_move_buff, moved_elems_source, moved_elems_count * arr->dataSizeInBytes );
	memcpy( moved_elems_dest, temp_move_buff, moved_elems_count * arr->dataSizeInBytes );
	free( temp_move_buff );

	if (arr->size > 10 && arr->usage < arr->size/2)
	{
		arr->size /= 2;
		arr->data = realloc(arr->data, arr->size*arr->dataSizeInBytes);
	}

	return 0;
}

int removeDataFromDynamicArray(DynamicArray* arr, void* data, boolval freeUp)
{
	size_t target_index = 0;
	boolval found = false;

	for ( ; target_index < arr->usage; ++target_index ){

		void **arr_pointer = ( void** ) ( ( char* ) arr->data + target_index*arr->dataSizeInBytes );

		if ( data == *arr_pointer ) 
		{
			found = true;
			break;
		}

	}

	if ( !found ) return 1;
	return removeDataAtIndexFromDynamicArray(arr, target_index, freeUp);
}

char* getFile(const char* path, int* length_out)
{
	FILE* file = fopen(path, "rb");
	if (!file) return NULL;

	fseek(file, 0, SEEK_END);
	int length = ftell(file) + 1;

	char* buffer = malloc(length);

	fseek(file, 0, SEEK_SET);

	fread(buffer, 1, length - 1, file);
	buffer[length-1] = '\0';
	fclose(file);

	*length_out = length;

	return buffer;
}

GLuint initialize_program(const char* vertexShaderPath, const char* fragmentShaderPath)
{

	GLuint vertexShader, fragmentShader;

	vertexShader = glCreateShader(GL_VERTEX_SHADER);

	int vertexShaderSourceLength = 0;
	char* vertexShaderSource = getFile(vertexShaderPath, &vertexShaderSourceLength);

	GLchar const* vertexShaderSources[] = { vertexShaderSource };
	GLint lengths[] = { vertexShaderSourceLength-1 };

	glShaderSource(vertexShader, 1, &vertexShaderSources[0], &lengths[0]);
	glCompileShader(vertexShader);



	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

	int fragShaderSourceLength = 0;
	char* fragShaderSource = getFile(fragmentShaderPath, &fragShaderSourceLength);

	GLchar const* fragShaderSources[] = { fragShaderSource };
	GLint lengths2[] = { fragShaderSourceLength-1 };

	glShaderSource(fragmentShader, 1, &fragShaderSources[0], &lengths2[0]);
	glCompileShader(fragmentShader);


	GLuint program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	

	free(vertexShaderSource);
	free(fragShaderSource);

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return program;
}

GLuint createVBO()
{
	GLuint bufferIdentifier;
	glGenBuffers(1, &bufferIdentifier);
	return bufferIdentifier;
}

void deleteVBO( GLuint id )
{
	glDeleteBuffers( 1, &id );
}

GLuint createAndFillVBO(void* data, size_t dataSizeBytes, GLenum bufferTarget, GLenum bufferUsage)
{
	GLuint bufferIdentifier = createVBO();
	glBindBuffer(bufferTarget, bufferIdentifier);
	glBufferData(bufferTarget, dataSizeBytes, data, bufferUsage);
	return bufferIdentifier;
}

GLuint loadTexture(const char* path)
{
	int width, height, channels;
	unsigned char* image = stbi_load(path, &width, &height, &channels, STBI_rgb_alpha);

	GLuint textureID;
	glGenTextures(1, &textureID);
	
	glBindTexture(GL_TEXTURE_2D, textureID);


	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

	glGenerateMipmap(GL_TEXTURE_2D);

	return textureID;
}

Vec3fl vec3fl_normalize(Vec3fl in)
{
	float magnitude = sqrt(in.x*in.x + in.y*in.y + in.z*in.z);
	if (magnitude == 0) return in;
	in.x /= magnitude; in.y /= magnitude; in.z /= magnitude;
	return in;
}

int min( int a, int b )
{
	return ( a < b ) ? a : b;
}

int max( int a, int b )
{
	return ( a > b ) ? a : b;
}

Vec3fl vec3fl_cross(Vec3fl u, Vec3fl v)
{
	Vec3fl out = {
		u.y*v.z - u.z*v.y,
		u.z*v.x - u.x*v.z,
		u.x*v.y - u.y*v.x
	};
	return out;
}

Vec3fl vec3fl_difference(Vec3fl a, Vec3fl b)
{
	Vec3fl out = {
		a.x - b.x,
		a.y - b.y,
		a.z - b.z
	};
	return out;
}

Vec3fl vec3fl_add(Vec3fl a, Vec3fl b)
{
	Vec3fl out = {
		a.x + b.x,
		a.y + b.y,
		a.z + b.z
	};
	return out;
}

Vec3fl vec3fl_average(Vec3fl* array, size_t count)
{
	Vec3fl out = {0, 0, 0};
	size_t found = 0;
	for (size_t i = 0; i < count; ++i){
		Vec3fl* vec_i = array + i;
		if (vec_i == NULL) continue;
		out.x += vec_i->x;
		out.y += vec_i->y;
		out.z += vec_i->z;
		++found;
	}
	out.x /= (float)found;
	out.y /= (float)found;
	out.z /= (float)found;
	return out;
}

Vec3fl vec3fl_divide(Vec3fl dividend, float divider)
{
	Vec3fl out = {
		dividend.x / divider,
		dividend.y / divider,
		dividend.z / divider
	};
	return out;
}

float vec3fl_magnitude(Vec3fl in)
{
	return sqrt( in.x * in.x + in.y * in.y + in.z * in.z );
}

char* capped_strcpy( char* destination, const char* source, size_t max_len )
{

	size_t src_len = strlen( source ) + 1;
	size_t len = min( src_len, max_len );
	
	char* src_cpy = (char*) malloc( src_len );
	strcpy( src_cpy, source );

	src_cpy[ len - 1 ] = '\0';

	strcpy( destination, src_cpy );
	free( src_cpy );	

}
