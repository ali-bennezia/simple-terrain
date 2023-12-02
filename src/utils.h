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
void clearDynamicArray(DynamicArray* arr);
void deleteDynamicArray(DynamicArray* arr);
void* pushDataInDynamicArray(DynamicArray* arr, void* data);
int removeDataAtIndexFromDynamicArray(DynamicArray* arr, size_t index, boolval freeUp);
int removeDataFromDynamicArray(DynamicArray* arr, void* data, boolval freeUp);

char* getFile(const char* path, int* length_out);
GLuint initialize_program(const char* vertexShaderPath, const char* fragmentShaderPath);

GLuint createVBO();
void deleteVBO( GLuint id );
GLuint createAndFillVBO(void* data, size_t dataBytes, GLenum bufferTarget, GLenum bufferUsage);
GLuint loadTexture(const char* path);

int min( int a, int b );
int max( int a, int b );

Vec3fl vec3fl_normalize(Vec3fl in);
Vec3fl vec3fl_cross(Vec3fl u, Vec3fl v);
Vec3fl vec3fl_difference(Vec3fl a, Vec3fl b);
Vec3fl vec3fl_add(Vec3fl a, Vec3fl b);
Vec3fl vec3fl_average(Vec3fl* array, size_t count);
Vec3fl vec3fl_divide(Vec3fl dividend, float divider);
float vec3fl_magnitude(Vec3fl in);

char* capped_strcpy( char* destination, const char* source, size_t max_len );
void thread_sleep( unsigned int ms );

#endif
