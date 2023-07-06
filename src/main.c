#include <stdlib.h>
#include <stdio.h>

#include <math.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cglm/cglm.h>

#include "boolvals.h"
#include "utils.h"
#include "materials.h"
#include "objects.h"
#include "factory.h"
#include "generation.h"

//Window params
GLFWwindow* g_window = NULL;
float g_aspectRatio = 1;

//Matrices
mat4 g_projectionMatrix, g_viewMatrix;

//Camera params
vec3 g_cameraPosition;
float g_cameraYaw, g_cameraPitch, g_cameraFOV, g_cameraRotateSpeed, g_cameraMoveSpeed;

//Game state
double g_deltaTime;
double g_mouseX, g_mouseY;
boolval g_forwardInput = false, g_backwardInput = false, g_leftInput = false, g_rightInput = false;
vec3 g_cameraVelocity;
boolval g_exit = false, g_lockMouse = true, g_movementEnabled = true;

//Light params
vec3 g_directionalLightDirection, g_directionalLightColor, g_ambientLightColor;
float g_directionalLightIntensity, g_ambientLightIntensity;

//Textures, shaders, materials
GLuint g_crateTexture, g_grassTexture;
GLuint g_defaultProgram, g_texturedProgram, g_texturedTerrainProgram;
Material g_untexturedMaterialLit, g_crateMaterialLit, g_grassTerrainMaterialLit;

void setAmbientLightColor(float r, float g, float b)
{
	g_ambientLightColor[0] = r;
	g_ambientLightColor[1] = g;
	g_ambientLightColor[2] = b;
}

void setAmbientLightIntensity(float intensity)
{
	g_ambientLightIntensity = intensity;
}

void setDirectionalLightDirection(float x, float y, float z)
{
	float magnitude = sqrt(x*x + y*y + z*z);
	if (magnitude == 0) return;
	float dx = x/magnitude;
	float dy = y/magnitude;
	float dz = z/magnitude;
	g_directionalLightDirection[0] = dx;
	g_directionalLightDirection[1] = dy;
	g_directionalLightDirection[2] = dz;
}

void setDirectionalLightColor(float r, float g, float b)
{
	g_directionalLightColor[0] = r;
	g_directionalLightColor[1] = g;
	g_directionalLightColor[2] = b;
}

void setDirectionalLightIntensity(float intensity)
{
	g_directionalLightIntensity = intensity;
}

void getCameraLookAtPosition(vec3 out)
{
	float xz_x = cos(g_cameraYaw), xz_z = sin(g_cameraYaw);
	float yz_z = cos(g_cameraPitch), yz_y = sin(g_cameraPitch);
	xz_x *= yz_z; xz_z *= yz_z;
	out[0] = xz_x;
	out[1] = yz_y;
	out[2] = xz_z;
}

void rotateDirectionToCameraLookAtDirection(vec3 direction, vec3 out)
{
	vec4 v4dir, result;
	v4dir[0] = direction[0];
	v4dir[1] = direction[1];
	v4dir[2] = direction[2];
	v4dir[3] = 0;
	
	mat4 viewMatrixInverse;
	glm_mat4_inv(g_viewMatrix, viewMatrixInverse);
	glm_mat4_mulv(viewMatrixInverse, v4dir, result);

	out[0] = result[0];
	out[1] = result[1];
	out[2] = result[2];
}

void updateViewMatrix()
{
	vec3 up = {0, 1, 0}, lookAt;
	getCameraLookAtPosition(lookAt);
	glm_vec3_add(g_cameraPosition, lookAt, lookAt);
	glm_lookat(g_cameraPosition, lookAt, up, g_viewMatrix);
}

void updateProjectionMatrix()
{
	int w, h;
	glfwGetFramebufferSize(g_window, &w, &h);
	g_aspectRatio = (float)w/(float)h;
	glm_perspective(45, g_aspectRatio, 0.1, 1000, g_projectionMatrix);
}

void set_camera_rotate_speed(float cameraRotateSpeed)
{
	g_cameraRotateSpeed = cameraRotateSpeed;
}

void set_camera_move_speed(float cameraMoveSpeed)
{
	g_cameraMoveSpeed = cameraMoveSpeed;
}

void set_camera_position(float x, float y, float z)
{
	g_cameraPosition[0] = x;
	g_cameraPosition[1] = y;
	g_cameraPosition[2] = z;
	updateViewMatrix();
}

