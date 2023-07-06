#ifndef UTILS_HEADERGUARD
#define UTILS_HEADERGUARD

#include <stddef.h>

#include "boolvals.h"

typedef unsigned int GLuint;
typedef unsigned int GLenum;

typedef struct Vec2fl {
	float x,y;
} Vec2fl;

typedef struct Vec3fl {
	float x,y,z;
} Vec3fl;

typedef struct DynamicArray {
	size_t size, usage, dataSizeInBytes;
	void* data;
} DynamicArray;

DynamicArray* createDynamicArray(size_t dataSizeInBytes);
void deleteDynamicArray(DynamicArray* arr);
void* pushDataInDynamicArray(DynamicArray* arr, void* data);
void removeDataFromDynamicArray(DynamicArray* arr, size_t index, boolval freeUp);

char* getFile(const char* path, int* length_out);
GLuint initialize_program(const char* vertexShaderPath, const char* fragmentShaderPath);

GLuint createAndFillVBO(void* data, size_t dataBytes, GLenum bufferTarget, GLenum bufferUsage);
GLuint loadTexture(const char* path);

Vec3fl vec3fl_normalize(Vec3fl in);
Vec3fl vec3fl_cross(Vec3fl u, Vec3fl v);
Vec3fl vec3fl_difference(Vec3fl a, Vec3fl b);
Vec3fl vec3fl_add(Vec3fl a, Vec3fl b);
Vec3fl vec3fl_average(Vec3fl* array, size_t count);
Vec3fl vec3fl_divide(Vec3fl dividend, float divider);

#endif
