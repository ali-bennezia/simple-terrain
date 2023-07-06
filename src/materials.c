#include "materials.h"

#include <string.h>
#include <stdlib.h>

#include "utils.h"

Material createMaterial(GLuint program, boolval drawWithMatrices, boolval drawWithLightData)
{
	Material mat;

	mat.drawProgram = program;

	mat.floatData = createDynamicArray(sizeof(float));
	mat.vec3fData = createDynamicArray(sizeof(Vec3fl));
	mat.textureData = createDynamicArray(sizeof(MaterialTexture));

	mat.floatDataUniformNames = createDynamicArray(sizeof(char*));
	mat.vec3fDataUniformNames = createDynamicArray(sizeof(char*));
	mat.textureDataUniformNames = createDynamicArray(sizeof(char*));

	mat.fetchMatrices = drawWithMatrices;
	mat.fetchLightData = drawWithLightData;

	return mat;
}

void deleteMaterial(Material* mat)
{
	deleteDynamicArray(mat->floatData);
	deleteDynamicArray(mat->vec3fData);
	deleteDynamicArray(mat->textureData);

	deleteDynamicArray(mat->floatDataUniformNames);
	deleteDynamicArray(mat->vec3fDataUniformNames);
	deleteDynamicArray(mat->textureDataUniformNames);

	free(mat);
}

void copyFloatAsUniform(Material* mat, char* uniformName, float data)
{
	char* uniformNameDuplicate = strdup(uniformName);
	pushDataInDynamicArray(mat->floatDataUniformNames, &uniformNameDuplicate);
	pushDataInDynamicArray(mat->floatData, &data);
}

void copyVec3fAsUniform(Material* mat, char* uniformName, Vec3fl data)
{
	char* uniformNameDuplicate = strdup(uniformName);
	pushDataInDynamicArray(mat->vec3fDataUniformNames, &uniformNameDuplicate);
	pushDataInDynamicArray(mat->vec3fData, &data);
}

void copyTextureAsUniform(Material* mat, char* uniformName, GLuint textureHandle, int textureIndex)
{
	MaterialTexture tex = {
		textureHandle,
		textureIndex
	};

	char* uniformNameDuplicate = strdup(uniformName);
	pushDataInDynamicArray(mat->textureDataUniformNames, &uniformNameDuplicate);
	pushDataInDynamicArray(mat->textureData, &tex);
}

void copyDataAsUniform(Material* mat, char* uniformName, void* data, enum MaterialUniformType type)
{
	char* uniformNameDuplicate = strdup(uniformName);

	switch(type){
		case MUT_FLOAT:
			copyFloatAsUniform(mat, uniformName, *(float*)data);
		break;
		case MUT_VEC3F:
			copyVec3fAsUniform(mat, uniformName, *(Vec3fl*)data);
		break;
		default:;
			MaterialTexture* data = (MaterialTexture*)data;
			copyTextureAsUniform(mat, uniformName, data->textureHandle, data->textureIndex);
		break;
	}
}

//TODO: free uniform name duplicates from mem
/*void removeUniformData(Material mat, char* uniformName, enum MaterialUniformType type)
{
	switch(type){
		case MUT_FLOAT:

			pushDataInDynamicArray(&(mat.floatDataUniformNames), uniformNameDuplicate);
			pushDataInDynamicArray(&(mat.floatData), data);
		break;
		case MUT_VEC3F:
			pushDataInDynamicArray(&(mat.vec3fDataUniformNames), uniformNameDuplicate);
			pushDataInDynamicArray(&(mat.vec3fData), data);
		break;
		default:
			pushDataInDynamicArray(&(mat.textureIDsDataUniformNames), uniformNameDuplicate);
			pushDataInDynamicArray(&(mat.textureIDsData), data);
		break;
	}
}*/
