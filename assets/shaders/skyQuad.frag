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

void main()
{
	vec3 unitSphereHitpoint = normalize( unitPlanePosition );

	//sky color

	LatitudeLongitude coords = calculate_normal_latitude_longitude( unitSphereHitpoint );

	float latitude = coords.latitude;
	float longitude = coords.longitude;

	vec3 blueColor = vec3(0.509, 0.787, 0.901);
	vec3 lightBlueColor = vec3(80.0/100.0, 90.0/100.0, 0.95);

	float verticalQuotient = abs( latitude ) / ( PI / 2.0 );

	vec3 finalColor = (1.0-verticalQuotient)*lightBlueColor + verticalQuotient*blueColor; 

	//sun

	float angleDistance = calculate_angle_between_normals( unitSphereHitpoint, sunDirection );
	float drawQuotient = 1.0 - smoothstep( 0.015, 0.022, angleDistance );
	drawQuotient += 0.0125 / pow( angleDistance, 2 );
	finalColor += vec3( drawQuotient );

	//output

	finalColor = clamp( finalColor, 0, 1 );
	gl_FragColor = vec4(finalColor, 1);
}
