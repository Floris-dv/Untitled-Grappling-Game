#pragma once
#include "Camera.h"
#include "Endscreen.h"
#include "Level.h"
#include "LevelTimer.h"
#include "Shader.h"
#include "Window.h"

class Game {
public:
  enum class GameState { Playing, Editing, Endscreen };

protected:
  std::unique_ptr<Camera> m_Camera;
  std::shared_ptr<Shader> m_InstancedShader;
  std::shared_ptr<Shader> m_NormalShader;
  LevelTimer m_Timer;
  GameState m_State;
  Level m_Level;

  Endscreen m_Endscreen;

  Window *m_Window;

  double m_StartTime;

  float m_StartFrame;

  UniformBuffer m_MatrixUBO;

  int m_LevelNr = 0;
  size_t m_BlockEditingIndex = 0;

  static constexpr int NR_LEVELS = 4;

  glm::vec3 m_SpherePos;        // DEBUG
  Object<MinimalVertex> Sphere; // DEBUG

  Camera::Camera_Movement GetMovement();

public:
  struct GameSettings {
    Camera::CameraOptions *CameraOptions;
  } Settings;

  Game(std::unique_ptr<Camera> &&camera, Level &&startLevel,
       std::shared_ptr<Shader> instancedShader,
       std::shared_ptr<Shader> normalShader, Window *window);

  // Update's the game, like physics
  void Update();

  // Renders the game
  void Render();

  // Does things like postprocessing, and depth shenanigans
  void Finalize();

  void SetLevel(int level);

  void Reset() { m_Camera->Reset(); }

  GameState GetState() { return m_State; }
};
