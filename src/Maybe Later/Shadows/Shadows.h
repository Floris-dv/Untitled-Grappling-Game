#include "Shader.h"

namespace Shadows {
	inline GLuint FBO;

	inline GLuint dirMap;
	inline GLuint pointMap;

	namespace Shaders {
		inline Shader* dirLight;
		inline Shader* iDirLight; // instanced

		inline Shader* pointLight;
		inline Shader* iPointLight; // instanced

		extern void Setup();
	}

	extern void Setup();
	extern void Delete();

	extern void Bind(GLuint depthMap, Shader* shader);
}