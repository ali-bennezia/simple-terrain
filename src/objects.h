#ifndef OBJECTS_HEADERGUARD
#define OBJECTS_HEADERGUARD

#include <cglm/cglm.h>

#include "boolvals.h"
#include "materials.h"

enum BufferType {
	VERTICES,
	NORMALS,
	UVS
};

typedef struct PerspectiveObject {
	Vec3fl position, eulerAnglesRotation;

	GLuint meshVBO, normalsVBO, UVsVBO;
	size_t vertices;
	boolval meshInitialized, normalsInitialized, UVsInitialized;

	Material* material;
	boolval useDepth, visible;
} PerspectiveObject;

void setObjectVBO(PerspectiveObject* objPtr, GLuint vboHandle, enum BufferType type);

void getPerspectiveObjectModelMatrix(PerspectiveObject* obj, mat4 out);
void drawPerspectiveObject(PerspectiveObject* obj);

void initialize_workspace();
void clear_workspace();
void render_workspace();
PerspectiveObject* createPerspectiveObject();
void deletePerspectiveObject( PerspectiveObject *obj );

#endif
