#pragma once

// Header file for vertex data generation and data:
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Vertex.h"

// Simple sphere generation, not anything fancy
static std::pair<std::vector<MinimalVertex>, std::vector<unsigned int>> CreateSphere(size_t stacks, size_t slices);

// Perlin noise with marching cubes mesh generation
std::vector<MinimalVertex> GenerateMesh(int xSize, int ySize, int zSize);

static std::pair<glm::vec3, glm::vec3> GenerateTanBitan(glm::vec3 pos[3], glm::vec2 uv[3], glm::vec3 nm);

static void GenerateModelMatricesInRing(const unsigned int amount, glm::mat4* modelMatrices, const float radius, const float displacement);

constexpr std::array<SimpleVertex, 36> boxVertices = { {
		// Positions				// Normals				// Texture coords
		// Front face
		{{-1.0f, -1.0f,  1.0f},		{0.0f, 0.0f,  1.0f},	{0.0f, 0.0f}},
		{{ 1.0f, -1.0f,  1.0f},		{0.0f, 0.0f,  1.0f},	{1.0f, 0.0f}},
		{{ 1.0f,  1.0f,  1.0f},		{0.0f, 0.0f,  1.0f},	{1.0f, 1.0f}},
		{{ 1.0f,  1.0f,  1.0f},		{0.0f, 0.0f,  1.0f},	{1.0f, 1.0f}},
		{{-1.0f,  1.0f,  1.0f},		{0.0f, 0.0f,  1.0f},	{0.0f, 1.0f}},
		{{-1.0f, -1.0f,  1.0f},		{0.0f, 0.0f,  1.0f},	{0.0f, 0.0f}},

		// Right face
		{{ 1.0f, -1.0f,  1.0f},		{ 1.0f, 0.0f, 0.0f},	{0.0f, 1.0f}},
		{{ 1.0f, -1.0f, -1.0f},		{ 1.0f, 0.0f, 0.0f},	{0.0f, 0.0f}},
		{{ 1.0f,  1.0f, -1.0f},		{ 1.0f, 0.0f, 0.0f},	{1.0f, 0.0f}},
		{{ 1.0f,  1.0f, -1.0f},		{ 1.0f, 0.0f, 0.0f},	{1.0f, 0.0f}},
		{{ 1.0f,  1.0f,  1.0f},		{ 1.0f, 0.0f, 0.0f},	{1.0f, 1.0f}},
		{{ 1.0f, -1.0f,  1.0f},		{ 1.0f, 0.0f, 0.0f},	{0.0f, 1.0f}},

		// Back
		{{ 1.0f, -1.0f, -1.0f},		{0.0f, 0.0f, -1.0f},	{1.0f, 0.0f}},
		{{-1.0f, -1.0f, -1.0f},		{0.0f, 0.0f, -1.0f},	{0.0f, 0.0f}},
		{{-1.0f,  1.0f, -1.0f},		{0.0f, 0.0f, -1.0f},	{0.0f, 1.0f}},
		{{-1.0f,  1.0f, -1.0f},		{0.0f, 0.0f, -1.0f},	{0.0f, 1.0f}},
		{{ 1.0f,  1.0f, -1.0f},		{0.0f, 0.0f, -1.0f},	{1.0f, 1.0f}},
		{{ 1.0f, -1.0f, -1.0f},		{0.0f, 0.0f, -1.0f},	{1.0f, 0.0f}},

		// Left
		{{-1.0f, -1.0f, -1.0f},		{-1.0f, 0.0f, 0.0f},	{0.0f, 0.0f}},
		{{-1.0f, -1.0f,  1.0f},		{-1.0f, 0.0f, 0.0f},	{0.0f, 1.0f}},
		{{-1.0f,  1.0f,  1.0f},		{-1.0f, 0.0f, 0.0f},	{1.0f, 1.0f}},
		{{-1.0f,  1.0f,  1.0f},		{-1.0f, 0.0f, 0.0f},	{1.0f, 1.0f}},
		{{-1.0f,  1.0f, -1.0f},		{-1.0f, 0.0f, 0.0f},	{1.0f, 0.0f}},
		{{-1.0f, -1.0f, -1.0f},		{-1.0f, 0.0f, 0.0f},	{0.0f, 0.0f}},

		// top
		{{-1.0f,  1.0f,  1.0f},		{0.0f,  1.0f, 0.0f},	{0.0f, 1.0f}},
		{{ 1.0f,  1.0f,  1.0f},		{0.0f,  1.0f, 0.0f},	{1.0f, 1.0f}},
		{{ 1.0f,  1.0f, -1.0f},		{0.0f,  1.0f, 0.0f},	{1.0f, 0.0f}},
		{{ 1.0f,  1.0f, -1.0f},		{0.0f,  1.0f, 0.0f},	{1.0f, 0.0f}},
		{{-1.0f,  1.0f, -1.0f},		{0.0f,  1.0f, 0.0f},	{0.0f, 0.0f}},
		{{-1.0f,  1.0f,  1.0f},		{0.0f,  1.0f, 0.0f},	{0.0f, 1.0f}},

		// bottom
		{{ 1.0f, -1.0f,  1.0f},		{0.0f, -1.0f, 0.0f},	{1.0f, 1.0f}},
		{{-1.0f, -1.0f,  1.0f},		{0.0f, -1.0f, 0.0f},	{0.0f, 1.0f}},
		{{-1.0f, -1.0f, -1.0f},		{0.0f, -1.0f, 0.0f},	{0.0f, 0.0f}},
		{{-1.0f, -1.0f, -1.0f},		{0.0f, -1.0f, 0.0f},	{0.0f, 0.0f}},
		{{ 1.0f, -1.0f, -1.0f},		{0.0f, -1.0f, 0.0f},	{1.0f, 0.0f}},
		{{ 1.0f, -1.0f,  1.0f},		{0.0f, -1.0f, 0.0f},	{1.0f, 1.0f}},
	} };

