#pragma once
#include <glm/glm.hpp>
#include "Material.h"
#include "DataBuffers.h"
#include "Object.h"

class Level
{
public:
	struct Block {
		glm::vec3 Start;
		glm::vec3 End;

		inline static Object<SimpleVertex> Object;
	};

	Level(const glm::vec3& startPlatformSize, const std::vector<Block>& blocks, const Material& levelBlocksTheme);

	// Assumes the MVP-matrix has been set
	void Render();

	void Render(Shader& shader);

	VertexArray m_VAO;
	VertexBuffer m_InstanceVBO;

private:
	glm::vec3 m_StartPlatformSize; // Centered around 0 0 0

	std::vector<Block> m_Blocks;

	Material m_LevelBlocksTheme;
};

