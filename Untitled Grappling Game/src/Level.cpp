#include "pch.h"
#define OVERLOAD 0
#include "Log.h"
#include "Level.h"
#include "VertexData.h"
#include "imgui/imgui.h"
#include <glm/glm.hpp>

template<typename OStream>
inline OStream& operator<<(OStream& output, Level::Block const& input) {
	output << input.Start;
	output << input.End;

	return output;
}

template<typename IStream>
inline IStream& operator>>(IStream& input, Level::Block& output) {
	input >> output.Start;
	input >> output.End;

	return input;
}


Level::Level(const glm::vec3& startPlatformSize, const Level::Block& finishBox, std::vector<Block>&& blocks, std::shared_ptr<Material> levelBlocksTheme) :
	m_FinishBox(finishBox), m_Blocks(std::move(blocks)), m_Material(std::move(levelBlocksTheme))
{
	m_Blocks.push_back({ { -startPlatformSize.x * 0.5f, -startPlatformSize.y, -startPlatformSize.z * 0.5f}, {startPlatformSize.x * 0.5f, 0.0f, startPlatformSize.z * 0.5f} });
	SetupMatricesInstanceVBO();
}

Level::Level(std::string_view levelFile, std::shared_ptr<Shader> shader)
{
	std::ifstream file(levelFile.data(), std::ios::binary);

	if (!file)
		NG_ERROR("File {} doesn't exist", levelFile);

	uint32_t version_nr;
	file >> version_nr;

	file >> m_Blocks;
	file >> m_FinishBox;

	m_Material = std::make_shared<Material>(Material::Deserialize(file, shader));

	SetupMatricesInstanceVBO();
}

void Level::Write(std::string_view levelFile)
{
	std::ofstream file(levelFile.data(), std::ios::binary);

	file << VERSION_NR << '\n';

	file << m_Blocks;

	file << m_FinishBox;

	m_Material->Serialize(file);
}

void Level::Render()
{
	Block::Object.DrawInstanced(true, m_Matrices.size());
}

void Level::Render(Material* material) {
	Block::Object.DrawInstanced(material, true, m_Matrices.size());
}

static bool CollidesWith(const Level::Block& block, const glm::vec3& point) {
	glm::bvec3 s = glm::lessThan(block.Start, point) && glm::lessThan(point, block.End);
	return glm::all(s);
}

// Min function, but seeing which is larger is based on the absolute value
static float AbsMin(float a, float b) {
	return glm::abs(a) > glm::abs(b) ? b : a;
}

// Assumes camera is colliding with block
static void BounceOn(const Level::Block& block, Camera& camera) {
	// camera.Vel -= (block.Start + block.End) * 0.5f - camera.Position * PUSHFACTOR;

	glm::vec3 min = {
		AbsMin(block.Start.x - camera.PhysicsPosition.x, block.End.x - camera.PhysicsPosition.x),
		AbsMin(block.Start.y - camera.PhysicsPosition.y, block.End.y - camera.PhysicsPosition.y),
		AbsMin(block.Start.z - camera.PhysicsPosition.z, block.End.z - camera.PhysicsPosition.z)
	};

	if (glm::abs(min.x) > glm::abs(min.y)) {
		if (glm::abs(min.z) > glm::abs(min.y)) {
			camera.Position.y += min.y;
			camera.Vel.y = 0.0f;
		}
		else {
			camera.Position.z += min.z;
			camera.Vel.z = 0.0f;
		}
	}
	else {
		if (glm::abs(min.z) > glm::abs(min.x)) {
			camera.Position.x += min.x;
			camera.Vel.x = 0.0f;
		}
		else {
			camera.Position.z += min.z;
			camera.Vel.z = 0.0f;
		}
	}

	camera.PhysicsPosition = { camera.Position.x, camera.Position.y - PHYSICSOFFSET, camera.Position.x };
}

void Level::UpdatePhysics(Camera& camera)
{
	for (const Level::Block& block : m_Blocks) {
		if (CollidesWith(block, camera.PhysicsPosition)) {
			BounceOn(block, camera);
		}
	}

	if (CollidesWith(m_FinishBox, camera.PhysicsPosition))
		ImGui::Text("Finish!!!");
}

void Level::SetupMatricesInstanceVBO()
{
	m_Matrices.reserve(m_Blocks.size());

	for (Block& block : m_Blocks) {
		glm::vec3 start = block.Start;
		block.Start = glm::min(start, block.End);
		block.End = glm::max(start, block.End);

		// Extra factors are for help with that boxVertices are -1 to 1 instead of 0-1
		glm::mat4 matrix = glm::translate(glm::mat4(1.0f), 0.5f * (block.Start + block.End));
		matrix = glm::scale(matrix, (block.End - block.Start) * 0.5f);

		m_Matrices.push_back(matrix);
	}

	static const BufferLayout instancedBufferLayout{ {{GL_FLOAT, 4}, {GL_FLOAT, 4}, {GL_FLOAT, 4}, {GL_FLOAT, 4}}, 5 };

	m_InstanceVBO = VertexBuffer(m_Matrices.size() * sizeof(m_Matrices[0]), m_Matrices.data());

	if (!Block::Object.IsValid())
		Block::Object = Object<SimpleVertex>(m_Material, boxVertices, SimpleVertex::Layout);

	Block::Object.SetInstanceBuffer(m_InstanceVBO);
}
