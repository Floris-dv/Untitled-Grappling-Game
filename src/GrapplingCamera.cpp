#include "pch.h"

#include "GrapplingCamera.h"
#include <imgui/imgui.h>

GrapplingCamera::GrapplingCamera(float airResistance, float jumpHeight,
                                 CameraOptions options, float aspectRatio,
                                 glm::vec3 position, glm::vec3 up, float yaw,
                                 float pitch)
    : Camera(CAMERA_TYPE_GRAPPLING, options, aspectRatio, position, up, yaw,
             pitch),
      m_AirResistance(airResistance),
      m_JumpHeight(jumpHeight), PhysicsPosition{position.x,
                                                position.y - PHYSICSOFFSET,
                                                position.z} {}

void GrapplingCamera::UpdatePosition(float deltaTime) {
  if (IsGrappling()) {
    Vel.x *= glm::pow(0.97f, deltaTime * m_AirResistance);
    Vel.z *= glm::pow(0.97f, deltaTime * m_AirResistance);
  } else {
    Vel.x *= glm::pow(0.97f, deltaTime * Options.Resistance);
    Vel.z *= glm::pow(0.97f, deltaTime * Options.Resistance);
  }

  Vel.y -= GRAVITY * deltaTime;
  Vel += CalculateRopeForce() * deltaTime;

  Position += Vel * deltaTime * 60.0f;

  PhysicsPosition = {Position.x, Position.y - PHYSICSOFFSET, Position.z};
}

void GrapplingCamera::LaunchAt(const glm::vec3 &point) {
  if (IsGrappling())
    return;

  m_GrapplingPoint = point;
  m_GrapplingHookActive = true;
  m_RopeLength = 10000.0f;

  Options.MovementSpeed *= 0.1f;
}

void GrapplingCamera::Release() {
  if (!IsGrappling())
    return;

  m_GrapplingHookActive = false;

  Options.MovementSpeed *= 10.0f;
}

void GrapplingCamera::ProcessKeyboard(Camera_Movement direction,
                                      float deltaTime) {
  Camera::ProcessKeyboard(direction, deltaTime);
  ImGui::Checkbox("Can jump", &m_CanJump);
  ImGui::DragFloat3("Pos", glm::value_ptr(Position));
  ImGui::DragFloat3("Vel", glm::value_ptr(Vel));
  if (direction & MOVEMENT_UP && m_CanJump) {
    Vel += m_WorldUp * m_JumpHeight;
    m_CanJump = false;
  }
}

void GrapplingCamera::Reset() {
  Camera::Reset();
  Release();
}

glm::vec3 GrapplingCamera::CalculateRopeForce() {
  if (!IsGrappling())
    return glm::vec3();

  glm::vec3 force{};
  float distance = glm::distance(Position, m_GrapplingPoint);

  if (distance > m_RopeLength) {
    force -= glm::vec3(distance - m_RopeLength * m_RopeStiffness) *
             (Position - m_GrapplingPoint);
    m_RopeLength = distance;
  } else
    m_RopeLength -= (m_RopeLength - distance) * 0.9f + distance * 0.01f;

  return force;
}
