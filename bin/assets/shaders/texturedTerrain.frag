#version 330 core

uniform vec3 ambientLightColor;
uniform float ambientLightIntensity;

uniform vec3 directionalLightDirection;
uniform vec3 directionalLightColor;
uniform float directionalLightIntensity;

uniform vec3 color;
uniform sampler2D albedoTexture_y;
uniform sampler2D albedoTexture_x;
uniform sampler2D albedoTexture_z;

in vec3 normal;
in vec2 UVs;

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
	

//	vec4 textureColor = texture2D(albedoTexture_y, UVs.xy);
	vec4 textureColor = texture2D( albedoTexture_y, UVs.xy ) * abs( dot( normal, vec3(0.0f, 1.0f, 0.0f) )) + texture2D( albedoTexture_x, UVs.xy ) * abs(dot( normal, vec3(1.0f, 0.0f, 0.0f) )) + texture2D( albedoTexture_z, UVs.xy ) * abs(dot( normal, vec3(0.0f, 0.0f, 1.0f) )) ;
	vec3 finalColor = vec3(color.r*textureColor.r*light.r, color.g*textureColor.g*light.g, color.b*textureColor.b*light.b);

	gl_FragColor = vec4(finalColor, 1);
}
