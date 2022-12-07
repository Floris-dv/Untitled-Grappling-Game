#pragma once
#include "DataBuffers.h"
#include "Texture.h"
#include "Material.h"
#include "Vertex.h"

template<typename T = Vertex>
struct Object
{
private:
	unsigned int m_NumVerts;
	bool m_UseIBO = false;

	VertexBuffer m_VBO;

	IndexBuffer m_IBO;

	Material* m_Material = nullptr;

public:
	VertexArray VAO;

	Object() noexcept : m_NumVerts(0), VAO(false) {}

	Object<T>& operator=(Object<T>&& other) noexcept;

	Object<T>& operator=(const Object& other) = delete;

	template<std::size_t VBOSize, size_t IBOSize = 0>
	Object(Material* material, const std::array<T, VBOSize>& vertices, const BufferLayout& bufferLayout, const std::array<GLuint, IBOSize>& indices = std::array<GLuint, 0>())
		: m_Material(material), m_UseIBO(!indices.empty()), m_VBO(vertices.size() * sizeof(T), vertices.data())
	{
		VAO.AddBuffer(m_VBO, bufferLayout);

		if (m_UseIBO) {
			m_NumVerts = indices.size();
			m_IBO = IndexBuffer(m_NumVerts * sizeof(unsigned int), indices.data());
			VAO.AddIndexBuffer(m_IBO);
		}
		else
			m_NumVerts = vertices.size();
	}

	Object(Material* material, const std::vector<T>& vertices, const BufferLayout& bufferLayout, const std::vector<GLuint>& indices);

	Object(Material* material, const std::vector<T>& vertices, const BufferLayout& bufferLayout);

	void Draw(const glm::mat4& modelMatrix, bool setMaterial) { Draw(m_Material, modelMatrix, setMaterial); }

	virtual void Draw(Material* material, const glm::mat4& modelMatrix, bool setMaterial);

	void DrawInstanced(bool setMaterial, unsigned int count) { DrawInstanced(m_Material, setMaterial, count); }

	virtual void DrawInstanced(Material* material, bool setMaterial, unsigned int count);

	virtual void SetInstanceBuffer(const VertexBuffer& instanceBuffer);

	inline bool IsValid() { return VAO.IsValid(); }

	void DoOpenGL(bool deleteAfter) { m_Material->LoadTextures(deleteAfter); }
};


static const BufferLayout instanceBufferLayout = {
	5,
	sizeof(glm::mat4),
	{
		{GL_FLOAT, 4},
		{GL_FLOAT, 4},
		{GL_FLOAT, 4},
		{GL_FLOAT, 4}
	},
	true
};

template<typename T>
Object<T>::Object(Material* material, const std::vector<T>& vertices, const BufferLayout& bufferLayout)
	: m_Material(material), m_NumVerts(vertices.size()), m_VBO(vertices.size() * sizeof(T), vertices.data())
{
	VAO.AddBuffer(m_VBO, bufferLayout);
}

template<typename T>
Object<T>::Object(Material* material, const std::vector<T>& vertices, const BufferLayout& bufferLayout, const std::vector<GLuint>& indices)
	: m_Material(material), m_UseIBO(true),
	m_VBO(vertices.size() * sizeof(T), vertices.data()),
	m_IBO(indices.size() * sizeof(GLuint), indices.data()),
	m_NumVerts(indices.size())
{
	VAO.AddBuffer(m_VBO, bufferLayout);
	VAO.AddIndexBuffer(m_IBO);
}

template<typename T>
void Object<T>::Draw(Material* material, const glm::mat4& modelMatrix, bool setMaterial) {
	if (!m_NumVerts)
		return;

	LoadMaterial(material, setMaterial);

	material->GetShader()->SetMat4("model", modelMatrix);

	VAO.Bind();
	if (m_UseIBO)
		glDrawElements(GL_TRIANGLES, m_NumVerts, GL_UNSIGNED_INT, (void*)0);
	else
		glDrawArrays(GL_TRIANGLES, 0, m_NumVerts);

	VAO.UnBind();
}

template<typename T>
void Object<T>::DrawInstanced(Material* material, bool setMaterial, unsigned int count)
{
	if (!m_NumVerts)
		return;

	LoadMaterial(material, setMaterial);

	VAO.Bind();
	if (m_UseIBO)
		glDrawElementsInstanced(GL_TRIANGLES, m_NumVerts, GL_UNSIGNED_INT, (void*)0, count);
	else
		glDrawArraysInstanced(GL_TRIANGLES, 0, m_NumVerts, count);

	VAO.UnBind();
}

template<typename T>
void Object<T>::SetInstanceBuffer(const VertexBuffer& instanceBuffer) {
	VAO.AddBuffer(instanceBuffer, instanceBufferLayout);
}

template<typename T>
Object<T>& Object<T>::operator=(Object<T>&& other) noexcept {
	m_VBO = std::move(other.m_VBO);
	m_IBO = std::move(other.m_IBO);
	VAO = std::move(other.VAO);

	std::swap(m_Material, other.m_Material);

	std::swap(m_NumVerts, other.m_NumVerts);
	std::swap(m_UseIBO, other.m_UseIBO);

	return *this;
}