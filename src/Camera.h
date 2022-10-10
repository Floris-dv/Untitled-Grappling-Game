#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/vec_swizzle.hpp>

#include "AABB.h"

#define DEFAULTYAW -90.0f
#define DEFAULTPITCH 0.0f
#define DEFAULTSPEED 2.5f
#define DEFAULTSENSITIVITY 0.1f
#define DEFAULTFOV 90.0f
#define SLIPPERYNESS 10.0f
#define GRAVITY 1.0f

enum class Camera_Movement {
	// positive x is to the right:
	FORWARD,  //  0,  0, -1
	BACKWARD, //  0,  0,  1
	LEFT,     // -1,  0, -0
	RIGHT,	  //  1,  0,  0
	UP,       //  0,  1, -0
	DOWN	  //  0, -1,  0
};
class Camera
{
	static Camera* s_Camera;

	bool m_IsDirty = true;
	glm::mat4 m_ProjMatrix{ 1.0f };
	glm::mat4 m_ViewMatrix{ 1.0f };
	glm::mat4 m_VPMatrix{ 1.0f };
	Frustum m_Frustum;

	void GenerateEverything();

	// constructor with vectors
	Camera(float aspect, float znear, float zfar, float mass, glm::vec3 position, glm::vec3 up, float yaw, float pitch);

public:
	// camera Attributes
	glm::vec3 m_Position;
	glm::vec3 m_Front;
	glm::vec3 m_Up;
	glm::vec3 m_Right;
	glm::vec3 m_WorldUp;
	glm::vec3 m_Vel; // velocity
	// euler Angles
	float m_Yaw;
	float m_Pitch;
	// camera options
	float m_MovementSpeed;
	float m_MouseSensitivity;
	float m_FovY;
	float m_Aspect;
	float m_zNear, m_zFar;

	float m_DMass; // 1/mass
	float k = 100.0f;

	static void Initialize(float aspect, float znear, float zfar, float mass = 80.0f, glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = DEFAULTYAW, float pitch = DEFAULTPITCH) {
		if (!s_Camera)
			s_Camera = new Camera(aspect, znear, zfar, mass, position, up, yaw, pitch);
	}

	static void Initialize(float aspect, float znear, float zfar, float mass, float posX, float posY, float posZ, float upX = 0.0f, float upY = 1.0f, float upZ = 0.0f, float yaw = DEFAULTYAW, float pitch = DEFAULTPITCH) {
		if (!s_Camera)
			s_Camera = new Camera(aspect, znear, zfar, mass, { posX, posY, posZ }, { upX, upY, upZ }, yaw, pitch);
	}

	static Camera& Get() { return *s_Camera; }

	// returns the view matrix calculated using Euler Angles and the LookAt Matrix
	const glm::mat4& GetViewMatrix();

	const glm::mat4& GetProjMatrix();

	const glm::mat4& GetVPMatrix();

	const Frustum& GetFrustum();

	// processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void ProcessKeyboard(Camera_Movement direction, float deltaTime);

	// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void ProcessMouseMovement(float xOffset, float yOffset, bool constrainPitch = true);

	// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll(float xOffset, float yOffset);

	// calculates the front vector from the Camera's (updated) Euler Angles
	void UpdateCameraVectors(float deltaTime);
};
