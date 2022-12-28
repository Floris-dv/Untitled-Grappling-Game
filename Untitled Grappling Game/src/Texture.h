#pragma once
#include "Settings.h"

enum class TextureType {
	unknown = -1,
	diffuse = 0,
	specular,
	normal,
	height,
	count
};

static constexpr const char* const names[(int)TextureType::count] = {
	"material.diffuse",
	"material.specular",
	"material.normal",
	"material.height"
};

struct Texture {
	unsigned int ID;
	TextureType Type;
	std::string Path;

	Texture(unsigned int id, TextureType type, const std::string& path) : ID(id), Type(type), Path(path) {}
	Texture() noexcept : ID(0), Type(TextureType::unknown) {}

	friend bool operator==(const Texture& t1, const Texture& t2) {
		return t1.Path == t2.Path;
	}

	bool IsValid() const { return ID != 0; }
};

// Is supposed to be used in parallel
struct LoadingTexture {
	unsigned char* Data;

	int Width;
	int Height;
	int Format;
	int InternalFormat;

	std::string Path;
	TextureType Type;

	LoadingTexture(const std::string& fname, TextureType type = TextureType::unknown);

	LoadingTexture(LoadingTexture&& t) noexcept : Data(std::move(t.Data)), Width(t.Width), Height(t.Height), Format(t.Format), InternalFormat(t.InternalFormat), Path(std::move(t.Path)), Type(t.Type) {}

	~LoadingTexture();

	unsigned char* read() const {
		if (Data)
			return Data;
		throw "Data is a nullptr!";
	}

	Texture Finish(TextureType type = TextureType::unknown, bool deleteAfter = true);
};

class LoadingTextures {
private:
	std::vector<std::future<LoadingTexture*>> Futures;
	std::vector<Texture> Textures;
	unsigned int Size;

public:
	explicit LoadingTextures(std::vector<std::string> paths);

	bool Is_Finished();

	// Uses openGL, so only do on the main thread
	std::vector<Texture> GetTextures(TextureType type = TextureType::unknown);
};

std::future<LoadingTexture*> StartLoadingTexture(const std::string& path, TextureType type = TextureType::unknown);

template<typename OStream>
inline OStream& operator<<(OStream& output, Texture const& texture) {
	output << texture.Path << '\n';
	output << (int)texture.Type << '\n';

	return output;
}