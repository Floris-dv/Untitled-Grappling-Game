#pragma once
// TODO: put this file inside PostprocessingLayer

#include "Shader.h"

enum class BloomMode { PreFilter = 0, DownSample, UpSample_First, UpSample };

struct BloomSettings {
  float Threshold;
  float Knee;

  glm::ivec2 TexSize;
  int NrMips;

  bool Toggled;
};

void RenderBloom(const BloomSettings &settings, Shader &bloomShader,
                 unsigned int texture, unsigned int renderTextures[3]);