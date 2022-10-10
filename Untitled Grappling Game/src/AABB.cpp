#include "pch.h"
#include "AABB.h"

bool BoundingVolume::isOnFrustum(const Frustum& camFrustum) const
{
	return (isOnOrForwardPlane(camFrustum.leftFace) &&
		isOnOrForwardPlane(camFrustum.rightFace) &&
		isOnOrForwardPlane(camFrustum.topFace) &&
		isOnOrForwardPlane(camFrustum.bottomFace) &&
		isOnOrForwardPlane(camFrustum.nearFace) &&
		isOnOrForwardPlane(camFrustum.farFace));
}

std::array<glm::vec3, 8> AABB::getVertices() const
{
	std::array<glm::vec3, 8> vertices = {
	 glm::vec3{ center.x - extents.x, center.y - extents.y, center.z - extents.z },
	 glm::vec3{ center.x + extents.x, center.y - extents.y, center.z - extents.z },
	 glm::vec3{ center.x - extents.x, center.y + extents.y, center.z - extents.z },
	 glm::vec3{ center.x + extents.x, center.y + extents.y, center.z - extents.z },
	 glm::vec3{ center.x - extents.x, center.y - extents.y, center.z + extents.z },
	 glm::vec3{ center.x + extents.x, center.y - extents.y, center.z + extents.z },
	 glm::vec3{ center.x - extents.x, center.y + extents.y, center.z + extents.z },
	 glm::vec3{ center.x + extents.x, center.y + extents.y, center.z + extents.z },
	};
	return vertices;
}

// see https://gdbooks.gitbooks.io/3dcollisions/content/Chapter2/static_aabb_plan.html
bool AABB::isOnOrForwardPlane(const Plane& plane) const
{
	// Compute the projection interval radius of b onto L(t) = b.c + t * p.n
	// const float r = glm::dot(glm::abs(plane.normal), extents);

	// Compute the projection interval radius of b onto L(t) = b.c + t * p.n
	const float r = extents.x * std::abs(plane.normal.x) +
		extents.y * std::abs(plane.normal.y) + extents.z * std::abs(plane.normal.z);

	return -r <= plane.getSignedDistanceToPlane(center);
}

bool AABB::isOnFrustum(const Frustum& camFrustum, const Transform& transform) const
{
	//Get global scale thanks to our transform
	const glm::vec3 globalCenter{ transform.getModelMatrix() * glm::vec4(center, 1.f) };

	// Scaled orientation
	const glm::vec3 right = transform.getRight() * extents.x;
	const glm::vec3 up = transform.getUp() * extents.y;
	const glm::vec3 forward = transform.getForward() * extents.z;

	const float newIi = std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, right)) +
		std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, up)) +
		std::abs(glm::dot(glm::vec3{ 1.f, 0.f, 0.f }, forward));

	const float newIj = std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, right)) +
		std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, up)) +
		std::abs(glm::dot(glm::vec3{ 0.f, 1.f, 0.f }, forward));

	const float newIk = std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, right)) +
		std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, up)) +
		std::abs(glm::dot(glm::vec3{ 0.f, 0.f, 1.f }, forward));

	const AABB globalAABB(globalCenter, newIi, newIj, newIk);

	return (globalAABB.isOnOrForwardPlane(camFrustum.leftFace) &&
		globalAABB.isOnOrForwardPlane(camFrustum.rightFace) &&
		globalAABB.isOnOrForwardPlane(camFrustum.topFace) &&
		globalAABB.isOnOrForwardPlane(camFrustum.bottomFace) &&
		globalAABB.isOnOrForwardPlane(camFrustum.nearFace) &&
		globalAABB.isOnOrForwardPlane(camFrustum.farFace));
}