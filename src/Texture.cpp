#include "pch.h"

#include "Texture.h"
#include "Timer.h"

#include <glad/glad.h>

#include <stb_image.h>

#include <DebugBreak.h>

using namespace std::literals::chrono_literals;

std::unordered_map<std::filesystem::path, Texture> loadedPaths;

std::mutex loadedPathsMtx;

LoadingTexture::LoadingTexture(const std::filesystem::path &fname,
                               TextureType type)
    : Format(0), InternalFormat(0), Path(fname), Type(type) {
  if (loadedPaths.contains(Path)) {
    Height = -1;
    Width = -1;
    Data = nullptr;
    return;
  }
  {
    std::scoped_lock lg{loadedPathsMtx};
    loadedPaths[Path] = Texture();
  }

  int channels;
  stbi_uc *temp;
  if constexpr (std::is_same_v<std::filesystem::path::value_type, char>)
    temp = stbi_load((char *)Path.c_str(), &Width, &Height, &channels, 0);
  else
    temp = stbi_load(Path.string().c_str(), &Width, &Height, &channels, 0);
  switch (channels) {
  case 1:
    Format = GL_RED;
    InternalFormat = GL_R8;
    break;
  case 3:
    Format = GL_RGB;
    InternalFormat = GL_RGB8;
    break;
  case 4:
    Format = GL_RGBA;
    InternalFormat = GL_RGBA8;
    break;
  default:
    NG_ERROR("Texture in file {} is weird: it has {} amount of channels, "
             "instead of the normal 1, 3, or 4. Error: {}",
             fname.string(), channels, stbi_failure_reason());
    break;
  }

  Data = temp;
}
LoadingTexture::~LoadingTexture() { stbi_image_free(Data); }

Texture LoadingTexture::Finish(TextureType FillInType, bool deleteAfter) {
  if (Data == nullptr) {
    Texture t = loadedPaths[Path];
    t.Type = FillInType == TextureType::unknown ? Type : FillInType;
    return t;
  }

  Timer t("OpenGL loading of " + Path.string());
  read();

  unsigned int textureID;

  glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

  glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER,
                      GL_LINEAR_MIPMAP_LINEAR);
  glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTextureStorage2D(textureID, 1, InternalFormat, Width, Height);
  glTextureSubImage2D(textureID, 0, 0, 0, Width, Height, Format,
                      GL_UNSIGNED_BYTE, Data);

  glGenerateTextureMipmap(textureID);

  // set the texture wrapping/filtering options (on the currently bound texture
  // object)

  if (deleteAfter) {
    stbi_image_free(Data);
    Data = nullptr;
  }

  // no need for mutexes, as this is single-threaded
  loadedPaths[Path] = Texture(
      textureID, FillInType == TextureType::unknown ? Type : FillInType, Path);
  return loadedPaths[Path];
}

LoadingTextures::LoadingTextures(std::vector<std::string> paths) {
  Futures.reserve(paths.size());
  Textures.reserve(paths.size());
  for (std::string &path : paths) {
    Futures.push_back(std::async(
        std::launch::async,
        [](std::string fname) {
          // if it's not heap allocated, the function scope will delete it's
          // data, and then it's useless
          LoadingTexture *t = new LoadingTexture(fname);

          return t;
        },
        path));
  }
}

bool LoadingTextures::Is_Finished() {
  for (const auto &f : Futures) {
    if (f.wait_for(10us) != std::future_status::ready)
      return false;
  }
  return true;
}

std::vector<Texture> LoadingTextures::GetTextures(TextureType type) {
  for (auto &f : Futures) {
    Textures.push_back(f.get()->Finish(type));
  }

  Futures.clear();

  return Textures;
}

Texture LoadingTextures::GenerateCubeMap() {
  assert(Futures.size() == 6);
  Texture texture;
  texture.Type = TextureType::cubeMap;

  glGenTextures(1, &texture.ID);
  glBindTexture(GL_TEXTURE_CUBE_MAP, texture.ID);

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  for (size_t i = 0; i < 6; i++) {
    try {
      LoadingTexture *t = Futures[i].get();
      if (t == nullptr) {
        NG_ERROR("Future got no result!");
        continue;
      }

      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + (unsigned int)i, 0,
                   static_cast<int>(t->Format), t->Width, t->Height, 0, GL_RGB,
                   GL_UNSIGNED_BYTE, t->read());

      if (i == 0)
        texture.Path = t->Path;

      delete t; // the deletor calls stbi_image_free
    } catch (const std::exception &e) {
      NG_ERROR(e.what());
    }
  }

  Futures.clear();

  return texture;
}

std::future<LoadingTexture *>
StartLoadingTexture(const std::filesystem::path &path, TextureType type) {
  // Path needs to be a copy, else classic 'reference to something that doesn't
  // exist anymore'
  return std::async(
      std::launch::async,
      [](const std::filesystem::path &path, TextureType type) {
        // if it's not heap allocated, LoadingTexture will delete it's data, and
        // then it's useless
        try {
          LoadingTexture *t = new LoadingTexture(path, type);
          return t;
        } catch (const std::string &e) {
          NG_ERROR("{}", e);
        } catch (const std::exception &e) {
          NG_ERROR("{}", e.what());
        }
        return (
            LoadingTexture *)nullptr; // To help the type deducing of std::async
      },
      path, type);
}
