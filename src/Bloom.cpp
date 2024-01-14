#include "pch.h"

#include "Bloom.h"

static GLuint SizeToGroupSize(int size) {
  return static_cast<GLuint>(glm::ceil(static_cast<float>(size) / 16.0f));
};

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
  bloomShader.SetIVec2("LodAndMode", 0, static_cast<int>(BloomMode::PreFilter));

  glBindTextureUnit(0, texture);
  bloomShader.SetInt("u_Texture", 0);

  glDispatchCompute(SizeToGroupSize(settings.TexSize.x),
                    SizeToGroupSize(settings.TexSize.y), 1);

  // Downsampling:
  for (int currentMip = 1; currentMip < settings.NrMips; currentMip++) {
    glm::ivec2 mipSize = settings.TexSize >> currentMip;

    // Ping
    bloomShader.SetIVec2("LodAndMode", currentMip - 1,
                         static_cast<int>(BloomMode::DownSample));

    glBindImageTexture(0, renderTextures[1], currentMip, GL_FALSE, 0,
                       GL_WRITE_ONLY, GL_RGBA16F);

    glBindTextureUnit(0, renderTextures[0]);
    glDispatchCompute(SizeToGroupSize(mipSize.x), SizeToGroupSize(mipSize.y),
                      1);

    // Pong
    bloomShader.SetIVec2("LodAndMode", currentMip,
                         static_cast<int>(BloomMode::DownSample));

    glBindImageTexture(0, renderTextures[0], currentMip, GL_FALSE, 0,
                       GL_WRITE_ONLY, GL_RGBA16F);

    glBindTextureUnit(0, renderTextures[1]);
    glDispatchCompute(SizeToGroupSize(mipSize.x), SizeToGroupSize(mipSize.y),
                      1);
  }

  // First upsample
  glBindImageTexture(0, renderTextures[2], settings.NrMips - 1, GL_FALSE, 0,
                     GL_WRITE_ONLY, GL_RGBA16F);

  bloomShader.SetIVec2("LodAndMode", settings.NrMips - 2,
                       static_cast<int>(BloomMode::UpSample_First));

  glBindTextureUnit(0, renderTextures[0]);

  glm::ivec2 currentMipSize =
      glm::vec2(settings.TexSize) *
      static_cast<float>(pow(2, -(settings.NrMips - 1)));

  glDispatchCompute(SizeToGroupSize(currentMipSize.x),
                    SizeToGroupSize(currentMipSize.y), 1);

  bloomShader.SetInt("u_BloomTexture", 1);

  // Rest of the upsamples
  for (int currentMip = settings.NrMips - 2; currentMip >= 0; currentMip--) {
    currentMipSize = glm::vec2(settings.TexSize) *
                     static_cast<float>(glm::pow(2, -currentMip));
    glBindImageTexture(0, renderTextures[2], currentMip, GL_FALSE, 0,
                       GL_WRITE_ONLY, GL_RGBA16F);

    bloomShader.SetIVec2("LodAndMode", currentMip,
                         static_cast<int>(BloomMode::UpSample));

    glBindTextureUnit(0, renderTextures[0]);

    glBindTextureUnit(1, renderTextures[2]);

    glDispatchCompute(SizeToGroupSize(currentMipSize.x),
                      SizeToGroupSize(currentMipSize.y), 1);
  }
}
