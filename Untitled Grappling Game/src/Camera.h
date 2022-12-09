#pragma once

#include <glm/glm.hpp>
#include "GrapplingHook.h"

#include "AABB.h"

#define SLIPPERYNESS 10.0f
#define GRAVITY 1.0f

enum Camera_Movement : unsigned int {
	MOVEMENT_NONE = 0,
	// positive x is to the right:
	MOVEMENT_FORWARD = 1,	//  0,  0, -1
	MOVEMENT_BACKWARD = 2,	//  0,  0,  1
	MOVEMENT_LEFT = 4,		// -1,  0, -0
	MOVEMENT_RIGHT = 8,		//  1,  0,  0
	MOVEMENT_UP = 16,		//  0,  1, -0
	MOVEMENT_DOWN = 32		//  0, -1,  0
};

class Camera
{
public:
	// camera options
	struct CameraOptions {
		float MovementSpeed;
		float MouseSensitivity;
		float FovY = 90.0f;
		float ZNear = 0.1f, ZFar = 500.0f;

		float Mass = 80.0f;
		float Resistance = 100.0f;
		float AirResistance = 50.0f;
	} Options;

private:
	static Camera* s_Camera;

	bool m_IsDirty = true;
	glm::mat4 m_ProjMatrix{ 1.0f };
	glm::mat4 m_ViewMatrix{ 1.0f };
	glm::mat4 m_VPMatrix{ 1.0f };
	Frustum m_Frustum;

	glm::vec3 m_Up;
	glm::vec3 m_Right;
	glm::vec3 m_WorldUp;
	// euler Angles
	float m_Yaw;
	float m_Pitch;

	void GenerateEverything();

	// constructor with vectors
	Camera(CameraOptions options, float aspectRatio, glm::vec3 position, glm::vec3 up, float yaw, float pitch);

public:
	// camera Attributes
	glm::vec3 Position;
	glm::vec3 Front; // READ-ONLY
	glm::vec3 Vel; // velocity

	GrapplingHook m_GrapplingHook;

	float AspectRatio;

	static void Initialize(CameraOptions options, float aspectRatio, glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f) {
		if (!s_Camera)
			s_Camera = new Camera(options, aspectRatio, position, up, yaw, pitch);
	}

	static void Initialize(CameraOptions options, float aspectRatio, float posX, float posY, float posZ, float upX = 0.0f, float upY = 1.0f, float upZ = 0.0f, float yaw = -90.0f, float pitch = 0.0f) {
		if (!s_Camera)
			s_Camera = new Camera(options, aspectRatio, { posX, posY, posZ }, { upX, upY, upZ }, yaw, pitch);
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
