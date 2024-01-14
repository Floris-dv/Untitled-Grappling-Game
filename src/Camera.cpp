#include "pch.h"

#include "Camera.h"
Camera::Camera(Camera_Type cameraType, CameraOptions options, float aspectRatio,
               glm::vec3 Position, glm::vec3 Up, float yaw, float pitch)
    : Options(options), m_Yaw(yaw), m_Pitch(pitch), m_WorldUp(Up), m_Up(Up),
      Position(Position), Front(glm::vec3(0.0f, 0.0f, -1.0f)),
      Vel(glm::vec3(0.0f)), CameraType(cameraType), AspectRatio(aspectRatio) {
  m_Right = glm::normalize(glm::cross(Front, m_WorldUp));

  Reset();
}

void Camera::GenerateEverything() {
  m_ViewMatrix = glm::lookAt(Position, Position + Front, m_Up);
  m_ProjMatrix = glm::perspective(glm::radians(Options.FovY), AspectRatio,
                                  Options.ZNear, Options.ZFar);
  m_VPMatrix = m_ProjMatrix * m_ViewMatrix;

  m_ModelMatrix = glm::translate(glm::mat4{1.0f}, Position) *
                  glm::scale(glm::mat4{1.0f}, Options.Size);

  const float halfVSide = Options.ZFar * tanf(glm::radians(Options.FovY) * .5f);
  const float halfHSide = halfVSide * AspectRatio;
  const glm::vec3 frontMultFar = Options.ZFar * Front;

  m_Frustum.nearFace = {Position + Options.ZNear * Front, Front};
  m_Frustum.farFace = {Position + frontMultFar, -Front};

  m_Frustum.rightFace = {Position,
                         glm::cross(m_Up, frontMultFar + m_Right * halfHSide)};
  m_Frustum.leftFace = {Position,
                        glm::cross(frontMultFar - m_Right * halfHSide, m_Up)};
  m_Frustum.bottomFace = {Position,
                          glm::cross(m_Right, frontMultFar - m_Up * halfVSide)};
  m_Frustum.topFace = {Position,
                       glm::cross(frontMultFar + m_Up * halfVSide, m_Right)};

  m_IsDirty = false;
}

const glm::mat4 &Camera::GetModelMatrix() {
  if (m_IsDirty)
    GenerateEverything();
  return m_ModelMatrix;
}

const glm::mat4 &Camera::GetViewMatrix() {
  if (m_IsDirty)
    GenerateEverything();
  return m_ViewMatrix;
}

const glm::mat4 &Camera::GetProjMatrix() {
  if (m_IsDirty)
    GenerateEverything();
  return m_ProjMatrix;
}

const glm::mat4 &Camera::GetVPMatrix() {
  if (m_IsDirty)
    GenerateEverything();
  return m_VPMatrix;
}

void Camera::ProcessKeyboard(Camera_Movement direction, float deltaTime) {
  const float velocity = Options.MovementSpeed * deltaTime;

  if (direction & MOVEMENT_FORWARD)
    Vel += glm::normalize(glm::vec3(Front.x, 0.0f, Front.z)) * velocity;
  if (direction & MOVEMENT_BACKWARD)
    Vel -= glm::normalize(glm::vec3(Front.x, 0.0f, Front.z)) * velocity;
  if (direction & MOVEMENT_RIGHT)
    Vel += glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z)) * velocity;
  if (direction & MOVEMENT_LEFT)
    Vel -= glm::normalize(glm::vec3(m_Right.x, 0.0f, m_Right.z)) * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset,
                                  bool constrainPitch) {
  xoffset *= Options.MouseSensitivity;
  yoffset *= Options.MouseSensitivity;

  m_Yaw += xoffset;
  m_Pitch += yoffset;

  // make sure that when m_Pitch is out of bounds, screen doesn't get flipped
  if (constrainPitch) {
    if (m_Pitch > 89.0f)
      m_Pitch = 89.0f;
    if (m_Pitch < -89.0f)
      m_Pitch = -89.0f;
  }
}

void Camera::ProcessMouseScroll([[maybe_unused]] float xoffset, float yoffset) {
  Options.FovY -= yoffset;
  if (Options.FovY < 1.0f)
    Options.FovY = 1.0f;

  if (Options.FovY > 180.0f)
    Options.FovY = 180.0f;
}

void Camera::UpdateCameraVectors(float deltaTime) {
  UpdatePosition(deltaTime);
  m_IsDirty = true;

  // calculate the new Front vector
  Front.x = glm::cos(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch));
  Front.y = glm::sin(glm::radians(m_Pitch));
  Front.z = glm::sin(glm::radians(m_Yaw)) * glm::cos(glm::radians(m_Pitch));
  Front = glm::normalize(Front);
  // also re-calculate the Right and Up vector
  m_Right = glm::normalize(glm::cross(
      Front, m_WorldUp)); // normalize the vectors, because their length gets
                          // closer to 0 the more you look Up or down which
                          // results in slower movement.
  m_Up = glm::normalize(glm::cross(m_Right, Front));
}

void Camera::Reset() {
  Vel = glm::vec3(0.0f);
  Position = glm::vec3(0.0f);
  Front = glm::vec3(1.0f, 0.0f, 0.0f);
  m_Yaw = 0.0f;
  m_Pitch = 0.0f;
}

const Frustum &Camera::GetFrustum() {
  if (m_IsDirty)
    GenerateEverything();
  return m_Frustum;
}
