#pragma once
#include "Texture.h"
#include "Shader.h"
#include <glm/vec3.hpp>

class Material
{
	Shader* m_Shader;
	std::vector<Texture> m_Textures;
	std::vector<std::shared_future<LoadingTexture*>> m_LoadingTextures;
	glm::vec3 m_Diffuse{ 0.0f };
	glm::vec3 m_Specular{ 0.0f };
	bool m_UseTextures;

	bool m_OpenGLPrepared;

public:
	Material(Shader* shader, const std::vector<Texture>& texs) noexcept;
	Material(Shader* shader, std::vector<Texture>&& texs) noexcept;
	Material(Shader* shader, std::vector<std::shared_future<LoadingTexture*>>&& loadingTexs) noexcept;
	Material(Shader* shader, const glm::vec3& diff, const glm::vec3& spec) noexcept;

	Material(const Material& other);

	void SetColors(const glm::vec3& diff, const glm::vec3& spec);
	void LoadTextures(bool deleteData);
	void Load(bool setTextures);
	void Load(Shader& shader, bool setTextures);
	~Material() noexcept;
};
