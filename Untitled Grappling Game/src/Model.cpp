#include "pch.h"
#include "Log.h"

#include "Model.h"
#include "Timer.h"

#pragma warning(push, 0)
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/material.inl>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

#include <DebugBreak.h>

// helper functions:
#if QUICK_LOADING
Model::Model(const std::string& path, bool loadTextures, bool deleteAfterLoading) : m_EnableTextures(loadTextures), m_DeleteAfterLoading(deleteAfterLoading) {
#if QUICK_LOADING
	m_Loading = std::async(std::launch::async, [this, path]() { LoadModel(path); });
#else
	LoadModel(path);
#endif
}

bool Model::isFinished() {
	using namespace std::literals::chrono_literals;
	// the way this is going to be used is like this:
	// if (isFinished()) DoOpenGL(),
	// so if it's already done that, just don't call the function, it's wastefull
	if (m_DoneOpenGL)
		return false;

	return m_Loading.wait_for(0s) == std::future_status::ready;
}

std::vector<std::shared_future<LoadingTexture*>> Model::LoadTexturesFromType(const aiMaterial* material, aiTextureType type, TextureType typeName) {
	std::vector<std::shared_future<LoadingTexture*>> futures;

	int tCount = material->GetTextureCount(type);

	if (!tCount)
		return futures;

	for (int i = 0; i < tCount; i++) {
		aiString str;
		material->GetTexture(type, i, &str);
		bool skip = false;

		// str gets deleted after this, so we need to copy the path:
		char* const fname = new char[str.length + 1];
		memcpy(fname, str.C_Str(), (size_t)str.length + 1);

		if (!skip) {
			futures.push_back(StartLoadingTexture(m_Directory + fname, typeName));
		}
	}

	return futures;
}
#else
#include <stb_image.h>

static unsigned int TextureFromFile(const char* path, const std::string& directory) {
	std::string filename = directory + '/' + std::string(path);
	// Timer t("Loading " + filename);

	unsigned int textureID;

	int width, height, nrChannels;
	width = height = nrChannels = 512;

	// Timer stbi("stbi_load " + filename);
	unsigned char* p_data = stbi_load(filename.c_str(), &width, &height, &nrChannels, 0);
	// stbi.~Timer();

	if (!p_data)
		throw "Texture::> failed to open/load texture: " + filename;

	GLenum format;
	if (nrChannels == 1)
		format = GL_RED;
	else if (nrChannels == 3)
		format = GL_RGB;
	else if (nrChannels == 4)
		format = GL_RGBA;
	else
		throw "Texture in file " + filename + " is weird: has " + std::to_string(nrChannels) + " amound of channels, instead of the normal 1, 3, or 4";

	// Timer gl("openGL load" + filename);
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, p_data);
	glGenerateMipmap(GL_TEXTURE_2D);

	// set the texture wrapping/filtering options (on the currently bound texture object)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// gl.~Timer();

	// Timer f("stbi freeing " + filename);
	stbi_image_free(p_data);

	return textureID;
}

std::vector<Texture> Model::LoadTexturesFromType(aiMaterial* mat, aiTextureType type, std::string typeName)
{
	std::vector<Texture> m_Textures;

	int tCount = mat->GetTextureCount(type);

	if (!tCount)
		return m_Textures;

	m_Textures.reserve(tCount);

	for (int i = 0; i < tCount; i++) {
		aiString str;
		mat->GetTexture(type, i, &str);
		bool skip = false;
		for (unsigned int j = 0; j < loaded_textures.size(); j++) {
			if (std::strcmp(loaded_textures[j].path.data(), str.C_Str()) == 0) {
				m_Textures.push_back(loaded_textures[j]);
				skip = true;
				break;
			}
		}

		if (!skip) {
			Texture t;
			t.path = str.C_Str();
			t.id = TextureFromFile(str.C_Str(), directory);
			t.type = typeName;
			m_Textures.push_back(t);
			loaded_textures.push_back(t);
		}
	}

	return std::move(m_Textures);
}
#endif

Model& Model::operator=(Model&& model) noexcept
{
#if QUICK_LOADING
	m_Loading = std::move(model.m_Loading);
#endif

	m_MultiMesh = std::move(model.m_MultiMesh);
	m_Directory = std::move(model.m_Directory);
	m_EnableTextures = model.m_EnableTextures;

	return *this;
}

void Model::Draw(Shader& shader, const Frustum& camFrustum, const Transform& transform, bool setTextures) {
	if (!m_DoneOpenGL)
		return;

	shader.Use();

	m_MultiMesh.Draw(shader, camFrustum, transform, setTextures);
}

void Model::DrawInstanced(Shader& shader, unsigned int count, const Frustum& camFrustum, const Transform& transform, bool setTextures) {
	if (!m_DoneOpenGL)
		return;

	shader.Use();

	m_MultiMesh.DrawInstanced(shader, count, camFrustum, transform, setTextures);
}

void Model::DoOpenGL() {
	if (m_DoneOpenGL)
		return;

	m_DoneOpenGL = true;

#if QUICK_LOADING
	m_Loading.get();
#endif

	m_MultiMesh.DoOpenGL(m_DeleteAfterLoading);
}

