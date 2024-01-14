#pragma once
#include "AABB.h"

#define SLIPPERYNESS 10.0f
#define GRAVITY 1.0f

class Camera {
public:
  // camera options
  struct CameraOptions {
    glm::vec3 Size;
    float MovementSpeed;
    float MouseSensitivity;
    float Resistance = 100.0f;
    float FovY = 90.0f;
    float ZNear = 0.1f, ZFar = 500.0f;
  } Options;

  enum Camera_Movement : unsigned int {
    MOVEMENT_NONE = 0,
    // positive x is to the right:
    MOVEMENT_FORWARD = 1,  //  0,  0, -1
    MOVEMENT_BACKWARD = 2, //  0,  0,  1
    MOVEMENT_LEFT = 4,     // -1,  0, -0
    MOVEMENT_RIGHT = 8,    //  1,  0,  0
    MOVEMENT_UP = 16,      //  0,  1, -0
    MOVEMENT_DOWN = 32     //  0, -1,  0
  };

  enum Camera_Type { CAMERA_TYPE_GRAPPLING = 1, CAMERA_TYPE_EDITING = 2 };

private:
  glm::mat4 m_ModelMatrix{1.0f};
  glm::mat4 m_ProjMatrix{1.0f};
  glm::mat4 m_ViewMatrix{1.0f};
  glm::mat4 m_VPMatrix{1.0f};
  Frustum m_Frustum;

  // euler Angles
  float m_Yaw;
  float m_Pitch;
  bool m_IsDirty = true;

  void GenerateEverything();

protected:
  bool m_CanJump = true;

  glm::vec3 m_WorldUp;
  glm::vec3 m_Up;    // READ-ONLY
  glm::vec3 m_Right; // READ-ONLY

public:
  // constructor with vectors
  Camera(Camera_Type cameraType, CameraOptions options, float aspectRatio,
         glm::vec3 position, glm::vec3 up, float yaw, float pitch);
  virtual ~Camera() {}

  Camera(const Camera &other) = default;
  Camera &operator=(const Camera &other) = default;
  Camera &operator=(Camera &&other) = default;

  // camera Attributes
  glm::vec3 Position;
  glm::vec3 Front; // READ-ONLY
  glm::vec3 Vel;   // velocity

  Camera_Type CameraType;

  float AspectRatio;

  glm::vec2 *GetEulerAngles() { return (glm::vec2 *)&m_Yaw; }

  const glm::mat4 &GetModelMatrix(); // Model of the player character

  // returns the view matrix calculated using Euler Angles and the LookAt Matrix
  const glm::mat4 &GetViewMatrix();

  const glm::mat4 &GetProjMatrix();

  const glm::mat4 &GetVPMatrix();

  const Frustum &GetFrustum();

  // processes input received from any keyboard-like input system. Accepts input
  // parameter in the form of camera defined ENUM (to abstract it from windowing
  // systems) Default implementation does left/right and forward/back movement
  virtual void ProcessKeyboard(Camera_Movement direction, float deltaTime);

  // processes input received from a mouse input system. Expects the offset
  // value in both the x and y direction.
  void ProcessMouseMovement(float xOffset, float yOffset,
                            bool constrainPitch = true);

  // processes input received from a mouse scroll-wheel event. Only requires
  // input on the vertical wheel-axis
  void ProcessMouseScroll([[maybe_unused]] float xOffset, float yOffset);

  // calculates the front vector from the Camera's (updated) Euler Angles
  void UpdateCameraVectors(float deltaTime);

  virtual void Reset();

protected:
  // Gets called by UpdateCameraVectors
  virtual void UpdatePosition(float deltaTime) = 0;
};
