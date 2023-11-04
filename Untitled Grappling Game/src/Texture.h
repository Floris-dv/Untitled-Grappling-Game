#pragma once
#include "Settings.h"
#include "UtilityMacros.h"

enum class TextureType {
  unknown = -1,
  diffuse = 0,
  specular,
  normal,
  height,
  cubeMap,
  count
};

static constexpr const char *const names[static_cast<int>(TextureType::count)] =
    {"material.diffuse", "material.specular", "material.normal",
     "material.height", "material.cubeMap"};

struct Texture {
  unsigned int ID;
  TextureType Type;
  std::filesystem::path Path;

  Texture(unsigned int id, TextureType type, const std::filesystem::path &path)
      : ID(id), Type(type), Path(path) {}
  Texture() noexcept : ID(0), Type(TextureType::unknown) {}

  friend bool operator==(const Texture &t1, const Texture &t2) {
    return t1.Path == t2.Path;
  }

  bool IsValid() const { return ID != 0; }
};

// Is supposed to be used in parallel
struct LoadingTexture {
  using Future = std::future<LoadingTexture *>;

  unsigned char *Data;

  int Width;
  int Height;
  unsigned int Format;
  unsigned int InternalFormat;

  std::filesystem::path Path;
  TextureType Type;

  LoadingTexture(const std::filesystem::path &fname,
                 TextureType type = TextureType::unknown);

  LoadingTexture(LoadingTexture &&t) noexcept
      : Data(std::move(t.Data)), Width(t.Width), Height(t.Height),
        Format(t.Format), InternalFormat(t.InternalFormat),
        Path(std::move(t.Path)), Type(t.Type) {}

  DELETE_COPY_CONSTRUCTOR(LoadingTexture)

  ~LoadingTexture();

  unsigned char *read() const {
    if (Data)
      return Data;
    throw "Data is a nullptr!";
  }

  Texture Finish(TextureType type = TextureType::unknown,
                 bool deleteAfter = true);
};

class LoadingTextures {
private:
  std::vector<std::future<LoadingTexture *>> Futures;
  std::vector<Texture> Textures;

public:
  explicit LoadingTextures(std::vector<std::string> paths);

  bool Is_Finished();

  // Uses openGL, so only do on the main thread
  std::vector<Texture> GetTextures(TextureType type = TextureType::unknown);

  Texture GenerateCubeMap();
};

std::future<LoadingTexture *>
StartLoadingTexture(const std::filesystem::path &path,
                    TextureType type = TextureType::unknown);

template <typename OStream>
inline OStream &operator<<(OStream &output, Texture const &texture) {
  output << texture.Path << '\n';
  output << static_cast<int>(texture.Type) << '\n';

  return output;
}
