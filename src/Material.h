#pragma once
#include "Texture.h"
#include "Shader.h"
#include <glm/vec3.hpp>

class Material
{
	Shader* shader;
	std::vector<Texture> textures;
	std::vector<std::shared_future<LoadingTexture*>> loadingTextures;
	glm::vec3 diffColor{ 0.0f };
	glm::vec3 specColor{ 0.0f };
	bool useTextures;

	bool OpenGLPrepared;

public:
	Material(Shader* shader, const std::vector<Texture>& texs) noexcept;
	Material(Shader* shader, std::vector<Texture>&& texs) noexcept;
	Material(Shader* shader, std::vector<std::shared_future<LoadingTexture*>>&& loadingTexs) noexcept;
	Material(Shader* shader, const glm::vec3& diff, const glm::vec3& spec) noexcept;

	void SetColors(const glm::vec3& diff, const glm::vec3& spec);
	void LoadTextures(bool deleteData);
	void Load(bool setTextures);
	void Load(Shader& shader, bool setTextures);
	~Material() noexcept;
};
