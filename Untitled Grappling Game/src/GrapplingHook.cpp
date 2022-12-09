#include "pch.h"
#include "GrapplingHook.h"
#include "VertexData.h"

glm::vec3 GrapplingHook::Update(const glm::vec3& position)
{
	if (!m_Held)
		return glm::vec3();

	glm::vec3 force{};
	float distance = glm::distance(position, m_GrapplingPoint);

	if (distance > m_RopeLength) {
		force -= glm::vec3(distance - m_RopeLength * m_Stiffness) * (position - m_GrapplingPoint);
		m_RopeLength = distance;
	}
	else
		m_RopeLength -= (m_RopeLength - distance) * 0.9f + distance * 0.01f;

	return force;
}

void GrapplingHook::Launch(const glm::vec3& point)
{
	if (m_Held)
		return;

	m_GrapplingPoint = point;
	m_Held = true;
	m_RopeLength = 10000.0f;
}