#include "pch.h"
#include "MultiMesh.h"
#include "Camera.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "AABB.h"

#include "Transform.h"

static const BufferLayout vertexBufferLayout = {
	0,
	sizeof(Vertex),
	{
		{GL_FLOAT, 3}, // position
		{GL_FLOAT, 3}, // normal
		{GL_FLOAT, 2}, // texCoords
		{GL_FLOAT, 3}, // tangent
		{GL_FLOAT, 3}, // bitangent
	},
	false
};

static bool test_AABB_against_frustum(const glm::mat4& MVP, const aiAABB& boundingBox) { return true; }

void MultiMesh::Add(const std::vector<Vertex>& verts, const AABB& boundingBox, const std::vector<unsigned int>& idxs, std::vector<Texture>&& textures, const glm::vec3& diff, const glm::vec3& spec, bool useTextures) {
	SubMesh sm;
	sm.DiffColor = diff;
	sm.SpecColor = spec;
	sm.UseTextures = useTextures;
	sm.Parent = this;
	sm.Textures = std::move(textures);
	sm.IndexCount = idxs.size();
	sm.IndexOffset = m_Indices.size();
	sm.VertexCount = verts.size();
	sm.VertexOffset = m_Vertices.size();
	sm.BoundingBox = boundingBox;
	m_Meshes.push_back(std::move(sm));

	size_t s = m_Vertices.size();
	m_Vertices.insert(m_Vertices.end(), verts.begin(), verts.end());

	m_Indices.reserve(m_Indices.size() + idxs.size());
	for (int i = 0; i < idxs.size(); i++) {
		m_Indices.push_back(idxs[i] + s);
	}
}

void MultiMesh::Add(const std::vector<Vertex>& verts, const AABB& boundingBox, const std::vector<unsigned int>& idxs, std::vector<std::shared_future<LoadingTexture*>>&& textures, const glm::vec3& diff, const glm::vec3& spec, bool useTexs) {
	SubMesh sm;
	sm.DiffColor = diff;
	sm.SpecColor = spec;
	sm.UseTextures = useTexs;
	sm.Parent = this;
	sm.LoadingTextures = std::move(textures);
	sm.IndexCount = idxs.size();
	sm.IndexOffset = m_Indices.size();
	sm.VertexCount = verts.size();
	sm.VertexOffset = m_Vertices.size();
	sm.BoundingBox = boundingBox;
	m_Meshes.push_back(std::move(sm));

	m_Vertices.insert(m_Vertices.end(), verts.begin(), verts.end());

	m_Indices.reserve(m_Indices.size() + idxs.size());

	// correct indices, as they are calculated with the new vertices in mind
	size_t s = m_Vertices.size() - verts.size();
	for (int i = 0; i < idxs.size(); i++) {
		m_Indices.push_back(idxs[i] + s);
	}
}

void MultiMesh::Draw(Shader& shader, const Frustum& camFrustum, const Transform& transform, bool setTextures) const
{
	if (!m_OpenGLPrepared)
		return;

	VAO.Bind();
	for (int i = 0; i < m_Meshes.size(); i++) {
		if (m_Meshes[i].BoundingBox.isOnFrustum(camFrustum, transform))
			m_Meshes[i].Render(shader, setTextures);
	}

	VAO.UnBind();

	glActiveTexture(GL_TEXTURE0);
}

void MultiMesh::DrawInstanced(Shader& shader, unsigned int count, const Frustum& camFrustum, const Transform& transform, bool setTextures) const
{
	if (!m_OpenGLPrepared)
		return;

	for (auto& m : m_Meshes)
		m.Render(shader, setTextures, count);

	VAO.UnBind();

	glActiveTexture(GL_TEXTURE0);
}

