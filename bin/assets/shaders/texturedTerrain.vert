#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;

uniform mat4 modelMatrix;
uniform mat4 viewMatrix;
uniform mat4 projectionMatrix;
uniform mat4 normalMatrix;

uniform float textureTiles;

out vec3 normal;
out vec2 UVs;

void main()
{
	vec4 normalVec4 = normalMatrix * vec4(aNormal, 0.0);
	vec4 worldPos = modelMatrix * vec4(aPosition, 1.0);
	
	UVs =  worldPos.xz / textureTiles;
	normal = normalVec4.xyz;
	gl_Position = projectionMatrix * viewMatrix * worldPos;
}
