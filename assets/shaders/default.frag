#version 330 core

uniform vec3 ambientLightColor;
uniform float ambientLightIntensity;

uniform vec3 directionalLightDirection;
uniform vec3 directionalLightColor;
uniform float directionalLightIntensity;

uniform vec3 color;

in vec3 normal;

void main()
{
	//ambient light
	vec3 ambientLight = ambientLightColor*ambientLightIntensity;

	//directional light
	float dirQuotient = dot(normal, directionalLightDirection);
	float clamped_dirQuotient = abs(min(0, dirQuotient));
	vec3 directionalLight = color*directionalLightIntensity;
	directionalLight *= clamped_dirQuotient;

	//lights sum
	vec3 light = vec3(ambientLight.r + directionalLight.r, ambientLight.g + directionalLight.g, ambientLight.b + directionalLight.b);

	vec3 finalColor = vec3(color.r*light.r, color.g*light.g, color.b*light.b);

	gl_FragColor = vec4(finalColor, 1);
}
