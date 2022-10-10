#include "pch.h"

#include <glm/glm.hpp>
#include "MarchingCubesData.h"
#include "Vertex.h"

extern float Surface;

float Perlin3D(float x, float y, float z);
std::vector<MinimalVertex> TriangulateCube(const glm::ivec3& pos, float cube[8]);

float* GenerateNoise(int xSize, int ySize, int zSize, float noiseScale = 0.06f) {
	float* map = new float[xSize * ySize * zSize];

	for (int x = 0; x < xSize; x++) {
		for (int y = 0; y < ySize; y++) {
			for (int z = 0; z < zSize; z++) {
				map[x * ySize * zSize + y * zSize + z] = Perlin3D(x * noiseScale, y * noiseScale, z * noiseScale);
			}
		}
	}

	return map;
}



std::vector<MinimalVertex> GenerateMesh(int xSize, int ySize, int zSize) {
	float* map = GenerateNoise(xSize+1, ySize+1, zSize+1);

	std::vector<MinimalVertex> vertices;

	float cube[8];
	for (int x = 0; x < xSize; x++) {
		for (int y = 0; y < ySize; y++) {
			for (int z = 0; z < zSize; z++) {
				for (int i = 0; i < 8; i++) {
					int ix = x + VertexOffset[i].x;
					int iy = y + VertexOffset[i].y;
					int iz = z + VertexOffset[i].z;

					cube[i] = map[ix * ySize * zSize + iy * zSize + iz];
				}

				auto cubeTriangulation = TriangulateCube(glm::ivec3(x, y, z), cube);
				vertices.insert(vertices.end(), cubeTriangulation.begin(), cubeTriangulation.end());
			}
		}
	}
	delete[] map;

	return vertices;
}

/// <summary>
/// Find the approximate point of intersection of the surface
/// </summary>
float GetOffset(float v1, float v2)
{
	float delta = v2 - v1;
	return (delta == 0.0f) ? Surface : (Surface - v1) / delta;
}

glm::vec3 interpolateVerts(glm::vec4 v1, glm::vec4 v2)
{
	float t = (Surface - v1.w) / (v2.w - v1.w);
	return v1 + t * (v2 - v1);
}

std::vector<MinimalVertex> TriangulateCube(const glm::ivec3& pos, float cube[8])
{
	std::vector<MinimalVertex> vertices;

	unsigned char index = 0;

	for (int i = 0; i < 8; i++)
		if (cube[i] < Surface)
			index |= 1 << i;

	int edgeFlags = CubeEdgeFlags[index];

	if (edgeFlags == 0)
		return std::vector<MinimalVertex>();

	float offset;

	glm::vec3 EdgeVertex[12];

	for (int i = 0; i < 12; i++) {
		// is there an intersection on this egde
		if ((edgeFlags & (1 << i)) != 0)
		{
			offset = GetOffset(cube[cornerIndexAFromEdge[i]], cube[cornerIndexBFromEdge[i]]);

			EdgeVertex[i] = glm::vec3(pos + VertexOffset[cornerIndexAFromEdge[i]]) + offset * glm::vec3(EdgeDirection[i]);
		}
	}

	for (int i = 0; triangulation[index][i] != -1; i += 3) {
		int vert[3];

		for (int j = 0; j < 3; j++)
			vert[j] = triangulation[index][i + j];

		glm::vec3 U = EdgeVertex[vert[1]] - EdgeVertex[vert[0]];
		glm::vec3 V = EdgeVertex[vert[2]] - EdgeVertex[vert[0]];

		glm::vec3 normal = glm::cross(U, V);
		for (int j = 0; j < 3; j++) {
			vertices.emplace_back(
				EdgeVertex[vert[j]], // pos
				normal				// normal
			);
		}
	}

	return vertices;
}