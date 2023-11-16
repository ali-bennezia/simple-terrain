#version 330 core

#define PI 3.1415926538

uniform vec3 sunDirection;

in vec3 unitPlanePosition;

struct LatitudeLongitude {
	float latitude;
	float longitude;
};

LatitudeLongitude calculate_normal_latitude_longitude( vec3 normal )
{
	LatitudeLongitude ret;
	
	vec2 normalCoordsXZ = normalize( normal.xz );
	vec2 normalCoordsXZY = normalize( vec2( length( normal.xz ), normal.y ) );

	ret.latitude = atan( normalCoordsXZY.y, normalCoordsXZY.x );
	ret.longitude = atan( normalCoordsXZ.y, normalCoordsXZ.x );

	return ret;
}

float calculate_angle_between_normals( vec3 a, vec3 b )
{
	return acos( dot( a,b ) );
}

vec3 render_sky( vec3 fragColor, vec3 unitSphereHitpoint )
{
	LatitudeLongitude coords = calculate_normal_latitude_longitude( unitSphereHitpoint );

	float latitude = coords.latitude;
	float longitude = coords.longitude;

	//vec3 topBottomColor = vec3(0.509, 0.787, 0.901);
	//vec3 horizonColor = vec3(80.0/100.0, 90.0/100.0, 0.95);

	vec3 topBottomColor = vec3( 0.745, 0.631, 0.949 );
	vec3 horizonColor = vec3( 0.98, 0.816, 0.98 );

	float verticalQuotient = abs( latitude ) / ( PI / 2.0 );

	return fragColor + (1.0-verticalQuotient)*horizonColor + verticalQuotient*topBottomColor; 
}

vec3 render_sun( vec3 fragColor, vec3 unitSphereHitpoint, vec3 skyDirection, float sphereSize, float sphereFadeOutSize, float glowSize )
{
	float angleDistance = calculate_angle_between_normals( unitSphereHitpoint, skyDirection );
	float drawQuotient = 1.0 - smoothstep( sphereSize, sphereSize+sphereFadeOutSize, angleDistance );
	drawQuotient += glowSize / (4.0*pow( angleDistance, 2 ));
	return fragColor + vec3( drawQuotient );
}


void main()
{
	vec3 unitSphereHitpoint = normalize( unitPlanePosition );
	vec3 finalColor = vec3(0);

	//sky color

	finalColor = render_sky( finalColor, unitSphereHitpoint );

	//suns

	finalColor = render_sun( finalColor, unitSphereHitpoint, sunDirection, 0.002, 0.002, 0.0025 );
	finalColor = render_sun( finalColor, unitSphereHitpoint, normalize( vec3(-1.4, 1, 0.2) ), 0.00002, 0.001, 0.0004 );

	//output

	finalColor = clamp( finalColor, 0, 1 );
	gl_FragColor = vec4(finalColor, 1);
}
