#version 330 core

#define PI 3.1415926538

uniform float cameraFOV;
uniform float aspectRatio;
uniform float cameraPitch;

uniform ivec2 viewportDimensions;

void main()
{
	const float toRad = PI/180.0;

	float verticalFOV = cameraFOV * (1.0/aspectRatio);
	float verticalFOVrads = verticalFOV*toRad;
	float verticalHalfFOVrads = verticalFOVrads / 2.0;


	vec3 blueColor = vec3(0.529, 0.807, 0.921);
	vec3 lightBlueColor = vec3(88.0/100.0, 99.0/100.0, 1);

	vec2 clipCoords = gl_FragCoord.xy / viewportDimensions;
	float verticalQuotient = pow(clipCoords.y, 2);

	vec3 finalColor = blueColor*verticalQuotient + lightBlueColor*(1.0-verticalQuotient);

	gl_FragColor = vec4(finalColor, 1);
}
