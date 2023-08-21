#pragma once
#include "DataBuffers.h"
#include "Material.h"
#include "Texture.h"
#include "UtilityMacros.h"
#include "Vertex.h"

template <typename T = Vertex> struct Object {
private:
  unsigned int m_NumVerts;
  bool m_UseIBO = false;

  VertexBuffer m_VBO;

  IndexBuffer m_IBO;

  std::shared_ptr<Material> m_MainMaterial = nullptr;

  void DoDrawCall();

  void DoInstancedDrawCall(unsigned int count);

public:
  VertexArray VAO;

  Object() noexcept : m_NumVerts(0), VAO(false) {}

  Object<T> &operator=(Object<T> &&other) noexcept;

  Object(std::shared_ptr<Material> material, const std::span<T> vertices,
         const BufferLayout &bufferLayout, const std::span<GLuint> indices);

  Object(std::shared_ptr<Material> material, const std::span<T> vertices,
         const BufferLayout &bufferLayout)
      : Object(material, vertices, bufferLayout, std::span<GLuint, 0>()) {}

  DELETE_COPY_CONSTRUCTOR(Object<T>)

  virtual ~Object() noexcept {}

  void Draw(const glm::mat4 &modelMatrix, bool setMaterial) {
    Draw(modelMatrix, m_MainMaterial.get(), setMaterial);
  }

  void Draw(const glm::mat4 &modelMatrix, Material *material,
            bool setMaterial) {
    if (setMaterial)
      Draw(modelMatrix, (EmptyMaterial *)material, material->GetShader());
    else
      Draw(modelMatrix, material->GetShader());
  }

  void Draw(const glm::mat4 &modelMatrix, EmptyMaterial *material,
            Shader *shader);

  void Draw(const glm::mat4 &modelMatrix, Shader *shader);

  void DrawInstanced(bool setMaterial, unsigned int count) {
    DrawInstanced(m_MainMaterial.get(), setMaterial, count);
  }

  void DrawInstanced(Material *material, bool setMaterial, unsigned int count) {
    if (setMaterial)
      DrawInstanced((EmptyMaterial *)material, material->GetShader(), count);
    else
      DrawInstanced(material->GetShader(), count);
  }

  void DrawInstanced(EmptyMaterial *material, Shader *shader,
                     unsigned int count);

  void DrawInstanced(Shader *shader, unsigned int count);

  virtual void SetInstanceBuffer(const VertexBuffer &instanceBuffer);

  inline bool IsValid() { return VAO.IsValid(); }

  void DoOpenGL(bool deleteAfter) { m_MainMaterial->LoadTextures(deleteAfter); }
};

static const BufferLayout instanceBufferLayout = {
    5,
    sizeof(glm::mat4),
    {{GL_FLOAT, 4}, {GL_FLOAT, 4}, {GL_FLOAT, 4}, {GL_FLOAT, 4}},
    true};

template <typename T>
inline void Object<T>::Draw(const glm::mat4 &modelMatrix,
                            EmptyMaterial *material, Shader *shader) {
  if (!m_NumVerts)
    return;

  material->Load(*shader);

  shader->SetMat4("model", modelMatrix);

  DoDrawCall();
}

template <typename T>
inline void Object<T>::Draw(const glm::mat4 &modelMatrix, Shader *shader) {
  if (!m_NumVerts)
    return;

  shader->Use();
  shader->SetBool("material.useTex", false);
  shader->SetMat4("model", modelMatrix);

  DoDrawCall();
}

template <typename T>
void Object<T>::DrawInstanced(EmptyMaterial *material, Shader *shader,
                              unsigned int count) {
  if (!m_NumVerts)
    return;

  material->Load(*shader);

  DoInstancedDrawCall(count);
}

template <typename T>
inline void Object<T>::DrawInstanced(Shader *shader, unsigned int count) {
  if (!m_NumVerts)
    return;

  shader->Use();
  shader->SetBool("material.useTex", false);

  DoInstancedDrawCall(count);
}

template <typename T>
void Object<T>::SetInstanceBuffer(const VertexBuffer &instanceBuffer) {
  VAO.AddBuffer(instanceBuffer, instanceBufferLayout);
}

template <typename T>
inline void Object<T>::DoInstancedDrawCall(unsigned int count) {
  VAO.Bind();
  if (m_UseIBO)
    glDrawElementsInstanced(GL_TRIANGLES, m_NumVerts, GL_UNSIGNED_INT,
                            (void *)0, count);
  else
    glDrawArraysInstanced(GL_TRIANGLES, 0, m_NumVerts, count);

  VAO.UnBind();
}

template <typename T> inline void Object<T>::DoDrawCall() {
  VAO.Bind();
  if (m_UseIBO)
    glDrawElements(GL_TRIANGLES, m_NumVerts, GL_UNSIGNED_INT, (void *)0);
  else
    glDrawArrays(GL_TRIANGLES, 0, m_NumVerts);

  VAO.UnBind();
}

template <typename T>
Object<T> &Object<T>::operator=(Object<T> &&other) noexcept {
  m_VBO = std::move(other.m_VBO);
  m_IBO = std::move(other.m_IBO);
  VAO = std::move(other.VAO);

  std::swap(m_MainMaterial, other.m_MainMaterial);

  std::swap(m_NumVerts, other.m_NumVerts);
  std::swap(m_UseIBO, other.m_UseIBO);

  return *this;
}

template <typename T>
inline Object<T>::Object(std::shared_ptr<Material> material,
                         const std::span<T> vertices,
                         const BufferLayout &bufferLayout,
                         const std::span<GLuint> indices)
    : m_MainMaterial(std::move(material)), m_UseIBO(!indices.empty()),
      m_VBO(vertices.size_bytes(), vertices.data()) {
  VAO.AddBuffer(m_VBO, bufferLayout);

  if (m_UseIBO) {
    m_NumVerts = indices.size();
    m_IBO = IndexBuffer(indices.size_bytes(), indices.data());
    VAO.AddIndexBuffer(m_IBO);
  } else
    m_NumVerts = vertices.size();
}
