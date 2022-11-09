#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <glad/glad.h>

class Shader
{
private:
	mutable std::unordered_map<std::string, GLint> UniformLocCache;
	GLint GetUniformLocation(std::string_view name) const;

public:
	// the program ID
	GLuint ID;

	// constructor reads and builds the shader
	Shader(std::string_view vertexPath, std::string_view fragmentPath, std::string_view geometryPath = "");

	template<size_t vSize, size_t fSize>
	Shader(const uint32_t vertex[vSize], const uint32_t fragment[fSize]) : Shader(vertex, vSize, fragment, fSize) {}

	Shader(const uint32_t* vertex, size_t vSize, const uint32_t* fragment, size_t fSize);

	Shader(std::string_view computePath);

	~Shader() {
		glDeleteProgram(ID);
	}

	void operator=(Shader&& other) noexcept {
		this->~Shader();
		ID = other.ID;
		other.ID = 0;
	}

	void operator=(Shader& other) = delete;

	Shader(const Shader& other) = delete;

	Shader(Shader&& other) noexcept : ID(other.ID) {
		other.ID = 0;
	}

	// use/activate the shader
	void Use() noexcept;

	// utility uniform functions, just thin wrappers
	void SetBool(std::string_view name, bool value) {
		glUniform1i(GetUniformLocation(name), (int)value);
	}
	void SetInt(std::string_view name, int value) {
		glUniform1i(GetUniformLocation(name), value);
	}
	void SetFloat(std::string_view name, float value) {
		glUniform1f(GetUniformLocation(name), value);
	}
	void SetVec2(std::string_view name, const glm::vec2& value) {
		glUniform2fv(GetUniformLocation(name), 1, glm::value_ptr(value));
	}
	void SetVec2(std::string_view name, float v1, float v2) {
		glUniform2f(GetUniformLocation(name), v1, v2);
	}
	void SetVec3(std::string_view name, const glm::vec3& value) {
		glUniform3fv(GetUniformLocation(name), 1, glm::value_ptr(value));
	}
	void SetVec3(std::string_view name, float v1, float v2, float v3) {
		glUniform3f(GetUniformLocation(name), v1, v2, v3);
	}
	void SetVec4(std::string_view name, const glm::vec4& value) {
		glUniform4fv(GetUniformLocation(name), 1, glm::value_ptr(value));
	}
	void SetVec4(std::string_view name, float v1, float v2, float v3, float v4) {
		glUniform4f(GetUniformLocation(name), v1, v2, v3, v4);
	}
	void SetMat4(std::string_view name, const glm::mat4& value) {
		glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
	}
	void SetBlock(std::string_view name, int binding_point) {
		GLuint blockID = glGetUniformBlockIndex(ID, name.data());
		glUniformBlockBinding(ID, blockID, binding_point);
	}

	static void ClearShaderCache();
};