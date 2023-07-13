#version 330 core

#define PI 3.1415926538

in vec3 unitPlanePosition;

void main()
{
	vec3 unitSphereHitpoint = normalize( unitPlanePosition );
	vec2 hitpointCoordsXZ = normalize( unitSphereHitpoint.xz );
	vec2 hitpointCoordsXZY = normalize( vec2( length( unitSphereHitpoint.xz ), unitSphereHitpoint.y ) );

	float hitpointLongitude = atan( hitpointCoordsXZ.y, hitpointCoordsXZ.x );
	float hitpointLatitude = atan( hitpointCoordsXZY.y, hitpointCoordsXZY.x );

	vec3 blueColor = vec3(0.509, 0.787, 0.901);
	vec3 lightBlueColor = vec3(88.0/100.0, 99.0/100.0, 1);

	float verticalQuotient = abs( hitpointLatitude ) / ( PI / 2.0 );

	vec3 finalColor = (1.0-verticalQuotient)*lightBlueColor + verticalQuotient*blueColor; 

	gl_FragColor = vec4(finalColor, 1);
}
