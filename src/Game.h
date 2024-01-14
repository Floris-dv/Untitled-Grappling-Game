#pragma once
#include "AudioSystem.h"
#include "Bloom.h"
#include "Camera.h"
#include "Endscreen.h"
#include "Level.h"
#include "LevelTimer.h"
#include "PostprocessingLayer.h"
#include "Shader.h"
#include "Vertex.h"
#include "Window.h"
#include <glad/glad.h>

class Game {
public:
  enum class GameState : uint8_t {
    // format: least significant bit decides if should be paused or not
    Endscreen = 0b0000'0001,
    Playing = 0b0000'0010,
    PlayingPaused = 0b0000'0011,
    Editing = 0b0000'0100,
    EditingPaused = 0b0000'0101,
  };
  struct GameSettings {
    BloomSettings BloomOptions;
  };
  Camera::CameraOptions *CameraOptions;

protected:
  // TODO: maybe make this class own the shaders
  Shader *m_InstancedShader = nullptr;
  Shader *m_NormalShader = nullptr;
  Shader *m_UIShader = nullptr;

  AudioSystem m_AudioSystem;

  float m_CrosshairSize = 0.1f;
  std::variant<Texture, LoadingTexture::Future> m_CrosshairTexture;
  PostprocessingLayer m_PostprocessingLayer;

  LevelTimer m_Timer;
  GameState m_State = GameState::PlayingPaused;
  GameSettings m_Settings;

  Window *m_Window = nullptr;

  VertexArray *m_ScreenVAO;

  double m_StartTime = 0.0;

  float m_StartFrame = 0.f;

  UniformBuffer m_MatrixUBO;

  std::unique_ptr<Camera> m_Camera;
  Level m_Level;
  Endscreen m_Endscreen;
  Mesh<SimpleVertex> m_Rope;

  int m_LevelNr = 1;
  size_t m_BlockEditingIndex = 0;

  static constexpr int NR_LEVELS = 4;

  Camera::Camera_Movement GetMovement();

public:
  void InitializeCallbacks(); // Needs to be called after construction

  Game(const std::string &startLevel, Shader *instancedShader,
       Shader *normalShader, Shader *textureShader, VertexArray *screenVAO,
       Window *window);

  DELETE_COPY_CONSTRUCTOR(Game)

  ~Game() {
    if (!m_CrosshairTexture.valueless_by_exception() &&
        m_CrosshairTexture.index() == 0)
      glDeleteTextures(1, &std::get<Texture>(m_CrosshairTexture).ID);
  }

  // Gets called first, like physics
  void Update();

  // Renders the game
  void Render();

  // Part of the update phase that is after the Render phase
  void Finalize();

  void Postprocess(unsigned int mainTextureID);

  // Gets called after postprocessing, Depth testing is disabled
  void DrawUI();

  void SetLevel(int level);
  void SetOptions(const GameSettings &settings);

  void Reset() { m_Camera->Reset(); }

  void WindowResizeCallback(uint32_t width, uint32_t height);

  template <typename CameraType, typename... Args>
    requires std::is_base_of_v<Camera, CameraType>
  void SetCamera(Args... args) {
    m_Camera = std::make_unique<CameraType>(args...);
  }

  GameState GetState() { return m_State; }
};
