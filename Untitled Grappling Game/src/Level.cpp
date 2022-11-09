#include "pch.h"
#include "Level.h"
#include "VertexData.h"

Level::Level(const glm::vec3& startPlatformSize, const std::vector<Block>& blocks, const Material& levelBlocksTheme) : m_StartPlatformSize(startPlatformSize), m_Blocks(blocks), m_VAO(), m_LevelBlocksTheme(std::move(levelBlocksTheme))
{
	std::vector<glm::mat4> matrices;
	matrices.reserve(m_Blocks.size());
	
	for (const Block& block : m_Blocks) {
		glm::mat4 matrix(1.0f);

		matrix = glm::scale(matrix, glm::abs(block.End - block.Start));
		matrix = glm::translate(matrix, block.Start);

		matrices.push_back(matrix);
	}

	static const BufferLayout instancedBufferLayout{ {{GL_FLOAT, 4}, {GL_FLOAT, 4}, {GL_FLOAT, 4}, {GL_FLOAT, 4}}, 5 };
	m_InstanceVBO = VertexBuffer(matrices.size() * sizeof(glm::mat4), matrices.data());

	if (!Block::Object.IsValid())
		Block::Object = Object<SimpleVertex>(&m_LevelBlocksTheme, boxVertices, SimpleVertex::Layout);
	
	Block::Object.SetInstanceBuffer(m_InstanceVBO);

	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)0);
	glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)12);
	glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)24);
	glVertexAttribPointer(8, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)36);
	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	glVertexAttribDivisor(7, 1);
	glVertexAttribDivisor(8, 1);
	
	m_InstanceVBO.Bind();

}

void Level::Render()
{
	Block::Object.DrawInstanced(true, m_Blocks.size());
}

void Level::Render(Shader& shader) {
	Block::Object.VAO.Bind();




	Block::Object.DrawInstanced(shader, true, m_Blocks.size());
}
