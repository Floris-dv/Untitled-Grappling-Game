#pragma once
#include <glm/glm.hpp>
#include "Material.h"
#include "DataBuffers.h"

class GrapplingHook
{
private:
	glm::vec3 m_GrapplingPoint{ 0.0f };
	float m_RopeLength = 0.0f;

	float m_Stiffness = 1.0f;

	bool m_Held = false;

	inline static VertexBuffer s_LineBuffer;
	inline static Material s_Material;

public:
	// Output should be seen as a force vector
	glm::vec3 Update(const glm::vec3& position);
	void Launch(const glm::vec3& point);
	void Release() { m_Held = false; }

	bool Active() { return m_Held; }
};

