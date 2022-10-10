#include "pch.h"
#include "Texture.h"
#include "Timer.h"

#include <glad/glad.h>

#include <stb_image.h>

#include <DebugBreak.h>

using namespace std::literals::chrono_literals;

std::unordered_map<std::string, Texture> loadedPaths;

std::mutex loadedPathsMtx;

LoadingTexture::LoadingTexture(const std::string& fname, TextureType type) : Path(fname), Format(0), InternalFormat(0), Type(type) {
	if (loadedPaths.contains(Path)) {
		Height = -1;
		Width = -1;
		Data = nullptr;
		return;
	}
	{
		std::scoped_lock lg{ loadedPathsMtx };
		loadedPaths[Path] = Texture();
	}

	int channels;

	unsigned char* temp = stbi_load(Path.c_str(), &Width, &Height, &channels, 0);
	switch (channels) {
	case 1:
		Format = GL_RED;
		InternalFormat = GL_R8;
		break;
	case 3:
		Format = GL_RGB;
		InternalFormat = GL_RGB8;
		break;
	case 4:
		Format = GL_RGBA;
		InternalFormat = GL_RGBA8;
		break;
	default:
		NG_ERROR("Texture in file {} is weird: it has {} amount of channels, instead of the normal 1, 3, or 4", fname, channels);
		break;
	}

	Data = temp;
}
LoadingTexture::~LoadingTexture() {
	stbi_image_free(Data);
}

Texture LoadingTexture::Finish(TextureType FillInType, bool deleteAfter) {
	if (Data == nullptr) {
		Texture t = loadedPaths[Path];
		t.Type = FillInType == TextureType::unknown ? Type : FillInType;
		return t;
	}

	Timer t("OpenGL loading of " + Path);
	read();

	unsigned int textureID;

	glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

	glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTextureStorage2D(textureID, 1, InternalFormat, Width, Height);
	glTextureSubImage2D(textureID, 0, 0, 0, Width, Height, Format, GL_UNSIGNED_BYTE, Data);

	glGenerateTextureMipmap(textureID);

	// set the texture wrapping/filtering options (on the currently bound texture object)

	if (deleteAfter) {
		stbi_image_free(Data);
		Data = nullptr;
	}

	// no need for mutexes, as this is single-threaded
	loadedPaths[Path] = Texture(textureID, FillInType == TextureType::unknown ? Type : FillInType, Path);
	return loadedPaths[Path];
}

LoadingTextures::LoadingTextures(std::vector<std::string> paths) : Size((unsigned int)paths.size()) {
	Futures.reserve(Size);
	Textures.reserve(Size);
	for (std::string& path : paths) {
		Futures.push_back(std::async(std::launch::async, [](std::string fname) {
			// if it's not heap allocated, the function scope will delete it's data, and then it's useless
			LoadingTexture* t = new LoadingTexture(fname);

			return t;
			}, path)
		);
	}
}

bool LoadingTextures::Is_Finished() {
	for (const auto& f : Futures) {
		if (f.wait_for(10us) != std::future_status::ready)
			return false;
	}
	return true;
}

std::vector<Texture> LoadingTextures::GetTextures(TextureType type) {
	if (Textures.size() == Size)
		return Textures;

	for (auto& f : Futures) {
		Textures.push_back(f.get()->Finish(type));
	}

	return Textures;
}