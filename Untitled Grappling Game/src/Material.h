#pragma once
#include "Texture.h"
#include "Shader.h"

class Material
{
	std::shared_ptr<Shader> m_Shader = nullptr;
	union {
		struct {
			std::vector<Texture> m_Textures;
			std::vector<std::shared_future<LoadingTexture*>> m_LoadingTextures;
		};
		struct {
			glm::vec3 m_Diffuse;
			glm::vec3 m_Specular;
		};
	};

	bool m_UseTextures = false;

	bool m_OpenGLPrepared = true;

public:
	Material() : m_Diffuse{ 0.0f, 0.0f, 0.0f }, m_Specular{ 0.0f, 0.0f, 0.0f } {}

	Material(std::shared_ptr<Shader> shader, const std::vector<Texture>& texs) noexcept;
	Material(std::shared_ptr<Shader> shader, std::vector<Texture>&& texs) noexcept;
	Material(std::shared_ptr<Shader> shader, std::vector<std::shared_future<LoadingTexture*>>&& loadingTexs) noexcept;
	Material(std::shared_ptr<Shader> shader, const glm::vec3& diff, const glm::vec3& spec) noexcept;

	Material(const Material& other);

	Material(Material&& other) noexcept;

	void SetColors(const glm::vec3& diff, const glm::vec3& spec);
	void LoadTextures(bool deleteData);
	void Load() { Load(*m_Shader); }
	void Load(Shader& shader);

	Shader* GetShader() { return m_Shader.get(); };

	template<typename OStream>
	void Serialize(OStream& output);

	template<typename IStream>
	static std::stringstream GetDataFromFile(IStream& input);

	template<typename IStream>
	static Material Deserialize(IStream& input, std::shared_ptr<Shader> shader);

	~Material() noexcept;
};

#define LoadMaterial(material, setTextures, ...) do{ \
	if (setTextures)\
		(material)->Load(__VA_ARGS__); \
	else {\
		(material)->GetShader()->Use(); \
		(material)->GetShader()->SetBool("material.useTex", false);\
	}} while(false)

template<typename OStream>
inline OStream& operator<<(OStream& output, glm::vec3 const& input) {
	output << input[0] << ' ';
	output << input[1] << ' ';
	output << input[2] << '\n';

	return output;
}

template<typename IStream>
inline IStream& operator>>(IStream& input, glm::vec3& output) {
	input >> output[0];
	input >> output[1];
	input >> output[2];

	return input;
}

template<typename OStream>
inline OStream& operator<<(OStream& output, glm::quat const& input) {
	output << input[0] << ' '
		<< input[1] << ' '
		<< input[2] << ' '
		<< input[3] << '\n';

	return output;
}

template<typename IStream>
inline IStream& operator>>(IStream& input, glm::quat& output) {
	input >> output[0]
		>> output[1]
		>> output[2]
		>> output[3];

	return input;
}

template<typename OStream, class T>
inline OStream& operator<<(OStream& output, std::vector<T> const& input) {
	uint32_t size = (uint32_t)input.size();

	output << size << '\n';

	for (auto& i : input) {
		output << i << '\n';
	}

	return output;
}

template<typename IStream, class T>
inline IStream& operator>>(IStream& input, std::vector<T>& output) {
	uint32_t size;

	input >> size;
	output.resize(size);
	for (uint32_t i = 0; i < size; i++) {
		input >> output[i];
	}

	return input;
}

template<typename OStream>
void Material::Serialize(OStream& output)
{
	if (!m_OpenGLPrepared)
		throw std::runtime_error("Trying to serialize an unprepared material!");
	output << m_UseTextures << '\n';
	if (m_UseTextures)
		output << m_Textures;
	else
		output << m_Diffuse << m_Specular;
}

template<typename IStream>
inline std::stringstream Material::GetDataFromFile(IStream& input)
{
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
	}
	else {
		glm::vec3 vec;
		input >> vec;
		s << vec;
		input >> vec;
		s << vec;
	}

	return s;
}

template<typename IStream>
inline Material Material::Deserialize(IStream& input, std::shared_ptr<Shader> shader)
{
	Material m;
	m.m_Shader = std::move(shader);
	input >> m.m_UseTextures;
	if (m.m_UseTextures) {
		std::string path;
		std::getline(input, path);
		TextureType t;
		input >> (int&)t;
		m.m_LoadingTextures.push_back(StartLoadingTexture(path, t));
		m.m_OpenGLPrepared = false;
	}
	else {
		input >> m.m_Diffuse >> m.m_Specular;
	}

	return m;
}
