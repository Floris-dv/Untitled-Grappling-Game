#pragma once
#include <glm/glm.hpp>
#include "Material.h"
#include "DataBuffers.h"
#include "Object.h"
#include "Camera.h"

class Level
{
public:
	struct Block {
		glm::vec3 Start;
		glm::vec3 End;

		inline static Object<SimpleVertex> Object;
	};

	Level(const glm::vec3& startPlatformSize, std::vector<Block>&& blocks, Material* levelBlocksTheme);

	// Assumes the MVP-matrix has been set
	void Render();

	void Render(Material* shader);

	const std::vector<Block>& GetBlocks() { return m_Blocks; }

	void UpdatePhysics(Camera& camera);

private:
	glm::vec3 m_StartPlatformSize; // Centered around 0 0 0

	std::vector<Block> m_Blocks;
	std::vector<glm::mat4> m_Matrices;

	Material* m_Material;

	VertexBuffer m_InstanceVBO;
};