constexpr std::array<Vertex, 6> floorVertices = { {
		// positions				// normal				// texture coords	// tangents				// bitangents
		{{-10.0f, -7.5f, -10.0f},	{0.0f, 1.0f, 0.0f},		{0.0f, 0.0f},		{1.0f, 0.0f, 0.0f},		{0.0f, 0.0f, 1.0f}},
		{{-10.0f, -7.5f,  10.0f},	{0.0f, 1.0f, 0.0f},		{0.0f, 2.0f},		{1.0f, 0.0f, 0.0f},		{0.0f, 0.0f, 1.0f}},
		{{ 10.0f, -7.5f,  10.0f},	{0.0f, 1.0f, 0.0f},		{2.0f, 2.0f},		{1.0f, 0.0f, 0.0f},		{0.0f, 0.0f, 1.0f}},
		{{-10.0f, -7.5f, -10.0f},	{0.0f, 1.0f, 0.0f},		{0.0f, 0.0f},		{1.0f, 0.0f, 0.0f},		{0.0f, 0.0f, 1.0f}},
		{{ 10.0f, -7.5f,  10.0f},	{0.0f, 1.0f, 0.0f},		{2.0f, 2.0f},		{1.0f, 0.0f, 0.0f},		{0.0f, 0.0f, 1.0f}},
		{{ 10.0f, -7.5f, -10.0f},	{0.0f, 1.0f, 0.0f},		{2.0f, 0.0f},		{1.0f, 0.0f, 0.0f},		{0.0f, 0.0f, 1.0f}},
	} };

// Data for light uniform:
constexpr std::array<float, 120> lightData = { {
		// Dirlight:
		-0.2f, -1.0f, -0.3f,    0.0f,	// direction (+ padding)
		0.2f, 0.2f, 0.2f,		0.0f,	// ambient (+ padding)
		1.0f, 1.0f, 1.0f,       0.0f,	// diffuse (+ padding)
		0.5f, 0.5f, 0.5f,       0.0f,	// specular (+ padding)

		// Pointlight[0]:
		0.7f, 0.2f, 2.0f,		0.0f,	// position (+ padding)
		0.05f, 0.05f, 0.05f,	0.0f,	// ambient (+ padding)
		2.0f, 2.0f, 2.0f,       0.0f,	// diffuse (+ padding)
		0.1f, 0.1f, 0.1f,		0.0f,	// specular (+ padding)
		0.09f,							// linear;
		0.032f,							// quadratic;
		0.0f, 0.0f,						// padding

		// Pointlight[1]:
		2.3f, -0.5f, -3.3f,		0.0f,	// position (+ padding)
		0.05f, 0.05f, 0.05f,	0.0f,	// ambient (+ padding)
		2.0f, 2.0f, 2.0f,       0.0f,	// diffuse (+ padding)
		0.1f, 0.1f, 0.1f,		0.0f,	// specular (+ padding)
		0.09f,							// linear;
		0.032f,							// quadratic;
		0.0f, 0.0f,						// padding

		// Pointlight[2]:
		-1.3f, 0.4f, 2.0f,		0.0f,	// position (+ padding)
		0.05f, 0.05f, 0.05f,	0.0f,	// ambient (+ padding)
		1.0f, 1.0f, 1.0f,       0.0f,	// diffuse (+ padding)
		0.1f, 0.1f, 0.1f,		0.0f,	// specular (+ padding)
		0.09f,							// linear;
		0.032f,							// quadratic;
		0.0f, 0.0f,						// padding

		// Pointlight[3]:
		0.0f, 0.0f, -3.0f,		0.0f,	// position (+ padding)
		0.05f, 0.05f, 0.05f,	0.0f,	// ambient (+ padding)
		1.0f, 1.0f, 1.0f,       0.0f,	// diffuse (+ padding)
		0.1f, 0.1f, 0.1f,		0.0f,	// specular (+ padding)
		0.09f,							// linear;
		0.032f,							// quadratic;
		0.0f, 0.0f,						// padding

		// Spotlight:
		0.0f, 0.0f, 3.0f,		0.0f,	// position (+ padding)
		0.0f, 0.0f, -1.0f,		0.0f,	// direction (+ padding)

		0.0f, 0.0f, 0.0f,		0.0f,	// ambient (+ padding)
		10.0f, 10.0f, 10.0f,	0.0f,	// diffuse (+ padding)
		1.0f, 1.0f, 1.0f,		0.0f,	// specular (+ padding)

		0.09f,							// linear
		0.032f,							// quadratic

		0.9762960071199334f,			// cutOff ( = cos(15 degrees)
		0.9659258262890683f,			// outerCutOff ( = cos(17.5 degrees)
	} };

