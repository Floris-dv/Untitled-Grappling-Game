#include "pch.h"
#include "Camera.h"

Camera* Camera::s_Camera;

Camera::Camera(CameraOptions options, float aspectRatio, glm::vec3 Position, glm::vec3 Up, float yaw, float pitch) :
	Options(options), Vel(glm::vec3(0.0f)), Front(glm::vec3(0.0f, 0.0f, -1.0f)), AspectRatio(aspectRatio), Position(Position), m_WorldUp(Up), m_Up(Up), m_Yaw(yaw), m_Pitch(pitch)
{
	m_Right = glm::normalize(glm::cross(Front, m_WorldUp));
}

void Camera::GenerateEverything()
{
	m_ViewMatrix = glm::lookAt(Position, Position + Front, m_Up);
	m_ProjMatrix = glm::perspective(glm::radians(Options.FovY), AspectRatio, Options.ZNear, Options.ZFar);
	m_VPMatrix = m_ProjMatrix * m_ViewMatrix;

	const float halfVSide = Options.ZFar * tanf(glm::radians(Options.FovY) * .5f);
	const float halfHSide = halfVSide * AspectRatio;
	const glm::vec3 frontMultFar = Options.ZFar * Front;

	m_Frustum.nearFace = { Position + Options.ZNear * Front, Front };
	m_Frustum.farFace = { Position + frontMultFar, -Front };

	m_Frustum.rightFace = { Position,
							glm::cross(m_Up,frontMultFar + m_Right * halfHSide) };
	m_Frustum.leftFace = { Position,
							glm::cross(frontMultFar - m_Right * halfHSide, m_Up) };
	m_Frustum.bottomFace = { Position,
							glm::cross(m_Right, frontMultFar - m_Up * halfVSide) };
	m_Frustum.topFace = { Position,
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
	const float velocity = Options.MovementSpeed * deltaTime;

	switch (direction) {
	case Camera_Movement::FORWARD:
		Vel += glm::normalize(glm::vec3(Front.x, 0.0f, Front.z)) * velocity; break;
	case Camera_Movement::BACKWARD:
		Vel -= glm::normalize(glm::vec3(Front.x, 0.0f, Front.z)) * velocity; break;
	case Camera_Movement::RIGHT:
		Vel += glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z)) * velocity; break;
	case Camera_Movement::LEFT:
		Vel -= glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z)) * velocity; break;
	case Camera_Movement::UP:
		Vel += m_WorldUp * velocity; break;
	}
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch)
{
	xoffset *= Options.MouseSensitivity;
	yoffset *= Options.MouseSensitivity;

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
	Options.FovY -= yoffset;
	if (Options.FovY < 1.0f)
		Options.FovY = 1.0f;

	if (Options.FovY > 180.0f)
		Options.FovY = 180.0f;
}

void Camera::UpdateCameraVectors(float deltaTime)
{
	m_IsDirty = true;

	Vel.x *= glm::pow(0.97f, deltaTime * Options.Resistance);

	// if Vel.z > 0: subtracts from it: else: adds to it
	Vel.z *= glm::pow(0.97f, deltaTime * Options.Resistance);

	if (Position.y < 0) {
		// reset the m_Position and Vel
		Position.y = 0;
		Vel.y = 0;
	}
	else if (Vel.y != 0)
		Vel.y -= GRAVITY * deltaTime;

	// update m_Position
	Position += Vel * deltaTime * 60.0f;

	// calculate the new Front vector
	Front.x = cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	Front.y = sin(glm::radians(m_Pitch));
	Front.z = sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch));
	Front = glm::normalize(Front);
	// also re-calculate the Right and Up vector
	m_Right = glm::normalize(glm::cross(Front, m_WorldUp));  // normalize the vectors, because their length gets closer to 0 the more you look Up or down which results in slower movement.
	m_Up = glm::normalize(glm::cross(m_Right, Front));
}

const Frustum& Camera::GetFrustum()
{
	if (m_IsDirty)
		GenerateEverything();
	return m_Frustum;
}