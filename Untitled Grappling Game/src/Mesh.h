#pragma once
#include "DataBuffers.h"
#include "Material.h"
#include "Texture.h"
#include "UtilityMacros.h"
#include "Vertex.h"

template <typename T = Vertex> struct Mesh {
public:
  Mesh() : m_VAO(false) {}

  void swap(Mesh<T> &mesh);

  OVERLOAD_OPERATOR_RVALUE(Mesh<T>)

  explicit Mesh(std::span<const T> vertices, const BufferLayout &bufferLayout,
                std::span<const GLuint> indices);

  explicit Mesh(std::span<const T> vertices, const BufferLayout &bufferLayout)
      : Mesh(vertices, bufferLayout, std::span<GLuint, 0>()) {}

  Mesh(const Mesh<T> &other) = delete;
  // apparently, this is the only thing that works on my machine
  Mesh<T> &operator=(const Mesh<T> &other) = delete;

  virtual ~Mesh() noexcept {}

  void Draw(const glm::mat4 &modelMatrix, Material *material,
            bool setMaterial) {
    if (setMaterial)
      Draw(modelMatrix, (EmptyMaterial *)material, material->GetShader());
    else
      Draw(modelMatrix, material->GetShader());
  }

  void Draw(const glm::mat4 &modelMatrix, const EmptyMaterial *material,
            Shader *shader);

  void Draw(const glm::mat4 &modelMatrix, Shader *shader);

  void DrawInstanced(Material *material, bool setMaterial, GLsizei count) {
    if (setMaterial)
      DrawInstanced((EmptyMaterial *)material, material->GetShader(), count);
    else
      DrawInstanced(material->GetShader(), count);
  }

  void DrawInstanced(EmptyMaterial *material, Shader *shader, GLsizei count);

  void DrawInstanced(Shader *shader, GLsizei count);

  virtual void SetInstanceBuffer(const VertexBuffer &instanceBuffer);

  inline bool IsValid() { return m_VAO.IsValid(); }

private:
  GLsizei m_NumVerts = 0;
  bool m_UseIBO = false;

  VertexBuffer m_VBO;
  IndexBuffer m_IBO;
  VertexArray m_VAO;

  void DoDrawCall();

  void DoInstancedDrawCall(GLsizei count);
};

namespace std {
template <typename T>
inline void swap(Mesh<T> &a, Mesh<T> &b) noexcept(noexcept(a.swap(b))) {
  a.swap(b);
}
} // namespace std

static const BufferLayout instanceBufferLayout = {
    5,
    sizeof(glm::mat4),
    {{GL_FLOAT, 4}, {GL_FLOAT, 4}, {GL_FLOAT, 4}, {GL_FLOAT, 4}},
    true};

template <typename T> void Mesh<T>::swap(Mesh<T> &other) {
  SWAP(m_VBO);
  SWAP(m_IBO);
  SWAP(m_VAO);
  SWAP(m_NumVerts);
  SWAP(m_UseIBO);
}

template <typename T>
inline void Mesh<T>::Draw(const glm::mat4 &modelMatrix,
                          const EmptyMaterial *material, Shader *shader) {
  if (!m_NumVerts)
    return;

  material->Load(*shader);

  shader->SetMat4("model", modelMatrix);

  DoDrawCall();
}

template <typename T>
inline void Mesh<T>::Draw(const glm::mat4 &modelMatrix, Shader *shader) {
  if (!m_NumVerts)
    return;

  shader->Use();
  shader->SetBool("material.useTex", false);
  shader->SetMat4("model", modelMatrix);

  DoDrawCall();
}

template <typename T>
void Mesh<T>::DrawInstanced(EmptyMaterial *material, Shader *shader,
                            GLsizei count) {
  if (!m_NumVerts)
    return;

  material->Load(*shader);

  DoInstancedDrawCall(count);
}

template <typename T>
inline void Mesh<T>::DrawInstanced(Shader *shader, GLsizei count) {
  if (!m_NumVerts)
    return;

  shader->Use();
  shader->SetBool("material.useTex", false);

  DoInstancedDrawCall(count);
}

template <typename T>
void Mesh<T>::SetInstanceBuffer(const VertexBuffer &instanceBuffer) {
  m_VAO.Bind();
  m_VAO.AddBuffer(instanceBuffer, instanceBufferLayout);
}

template <typename T> inline void Mesh<T>::DoInstancedDrawCall(GLsizei count) {
  m_VAO.Bind();
  if (m_UseIBO)
    glDrawElementsInstanced(GL_TRIANGLES, m_NumVerts, GL_UNSIGNED_INT,
                            (void *)0, count);
  else
    glDrawArraysInstanced(GL_TRIANGLES, 0, m_NumVerts, count);

  m_VAO.UnBind();
}

template <typename T> inline void Mesh<T>::DoDrawCall() {
  m_VAO.Bind();
  if (m_UseIBO)
    glDrawElements(GL_TRIANGLES, m_NumVerts, GL_UNSIGNED_INT, (void *)0);
  else
    glDrawArrays(GL_TRIANGLES, 0, m_NumVerts);

  m_VAO.UnBind();
}

template <typename T>
inline Mesh<T>::Mesh(std::span<const T> vertices,
                     const BufferLayout &bufferLayout,
                     std::span<const GLuint> indices)
    : m_UseIBO(!indices.empty()),
      m_VBO((unsigned int)vertices.size_bytes(), vertices.data()) {
  m_VAO.AddBuffer(m_VBO, bufferLayout);

  if (m_UseIBO) {
    m_NumVerts = (GLsizei)indices.size();
    m_IBO = IndexBuffer(indices.size_bytes(), indices.data());
    m_VAO.AddIndexBuffer(m_IBO);
  } else
    m_NumVerts = (GLsizei)vertices.size();
}
