#include "pch.h"
#include "Shader.h"

#include <DebugBreak.h>
#include "Log.h"

#include <glad/glad.h>

static std::unordered_map<std::string, GLuint> s_ShaderCache;

static unsigned int s_ShaderInUse = 0;

static unsigned int GetShader(const std::string& filename, GLenum type) {
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
	}
	catch (std::ifstream::failure e) {
		NG_ERROR("Couldn't read file {}", filename);
	}

	const std::string ssrc = Source.str();

	const char* src = ssrc.c_str();
	file.close();

	// create the Shader, set the source, then compile it
	const GLuint Shader = glCreateShader(type);
	glShaderSource(Shader, 1, &src, nullptr);
	glCompileShader(Shader);

	// check if it went well
	int succes;
	glGetShaderiv(Shader, GL_COMPILE_STATUS, &succes);

	if (!succes) {
		char infolog[512];
		glGetShaderInfoLog(Shader, 512, nullptr, &infolog[0]);
		NG_ERROR("Shader {} compilation failed: {}", filename, &infolog[0]);
	}

	s_ShaderCache[filename] = Shader;

	return Shader;
}


GLint Shader::GetUniformLocation(std::string_view name) const
{
	std::string name_s = name.data();
	if (UniformLocCache.contains(name_s))
		return UniformLocCache[name.data()];

	else {
		GLint loc = glGetUniformLocation(ID, name.data());
		UniformLocCache[name.data()] = loc;
		if (loc == -1)
			NG_WARN("Uniform {} doesn't exist!", name.data());

		return loc;
	}
}

Shader::Shader(std::string_view vertexPath, std::string_view fragmentPath, std::string_view geometryPath /* = ""*/)
{
	const GLuint VertexShader = GetShader(std::string(vertexPath), GL_VERTEX_SHADER);
	const GLuint FragmentShader = GetShader(std::string(fragmentPath), GL_FRAGMENT_SHADER);
	const GLuint GeometryShader = GetShader(std::string(geometryPath), GL_GEOMETRY_SHADER);

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
		NG_ERROR("Shader {} (with shaders {} and {}) linking failed: {}. Error raised is {}", ID, vertexPath, fragmentPath, &infolog[0], error);
	}
}

Shader::Shader(std::string_view computePath)
{
	const GLuint ComputeShader = GetShader(std::string(computePath), GL_COMPUTE_SHADER);

	ID = glCreateProgram();

	glAttachShader(ID, ComputeShader);

	glLinkProgram(ID);

	GLint succes;

	glGetProgramiv(ID, GL_LINK_STATUS, &succes);

	if (!succes) {
		char infolog[512];
		const GLenum error = glGetError();

		glGetProgramInfoLog(ID, 512, NULL, &infolog[0]);

		NG_ERROR("Compute shader {} (with shader {}) linking failed: {}. Error raised is {}", ID, computePath, &infolog[0], error);
	}
}

// use/activate the shader
void Shader::Use() noexcept {
	if (s_ShaderInUse != ID) {
		s_ShaderInUse = ID;
		glUseProgram(ID);
	}
}

void Shader::ClearShaderCache()
{
	for (auto& [file, id] : s_ShaderCache)
		glDeleteShader(id);

	s_ShaderCache.clear();
}