void translate_camera(float x, float y, float z)
{
	set_camera_position(g_cameraPosition[0] + x, g_cameraPosition[1] + y, g_cameraPosition[2] + z);
}

void set_camera_yaw(float yaw)
{
	g_cameraYaw = yaw;
	updateViewMatrix();
}

void set_camera_pitch(float pitch)
{
	g_cameraPitch = pitch;
	updateViewMatrix();
}

void rotate_camera_yaw(float yaw)
{
	set_camera_yaw(g_cameraYaw + yaw);
}

void rotate_camera_pitch(float pitch)
{
	set_camera_pitch(g_cameraPitch + pitch);
}

void set_camera_FOV(float FOV)
{
	g_cameraFOV = FOV;
	updateProjectionMatrix();
}

void cleanup()
{
	if (g_window != NULL)
		glfwDestroyWindow(g_window);
	glfwTerminate();
}


void set_cursor_lock(boolval lock)
{
	g_lockMouse = lock;
	g_movementEnabled = lock;
	glfwSetInputMode(g_window, GLFW_CURSOR, lock == true ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
}

void toggle_cursor_lock()
{
	set_cursor_lock(!g_lockMouse);
}


void handle_key_input(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		if (key == GLFW_KEY_ESCAPE)
			g_exit = true;

		if (key == GLFW_KEY_SPACE)
			toggle_cursor_lock();

		if (key == GLFW_KEY_W)
			g_forwardInput = true;
		if (key == GLFW_KEY_S)
			g_backwardInput = true;
		if (key == GLFW_KEY_D)
			g_rightInput = true;
		if (key == GLFW_KEY_A)
			g_leftInput = true;
	}else if (action == GLFW_RELEASE)
	{
		if (key == GLFW_KEY_W)
			g_forwardInput = false;
		if (key == GLFW_KEY_S)
			g_backwardInput = false;
		if (key == GLFW_KEY_D)
			g_rightInput = false;
		if (key == GLFW_KEY_A)
			g_leftInput = false;
	}


}

void handle_cursor_pos_change(GLFWwindow* window, double mouseX, double mouseY)
{
	double deltaX = mouseX-g_mouseX, deltaY = mouseY-g_mouseY;

	if (g_movementEnabled)
	{
		rotate_camera_yaw(deltaX * g_cameraRotateSpeed * g_deltaTime);
		rotate_camera_pitch(-deltaY * g_cameraRotateSpeed * g_deltaTime);
	}

	g_mouseX = mouseX; g_mouseY = mouseY;
}

void handle_window_resize(GLFWwindow* window, int width, int height)
{
	int w, h;
	glfwGetFramebufferSize(g_window, &w, &h);
	glViewport(0, 0, w, h);
	updateProjectionMatrix();
}

void initialize_default_shaders()
{
	g_defaultProgram = initialize_program("assets/shaders/default.vert", "assets/shaders/default.frag");
	g_texturedProgram = initialize_program("assets/shaders/textured.vert", "assets/shaders/textured.frag");
	g_texturedTerrainProgram = initialize_program("assets/shaders/texturedTerrain.vert", "assets/shaders/texturedTerrain.frag");
}

void initialize_default_materials()
{
	g_crateTexture = loadTexture("assets/textures/crate1_diffuse.png");
	g_grassTexture = loadTexture("assets/textures/grass_diffuse.png");

	g_untexturedMaterialLit = createMaterial(g_defaultProgram, true, true);
	g_crateMaterialLit = createMaterial(g_texturedProgram, true, true);
	g_grassTerrainMaterialLit = createMaterial(g_texturedTerrainProgram, true, true);


	//untextured white material
	Vec3fl white = {1, 1, 1};

	copyVec3fAsUniform(&g_untexturedMaterialLit, "color", white);

	//crate material
	copyVec3fAsUniform(&g_crateMaterialLit, "color", white);
	copyTextureAsUniform(&g_crateMaterialLit, "albedoTexture", g_crateTexture, 0);

	//grass terrain material
	copyVec3fAsUniform(&g_grassTerrainMaterialLit, "color", white);
	copyTextureAsUniform(&g_grassTerrainMaterialLit, "albedoTexture", g_grassTexture, 0);
	copyFloatAsUniform(&g_grassTerrainMaterialLit, "textureTiles", 1);
}

