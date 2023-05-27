#include "pch.h"
#include "Material.h"
#include <glad/glad.h>
#pragma warning(disable:C26495) // Variable in union is uninitialized

Material::Material(std::shared_ptr<Shader> shader, const std::vector<Texture>& texs) noexcept : m_Textures(texs), m_Shader(std::move(shader))
{}

Material::Material(std::shared_ptr<Shader> shader, std::vector<Texture>&& texs) noexcept : m_Textures(std::move(texs)), m_Shader(std::move(shader)), m_UseTextures(true)
{}

Material::Material(std::shared_ptr<Shader> shader, std::vector<std::shared_future<LoadingTexture*>>&& loadingTexs) noexcept :
	m_Textures(loadingTexs.size()), m_LoadingTextures(std::move(loadingTexs)), m_OpenGLPrepared(false), m_Shader(std::move(shader)), m_UseTextures(true)
{}

Material::Material(std::shared_ptr<Shader> shader, const glm::vec3& diff, const glm::vec3& spec) noexcept :
	m_Diffuse(diff), m_Specular(spec), m_Shader(std::move(shader))
{}

Material::Material(const Material& other) : m_Shader(other.m_Shader), m_UseTextures(other.m_UseTextures), m_OpenGLPrepared(true)
{
	if (other.m_UseTextures) {
		m_Textures = other.m_Textures;
	}
	else {
		m_Diffuse = other.m_Diffuse;
		m_Specular = other.m_Specular;
	}

	if (!other.m_OpenGLPrepared)
		throw std::runtime_error("Can't copy other futures");
}

Material::Material(Material&& other) noexcept
{
#define SWAP(a) std::swap(##a, other.##a)
	SWAP(m_Shader);
	SWAP(m_UseTextures);
	SWAP(m_OpenGLPrepared);

	if (m_UseTextures) {
		SWAP(m_Textures);
		SWAP(m_LoadingTextures);
	}
	else {
		SWAP(m_Diffuse);
		SWAP(m_Specular);
	}
#undef SWAP
}

void Material::SetColors(const glm::vec3& diff, const glm::vec3& spec)
{
	m_Diffuse = diff;
	m_Specular = spec;
}

void Material::LoadTextures(bool deleteData)
{
	if (m_OpenGLPrepared || !m_UseTextures)
		return;

	m_OpenGLPrepared = true;

	m_Textures.reserve(m_LoadingTextures.size());
	for (auto& lt : m_LoadingTextures) {
		auto& s = lt.get();
		m_Textures.push_back(s->Finish());
		if (deleteData)
			delete s;
	}

	if (deleteData)
		m_LoadingTextures.clear();
}

void Material::Load(Shader& shader)
{
	shader.Use();
	shader.SetBool("material.diffspecTex", m_UseTextures);

	if (!m_UseTextures) {
		shader.SetVec3("material.diff0", m_Diffuse);
		shader.SetVec3("material.spec0", m_Specular);

		shader.SetBool("material.normalMapping", false);
		return;
	}

	unsigned int diffuseNr = 0, specularNr = 0, normalNr = 0, heightNr = 0;

	for (int i = 0; i < m_Textures.size(); i++) {
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

		shader.SetInt((names[(int)type] + number).c_str(), i);
		glBindTextureUnit(i, m_Textures[i].ID);
	}

	shader.SetBool("material.normalMapping", (bool)normalNr);
}

Material::~Material() noexcept
{
	if (m_UseTextures) {
		for (auto& t : m_Textures)
			glDeleteTextures(1, &t.ID);
	}
	m_OpenGLPrepared = false;
}

#pragma warning(default:C26495)