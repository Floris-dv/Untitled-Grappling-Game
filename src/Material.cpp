#include "pch.h"
#include "Material.h"
#include <glad/glad.h>

Material::Material(Shader* shader, const std::vector<Texture>& texs) noexcept : textures(texs), OpenGLPrepared(true), shader(shader), useTextures(true)
{}

Material::Material(Shader* shader, std::vector<Texture>&& texs) noexcept : textures(std::move(texs)), OpenGLPrepared(true), shader(shader), useTextures(true)
{}

Material::Material(Shader* shader, std::vector<std::shared_future<LoadingTexture*>>&& loadingTexs) noexcept : loadingTextures(std::move(loadingTexs)), OpenGLPrepared(false), shader(shader), useTextures(true)
{}

Material::Material(Shader* shader, const glm::vec3& diff, const glm::vec3& spec) noexcept : diffColor(diff), specColor(spec), shader(shader), useTextures(false), OpenGLPrepared(true)
{}

void Material::SetColors(const glm::vec3& diff, const glm::vec3& spec)
{
	diffColor = diff;
	specColor = spec;
}

void Material::LoadTextures(bool deleteData)
{
	if (OpenGLPrepared)
		return;

	OpenGLPrepared = true;
	for (auto& lt : loadingTextures) {
		auto& s = lt.get();
		textures.push_back(s->Finish());
		if (deleteData)
			delete s;
	}

	if (deleteData)
		loadingTextures.clear();
}

void Material::Load(bool setTextures)
{
	shader->Use();
	unsigned int diffuseNr = 0, specularNr = 0, normalNr = 0, heightNr = 0;

	if (useTextures && setTextures) {
		for (int i = 0; i < textures.size(); i++) {
			std::string number;
			TextureType type = textures[i].Type;

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

			shader->SetInt((names[(int)type] + number).c_str(), i);
			glBindTextureUnit(i, textures[i].ID);
		}
	}
	else if (setTextures) {
		shader->SetVec3("material.diff0", diffColor);
		shader->SetVec3("material.spec0", specColor);
	}

	shader->SetBool("material.useTex", setTextures && useTextures);
}

void Material::Load(Shader& shader, bool setTextures)
{
	shader.Use();
	unsigned int diffuseNr = 0, specularNr = 0, normalNr = 0, heightNr = 0;

	if (useTextures && setTextures) {
		for (int i = 0; i < textures.size(); i++) {
			std::string number;
			TextureType type = textures[i].Type;

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
			glBindTextureUnit(i, textures[i].ID);
		}
	}
	else if (setTextures) {
		shader.SetVec3("material.diff0", diffColor);
		shader.SetVec3("material.spec0", specColor);
	}

	shader.SetBool("material.useTex", setTextures && useTextures);
}

Material::~Material() noexcept
{
	for (auto& t : textures)
		glDeleteTextures(1, &t.ID);
}