#pragma once
#include "AudioSystem.h"
#include "Bloom.h"
#include "Camera.h"
#include "Endscreen.h"
#include "Level.h"
#include "LevelTimer.h"
#include "Shader.h"
#include "Window.h"
#include <optional>

class Game {
public:
  enum class GameState { Playing, Editing, Endscreen };

protected:
  // TODO: maybe make this class own the shaders
  Shader *m_InstancedShader = nullptr;
  Shader *m_NormalShader = nullptr;
  Shader *m_UIShader = nullptr;

  std::optional<AudioSystem> m_AudioSystem;

  float m_CrosshairSize = 0.1f;
  std::variant<Texture, LoadingTexture::Future> m_CrosshairTexture;
  LevelTimer m_Timer;
  GameState m_State = GameState::Playing;
  Level m_Level;

  Window *m_Window = nullptr;
  Endscreen m_Endscreen;

  VertexArray *m_ScreenVAO;

  double m_StartTime = 0.0;

  float m_StartFrame = 0.f;

  UniformBuffer m_MatrixUBO;

  std::unique_ptr<Camera> m_Camera;

  int m_LevelNr = 1;
  size_t m_BlockEditingIndex = 0;

  static constexpr int NR_LEVELS = 4;

  Camera::Camera_Movement GetMovement();

public:
  void InitializeCallbacks(); // Needs to be called after construction

  struct GameSettings {
    Camera::CameraOptions *CameraOptions;
    BloomSettings BloomSettings;
  } Settings;

  Game() : Settings({nullptr, BloomSettings{0.8f, 0.05f, {0, 0}, 0, false}}) {}

  Game(const std::string &startLevel, Shader *instancedShader,
       Shader *normalShader, Shader *textureShader, Window *window);

  DELETE_COPY_CONSTRUCTOR(Game)

  Game(Game &&other) noexcept { swap(other); }

  ~Game() {
    if (!m_CrosshairTexture.valueless_by_exception() &&
        m_CrosshairTexture.index() == 0)
      glDeleteTextures(1, &std::get<Texture>(m_CrosshairTexture).ID);
  }

  void swap(Game &other) noexcept;

  OVERLOAD_OPERATOR_RVALUE(Game)

  // Gets called first, like physics
  void Update();

  // Renders the game
  void Render();

  // Part of the update phase that is after the Render phase
  void Finalize();

  // Maybe Later?
  // void Postprocess(unsigned int mainTextureID);

  // Gets called after postprocessing, Depth testing is disabled
  void DrawUI(VertexArray *screenVAO);

  void SetLevel(int level);

  void Reset() { m_Camera->Reset(); }

  void WindowResizeCallback(uint32_t width, uint32_t height);

  template <typename CameraType, typename... Args>
    requires std::is_base_of_v<Camera, CameraType>
  void SetCamera(Args... args) {
    m_Camera = std::make_unique<CameraType>(args...);
  }

  GameState GetState() { return m_State; }
};

OVERLOAD_STD_SWAP(Game)
