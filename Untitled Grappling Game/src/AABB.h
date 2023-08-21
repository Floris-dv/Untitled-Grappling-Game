#pragma once
#include "Transform.h"
#include <assimp/aabb.h>

struct Plane {
  // unit vector
  glm::vec3 normal = {0.f, 1.f, 0.f};

  // distance from origin to the nearest point in the plane
  float distance = 0.f;

  Plane() = default;

  Plane(const glm::vec3 &p1, const glm::vec3 &norm)
      : normal(glm::normalize(norm)), distance(glm::dot(normal, p1)) {}

  float getSignedDistanceToPlane(const glm::vec3 &point) const {
    return glm::dot(normal, point) - distance;
  }
};

struct Frustum {
  Plane topFace;
  Plane bottomFace;

  Plane rightFace;
  Plane leftFace;

  Plane farFace;
  Plane nearFace;
};

struct BoundingVolume {
  virtual ~BoundingVolume() {}

  virtual bool isOnFrustum(const Frustum &camFrustum,
                           const Transform &transform) const = 0;

  virtual bool isOnOrForwardPlane(const Plane &plan) const = 0;

  bool isOnFrustum(const Frustum &camFrustum) const;
};

struct AABB : public BoundingVolume {
  glm::vec3 center{0.f, 0.f, 0.f};
  glm::vec3 extents{0.f, 0.f, 0.f};

  AABB(const glm::vec3 &min, const glm::vec3 &max)
      : BoundingVolume{}, center{(max + min) * 0.5f}, extents{max.x - center.x,
                                                              max.y - center.y,
                                                              max.z -
                                                                  center.z} {}

  AABB(const glm::vec3 &inCenter, float iI, float iJ, float iK)
      : BoundingVolume{}, center{inCenter}, extents{iI, iJ, iK} {}

  AABB(const aiAABB &aabb)
      : AABB({aabb.mMin.x, aabb.mMin.y, aabb.mMin.z},
             {aabb.mMax.x, aabb.mMax.y, aabb.mMax.z}) {}

  AABB(const AABB &aabb) = default;

  AABB() = default;

  virtual ~AABB() {}

  std::array<glm::vec3, 8> getVertices() const;
  // see
  // https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plan.html
  bool isOnOrForwardPlane(const Plane &plane) const final;
  bool isOnFrustum(const Frustum &camFrustum,
                   const Transform &transform) const final;
  ;
};