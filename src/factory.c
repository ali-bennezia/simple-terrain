#include "factory.h"

#include <stdlib.h>
//#include <stddef.h>
#include <math.h>

#include "utils.h"

Vec3fl neighborNormalsAverage(Vec3fl *a, Vec3fl *b, Vec3fl *c, Vec3fl *d)
{
	Vec3fl normal = {0, 0, 0};
	size_t count = 0;

	if (a != NULL){
		++count;
		normal = vec3fl_add(normal, *a);
	}

	if (b != NULL){
		++count;
		normal = vec3fl_add(normal, *b);
	}

	if (c != NULL){
		++count;
		normal = vec3fl_add(normal, *c);
	}

	if (d != NULL){
		++count;
		normal = vec3fl_add(normal, *d);
	}

	return count != 0 ? vec3fl_divide(normal, count) : normal;
}

Vec3fl getQuadNormal(float xCoordsOffset, float zCoordsOffset, int xQuadCoord, int zQuadCoord, float quadSideSize, float(*heightMapFunction)(float, float))
{
	Vec3fl vertexNW = {
		xQuadCoord * quadSideSize, 
		0,
		zQuadCoord * quadSideSize
	};
	Vec3fl vertexSW = {
		xQuadCoord * quadSideSize, 
		0,
		( zQuadCoord + 1 ) * quadSideSize
	};
	Vec3fl vertexNE = {
		( xQuadCoord + 1 ) * quadSideSize, 
		0,
		zQuadCoord * quadSideSize
	};

	float heightNW = heightMapFunction( xCoordsOffset + vertexNW.x, zCoordsOffset + vertexNW.z );
	float heightSW = heightMapFunction( xCoordsOffset + vertexSW.x, zCoordsOffset + vertexSW.z );
	float heightNE = heightMapFunction( xCoordsOffset + vertexNE.x, zCoordsOffset + vertexNE.z );

	vertexNW.y = heightNW; vertexSW.y = heightSW; vertexNE.y = heightNE;

	Vec3fl quadNormal = vec3fl_normalize(
		vec3fl_cross(
			vec3fl_difference( vertexSW, vertexNW ),
			vec3fl_difference( vertexNE, vertexNW )
		)
	);

	return quadNormal;
}