void MultiMesh::DoOpenGL(bool deleteAfter) noexcept
{
	if (m_OpenGLPrepared)
		return;

	m_OpenGLPrepared = true;

	// create buffers/arrays
	VAO = VertexArray();
	m_VBO = VertexBuffer((unsigned int)m_Vertices.size() * sizeof(Vertex), m_Vertices.data());
	m_IBO = IndexBuffer((unsigned int)m_Indices.size() * sizeof(unsigned int), m_Indices.data());

	VAO.AddBuffer(m_VBO, vertexBufferLayout);
	VAO.AddIndexBuffer(m_IBO);

	for (auto& m : m_Meshes)
		m.InitTextures(deleteAfter);

	if (deleteAfter) {
		m_Vertices.clear();
		m_Indices.clear();
	}
}

MultiMesh::MultiMesh(MultiMesh&& other) noexcept : VAO(false)
{
	m_VBO = std::move(other.m_VBO);
	VAO = std::move(other.VAO);
	m_IBO = std::move(other.m_IBO);

	m_Meshes = std::move(other.m_Meshes);

	m_OpenGLPrepared = other.m_OpenGLPrepared;

	m_Vertices = std::move(other.m_Vertices);
	m_Indices = std::move(other.m_Indices);
}

MultiMesh& MultiMesh::operator=(MultiMesh&& other) noexcept
{
	m_VBO = std::move(other.m_VBO);
	VAO = std::move(other.VAO);
	m_IBO = std::move(other.m_IBO);

	m_Meshes = std::move(other.m_Meshes);

	m_OpenGLPrepared = other.m_OpenGLPrepared;

	m_Vertices = std::move(other.m_Vertices);
	m_Indices = std::move(other.m_Indices);

	return *this;
}

void MultiMesh::SubMesh::Render(Shader& shader, bool setTextures) const
{
	if (IndexCount == 0 || VertexCount == 0)
		return;

	if (setTextures)
		SetTextures(shader);

	shader.SetBool("material.useTex", setTextures && UseTextures);

	Parent->VAO.Bind();
	glDrawElements(GL_TRIANGLES, IndexCount, GL_UNSIGNED_INT, (void*)(IndexOffset * sizeof(unsigned int)));
}

void MultiMesh::SubMesh::Render(Shader& shader, bool setTextures, unsigned int count) const
{
	if (IndexCount == 0 || VertexCount == 0)
		return;

	if (setTextures)
		SetTextures(shader);

	shader.SetBool("material.useTex", setTextures && UseTextures);

	Parent->VAO.Bind();
	glDrawElementsInstanced(GL_TRIANGLES, IndexCount, GL_UNSIGNED_INT, (void*)(IndexOffset * sizeof(unsigned int)), count);
}

void MultiMesh::SubMesh::InitTextures(bool deleteAfter)
{
	if (LoadingTextures.empty())
		return;

	for (auto& lt : LoadingTextures) {
		auto& s = lt.get();
		Textures.push_back(s->Finish());
		if (deleteAfter)
			delete s;
	}
	LoadingTextures.clear();
}

void MultiMesh::SubMesh::SetTextures(Shader& shader) const
{
	unsigned int diffuseNr = 0, specularNr = 0, normalNr = 0, heightNr = 0;

	if (UseTextures) {
		for (int i = 0; i < Textures.size(); i++) {
			std::string number;
			TextureType type = Textures[i].Type;

			switch (type) {
			case TextureType::diffuse:
				number = std::to_string(diffuseNr++);
				break;
			case TextureType::specular:
				number = std::to_string(specularNr++); // transfer unsigned int to stream
				break;
			case TextureType::normal:
				number = std::to_string(normalNr++); // transfer unsigned int to stream
				break;
			case TextureType::height:
				number = std::to_string(heightNr++); // transfer unsigned int to stream
				break;
			default:
				throw "ERROR: Texture type is not defined\n";
			}

			glActiveTexture(GL_TEXTURE0 + i);
			shader.SetInt((names[(int)type] + number).c_str(), i);
			glBindTexture(GL_TEXTURE_2D, Textures[i].ID);
		}
	}
	else {
		shader.SetVec3("material.diff0", DiffColor);
		shader.SetVec3("material.spec0", SpecColor);
	}
}