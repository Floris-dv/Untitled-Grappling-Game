#pragma once
#include "Bloom.h"
#include "DataBuffers.h"
#include "Game.h"
#include "PostprocessingLayer.h"
#include "Shader.h"
#include "UtilityMacros.h"
#include "Window.h"

class Application {
private:
  Shader m_NormalShader;
  Shader m_InstancedShader;
  Shader m_TextureShader;

  GLuint m_MainFramebuffer;
  GLuint m_MainRenderbuffer;

  GLuint m_MainTexture;

  BloomSettings m_BloomSettings;

  VertexArray m_ScreenVAO;
  VertexBuffer m_ScreenVBO;

  UniformBuffer m_LightsUBO;

  PostprocessingLayer m_PostprocessingLayer;

  union {
    Game m_Game;
  };

  Window m_Window;

  void Initialize();
  void FillMainRenderBufferAndTexture();

  void StartFrame();
  void EndFrame();

  // Does postprocessing (including bloom), renders the main texture to the
  // screen, and swaps the buffers
  void PostProcess();

  void WindowResizeCallback(uint32_t width, uint32_t height);

public:
  Application();

  DELETE_COPY_CONSTRUCTOR(Application)

  void Mainloop();

  void Cleanup();

  ~Application() { Cleanup(); }
};
