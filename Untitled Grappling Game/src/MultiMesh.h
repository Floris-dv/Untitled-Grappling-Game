#pragma once
#include "Shader.h"
#include "Texture.h"
#include "Settings.h"

#include "DataBuffers.h"

#include <glm/glm.hpp>

#include "Vertex.h"

#include "AABB.h"

class MultiMesh {
private:
	struct SubMesh {
		MultiMesh* Parent = nullptr;
		glm::vec3 DiffColor{ 0.0f };
		glm::vec3 SpecColor{ 0.0f };
		std::vector<std::shared_future<LoadingTexture*>> LoadingTextures;
		std::vector<Texture> Textures;
		AABB BoundingBox;

		bool UseTextures = false;
		unsigned int IndexOffset = 0;
		unsigned int IndexCount = 0;
		unsigned int VertexOffset = 0;
		unsigned int VertexCount = 0;

		void Render(Shader& shader, bool setTextures) const;
		void Render(Shader& shader, bool setTextures, unsigned int count) const;

		// TODO: combine different textures into one texture atlas
		void InitTextures(bool deleteAfter);

		void SetTextures(Shader& shader) const;
	};

	std::vector<Vertex> m_Vertices;

	std::vector<unsigned int> m_Indices;

	std::vector<SubMesh> m_Meshes;

	VertexBuffer m_VBO;
	IndexBuffer m_IBO;

	bool m_OpenGLPrepared = false;

	//VertexBuffer instanceVBO;

public:
	VertexArray VAO;

	MultiMesh() : VAO(false) {}

	MultiMesh(const MultiMesh& MultiMesh) = delete;

	MultiMesh(MultiMesh&& other) noexcept;

	MultiMesh& operator=(MultiMesh&& other) noexcept;

	void Add(const std::vector<Vertex>& verts, const AABB& boundingBox, const std::vector<unsigned int>& idxs, std::vector<Texture>&& m_Textures, const glm::vec3& diff = glm::vec3(0.0f), const glm::vec3& spec = glm::vec3(0.0f), bool m_UseTextures = true);
	void Add(const std::vector<Vertex>& verts, const AABB& boundingBox, const std::vector<unsigned int>& idxs, std::vector<std::shared_future<LoadingTexture*>>&& m_Textures, const glm::vec3& diff = glm::vec3(0.0f), const glm::vec3& spec = glm::vec3(0.0f), bool m_UseTextures = true);

	// TODO: make these frustums take from the global camera
	void Draw(Shader& shader, const Frustum& camFrustum, const Transform& transform, bool setTextures) const;

	// For instanced arrays: location = 3;
	void DrawInstanced(Shader& shader, unsigned int count, const Frustum& camFrustum, const Transform& transform, bool setTextures) const;

	void DoOpenGL(bool deleteAfter) noexcept;
};