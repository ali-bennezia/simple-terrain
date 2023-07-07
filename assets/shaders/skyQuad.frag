#version 330 core

uniform float cameraFOV;
uniform float aspectRatio;
uniform float cameraPitch;

uniform ivec2 viewportDimensions;

void main()
{
	float verticalFOV = cameraFOV * (1.0/aspectRatio);

	vec3 finalColor = vec3(0.529, 0.807, 0.921);

	gl_FragColor = vec4(finalColor, 1);
}
