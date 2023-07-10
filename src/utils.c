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
}

void removeDataFromDynamicArray(DynamicArray* arr, size_t index, boolval freeUp)
{
	--(arr->usage);
	if (freeUp) free(*((void**)(arr->data + index)));
	memcpy((char*)arr->data + index*arr->dataSizeInBytes, (char*)arr->data + (index + 1)*arr->dataSizeInBytes, ((arr->usage+1)-(index+1))*(arr->dataSizeInBytes));

	if (arr->size > 10 && arr->usage < arr->size/2)
	{
		arr->size /= 2;
		arr->data = realloc(arr->data, arr->size*arr->dataSizeInBytes);
	}
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

GLuint createAndFillVBO(void* data, size_t dataSizeBytes, GLenum bufferTarget, GLenum bufferUsage)
{
	GLuint bufferIdentifier;
	glGenBuffers(1, &bufferIdentifier);
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