boolval initialize()
{

	if (!glfwInit())
	{
		fprintf(stderr, "Couldn't initialize GLFW\n");
		return true;
	}

	atexit(cleanup);

	g_window = glfwCreateWindow(640, 480, "Renderer", NULL, NULL);
	updateProjectionMatrix();
	
	if (g_window == NULL)
	{
		fprintf(stderr, "Couldn't create window\n");
		return true;
	}

	glfwMakeContextCurrent(g_window);

	if (glewInit())
	{
		fprintf(stderr, "Couldn't initialize GLEW\n");
		return true;
	}

	if (glfwRawMouseMotionSupported())
		glfwSetInputMode(g_window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

	glfwSetWindowSizeCallback(g_window, handle_window_resize);
	glfwSetKeyCallback(g_window, handle_key_input);
	glfwSetCursorPosCallback(g_window, handle_cursor_pos_change);
	glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_TEXTURE_2D);
	glEnable(GL_CULL_FACE);

	glCullFace(GL_BACK);

	glClearColor(0, 0, 0, 1);


	return false;

}

void update()
{
	vec3 forward = {0, 0, -1}, backward = {0, 0, 1}, right = {1, 0, 0}, left = {-1, 0, 0};

	rotateDirectionToCameraLookAtDirection(forward, forward);
	rotateDirectionToCameraLookAtDirection(backward, backward);

	rotateDirectionToCameraLookAtDirection(right, right);
	rotateDirectionToCameraLookAtDirection(left, left);

	g_cameraVelocity[0] = 0; g_cameraVelocity[1] = 0; g_cameraVelocity[2] = 0; 

	if (g_forwardInput)
	{
		g_cameraVelocity[0] += forward[0];
		g_cameraVelocity[1] += forward[1];
		g_cameraVelocity[2] += forward[2];
	}

	if (g_backwardInput)
	{
		g_cameraVelocity[0] += backward[0];
		g_cameraVelocity[1] += backward[1];
		g_cameraVelocity[2] += backward[2];
	}

	if (g_rightInput)
	{
		g_cameraVelocity[0] += right[0];
		g_cameraVelocity[1] += right[1];
		g_cameraVelocity[2] += right[2];
	}

	if (g_leftInput)
	{
		g_cameraVelocity[0] += left[0];
		g_cameraVelocity[1] += left[1];
		g_cameraVelocity[2] += left[2];
	}

	translate_camera(g_cameraVelocity[0]*g_deltaTime*g_cameraMoveSpeed, g_cameraVelocity[1]*g_deltaTime*g_cameraMoveSpeed, g_cameraVelocity[2]*g_deltaTime*g_cameraMoveSpeed);
}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	renderWorkspace();
}

