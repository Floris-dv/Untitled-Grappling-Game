#pragma once
#include <glad/glad.h>
#include "Shader.h"
#include <glm/glm.hpp>

class VertexBuffer
{
private:
	GLuint m_ID;

public:
	VertexBuffer() : m_ID(0) {}
	VertexBuffer(unsigned int size, const void* data);
	~VertexBuffer() {
		if (m_ID)
			glDeleteBuffers(1, &m_ID);
	}

	VertexBuffer& operator=(VertexBuffer&& other) noexcept {
		std::swap(m_ID, other.m_ID);
		return *this;
	}

	VertexBuffer& operator=(const VertexBuffer& other) = delete;

	void Bind() const;

	void UnBind() const;

	bool IsValid() const { return m_ID != 0; }

	GLuint ID() const { return m_ID; }
};

class IndexBuffer
{
private:
	GLuint m_ID;

public:
	IndexBuffer() : m_ID(0) {}

	IndexBuffer(size_t size, const void* data);
	~IndexBuffer() {
		if (m_ID)
			glDeleteBuffers(1, &m_ID);
	}

	IndexBuffer& operator=(IndexBuffer&& other) noexcept {
		std::swap(m_ID, other.m_ID);
		return *this;
	}

	IndexBuffer& operator=(const IndexBuffer& other) = delete;

	void Bind() const;

	void UnBind() const;

	bool IsValid() const { return m_ID != 0; }

	GLuint ID() const { return m_ID; }
};

class UniformBuffer {
private:
	static inline GLuint s_MaxBlock = 0;

	GLuint m_ID;
	GLuint m_BlockID;
	std::string m_Name;

public:
	UniformBuffer(size_t size, const std::string& name, const void* data = nullptr);
	~UniformBuffer() { glDeleteBuffers(1, &m_ID); }

	void SetData(size_t offset, size_t size, const void* data);
	void SetBlock(Shader& shader);

	void Bind() const;

	void UnBind() const;
};


struct LayoutElement {
	unsigned int Type;
	unsigned int Count;

	bool Normalized = true;

	constexpr unsigned int GetSize() const {
		switch (Type) {
		case GL_FLOAT:
			return sizeof(GLfloat) * Count;
		case GL_UNSIGNED_INT:
			return sizeof(GLuint) * Count;
		case GL_UNSIGNED_BYTE:
			return sizeof(GLubyte) * Count;
		default:
			return -1;
		}
	}
};

class BufferLayout
{
private:
	unsigned int m_StartIndex;

	unsigned int m_Stride = 0;
	std::vector<LayoutElement> m_Elements;

	bool m_IsInstanced;

	// to work with g++: otherwise an error is raised: explicit specialization in non namespace
	template <typename T>
	struct identity { typedef T type; };

	template<typename T>
	constexpr void Push(unsigned int count, identity<T>) { assert(false); }

	constexpr void Push(unsigned int count, identity<float>) {
		m_Elements.push_back({ GL_FLOAT, count });
		m_Stride += count * sizeof(GLfloat);
	}

	constexpr void Push(unsigned int count, identity<glm::mat4>) {
		for (unsigned int i = 0; i < count * 4; i++) {
			Push<float>(4);
		}
	}

	constexpr void Push(unsigned int count, identity<unsigned int>) {
		m_Elements.push_back({ GL_UNSIGNED_INT, count });
		m_Stride += count * sizeof(GLuint);
	}

public:
	template<typename T> // currently implemented: float, glm::mat4, and unsigned int
	constexpr void Push(unsigned int count) { Push(count, identity<T>()); }

	constexpr BufferLayout(const std::vector<LayoutElement>& elements, unsigned int startIndex = 0, bool instanced = false) : m_Elements(elements), m_StartIndex(startIndex), m_IsInstanced(instanced) {
		for (const LayoutElement& le : elements)
			m_Stride += le.GetSize();
	}

	constexpr BufferLayout(std::vector<LayoutElement>&& elements, unsigned int startIndex = 0, bool instanced = false) : m_StartIndex(startIndex), m_Stride(0), m_IsInstanced(instanced) {
		m_Elements = std::move(elements);
		for (const auto& le : m_Elements)
			m_Stride += le.GetSize();
	}

	constexpr BufferLayout(unsigned int startIndex, unsigned int stride, const std::vector<LayoutElement>& elements, bool instanced) : m_StartIndex(startIndex), m_Stride(stride), m_Elements(elements), m_IsInstanced(instanced) {}

	constexpr BufferLayout(unsigned int StartIndex = 0, bool instanced = false) : m_StartIndex(StartIndex), m_IsInstanced(instanced) {}

	constexpr unsigned int GetStride() const { return m_Stride; }

	constexpr const std::vector<LayoutElement>& GetElements() const { return m_Elements; }

	constexpr unsigned int GetStartIndex() const { return m_StartIndex; }

	constexpr bool GetInstanced() const { return m_IsInstanced; }

	constexpr void SetInstanced(bool instanced = true) { m_IsInstanced = instanced; }
};

class VertexArray
{
private:
	GLuint m_ID;
	unsigned int m_VBIndex = 0;

public:
	explicit VertexArray(bool actuallyGenerateNow = true);

	bool IsValid() const { return m_ID != 0; }

	~VertexArray();

	VertexArray& operator=(VertexArray&& other) noexcept {
		std::swap(m_ID, other.m_ID);
		std::swap(m_VBIndex, other.m_VBIndex);
		return *this;
	}

	VertexArray& operator=(const VertexArray& other) = delete;

	void Bind() const;
	void UnBind() const;

	void AddBuffer(const VertexBuffer& VBO, const BufferLayout& layout);
	void AddIndexBuffer(const IndexBuffer& IBO);
};
