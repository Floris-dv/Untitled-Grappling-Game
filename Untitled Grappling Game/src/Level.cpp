#include "pch.h"
#include "Level.h"
#include "VertexData.h"
#include "imgui/imgui.h"
#include <glm/glm.hpp>

Level::Level(const glm::vec3& startPlatformSize, std::vector<Block>&& blocks, Material* levelBlocksTheme) : m_StartPlatformSize(startPlatformSize), m_Blocks(std::move(blocks)), m_Material(levelBlocksTheme)
{
	std::vector<glm::mat4> matrices;
	m_Matrices.reserve(m_Blocks.size());

	for (Block& block : m_Blocks) {
		glm::mat4 matrix(1.0f);

		glm::vec3 start = block.Start;
		block.Start = glm::min(start, block.End);
		block.End = glm::max(start, block.End);

		// Extra factors are for help with that boxVertices are -1 to 1 instead of 0-1
		matrix *= glm::scale(glm::translate(glm::mat4(1.0f), 0.5f * (block.Start + block.End)), (block.End - block.Start) * 0.5f);

		m_Matrices.push_back(matrix);
	}

	static const BufferLayout instancedBufferLayout{ {{GL_FLOAT, 4}, {GL_FLOAT, 4}, {GL_FLOAT, 4}, {GL_FLOAT, 4}}, 5 };

	m_InstanceVBO = VertexBuffer(m_Matrices.size() * sizeof(m_Matrices[0]), m_Matrices.data());

	if (!Block::Object.IsValid())
		Block::Object = Object<SimpleVertex>(m_Material, boxVertices, SimpleVertex::Layout);

	Block::Object.SetInstanceBuffer(m_InstanceVBO);
}

void Level::Render()
{
	Block::Object.DrawInstanced(true, m_Matrices.size());
}

void Level::Render(Material* material) {
	Block::Object.DrawInstanced(material, true, m_Matrices.size());
}

void Level::UpdatePhysics(Camera& camera)
{
	for (const Level::Block& block : m_Blocks) {
		glm::bvec3 s = glm::lessThan(block.Start, camera.Position) && glm::lessThan(camera.Position, block.End);
		if (glm::all(s)) {
			ImGui::Text("Inside!");
		}
	}
}
