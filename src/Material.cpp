#include "pch.h"

#include "Material.h"
#include "UtilityMacros.h"
#include <glad/glad.h>
#pragma warning(disable : 26495) // Variable in union is uninitialized

EmptyMaterial::EmptyMaterial(const std::vector<Texture> &texs) noexcept
    : m_Textures(texs) {}

EmptyMaterial::EmptyMaterial(std::vector<Texture> &&texs) noexcept
    : m_Textures(std::move(texs)), m_UseTextures(true) {}

EmptyMaterial::EmptyMaterial(
    std::vector<std::shared_future<LoadingTexture *>> &&loadingTexs) noexcept
    : m_Textures(loadingTexs.size()), m_LoadingTextures(std::move(loadingTexs)),
      m_UseTextures(true), m_OpenGLPrepared(false) {}

EmptyMaterial::EmptyMaterial(const glm::vec3 &diff,
                             const glm::vec3 &spec) noexcept
    : m_Diffuse(diff), m_Specular(spec) {}

EmptyMaterial::EmptyMaterial(EmptyMaterial &&other) noexcept {
  m_UseTextures = other.m_UseTextures;
  m_OpenGLPrepared = other.m_OpenGLPrepared;

  other.m_OpenGLPrepared = true;

  if (m_UseTextures) {
    m_Textures = std::move(other.m_Textures);
    m_LoadingTextures = std::move(other.m_LoadingTextures);
  } else {
    m_Diffuse = std::move(other.m_Diffuse);
    m_Specular = std::move(other.m_Specular);
  }
}
void EmptyMaterial::swap(EmptyMaterial &other) noexcept {
  SWAP(m_UseTextures);
  SWAP(m_OpenGLPrepared);

  if (m_UseTextures) {
    SWAP(m_Textures);
    SWAP(m_LoadingTextures);
  } else {
    SWAP(m_Diffuse);
    SWAP(m_Specular);
  }
}

void EmptyMaterial::SetColors(const glm::vec3 &diff, const glm::vec3 &spec) {
  m_Diffuse = diff;
  m_Specular = spec;
}

void EmptyMaterial::LoadTextures(bool deleteData) {
  if (m_OpenGLPrepared || !m_UseTextures)
    return;

  m_OpenGLPrepared = true;

  m_Textures.reserve(m_LoadingTextures.size());
  for (auto &lt : m_LoadingTextures) {
    auto &s = lt.get();
    m_Textures.push_back(s->Finish());
    if (deleteData)
      delete s;
  }

  if (deleteData)
    m_LoadingTextures.clear();
}

void EmptyMaterial::Load(Shader &shader) const {
  shader.Use();
  shader.SetBool("material.diffspecTex", m_UseTextures);

  if (!m_UseTextures) {
    shader.SetVec3("material.diff0", m_Diffuse);
    shader.SetVec3("material.spec0", m_Specular);

    shader.SetBool("material.normalMapping", false);
    return;
  }

  unsigned int diffuseNr = 0, specularNr = 0, normalNr = 0, heightNr = 0;

  for (size_t i = 0; i < m_Textures.size(); i++) {
    std::string number;
    TextureType type = m_Textures[i].Type;

    switch (type) {
    case TextureType::diffuse:
      number = std::to_string(diffuseNr++);
      break;
    case TextureType::specular:
      number = std::to_string(specularNr++); // transfer unsigned int to stream
      break;
    case TextureType::normal:
      number = std::to_string(normalNr++); // transfer unsigned int to stream
      break;
    case TextureType::height:
      number = std::to_string(heightNr++); // transfer unsigned int to stream
      break;
    default:
      throw "ERROR: Texture type is not defined\n";
    }

    shader.SetInt((names[static_cast<int>(type)] + number).c_str(),
                  static_cast<int>(i));
    glBindTextureUnit(static_cast<GLuint>(i), m_Textures[i].ID);
  }

  shader.SetBool("Material.normalMapping", static_cast<bool>(normalNr));
}

EmptyMaterial::~EmptyMaterial() noexcept {
  if (m_UseTextures) {
    for (auto &t : m_Textures)
      glDeleteTextures(1, &t.ID);
  }
  m_OpenGLPrepared = false;
}

#pragma warning(default : 26495)
