#include "pch.h"

#include "Log.h"
#include "Shader.h"

#include <DebugBreak.h>

#include <glad/glad.h>

/*
 * Shader Format:
 * Vertex in:
 *	0 Pos (vec3)
 *	1 Normal (vec3)
 *	2 TexCoords (vec2)
 *	3 Tangent (vec3)
 *	4 Bitangent (vec3) (not used)
 *	5-8 InstanceMatrix (mat4) (for instanced rendering)
 * Fragment in is with interface blocks. For full-screen passes 0 is used for
 *texCoords Uniforms: Uniform Buffer binding: 0 Matrices (for now VP matrix &
 *viewPos) 1 Lights Uniforms: 0 Model 1 Vertex miscelanious 2 Material 10-
 *Fragment miscelanious
 */

static std::unordered_map<std::string, GLuint> s_ShaderCache;
static std::unordered_map<const uint32_t *, GLuint> s_SPIRShaderCache;

static unsigned int s_ShaderInUse = 0;

static GLuint GetShader(const std::string &filename, GLenum type) {
  if (filename.empty())
    return 0;

  if (s_ShaderCache.contains(filename))
    return s_ShaderCache[filename];

  // Read the file
  std::ifstream file(filename);

  if (!file) {
    NG_ERROR("File {} doesn't exist", filename);
    return 0;
  }

  std::stringstream Source;

  try {
    Source << file.rdbuf();
  } catch (std::ifstream::failure) {
    NG_ERROR("Couldn't read file {}", filename);
  }

  const std::string ssrc = Source.str();

  const char *src = ssrc.c_str();
  file.close();

  // create the Shader, set the source, then compile it
  const GLuint returnShader = glCreateShader(type);
  glShaderSource(returnShader, 1, &src, nullptr);
  glCompileShader(returnShader);

  // check if it went well
  int succes;
  glGetShaderiv(returnShader, GL_COMPILE_STATUS, &succes);

  if (!succes) {
    char infolog[512];
    glGetShaderInfoLog(returnShader, 512, nullptr, &infolog[0]);
    NG_ERROR("Shader {} compilation failed: {}", filename, &infolog[0]);
  }

  s_ShaderCache[filename] = returnShader;

  return returnShader;
}

static GLuint GetShader(const uint32_t *shaderCode, GLsizei size, GLenum type) {
  if (s_SPIRShaderCache.contains(shaderCode))
    return s_SPIRShaderCache[shaderCode];

  GLuint shader = glCreateShader(type);
  glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V, shaderCode, size);
  glSpecializeShader(shader, "main", 0, nullptr, nullptr);

  // check if it went well
  int succes;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &succes);

  if (!succes) {
    char infolog[512];
    glGetShaderInfoLog(shader, 512, nullptr, &infolog[0]);
    NG_ERROR("Shader {} compilation failed: {}", type, &infolog[0]);
  }

  s_SPIRShaderCache[shaderCode] = shader;

  return shader;
}

GLint Shader::GetUniformLocation(std::string_view name) const {
  std::string name_s(name);
  if (!m_UniformLocationCache.empty() &&
      m_UniformLocationCache.find(name_s) != m_UniformLocationCache.end())
    return m_UniformLocationCache[name_s];

  GLint loc = glGetUniformLocation(ID, name.data());
  m_UniformLocationCache[name_s] = loc;
  if (loc == -1)
    NG_WARN("Uniform {} doesn't exist!", name.data());

  return loc;
}

Shader::Shader(std::string_view vertexPath, std::string_view fragmentPath,
               std::string_view geometryPath /* = ""*/) {
  const GLuint VertexShader =
      GetShader(std::string(vertexPath), GL_VERTEX_SHADER);
  const GLuint FragmentShader =
      GetShader(std::string(fragmentPath), GL_FRAGMENT_SHADER);
  const GLuint GeometryShader =
      GetShader(std::string(geometryPath), GL_GEOMETRY_SHADER);

  ID = glCreateProgram();

  glAttachShader(ID, VertexShader);
  glAttachShader(ID, FragmentShader);

  if (GeometryShader)
    glAttachShader(ID, GeometryShader);

  glLinkProgram(ID);

  GLint succes;
  glGetProgramiv(ID, GL_LINK_STATUS, &succes);

  if (!succes) {
    char infolog[512];
    const GLenum error = glGetError();

    glGetProgramInfoLog(ID, 512, NULL, &infolog[0]);
    NG_ERROR("Shader {} (with shaders {} and {}) linking failed: {}. Error "
             "raised is {}",
             ID, vertexPath, fragmentPath, &infolog[0], error);
  }
}

Shader::Shader(const uint32_t *vertex, GLsizei vSize, const uint32_t *fragment,
               GLsizei fSize) {
  const GLuint VertexShader = GetShader(vertex, vSize, GL_VERTEX_SHADER);
  const GLuint FragmentShader = GetShader(fragment, fSize, GL_FRAGMENT_SHADER);

  ID = glCreateProgram();

  glAttachShader(ID, VertexShader);
  glAttachShader(ID, FragmentShader);

  glLinkProgram(ID);

  GLint succes;
  glGetProgramiv(ID, GL_LINK_STATUS, &succes);

  if (!succes) {
    char infolog[512];
    const GLenum error = glGetError();

    glGetProgramInfoLog(ID, 512, NULL, &infolog[0]);
    NG_ERROR("Shader {} (with shaders {} and {}) linking failed: {}. Error "
             "raised is {}",
             ID, VertexShader, FragmentShader, &infolog[0], error);
  }
}

Shader::Shader(std::string_view computePath) {
  const GLuint ComputeShader =
      GetShader(std::string(computePath), GL_COMPUTE_SHADER);

  ID = glCreateProgram();

  glAttachShader(ID, ComputeShader);

  glLinkProgram(ID);

  GLint succes;

  glGetProgramiv(ID, GL_LINK_STATUS, &succes);

  if (!succes) {
    char infolog[512];
    const GLenum error = glGetError();

    glGetProgramInfoLog(ID, 512, NULL, &infolog[0]);

    NG_ERROR("Compute shader {} (with shader {}) linking failed: {}. Error "
             "raised is {}",
             ID, computePath, &infolog[0], error);
  }
}

Shader::Shader(uint32_t *compute, GLsizei cSize) {
  const GLuint ComputeShader = GetShader(compute, cSize, GL_COMPUTE_SHADER);

  ID = glCreateProgram();
  glAttachShader(ID, ComputeShader);

  glLinkProgram(ID);

  GLint succes;
  glGetProgramiv(ID, GL_LINK_STATUS, &succes);

  if (!succes) {
    char infolog[512];
    const GLenum error = glGetError();

    glGetProgramInfoLog(ID, 512, NULL, &infolog[0]);
    NG_ERROR("Compute Shader {} (with shader {}) linking failed: {}. Error "
             "raised is {}",
             ID, ComputeShader, &infolog[0], error);
  }
}

// use/activate the shader
void Shader::Use() noexcept {
  if (s_ShaderInUse != ID) {
    s_ShaderInUse = ID;
    glUseProgram(ID);
  }
}

void Shader::ClearShaderCache() {
  for (auto &[file, id] : s_ShaderCache)
    glDeleteShader(id);

  s_ShaderCache.clear();
}
