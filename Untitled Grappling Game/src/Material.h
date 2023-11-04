#ifndef MATERIAL_H
#define MATERIAL_H
#include "Shader.h"
#include "Texture.h"
#include "UtilityMacros.h"

// Material without a shader attached
class EmptyMaterial {
protected:
  union {
    std::vector<Texture> m_Textures;
    glm::vec3 m_Diffuse;
  };
  union {
    std::vector<std::shared_future<LoadingTexture *>> m_LoadingTextures;
    glm::vec3 m_Specular;
  };

  bool m_UseTextures = false;

  bool m_OpenGLPrepared = true;

public:
#pragma warning(disable : 4582) // constructor is not implicitly called: that's
                                // what we want
  EmptyMaterial() : m_Diffuse{0.0f, 0.0f, 0.0f}, m_Specular{0.0f, 0.0f, 0.0f} {}
#pragma warning(default : 4582)
  EmptyMaterial(const std::vector<Texture> &texs) noexcept;
  EmptyMaterial(std::vector<Texture> &&texs) noexcept;
  EmptyMaterial(
      std::vector<std::shared_future<LoadingTexture *>> &&loadingTexs) noexcept;
  EmptyMaterial(const glm::vec3 &diff, const glm::vec3 &spec) noexcept;

  EmptyMaterial(EmptyMaterial &&other) noexcept;

  void swap(EmptyMaterial &other) noexcept;

  OVERLOAD_OPERATOR_RVALUE(EmptyMaterial);

  DELETE_COPY_CONSTRUCTOR(EmptyMaterial)

  glm::vec3 GetDiff() const { return m_Diffuse; }
  glm::vec3 GetSpec() const { return m_Specular; }

  void SetColors(const glm::vec3 &diff, const glm::vec3 &spec);
  void LoadTextures(bool deleteData);
  void Load(Shader &shader);

  template <typename OStream> void Serialize(OStream &output);

  template <typename IStream>
  static std::stringstream GetDataFromFile(IStream &input);

  template <typename IStream>
    requires std::is_base_of_v<std::istream, IStream>
  explicit EmptyMaterial(IStream &input);

  ~EmptyMaterial() noexcept;
};

OVERLOAD_STD_SWAP(EmptyMaterial);

// TODO: overhaul this so it's non-owning (doesn't 'own' the shared_ptr)
class Material : public EmptyMaterial {
  std::shared_ptr<Shader> m_Shader = nullptr;

public:
  Material() noexcept {}

  Material(std::shared_ptr<Shader> shader,
           const std::vector<Texture> &texs) noexcept
      : EmptyMaterial(texs), m_Shader(std::move(shader)) {}
  Material(std::shared_ptr<Shader> shader, std::vector<Texture> &&texs)
      : EmptyMaterial(std::move(texs)), m_Shader(std::move(shader)) {}
  Material(
      std::shared_ptr<Shader> shader,
      std::vector<std::shared_future<LoadingTexture *>> &&loadingTexs) noexcept
      : EmptyMaterial(std::move(loadingTexs)), m_Shader(std::move(shader)) {}
  Material(std::shared_ptr<Shader> shader, const glm::vec3 &diff,
           const glm::vec3 &spec) noexcept
      : EmptyMaterial(diff, spec), m_Shader(std::move(shader)) {}

  Material(std::shared_ptr<Shader> shader, EmptyMaterial &&material)
      : EmptyMaterial(std::forward<EmptyMaterial>(material)),
        m_Shader(std::move(shader)) {}

  Material(std::shared_ptr<Shader> &&shader, EmptyMaterial &&material)
      : EmptyMaterial(std::forward<EmptyMaterial>(material)),
        m_Shader(std::move(shader)) {}

  DELETE_COPY_CONSTRUCTOR(Material)

  void Load() { EmptyMaterial::Load(*m_Shader); }

  void Load(Shader &shader) { EmptyMaterial::Load(shader); }

  Shader *GetShader() { return m_Shader.get(); };

  template <typename IStream>
  Material(IStream &input, std::shared_ptr<Shader> shader)
      : EmptyMaterial(input), m_Shader(shader) {}
};

#define LoadMaterial(material, setTextures, ...)                               \
  do {                                                                         \
    if (setTextures)                                                           \
      (material)->Load(__VA_ARGS__);                                           \
    else {                                                                     \
      (material)->GetShader()->Use();                                          \
      (material)->GetShader()->SetBool("material.useTex", false);              \
    }                                                                          \
  } while (false)

template <typename OStream>
inline OStream &operator<<(OStream &output, glm::vec3 const &input) {
  output << input[0] << ' ';
  output << input[1] << ' ';
  output << input[2] << '\n';

  return output;
}

template <typename IStream>
inline IStream &operator>>(IStream &input, glm::vec3 &output) {
  input >> output[0];
  input >> output[1];
  input >> output[2];

  return input;
}

template <typename OStream>
inline OStream &operator<<(OStream &output, glm::quat const &input) {
  output << input[0] << ' ' << input[1] << ' ' << input[2] << ' ' << input[3]
         << '\n';

  return output;
}

template <typename IStream>
inline IStream &operator>>(IStream &input, glm::quat &output) {
  input >> output[0] >> output[1] >> output[2] >> output[3];

  return input;
}

template <typename OStream, class T>
inline OStream &operator<<(OStream &output, std::vector<T> const &input) {
  uint32_t size = (uint32_t)input.size();

  output << size << '\n';

  for (auto &i : input) {
    output << i << '\n';
  }

  return output;
}

template <typename IStream, class T>
inline IStream &operator>>(IStream &input, std::vector<T> &output) {
  uint32_t size;

  input >> size;
  output.resize(size);
  for (uint32_t i = 0; i < size; i++) {
    input >> output[i];
  }

  return input;
}

template <typename OStream> void EmptyMaterial::Serialize(OStream &output) {
  if (!m_OpenGLPrepared)
    throw std::runtime_error("Trying to serialize an unprepared material!");
  output << m_UseTextures << '\n';
  if (m_UseTextures)
    output << m_Textures;
  else
    output << m_Diffuse << m_Specular;
}

template <typename IStream>
inline std::stringstream EmptyMaterial::GetDataFromFile(IStream &input) {
  std::stringstream s;

  bool useTextures;
  input >> useTextures;
  s << useTextures << '\n';

  if (useTextures) {
    int n;
    input >> n;
    input.ignore(20, '\n');
    s << n << '\n';
    for (int i = 0; i < n; i++) {
      std::string path;
      std::getline(input, path);
      int textureType;
      input >> textureType;
      s << path << '\n' << textureType << '\n';
    }
  } else {
    glm::vec3 vec;
    input >> vec;
    s << vec;
    input >> vec;
    s << vec;
  }

  return s;
}

template <typename IStream>
  requires std::is_base_of_v<std::istream, IStream>
inline EmptyMaterial::EmptyMaterial(IStream &input) {
  input >> m_UseTextures;

  if (m_UseTextures) {
    m_Textures = std::vector<Texture>();
    m_LoadingTextures = decltype(m_LoadingTextures)(); // too long of a name
    std::string path;
    std::getline(input, path);
    TextureType t;
    input >> (int &)t;
    m_LoadingTextures.push_back(StartLoadingTexture(path, t));
    m_OpenGLPrepared = false;
  } else {
    input >> m_Diffuse >> m_Specular;
  }
}
#endif
