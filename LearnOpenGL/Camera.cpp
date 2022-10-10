#include "pch.h"
#include "Camera.h"

Camera* Camera::s_Camera;

Camera::Camera(float m_Aspect, float m_zNear, float m_zFar, float mass, glm::vec3 m_Position, glm::vec3 m_Up, float m_Yaw, float m_Pitch) :
	m_DMass(1 / mass), m_Vel(glm::vec3(0.0f)), m_Front(glm::vec3(0.0f, 0.0f, -1.0f)), m_MovementSpeed(DEFAULTSPEED), m_MouseSensitivity(DEFAULTSENSITIVITY),
	m_FovY(DEFAULTFOV), m_Aspect(m_Aspect), m_zNear(m_zNear), m_zFar(m_zFar), m_Position(m_Position), m_WorldUp(m_Up), m_Up(m_Up), m_Yaw(m_Yaw), m_Pitch(m_Pitch)
{
	m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));
}

void Camera::GenerateEverything()
{
	m_ViewMatrix = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
	m_ProjMatrix = glm::perspective(glm::radians(m_FovY), m_Aspect, m_zNear, m_zFar);
	m_VPMatrix = m_ProjMatrix * m_ViewMatrix;

	const float halfVSide = m_zFar * tanf(glm::radians(m_FovY) * .5f);
	const float halfHSide = halfVSide * m_Aspect;
	const glm::vec3 frontMultFar = m_zFar * m_Front;

	m_Frustum.nearFace = { m_Position + m_zNear * m_Front, m_Front };
	m_Frustum.farFace = { m_Position + frontMultFar, -m_Front };

	m_Frustum.rightFace = { m_Position,
							glm::cross(m_Up,frontMultFar + m_Right * halfHSide) };
	m_Frustum.leftFace = { m_Position,
							glm::cross(frontMultFar - m_Right * halfHSide, m_Up) };
	m_Frustum.bottomFace = { m_Position,
							glm::cross(m_Right, frontMultFar - m_Up * halfVSide) };
	m_Frustum.topFace = { m_Position,
							glm::cross(frontMultFar + m_Up * halfVSide, m_Right) };

	m_IsDirty = false;
}

const glm::mat4& Camera::GetViewMatrix()
{
	if (m_IsDirty)
		GenerateEverything();
	return m_ViewMatrix;
}

const glm::mat4& Camera::GetProjMatrix()
{
	if (m_IsDirty)
		GenerateEverything();
	return m_ProjMatrix;
}

const glm::mat4& Camera::GetVPMatrix()
{
	if (m_IsDirty)
		GenerateEverything();
	return m_VPMatrix;
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime)
{
	const float velocity = m_MovementSpeed * deltaTime;

	switch (direction) {
	case Camera_Movement::FORWARD:
		m_Vel += glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z)) * velocity; break;
	case Camera_Movement::BACKWARD:
		m_Vel -= glm::normalize(glm::vec3(m_Front.x, 0.0f, m_Front.z)) * velocity; break;
	case Camera_Movement::RIGHT:
		m_Vel += glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z)) * velocity; break;
	case Camera_Movement::LEFT:
		m_Vel -= glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z)) * velocity; break;
	case Camera_Movement::UP:
		m_Vel += m_WorldUp * velocity; break;
	}
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
	xoffset *= m_MouseSensitivity;
	yoffset *= m_MouseSensitivity;

	m_Yaw += xoffset;
	m_Pitch += yoffset;

	// make sure that when m_Pitch is out of bounds, screen doesn't get flipped
	if (constrainPitch)
	{
		if (m_Pitch > 89.0f)
			m_Pitch = 89.0f;
		if (m_Pitch < -89.0f)
			m_Pitch = -89.0f;
	}
}

void Camera::ProcessMouseScroll(float xoffset, float yoffset)
{
	m_FovY -= yoffset;
	if (m_FovY < 1.0f)
		m_FovY = 1.0f;

	if (m_FovY > 180.0f)
		m_FovY = 180.0f;
}

void Camera::UpdateCameraVectors(float deltaTime)
{
	m_IsDirty = true;

	m_Vel.x *= glm::pow(0.97f, deltaTime * k);

	// if m_Vel.z > 0: subtracts from it: else: adds to it
	m_Vel.z *= glm::pow(0.97f, deltaTime * k);

	if (m_Position.y < 0) {
		// reset the m_Position and m_Vel
		m_Position.y = 0;
		m_Vel.y = 0;
	}
	else if (m_Vel.y != 0)
		m_Vel.y -= GRAVITY * deltaTime;

	// update m_Position
	m_Position += m_Vel * deltaTime * 60.0f;

	// calculate the new m_Front vector
	glm::vec3 Front;
	Front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	Front.y = sin(glm::radians(m_Pitch));
	Front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	m_Front = glm::normalize(Front);
	// also re-calculate the m_Right and m_Up vector
	m_Right = glm::normalize(glm::cross(m_Front, m_WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look m_Up or down which results in slower movement.
	m_Up = glm::normalize(glm::cross(m_Right, m_Front));
}

const Frustum& Camera::GetFrustum()
{
	if (m_IsDirty)
		GenerateEverything();
	return m_Frustum;
}