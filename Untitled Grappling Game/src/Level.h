#pragma once
#include "Material.h"
#include "DataBuffers.h"
#include "Object.h"
#include "GrapplingCamera.h"

/* TODO
* Structure that I want:
*  - Level: base class (blocks, finishBox)
*  - PlayingLevel: Purely playing (creates the matrices, then deletes them), instanced rendering. Can read
*  - EditingLevel: Normal rendering. Can read/write
*/

#define VERSION(major, minor, build) (((major) << 22) | ((minor) << 12) | (build))

class Level
{
public:
	struct Block {
		glm::vec3 Start;
		glm::vec3 End;
		glm::quat Rotation = { 1.0f, 0.0f, 0.0f, 0.0f };

		inline static Object<SimpleVertex> Object;
	};

public:
	Level(const glm::vec3& startPlatformSize, const Block& finishBox, std::vector<Block>&& blocks, std::unique_ptr<Material>&& instancedMaterial, std::unique_ptr<Material>&& normalMaterial, std::unique_ptr<Material>&& finishMaterial);

	Level(const std::string& levelFile, std::shared_ptr<Shader> instancedShader, std::shared_ptr<Shader> normalShader);

	void Write(std::string_view levelFile);

	// Assumes the MVP-matrix has been set
	void Render() { Render(m_InstancedMaterial.get(), m_FinishMaterial.get()); }

	void Render(Material* material, Material* finishMaterial);

	// Doesn't use instanced rendering, instead uses normal rendering, used for editing mode
	void RenderOneByOne();

	void RenderEditingMode(size_t& index);

	const std::vector<Block>& GetBlocks() { return m_Blocks; }

	// Returns: if the level is finished
	bool UpdatePhysics(GrapplingCamera& camera);

	// Needs to be called when switching from RenderOneByOne() to Render()
	void UpdateInstanceVBO();

	double GetStartTime() { return m_StartTime; }

private:
	// First block: finish box, Second block (by convention): startplatform
	std::vector<Block> m_Blocks;
	std::vector<glm::mat4> m_Matrices;

	std::unique_ptr<Material> m_InstancedMaterial;
	std::unique_ptr<Material> m_Material;
	std::unique_ptr<Material> m_FinishMaterial;

	VertexBuffer m_InstanceVBO;

	double m_StartTime = 0.0;

	// TODO: add a level name here

	std::string m_FileName;

	bool m_AllowEditing;

	static constexpr uint32_t VERSION_NR = VERSION(0, 0, 1);

	// Uses m_Matrices to setup m_InstanceVBO and Block::Object
	void SetupÍnstanceVBO();

	// Uses m_Blocks to setup m_Matrices
	void SetupMatrices();
};

