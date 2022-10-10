#pragma once
#include "DataBuffers.h"
#include "Texture.h"
#include "Material.h"
#include "Vertex.h"

template<typename T = Vertex>
struct Object
{
private:
	std::vector<T> m_Vertices;
	std::vector<GLuint> m_Indices;

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
	Object(Material* material, const std::array<T, VBOSize>& VBOData, const BufferLayout& bufferLayout, const std::array<GLuint, IBOSize>& IBOData = std::array<GLuint, 0>())
		: m_Vertices{ VBOData.begin(), VBOData.end() }, m_Indices{ IBOData.begin(), IBOData.end() }, m_Material(material), m_UseIBO(!IBOData.empty()), m_VBO(m_Vertices.size() * sizeof(T), m_Vertices.data())
	{
		VAO.AddBuffer(m_VBO, bufferLayout);

		if (m_UseIBO) {
			m_NumVerts = IBOData.size();
			m_IBO = IndexBuffer(IBOData.size() * sizeof(unsigned int), IBOData.data());
			VAO.AddIndexBuffer(m_IBO);
		}
		else
			m_NumVerts = VBOData.size();
	}

	Object(Material* material, std::vector<T>&& vertices, const BufferLayout& bufferLayout, std::vector<GLuint>&& indices);

	Object(Material* material, std::vector<T>&& vertices, const BufferLayout& bufferLayout);

	virtual void Draw(bool setTextures);

	virtual void Draw(Shader& shader, bool setTextures);

	virtual void DrawInstanced(Shader& shader, bool setTextures, unsigned int count);

	virtual void SetInstanceBuffer(const VertexBuffer& instanceBuffer);

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
Object<T>::Object(Material* material, std::vector<T>&& vertices, const BufferLayout& bufferLayout)
	: m_Material(material), m_NumVerts(m_Vertices.size()), m_Vertices(std::move(vertices)),
	m_VBO(m_Vertices.size() * sizeof(T), m_Vertices.data())
{
	VAO.AddBuffer(m_VBO, bufferLayout);
}

template<typename T>
inline void Object<T>::Draw(bool setTextures)
{
	if (!m_NumVerts)
		return;

	m_Material->Load(setTextures);

	VAO.Bind();
	if (m_UseIBO)
		glDrawElements(GL_TRIANGLES, m_NumVerts, GL_UNSIGNED_INT, (void*)0);
	else
		glDrawArrays(GL_TRIANGLES, 0, m_NumVerts);

	VAO.UnBind();

	glActiveTexture(GL_TEXTURE0);
}

template<typename T>
Object<T>::Object(Material* material, std::vector<T>&& vertices, const BufferLayout& bufferLayout, std::vector<GLuint>&& indices)
	: m_Material(material), m_Vertices(std::move(vertices)), m_Indices(std::move(indices)), m_UseIBO(true),
	m_VBO(m_Vertices.size() * sizeof(T), m_Vertices.data()),
	m_IBO(m_Indices.size() * sizeof(GLuint), m_Indices.data()),
	m_NumVerts(m_Indices.size())
{
	VAO.AddBuffer(m_VBO, bufferLayout);
	VAO.AddIndexBuffer(m_IBO);
}

template<typename T>
void Object<T>::Draw(Shader& shader, bool setTextures) {
	if (!m_NumVerts)
		return;

	m_Material->Load(shader, setTextures);

	VAO.Bind();
	if (m_UseIBO)
		glDrawElements(GL_TRIANGLES, m_NumVerts, GL_UNSIGNED_INT, (void*)0);
	else
		glDrawArrays(GL_TRIANGLES, 0, m_NumVerts);

	VAO.UnBind();

	glActiveTexture(GL_TEXTURE0);
}

template<typename T>
void Object<T>::DrawInstanced(Shader& shader, bool setTextures, unsigned int count)
{
	if (!m_NumVerts)
		return;

	m_Material->Load(shader, setTextures);

	VAO.Bind();
	if (m_UseIBO)
		glDrawElementsInstanced(GL_TRIANGLES, m_NumVerts, GL_UNSIGNED_INT, (void*)0, count);
	else
		glDrawArraysInstanced(GL_TRIANGLES, 0, m_NumVerts, count);

	VAO.UnBind();

	glActiveTexture(GL_TEXTURE0);
}

template<typename T>
void Object<T>::SetInstanceBuffer(const VertexBuffer& instanceBuffer) {
	VAO.AddBuffer(instanceBuffer, instanceBufferLayout);

	glVertexAttribDivisor(5, 1);
	glVertexAttribDivisor(6, 1);
	glVertexAttribDivisor(7, 1);
	glVertexAttribDivisor(8, 1);
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