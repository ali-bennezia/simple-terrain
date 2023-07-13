#version 330 core

layout(location = 0) in vec3 aPosition;

uniform mat4 iViewMatrix;
uniform float aspectRatio;

out vec3 unitPlanePosition;

void main()
{
	float i_aspectRatio = (1.0/aspectRatio);
	unitPlanePosition = (iViewMatrix * vec4(aPosition.x, aPosition.y*i_aspectRatio, -1.0, 0.0)).xyz;
	gl_Position = vec4(aPosition, 1.0);
}
