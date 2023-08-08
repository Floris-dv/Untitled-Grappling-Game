// Implementation file of the VertexBuffer, IndexBuffer, BufferLayout, and VertexArray classes
#include "pch.h"
#include "DataBuffers.h"

// Buffer bind caching:
static GLuint s_VertexBufferBoundID = 0;
static GLuint s_IndexBufferBoundID = 0;

static GLuint s_VertexArrayBoundID = 0;

static GLuint s_UniformBufferBoundID = 0;

// IndexBuffer:
IndexBuffer::IndexBuffer(size_t size, const void* data) {
	glCreateBuffers(1, &m_ID);
	glNamedBufferData(m_ID, size, data, GL_STATIC_DRAW);
}

void IndexBuffer::Bind() const {
	if (s_IndexBufferBoundID != m_ID) {
		s_IndexBufferBoundID = m_ID;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ID);
	}
}

void IndexBuffer::UnBind() const {
#ifdef _DEBUG
	if (s_IndexBufferBoundID != 0) {
		s_IndexBufferBoundID = 0;
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
#endif
}

// VertexBuffer:
VertexBuffer::VertexBuffer(unsigned int size, const void* data) {
	glCreateBuffers(1, &m_ID);
	glNamedBufferData(m_ID, size, data, GL_STATIC_DRAW);
}

void VertexBuffer::Bind() const {
	if (s_VertexBufferBoundID != m_ID) {
		s_VertexBufferBoundID = m_ID;
		glBindBuffer(GL_ARRAY_BUFFER, m_ID);
	}
}

void VertexBuffer::UnBind() const {
#if _DEBUG
	if (s_VertexBufferBoundID != 0) {
		s_VertexBufferBoundID = 0;
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
#endif
}

VertexArray::VertexArray(bool actuallyGenerateNow) {
	if (actuallyGenerateNow) {
		glGenVertexArrays(1, &m_ID);
		Bind();
	}
	else
		m_ID = 0;
}

VertexArray::~VertexArray() {
	if (m_ID)
		glDeleteVertexArrays(1, &m_ID);
}

// VertexArray:
void VertexArray::Bind() const {
	if (m_ID != s_VertexArrayBoundID) {
		s_VertexArrayBoundID = m_ID;
		glBindVertexArray(m_ID);
	}
}

void VertexArray::UnBind() const {
#if _DEBUG
	if (s_VertexArrayBoundID != 0) {
		s_VertexArrayBoundID = 0;
		glBindVertexArray(0);
	}
#endif
}

void VertexArray::AddBuffer(const VertexBuffer& VBO, const BufferLayout& layout) {
	AddBuffer(VBO, layout, m_VBIndex++);
}

void VertexArray::AddBuffer(const VertexBuffer& VBO, const BufferLayout& layout, unsigned int VBIndex)
{
	Bind();
	VBO.Bind();

	unsigned int offset = 0;
	for (int i = 0; i < layout.GetElements().size(); i++) {
		LayoutElement e = layout.GetElements()[i];
		GLuint j = layout.GetStartIndex() + i;
		glEnableVertexArrayAttrib(m_ID, j);
		glVertexArrayAttribBinding(m_ID, j, VBIndex);
		glVertexArrayAttribFormat(m_ID, j, e.Count, e.Type, e.Normalized ? (GLboolean)GL_TRUE : (GLboolean)GL_FALSE, offset);

		offset += e.GetSize();
	}
	glVertexArrayVertexBuffer(m_ID, VBIndex, VBO.ID(), 0, (GLsizei)layout.GetStride());

	if (layout.GetInstanced())
		glVertexArrayBindingDivisor(m_ID, VBIndex, 1);
}

void VertexArray::AddIndexBuffer(const IndexBuffer& IBO) {
	glVertexArrayElementBuffer(m_ID, IBO.ID());
}


UniformBuffer::UniformBuffer(size_t size, const std::string& name, const void* data) : m_Name(name)
{
	glCreateBuffers(1, &m_ID);
	glNamedBufferData(m_ID, (GLsizeiptr)size, data, GL_STATIC_DRAW);

	Bind();
	glBindBufferBase(GL_UNIFORM_BUFFER, s_MaxBlock, m_ID);
	m_BlockID = s_MaxBlock;
	s_MaxBlock++;
}

void UniformBuffer::SetBlock(Shader& shader)
{
	shader.SetBlock(m_Name.c_str(), m_BlockID);
}

void UniformBuffer::SetData(size_t offset, size_t size, const void* data)
{
	Bind();
	glBufferSubData(GL_UNIFORM_BUFFER, (GLintptr)offset, (GLsizeiptr)size, data);
}

void UniformBuffer::Bind() const
{
	if (s_UniformBufferBoundID != m_ID) {
		s_UniformBufferBoundID = m_ID;
		glBindBuffer(GL_UNIFORM_BUFFER, m_ID);
	}
}

void UniformBuffer::UnBind() const
{
#ifdef _DEBUG
	if (s_UniformBufferBoundID != 0) {
		s_UniformBufferBoundID = 0;
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
#endif
}

