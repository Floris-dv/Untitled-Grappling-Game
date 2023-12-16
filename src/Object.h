#pragma once
#include "Material.h"
#include "Mesh.h"
#include "UtilityMacros.h"

template <typename T = Vertex> struct Object : public Mesh<T> {
private:
  std::shared_ptr<Material> m_MainMaterial = nullptr;

public:
  Object() {}

  explicit Object(std::shared_ptr<Material> material,
                  const std::span<T> vertices, const BufferLayout &bufferLayout,
                  const std::span<GLuint> indices)
      : Mesh<T>(vertices, bufferLayout, indices),
        m_MainMaterial(std::move(material)) {}

  explicit Object(std::shared_ptr<Material> material,
                  const std::span<T> vertices, const BufferLayout &bufferLayout)
      : Object(material, vertices, bufferLayout, std::span<GLuint, 0>()) {}
  void swap(Object<T> &other);

  Object<T> &operator=(Object<T> &&other) noexcept;

  Object<T> &operator=(const Object<T> &other) = delete;

  virtual ~Object() noexcept {}

  void Draw(const glm::mat4 &modelMatrix, bool setMaterial) {
    Draw(modelMatrix, m_MainMaterial.get(), setMaterial);
  }

  void DrawInstanced(bool setMaterial, GLsizei count) {
    DrawInstanced(m_MainMaterial.get(), setMaterial, count);
  }

  void DoOpenGL(bool deleteAfter) { m_MainMaterial->LoadTextures(deleteAfter); }
};

template <typename T> void Object<T>::swap(Object<T> &other) {
  SWAP(m_MainMaterial);
  std::swap((Mesh<T> &)*this, (Mesh<T> &)other);
}
template <typename T>
Object<T> &Object<T>::operator=(Object<T> &&other) noexcept {
  swap(other);
  return *this;
}
