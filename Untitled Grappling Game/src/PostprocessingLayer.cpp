#include "pch.h"

#include "PostprocessingLayer.h"
#include <glm/gtc/integer.hpp>

PostprocessingLayer::PostprocessingLayer(uint32_t screenWidth,
                                         uint32_t screenHeight,
                                         VertexArray *screenVAO)
    : m_ScreenWidth(screenWidth), m_ScreenHeight(screenHeight), m_ScreenVAO(screenVAO) {
  SetBloomSettings({});
  SetupBloomTextures();

  m_MainShader =
      Shader("src/Shaders/FrameBuffer.vert", "src/Shaders/PostProcessing.frag");

  m_BloomShader = Shader("src/Shaders/Bloom.comp");
}

PostprocessingLayer::~PostprocessingLayer() {
  glDeleteTextures(3, m_BloomRenderTextures);
}

void PostprocessingLayer::Postprocess(unsigned int mainTexture) {
  RenderBloom(m_BloomSettings, m_BloomShader, mainTexture,
              m_BloomRenderTextures);

  m_MainShader.Use();
  m_MainShader.SetBool("bloom", m_BloomSettings.Toggled);
  m_MainShader.SetInt("screen", 0);
  m_MainShader.SetInt("bloomBlur", 1);

  if (m_BloomSettings.Toggled) {
    glBindImageTexture(1, m_BloomRenderTextures[2], 0, GL_FALSE, 0,
                       GL_READ_ONLY, GL_RGBA16F);
  }

  glDisable(GL_DEPTH_TEST);

  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  glClear(GL_COLOR_BUFFER_BIT);

  glBindTextureUnit(0, mainTexture);

  m_ScreenVAO->Bind();
  glDrawArrays(GL_TRIANGLES, 0, 6);
  m_ScreenVAO->UnBind();
}

void PostprocessingLayer::OnWindowResize(uint32_t newWidth,
                                         uint32_t newHeight) {

  m_ScreenWidth = newWidth;
  m_ScreenHeight = newHeight;

  glDeleteTextures(3, m_BloomRenderTextures);

  SetupBloomTextures();
}

void PostprocessingLayer::SetBloomSettings(const BloomSettings &settings) {
  m_BloomSettings = settings;
  // bloom:
  m_BloomSettings.TexSize = glm::ivec2(m_ScreenWidth / 2, m_ScreenHeight / 2);
  // Make Bloom Texture Size a multiple of 16
  m_BloomSettings.TexSize += glm::ivec2(16) - (m_BloomSettings.TexSize % 16);
  m_BloomSettings.NrMips =
      (int)glm::log2(glm::min(m_ScreenWidth, m_ScreenHeight)) -
      4; // don't want mips of like 1x1
}

void PostprocessingLayer::SetupBloomTextures() {
  glCreateTextures(GL_TEXTURE_2D, 3, m_BloomRenderTextures);

  for (unsigned int i = 0; i < 3; i++) {
    glTextureStorage2D(m_BloomRenderTextures[i], m_BloomSettings.NrMips,
                       GL_RGBA16F, m_BloomSettings.TexSize.x,
                       m_BloomSettings.TexSize.y);

    glGenerateTextureMipmap(m_BloomRenderTextures[i]);
    glTextureParameteri(m_BloomRenderTextures[i], GL_TEXTURE_MIN_FILTER,
                        GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(m_BloomRenderTextures[i], GL_TEXTURE_MAG_FILTER,
                        GL_LINEAR);
    glTextureParameteri(m_BloomRenderTextures[i], GL_TEXTURE_WRAP_S,
                        GL_CLAMP_TO_EDGE);
    glTextureParameteri(m_BloomRenderTextures[i], GL_TEXTURE_WRAP_T,
                        GL_CLAMP_TO_EDGE);
  }
}
