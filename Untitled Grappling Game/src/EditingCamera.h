#pragma once
#include "Camera.h"
class EditingCamera :
	public Camera
{
public:
	EditingCamera(CameraOptions options, float aspectRatio, glm::vec3 position, glm::vec3 up, float yaw, float pitch);

	void UpdatePosition(float deltaTime) override;

	void ProcessKeyboard(Camera_Movement direction, float deltaTime) override;
};

