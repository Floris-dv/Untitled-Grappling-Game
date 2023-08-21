#include "pch.h"

#include "EditingCamera.h"

EditingCamera::EditingCamera(CameraOptions options, float aspectRatio,
                             glm::vec3 position, glm::vec3 up, float yaw,
                             float pitch)
    : Camera(CAMERA_TYPE_EDITING, options, aspectRatio, position, up, yaw,
             pitch) {}

void EditingCamera::UpdatePosition(float deltaTime) {
  Vel.x *= glm::pow(0.97f, deltaTime * Options.Resistance);
  Vel.y *= glm::pow(0.97f, deltaTime * Options.Resistance);
  Vel.z *= glm::pow(0.97f, deltaTime * Options.Resistance);

  Position += Vel * deltaTime * 60.0f;
}

void EditingCamera::ProcessKeyboard(Camera_Movement direction,
                                    float deltaTime) {
  Camera::ProcessKeyboard(direction, deltaTime);

  const float velocity = Options.MovementSpeed * deltaTime;

  if (direction & MOVEMENT_UP)
    Vel += m_WorldUp * velocity;
  if (direction & MOVEMENT_DOWN)
    Vel -= m_WorldUp * velocity;
}
