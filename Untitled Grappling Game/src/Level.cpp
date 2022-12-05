#include "pch.h"
#include "Level.h"
#include "VertexData.h"

Level::Level(const glm::vec3& startPlatformSize, const std::vector<Block>& blocks, Material* levelBlocksTheme) : m_StartPlatformSize(startPlatformSize), m_Material(levelBlocksTheme)
{
	std::vector<glm::mat4> matrices;
	m_Matrices.reserve(blocks.size());

	for (const Block& block : blocks) {
		glm::mat4 matrix(1.0f);

		matrix = glm::translate(matrix, block.Start);
		matrix = glm::scale(matrix, glm::abs(block.End - block.Start));

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

void Level::Render(Shader& shader) {
	Block::Object.DrawInstanced(shader, true, m_Matrices.size());
}
