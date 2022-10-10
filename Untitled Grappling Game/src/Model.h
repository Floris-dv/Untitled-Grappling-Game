#pragma once
#include "pch.h"

#pragma warning(push, 0)
#include <assimp/scene.h>
#pragma warning(pop)

#include "MultiMesh.h"
#include "Shader.h"
#include "Settings.h"
#include "Transform.h"

class Model
{
private:

	bool m_DeleteAfterLoading;

	bool m_EnableTextures;

	MultiMesh m_MultiMesh;

	Transform m_Transform;

public:
	Model(const std::string& path, bool loadTextures = true, bool deleteAfterLoading = true);

	Model& operator=(Model&& model) noexcept;

	Model() noexcept : m_EnableTextures(false), m_DeleteAfterLoading(false) {}

	void Draw(Shader& shader, const Frustum& camFrustum, const Transform& transform, bool setTextures = true);

	void DrawInstanced(Shader& shader, unsigned int count, const Frustum& camFrustum, const Transform& transform, bool setTextures = true);

	MultiMesh& getMesh() { return m_MultiMesh; }

	// DO ONLY ON MAIN THREAD
	void DoOpenGL();

#if QUICK_LOADING
	bool isFinished();
#endif

private:
#if QUICK_LOADING
	// for the parallel loading of the model itself:
	std::future<void> m_Loading;

	// for the parallel loading of the textures
	std::mutex m_MeshesMtx;
	std::vector<std::shared_future<LoadingTexture*>> LoadTexturesFromType(const aiMaterial* material, aiTextureType type, TextureType typeName);
#else
	std::vector<Texture> LoadTexturesFromType(aiMaterial* material, aiTextureType type, std::string typeName);
	std::vector<Texture> Loaded_textures;
#endif
	std::string m_Directory;

	void LoadModel(const std::string& path);
	void ProcessNode(aiNode* node, const aiScene* scene);
	void ProcessMesh(aiMesh* mesh, const aiScene* scene);

	bool m_DoneOpenGL = false;
};
