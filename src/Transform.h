#pragma once
class Transform {
protected:
  // Local space information
  glm::vec3 m_Pos = {0.0f, 0.0f, 0.0f};
  glm::vec3 m_EulerRot = {0.0f, 0.0f, 0.0f}; // In degrees
  glm::vec3 m_Scale = {1.0f, 1.0f, 1.0f};

  // Global space informaiton concatenate in matrix, just for caching
  mutable glm::mat4 m_ModelMatrix = glm::mat4(1.0f);

  // Dirty flag, just for caching
  mutable bool m_IsDirty = false;

protected:
  glm::mat4 getLocalModelMatrix() const {
    const glm::mat4 transformX =
        glm::rotate(glm::mat4(1.0f), glm::radians(m_EulerRot.x),
                    glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 transformY =
        glm::rotate(glm::mat4(1.0f), glm::radians(m_EulerRot.y),
                    glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 transformZ =
        glm::rotate(glm::mat4(1.0f), glm::radians(m_EulerRot.z),
                    glm::vec3(0.0f, 0.0f, 1.0f));

    // Y * X * Z
    const glm::mat4 rotationMatrix = transformY * transformX * transformZ;

    // translation * rotation * scale (also know as TRS matrix)
    return glm::translate(glm::mat4(1.0f), m_Pos) * rotationMatrix *
           glm::scale(glm::mat4(1.0f), m_Scale);
  }

public:
  void setLocalPosition(const glm::vec3 &newPosition) {
    m_Pos = newPosition;
    m_IsDirty = true;
  }

  void setLocalRotation(const glm::vec3 &newRotation) {
    m_EulerRot = newRotation;
    m_IsDirty = true;
  }

  void setLocalScale(const glm::vec3 &newScale) {
    m_Scale = newScale;
    m_IsDirty = true;
  }

  glm::vec3 getGlobalPosition() const { return glm::vec3(m_ModelMatrix[3]); }

  const glm::vec3 &getLocalPosition() const { return m_Pos; }

  const glm::vec3 &getLocalRotation() const { return m_EulerRot; }

  const glm::vec3 &getLocalScale() const { return m_Scale; }

  const glm::mat4 &getModelMatrix() const {
    if (m_IsDirty) {
      m_ModelMatrix = getLocalModelMatrix();
      m_IsDirty = false;
    }
    return m_ModelMatrix;
  }

  glm::vec3 getRight() const { return m_ModelMatrix[0]; }

  glm::vec3 getUp() const { return m_ModelMatrix[1]; }

  glm::vec3 getBackward() const { return m_ModelMatrix[2]; }

  glm::vec3 getForward() const { return -m_ModelMatrix[2]; }

  glm::vec3 getGlobalScale() const {
    return {glm::length(getRight()), glm::length(getUp()),
            glm::length(getBackward())};
  }

  bool isDirty() const { return m_IsDirty; }
};