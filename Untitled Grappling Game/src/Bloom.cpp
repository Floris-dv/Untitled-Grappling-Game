#include "pch.h"

#include "Bloom.h"

void RenderBloom(const BloomSettings &settings, Shader &bloomShader,
                 unsigned int texture, unsigned int renderTextures[3]) {
  if (!settings.Toggled)
    return;

  bloomShader.Use();
  bloomShader.SetInt("o_Image", 0);
  glBindImageTexture(0, renderTextures[0], 0, GL_FALSE, 0, GL_WRITE_ONLY,
                     GL_RGBA16F);

  bloomShader.SetVec4("Params", settings.Threshold,
                      settings.Threshold - settings.Knee, settings.Knee * 2,
                      .25f / settings.Knee);
  bloomShader.SetIVec2("LodAndMode", 0, (int)BloomMode::PreFilter);

  glBindTextureUnit(0, texture);
  bloomShader.SetInt("u_Texture", 0);

  glDispatchCompute((GLuint)glm::ceil((float)settings.TexSize.x / 16.0f),
                    (GLuint)glm::ceil((float)settings.TexSize.y / 16.0f), 1);

  // Downsampling:
  for (int currentMip = 1; currentMip < settings.NrMips; currentMip++) {
    glm::ivec2 mipSize = settings.TexSize >> currentMip;

    // Ping
    bloomShader.SetIVec2("LodAndMode", currentMip - 1,
                         (int)BloomMode::DownSample);

    glBindImageTexture(0, renderTextures[1], currentMip, GL_FALSE, 0,
                       GL_WRITE_ONLY, GL_RGBA16F);

    glBindTextureUnit(0, renderTextures[0]);
    glDispatchCompute((GLuint)ceil((float)mipSize.x / 16.0f),
                      (GLuint)ceil((float)mipSize.y / 16.0f), 1);

    // Pong
    bloomShader.SetIVec2("LodAndMode", currentMip, (int)BloomMode::DownSample);

    glBindImageTexture(0, renderTextures[0], currentMip, GL_FALSE, 0,
                       GL_WRITE_ONLY, GL_RGBA16F);

    glBindTextureUnit(0, renderTextures[1]);
    glDispatchCompute((GLuint)glm::ceil((float)mipSize.x / 16.0f),
                      (GLuint)glm::ceil((float)mipSize.y / 16.0f), 1);
  }

  // First upsample
  glBindImageTexture(0, renderTextures[2], settings.NrMips - 1, GL_FALSE, 0,
                     GL_WRITE_ONLY, GL_RGBA16F);

  bloomShader.SetIVec2("LodAndMode", settings.NrMips - 2,
                       (int)BloomMode::UpSample_First);

  glBindTextureUnit(0, renderTextures[0]);

  glm::ivec2 currentMipSize =
      glm::vec2(settings.TexSize) * (float)pow(2, -(settings.NrMips - 1));

  glDispatchCompute((GLuint)glm::ceil((float)currentMipSize.x / 16.0f),
                    (GLuint)glm::ceil((float)currentMipSize.y / 16.0f), 1);

  bloomShader.SetInt("u_BloomTexture", 1);

  // Rest of the upsamples
  for (int currentMip = settings.NrMips - 2; currentMip >= 0; currentMip--) {
    currentMipSize =
        glm::vec2(settings.TexSize) * (float)glm::pow(2, -currentMip);
    glBindImageTexture(0, renderTextures[2], currentMip, GL_FALSE, 0,
                       GL_WRITE_ONLY, GL_RGBA16F);

    bloomShader.SetIVec2("LodAndMode", currentMip, (int)BloomMode::UpSample);

    glBindTextureUnit(0, renderTextures[0]);

    glBindTextureUnit(1, renderTextures[2]);

    glDispatchCompute((GLuint)glm::ceil((float)currentMipSize.x / 16.0f),
                      (GLuint)glm::ceil((float)currentMipSize.y / 16.0f), 1);
  }
  return;
}
