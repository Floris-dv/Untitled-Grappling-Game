class StaticRigidBody {
  glm::vec3 m_Position;
  glm::vec3 m_Size;
  glm::quat m_Rotation;

public:
  StaticRigidBody(const glm::vec3 &m_Position, const glm::vec3 &m_Size,
                  const glm::quat &m_Rotation)
      : m_Position(m_Position), m_Size(m_Size), m_Rotation(m_Rotation) {}
  StaticRigidBody(const StaticRigidBody &) = default;
  StaticRigidBody(StaticRigidBody &&) = default;
  StaticRigidBody &operator=(const StaticRigidBody &) = default;
  StaticRigidBody &operator=(StaticRigidBody &&) = default;
};
