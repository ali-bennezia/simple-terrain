#include "objects.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

extern GLuint g_defaultProgram;
extern GLuint g_texturedProgram;

extern mat4 g_viewMatrix;
extern mat4 g_projectionMatrix;

extern vec3 g_ambientLightColor;
extern float g_ambientLightIntensity;

extern vec3 g_directionalLightDirection;
extern vec3 g_directionalLightColor;
extern float g_directionalLightIntensity;

void getPerspectiveObjectModelMatrix(PerspectiveObject obj, mat4 out)
{
	vec3 xAxis = {1, 0, 0}, yAxis = {0, 1, 0}, zAxis = {0, 0, 1};

	glm_translate_make(out, obj.position);
	glm_rotate(out, glm_rad(obj.eulerAnglesRotation[0]), xAxis);
	glm_rotate(out, glm_rad(obj.eulerAnglesRotation[1]), yAxis);
	glm_rotate(out, glm_rad(obj.eulerAnglesRotation[2]), zAxis);
}

void drawPerspectiveObject(PerspectiveObject obj)
{
	GLuint drawProgram = obj.material.drawProgram;

	if (obj.meshInitialized){
		glBindBuffer(GL_ARRAY_BUFFER, obj.meshVBO);

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}

	if (obj.normalsInitialized){
		glBindBuffer(GL_ARRAY_BUFFER, obj.normalsVBO);

		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	}

	if (obj.UVsInitialized){
		glBindBuffer(GL_ARRAY_BUFFER, obj.UVsVBO);

		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
	}

	glUseProgram(drawProgram);

	//Matrices fetch
	mat4 modelMatrix;
	vec3 position = {1, 0, 10}, eulerAnglesRotation = {0, 90, 0};
	getPerspectiveObjectModelMatrix(obj, modelMatrix);

	mat4 preNormalMatrix, normalMatrix;
	glm_mat4_copy(modelMatrix, preNormalMatrix);
	glm_mat4_transpose(preNormalMatrix);
	glm_mat4_inv(preNormalMatrix, normalMatrix);

	glUniformMatrix4fv(glGetUniformLocation(drawProgram, "normalMatrix"), 1, GL_FALSE, normalMatrix[0]);
	glUniformMatrix4fv(glGetUniformLocation(drawProgram, "projectionMatrix"), 1, GL_FALSE, g_projectionMatrix[0]);
	glUniformMatrix4fv(glGetUniformLocation(drawProgram, "viewMatrix"), 1, GL_FALSE, g_viewMatrix[0]);
	glUniformMatrix4fv(glGetUniformLocation(drawProgram, "modelMatrix"), 1, GL_FALSE, modelMatrix[0]);

	//Light data fetch
	glUniform3fv(glGetUniformLocation(drawProgram, "ambientLightColor"), 1, &g_ambientLightColor[0]);
	glUniform1f(glGetUniformLocation(drawProgram, "ambientLightIntensity"), g_ambientLightIntensity);

	glUniform3fv(glGetUniformLocation(drawProgram, "directionalLightDirection"), 1, &g_directionalLightDirection[0]);
	glUniform3fv(glGetUniformLocation(drawProgram, "directionalLightColor"), 1, &g_directionalLightColor[0]);
	glUniform1f(glGetUniformLocation(drawProgram, "directionalLightIntensity"), g_directionalLightIntensity);

	//Material-specific uniforms
	for (size_t i = 0; i < obj.material.floatDataUniformNames.usage; ++i)
	{
		char* uniformName = *((char**)obj.material.floatDataUniformNames.data + i);
		float* data = (float*)obj.material.floatData.data + i;
		glUniform1f(glGetUniformLocation(drawProgram, uniformName), *data);
	}

	for (size_t i = 0; i < obj.material.vec3fDataUniformNames.usage; ++i)
	{
		char* uniformName = *((char**)obj.material.vec3fDataUniformNames.data + i);
		Vec3fl* data = (Vec3fl*)obj.material.vec3fData.data + i;
		vec3 v3arrdata;
		v3arrdata[0] = data->x; v3arrdata[1] = data->y; v3arrdata[2] = data->z;
		glUniform3fv(glGetUniformLocation(drawProgram, uniformName), 1, &v3arrdata[0]);
	}

	for (size_t i = 0; i < obj.material.textureDataUniformNames.usage; ++i)
	{
		char* uniformName = *((char**)obj.material.textureDataUniformNames.data + i);
		MaterialTexture* data = (MaterialTexture*)obj.material.textureData.data + i;

		glActiveTexture(GL_TEXTURE0 + data->textureIndex);
		glBindTexture(GL_TEXTURE_2D, data->textureHandle);
		glUniform1i(glGetUniformLocation(drawProgram, uniformName), data->textureIndex);
	}

	glDrawArrays(GL_TRIANGLES, 0, obj.vertices);
}

void setObjectVBO(PerspectiveObject* objPtr, GLuint vboHandle, enum BufferType type)
{
	switch (type){
		case VERTICES:
			(*objPtr).meshVBO = vboHandle;
			(*objPtr).meshInitialized = true;
		break;
		case NORMALS:
			(*objPtr).normalsVBO = vboHandle;
			(*objPtr).normalsInitialized = true;
		break;
		default:
			(*objPtr).UVsVBO = vboHandle;
			(*objPtr).UVsInitialized = true;
		break;
	}
}


