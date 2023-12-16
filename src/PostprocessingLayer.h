#pragma once

#include "Bloom.h"
#include "DataBuffers.h"
#include "Shader.h"
class PostprocessingLayer {
private:
  VertexArray *m_ScreenVAO = nullptr;

  Shader m_MainShader;

  Shader m_BloomShader;
  BloomSettings m_BloomSettings;
  unsigned int m_BloomRenderTextures[3]{0};

  uint32_t m_ScreenWidth, m_ScreenHeight;

public:
  PostprocessingLayer() : m_ScreenWidth(0), m_ScreenHeight(0) {}
  PostprocessingLayer(uint32_t screenWidth, uint32_t screenHeight,
                      VertexArray *screenVAO);
  ~PostprocessingLayer();

  // Does postprocessing, and render mainTexture to the screen (framebuffer 0)
  void Postprocess(unsigned int mainTexture);

  void OnWindowResize(uint32_t newWidth, uint32_t newHeight);

public:
  void SetBloomSettings(const BloomSettings &settings);

private:
  void SetupBloomTextures();
};
