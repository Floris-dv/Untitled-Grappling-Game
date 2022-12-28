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

	Level(const glm::vec3& startPlatformSize, const Block& finishBox, std::vector<Block>&& blocks, std::shared_ptr<Material> levelBlocksTheme);

	// TODO: make the levelFile store the material.
	Level(std::string_view levelFile, std::shared_ptr<Shader> levelBlocksTheme);

	void Write(std::string_view levelFile);

	// Assumes the MVP-matrix has been set
	void Render();

	void Render(Material* shader);

	const std::vector<Block>& GetBlocks() { return m_Blocks; }

	void UpdatePhysics(Camera& camera);

private:
	std::vector<Block> m_Blocks;
	std::vector<glm::mat4> m_Matrices;

	Block m_FinishBox;

	std::shared_ptr<Material> m_Material;

	VertexBuffer m_InstanceVBO;

	static constexpr uint32_t VERSION_NR = 0;

	// Uses m_Blocks and m_StartPlatformSize to setup m_Matrices and m_InstanceVBO
	void SetupMatricesInstanceVBO();
};