int generateTessellatedQuad(
	float xCoordsOffset,
	float zCoordsOffset,
	float** meshDestination, 
	float** normalsDestination, 
	unsigned int tessellations, 
	float size, 
	float(*heightMapFunction)(float, float), 
	size_t *verticesCount,
	size_t *normalsCount
)
{
	const size_t sideQuadsAmount = pow(2, tessellations);
	const float smallestWidth = (float)size / sideQuadsAmount;
	const size_t totalQuadsAmount = pow(sideQuadsAmount, 2);

	const size_t vertices = totalQuadsAmount*6;
	const size_t normals = totalQuadsAmount*6;

	*meshDestination = malloc(vertices*3*sizeof(float));
	*normalsDestination = malloc(normals*3*sizeof(float));

	*verticesCount = vertices;
	*normalsCount = normals;

	float* normalsPrecalculationBuffer = malloc(totalQuadsAmount*sizeof(float)*3);

	for (size_t x = 0; x < sideQuadsAmount; ++x){

		for (size_t z = 0; z < sideQuadsAmount; ++z){
	
			size_t quadIndex = z*sideQuadsAmount + x;
			size_t index = quadIndex*6*3;
			float quadPositionX = x*smallestWidth, quadPositionZ = z*smallestWidth;
			
			Vec2fl quadTopLeftPosition = {quadPositionX, quadPositionZ}, 
				quadTopRightPosition = {quadPositionX+smallestWidth, quadPositionZ},
				quadBottomLeftPosition = {quadPositionX, quadPositionZ+smallestWidth}, 
				quadBottomRightPosition = {quadPositionX+smallestWidth, quadPositionZ+smallestWidth};

			float quadTopLeftHeight = heightMapFunction(xCoordsOffset + quadTopLeftPosition.x, zCoordsOffset + quadTopLeftPosition.y),
				quadTopRightHeight = heightMapFunction(xCoordsOffset + quadTopRightPosition.x, zCoordsOffset + quadTopRightPosition.y),
				quadBottomLeftHeight = heightMapFunction(xCoordsOffset + quadBottomLeftPosition.x, zCoordsOffset + quadBottomLeftPosition.y),
				quadBottomRightHeight = heightMapFunction(xCoordsOffset + quadBottomRightPosition.x, zCoordsOffset + quadBottomRightPosition.y);

			Vec3fl quadTopLeftPositionVec3fl = {quadTopLeftPosition.x, quadTopLeftHeight, quadTopLeftPosition.y},
				quadBottomLeftPositionVec3fl = {quadBottomLeftPosition.x, quadBottomLeftHeight, quadBottomLeftPosition.y},
				quadTopRightPositionVec3fl = {quadTopRightPosition.x, quadTopRightHeight, quadTopRightPosition.y};

			Vec3fl quadNormal = vec3fl_normalize( 
				vec3fl_cross(
					vec3fl_difference( quadBottomLeftPositionVec3fl, quadTopLeftPositionVec3fl ),
					vec3fl_difference( quadTopRightPositionVec3fl, quadTopLeftPositionVec3fl )
				)
			);

			*(normalsPrecalculationBuffer + 3*quadIndex + 0) = quadNormal.x;
			*(normalsPrecalculationBuffer + 3*quadIndex + 1) = quadNormal.y;
			*(normalsPrecalculationBuffer + 3*quadIndex + 2) = quadNormal.z;

			*((*meshDestination) + index + 0) = quadTopLeftPosition.x;
			*((*meshDestination) + index + 1) = quadTopLeftHeight;
			*((*meshDestination) + index + 2) = quadTopLeftPosition.y;

			*((*meshDestination) + index + 3) = quadBottomLeftPosition.x;
			*((*meshDestination) + index + 4) = quadBottomLeftHeight;
			*((*meshDestination) + index + 5) = quadBottomLeftPosition.y;

			*((*meshDestination) + index + 6) = quadBottomRightPosition.x;
			*((*meshDestination) + index + 7) = quadBottomRightHeight;
			*((*meshDestination) + index + 8) = quadBottomRightPosition.y;


			*((*meshDestination) + index + 9) = quadTopLeftPosition.x;
			*((*meshDestination) + index + 10) = quadTopLeftHeight;
			*((*meshDestination) + index + 11) = quadTopLeftPosition.y;

			*((*meshDestination) + index + 12) = quadBottomRightPosition.x;
			*((*meshDestination) + index + 13) = quadBottomRightHeight;
			*((*meshDestination) + index + 14) = quadBottomRightPosition.y;

			*((*meshDestination) + index + 15) = quadTopRightPosition.x;
			*((*meshDestination) + index + 16) = quadTopRightHeight;
			*((*meshDestination) + index + 17) = quadTopRightPosition.y;

		}

	}

	for (size_t x = 0; x < sideQuadsAmount; ++x){

		for (size_t z = 0; z < sideQuadsAmount; ++z){

			size_t quadIndex = z*sideQuadsAmount + x;

			Vec3fl computedQuadNeighborNormalsNW, computedQuadNeighborNormalsN, computedQuadNeighborNormalsNE,
				computedQuadNeighborNormalsW, computedQuadNeighborNormalsE, 
				computedQuadNeighborNormalsSW, computedQuadNeighborNormalsS, computedQuadNeighborNormalsSE; 
				  

			if ( !( x > 0 && z > 0 ) )
				computedQuadNeighborNormalsNW = getQuadNormal( 
					xCoordsOffset, 
					zCoordsOffset, 
					x-1,
					z-1,
					smallestWidth, heightMapFunction
				);
			if ( !( z > 0 ) )
				computedQuadNeighborNormalsN = getQuadNormal( 
					xCoordsOffset, 
					zCoordsOffset, 
					x,
					z-1,
					smallestWidth, heightMapFunction
				);
			if ( !( x < (sideQuadsAmount - 1) && z > 0 ) )
				computedQuadNeighborNormalsNE = getQuadNormal( 
					xCoordsOffset, 
					zCoordsOffset, 
					x+1,
					z-1,
					smallestWidth, heightMapFunction
				);

			if ( !( x > 0 ) )
				computedQuadNeighborNormalsW = getQuadNormal( 
					xCoordsOffset, 
					zCoordsOffset, 
					x-1,
					z,
					smallestWidth, heightMapFunction
				);
			if ( !( x < (sideQuadsAmount - 1) ) )
				computedQuadNeighborNormalsE = getQuadNormal( 
					xCoordsOffset, 
					zCoordsOffset, 
					x+1,
					z,
					smallestWidth, heightMapFunction
				);

			if ( !( x > 0 && z < (sideQuadsAmount - 1) ) )
				computedQuadNeighborNormalsSW = getQuadNormal( 
					xCoordsOffset, 
					zCoordsOffset, 
					x-1,
					z+1,
					smallestWidth, heightMapFunction
				);
			if ( !( z < (sideQuadsAmount - 1) ) )
				computedQuadNeighborNormalsS = getQuadNormal( 
					xCoordsOffset, 
					zCoordsOffset, 
					x,
					z+1,
					smallestWidth, heightMapFunction
				);
			if ( !( x < (sideQuadsAmount-1) && z < (sideQuadsAmount-1) ) )
				computedQuadNeighborNormalsSE = getQuadNormal( 
					xCoordsOffset, 
					zCoordsOffset, 
					x+1,
					z+1,
					smallestWidth, heightMapFunction
				);

			Vec3fl  *quadNeighborNormalsNW = (x > 0 && z > 0) ? ((Vec3fl*)normalsPrecalculationBuffer + quadIndex - sideQuadsAmount - 1) : 
					&computedQuadNeighborNormalsNW,
				*quadNeighborNormalsN = (z > 0) ? ((Vec3fl*)normalsPrecalculationBuffer + quadIndex - sideQuadsAmount) : 
					&computedQuadNeighborNormalsN,
				*quadNeighborNormalsNE = (x < (sideQuadsAmount - 1) && z > 0) ? ((Vec3fl*)normalsPrecalculationBuffer + quadIndex - sideQuadsAmount + 1) : 
					&computedQuadNeighborNormalsNE,

				*quadNeighborNormalsW = (x > 0) ? ((Vec3fl*)normalsPrecalculationBuffer + quadIndex - 1) : 
					&computedQuadNeighborNormalsW,
				*quadNormals = (Vec3fl*)normalsPrecalculationBuffer + quadIndex,
				*quadNeighborNormalsE = (x < (sideQuadsAmount - 1)) ? ((Vec3fl*)normalsPrecalculationBuffer + quadIndex + 1) : 
					&computedQuadNeighborNormalsE,

				*quadNeighborNormalsSW = (x > 0 && z < (sideQuadsAmount - 1)) ? ((Vec3fl*)normalsPrecalculationBuffer + quadIndex + sideQuadsAmount - 1) : 
					&computedQuadNeighborNormalsSW,
				*quadNeighborNormalsS = (z < (sideQuadsAmount - 1)) ? ((Vec3fl*)normalsPrecalculationBuffer + quadIndex + sideQuadsAmount) : 
					&computedQuadNeighborNormalsS,
				*quadNeighborNormalsSE = (x < (sideQuadsAmount - 1) && z < (sideQuadsAmount - 1)) ? ((Vec3fl*)normalsPrecalculationBuffer + quadIndex + sideQuadsAmount + 1) : 
					&computedQuadNeighborNormalsSE;


			Vec3fl verticeNWNormal = neighborNormalsAverage(
				quadNeighborNormalsNW,
				quadNeighborNormalsN,
				quadNeighborNormalsW,
				quadNormals
			);

			Vec3fl verticeNENormal = neighborNormalsAverage(
				quadNeighborNormalsNE,
				quadNeighborNormalsN,
				quadNeighborNormalsE,
				quadNormals
			);

			Vec3fl verticeSWNormal = neighborNormalsAverage(
				quadNeighborNormalsSW,
				quadNeighborNormalsS,
				quadNeighborNormalsW,
				quadNormals
			);

			Vec3fl verticeSENormal = neighborNormalsAverage(
				quadNeighborNormalsSE,
				quadNeighborNormalsS,
				quadNeighborNormalsE,
				quadNormals
			);

			//NW, SW, SE, NW, SE, NE
						
			*((Vec3fl*)(*normalsDestination) + quadIndex*6 + 0) = verticeNWNormal;
			*((Vec3fl*)(*normalsDestination) + quadIndex*6 + 1) = verticeSWNormal;
			*((Vec3fl*)(*normalsDestination) + quadIndex*6 + 2) = verticeSENormal;
			*((Vec3fl*)(*normalsDestination) + quadIndex*6 + 3) = verticeNWNormal;
			*((Vec3fl*)(*normalsDestination) + quadIndex*6 + 4) = verticeSENormal;
			*((Vec3fl*)(*normalsDestination) + quadIndex*6 + 5) = verticeNENormal;

		}

	}

	free(normalsPrecalculationBuffer);

	return 0;
}
