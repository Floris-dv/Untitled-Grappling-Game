#pragma once
#include "Camera.h"
#include "DataBuffers.h"
#include "Material.h"

class GrapplingCamera : public Camera {
private:
  float m_AirResistance;
  float m_JumpHeight;

  // Grappling hook parameters
  glm::vec3 m_GrapplingPoint{0.0f};
  float m_RopeLength = 0.0f;

  float m_RopeStiffness = 1.0f;

  bool m_GrapplingHookActive = false;
  inline static VertexBuffer s_RopeBuffer;
  inline static Material s_RopeMaterial;

public:
  GrapplingCamera(float airResistance, float jumpHeight, CameraOptions options,
                  float aspectRatio, glm::vec3 position, glm::vec3 up,
                  float yaw, float pitch);

  bool m_CanJump = true;

#define PHYSICSOFFSET 0.5f
  glm::vec3 PhysicsPosition; // Is PHYSICSOFFSET units below Position

  void UpdatePosition(float deltaTime) override;

  void LaunchAt(const glm::vec3 &point);
  void Release();
  bool IsGrappling() { return m_GrapplingHookActive; }

  glm::vec3 GrapplingPosition() { return m_GrapplingPoint; }

  void ProcessKeyboard(Camera_Movement direction, float deltaTime) override;

  void Reset() override;

private:
  // Output should be seen as a force vector
  glm::vec3 CalculateRopeForce();
};