void Model::LoadModel(const std::string& path)
{
	Timer t("Loading model of " + path);
	Assimp::Importer importer;
	importer.SetPropertyInteger(AI_CONFIG_FAVOUR_SPEED, 1);
	importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_LINE | aiPrimitiveType_POINT);
	importer.SetPropertyBool(AI_CONFIG_FBX_CONVERT_TO_M, true);

	unsigned int pFlags = 0;
	pFlags |= aiProcess_SortByPType | aiProcess_FlipUVs | aiProcess_PreTransformVertices | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenUVCoords; // Needed for rendering
	pFlags |= aiProcess_GenBoundingBoxes; // let assimp make the AABB automatically
	// aiProcess_OptimizeGraph is incompatible with aiProcess_PreTransformVertices
	pFlags |= aiProcess_JoinIdenticalVertices | aiProcess_ImproveCacheLocality | aiProcess_OptimizeMeshes | aiProcess_JoinIdenticalVertices | aiProcess_RemoveRedundantMaterials; // Optimisations to do now while on a different thread
#if _DEBUG
	pFlags |= aiProcess_ValidateDataStructure;
#endif

	const aiScene* scene = importer.ReadFile(path, pFlags);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
		NG_ERROR("Assimp error: {} for path {}", importer.GetErrorString(), path);
		return;
	}
	m_Directory = path.substr(0, path.find_last_of('/') + 1); // Want to include the '/'

	ProcessNode(scene->mRootNode, scene);
}

void Model::ProcessNode(aiNode* node, const aiScene* scene)
{
	for (unsigned int i = 0; i < node->mNumMeshes; i++) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

		ProcessMesh(mesh, scene);
	}

	for (unsigned int i = 0; i < node->mNumChildren; i++)
		ProcessNode(node->mChildren[i], scene);
}

void Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices;
	vertices.reserve(mesh->mNumVertices);
	std::vector<unsigned int> indices;

	aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

#if QUICK_LOADING
	// start futures: they can do their thing while the vertices and indices are being processed
	std::vector<std::shared_future<LoadingTexture*>> m_Textures;
	std::vector<std::shared_future<LoadingTexture*>> maps; // just meant as an transport vehicle
#else
	std::vector<Texture> m_Textures;
	std::vector<Texture> maps; // just meant as an transport vehicle
#endif
	if (m_EnableTextures)
	{
		// process the textures
		// 1. diffuse maps
		if (material->GetTextureCount(aiTextureType_DIFFUSE)) {
			maps = LoadTexturesFromType(material, aiTextureType_DIFFUSE, TextureType::diffuse);
			m_Textures.insert(m_Textures.end(), maps.begin(), maps.end());
		}
		// 2. specular maps
		if (material->GetTextureCount(aiTextureType_SPECULAR)) {
			maps = LoadTexturesFromType(material, aiTextureType_SPECULAR, TextureType::specular);
			m_Textures.insert(m_Textures.end(), maps.begin(), maps.end());
		}
		// 3. normal maps
		if (material->GetTextureCount(aiTextureType_HEIGHT)) {
			maps = LoadTexturesFromType(material, aiTextureType_HEIGHT, TextureType::normal);
			m_Textures.insert(m_Textures.end(), maps.begin(), maps.end());
		}
		// 4. height maps
		if (material->GetTextureCount(aiTextureType_AMBIENT)) {
			maps = LoadTexturesFromType(material, aiTextureType_AMBIENT, TextureType::height);
			m_Textures.insert(m_Textures.end(), maps.begin(), maps.end());
		}
	}

	// process vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
		Vertex v{};
		const auto& mVert = mesh->mVertices[i]; // essentially makes it a list
		v.Position = glm::vec3(mVert.x, mVert.y, mVert.z);

		if (mesh->HasNormals()) {
			const auto& meshNormals = mesh->mNormals[i];
			v.Normal = glm::vec3(meshNormals.x, meshNormals.y, meshNormals.z);
		}

		if (m_EnableTextures && bool(mesh->mTextureCoords[0])) {
			const auto& meshTexCoords = mesh->mTextureCoords[0][i];
			v.TexCoords = glm::vec2(meshTexCoords.x, meshTexCoords.y);

			if (mesh->mTangents)
			{
				const auto& mesh_tangent = mesh->mTangents[i];
				v.Tangent = glm::vec3(mesh_tangent.x, mesh_tangent.y, mesh_tangent.z);
			}

			if (mesh->mBitangents)
			{
				const auto& mesh_bitangent = mesh->mBitangents[i];
				v.Bitangent = glm::vec3(mesh_bitangent.x, mesh_bitangent.y, mesh_bitangent.z);
			}
		}
		else
			v.TexCoords = glm::vec2(0.0f, 0.0f);

		vertices.push_back(v);
	}

	// process indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
		aiFace face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	// process materials

	// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
	// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER.
	// Same applies to other texture as the following list summarizes:
	// diffuse:  diffuseN
	// specular: specularN
	// normal:   normalN
	// height:   heightN

	aiColor3D diff, spec;
	material->Get(AI_MATKEY_COLOR_DIFFUSE, diff);
	material->Get(AI_MATKEY_COLOR_SPECULAR, spec);

	glm::vec3 diffuse(diff[0], diff[1], diff[2]), specular(spec[0], spec[1], spec[2]);
	std::scoped_lock lg{ m_MeshesMtx };

	m_MultiMesh.Add(vertices, mesh->mAABB, indices, std::move(m_Textures), diffuse, specular, m_EnableTextures && (m_Textures.size() > 0));
	return;
}