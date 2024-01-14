#pragma once

// Collider implementing the support function used by the
// Gilbert–Johnson–Keerthi algorithm
struct Collider {
  glm::vec3 Center;
  glm::mat3 MatrixRS; // Rotation and Scale
  glm::mat3 Inverse_MatrixRS;

  Collider(const glm::vec3 &c, const glm::mat4 &m)
      : Center(c), MatrixRS(m), Inverse_MatrixRS(glm::inverse(m)) {}

  Collider() = default;

  virtual ~Collider() {}

  virtual glm::vec3 Support(const glm::vec3 &direction) const {
    return MatrixRS * FindFurthestPoint(Inverse_MatrixRS * direction) + Center;
  }

  virtual glm::vec3 FindFurthestPoint(const glm::vec3 &direction) const = 0;
};

// Box with with sidelength 2 around the origin, everything else needs to be in
// the Collider
struct BoxCollider : public Collider {
  BoxCollider(const glm::vec3 &c = glm::vec3{0.0f},
              const glm::vec3 &s = glm::vec3{0.5f},
              const glm::quat &rot = {1.0f, 0.0f, 0.0f, 0.0f})
      : Collider{c,
                 glm::mat3{rot} * glm::mat3(glm::scale(glm::mat4{1.0f}, s))} {}

  glm::vec3 FindFurthestPoint(const glm::vec3 &direction) const override {
    return sign(direction);
  }
};

// Capsule: Height-aligned with y-axis
struct CapsuleCollider : public Collider {
  float r, y_base, y_cap;

  glm::vec3 FindFurthestPoint(const glm::vec3 &direction) const override {
    glm::vec3 result = glm::normalize(direction) * r;
    result.y += (direction.y > 0) ? y_cap : y_base;

    return result;
  }
};
struct SphereCollider : public Collider {
  float radius;

  SphereCollider(const glm::vec3 &c, float r)
      : Collider{c, glm::mat3{1.0f}}, radius(r) {}

  // Override Support directly, as it's simpeler
  glm::vec3 Support(const glm::vec3 &direction) const override {
    return glm::normalize(direction) * radius + Center;
  }

  glm::vec3 FindFurthestPoint(const glm::vec3 &direction) const override {
    return glm::normalize(direction);
  }
};

constexpr glm::vec3 Support(const Collider *colliderA,
                            const Collider *colliderB,
                            const glm::vec3 &direction) {
  return colliderB->Support(direction) - colliderA->Support(-direction);
}

struct HitInfo {
  glm::vec3 Normal;
  float PenetrationDepth;
  bool Hit;
};

HitInfo Collide(const Collider *p, const Collider *q,
                const glm::vec3 &initial_axis);

bool Intersect(const Collider *p, const Collider *q,
               const glm::vec3 &initial_axis);

inline bool Intersect(const Collider *p, const Collider *q) {
  return Intersect(p, q, glm::vec3{1.0f, 0.0f, 0.0f});
}
inline HitInfo Collide(const Collider *p, const Collider *q) {
  return Collide(p, q, glm::vec3{1.0f, 0.0f, 0.0f});
}
