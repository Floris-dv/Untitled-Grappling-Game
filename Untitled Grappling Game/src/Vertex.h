#pragma once
#include "DataBuffers.h"

struct Vertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;

	inline static BufferLayout Layout{ {{GL_FLOAT, 3}, {GL_FLOAT, 3}, {GL_FLOAT, 2}, {GL_FLOAT, 3}, {GL_FLOAT, 3}} };
};

struct SimpleVertex {
	glm::vec3 Position;
	glm::vec3 Normal;
	glm::vec2 TexCoords;

	inline static BufferLayout Layout{ {{GL_FLOAT, 3}, {GL_FLOAT, 3}, {GL_FLOAT, 2}} };
};

struct MinimalVertex {
	glm::vec3 Position;
	glm::vec3 Normal;

	inline static BufferLayout Layout{ {{GL_FLOAT, 3}, {GL_FLOAT, 3}} };
};