static std::pair<std::vector<MinimalVertex>, std::vector<unsigned int>> CreateSphere(size_t stacks, size_t slices) {
	static constexpr float PI = glm::pi<float>();
	std::vector<glm::vec3> positions;
	positions.reserve(stacks * slices);
	std::vector<unsigned int> indices;
	indices.reserve((slices * stacks + slices) * 6);

	// loop through stacks.
	for (size_t i = 0; i <= stacks; ++i) {
		float V = (float)i / (float)stacks;
		float phi = V * PI;

		// loop through the slices.
		for (size_t j = 0; j <= slices; ++j) {
			float U = (float)j / (float)slices;
			float theta = U * (PI * 2);

			// use spherical coordinates to calculate the positions.
			float x = cos(theta) * sin(phi);
			float y = cos(phi);
			float z = sin(theta) * sin(phi);

			positions.push_back({ x, y, z });
		}
	}

	std::vector<MinimalVertex> vertices;
	vertices.reserve(stacks * slices * 2);

	// Calc The Index Positions
	for (int i = 0; i < slices * stacks + slices; ++i) {
		indices.push_back((unsigned int)(i * 2));
		indices.push_back((unsigned int)((i + slices + 1) * 2));
		indices.push_back((unsigned int)((i + slices) * 2));
		// MAYBE: add tangents/bitangents here
		vertices.emplace_back(
			positions[i],
			glm::cross(positions[i] - positions[i + slices], positions[i] - positions[i + slices + 1])
		);
		indices.push_back((unsigned int)(i * 2));
		indices.push_back((unsigned int)((i + 1) * 2));
		indices.push_back((unsigned int)((i + slices + 1) * 2));

		vertices.emplace_back(
			positions[i],
			glm::cross(positions[i] - positions[i + slices + 1], positions[i] - positions[i + 1])
		);
	}

	return std::make_pair(vertices, indices);
}

static void GenerateModelMatricesInRing(const unsigned int amount, glm::mat4* modelMatrices, const float radius, const float displacementOffset)
{
	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_real_distribution<float> displacement(-displacementOffset, displacementOffset);
	std::uniform_real_distribution<float> scale(0.25f, 1.0f);
	std::uniform_real_distribution<float> rotation(0.0f, 360.0f);

	// transform the x and z position of the asteroid along a circle with a radius of radius
	for (unsigned int i = 0; i < amount; i++) {
		glm::mat4 model(1.0f);
		// 1. translation: displace along circle with 'r' in range [-displacementOffset, displacementOffset]
		float angle = (float)i / (float)amount * 360.0f;

		float x = glm::sin(angle) * radius + displacement(gen);
		float y = displacement(gen) * 0.4f + 10.0f; // keep it less of a thing, as I want it to be more like reality
		float z = glm::cos(angle) * radius + displacement(gen);

		model = glm::translate(model, glm::vec3(x, y, z));

		// 2. scale randomly between 0.05 and 0.25f
		model = glm::scale(model, glm::vec3(scale(gen)));

		// 3. add random rotation
		float rotAngle = (float)(rand() % 360);
		model = glm::rotate(model, rotation(gen), glm::vec3(0.4f, 0.6f, 0.8f));

		// 4. add to list of matrices:
		modelMatrices[i] = model;
	}
}