int main( int argc, char* argv[] )
{
	boolval result = initialize();
	if (result == true)
		return true;

	initialize_default_shaders();
	initialize_default_materials();

	initializeWorkspace();

	set_camera_move_speed(10);
	set_camera_rotate_speed(1);
	set_camera_position(0, 0, 0);
	set_camera_yaw(180);
	set_camera_pitch(0);
	set_camera_FOV(45);

	setAmbientLightColor(0.2, 0.2, 0.2);
	setAmbientLightIntensity(1);

	setDirectionalLightDirection(1, -1, 0);
	setDirectionalLightColor(1, 1, 1);
	setDirectionalLightIntensity(1);

	updateViewMatrix();



	PerspectiveObject *terrain = createPerspectiveObject();

	float *tquadVertices = NULL, *tquadNormals = NULL;
	size_t tquadVerticesCount, tquadNormalsCount;

	generateTessellatedQuad(&tquadVertices, &tquadNormals, 10, 100, terrain_heightmap_func, &tquadVerticesCount, &tquadNormalsCount);

	setObjectVBO(terrain, createAndFillVBO(tquadVertices, tquadVerticesCount*3*sizeof(float), GL_ARRAY_BUFFER, GL_STATIC_DRAW), VERTICES);
	setObjectVBO(terrain, createAndFillVBO(tquadNormals, tquadNormalsCount*3*sizeof(float), GL_ARRAY_BUFFER, GL_STATIC_DRAW), NORMALS);

	terrain->material = &g_grassTerrainMaterialLit;
	terrain->vertices = tquadVerticesCount;

	//crate

	/*PerspectiveObject *cube = createPerspectiveObject();

	float vertices[] = {
		0.5, 0.5, 0.5, 
		-0.5, 0.5, 0.5, 
		-0.5, -0.5, 0.5,

		0.5, 0.5, 0.5,
		-0.5, -0.5, 0.5,
		0.5, -0.5, 0.5,


		0.5, 0.5, -0.5, 
		-0.5, 0.5, -0.5, 
		-0.5, -0.5, -0.5,

		0.5, 0.5, -0.5,
		-0.5, -0.5, -0.5,
		0.5, -0.5, -0.5,


		0.5, 0.5, -0.5,
		0.5, 0.5, 0.5,
		0.5, -0.5, 0.5,

		0.5, 0.5, -0.5,
		0.5, -0.5, 0.5,
		0.5, -0.5, -0.5,


		-0.5, 0.5, -0.5,
		-0.5, 0.5, 0.5,
		-0.5, -0.5, 0.5,

		-0.5, 0.5, -0.5,
		-0.5, -0.5, 0.5,
		-0.5, -0.5, -0.5,


		0.5, 0.5, -0.5,
		-0.5, 0.5, -0.5,
		-0.5, 0.5, 0.5,

		0.5, 0.5, -0.5,
		-0.5, 0.5, 0.5,
		0.5, 0.5, 0.5,


		0.5, -0.5, -0.5,
		-0.5, -0.5, -0.5,
		-0.5, -0.5, 0.5,

		0.5, -0.5, -0.5,
		-0.5, -0.5, 0.5,
		0.5, -0.5, 0.5
	};

	float normals[] = {
		0, 0, 1,
		0, 0, 1,
		0, 0, 1,

		0, 0, 1,
		0, 0, 1,
		0, 0, 1,


		0, 0, -1,
		0, 0, -1,
		0, 0, -1,

		0, 0, -1,
		0, 0, -1,
		0, 0, -1,


		1, 0, 0,
		1, 0, 0,
		1, 0, 0,

		1, 0, 0,
		1, 0, 0,
		1, 0, 0,


		-1, 0, 0,
		-1, 0, 0,
		-1, 0, 0,

		-1, 0, 0,
		-1, 0, 0,
		-1, 0, 0,


		0, 1, 0,
		0, 1, 0,
		0, 1, 0,

		0, 1, 0,
		0, 1, 0,
		0, 1, 0,


		0, -1, 0,
		0, -1, 0,
		0, -1, 0,

		0, -1, 0,
		0, -1, 0,
		0, -1, 0
	};

	float UVs[] = {

		1, 1, 
		0, 1, 
		0, 0, 
		
		1, 1, 
		0, 0, 
		0, 1, 
		
		
		1, 1, 
		0, 1, 
		0, 0, 
		
		1, 1, 
		0, 0, 
		0, 1, 
		
		
		1, 1, 
		0, 1, 
		0, 0, 
		
		1, 1, 
		0, 0, 
		0, 1, 
		
		
		1, 1, 
		0, 1, 
		0, 0, 
		
		1, 1, 
		0, 0, 
		0, 1, 
		
		
		1, 1, 
		0, 1, 
		0, 0, 
		
		1, 1, 
		0, 0, 
		0, 1, 
		
		
		1, 1, 
		0, 1, 
		0, 0, 
		
		1, 1, 
		0, 0, 
		0, 1
		
	};

	setObjectVBO(cube, createAndFillVBO(&vertices[0], sizeof(vertices), GL_ARRAY_BUFFER, GL_STATIC_DRAW), VERTICES);
	setObjectVBO(cube, createAndFillVBO(&normals[0], sizeof(normals), GL_ARRAY_BUFFER, GL_STATIC_DRAW), NORMALS);
	setObjectVBO(cube, createAndFillVBO(&UVs[0], sizeof(UVs), GL_ARRAY_BUFFER, GL_STATIC_DRAW), UVS);

	cube->material = &g_crateMaterialLit;
	cube->vertices = sizeof(vertices) / (sizeof(float)*3);

	cube->position.z = 5;*/	

	double previousTime = glfwGetTime();

	while ( !(glfwWindowShouldClose(g_window) || g_exit ))
	{
		g_deltaTime = glfwGetTime() - previousTime;
		previousTime = glfwGetTime();

		update();
		render();

		glfwSwapBuffers(g_window);
		glfwPollEvents();
	}

	/*glDeleteBuffers(1, &cube->meshVBO);
	glDeleteBuffers(1, &cube->normalsVBO);
	glDeleteBuffers(1, &cube->UVsVBO);*/

	return result;
}
