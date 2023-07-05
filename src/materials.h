#ifndef MATERIALS_HEADERGUARD
#define MATERIALS_HEADERGUARD

#include "boolvals.h"
#include "utils.h"

typedef unsigned int GLuint;

enum MaterialUniformType {
	MUT_FLOAT,
	MUT_VEC3F,
	MUT_TEX
};

typedef struct MaterialTexture {
	GLuint textureHandle;
	int textureIndex;
} MaterialTexture;

typedef struct Material {
	GLuint drawProgram;

	//DynArrays of floats & GLuints respectively.
	DynamicArray floatData, floatDataUniformNames;
	DynamicArray vec3fData, vec3fDataUniformNames;
	DynamicArray textureData, textureDataUniformNames;

	boolval fetchMatrices;
	boolval fetchLightData;
} Material;

Material createMaterial(GLuint program, boolval drawWithMatrices, boolval drawWithLightData);
void deleteMaterial(Material* mat);
void copyFloatAsUniform(Material* mat, char* uniformName, float data);
void copyVec3fAsUniform(Material* mat, char* uniformName, Vec3fl data);
void copyTextureAsUniform(Material* mat, char* uniformName, GLuint textureHandle, int textureIndex);
void copyDataAsUniform(Material* mat, char* uniformName, void* data, enum MaterialUniformType type);
//void removeUniformData(Material* mat, char* uniformName, enum MaterialUniformType type);

#endif
